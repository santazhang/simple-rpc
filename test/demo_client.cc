#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "rpc/client.h"
#include "rpc/server.h"
#include "rpc/marshal.h"

#include "demo_service.h"

using namespace rpc;
using namespace demo;


int main(int argc, char* argv[]) {
    srand(getpid());
    PollMgr* poll = new PollMgr;
    Client* cl = new Client(poll);
    verify(cl->connect("127.0.0.1:1987") == 0);
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

    for (int i = 0; i < concurrency; i++) {
        class CB: public FutureCallback {
        public:
            CB(Client* cl, int* rpc_counter): cl_(cl), rpc_counter_(rpc_counter) {}
            void run(Future* fu) {
                (*rpc_counter_)++;
                FutureAttr attr;
                attr.callback = new CB(cl_, rpc_counter_);
                Future* fu2 = MathProxy(cl_).async_is_prime(rand(), attr);
/*
                point3 p1, p2;
                p1.x = 0.0;
                p1.y = 0.0;
                p1.z = 0.0;
                p2.x = 0.0;
                p2.y = 0.0;
                p2.z = 0.0;
                Future* fu2 = MathProxy(cl_).async_dot_prod(p1, p2, attr);
*/
                fu2->release();
            }
        private:
            Client* cl_;
            int* rpc_counter_;
        };

        FutureAttr attr;
        attr.callback = new CB(cl, &rpc_counter);
        Future* fu = MathProxy(cl).async_is_prime(rand(), attr);
/*
        point3 p1, p2;
        p1.x = 0.0;
        p1.y = 0.0;
        p1.z = 0.0;
        p2.x = 0.0;
        p2.y = 0.0;
        p2.z = 0.0;
        Future* fu = MathProxy(cl).async_dot_prod(p1, p2, attr);
*/
        fu->release();
    }

    while (1) {
        Log::debug("clock tick, about %d rpc done", rpc_counter);
        sleep(1);
    }

    cl->close_and_release();
    poll->release();
    return 0;
}
