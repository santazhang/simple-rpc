#pragma once

#include <unordered_map>
#include <unordered_set>

#include <pthread.h>

#include "marshal.h"
#include "polling.h"

// for getaddrinfo() used in Server::start()
struct addrinfo;

namespace rpc {

class Server;

/**
 * The raw packet sent from client will be like this:
 * <size> <xid> <rpc_id> <arg1> <arg2> ... <argN>
 * NOTE: size does not include the size itself (<xid>..<argN>).
 *
 * For the request object, the marshal only contains <arg1>..<argN>,
 * other fields are already consumed.
 */
struct Request {
    Marshal m;
    i64 xid;
};

class Service {
public:
    virtual ~Service() {}
    virtual int __reg_to__(Server*) = 0;
};

class ServerConnection: public Pollable {

    friend class Server;

    Marshal in_, out_;
    SpinLock out_l_;

    Server* server_;
    int socket_;

    Marshal::bookmark* bmark_;

    enum {
        CONNECTED, CLOSED
    } status_;

    /**
     * Only to be called by:
     * 1: ~Server(), which is called when destroying Server
     * 2: handle_error(), which is called by PollMgr
     */
    void close();

protected:

    // Protected destructor as required by RefCounted.
    ~ServerConnection() {
        //Log_debug("rpc::ServerConnection: destroyed");
    }

public:

    ServerConnection(Server* server, int socket): server_(server), socket_(socket), bmark_(nullptr), status_(CONNECTED) {}

    /**
     * Start a reply message. Must be paired with end_reply().
     *
     * Reply message format:
     * <size> <xid> <error_code> <ret1> <ret2> ... <retN>
     * NOTE: size does not include size itself (<xid>..<retN>).
     *
     * User only need to fill <ret1>..<retN>.
     *
     * Currently used errno:
     * 0: everything is fine
     * ENOENT: method not found
     * EINVAL: invalid packet (field missing)
     */
    void begin_reply(Request* req, i32 error_code = 0);

    void end_reply();

    // helper function, do some work in background
    void run_async(const std::function<void()>& f);

    template<class T>
    ServerConnection& operator <<(const T& v) {
        this->out_ << v;
        return *this;
    }

    ServerConnection& operator <<(Marshal& m) {
        this->out_.read_from_marshal(m, m.content_size());
        return *this;
    }

    int fd() {
        return socket_;
    }

    int poll_mode();
    void handle_write();
    void handle_read();
    void handle_error();
};

class Server: public NoCopy {

    friend class ServerConnection;

    std::unordered_map<i32, std::function<void(Request*, ServerConnection*)>> handlers_;
    PollMgr* pollmgr_;
    ThreadPool* threadpool_;
    int server_sock_;

    SpinLock sconns_l_;
    std::unordered_set<ServerConnection*> sconns_;

    enum {
        NEW, RUNNING, STOPPING, STOPPED
    } status_;

    pthread_t loop_th_;

    static void* start_server_loop(void* arg);
    void server_loop(struct addrinfo* svr_addr);

public:

    Server(PollMgr* pollmgr = nullptr, ThreadPool* thrpool = nullptr);
    virtual ~Server();

    int start(const char* bind_addr);

    int reg(Service* svc) {
        return svc->__reg_to__(this);
    }

    /**
     * The svc_func need to do this:
     *
     *  {
     *     // process request
     *     ..
     *
     *     // send reply
     *     server_connection->begin_reply();
     *     *server_connection << {reply_content};
     *     server_connection->end_reply();
     *
     *     // cleanup resource
     *     delete request;
     *     server_connection->release();
     *  }
     */
    int reg(i32 rpc_id, const std::function<void(Request*, ServerConnection*)>& func);

    template<class S>
    int reg(i32 rpc_id, S* svc, void (S::*svc_func)(Request*, ServerConnection*)) {

        // disallow duplicate rpc_id
        if (handlers_.find(rpc_id) != handlers_.end()) {
            return EEXIST;
        }

        handlers_[rpc_id] = [svc, svc_func] (Request* req, ServerConnection* sconn) {
            (svc->*svc_func)(req, sconn);
        };

        return 0;
    }

    void unreg(i32 rpc_id);
};

}

