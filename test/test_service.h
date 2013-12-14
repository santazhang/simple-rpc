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

class empty_serviceService: public rpc::Service {
public:
    enum {
    };
    int reg_to(rpc::Server* svr) {
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

} // namespace test



