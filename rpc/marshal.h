#pragma once

#include <list>
#include <vector>
#include <string>
#include <map>
#include <set>
#include <limits>

#include <inttypes.h>
#include <string.h>
#include <unistd.h>

#include "utils.h"

// sample packet size
//#define PKT_SAMPLING

namespace rpc {

#ifdef PKT_SAMPLING

void _pkt_sample_in(size_t size);
void _pkt_sample_out(size_t size);

#endif // PKT_SAMPLING

struct io_ratelimit {
    int min_size;
    double interval;

    io_ratelimit(): min_size(-1), interval(-1) {}
};

// not thread safe, for better performance
class FastMarshal: public NoCopy {
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

    struct chunk: public NoCopy {
        raw_bytes* data;
        size_t read_idx;
        size_t write_idx;
        bool rdonly;
        chunk* next;

        chunk(): data(new raw_bytes), read_idx(0), write_idx(0), rdonly(false), next(nullptr) {}
        chunk(const void* p, size_t n): data(new raw_bytes(p, n)), read_idx(0), write_idx(n), rdonly(false), next(nullptr) {}
        ~chunk() { data->release(); }

    private:
        // make readonly copy
        chunk(raw_bytes* data, size_t read_idx, size_t write_idx)
                : data((raw_bytes *) data->ref_copy()), read_idx(read_idx), write_idx(write_idx), rdonly(true), next(nullptr) {
            assert(write_idx <= data->size);
            assert(read_idx <= write_idx);
        }
    public:
        chunk* rdonly_copy() const {
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

            verify(!rdonly);
            char* p = &data->ptr[write_idx];
            write_idx++;

            assert(write_idx <= data->size);
            assert(read_idx <= write_idx);
            return p;
        }

