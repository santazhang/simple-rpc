#pragma once

#include "utils.h"

namespace rpc {

struct raw_bytes: public RefCounted {
    char* ptr;
    size_t size;
    static const size_t min_size;

    raw_bytes(size_t sz = min_size) {
        size = std::max(sz, min_size);
        ptr = new char[size];
    }
    raw_bytes(const void* p, size_t n) {
        size = std::max(n, min_size);
        ptr = new char[size];
        memcpy(ptr, p, n);
    }
    ~raw_bytes() { delete[] ptr; }
};


class Buffer {
public:
    virtual ~Buffer() {}
    virtual size_t content_size() = 0;

    virtual size_t write(const void* p, size_t n) = 0;
    virtual size_t read(void* p, size_t n) = 0;
    virtual size_t peek(void* p, size_t n) const = 0;
};


}   // namespace rpc
