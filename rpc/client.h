#pragma once

#include <map>

#include "marshal.h"
#include "polling.h"

namespace rpc {

class Future;
class Client;

struct FutureAttr {
    void (*callback)(Future* fu, void* callback_arg);
    void* callback_arg;

    FutureAttr()
            : callback(NULL), callback_arg(NULL) {
    }
};

class Future: public RefCounted {
    friend class Client;

    i64 xid_;
    i32 error_code_;

    FutureAttr attr_;
    Marshal reply_;

    bool ready_;
    pthread_cond_t ready_cond_;
    pthread_mutex_t ready_m_;

    // Private destructor as required by RefCounted.
    ~Future() {
        Pthread_mutex_destroy(&ready_m_);
        Pthread_cond_destroy(&ready_cond_);
    }

public:

    Future(i64 xid, const FutureAttr& attr = FutureAttr())
            : xid_(xid), error_code_(0), attr_(attr), ready_(false) {
        Pthread_mutex_init(&ready_m_, NULL);
        Pthread_cond_init(&ready_cond_, NULL);
    }

    // wait till reply done
    void wait();

    Marshal& get_reply() {
        wait();
        return reply_;
    }

    i32 get_error_code() {
        wait();
        return error_code_;
    }
};

class Client: public Pollable {
    Marshal in_, out_;

    /**
     * NOT a refcopy! This is intend to avoid circular reference, which prevents everything to be released correctly.
     */
    PollMgr* pollmgr_;

    int sock_;
    enum {
        NEW, CONNECTED, CLOSED
    } status_;

    Marshal::Bookmark* bmark_;

    Counter xid_counter_;
    std::map<i64, Future*> pending_fu_;

    pthread_mutex_t pending_fu_m_, out_m_;

    // reentrant, could be called multiple times before releasing
    void close();

    void invalidate_pending_futures();

    // prevent direct usage
    using RefCounted::release;

protected:

    virtual ~Client();

public:

    Client(PollMgr* pollmgr);

    /**
     * Start a new request. Must be paired with end_request(), even if NULL returned.
     *
     * The request packet format is: <size> <xid> <rpc_id> <arg1> <arg2> ... <argN>
     */
    Future* begin_request(const FutureAttr& attr = FutureAttr());

    void end_request();

    template<class T>
    Client& operator <<(const T& v) {
        if (status_ == CONNECTED) {
            this->out_ << v;
        }
        return *this;
    }

    int connect(const char* addr);

    void close_and_release() {
        close();
        release();
    }


    int fd() {
        return sock_;
    }

    int poll_mode();
    void handle_read();
    void handle_write();
    void handle_error();

};

}
