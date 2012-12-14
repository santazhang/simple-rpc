// rpc request format: i32(size), marshal(i64(xid), i32(svc_id), args..) (size = xid..args..)
// rpc reply format: i32(size), marshal(i64(xid), i32(rpc_errno), i64(fun_ret_code), fun_result) (size = xid..fun_result)

#pragma once

#include <sys/time.h>
#include <map>

#include "marshal.h"
#include "tcp_network.h"
#include "polling.h"

class RpcErrno {
public:
    static const i32 ESERVICE = -1;
};

class RpcServer: public NoCopy {

    friend class ServerConnection;

    class Handler {
    public:
        virtual void handle(Marshal& req, Marshal& rep) = 0;
    };

    std::map<i32, Handler*> handlers_;
    SmartPtr<PollMgr> poll_;
    Network* net_;
    ThreadPool thrpool_;
    int n_invoke_;
    struct timeval qps_last_;

    i32 invoke(Marshal& req, Marshal& rep);

public:

    RpcServer(const SmartPtr<PollMgr>& poll = new PollMgr, Network* net = new TcpNetwork);

    ~RpcServer() {
        delete net_;
    }

    void run(const char* addr);

// *** do not modify the following lines, they are automatically generated!
// __AUTO_GEN_SERVER__(10)

    template<class S, class R>
    void reg(i32 svc_id, S* svc, i32 (S::*svc_func)(R& r)) {
        class H0: public Handler {
            S* s;
            i32 (S::*f)(R& r);
        public:
            H0(S* s, i32 (S::*f)(R& r)): s(s), f(f) {}
            void handle(Marshal& req, Marshal& rep) {
                R r;
                i32 ret = (s->*f)(r);
                VERIFY(ret >= 0); // Negative error code are reserved for RPC error.
                rep << ret << r;
            }
        };
        handlers_[svc_id] = new H0(svc, svc_func);
    }

    template<class S, class R, class T0>
    void reg(i32 svc_id, S* svc, i32 (S::*svc_func)(const T0& t0, R& r)) {
        class H1: public Handler {
            S* s;
            i32 (S::*f)(const T0&, R& r);
        public:
            H1(S* s, i32 (S::*f)(const T0&, R& r)): s(s), f(f) {}
            void handle(Marshal& req, Marshal& rep) {
                T0 t0;
                req >> t0;
                R r;
                i32 ret = (s->*f)(t0, r);
                VERIFY(ret >= 0); // Negative error code are reserved for RPC error.
                rep << ret << r;
            }
        };
        handlers_[svc_id] = new H1(svc, svc_func);
    }

    template<class S, class R, class T0, class T1>
    void reg(i32 svc_id, S* svc, i32 (S::*svc_func)(const T0& t0, const T1& t1, R& r)) {
        class H2: public Handler {
            S* s;
            i32 (S::*f)(const T0&, const T1&, R& r);
        public:
            H2(S* s, i32 (S::*f)(const T0&, const T1&, R& r)): s(s), f(f) {}
            void handle(Marshal& req, Marshal& rep) {
                T0 t0;
                T1 t1;
                req >> t0 >> t1;
                R r;
                i32 ret = (s->*f)(t0, t1, r);
                VERIFY(ret >= 0); // Negative error code are reserved for RPC error.
                rep << ret << r;
            }
        };
        handlers_[svc_id] = new H2(svc, svc_func);
    }

    template<class S, class R, class T0, class T1, class T2>
    void reg(i32 svc_id, S* svc, i32 (S::*svc_func)(const T0& t0, const T1& t1, const T2& t2, R& r)) {
        class H3: public Handler {
            S* s;
            i32 (S::*f)(const T0&, const T1&, const T2&, R& r);
        public:
            H3(S* s, i32 (S::*f)(const T0&, const T1&, const T2&, R& r)): s(s), f(f) {}
            void handle(Marshal& req, Marshal& rep) {
                T0 t0;
                T1 t1;
                T2 t2;
                req >> t0 >> t1 >> t2;
                R r;
                i32 ret = (s->*f)(t0, t1, t2, r);
                VERIFY(ret >= 0); // Negative error code are reserved for RPC error.
                rep << ret << r;
            }
        };
        handlers_[svc_id] = new H3(svc, svc_func);
    }

