#include "base/all.h"
#include "rpc/marshal.h"

using namespace rpc;

TEST(marshal, content_size) {
    Marshal m;
    rpc::i32 a = 4;
    EXPECT_EQ(m.content_size(), 0);
    m << a;
    EXPECT_EQ(m.content_size(), 4);
    rpc::i32 b = 9;
    m >> b;
    EXPECT_EQ(m.content_size(), 0);
    EXPECT_EQ(a, b);
}
