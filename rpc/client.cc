#include <string>

#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/tcp.h>

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

Client::Client(PollMgr* pollmgr, ThreadPool* thrpool /* =? */)
        : thrpool_(thrpool), pollmgr_(pollmgr), sock_(-1), status_(NEW), bmark_(NULL) {
    Pthread_mutex_init(&pending_fu_m_, NULL);
    Pthread_mutex_init(&out_m_, NULL);
}

Client::~Client() {
    invalidate_pending_futures();

    Pthread_mutex_destroy(&pending_fu_m_);
    Pthread_mutex_destroy(&out_m_);

    //Log::debug("rpc::Client: destroyed");
}

void Client::invalidate_pending_futures() {
    list<Future*> futures;
    Pthread_mutex_lock(&pending_fu_m_);
    for (map<i64, Future*>::iterator it = pending_fu_.begin(); it != pending_fu_.end(); ++it) {
        futures.push_back(it->second);
    }
    Pthread_mutex_unlock(&pending_fu_m_);

    for (list<Future*>::iterator it = futures.begin(); it != futures.end(); ++it) {
        Pthread_mutex_lock(&pending_fu_m_);
        map<i64, Future*>::iterator find_it = pending_fu_.find((*it)->xid_);
        Future* fu = NULL;
        if (find_it != pending_fu_.end()) {
            fu = find_it->second;
            pending_fu_.erase(find_it);
        }
        Pthread_mutex_unlock(&pending_fu_m_);

        if (fu != NULL) {
            fu->error_code_ = ENOTCONN;

            Pthread_mutex_lock(&fu->ready_m_);
            fu->ready_ = true;
            Pthread_cond_signal(&fu->ready_cond_);
            Pthread_mutex_unlock(&fu->ready_m_);

            RUNNABLE_CLASS1(R, Future*, fu, {
                if (fu->attr_.callback != NULL) {
                    fu->attr_.callback->run(fu);

                    // automatically cleanup the callback
                    delete fu->attr_.callback;
                    fu->attr_.callback = NULL;
                }

                // since we removed it from pending_fu_
                fu->release();
            });

            if (thrpool_ != NULL) {
                thrpool_->run_async(new R(fu));
            } else {
                R r(fu);
                r.run();
            }
        }
    }

}

void Client::close() {
    if (status_ == CONNECTED) {
        pollmgr_->remove(this);
        ::close(sock_);
        status_ = CLOSED;

        invalidate_pending_futures();
    }
    status_ = CLOSED;
}

int Client::connect(const char* addr) {
    string addr_str(addr);
    size_t idx = addr_str.find(":");
    if (idx == string::npos) {
        Log::error("rpc::Client: bad connect address: %s", addr);
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
        verify(setsockopt(sock_, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == 0);
        verify(setsockopt(sock_, IPPROTO_TCP, TCP_NODELAY, &yes, sizeof(yes)) == 0);

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
    out_.write_to_fd(sock_);
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
            Pthread_mutex_unlock(&pending_fu_m_);

            fu->error_code_ = error_code;
            fu->reply_.read_from_marshal(in_, packet_size - sizeof(reply_xid) - sizeof(error_code));

            Pthread_mutex_lock(&fu->ready_m_);
            fu->ready_ = true;
            Pthread_cond_signal(&fu->ready_cond_);
            Pthread_mutex_unlock(&fu->ready_m_);

            if (fu->attr_.callback != NULL) {
                fu->attr_.callback->run(fu);

                // automatically cleanup the callback
                delete fu->attr_.callback;
                fu->attr_.callback = NULL;
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

Future* Client::begin_request(i32 rpc_id, const FutureAttr& attr /* =... */) {
    Pthread_mutex_lock(&out_m_);

    if (status_ != CONNECTED) {
        return NULL;
    }

    Future* fu = new Future(xid_counter_.next(), attr);
    Pthread_mutex_lock(&pending_fu_m_);
    pending_fu_[fu->xid_] = fu;
    pending_fu_.size();
    Pthread_mutex_unlock(&pending_fu_m_);

    // check if the client gets closed in the meantime
    if (status_ != CONNECTED) {
        Pthread_mutex_lock(&pending_fu_m_);
        map<i64, Future*>::iterator it = pending_fu_.find(fu->xid_);
        if (it != pending_fu_.end()) {
            it->second->release();
            pending_fu_.erase(it);
        }
        Pthread_mutex_unlock(&pending_fu_m_);

        return NULL;
    }

    bmark_ = out_.set_bookmark(sizeof(i32)); // will fill packet size later

    *this << fu->xid_;
    *this << rpc_id;

    // one ref is already in pending_fu_
    return (Future *) fu->ref_copy();
}

void Client::end_request() {
    // set reply size in packet
    if (bmark_ != NULL) {
        i32 request_size = out_.get_write_counter_and_reset();
        out_.write_bookmark(bmark_, &request_size);
        delete bmark_;
        bmark_ = NULL;
    }

    if (!out_.empty()) {
        pollmgr_->update_mode(this, Pollable::READ | Pollable::WRITE);
    }

    Pthread_mutex_unlock(&out_m_);
}

ClientPool::ClientPool(PollMgr* pollmgr) {
    Pthread_mutex_init(&m_, NULL);

    if (pollmgr == NULL) {
        pollmgr_ = new PollMgr(1);
    } else {
        pollmgr_ = (PollMgr *) pollmgr->ref_copy();
    }
}

ClientPool::~ClientPool() {
    for (map<string, Client*>::iterator it = cache_.begin(); it != cache_.end(); ++it) {
        it->second->close_and_release();
    }

    pollmgr_->release();

    Pthread_mutex_destroy(&m_);
}

Client* ClientPool::get_client(const string& addr) {
    Client* cl = NULL;
    Pthread_mutex_lock(&m_);
    map<string, Client*>::iterator it = cache_.find(addr);
    if (it != cache_.end()) {
        cl = it->second;
    } else {
        cl = new Client(this->pollmgr_, &this->thrpool_);
        if (cl->connect(addr.c_str()) != 0) {
            // connect failure
            cl->close_and_release();
            cl = NULL;
        } else {
            // connect success
            cache_[addr] = cl;
        }
    }
    Pthread_mutex_unlock(&m_);
    return cl;
}

}
