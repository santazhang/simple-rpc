#include <stdio.h>
#include <sys/time.h>
#include <signal.h>

#include "rpc/marshal.h"
#include "rpc/client.h"
#include "rpc/server.h"

#include "regression.h"
#include "rpc_regression.h"

using namespace std;
using namespace rpc;

using namespace test;

void MathService::add(const rpc::i32& a, const rpc::i32& b, rpc::i32* r) {
    *r = a + b;
}

void MathService::sub(const rpc::i32& a, const rpc::i32& b, rpc::i32* r) {
    *r = a - b;
}

void MathService::add_vec(const std::vector<IntValue>& vec, rpc::i32* sum) {
    *sum = 0;
    for (std::vector<IntValue>::const_iterator it = vec.begin(); it != vec.end(); ++it) {
        *sum += it->v;
    }
}

void MathService::div_mod(const rpc::i32& a, const rpc::i32& b, rpc::i32* q, rpc::i32* r) {
    *q = a / b;
    *r = a % b;
}

void MathService::noop(rpc::Request* req, rpc::ServerConnection* sconn) {
    sconn->begin_reply(req);
    sconn->end_reply();
    delete req;
    sconn->release();
}

bool marshal_size_eq(const Marshal& m, int sz) {
    return m.content_size_gt(sz - 1) && !m.content_size_gt(sz);
}

void marshal_test(int argc, char* argv[]) {
    Marshal m;

    i32 v1 = 0x18263, v2;
    m << v1;
    verify(marshal_size_eq(m, sizeof(i32)));
    m >> v2;
    verify(marshal_size_eq(m, 0));
    verify(v1 == v2);

    i64 v3 = 0x18263f835, v4;
    m << v3;
    verify(marshal_size_eq(m, sizeof(i64)));
    m >> v4;
    verify(marshal_size_eq(m, 0));
    verify(v3 == v4);

    string s1 = "hello world!!";
    int n = 89345;
    for (int i = 0; i < n; i++) {
        m << s1;
    }

    Marshal m2;
    for (;;) {
        const int read_size = 98344;
        int n = m2.read_from_marshal(m, read_size);
        if (n != read_size) {
            verify(marshal_size_eq(m, 0));
            break;
        }
    }
    string s2;
    for (int i = 0; i < n; i++) {
        m2 >> s2;
        verify(s2 == s1);
    }
    verify(marshal_size_eq(m2, 0));
}

void marshal_perf(int argc, char* argv[]) {
    Marshal m;

    string s1 = "hello world!!";
    int n = 987238;

    timeval start, stop;
    gettimeofday(&start, NULL);
    for (int i = 0; i < n; i++) {
        m << s1;
    }
    gettimeofday(&stop, NULL);
    double sec = (stop.tv_sec - start.tv_sec) + (stop.tv_usec - start.tv_usec) / 1000000.0;
    double nops = n / ((stop.tv_sec - start.tv_sec) + (stop.tv_usec - start.tv_usec) / 1000000.0);
    printf("Marshal << '%s' %d times takes %lf sec (%.0lf op/s)\n", s1.c_str(), n, sec, nops);

    printf("marshal size = %d\n", m.content_size());

    Marshal m2;
    gettimeofday(&start, NULL);
    for (;;) {
        const int read_size = 2347;
        int n = m2.read_from_marshal(m, read_size);
        if (n != read_size) {
            verify(marshal_size_eq(m, 0));
            break;
        }
    }
    gettimeofday(&stop, NULL);
    sec = (stop.tv_sec - start.tv_sec) + (stop.tv_usec - start.tv_usec) / 1000000.0;
    double mbps = (sizeof(i32) + s1.length()) * n / sec / 1024 / 1024;
    printf("Marshal::read_from_marshal (small read) on the above marshal takes %lf sec (%.2lf M/s)\n", sec, mbps);

    printf("marshal (copied) size = %d\n", m2.content_size());

    string s2;
    gettimeofday(&start, NULL);
    for (int i = 0; i < n; i++) {
        m2 >> s2;
        verify(s2 == s1);
    }
    gettimeofday(&stop, NULL);
    verify(marshal_size_eq(m2, 0));

    sec = (stop.tv_sec - start.tv_sec) + (stop.tv_usec - start.tv_usec) / 1000000.0;
    nops = n / ((stop.tv_sec - start.tv_sec) + (stop.tv_usec - start.tv_usec) / 1000000.0);
    printf("Marshal >> '%s' %d times takes %lf sec (%.0lf op/s)\n", s1.c_str(), n, sec, nops);

    for (int i = 0; i < n; i++) {
        m2 << s1;
    }

    int m_size = m2.content_size();
    gettimeofday(&start, NULL);
    verify(m.read_from_marshal(m2, m_size) == m_size);
    gettimeofday(&stop, NULL);
    sec = (stop.tv_sec - start.tv_sec) + (stop.tv_usec - start.tv_usec) / 1000000.0;
    mbps = (sizeof(i32) + s1.length()) * n / sec / 1024 / 1024;

    printf("Marshal::read_from_marshal (large read) on the above marshal takes %lf sec (%.2lf M/s)\n", sec, mbps);

    verify(m.content_size() == m_size);
    verify(m2.content_size() == 0);
    for (int i = 0; i < n; i++) {
        m >> s2;
        verify(s2 == s1);
    }
}

