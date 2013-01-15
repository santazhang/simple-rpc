// this file is generated from 'rpc_regression.def'
// make sure you have included server.h and client.h before including this file

#pragma once

#include <errno.h>

namespace test {

struct IntValue {
    int v;
};

inline rpc::Marshal& operator <<(rpc::Marshal& m, const IntValue& o) {
    m << o.v;
    return m;
}

inline rpc::Marshal& operator >>(rpc::Marshal& m, IntValue& o) {
    m >> o.v;
    return m;
}

class MathService: public rpc::Service {
public:
    enum {
        ADD = 0x1001,
        SUB = 0x1002,
        ADD_VEC = 0x1003,
        DIV_MOD = 0x1004,
        NOOP = 0x1005,
    };

    void reg_to(rpc::Server* svr) {
        svr->reg(ADD, this, &MathService::__add__wrapped__);
        svr->reg(SUB, this, &MathService::__sub__wrapped__);
        svr->reg(ADD_VEC, this, &MathService::__add_vec__wrapped__);
        svr->reg(DIV_MOD, this, &MathService::__div_mod__wrapped__);
        svr->reg(NOOP, this, &MathService::noop);
    }

private:
    void __add__wrapped__(rpc::Request* req, rpc::ServerConnection* sconn) {
        rpc::i32 in_0;
        req->m >> in_0;
        rpc::i32 in_1;
        req->m >> in_1;
        rpc::i32 out_0;
        this->add(in_0, in_1, &out_0);
        sconn->begin_reply(req);
        *sconn << out_0;
        sconn->end_reply();
        delete req;
        sconn->release();
    }

    void __sub__wrapped__(rpc::Request* req, rpc::ServerConnection* sconn) {
        rpc::i32 in_0;
        req->m >> in_0;
        rpc::i32 in_1;
        req->m >> in_1;
        rpc::i32 out_0;
        this->sub(in_0, in_1, &out_0);
        sconn->begin_reply(req);
        *sconn << out_0;
        sconn->end_reply();
        delete req;
        sconn->release();
    }

    void __add_vec__wrapped__(rpc::Request* req, rpc::ServerConnection* sconn) {
        class R: public rpc::Runnable {
            MathService* __thiz__;
            rpc::Request* __req__;
            rpc::ServerConnection* __sconn__;
        public:
            R(MathService* thiz, rpc::Request* r, rpc::ServerConnection* sc): __thiz__(thiz), __req__(r), __sconn__(sc) {}
            void run() {
                std::vector<IntValue > in_0;
                __req__->m >> in_0;
                rpc::i32 out_0;
                __thiz__->add_vec(in_0, &out_0);
                __sconn__->begin_reply(__req__);
                *__sconn__ << out_0;
                __sconn__->end_reply();
                delete __req__;
                __sconn__->release();
            }
        };
        sconn->run_async(new R(this, req, sconn));
    }

    void __div_mod__wrapped__(rpc::Request* req, rpc::ServerConnection* sconn) {
        rpc::i32 in_0;
        req->m >> in_0;
        rpc::i32 in_1;
        req->m >> in_1;
        rpc::i32 out_0;
        rpc::i32 out_1;
        this->div_mod(in_0, in_1, &out_0, &out_1);
        sconn->begin_reply(req);
        *sconn << out_0;
        *sconn << out_1;
        sconn->end_reply();
        delete req;
        sconn->release();
    }

public:
    // these member functions need to be implemented by user
    virtual void add(const rpc::i32&, const rpc::i32&, rpc::i32*);
    virtual void sub(const rpc::i32&, const rpc::i32&, rpc::i32*);
    virtual void add_vec(const std::vector<IntValue >& vec, rpc::i32* sum);
    virtual void div_mod(const rpc::i32&, const rpc::i32&, rpc::i32*, rpc::i32*);
    // NOTE: remember to reply req, delete req, and sconn->release(); use sconn->run_async for heavy job
    virtual void noop(rpc::Request* req, rpc::ServerConnection* sconn);

}; // class MathService

class MathProxy {
    rpc::Client* __cl__;
public:
    MathProxy(rpc::Client* cl): __cl__(cl) {}

