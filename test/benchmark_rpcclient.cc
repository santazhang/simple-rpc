#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "rpc.h"
#include "benchmark_rpccommon.h"

using namespace std;

string g_test_set;
int g_n_requests = 10000;

void* bench_thread(void* ptr) {
    RpcClient* c = (RpcClient *) ptr;
    string slong, sshort;
    slong.reserve(1000 * 100);
    for (int i = 0; i < 100 * 100; i++) {
        slong += "0123456789";
    }
    for (int i = 0; i < 100; i++) {
        sshort += "0123456789";
    }

    for (int i = 0; i < g_n_requests; i++) {
        i32 r, ret;
        if (g_test_set == "" || g_test_set == "math_add") {
            ret = c->call(Math_add, 2, 3, r);
            VERIFY(ret == 0); VERIFY(r == 5);
        }
        if (g_test_set == "" || g_test_set == "math_add_vec") {
            vector<i32> v;
            for (int j = 0; j < 100; j++) {
                v.push_back(j);
            }
            ret = c->call(Math_add_vec, v, r);
            VERIFY(ret == 0);
        }
        if (g_test_set == "" || g_test_set == "text_len") {
            ret = c->call(Text_len, sshort, r);
            VERIFY(r == sshort.length());
        }
        if (g_test_set == "" || g_test_set == "text_len_long") {
            ret = c->call(Text_len, slong, r);
            VERIFY(r == slong.length());
        }
    }

    pthread_exit(NULL);
    return NULL;
}

int main(int argc, char* argv[]) {
    string server_addr;
    int n_threads = 1;
    int n_proc = 1;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-p") == 0 && ++i < argc) {
            n_proc = atoi(argv[i]);
            Log::info("Using %d processes for benchmark", n_proc);
        } else if (strcmp(argv[i], "-c") == 0 && ++i < argc) {
            n_threads = atoi(argv[i]);
            Log::info("Using %d threads for benchmark", n_threads);
        } else if (strcmp(argv[i], "-n") == 0 && ++i < argc) {
            g_n_requests = atoi(argv[i]);
            Log::info("Each thread will issue %d requests", g_n_requests);
        } else if (strcmp(argv[i], "-t") == 0 && ++i < argc) {
            g_test_set = argv[i];
            Log::info("Will run test case '%s'", g_test_set.c_str());
        } else {
            server_addr = argv[i];
            if (server_addr.find(":") == string::npos) {
                server_addr = "127.0.0.1:" + server_addr;
            }
            Log::info("Server at %s", server_addr.c_str());
        }
    }
    if (server_addr == "") {
        printf("usage: %s [-p n_proc] [-c n_thread] [-n n_request] [-t test_set] <[host:]port>\n", argv[0]);
        exit(0);
    }

    if (n_proc == 1) {
        RpcClient c;
        c.connect(server_addr.c_str());

        pthread_t* th = new pthread_t[n_threads];
        for (int i = 0; i < n_threads; i++) {
            pthread_create(&th[i], NULL, bench_thread, &c);
        }
        for (int i = 0; i < n_threads; i++) {
            pthread_join(th[i], NULL);
        }
        c.disconnect();

        delete [] th;
    } else {
        for (int j = 0; j < n_proc; j++) {
            if (fork() == 0) {
                RpcClient c;
                c.connect(server_addr.c_str());

                pthread_t* th = new pthread_t[n_threads];
                for (int i = 0; i < n_threads; i++) {
                    pthread_create(&th[i], NULL, bench_thread, &c);
                }
                for (int i = 0; i < n_threads; i++) {
                    pthread_join(th[i], NULL);
                }
                c.disconnect();

                delete [] th;
                exit(0);
            }
        }
        for (int j = 0; j < n_proc; j++) {
            wait(NULL);
        }
    }
    return 0;
}
