#include <stdio.h>

#include "rpc.h"
#include "benchmark_rpccommon.h"

using namespace std;

int main(int argc, char* argv[]) {

    if (argc < 2) {
        printf("usage: %s <port>\n", argv[0]);
        exit(0);
    }

    RpcServer s;
    Math m;
    s.reg(Math_add, &m, &Math::add);
    s.reg(Math_add_vec, &m, &Math::add_vec);
    Text t;
    s.reg(Text_len, &t, &Text::len);

    string addr = "0.0.0.0:";
    addr += argv[1];
    s.run(addr.c_str());

    return 0;
}