    rpc::i32 add(const rpc::i32& in_0, const rpc::i32& in_1, rpc::i32* out_0) {
        rpc::Future* __fu__ = async_add(in_0, in_1);
        if (__fu__ == NULL) {
            return ENOTCONN;
        }
        rpc::i32 __ret__ = __fu__->get_error_code();
        if (__ret__ == 0) {
            __fu__->get_reply() >> *out_0;
        }
        __fu__->release();
        return __ret__;
    }

    rpc::Future* async_add(const rpc::i32& in_0, const rpc::i32& in_1) {
        rpc::Future* __fu__ = __cl__->begin_request();
        if (__fu__ != NULL) {
            rpc::i32 __rpc_id__ = MathService::ADD;
            *__cl__ << __rpc_id__;
            *__cl__ << in_0;
            *__cl__ << in_1;
        }
        __cl__->end_request();
        return __fu__;
    }

    rpc::i32 sub(const rpc::i32& in_0, const rpc::i32& in_1, rpc::i32* out_0) {
        rpc::Future* __fu__ = async_sub(in_0, in_1);
        if (__fu__ == NULL) {
            return ENOTCONN;
        }
        rpc::i32 __ret__ = __fu__->get_error_code();
        if (__ret__ == 0) {
            __fu__->get_reply() >> *out_0;
        }
        __fu__->release();
        return __ret__;
    }

    rpc::Future* async_sub(const rpc::i32& in_0, const rpc::i32& in_1) {
        rpc::Future* __fu__ = __cl__->begin_request();
        if (__fu__ != NULL) {
            rpc::i32 __rpc_id__ = MathService::SUB;
            *__cl__ << __rpc_id__;
            *__cl__ << in_0;
            *__cl__ << in_1;
        }
        __cl__->end_request();
        return __fu__;
    }

    rpc::i32 add_vec(const std::vector<IntValue >& vec, rpc::i32* sum) {
        rpc::Future* __fu__ = async_add_vec(vec);
        if (__fu__ == NULL) {
            return ENOTCONN;
        }
        rpc::i32 __ret__ = __fu__->get_error_code();
        if (__ret__ == 0) {
            __fu__->get_reply() >> *sum;
        }
        __fu__->release();
        return __ret__;
    }

    rpc::Future* async_add_vec(const std::vector<IntValue >& vec) {
        rpc::Future* __fu__ = __cl__->begin_request();
        if (__fu__ != NULL) {
            rpc::i32 __rpc_id__ = MathService::ADD_VEC;
            *__cl__ << __rpc_id__;
            *__cl__ << vec;
        }
        __cl__->end_request();
        return __fu__;
    }

    rpc::i32 div_mod(const rpc::i32& in_0, const rpc::i32& in_1, rpc::i32* out_0, rpc::i32* out_1) {
        rpc::Future* __fu__ = async_div_mod(in_0, in_1);
        if (__fu__ == NULL) {
            return ENOTCONN;
        }
        rpc::i32 __ret__ = __fu__->get_error_code();
        if (__ret__ == 0) {
            __fu__->get_reply() >> *out_0;
            __fu__->get_reply() >> *out_1;
        }
        __fu__->release();
        return __ret__;
    }

    rpc::Future* async_div_mod(const rpc::i32& in_0, const rpc::i32& in_1) {
        rpc::Future* __fu__ = __cl__->begin_request();
        if (__fu__ != NULL) {
            rpc::i32 __rpc_id__ = MathService::DIV_MOD;
            *__cl__ << __rpc_id__;
            *__cl__ << in_0;
            *__cl__ << in_1;
        }
        __cl__->end_request();
        return __fu__;
    }

    // raw rpc 'noop' not included

}; // class MathProxy

} // namespace test
