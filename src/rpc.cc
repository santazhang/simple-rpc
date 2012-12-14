#include <errno.h>
#include <stdio.h>
#include <signal.h>

#include "utils.h"
#include "rpc.h"

using namespace std;

class ServerConnection: public Pollable {
    RpcServer* server_;
    Socket* sock_;
    Buffer in_, out_;
    int ref_;
    pthread_mutex_t ref_m_;
    pthread_mutex_t packet_m_;

    void incr_ref() {
        pthread_mutex_lock(&ref_m_);
        ref_++;
        pthread_mutex_unlock(&ref_m_);
    }

    void decr_ref() {
        pthread_mutex_lock(&ref_m_);
        ref_--;
        pthread_mutex_unlock(&ref_m_);
        if (ref_ == 0) {
            delete this;
        }
    }

    // The ServerConnection is ref_counted, it manages its own life cycle. We don't want to
    // allocate it on stack, otherwise the destructor will be called whatever the ref_counter is
    // So we want to limit ServerConnection to be created on heap. This is dont by setting
    // constructor to private, and use a static class method to create ServerConnection on heap.
    ServerConnection(RpcServer* server, Socket* sock): server_(server), sock_(sock), ref_(0) {
        VERIFY(sock_ != NULL);
        pthread_mutex_init(&ref_m_, NULL);
        pthread_mutex_init(&packet_m_, NULL);
    }

public:

    static ServerConnection* create(RpcServer* server, Socket* sock) {
        return new ServerConnection(server, sock);
    }

    ~ServerConnection() {
        pthread_mutex_destroy(&ref_m_);
        pthread_mutex_destroy(&packet_m_);
        delete sock_;
    }

    class RefCopy: public Pollable {
        ServerConnection* c_;
    public:
        RefCopy(ServerConnection* c): c_(c) {
            c_->incr_ref();
        }
        ~RefCopy() {
            c_->decr_ref();
        }
        void begin_packet() {
            c_->begin_packet();
        }
        void end_packet() {
            c_->end_packet();
        }
        void handle_read() {
            c_->handle_read();
        }
        void handle_write() {
            c_->handle_write();
        }
        int fd() {
            return c_->fd();
        }
        int poll_mode() {
            return c_->poll_mode();
        }
        void write(const void* data, int n) {
            c_->write(data, n);
        }
        void write(Marshal& m) {
            c_->write(m);
        }
    };

    // We need a ref_counted copy to prevent early destroy of the ServerConnection.
    // Otherwise when thread pool finishes executing request, it may face a broken ServerConnection.
    RefCopy* ref_copy() {
        return new RefCopy(this);
    }

    void begin_packet() {
        pthread_mutex_lock(&packet_m_);
    }

    void end_packet() {
        pthread_mutex_unlock(&packet_m_);
    }

    void handle_read() {
        for (;;) {
            const int buf_size = 1024;
            char buf[buf_size];

            errno = 0;
            int cnt = sock_->read(buf, buf_size);
            if (cnt == -1 || errno == EAGAIN) {
                break;
            }
            if (cnt == 0) {
                Log::info("client closed connection: fd=%d", fd());

                // need to immediately return after remove call, since
                // this object might be destroyed by remove.
                server_->poll_->remove(fd());
                return;
            }
            in_.write(buf, cnt);
        }

        for (;;) {
            i32 packet_size;
            if (in_.peek(&packet_size, sizeof(i32)) != sizeof(i32)) {
                return;
            }
            if ((size_t) in_.size() < packet_size + sizeof(i32)) {
                return;
            }
            in_.discard(sizeof(32));

            Marshal* rep = new Marshal;
            Marshal* req = new Marshal;
            const int buf_size = 1024;
            char buf[buf_size];
            int bytes_left = packet_size;
            while (bytes_left > 0) {
                int n = in_.consume(buf, ::min(bytes_left, buf_size));
                bytes_left -= n;
                req->write_bytes(buf, n);
            }

            class InvokeJob: public Runnable {
                RpcServer* server_;
                RefCopy* conn_;
                Marshal *req_, *rep_;
            public:
                InvokeJob(RpcServer* server, RefCopy* conn, Marshal* req, Marshal* rep): server_(server), conn_(conn), req_(req), rep_(rep) {}
                ~InvokeJob() {
                    delete req_;
                    delete rep_;
                }

                void run() {
                    server_->invoke(*req_, *rep_);
                    i32 sz = rep_->size();
                    conn_->begin_packet();
                    conn_->write(&sz, sizeof(i32));
                    conn_->write(*rep_);
                    conn_->end_packet();
                    delete conn_;
                }
            };

            server_->thrpool_.run_async(new InvokeJob(server_, ref_copy(), req, rep));
        }
    }