    template<class S, class R, class T0, class T1, class T2, class T3>
    void reg(i32 svc_id, S* svc, i32 (S::*svc_func)(const T0& t0, const T1& t1, const T2& t2, const T3& t3, R& r)) {
        class H4: public Handler {
            S* s;
            i32 (S::*f)(const T0&, const T1&, const T2&, const T3&, R& r);
        public:
            H4(S* s, i32 (S::*f)(const T0&, const T1&, const T2&, const T3&, R& r)): s(s), f(f) {}
            void handle(Marshal& req, Marshal& rep) {
                T0 t0;
                T1 t1;
                T2 t2;
                T3 t3;
                req >> t0 >> t1 >> t2 >> t3;
                R r;
                i32 ret = (s->*f)(t0, t1, t2, t3, r);
                VERIFY(ret >= 0); // Negative error code are reserved for RPC error.
                rep << ret << r;
            }
        };
        handlers_[svc_id] = new H4(svc, svc_func);
    }

    template<class S, class R, class T0, class T1, class T2, class T3, class T4>
    void reg(i32 svc_id, S* svc, i32 (S::*svc_func)(const T0& t0, const T1& t1, const T2& t2, const T3& t3, const T4& t4, R& r)) {
        class H5: public Handler {
            S* s;
            i32 (S::*f)(const T0&, const T1&, const T2&, const T3&, const T4&, R& r);
        public:
            H5(S* s, i32 (S::*f)(const T0&, const T1&, const T2&, const T3&, const T4&, R& r)): s(s), f(f) {}
            void handle(Marshal& req, Marshal& rep) {
                T0 t0;
                T1 t1;
                T2 t2;
                T3 t3;
                T4 t4;
                req >> t0 >> t1 >> t2 >> t3 >> t4;
                R r;
                i32 ret = (s->*f)(t0, t1, t2, t3, t4, r);
                VERIFY(ret >= 0); // Negative error code are reserved for RPC error.
                rep << ret << r;
            }
        };
        handlers_[svc_id] = new H5(svc, svc_func);
    }

    template<class S, class R, class T0, class T1, class T2, class T3, class T4, class T5>
    void reg(i32 svc_id, S* svc, i32 (S::*svc_func)(const T0& t0, const T1& t1, const T2& t2, const T3& t3, const T4& t4, const T5& t5, R& r)) {
        class H6: public Handler {
            S* s;
            i32 (S::*f)(const T0&, const T1&, const T2&, const T3&, const T4&, const T5&, R& r);
        public:
            H6(S* s, i32 (S::*f)(const T0&, const T1&, const T2&, const T3&, const T4&, const T5&, R& r)): s(s), f(f) {}
            void handle(Marshal& req, Marshal& rep) {
                T0 t0;
                T1 t1;
                T2 t2;
                T3 t3;
                T4 t4;
                T5 t5;
                req >> t0 >> t1 >> t2 >> t3 >> t4 >> t5;
                R r;
                i32 ret = (s->*f)(t0, t1, t2, t3, t4, t5, r);
                VERIFY(ret >= 0); // Negative error code are reserved for RPC error.
                rep << ret << r;
            }
        };
        handlers_[svc_id] = new H6(svc, svc_func);
    }

    template<class S, class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6>
    void reg(i32 svc_id, S* svc, i32 (S::*svc_func)(const T0& t0, const T1& t1, const T2& t2, const T3& t3, const T4& t4, const T5& t5, const T6& t6, R& r)) {
        class H7: public Handler {
            S* s;
            i32 (S::*f)(const T0&, const T1&, const T2&, const T3&, const T4&, const T5&, const T6&, R& r);
        public:
            H7(S* s, i32 (S::*f)(const T0&, const T1&, const T2&, const T3&, const T4&, const T5&, const T6&, R& r)): s(s), f(f) {}
            void handle(Marshal& req, Marshal& rep) {
                T0 t0;
                T1 t1;
                T2 t2;
                T3 t3;
                T4 t4;
                T5 t5;
                T6 t6;
                req >> t0 >> t1 >> t2 >> t3 >> t4 >> t5 >> t6;
                R r;
                i32 ret = (s->*f)(t0, t1, t2, t3, t4, t5, t6, r);
                VERIFY(ret >= 0); // Negative error code are reserved for RPC error.
                rep << ret << r;
            }
        };
        handlers_[svc_id] = new H7(svc, svc_func);
    }