void rpc_perf_server(int argc, char* argv[]) {
    Server s;
    s.start("0.0.0.0:1987");
    sleep(20);
}

struct rpc_perf_client_thread_arg_type {
    PollMgr* poll;
    char* server_addr;
};

void* rpc_perf_client_thread(void* arg) {
    rpc_perf_client_thread_arg_type* args = (rpc_perf_client_thread_arg_type *) arg;
    Client* cl = new Client(args->poll);

    cl->connect(args->server_addr);
    const int batch_size = 100;
    const int batch_rounds = 100;
    for (int batch = 0; batch < batch_rounds; batch++) {
        Future* batch_fu[batch_size];
        for (int i = 0; i < batch_size; i++) {
            batch_fu[i] = cl->begin_request();
            cl->end_request();
        }
        for (int i = 0; i < batch_size; i++) {
            batch_fu[i]->get_error_code();
            batch_fu[i]->release();
        }
    }
    cl->close_and_release();

    pthread_exit(NULL);
    return NULL;
}

void rpc_perf_client(int argc, char* argv[]) {

    if (argc <= 2) {
        printf("usage: %s %s <server-addr>\n", argv[0], argv[1]);
        exit(-1);
    }

    rpc_perf_client_thread_arg_type args;
    args.poll = new PollMgr;
    args.server_addr = argv[2];

    const int n_threads = 100;
    pthread_t threads[n_threads];

    for (int i = 0; i < n_threads; i++) {
        Pthread_create(&threads[i], NULL, rpc_perf_client_thread, &args);
    }

    for (int i = 0; i < n_threads; i++) {
        Pthread_join(threads[i], NULL);
    }

    args.poll->release();
}

void rpc_perf(int argc, char* argv[]) {
    {
        PollMgr* poll = new PollMgr(8);

        Server* s = new Server(poll);
        s->start("0.0.0.0:1987");

        ThreadPool* tpoll = new ThreadPool;

        class PerfClient: public Runnable {
            PollMgr* poll_;
        public:
            PerfClient(PollMgr* poll) {
                poll_ = poll;
            }
            void run() {
                Client* cl = new Client(poll_);
                cl->connect("127.0.0.1:1987");
                const int batch_size = 1000;
                const int batch_rounds = 1000;
                for (int batch = 0; batch < batch_rounds; batch++) {
                    Future* batch_fu[batch_size];
                    for (int i = 0; i < batch_size; i++) {
                        batch_fu[i] = cl->begin_request();
                        cl->end_request();
                    }
                    for (int i = 0; i < batch_size; i++) {
                        batch_fu[i]->get_error_code();
                        batch_fu[i]->release();
                    }
                }
                cl->close_and_release();
            }
        };

        for (int i = 0; i < 10; i++) {
            tpoll->run_async(new PerfClient(poll));
        }

        delete tpoll;
        delete s;

        poll->release();
    }
}

void rpc_test(int argc, char* argv[]) {

    {
        // server not running
        PollMgr* poll = new PollMgr;
        for (int i = 0; i < 10; i++) {
            Server s(poll);
            MathService ms;
            s.reg(&ms);
            s.start("0.0.0.0:1987");

            Client* cl = new Client(poll);
            cl->connect("127.0.0.1:1987");

            MathProxy mcl(cl);
            for (int j = 0; j < 100; j++) {
                int r;
                mcl.add(4, 3, &r);
                verify(r == 7);
                mcl.sub(4, 3, &r);
                verify(r == 1);
            }

            cl->close_and_release();
        }
        poll->release();
    }

    {
        PollMgr* poll = new PollMgr(8);
        for (int i = 0; i < 10; i++) {
            Server s(poll);
            MathService ms;
            s.reg(&ms);
            s.start("0.0.0.0:1987");

            Client* cl = new Client(poll);
            cl->connect("127.0.0.1:1987");

            cl->close_and_release();
        }
        poll->release();
    }

    {
        // connect to running server
        PollMgr* poll = new PollMgr(8);
        for (int i = 0; i < 10; i++) {
            Server s(poll);
            s.start("0.0.0.0:1987");

            Client* cl = new Client(poll);
            cl->connect("127.0.0.1:1987");

            cl->close_and_release();
        }
        poll->release();
    }

}

int main(int argc, char* argv[]) {

    signal(SIGPIPE, SIG_IGN);
    signal(SIGHUP, SIG_IGN);

    TestRunner tr;
    tr.reg("marshal-test", marshal_test);
    tr.reg("marshal-perf", marshal_perf);
    tr.reg("rpc-test", rpc_test);
    tr.reg("rpc-perf", rpc_perf);
    tr.reg("rpc-perf-server", rpc_perf_server);
    tr.reg("rpc-perf-client", rpc_perf_client);
    tr.run(argc, argv);
    return 0;
}
