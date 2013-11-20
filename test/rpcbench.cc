#include <string>

#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <unistd.h>
#include <sys/time.h>
#include <signal.h>

#include "rpc/client.h"
#include "rpc/server.h"
#include "benchmark_service.h"

using namespace benchmark;
using namespace rpc;
using namespace std;

const char *svr_addr = "127.0.0.1:8848";
int byte_size = 10;
int epoll_instances = 2;
bool fast_requests = false;
int seconds = 10;
int outgoing_requests = 1000;
int client_threads = 8;
int worker_threads = 16;

static string request_str;
PollMgr* poll;
ThreadPool* thrpool;

Counter req_counter;

bool should_stop = false;

void* stat_proc(void*) {
    i64 last_cnt = 0;
    for (int i = 0; i < seconds; i++) {
        int cnt = req_counter.peek_next();
        if (last_cnt != 0) {
            Log::debug("qps: %ld", cnt - last_cnt);
        }
        last_cnt = cnt;
        sleep(1);
    }
    should_stop = true;
    pthread_exit(nullptr);
    return nullptr;
}

void* client_proc(void*) {
    Client* cl = new Client(poll);
    verify(cl->connect(svr_addr) == 0);
    BenchmarkProxy bm(cl);
    if (fast_requests) {
        FutureAttr fu_attr;
        auto do_work = [&bm, &fu_attr] {
            if (!should_stop) {
                Future* fu = bm.async_fast_nop(request_str, fu_attr);
                Future::safe_release(fu);
                req_counter.next();
            }
        };
        fu_attr.callback = [&do_work] (Future* fu) {
            if (fu->get_error_code() != 0) {
                return;
            }
            thrpool->run_async([&do_work] {
                do_work();
            });
        };
        for (int i = 0; i < outgoing_requests; i++) {
            do_work();
        }
    } else {
        FutureAttr fu_attr;
        auto do_work = [&bm, &fu_attr] {
            if (!should_stop) {
                Future* fu = bm.async_nop(request_str, fu_attr);
                Future::safe_release(fu);
                req_counter.next();
            }
        };
        fu_attr.callback = [&do_work] (Future* fu) {
            if (fu->get_error_code() != 0) {
                return;
            }
            thrpool->run_async([&do_work] {
                do_work();
            });
        };
        for (int i = 0; i < outgoing_requests; i++) {
            do_work();
        }
    }
    while (!should_stop) {
        sleep(1);
    }
    cl->close_and_release();
    pthread_exit(nullptr);
    return nullptr;
}

int main(int argc, char **argv) {
    signal(SIGPIPE, SIG_IGN);
    signal(SIGHUP, SIG_IGN);
    signal(SIGCHLD, SIG_IGN);

    bool is_client = false, is_server = false;

    if (argc < 2) {
        printf("usage: perftest OPTIONS\n");
        printf("                -c|-s ip:port\n");
        printf("                -b    byte_size\n");
        printf("                -e    epoll_instances\n");
        printf("                -f    fast_requests\n");
        printf("                -n    seconds\n");
        printf("                -o    outgoing_requests\n");
        printf("                -t    client_threads\n");
        printf("                -w    worker_threads\n");
        exit(1);
    }

    char ch = 0;
    while ((ch = getopt(argc, argv, "c:s:b:e:fn:o:t:w:"))!= -1) {
        switch (ch) {
        case 'c':
            is_client = true;
            svr_addr = optarg;
            break;
        case 's':
            is_server = true;
            svr_addr = optarg;
            break;
        case 'b':
            byte_size = atoi(optarg);
            break;
        case 'e':
            epoll_instances = atoi(optarg);
            break;
        case 'f':
            fast_requests = true;
            break;
        case 'n':
            seconds = atoi(optarg);
            break;
        case 'o':
            outgoing_requests = atoi(optarg);
            break;
        case 't':
            client_threads = atoi(optarg);
            break;
        case 'w':
            worker_threads = atoi(optarg);
            break;
        default:
            break;
        }
    }
    verify(is_server || is_client);
    if (is_server) {
        Log::info("server will start at     %s", svr_addr);
    } else {
        Log::info("client will connect to   %s", svr_addr);
    }
    Log::info("packet byte size:        %d", byte_size);
    Log::info("epoll instances:         %d", epoll_instances);
    Log::info("fast reqeust:            %s", fast_requests ? "true" : "false");
    Log::info("running seconds:         %d", seconds);
    Log::info("outgoing requests:       %d", outgoing_requests);
    Log::info("client threads:          %d", client_threads);
    Log::info("worker threads:          %d", worker_threads);

    poll_options poll_opt;
    poll_opt.n_threads = epoll_instances;
    poll = new PollMgr(poll_opt);
    thrpool = new ThreadPool(worker_threads);
    if (is_server) {
        Server svr(poll, thrpool);
        BenchmarkService svc;
        svr.reg(&svc);
        verify(svr.start(svr_addr) == 0);
        for (;;) {
            sleep(1);
        }
    } else {
        pthread_t* client_th = new pthread_t[client_threads];
        for (int i = 0; i < client_threads; i++) {
            Pthread_create(&client_th[i], nullptr, client_proc, nullptr);
        }
        pthread_t stat_th;
        Pthread_create(&stat_th, nullptr, stat_proc, nullptr);
        Pthread_join(stat_th, nullptr);
        for (int i = 0; i < client_threads; i++) {
            Pthread_join(client_th[i], nullptr);
        }
        delete[] client_th;
    }

    poll->release();
    thrpool->release();
    return 0;
}