    void handle_write() {
        for (;;) {
            const int buf_size = 1024;
            char buf[buf_size];

            int cnt = out_.peek(buf, buf_size);
            if (cnt == 0) {
                return;
            }
            errno = 0;
            int write_n = sock_->write(buf, cnt);
            if (write_n <= 0 || errno == EAGAIN) {
                break;
            }
            VERIFY(write_n == out_.discard(write_n));
        }
    }

    void write(const void* data, int n) {
        out_.write(data, n);
        server_->poll_->update_poll_mode(fd(), Pollable::READ | Pollable::WRITE);
    }

    void write(Marshal& m) {
        const int buf_size = 1024;
        char buf[buf_size];
        int n;
        while ((n = m.read_bytes(buf, buf_size)) > 0) {
            out_.write(buf, n);
        }
        server_->poll_->update_poll_mode(fd(), Pollable::READ | Pollable::WRITE);
    }

    int fd() {
        return sock_->fd();
    }

    int poll_mode() {
        int r = Pollable::READ;
        int w = (out_.empty()) ? 0 : Pollable::WRITE;
        return r | w;
    }
};


RpcServer::RpcServer(const SmartPtr<PollMgr>& poll /* =? */, Network* net /* =? */): poll_(poll), net_(net), n_invoke_(0) {
    // ignore SIGPIPE, which happens when client dies
    signal(SIGPIPE, SIG_IGN);

    qps_last_.tv_sec = -1;
    qps_last_.tv_usec = -1;
}


i32 RpcServer::invoke(Marshal& req, Marshal& rep) {
    i32 ret;
    i64 xid = req.read_i64();
    rep << xid;
    i32 svc_id = req.read_i32();
    map<i32, Handler*>::iterator it = handlers_.find(svc_id);
    if (it == handlers_.end()) {
        ret = RpcErrno::ESERVICE;
        rep << ret;
    } else {
        // Leave returned application error for handler.
        it->second->handle(req, rep);
    }
    int n_invoke = __sync_add_and_fetch(&n_invoke_, 1); // gcc built-in atomic op
    const int report_interval = 10000;
    if (n_invoke % report_interval == 0) {
        if (qps_last_.tv_sec == -1 && qps_last_.tv_usec == -1) {
            Log::debug("%d invokes handled", n_invoke);
            gettimeofday(&qps_last_, NULL);
        } else {
            timeval now;
            gettimeofday(&now, NULL);
            double qps = report_interval / (now.tv_sec - qps_last_.tv_sec + double(now.tv_usec - qps_last_.tv_usec) / 1000000.0);
            Log::debug("%d invokes handled, QPS is %.2lf", n_invoke, qps);
            gettimeofday(&qps_last_, NULL);
        }
    }
    return ret;
}

void RpcServer::run(const char* addr) {
    Socket* ss = net_->makeSocket();
    VERIFY(ss->bind(addr) == 0);
    for (;;) {
        Socket* s = ss->accept();
        if (s != NULL) {
            s->set(Socket::NONBLOCKING, true);
            Log::info("got new client connection: fd=%d", s->fd());
            poll_->add(ServerConnection::create(this, s)->ref_copy());
        }
    }
}

class ClientConnection: public Pollable {
    Buffer in_, out_;
    RpcClient* clnt_;
    Socket* s_;
    pthread_mutex_t m_;
    pthread_mutex_t packet_m_;
    pthread_cond_t c_;
    map<i64, Marshal*> reply_;

public:

    ClientConnection(RpcClient* clnt, Socket* s): clnt_(clnt), s_(s) {
        pthread_mutex_init(&m_, NULL);
        pthread_mutex_init(&packet_m_, NULL);
        pthread_cond_init(&c_, NULL);
    }

    ~ClientConnection() {
        pthread_mutex_destroy(&m_);
        pthread_mutex_destroy(&packet_m_);
        pthread_cond_destroy(&c_);
        delete s_;
    }

    void begin_packet() {
        pthread_mutex_lock(&packet_m_);
    }

    void end_packet() {
        pthread_mutex_unlock(&packet_m_);
    }

    void handle_write() {
        for (;;) {
            const int buf_size = 1024;
            char buf[buf_size];

            int cnt = out_.peek(buf, buf_size);
            if (cnt == 0) {
                return;
            }
            errno = 0;
            int write_n = s_->write(buf, cnt);
            if (write_n <= 0 || errno == EAGAIN) {
                break;
            }
            VERIFY(write_n == out_.discard(write_n));
        }
    }

    void handle_read() {
        for (;;) {
            const int buf_size = 1024;
            char buf[buf_size];

            errno = 0;
            int cnt = s_->read(buf, buf_size);
            VERIFY(cnt != 0);
            if (cnt == -1 || errno == EAGAIN) {
                break;
            }
            in_.write(buf, cnt);
        }

        for (;;) {
            i32 packet_size;
            if (in_.peek(&packet_size, sizeof(i32)) != sizeof(i32)) {
                return;
            }
            if ((size_t) in_.size() < packet_size + sizeof(i32)) {
                return;
            }
            in_.discard(sizeof(32));

            Marshal* rep = new Marshal;
            const int buf_size = 1024;
            char buf[buf_size];
            int bytes_left = packet_size;
            while (bytes_left > 0) {
                int n = in_.consume(buf, ::min(bytes_left, buf_size));
                bytes_left -= n;
                rep->write_bytes(buf, n);
            }

            pthread_mutex_lock(&m_);
            i64 xid = rep->read_i64();
            reply_[xid] = rep;
            pthread_mutex_unlock(&m_);
            pthread_cond_broadcast(&c_);
        }
    }

    int fd() {
        return s_->fd();
    }

    int poll_mode() {
        int r = Pollable::READ;
        int w = (out_.empty()) ? 0 : Pollable::WRITE;
        return r | w;
    }

    void write(const void* data, int n) {
        out_.write(data, n);
        clnt_->poll_->update_poll_mode(fd(), Pollable::READ | Pollable::WRITE);
    }

    void write(Marshal& m) {
        const int buf_size = 1024;
        char buf[buf_size];
        int n;
        while ((n = m.read_bytes(buf, buf_size)) > 0) {
            out_.write(buf, n);
        }
        clnt_->poll_->update_poll_mode(fd(), Pollable::READ | Pollable::WRITE);
    }

    Marshal* wait_reply(i64 xid) {
        Marshal* rep = NULL;
        for (;;) {
            ScopedLock sl(&m_);
            map<i64, Marshal*>::iterator it = reply_.find(xid);
            if (it != reply_.end()) {
                rep = it->second; // it will be deleted in outer invoke calls
                reply_.erase(it);
                break;
            }
            pthread_cond_wait(&c_, &m_);
        }
        return rep;
    }
};

RpcClient::~RpcClient() {
    disconnect();
}

void RpcClient::connect(const char* addr) {
    Socket* s = net_->makeSocket();
    s->connect(addr);
    s->set(Socket::NONBLOCKING, true);
    conn_ = new ClientConnection(this, s);
    poll_->add(conn_);
}

void RpcClient::disconnect() {
    if (conn_) {
        // the conn_ will be deleted by remove
        poll_->remove(conn_->fd());
        conn_ = NULL;
    }
}

i32 RpcClient::call(i64 xid, Marshal& req, Marshal*& rep) {
    i32 sz = req.size();

    conn_->begin_packet();
    conn_->write(&sz, sizeof(i32));
    conn_->write(req);
    conn_->end_packet();

    rep = conn_->wait_reply(xid);
    // xid has been consumed by wait_reply
    i32 rpc_errno;
    *rep >> rpc_errno;
    return rpc_errno;
}
