#include <stdio.h>
#include <unistd.h>
#include <signal.h>

#include "rpc/client.h"
#include "rpc/server.h"
#include "demo_service.h"

using namespace rpc;
using namespace demo;

int main(int argc, char* argv[]) {
    signal(SIGPIPE, SIG_IGN);
    signal(SIGHUP, SIG_IGN);
    signal(SIGCHLD, SIG_IGN);

    Server svr;
    MathService math_svc;
    svr.reg(&math_svc);
    svr.start("0.0.0.0:1987");
    while (1) {
        sleep(1);
    }
    return 0;
}
