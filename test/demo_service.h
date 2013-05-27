// this file is generated from 'demo_service.rpc'
// make sure you have included server.h and client.h before including this file

#pragma once

#include <errno.h>

namespace demo {

struct point3 {
    double x;
    double y;
    double z;
};

} // namespace demo

// back to default namespace, we want the marshaling operators be avaialbe without using namespace demo
inline rpc::Marshal& operator <<(rpc::Marshal& m, const demo::point3& o) {
    m << o.x;
    m << o.y;
    m << o.z;
    return m;
}

inline rpc::Marshal& operator >>(rpc::Marshal& m, demo::point3& o) {
    m >> o.x;
    m >> o.y;
    m >> o.z;
    return m;
}

namespace demo {

class MathService: public rpc::Service {
public:
    enum {
        IS_PRIME = 0x1001,
        DOT_PROD = 0x1002,
    };

    void reg_to(rpc::Server* svr) {
        svr->reg(IS_PRIME, this, &MathService::__is_prime__wrapped__);
        svr->reg(DOT_PROD, this, &MathService::__dot_prod__wrapped__);
    }

private:
    void __is_prime__wrapped__(rpc::Request* req, rpc::ServerConnection* sconn) {
        class R: public rpc::Runnable {
            MathService* __thiz__;
            rpc::Request* __req__;
            rpc::ServerConnection* __sconn__;
        public:
            R(MathService* thiz, rpc::Request* r, rpc::ServerConnection* sc): __thiz__(thiz), __req__(r), __sconn__(sc) {}
            void run() {
                rpc::i32 in_0;
                __req__->m >> in_0;
                rpc::i32 out_0;
                __thiz__->is_prime(in_0, &out_0);
                __sconn__->begin_reply(__req__);
                *__sconn__ << out_0;
                __sconn__->end_reply();
                delete __req__;
                __sconn__->release();
            }
        };
        sconn->run_async(new R(this, req, sconn));
    }

    void __dot_prod__wrapped__(rpc::Request* req, rpc::ServerConnection* sconn) {
        point3 in_0;
        req->m >> in_0;
        point3 in_1;
        req->m >> in_1;
        double out_0;
        this->dot_prod(in_0, in_1, &out_0);
        sconn->begin_reply(req);
        *sconn << out_0;
        sconn->end_reply();
        delete req;
        sconn->release();
    }

public:
    // these member functions need to be implemented by user
    virtual void is_prime(const rpc::i32& n, rpc::i32* flag);
    virtual void dot_prod(const point3& p1, const point3& p2, double* v);

}; // class MathService

class MathProxy {
protected:
    rpc::Client* __cl__;
public:
    MathProxy(rpc::Client* cl): __cl__(cl) {}

    rpc::i32 is_prime(const rpc::i32& n, rpc::i32* flag) {
        rpc::Future* __fu__ = async_is_prime(n);
        if (__fu__ == NULL) {
            return ENOTCONN;
        }
        rpc::i32 __ret__ = __fu__->get_error_code();
        if (__ret__ == 0) {
            __fu__->get_reply() >> *flag;
        }
        __fu__->release();
        return __ret__;
    }

    rpc::Future* async_is_prime(const rpc::i32& n, const rpc::FutureAttr& __fu_attr__ = rpc::FutureAttr()) {
        rpc::Future* __fu__ = __cl__->begin_request(MathService::IS_PRIME, __fu_attr__);
        if (__fu__ != NULL) {
            *__cl__ << n;
        }
        __cl__->end_request();
        return __fu__;
    }

    rpc::i32 dot_prod(const point3& p1, const point3& p2, double* v) {
        rpc::Future* __fu__ = async_dot_prod(p1, p2);
        if (__fu__ == NULL) {
            return ENOTCONN;
        }
        rpc::i32 __ret__ = __fu__->get_error_code();
        if (__ret__ == 0) {
            __fu__->get_reply() >> *v;
        }
        __fu__->release();
        return __ret__;
    }

    rpc::Future* async_dot_prod(const point3& p1, const point3& p2, const rpc::FutureAttr& __fu_attr__ = rpc::FutureAttr()) {
        rpc::Future* __fu__ = __cl__->begin_request(MathService::DOT_PROD, __fu_attr__);
        if (__fu__ != NULL) {
            *__cl__ << p1;
            *__cl__ << p2;
        }
        __cl__->end_request();
        return __fu__;
    }

}; // class MathProxy

} // namespace demo

namespace demo {
inline void MathService::dot_prod(const point3& p1, const point3& p2, double* v) {
    *v = p1.x * p2.x + p1.y * p2.y + p1.z * p2.z;
}
}
