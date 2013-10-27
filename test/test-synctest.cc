#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <unistd.h>
#include <sys/time.h>
#include <signal.h>
#include <vector>
#include <map>

#include "base/all.h"
#include "rpc/client.h"
#include "rpc/server.h"
#include "demo_service.h"

using namespace demo;
using namespace rpc;
using namespace std;

char buffer[1024];
char* fmt(const char* fmt, ...) {
    va_list l;
    va_start(l, fmt);
    vsprintf(buffer, fmt, l);
    va_end(l);
    return buffer;
}

DemoProxy* start_server(int port) {
    auto poll = new PollMgr();
    auto pool = new ThreadPool(8);
    auto svc = new DemoService();
    auto server = new Server(poll, pool);
    server->reg(svc);
    server->start(fmt("localhost:%d", port));

    auto client = new Client(poll);
    client->connect(fmt("localhost:%d", port));
    return new DemoProxy(client);
}

TEST(integration, sync_test) {
    vector<DemoProxy*> servers;
    for (int i = 0; i < 4; ++i) {
        servers.push_back(start_server(9999 + i));
    }

    int n_total_batches = 100;
    for (int i = 1; i <= n_total_batches; ++i) {
        if (i % 10 == 0) {
            Log_info("Running %d/%d batch...", i, n_total_batches);
        }
        vector<Future*> f;
        for (int j = 0; j < 10000; ++j) {
            f.push_back(servers[j % 4]->async_prime(j));
        }
//        Log_info("sent 10000 requests");

        for (int k = 0; k < f.size(); ++k) {
            f[k]->wait();
            f[k]->release();
        }
//        Log_info("got 10000 replies");
    }
}
