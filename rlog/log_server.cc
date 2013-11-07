#include <string>

#include <stdio.h>
#include <unistd.h>
#include <signal.h>

#include "rpc/server.h"
#include "rpc/client.h"
#include "log_service_impl.h"

using namespace std;
using namespace rpc;
using namespace rlog;

Mutex g_stop_mutex;
CondVar g_stop_condvar;
bool g_stop_flag = false;

static void signal_handler(int sig) {
    Log_info("caught signal %d, stopping server now", sig);
    g_stop_flag = true;
    g_stop_condvar.signal();
}

int main(int argc, char* argv[]) {
    string bind_addr = "0.0.0.0:8848";
    printf("usage: %s [bind_addr=%s]\n", argv[0], bind_addr.c_str());
    if (argc >= 2) {
        bind_addr = argv[1];
        if (bind_addr.find(":") == string::npos) {
            bind_addr = "0.0.0.0:" + bind_addr;
        }
    }

    Log::set_level(Log::INFO);

    RLogService* log_service = new RLogServiceImpl;
    Server* server = new Server;
    server->reg(log_service);

    int ret;
    if ((ret = server->start(bind_addr.c_str())) == 0) {
        signal(SIGPIPE, SIG_IGN);
        signal(SIGHUP, SIG_IGN);
        signal(SIGCHLD, SIG_IGN);

        signal(SIGALRM, signal_handler);
        signal(SIGINT, signal_handler);
        signal(SIGQUIT, signal_handler);
        signal(SIGTERM, signal_handler);

        g_stop_mutex.lock();
        while (g_stop_flag == false) {
            g_stop_condvar.wait(g_stop_mutex);
        }
        g_stop_mutex.unlock();

        delete server;
        delete log_service;
    }

    return ret;
}
