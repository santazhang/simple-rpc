#include <sys/time.h>

#include "rpc/utils.h"
#include "rpc/marshal.h"
#include "test_service.h"

using namespace rpc;
using namespace std;
using namespace test;

TEST(bm_serialization, person) {
    Person person;
    person.id = 1;
    person.name = "hello world";
    person.email = "big boss";

    Marshal m;

    struct timeval time_begin, time_end;

    const int n_rounds = 1000 * 1000;
    gettimeofday(&time_begin, nullptr);
    for (int i = 0; i < n_rounds; i++) {
        m << person;
        m >> person;
    }
    gettimeofday(&time_end, nullptr);

    double elapsed = time_end.tv_sec - time_begin.tv_sec + (time_end.tv_usec - time_begin.tv_usec) / 1000.0 / 1000.0;
    LOG_INFO << "rounds = " << n_rounds;
    LOG_INFO("elapsed = %.3lf sec", elapsed);
    LOG_INFO("qps = %.3lf Mop/s", n_rounds / elapsed / 1000.0 / 1000.0);
}
