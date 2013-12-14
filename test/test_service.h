// generated from 'test_service.rpc'

#pragma once

#include "rpc/server.h"
#include "rpc/client.h"

#include <errno.h>


namespace test {

struct empty_struct {
};

inline rpc::Marshal& operator <<(rpc::Marshal& m, const empty_struct& o) {
    return m;
}

inline rpc::Marshal& operator >>(rpc::Marshal& m, empty_struct& o) {
    return m;
}

struct complex_struct {
    std::map<std::string, std::vector<std::vector<std::string>>> d;
    std::set<std::string> s;
};

inline rpc::Marshal& operator <<(rpc::Marshal& m, const complex_struct& o) {
    m << o.d;
    m << o.s;
    return m;
}

inline rpc::Marshal& operator >>(rpc::Marshal& m, complex_struct& o) {
    m >> o.d;
    m >> o.s;
    return m;
}

class empty_serviceService: public rpc::Service {
public:
    enum {
    };
    int __reg_to__(rpc::Server* svr) {
        int ret = 0;
        return 0;
    err:
        return ret;
    }
    // these RPC handler functions need to be implemented by user
    // for 'raw' handlers, remember to reply req, delete req, and sconn->release(); use sconn->run_async for heavy job
private:
};

class empty_serviceProxy {
protected:
    rpc::Client* __cl__;
public:
    empty_serviceProxy(rpc::Client* cl): __cl__(cl) { }
};

class math_serviceService: public rpc::Service {
public:
    enum {
        GCD = 0x6a2d6174,
    };
    int __reg_to__(rpc::Server* svr) {
        int ret = 0;
        if ((ret = svr->reg(GCD, this, &math_serviceService::__gcd__wrapper__)) != 0) {
            goto err;
        }
        return 0;
    err:
        svr->unreg(GCD);
        return ret;
    }
    // these RPC handler functions need to be implemented by user
    // for 'raw' handlers, remember to reply req, delete req, and sconn->release(); use sconn->run_async for heavy job
    virtual void gcd(const rpc::i64& a, const rpc::i64& b, rpc::i64* g);
private:
    void __gcd__wrapper__(rpc::Request* req, rpc::ServerConnection* sconn) {
        auto f = [=] {
            rpc::i64 in_0;
            req->m >> in_0;
            rpc::i64 in_1;
            req->m >> in_1;
            rpc::i64 out_0;
            this->gcd(in_0, in_1, &out_0);
            sconn->begin_reply(req);
            *sconn << out_0;
            sconn->end_reply();
            delete req;
            sconn->release();
        };
        sconn->run_async(f);
    }
};

class math_serviceProxy {
protected:
    rpc::Client* __cl__;
public:
    math_serviceProxy(rpc::Client* cl): __cl__(cl) { }
    rpc::Future* async_gcd(const rpc::i64& a, const rpc::i64& b, const rpc::FutureAttr& __fu_attr__ = rpc::FutureAttr()) {
        rpc::Future* __fu__ = __cl__->begin_request(math_serviceService::GCD, __fu_attr__);
        if (__fu__ != nullptr) {
            *__cl__ << a;
            *__cl__ << b;
        }
        __cl__->end_request();
        return __fu__;
    }
    rpc::i32 gcd(const rpc::i64& a, const rpc::i64& b, rpc::i64* g) {
        rpc::Future* __fu__ = this->async_gcd(a, b);
        if (__fu__ == nullptr) {
            return ENOTCONN;
        }
        rpc::i32 __ret__ = __fu__->get_error_code();
        if (__ret__ == 0) {
            __fu__->get_reply() >> *g;
        }
        __fu__->release();
        return __ret__;
    }
};

} // namespace test



