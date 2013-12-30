#include <math.h>

#include "rpc/client.h"
#include "rpc/server.h"
#include "benchmark_service.h"

using namespace benchmark;
using namespace rpc;

static Counter g_nop_counter;

void BenchmarkService::fast_prime(const i32& n, i8* flag) {
    if (n <= 0) {
        *flag = -1;
    } else if (n <= 3) {
        *flag = 1;
    } else if (n % 2 == 0) {
        *flag = 0;
    } else {
        int d = 3;
        int m = sqrt(n) + 1; // +1 for sqrt float errors
        while (d <= m) {
            if (n % d == 0) {
                *flag = 0;
                return;
            }
            d++;
        }
        *flag = 1;
    }
}

void BenchmarkService::nop(const std::string&) {
    int cnt = g_nop_counter.next();
    if (cnt % 10000 == 0) {
        Log::info("%d nop requests", cnt);
    }
}
