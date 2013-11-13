#include "base/all.h"
#include "rpc/client.h"
#include "rpc/server.h"
#include "benchmark_service.h"

using namespace base;
using namespace std;
using namespace rpc;
using namespace benchmark;

TEST(integration, rpc_bench_local) {
    PollMgr* poll = new PollMgr;
    ThreadPool* thrpool = new ThreadPool;

    // start the server
    int svc_port = find_open_port();
    EXPECT_NEQ(svc_port, -1);
    Server* svr = new Server(poll, thrpool);
    BenchmarkService bench_svc;
    svr->reg(&bench_svc);
    char svc_addr[100];
    sprintf(svc_addr, "127.0.0.1:%d", svc_port);
    svr->start(svc_addr);

    // start the client
    ClientPool* clnt_pool = new ClientPool(poll);
    BenchmarkProxy* clnt = new BenchmarkProxy(clnt_pool->get_client(svc_addr));

    Timer timer;

    timer.start();
    const int n_fast_prime = 100000;
    for (int i = 0; i < n_fast_prime; i++) {
        i32 flag = 0;
        clnt->fast_prime(i, &flag);
    }
    timer.stop();
    Log::debug("fast_prime, sync: %.2lf", n_fast_prime / timer.elapsed());
    timer.reset();

    FutureGroup* fu_group = new FutureGroup;
    timer.start();
    for (int i = 0; i < n_fast_prime; i++) {
        fu_group->add(clnt->async_fast_prime(i));
    }
    delete fu_group;
    timer.stop();
    Log::debug("fast_prime, async: %.2lf", n_fast_prime / timer.elapsed());
    timer.reset();

    delete clnt;
    delete clnt_pool;
    delete svr;

    thrpool->release();
    poll->release();
}
