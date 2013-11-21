#include <sstream>

#include <sys/time.h>

#include "marshal.h"

using namespace std;

namespace rpc {

/**
 * 8kb minimum chunk size.
 * NOTE: this value directly affects how many read/write syscall will be issued.
 */
const size_t Marshal::raw_bytes::min_size = 8192;

Marshal::~Marshal() {
    chunk* chnk = head_;
    while (chnk != nullptr) {
        chunk* next = chnk->next;
        delete chnk;
        chnk = next;
    }
}

bool Marshal::content_size_ge(size_t n) const {
    size_t sz = 0;
    chunk* chnk = head_;
    while (chnk != nullptr) {
        sz += chnk->content_size();
        if (sz >= n) {
            return true;
        }
        chnk = chnk->next;
    }
    return sz >= n;
}

size_t Marshal::content_size() const {
    assert(tail_ == nullptr || tail_->next == nullptr);

    size_t sz = 0;
    chunk* chnk = head_;
    while (chnk != nullptr) {
        sz += chnk->content_size();
        chnk = chnk->next;
    }
    return sz;
}

size_t Marshal::write(const void* p, size_t n) {
    assert(tail_ == nullptr || tail_->next == nullptr);

    if (head_ == nullptr) {
        assert(tail_ == nullptr);

        head_ = new chunk(p, n);
        tail_ = head_;
    } else if (tail_->fully_written()) {
        tail_->next = new chunk(p, n);
        tail_ = tail_->next;
    } else {
        size_t n_write = tail_->write(p, n);

        // otherwise the above fully_written() should have returned true
        assert(n_write > 0);

        if (n_write < n) {
            const char* pc = (const char *) p;
            tail_->next = new chunk(pc + n_write, n - n_write);
            tail_ = tail_->next;
        }
    }
    write_cnt_ += n;

    return n;
}

size_t Marshal::read(void* p, size_t n) {
    assert(tail_ == nullptr || tail_->next == nullptr);
    assert(empty() || (head_ != nullptr && !head_->fully_read()));

    char* pc = (char *) p;
    size_t n_read = 0;
    while (n_read < n && head_ != nullptr && head_->content_size() > 0) {
        size_t cnt = head_->read(pc + n_read, n - n_read);
        if (head_->fully_read()) {
            if (tail_ == head_) {
                // deleted the only chunk
                tail_ = nullptr;
            }
            chunk* chnk = head_;
            head_ = head_->next;
            delete chnk;
        }
        if (cnt == 0) {
            // currently there's no data available, so stop
            break;
        }
        n_read += cnt;
    }
    assert(n_read <= n);
    assert(tail_ == nullptr || tail_->next == nullptr);
    assert(empty() || (head_ != nullptr && !head_->fully_read()));

    return n_read;
}

size_t Marshal::peek(void* p, size_t n) const {
    assert(tail_ == nullptr || tail_->next == nullptr);
    assert(empty() || (head_ != nullptr && !head_->fully_read()));

    char* pc = (char *) p;
    size_t n_peek = 0;
    chunk* chnk = head_;
    while (chnk != nullptr && n - n_peek > 0) {
        size_t cnt = chnk->peek(pc + n_peek, n - n_peek);
        if (cnt == 0) {
            // no more data to peek, quit
            break;
        }
        n_peek += cnt;
        chnk = chnk->next;
    }

    assert(n_peek <= n);
    assert(tail_ == nullptr || tail_->next == nullptr);
    assert(empty() || (head_ != nullptr && !head_->fully_read()));
    return n_peek;
}

size_t Marshal::read_from_fd(int fd) {
    assert(empty() || (head_ != nullptr && !head_->fully_read()));

    size_t n_bytes = 0;
    for (;;) {
        if (head_ == nullptr) {
            head_ = new chunk;
            tail_ = head_;
        } else if (tail_->fully_written()) {
            tail_->next = new chunk;
            tail_ = tail_->next;
        }
        int r = tail_->read_from_fd(fd);
        if (r <= 0) {
            break;
        }
        n_bytes += r;
    }
    write_cnt_ += n_bytes;

    assert(empty() || (head_ != nullptr && !head_->fully_read()));
    return n_bytes;
}

size_t Marshal::read_from_marshal(Marshal& m, size_t n) {
    assert(head_ == nullptr && tail_ == nullptr);
    assert(n == 0 || m.content_size_gt(n - 1));   // require m.content_size() >= n > 0

    size_t n_fetch = 0;
    while (n_fetch < n) {
        chunk* chnk = m.head_->rdonly_copy();
        if (n_fetch + chnk->content_size() > n) {
            // only fetch enough bytes we need
            chnk->write_idx -= (n_fetch + chnk->content_size()) - n;
        }
        size_t cnt = chnk->content_size();
        assert(cnt > 0);
        n_fetch += cnt;
        verify(m.head_->discard(cnt) == cnt);
        if (head_ == nullptr) {
            head_ = tail_ = chnk;
        } else {
            tail_->next = chnk;
            tail_ = chnk;
        }
        if (m.head_->fully_read()) {
            if (m.tail_ == m.head_) {
                // deleted the only chunk
                m.tail_ = nullptr;
            }
            chunk* next = m.head_->next;
            delete m.head_;
            m.head_ = next;
        }
    }
    write_cnt_ += n_fetch;

    assert(n_fetch == n);
    return n_fetch;
}


size_t Marshal::write_to_fd(int fd) {
    size_t n_write = 0;
    while (!empty()) {
        int cnt = head_->write_to_fd(fd);
        if (head_->fully_read()) {
            chunk* chnk = head_;
            head_ = head_->next;
            delete chnk;
        }
        if (cnt <= 0) {
            // currently there's no data available, so stop
            break;
        }
        n_write += cnt;
    }

    return n_write;
}

Marshal::bookmark* Marshal::set_bookmark(size_t n) {
    verify(write_cnt_ == 0);

    bookmark* bm = new bookmark;
    bm->size = n;
    bm->ptr = new char*[bm->size];
    for (size_t i = 0; i < n; i++) {
        if (head_ == nullptr) {
            head_ = new chunk;
            tail_ = head_;
        } else if (tail_->fully_written()) {
            tail_->next = new chunk;
            tail_ = tail_->next;
        }
        bm->ptr[i] = tail_->set_bookmark();
    }

    return bm;
}

} // namespace rpc
