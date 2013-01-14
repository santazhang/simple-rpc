#pragma once

#include <list>
#include <vector>
#include <string>

#include <inttypes.h>
#include <string.h>
#include <unistd.h>

#include "utils.h"

namespace rpc {

class Marshal;

/**
 * Not thread safe, for better performance.
 */
class Chunk: public NoCopy {
    friend class Marshal;

    char* data_;
    int size_;

    // Content range [read_idx_, write_idx_)
    int read_idx_;
    int write_idx_;

    static const int min_size;

public:

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

    int content_size() const {
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
class Marshal: public NoCopy {

    std::list<Chunk*> chunk_;

public:

    class Bookmark: public NoCopy {
        friend class Marshal;
        int size_;
        char** ptr_;
    public:
        Bookmark()
                : size_(-1), ptr_(NULL) {
        }
        ~Bookmark() {
            delete[] ptr_;
        }
    };

    ~Marshal();

    template<class T>
    int write(const T& v);

    template<class T>
    int write(const std::vector<T>& v);

    /**
     * Note: Need to delete the bookmark manually.
     */
    Bookmark* set_bookmark(int size);
    void write_bookmark(Bookmark*, const void*);

    int write(const void* p, int n);
    int read(void* p, int n);
    int peek(void* p, int n) const;

    int read_from_marshal(Marshal&, int n);
    int read_from_fd(int fd);
    int write_to_fd(int fd);

    /**
     * Check if content size >= sz
     */
    bool content_size_gt(int size) const;

    int content_size() const;

    bool empty() const {
        return !content_size_gt(0);
    }
};

template<>
inline int Marshal::write(const i32& v) {
    int r = this->write(&v, sizeof(v));
    assert(sizeof(v) == r);
    return r;
}

template<>
inline int Marshal::write(const i64& v) {
    int r = this->write(&v, sizeof(v));
    assert(sizeof(v) == r);
    return r;
}

template<>
inline int Marshal::write(const double& v) {
    int r = this->write(&v, sizeof(v));
    assert(sizeof(v) == r);
    return r;
}

template<>
inline int Marshal::write(const std::string& v) {
    i32 len = (i32) v.length();
    int cnt = this->write(len);
    cnt += this->write(v.c_str(), len);

    assert(cnt == sizeof(i32) + len);

    return cnt;
}

template<class T>
inline int Marshal::write(const std::vector<T>& v) {
    i32 len = (i32) v.size();
    int cnt = this->write(len);
    assert(cnt == sizeof(i32));
    for (typename std::vector<T>::const_iterator it = v.begin(); it != v.end(); ++it) {
        cnt += this->write(*it);
    }
    return cnt;
}

template<class T>
inline Marshal& operator <<(Marshal& m, const T& v) {
    m.write(v);
    return m;
}

inline Marshal& operator >>(Marshal& m, i32& v) {
    verify(m.read(&v, sizeof(v)) == sizeof(v));
    return m;
}

inline Marshal& operator >>(Marshal& m, i64& v) {
    verify(m.read(&v, sizeof(v)) == sizeof(v));
    return m;
}

inline Marshal& operator >>(Marshal& m, double& v) {
    verify(m.read(&v, sizeof(v)) == sizeof(v));
    return m;
}

inline Marshal& operator >>(Marshal& m, std::string& v) {
    i32 len;
    m >> len;
    v.resize(len);
    verify(m.read(&v[0], len) == len);
    return m;
}

template<class T>
inline Marshal& operator >>(Marshal& m, std::vector<T>& v) {
    i32 len;
    verify(m.read(&len, sizeof(len)) == sizeof(len));
    v.resize(0);
    v.reserve(len);
    for (i32 i = 0; i < len; i++) {
        T elem;
        m >> elem;
        v.push_back(elem);
    }
    return m;
}

} // namespace rpc