        size_t write(const void* p, size_t n) {
            assert(write_idx <= data->size);
            assert(read_idx <= write_idx);

            verify(!rdonly);
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

        int write_to_fd(int fd, size_t barrier = std::numeric_limits<size_t>::max()) {
            assert(write_idx <= data->size);
            barrier = std::min(write_idx, barrier);
            assert(read_idx <= barrier);

            int cnt = 0;
            if (barrier > read_idx) {
                cnt = ::write(fd, data->ptr + read_idx, barrier - read_idx);
                if (cnt > 0) {
                    read_idx += cnt;

#ifdef PKT_SAMPLING
                    _pkt_sample_out(cnt);
#endif // PKT_SAMPLING
                }
            }

            assert(write_idx <= data->size);
            assert(read_idx <= barrier);
            return cnt;
        }

        int read_from_fd(int fd) {
            assert(write_idx <= data->size);
            assert(read_idx <= write_idx);

            verify(!rdonly);
            int cnt = 0;
            if (write_idx < data->size) {
                cnt = ::read(fd, data->ptr + write_idx, data->size - write_idx);
                if (cnt > 0) {
                    write_idx += cnt;

#ifdef PKT_SAMPLING
                    _pkt_sample_in(cnt);
#endif // PKT_SAMPLING
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

    chunk* head_;
    chunk* tail_;
    i32 write_cnt_;
    double last_write_fd_tm_;

public:

    struct read_barrier {
        raw_bytes* rb_data;
        size_t rb_idx;

        read_barrier(): rb_data(nullptr), rb_idx(0) {}
    };

    struct bookmark: public NoCopy {
        size_t size;
        char** ptr;

        ~bookmark() {
            delete[] ptr;
        }
    };

    FastMarshal(): head_(nullptr), tail_(nullptr), write_cnt_(0), last_write_fd_tm_(-1) {}
    ~FastMarshal();

    bool empty() const {
        return !content_size_gt(0);
    }
    bool content_size_gt(size_t n) const;
    size_t content_size() const;

    size_t write(const void* p, size_t n);
    size_t read(void* p, size_t n);
    size_t peek(void* p, size_t n) const;

    size_t read_from_fd(int fd);

    // this must be newly created empty FastMarshal, m must have at least n bytes of data
    size_t read_from_marshal(FastMarshal& m, size_t n);

    // write content to fd with a read_barrier, which avoid modification on tail_ by
    // output thread, thus does not require locking on tail_
    read_barrier get_read_barrier();
    size_t write_to_fd(int fd, const read_barrier& rb, const io_ratelimit& rate);

    bookmark* set_bookmark(size_t n);
    void write_bookmark(bookmark* bm, const void* p) {
        const char* pc = (const char *) p;
        assert(bm != nullptr && bm->ptr != nullptr && p != nullptr);
        for (size_t i = 0; i < bm->size; i++) {
            *(bm->ptr[i]) = pc[i];
        }
    }

    i32 get_and_reset_write_cnt() {
        i32 cnt = write_cnt_;
        write_cnt_ = 0;
        return cnt;
    }
};


/**
 * Not thread safe, for better performance.
 */
class Chunk: public NoCopy {
public:

    char* data_;
    int size_;

    // Content range [read_idx_, write_idx_)
    int read_idx_;
    int write_idx_;

    static const int min_size;

    Chunk(int size = Chunk::min_size)
            : read_idx_(0), write_idx_(0) {
        size_ = std::max(Chunk::min_size, size);
        data_ = new char[size_];
    }

    /**
     * Construct by copying a buffer.
     */
    Chunk(const void* p, int n)
            : read_idx_(0), write_idx_(n) {
        size_ = std::max(Chunk::min_size, n);
        data_ = new char[size_];
        memcpy(data_, p, n);
    }

    ~Chunk() {
        delete[] data_;
    }

    size_t content_size() const {
        assert(write_idx_ <= size_);
        assert(read_idx_ <= write_idx_);

        return write_idx_ - read_idx_;
    }

    char* set_bookmark() {
        assert(write_idx_ <= size_);
        assert(read_idx_ <= write_idx_);

        char* p = &data_[write_idx_];

        write_idx_++;

        assert(write_idx_ <= size_);
        assert(read_idx_ <= write_idx_);

        return p;
    }

    int write(const void* p, int n) {

        assert(write_idx_ <= size_);
        assert(read_idx_ <= write_idx_);

        int n_write = std::min(n, size_ - write_idx_);
        memcpy(data_ + write_idx_, p, n_write);

        write_idx_ += n_write;

        assert(write_idx_ <= size_);
        assert(read_idx_ <= write_idx_);

        return n_write;
    }

    int read(void* p, int n) {

        assert(write_idx_ <= size_);
        assert(read_idx_ <= write_idx_);

        int n_read = std::min(n, write_idx_ - read_idx_);
        memcpy(p, data_ + read_idx_, n_read);
        read_idx_ += n_read;

        assert(write_idx_ <= size_);
        assert(read_idx_ <= write_idx_);

        return n_read;
    }

    int peek(void* p, int n) const {

        assert(write_idx_ <= size_);
        assert(read_idx_ <= write_idx_);

        int n_peek = std::min(n, write_idx_ - read_idx_);
        memcpy(p, data_ + read_idx_, n_peek);

        return n_peek;
    }

    int write_to_fd(int fd) {
        assert(write_idx_ <= size_);
        assert(read_idx_ <= write_idx_);

        int r = ::write(fd, data_ + read_idx_, write_idx_ - read_idx_);
        if (r > 0) {
            read_idx_ += r;

#ifdef PKT_SAMPLING
            _pkt_sample_out(r);
#endif // PKT_SAMPLING

        }

        assert(write_idx_ <= size_);
        assert(read_idx_ <= write_idx_);

        return r;
    }

    int read_from_fd(int fd) {

        assert(write_idx_ <= size_);
        assert(read_idx_ <= write_idx_);

        int r = ::read(fd, data_ + write_idx_, size_ - write_idx_);
        if (r > 0) {
            write_idx_ += r;

#ifdef PKT_SAMPLING
            _pkt_sample_in(r);
#endif // PKT_SAMPLING

        }

        assert(write_idx_ <= size_);
        assert(read_idx_ <= write_idx_);

        return r;
    }

    int read_from_chunk(Chunk& chnk, int n) {

        assert(write_idx_ <= size_);
        assert(read_idx_ <= write_idx_);

        assert(chnk.write_idx_ <= chnk.size_);
        assert(chnk.read_idx_ <= chnk.write_idx_);

        int writable_size = size_ - write_idx_;
        int readable_size = chnk.write_idx_ - chnk.read_idx_;

        int n_read = std::min(std::min(writable_size, readable_size), n);

        memcpy(data_ + write_idx_, chnk.data_ + chnk.read_idx_, n_read);
        write_idx_ += n_read;
        chnk.read_idx_ += n_read;

        assert(write_idx_ <= size_);
        assert(read_idx_ <= write_idx_);

        assert(chnk.write_idx_ <= chnk.size_);
        assert(chnk.read_idx_ <= chnk.write_idx_);

        return n_read;
    }

    /**
     * Check if it is not possible to write to the chunk anymore.
     */
    bool fully_written() const {
        assert(write_idx_ <= size_);
        assert(read_idx_ <= write_idx_);

        return write_idx_ == size_;
    }

    /**
     * Check if it is not possible to read from the chunk anymore, even if
     * we retry later.
     */
    bool fully_read() const {
        assert(write_idx_ <= size_);
        assert(read_idx_ <= write_idx_);

        return read_idx_ == size_;
    }
};

/**
 * Not thread safe, for better performance.
 */
class Marshal1: public NoCopy {

    std::list<Chunk*> chunk_;
    i32 write_counter_;
    double last_write_fd_tm_;

public:

    class Bookmark: public NoCopy {
    public:
        int size_;
        char** ptr_;

        Bookmark()
                : size_(-1), ptr_(NULL) {
        }
        ~Bookmark() {
            delete[] ptr_;
        }
    };

    Marshal1() : write_counter_(0), last_write_fd_tm_(0) { }
    Marshal1(const std::string& data) : write_counter_(0), last_write_fd_tm_(0) {
        chunk_.push_back(new Chunk(&data[0], data.length()));
    }
    ~Marshal1() {
        for (auto it : chunk_) {
            delete it;
        }
    }

    /**
     * Note: Need to delete the bookmark manually.
     */
    Bookmark* set_bookmark(int size);
    void write_bookmark(Bookmark*, const void*);

    i32 get_write_counter_and_reset() {
        i32 value = write_counter_;
        write_counter_ = 0;
        return value;
    }

    int write(const void* p, int n);
    int read(void* p, int n);
    int peek(void* p, int n) const;

    int read_from_marshal(Marshal1&, int n = std::numeric_limits<int>::max());
    int read_from_fd(int fd);
    int write_to_fd(int fd, const io_ratelimit& rate);

    /**
     * Faster way to check if content_size > size.
     */
    bool content_size_gt(int size) const;

    size_t content_size() const;

    bool empty() const {
        return !content_size_gt(0);
    }
};

//typedef Marshal1 Marshal;
typedef FastMarshal Marshal;

inline rpc::Marshal& operator <<(rpc::Marshal& m, const rpc::i32& v) {
    verify(m.write(&v, sizeof(v)) == sizeof(v));
    return m;
}

inline rpc::Marshal& operator <<(rpc::Marshal& m, const rpc::i64& v) {
    verify(m.write(&v, sizeof(v)) == sizeof(v));
    return m;
}

inline rpc::Marshal& operator <<(rpc::Marshal& m, const double& v) {
    verify(m.write(&v, sizeof(v)) == sizeof(v));
    return m;
}

inline rpc::Marshal& operator <<(rpc::Marshal& m, const std::string& v) {
    rpc::i32 len = (rpc::i32) v.length();
    m << len;
    if (len > 0) {
        verify(m.write(v.c_str(), len) == len);
    }
    return m;
}

template<class T>
inline rpc::Marshal& operator <<(rpc::Marshal& m, const std::vector<T>& v) {
    rpc::i32 len = (rpc::i32) v.size();
    m << len;
    for (typename std::vector<T>::const_iterator it = v.begin(); it != v.end(); ++it) {
        m << *it;
    }
    return m;
}

template<class T>
inline rpc::Marshal& operator <<(rpc::Marshal& m, const std::list<T>& v) {
    rpc::i32 len = (rpc::i32) v.size();
    m << len;
    for (typename std::list<T>::const_iterator it = v.begin(); it != v.end(); ++it) {
        m << *it;
    }
    return m;
}

template<class T>
inline rpc::Marshal& operator <<(rpc::Marshal& m, const std::set<T>& v) {
    rpc::i32 len = (rpc::i32) v.size();
    m << len;
    for (typename std::set<T>::const_iterator it = v.begin(); it != v.end(); ++it) {
        m << *it;
    }
    return m;
}

template<class K, class V>
inline rpc::Marshal& operator <<(rpc::Marshal& m, const std::map<K, V>& v) {
    rpc::i32 len = (rpc::i32) v.size();
    m << len;
    for (typename std::map<K, V>::const_iterator it = v.begin(); it != v.end(); ++it) {
        m << it->first << it->second;
    }
    return m;
}

inline rpc::Marshal& operator >>(rpc::Marshal& m, rpc::i32& v) {
    verify(m.read(&v, sizeof(v)) == sizeof(v));
    return m;
}

inline rpc::Marshal& operator >>(rpc::Marshal& m, rpc::i64& v) {
    verify(m.read(&v, sizeof(v)) == sizeof(v));
    return m;
}

inline rpc::Marshal& operator >>(rpc::Marshal& m, double& v) {
    verify(m.read(&v, sizeof(v)) == sizeof(v));
    return m;
}

inline rpc::Marshal& operator >>(rpc::Marshal& m, std::string& v) {
    rpc::i32 len;
    m >> len;
    v.resize(len);
    if (len > 0) {
        verify(m.read(&v[0], len) == len);
    }
    return m;
}

template<class T>
inline rpc::Marshal& operator >>(rpc::Marshal& m, std::vector<T>& v) {
    rpc::i32 len;
    verify(m.read(&len, sizeof(len)) == sizeof(len));
    v.clear();
    v.reserve(len);
    for (rpc::i32 i = 0; i < len; i++) {
        T elem;
        m >> elem;
        v.push_back(elem);
    }
    return m;
}

template<class T>
inline rpc::Marshal& operator >>(rpc::Marshal& m, std::list<T>& v) {
    rpc::i32 len;
    verify(m.read(&len, sizeof(len)) == sizeof(len));
    v.clear();
    for (rpc::i32 i = 0; i < len; i++) {
        T elem;
        m >> elem;
        v.push_back(elem);
    }
    return m;
}

template<class T>
inline rpc::Marshal& operator >>(rpc::Marshal& m, std::set<T>& v) {
    rpc::i32 len;
    verify(m.read(&len, sizeof(len)) == sizeof(len));
    v.clear();
    for (rpc::i32 i = 0; i < len; i++) {
        T elem;
        m >> elem;
        v.insert(elem);
    }
    return m;
}

template<class K, class V>
inline rpc::Marshal& operator >>(rpc::Marshal& m, std::map<K, V>& v) {
    rpc::i32 len;
    verify(m.read(&len, sizeof(len)) == sizeof(len));
    v.clear();
    for (rpc::i32 i = 0; i < len; i++) {
        K key;
        V value;
        m >> key >> value;

        // http://stackoverflow.com/questions/4623610/c-value-type-versus-make-pair-which-is-faster-for-map-insert
        // * value_type instead of make_pair to avoid potential additional copy
        // * insert(...) is faster then []= when adding (not updating) elements
        v.insert(std::map<K, V>::value_type(key, value));
    }
    return m;
}

} // namespace rpc
