#include "rpc/utils.h"

using namespace rpc;

TEST(utils, find_open_port) {
    EXPECT_NEQ(find_open_port(), -1);
}

TEST(utils, get_host_name) {
    std::string host = get_host_name();
    Log::info("hostname: %s", host.c_str());
}