    template<class S, class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7>
    void reg(i32 svc_id, S* svc, i32 (S::*svc_func)(const T0& t0, const T1& t1, const T2& t2, const T3& t3, const T4& t4, const T5& t5, const T6& t6, const T7& t7, R& r)) {
        class H8: public Handler {
            S* s;
            i32 (S::*f)(const T0&, const T1&, const T2&, const T3&, const T4&, const T5&, const T6&, const T7&, R& r);
        public:
            H8(S* s, i32 (S::*f)(const T0&, const T1&, const T2&, const T3&, const T4&, const T5&, const T6&, const T7&, R& r)): s(s), f(f) {}
            void handle(Marshal& req, Marshal& rep) {
                T0 t0;
                T1 t1;
                T2 t2;
                T3 t3;
                T4 t4;
                T5 t5;
                T6 t6;
                T7 t7;
                req >> t0 >> t1 >> t2 >> t3 >> t4 >> t5 >> t6 >> t7;
                R r;
                i32 ret = (s->*f)(t0, t1, t2, t3, t4, t5, t6, t7, r);
                VERIFY(ret >= 0); // Negative error code are reserved for RPC error.
                rep << ret << r;
            }
        };
        handlers_[svc_id] = new H8(svc, svc_func);
    }

    template<class S, class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8>
    void reg(i32 svc_id, S* svc, i32 (S::*svc_func)(const T0& t0, const T1& t1, const T2& t2, const T3& t3, const T4& t4, const T5& t5, const T6& t6, const T7& t7, const T8& t8, R& r)) {
        class H9: public Handler {
            S* s;
            i32 (S::*f)(const T0&, const T1&, const T2&, const T3&, const T4&, const T5&, const T6&, const T7&, const T8&, R& r);
        public:
            H9(S* s, i32 (S::*f)(const T0&, const T1&, const T2&, const T3&, const T4&, const T5&, const T6&, const T7&, const T8&, R& r)): s(s), f(f) {}
            void handle(Marshal& req, Marshal& rep) {
                T0 t0;
                T1 t1;
                T2 t2;
                T3 t3;
                T4 t4;
                T5 t5;
                T6 t6;
                T7 t7;
                T8 t8;
                req >> t0 >> t1 >> t2 >> t3 >> t4 >> t5 >> t6 >> t7 >> t8;
                R r;
                i32 ret = (s->*f)(t0, t1, t2, t3, t4, t5, t6, t7, t8, r);
                VERIFY(ret >= 0); // Negative error code are reserved for RPC error.
                rep << ret << r;
            }
        };
        handlers_[svc_id] = new H9(svc, svc_func);
    }

// __AUTO_GEN_SERVER_END__
// *** do not modify the above lines, they are automatically generated!

};

class ClientConnection;
class RpcClient: public NoCopy {

    friend class ClientConnection;

    SmartPtr<PollMgr> poll_;
    Network* net_;
    ClientConnection* conn_;
    Counter xid_counter_;

    i32 call(i64 xid, Marshal& req, Marshal*& rep);

public:

    RpcClient(const SmartPtr<PollMgr>& poll = new PollMgr, Network* net = new TcpNetwork): poll_(poll), net_(net), conn_(NULL) {}
    ~RpcClient();

    void connect(const char* addr);
    void disconnect();

// *** do not modify the following lines, they are automatically generated!
// __AUTO_GEN_CLIENT__(10)

    template<class R>
    i32 call(i32 svc_id, R& r) {
        i32 ret;
        Marshal req, *rep;
        i64 xid = xid_counter_.next();
        req << xid << svc_id;
        ret = call(xid, req, rep);
        if (ret >= 0) { // No RPC error
            *rep >> r;
        }
        delete rep;
        return ret;
    }

    template<class R, class T0>
    i32 call(i32 svc_id, const T0& t0, R& r) {
        i32 ret;
        Marshal req, *rep;
        i64 xid = xid_counter_.next();
        req << xid << svc_id << t0;
        ret = call(xid, req, rep);
        if (ret >= 0) { // No RPC error
            *rep >> r;
        }
        delete rep;
        return ret;
    }

