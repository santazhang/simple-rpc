#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <unistd.h>
#include <sys/time.h>
#include <signal.h>
#include <vector>
#include <map>

#include "rpc/client.h"
#include "rpc/server.h"
#include "demo_service.h"

#define NUM 10000000

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

int main(int argc, char **argv) {
  vector<DemoProxy*> servers;
  for (int i = 0; i < 4; ++i) {
    servers.push_back(start_server(9999 + i));
  }

  for (int i = 0; i < 100; ++i) {
    vector<Future*> f;
    for (int j = 0; j < 10000; ++j) {
      f.push_back(servers[j % 4]->async_prime(j));
      Log::info("Sending... %d", j);
    }

    for (int k = 0; k < f.size(); ++k) {
      Log::info("Waiting... %d", k);
      f[k]->wait();
      f[k]->release();
    }
  }
}
