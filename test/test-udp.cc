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
