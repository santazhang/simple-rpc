// for testing

namespace test

struct IntValue {
    i32 v
}

service Math {
    fast add(i32, i32; i32)
    fast sub(i32, i32; i32)
    add_vec(vector<IntValue> vec; i32 sum)
    fast div_mod(i32, i32; i32, i32)
    raw noop()
}
