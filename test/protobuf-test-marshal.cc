#include <unistd.h>
#include <fcntl.h>
#include <cstdlib>

#include "base/all.h"
#include "rpc/marshal.h"

#include "protobuf/marshal-protobuf.h"
#include "person.pb.h"

using namespace rpc;

TEST(marshal, protobuf) {
    Person p1, p2;
    Marshal m;
    p1.set_id(1987);
    p1.set_name("Santa Zhang");
    m << p1;
    m >> p2;
    EXPECT_EQ(p2.id(), p1.id());
    EXPECT_EQ(p2.name(), p1.name());
    EXPECT_EQ(p2.id(), 1987);
    EXPECT_EQ(p2.name(), "Santa Zhang");
    EXPECT_EQ(m.content_size(), 0);
}
