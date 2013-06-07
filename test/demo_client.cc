#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "rpc/client.h"
#include "rpc/server.h"
#include "rpc/marshal.h"

#include "demo_service.h"

using namespace rpc;
using namespace demo;

class CallbackHanler {
public:
    void onCallback(Client* cl, int* rpc_counter, Future* fu) {
        (*rpc_counter)++;
        FutureAttr attr;
        attr.callback = makeCallable(&CallbackHanler::onCallback, this, cl, rpc_counter);
#if 1
        Future* fu2 = MathProxy(cl).async_is_prime(rand(), attr);
#else
        point3 p1, p2;
        p1.x = 0.0;
        p1.y = 0.0;
        p1.z = 0.0;
        p2.x = 0.0;
        p2.y = 0.0;
        p2.z = 0.0;
        Future* fu2 = MathProxy(cl_).async_dot_prod(p1, p2, attr);
#endif
        if (fu2 != NULL) {
            fu2->release();
        }
    }
};

int main(int argc, char* argv[]) {
    const char* svr_addr = "127.0.0.1:1987";
    printf("usage: %s [svr_addr=%s]\n", argv[0], svr_addr);
    if (argc > 1) {
        svr_addr = argv[1];
    }

    srand(getpid());
    PollMgr* poll = new PollMgr;
    Client* cl = new Client(poll);
    verify(cl->connect(svr_addr) == 0);
    MathProxy mp(cl);
    for (i32 n = 1; n <= 10; n++) {
        i32 flag;
        verify(mp.is_prime(n, &flag) == 0);
        if (flag) {
            Log::info("%d is prime", n);
        } else {
            Log::info("%d is not prime", n);
        }
    }
    for (int i = 0; i < 10; i++) {
        point3 p1, p2;
        p1.x = ((double) rand()) / RAND_MAX;
        p1.y = ((double) rand()) / RAND_MAX;
        p1.z = ((double) rand()) / RAND_MAX;
        p2.x = ((double) rand()) / RAND_MAX;
        p2.y = ((double) rand()) / RAND_MAX;
        p2.z = ((double) rand()) / RAND_MAX;
        double v;
        verify(mp.dot_prod(p1, p2, &v) == 0);
        Log::info("(%lf, %lf, %lf) dot_prod (%lf, %lf, %lf) = %lf", p1.x, p1.y, p1.z, p2.x, p2.y, p2.z, v);
    }

    Log::info("-- benchmark --");

    const int concurrency = 1000;

    int rpc_counter = 0;
    CallbackHanler handler; // Thread-safe

    for (int i = 0; i < concurrency; i++) {
        FutureAttr attr;
        attr.callback = makeCallable(&CallbackHanler::onCallback, &handler, cl, &rpc_counter);
#if 1
        // Start the endless chains of callbacks until close_and_release.
        Future* fu = MathProxy(cl).async_is_prime(rand(), attr);
#else
        point3 p1, p2;
        p1.x = 0.0;
        p1.y = 0.0;
        p1.z = 0.0;
        p2.x = 0.0;
        p2.y = 0.0;
        p2.z = 0.0;
        Future* fu = MathProxy(cl).async_dot_prod(p1, p2, attr);
#endif
        if (fu != NULL) {
            fu->release();
        }
    }

    for (int i = 0; i < 10; i++) {
        Log::debug("clock tick, about %d rpc done", rpc_counter);
        sleep(1);
    }

    cl->close_and_release();
    poll->release();
    return 0;
}
