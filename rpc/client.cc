#include <string>

#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include "client.h"

using namespace std;

namespace rpc {

void Future::wait() {
    Pthread_mutex_lock(&ready_m_);
    while (!ready_) {
        Pthread_cond_wait(&ready_cond_, &ready_m_);
    }
    Pthread_mutex_unlock(&ready_m_);
}

Client::Client(PollMgr* pollmgr /* =... */)
        : sock_(-1), status_(NEW), bmark_(NULL), request_size_(-1), pollmgr_(pollmgr) {
    Pthread_mutex_init(&pending_fu_m_, NULL);
    Pthread_mutex_init(&out_m_, NULL);
}

Client::~Client() {
    for (map<i64, Future*>::iterator it = pending_fu_.begin(); it != pending_fu_.end(); ++it) {
        Future* fu = it->second;

        // client closed
        fu->error_code_ = EBADF;
        fu->ready_ = true;
        Pthread_cond_signal(&fu->ready_cond_);

        if (fu->attr_.callback != NULL) {
            fu->attr_.callback(fu, fu->attr_.callback_arg);
        }

        // since we removed it from pending_fu_
        fu->release();
    }

    Pthread_mutex_destroy(&pending_fu_m_);
    Pthread_mutex_destroy(&out_m_);

    //Log::debug("rpc::Client: destroyed");
}

void Client::close() {
    if (status_ == CONNECTED) {
        pollmgr_->remove(this);
        ::close(sock_);
    }
    status_ = CLOSED;
}

int Client::connect(const char* addr) {
    string addr_str(addr);
    int idx = addr_str.find(":");
    if (idx == string::npos) {
        Log::error("rpc::Client: bad bind address: %s", addr);
        errno = EINVAL;
        return -1;
    }
    string host = addr_str.substr(0, idx);
    string port = addr_str.substr(idx + 1);

    struct addrinfo hints, *result, *rp;
    memset(&hints, 0, sizeof(struct addrinfo));

    hints.ai_family = AF_INET; // ipv4
    hints.ai_socktype = SOCK_STREAM; // tcp

    int r = getaddrinfo(host.c_str(), port.c_str(), &hints, &result);
    if (r != 0) {
        Log::error("rpc::Client: getaddrinfo(): %s", gai_strerror(r));
        return -1;
    }

    for (rp = result; rp != NULL; rp = rp->ai_next) {
        sock_ = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sock_ == -1) {
            continue;
        }

        const int yes = 1;
        setsockopt(sock_, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

        if (::connect(sock_, rp->ai_addr, rp->ai_addrlen) == 0) {
            break;
        }
        ::close(sock_);
        sock_ = -1;
    }
    freeaddrinfo(result);

    if (rp == NULL) {
        // failed to connect
        Log::error("rpc::Client: connect(): %s", strerror(errno));
        return -1;
    }

    verify(set_nonblocking(sock_, true) == 0);
    Log::info("rpc::Client: connected to %s", addr);

    status_ = CONNECTED;
    pollmgr_->add(this);

    return 0;
}

void Client::handle_error() {
    close();
}

void Client::handle_write() {
    if (status_ != CONNECTED) {
        return;
    }

    Pthread_mutex_lock(&out_m_);
    int bytes_written = out_.write_to_fd(sock_);
    if (out_.empty()) {
        pollmgr_->update_mode(this, Pollable::READ);
    }
    Pthread_mutex_unlock(&out_m_);
}

void Client::handle_read() {
    if (status_ != CONNECTED) {
        return;
    }

    int bytes_read = in_.read_from_fd(sock_);
    if (bytes_read == 0) {
        return;
    }

    for (;;) {
        i32 packet_size;
        int n_peek = in_.peek(&packet_size, sizeof(i32));
        if (n_peek == sizeof(i32) && in_.content_size_gt(packet_size + sizeof(i32) - 1)) {
            // consume the packet size
            verify(in_.read(&packet_size, sizeof(i32)) == sizeof(i32));

            i64 reply_xid;
            i32 error_code;

            in_ >> reply_xid >> error_code;

            Pthread_mutex_lock(&pending_fu_m_);
            map<i64, Future*>::iterator it = pending_fu_.find(reply_xid);
            verify(it != pending_fu_.end());

            Future* fu = it->second;
            verify(fu->xid_ == reply_xid);
            pending_fu_.erase(it);
            int sz = pending_fu_.size();
            Pthread_mutex_unlock(&pending_fu_m_);

            fu->error_code_ = error_code;
            fu->reply_.read_from_marshal(in_, packet_size - sizeof(reply_xid) - sizeof(error_code));

            Pthread_mutex_lock(&fu->ready_m_);
            fu->ready_ = true;
            Pthread_cond_signal(&fu->ready_cond_);
            Pthread_mutex_unlock(&fu->ready_m_);

            if (fu->attr_.callback != NULL) {
                fu->attr_.callback(fu, fu->attr_.callback_arg);
            }

            // since we removed it from pending_fu_
            fu->release();

        } else {
            // packet incomplete or no more packets to process
            break;
        }
    }
}

int Client::poll_mode() {
    int mode = Pollable::READ;
    Pthread_mutex_lock(&out_m_);
    if (!out_.empty()) {
        mode |= Pollable::WRITE;
    }
    Pthread_mutex_unlock(&out_m_);
    return mode;
}

Future* Client::begin_request(const FutureAttr& attr /* =... */) {
    if (status_ != CONNECTED) {
        return NULL;
    }

    Pthread_mutex_lock(&out_m_);

    Future* fu = new Future(xid_counter_.next(), attr);
    Pthread_mutex_lock(&pending_fu_m_);
    pending_fu_[fu->xid_] = fu;
    int sz = pending_fu_.size();
    Pthread_mutex_unlock(&pending_fu_m_);

    bmark_ = out_.set_bookmark(sizeof(request_size_)); // will fill packet size later
    request_size_ = 0;  // reply size does not include the size field itself.

    *this << fu->xid_;

    // one ref is already in pending_fu_
    return (Future *) fu->ref_copy();
}

void Client::end_request() {
    // set reply size in packet
    if (bmark_ != NULL) {
        out_.write_bookmark(bmark_, &request_size_);
        delete bmark_;
        bmark_ = NULL;
    }
    request_size_ = -1;

    if (!out_.empty()) {
        pollmgr_->update_mode(this, Pollable::READ | Pollable::WRITE);
    }

    Pthread_mutex_unlock(&out_m_);
}

}