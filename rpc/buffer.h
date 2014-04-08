#pragma once

#include <unistd.h>

#include "utils.h"

namespace rpc {

class Buffer {
public:
    virtual ~Buffer() {}
    virtual size_t content_size() = 0;

    virtual size_t write(const void* p, size_t n) = 0;
    virtual size_t read(void* p, size_t n) = 0;
    virtual size_t peek(void* p, size_t n) const = 0;
};


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


#ifdef RPC_STATISTICS
void stat_marshal_in(int fd, const void* buf, size_t nbytes, ssize_t ret);
void stat_marshal_out(int fd, const void* buf, size_t nbytes, ssize_t ret);
#endif // RPC_STATISTICS

struct chunk: public NoCopy {
    friend class UdpBuffer;

private:
    chunk(raw_bytes* dt, size_t rd_idx, size_t wr_idx)
            : data((raw_bytes *) dt->ref_copy()), read_idx(rd_idx), write_idx(wr_idx), next(nullptr) {
        assert(write_idx <= data->size);
        assert(read_idx <= write_idx);
    }

public:

    raw_bytes* data;
    size_t read_idx;
    size_t write_idx;
    chunk* next;

    chunk(): data(new raw_bytes), read_idx(0), write_idx(0), next(nullptr) {}
    chunk(const void* p, size_t n): data(new raw_bytes(p, n)), read_idx(0), write_idx(n), next(nullptr) {}
    ~chunk() { data->release(); }

    // NOTE: This function is only intended for Marshal::read_from_marshal.
    chunk* shared_copy() const {
        return new chunk(data, read_idx, write_idx);
    }

    size_t content_size() const {
        assert(write_idx <= data->size);
        assert(read_idx <= write_idx);
        return write_idx - read_idx;
    }

    char* set_bookmark() {
        assert(write_idx <= data->size);
        assert(read_idx <= write_idx);

        char* p = &data->ptr[write_idx];
        write_idx++;

        assert(write_idx <= data->size);
        assert(read_idx <= write_idx);
        return p;
    }

    size_t write(const void* p, size_t n) {
        assert(write_idx <= data->size);
        assert(read_idx <= write_idx);

        size_t n_write = std::min(n, data->size - write_idx);
        if (n_write > 0) {
            memcpy(data->ptr + write_idx, p, n_write);
        }
        write_idx += n_write;

        assert(write_idx <= data->size);
        assert(read_idx <= write_idx);
        return n_write;
    }

    size_t read(void* p, size_t n) {
        assert(write_idx <= data->size);
        assert(read_idx <= write_idx);

        size_t n_read = std::min(n, write_idx - read_idx);
        if (n_read > 0) {
            memcpy(p, data->ptr + read_idx, n_read);
        }
        read_idx += n_read;

        assert(write_idx <= data->size);
        assert(read_idx <= write_idx);
        return n_read;
    }

    size_t peek(void* p, size_t n) const {
        assert(write_idx <= data->size);
        assert(read_idx <= write_idx);

        size_t n_peek = std::min(n, write_idx - read_idx);
        if (n_peek > 0) {
            memcpy(p, data->ptr + read_idx, n_peek);
        }

        return n_peek;
    }

    size_t discard(size_t n) {
        assert(write_idx <= data->size);
        assert(read_idx <= write_idx);

        size_t n_discard = std::min(n, write_idx - read_idx);
        read_idx += n_discard;

        assert(write_idx <= data->size);
        assert(read_idx <= write_idx);
        return n_discard;
    }

    int write_to_fd(int fd) {
        assert(write_idx <= data->size);
        int cnt = ::write(fd, data->ptr + read_idx, write_idx - read_idx);

#ifdef RPC_STATISTICS
        stat_marshal_out(fd, data->ptr + write_idx, data->size - write_idx, cnt);
#endif // RPC_STATISTICS

        if (cnt > 0) {
            read_idx += cnt;
        }

        assert(write_idx <= data->size);
        return cnt;
    }

    int read_from_fd(int fd) {
        assert(write_idx <= data->size);
        assert(read_idx <= write_idx);

        int cnt = 0;
        if (write_idx < data->size) {
            cnt = ::read(fd, data->ptr + write_idx, data->size - write_idx);

#ifdef RPC_STATISTICS
            stat_marshal_in(fd, data->ptr + write_idx, data->size - write_idx, cnt);
#endif // RPC_STATISTICS

            if (cnt > 0) {
                write_idx += cnt;
            }
        }

        assert(write_idx <= data->size);
        assert(read_idx <= write_idx);
        return cnt;
    }

    // check if it is not possible to write to the chunk anymore.
    bool fully_written() const {
        assert(write_idx <= data->size);
        assert(read_idx <= write_idx);
        return write_idx == data->size;
    }

    // check if it is not possible to read any data even if retry later
    bool fully_read() const {
        assert(write_idx <= data->size);
        assert(read_idx <= write_idx);
        return read_idx == data->size;
    }
};



}   // namespace rpc
