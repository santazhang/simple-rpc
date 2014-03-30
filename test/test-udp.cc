#include "base/all.h"
#include "rpc/client.h"
#include "rpc/server.h"
#include "benchmark_service.h"

using namespace base;
using namespace std;
using namespace rpc;
using namespace benchmark;


TEST(udp, server_start_stop) {
    Server* svr = new Server;
    BenchmarkService bench_svc;
    svr->reg(&bench_svc);
    svr->start("localhost:8848");
    delete svr;
}


TEST(udp, simple_rpc) {
    Server* svr = new Server;
    BenchmarkService bench_svc;
    svr->reg(&bench_svc);
    svr->start("localhost:8848");
    PollMgr* poll = new PollMgr;
    Client* clnt = new Client(poll);
    clnt->connect("localhost:8848");
    BenchmarkProxy proxy(clnt);

    i32 p = 17;
    i8 flag = 0;
    proxy.fast_prime(p, &flag);
    Log::debug("prime(%d) -> %d", p, flag);

    i32 dummy1 = 1987;
    i32 dummy2 = 1989;
    proxy.lossy_nop(dummy1, dummy2);

    clnt->close_and_release();
    poll->release();
    delete svr;
}
