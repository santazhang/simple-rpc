#include <sstream>

#include <sys/time.h>

#include "marshal.h"

using namespace std;

namespace rpc {

#ifdef PKT_SAMPLING

#define PKT_SAMPLE_SIZE 19
static size_t _pkt_sample_in_size[PKT_SAMPLE_SIZE];
static size_t _pkt_sample_out_size[PKT_SAMPLE_SIZE];

static void _pkt_sampling_report() {
    static size_t ratelimit_counter = 0;
    static int last_report_tm = 0;
    if (ratelimit_counter++ % 1024 == 0) {
        struct timeval now;
        gettimeofday(&now, NULL);
        if (now.tv_sec - last_report_tm >= 1) {
            {
                ostringstream ostr;
                for (int i = 0; i < PKT_SAMPLE_SIZE; i++) {
                    ostr << " " << _pkt_sample_in_size[i];
                }
                Log::info("PKT_SAMPLE_IN: %s", ostr.str().c_str());
            }
            {
                ostringstream ostr;
                for (int i = 0; i < PKT_SAMPLE_SIZE; i++) {
                    ostr << " " << _pkt_sample_out_size[i];
                }
                Log::info("PKT_SAMPLE_OUT:%s", ostr.str().c_str());
            }
            last_report_tm = now.tv_sec;
        }
    }
}

void _pkt_sample_in(size_t size) {
    // not thread safe, but ok, since we only need approximate results
    static size_t _pkt_sample_counter = 0;
    _pkt_sample_in_size[_pkt_sample_counter++ % PKT_SAMPLE_SIZE] = size;
    _pkt_sampling_report();
}

void _pkt_sample_out(size_t size) {
    // not thread safe, but ok, since we only need approximate results
    static size_t _pkt_sample_counter = 0;
    _pkt_sample_out_size[_pkt_sample_counter++ % PKT_SAMPLE_SIZE] = size;
    _pkt_sampling_report();
}

#endif // PKT_SAMPLING

/**
 * 8kb minimum chunk size.
 * NOTE: this value directly affects how many read/write syscall will be issued.
 */
const size_t FastMarshal::raw_bytes::min_size = 8192;

FastMarshal::~FastMarshal() {
    chunk* chnk = head_;
    while (chnk != nullptr) {
        chunk* next = chnk->next;
        delete chnk;
        chnk = next;
    }
}

bool FastMarshal::content_size_gt(size_t n) const {
    assert(tail_ == nullptr || tail_->next == nullptr);

    size_t sz = 0;
    chunk* chnk = head_;
    while (chnk != nullptr) {
        sz += chnk->content_size();
        if (sz > n) {
            return true;
        }
        chnk = chnk->next;
    }
    return sz > n;
}

size_t FastMarshal::content_size() const {
    assert(tail_ == nullptr || tail_->next == nullptr);

    size_t sz = 0;
    chunk* chnk = head_;
    while (chnk != nullptr) {
        sz += chnk->content_size();
        chnk = chnk->next;
    }
    return sz;
}

size_t FastMarshal::write(const void* p, size_t n) {
    assert(tail_ == nullptr || tail_->next == nullptr);
    assert(empty() || (head_ != nullptr && !head_->fully_read()));

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

size_t FastMarshal::read(void* p, size_t n) {
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

size_t FastMarshal::peek(void* p, size_t n) const {
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

size_t FastMarshal::read_from_fd(int fd) {
    assert(empty() || (head_ != nullptr && !head_->fully_read()));

    size_t n_read = 0;
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
        n_read += r;
    }

    assert(empty() || (head_ != nullptr && !head_->fully_read()));
    return n_read;
}

size_t FastMarshal::read_from_marshal(FastMarshal& m, size_t n) {
    assert(head_ == nullptr && tail_ == nullptr);
    assert(n == 0 || m.content_size_gt(n - 1));   // require m.content_size() >= n > 0

    size_t n_fetch = 0;
    while (n_fetch < n) {
        chunk* chnk = m.head_->rdonly_copy();
        if (n_fetch + chnk->content_size() > n) {
            // only fetch enough bytes we need
            chnk->write_idx -= (n_fetch + chnk->content_size()) - n;
        }
        assert(chnk->content_size() > 0);
        n_fetch += chnk->content_size();
        if (head_ == nullptr) {
            head_ = tail_ = chnk;
        } else {
            tail_->next = chnk;
            tail_ = chnk;
        }
        if (n_fetch < n) {
            chunk* next = m.head_->next;
            delete m.head_;
            m.head_ = next;
            if (m.head_ == nullptr) {
                // fetched the last piece of chunk
                m.tail_ = nullptr;
            }
        }
    }

    assert(n_fetch == n);
    return n_fetch;
}

FastMarshal::read_barrier FastMarshal::get_read_barrier() {
    assert(empty() || (head_ != nullptr && !head_->fully_read()));

    read_barrier rb;
    if (tail_ != nullptr) {
        // advance tail_ if it's fully written
        // this ensures that reading till read_barrier will not update tail_
        // NOTE: if tail_ is fully written, it could be drained to fully read, and need to be
        //       removed, thus tail_ need to be modified, violating read_barrier's intention
        if (tail_->fully_written()) {
            tail_->next = new chunk;
            tail_ = tail_->next;
        }

        rb.rb_data = tail_->data;
        rb.rb_idx = tail_->write_idx;
    }

    assert(empty() || (head_ != nullptr && !head_->fully_read()));
    return rb;
}

size_t FastMarshal::write_to_fd(int fd, const FastMarshal::read_barrier& rb, const io_ratelimit& rate) {
    assert(empty() || (head_ != nullptr && !head_->fully_read()));

    if (rb.rb_data == nullptr) {
        return 0;
    }

    if (rate.min_size > 0 || rate.interval > 0) {
        // rpc batching, check if should wait till next batch
        bool should_wait = true;
        if (rate.min_size > 0 && content_size_gt(rate.min_size)) {
            should_wait = false;
        }

        if (rate.interval > 0) {
            struct timeval tm;
            gettimeofday(&tm, NULL);
            double now = tm.tv_sec + tm.tv_usec / 1000.0 / 1000.0;
            if (should_wait && now - last_write_fd_tm_ > rate.interval) {
                should_wait = false;
            }
            if (should_wait == false) {
                last_write_fd_tm_ = now;
            }
        }

        if (should_wait) {
            return 0;
        }
    }

    size_t n_write = 0;
    while (!empty()) {
        int cnt;
        if (head_->data == rb.rb_data) {
            cnt = head_->write_to_fd(fd, rb.rb_idx);
        } else {
            cnt = head_->write_to_fd(fd);
        }
        if (head_->fully_read()) {
            // since read_barrier should have prevented any possibility to update (advance) tail_
            assert(tail_ != head_);

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

    assert(tail_ == nullptr || tail_->next == nullptr);
    assert(empty() || (head_ != nullptr && !head_->fully_read()));
    return n_write;
}

FastMarshal::bookmark* FastMarshal::set_bookmark(size_t n) {
    assert(empty() || (head_ != nullptr && !head_->fully_read()));
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


/**
 * 8kb minimum chunk size.
 * NOTE: this value directly affects how many read/write syscall will be issued.
 */
const int Chunk::min_size = 8192;

Marshal1::Bookmark* Marshal1::set_bookmark(int size) {
    verify(write_counter_ == 0);

    // invariant: head of chunk list is not fully read (otherwise it's a waste of memory)
    assert(chunk_.empty() || !chunk_.front()->fully_read());

    Bookmark* bmark = new Bookmark();
    bmark->size_ = size;
    bmark->ptr_ = new char*[bmark->size_];

    for (int i = 0; i < bmark->size_; i++) {
        if (chunk_.empty() || chunk_.back()->fully_written()) {
            chunk_.push_back(new Chunk);
        }
        bmark->ptr_[i] = chunk_.back()->set_bookmark();
    }

    return bmark;
}

void Marshal1::write_bookmark(Bookmark* bmark, const void* ptr) {
    char* pc = (char *) ptr;
    verify(bmark != NULL && bmark->ptr_ != NULL && bmark->size_ >= 0);
    for (int i = 0; i < bmark->size_; i++) {
        *(bmark->ptr_[i]) = pc[i];
    }
}

int Marshal1::write(const void* p, int n) {
    assert(chunk_.empty() || !chunk_.front()->fully_read());

    if (chunk_.empty() || chunk_.back()->fully_written()) {
        chunk_.push_back(new Chunk(p, n));
    } else {
        int n_write = chunk_.back()->write(p, n);

        // otherwise the above fully_written() will return true
        assert(n_write > 0);

        if (n_write < n) {
            const char* pc = (const char *) p;
            chunk_.push_back(new Chunk(pc + n_write, n - n_write));
        }
    }

    write_counter_ += n;
    return n;
}

int Marshal1::read(void* p, int n) {
    assert(chunk_.empty() || !chunk_.front()->fully_read());

    char* pc = (char *) p;
    int n_read = 0;
    while (!chunk_.empty() && n_read < n) {
        int r = chunk_.front()->read(pc + n_read, n - n_read);
        if (chunk_.front()->fully_read()) {
            // remove fully read chunks, avoid unnecessary mem usage
            delete chunk_.front();
            chunk_.pop_front();
        }
        if (r == 0) {
            // currently there's no content for us to read, so stop.
            break;
        }
        n_read += r;
    }

    verify(n_read <= n);
    assert(chunk_.empty() || !chunk_.front()->fully_read());

    return n_read;
}

int Marshal1::peek(void* p, int n) const {
    assert(chunk_.empty() || !chunk_.front()->fully_read());

    char* pc = (char *) p;
    int n_peek = 0;

    for (list<Chunk*>::const_iterator it = chunk_.begin(); it != chunk_.end(); ++it) {
        int r = (*it)->peek(pc + n_peek, n - n_peek);
        if (r == 0) {
            // no more data to peek, so stop
            break;
        }
        n_peek += r;
        if (n_peek == n) {
            // read enough data, so stop
            break;
        }
    }

    assert(n_peek <= n);
    assert(chunk_.empty() || !chunk_.front()->fully_read());

    return n_peek;
}

int Marshal1::write_to_fd(int fd, const io_ratelimit& rate) {
    assert(chunk_.empty() || !chunk_.front()->fully_read());

    if (rate.min_size > 0 || rate.interval > 0) {
        // rpc batching, check if should wait till next batch
        bool should_wait = true;
        if (rate.min_size > 0 && content_size_gt(rate.min_size)) {
            should_wait = false;
        }

        if (rate.interval > 0) {
            struct timeval tm;
            gettimeofday(&tm, NULL);
            double now = tm.tv_sec + tm.tv_usec / 1000.0 / 1000.0;
            if (should_wait && now - last_write_fd_tm_ > rate.interval) {
                should_wait = false;
            }
            if (should_wait == false) {
                last_write_fd_tm_ = now;
            }
        }

        if (should_wait) {
            return 0;
        }
    }

    int n_write = 0;
    while (!chunk_.empty()) {
        int r = chunk_.front()->write_to_fd(fd);
        if (chunk_.front()->fully_read()) {
            // remove useless chunks when they are fully read
            delete chunk_.front();
            chunk_.pop_front();
        }
        if (r <= 0) {
            break;
        }
        n_write += r;
    }

    assert(chunk_.empty() || !chunk_.front()->fully_read());

    return n_write;
}

int Marshal1::read_from_marshal(Marshal1& m, int n /* =? */) {
    assert(chunk_.empty() || !chunk_.front()->fully_read());
    assert(m.chunk_.empty() || !m.chunk_.front()->fully_read());

    int n_read = 0;
    while (n_read < n) {
        if (m.chunk_.empty()) {
            // nothing more to read
            break;
        }
        if (chunk_.empty() || chunk_.back()->fully_written()) {
            int head_size = m.chunk_.front()->content_size();
            if (head_size < n - n_read) {

                // speed up: directly transfer chunk pointer, avoid memory copying
                chunk_.push_back(m.chunk_.front());
                m.chunk_.pop_front();
                n_read += head_size;

                // skip read_from_chunk operations
                continue;

            } else {
                chunk_.push_back(new Chunk);
            }
        }
        int r = chunk_.back()->read_from_chunk(*m.chunk_.front(), n - n_read);

        if (m.chunk_.front()->fully_read()) {
            // remove useless chunks when they are fully read
            delete m.chunk_.front();
            m.chunk_.pop_front();
        }
        if (r == 0) {
            // no more data to read
            break;
        }
        n_read += r;
    }

    assert(chunk_.empty() || !chunk_.front()->fully_read());
    assert(m.chunk_.empty() || !m.chunk_.front()->fully_read());

    return n_read;
}

int Marshal1::read_from_fd(int fd) {
    assert(chunk_.empty() || !chunk_.front()->fully_read());

    int n_read = 0;
    for (;;) {
        if (chunk_.empty() || chunk_.back()->fully_written()) {
            chunk_.push_back(new Chunk);
        }
        int r = chunk_.back()->read_from_fd(fd);
        if (r <= 0) {
            break;
        }
        n_read += r;
    }

    assert(chunk_.empty() || !chunk_.front()->fully_read());

    return n_read;
}

bool Marshal1::content_size_gt(int size) const {

    int size_visited = 0;
    for (list<Chunk*>::const_iterator it = chunk_.begin(); it != chunk_.end(); ++it) {
        size_visited += (*it)->content_size();
        if (size_visited > size) {
            return true;
        }
    }

    return size_visited > size;
}

size_t Marshal1::content_size() const {
    size_t size = 0;
    for (list<Chunk*>::const_iterator it = chunk_.begin(); it != chunk_.end(); ++it) {
        size += (*it)->content_size();
    }
    return size;
}

}
