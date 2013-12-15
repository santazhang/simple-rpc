// generated from 'pyonly.rpc'

#pragma once

#include "rpc/server.h"
#include "rpc/client.h"

#include <errno.h>


class PythonOnlyService: public rpc::Service {
public:
    enum {
        PYONLY_RPC = 0x2d5bdf12,
    };
    int __reg_to__(rpc::Server* svr) {
        int ret = 0;
        if ((ret = svr->reg(PYONLY_RPC, this, &PythonOnlyService::__pyonly_rpc__wrapper__)) != 0) {
            goto err;
        }
        return 0;
    err:
        svr->unreg(PYONLY_RPC);
        return ret;
    }
    // these RPC handler functions need to be implemented by user
    // for 'raw' handlers, remember to reply req, delete req, and sconn->release(); use sconn->run_async for heavy job
    virtual void pyonly_rpc(const std::string& enc_in, std::string* enc_out);
private:
    void __pyonly_rpc__wrapper__(rpc::Request* req, rpc::ServerConnection* sconn) {
        auto f = [=] {
            std::string in_0;
            req->m >> in_0;
            std::string out_0;
            this->pyonly_rpc(in_0, &out_0);
            sconn->begin_reply(req);
            *sconn << out_0;
            sconn->end_reply();
            delete req;
            sconn->release();
        };
        sconn->run_async(f);
    }
};

class PythonOnlyProxy {
protected:
    rpc::Client* __cl__;
public:
    PythonOnlyProxy(rpc::Client* cl): __cl__(cl) { }
    rpc::Future* async_pyonly_rpc(const std::string& enc_in, const rpc::FutureAttr& __fu_attr__ = rpc::FutureAttr()) {
        rpc::Future* __fu__ = __cl__->begin_request(PythonOnlyService::PYONLY_RPC, __fu_attr__);
        if (__fu__ != nullptr) {
            *__cl__ << enc_in;
        }
        __cl__->end_request();
        return __fu__;
    }
    rpc::i32 pyonly_rpc(const std::string& enc_in, std::string* enc_out) {
        rpc::Future* __fu__ = this->async_pyonly_rpc(enc_in);
        if (__fu__ == nullptr) {
            return ENOTCONN;
        }
        rpc::i32 __ret__ = __fu__->get_error_code();
        if (__ret__ == 0) {
            __fu__->get_reply() >> *enc_out;
        }
        __fu__->release();
        return __ret__;
    }
};