    template<class R, class T0, class T1>
    i32 call(i32 svc_id, const T0& t0, const T1& t1, R& r) {
        i32 ret;
        Marshal req, *rep;
        i64 xid = xid_counter_.next();
        req << xid << svc_id << t0 << t1;
        ret = call(xid, req, rep);
        if (ret >= 0) { // No RPC error
            *rep >> r;
        }
        delete rep;
        return ret;
    }

    template<class R, class T0, class T1, class T2>
    i32 call(i32 svc_id, const T0& t0, const T1& t1, const T2& t2, R& r) {
        i32 ret;
        Marshal req, *rep;
        i64 xid = xid_counter_.next();
        req << xid << svc_id << t0 << t1 << t2;
        ret = call(xid, req, rep);
        if (ret >= 0) { // No RPC error
            *rep >> r;
        }
        delete rep;
        return ret;
    }

    template<class R, class T0, class T1, class T2, class T3>
    i32 call(i32 svc_id, const T0& t0, const T1& t1, const T2& t2, const T3& t3, R& r) {
        i32 ret;
        Marshal req, *rep;
        i64 xid = xid_counter_.next();
        req << xid << svc_id << t0 << t1 << t2 << t3;
        ret = call(xid, req, rep);
        if (ret >= 0) { // No RPC error
            *rep >> r;
        }
        delete rep;
        return ret;
    }

    template<class R, class T0, class T1, class T2, class T3, class T4>
    i32 call(i32 svc_id, const T0& t0, const T1& t1, const T2& t2, const T3& t3, const T4& t4, R& r) {
        i32 ret;
        Marshal req, *rep;
        i64 xid = xid_counter_.next();
        req << xid << svc_id << t0 << t1 << t2 << t3 << t4;
        ret = call(xid, req, rep);
        if (ret >= 0) { // No RPC error
            *rep >> r;
        }
        delete rep;
        return ret;
    }

    template<class R, class T0, class T1, class T2, class T3, class T4, class T5>
    i32 call(i32 svc_id, const T0& t0, const T1& t1, const T2& t2, const T3& t3, const T4& t4, const T5& t5, R& r) {
        i32 ret;
        Marshal req, *rep;
        i64 xid = xid_counter_.next();
        req << xid << svc_id << t0 << t1 << t2 << t3 << t4 << t5;
        ret = call(xid, req, rep);
        if (ret >= 0) { // No RPC error
            *rep >> r;
        }
        delete rep;
        return ret;
    }

    template<class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6>
    i32 call(i32 svc_id, const T0& t0, const T1& t1, const T2& t2, const T3& t3, const T4& t4, const T5& t5, const T6& t6, R& r) {
        i32 ret;
        Marshal req, *rep;
        i64 xid = xid_counter_.next();
        req << xid << svc_id << t0 << t1 << t2 << t3 << t4 << t5 << t6;
        ret = call(xid, req, rep);
        if (ret >= 0) { // No RPC error
            *rep >> r;
        }
        delete rep;
        return ret;
    }

    template<class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7>
    i32 call(i32 svc_id, const T0& t0, const T1& t1, const T2& t2, const T3& t3, const T4& t4, const T5& t5, const T6& t6, const T7& t7, R& r) {
        i32 ret;
        Marshal req, *rep;
        i64 xid = xid_counter_.next();
        req << xid << svc_id << t0 << t1 << t2 << t3 << t4 << t5 << t6 << t7;
        ret = call(xid, req, rep);
        if (ret >= 0) { // No RPC error
            *rep >> r;
        }
        delete rep;
        return ret;
    }

    template<class R, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8>
    i32 call(i32 svc_id, const T0& t0, const T1& t1, const T2& t2, const T3& t3, const T4& t4, const T5& t5, const T6& t6, const T7& t7, const T8& t8, R& r) {
        i32 ret;
        Marshal req, *rep;
        i64 xid = xid_counter_.next();
        req << xid << svc_id << t0 << t1 << t2 << t3 << t4 << t5 << t6 << t7 << t8;
        ret = call(xid, req, rep);
        if (ret >= 0) { // No RPC error
            *rep >> r;
        }
        delete rep;
        return ret;
    }

// __AUTO_GEN_CLIENT_END__
// *** do not modify the above lines, they are automatically generated!

};

