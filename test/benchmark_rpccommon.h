#pragma once

#include <string>

#include "utils.h"

class Math {
public:
    i32 add(const i32& a, const i32& b, i32& r) {
        r = a + b;
        return 0;
    }

    i32 add_vec(const std::vector<i32>& v, i32& r) {
        r = 0;
        for (std::vector<i32>::const_iterator it = v.begin(); it != v.end(); it++) {
            r += *it;
        }
        return 0;
    }
};

class Text {
public:
    i32 len(const std::string& s, i32& r) {
        r = s.length();
        return 0;
    }
};

enum {
    Math_add = 0x8001,
    Math_add_vec,
    Text_len
};
