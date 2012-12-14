#include <fcntl.h>
#include <stdio.h>
#include <stdarg.h>

#include "utils.h"

using namespace std;

struct BufferSegment : public NoCopy {
    int read_idx_;
    int write_idx_;
    int size_;
    char* data_;
    static const int k_default_size_;

    BufferSegment(): read_idx_(0), write_idx_(0), size_(k_default_size_), data_(new char[size_]) {}
    BufferSegment(const void* data, int n): read_idx_(0), write_idx_(n), size_(::max(n, k_default_size_)), data_(new char[size_]) {
        memcpy(data_, data, n);
    }
    ~BufferSegment() {
        VERIFY(read_idx_ <= write_idx_);
        VERIFY(write_idx_ <= size_);
        delete [] data_;
    }

    BufferSegment* clone() const {
        return new BufferSegment(data_ + read_idx_, write_idx_ - read_idx_);
    }

    int size() const {
        VERIFY(read_idx_ <= write_idx_);
        VERIFY(write_idx_ <= size_);
        return write_idx_ - read_idx_;
    }

    bool full() const {
        VERIFY(read_idx_ <= write_idx_);
        VERIFY(write_idx_ <= size_);
        return write_idx_ == size_;
    }

    bool consumed() const {
        VERIFY(read_idx_ <= write_idx_);
        VERIFY(write_idx_ <= size_);
        return read_idx_ == size_;
    }

    int write(const void* p, int n) {
        VERIFY(read_idx_ <= write_idx_);
        VERIFY(write_idx_ <= size_);
        int cnt = ::min(size_ - write_idx_, n);
        if (cnt > 0) {
            memcpy(data_ + write_idx_, p, cnt);
            write_idx_ += cnt;
        }
        VERIFY(write_idx_ <= size_);
        return cnt;
    }

    int peek(void* p, int n) const {
        VERIFY(read_idx_ <= write_idx_);
        VERIFY(write_idx_ <= size_);
        int cnt = ::min(write_idx_ - read_idx_, n);
        if (cnt > 0) {
            memcpy(p, data_ + read_idx_, cnt);
        }
        VERIFY(read_idx_ <= write_idx_);
        return cnt;
    }

    int consume(void* p, int n) {
        VERIFY(read_idx_ <= write_idx_);
        VERIFY(write_idx_ <= size_);
        int cnt = ::min(write_idx_ - read_idx_, n);
        if (cnt > 0) {
            memcpy(p, data_ + read_idx_, cnt);
            read_idx_ += cnt;
        }
        VERIFY(read_idx_ <= write_idx_);
        return cnt;
    }

    int discard(int n) {
        VERIFY(read_idx_ <= write_idx_);
        VERIFY(write_idx_ <= size_);
        int cnt = ::min(write_idx_ - read_idx_, n);
        if (cnt > 0) {
            read_idx_ += cnt;
        }
        VERIFY(read_idx_ <= write_idx_);
        return cnt;
    }
};
const int BufferSegment::k_default_size_ = 1024;

Buffer::Buffer(const Buffer& b) {
    pthread_mutex_init(&m_, NULL);
    ScopedLock sl1(&m_);
    ScopedLock sl2(const_cast<pthread_mutex_t*>(&b.m_));
    for (list<BufferSegment*>::const_iterator it = b.buf_.begin(); it != b.buf_.end(); ++it) {
        buf_.push_back((*it)->clone());
    }
}

Buffer::~Buffer() {
    pthread_mutex_destroy(&m_);
    for (std::list<BufferSegment*>::iterator it = buf_.begin(); it != buf_.end(); ++it) {
        delete *it;
    }
}

const Buffer& Buffer::operator =(const Buffer& b) {
    if (&b != this) {
        ScopedLock sl1(&m_);
        ScopedLock sl2(const_cast<pthread_mutex_t*>(&b.m_));
        // destroy old data
        while (!buf_.empty()) {
            BufferSegment* s = buf_.front();
            buf_.pop_front();
            delete s;
        }
        // copy new data
        for (list<BufferSegment*>::const_iterator it = b.buf_.begin(); it != b.buf_.end(); ++it) {
            buf_.push_back((*it)->clone());
        }
    }
    return *this;
}

void Buffer::write(const void* p, int n) {
    ScopedLock sl(&m_);
    if (buf_.empty()) {
        buf_.push_back(new BufferSegment);
    }
    BufferSegment* s = buf_.back();
    int cnt = s->write(p, n);
    if (cnt < n) {
        buf_.push_back(new BufferSegment((char *)p + cnt, n - cnt));
    }
}

int Buffer::peek(void* p, int n) const {
    ScopedLock sl(const_cast<pthread_mutex_t*>(&m_));
    int cnt = 0;
    for (std::list<BufferSegment*>::const_iterator it = buf_.begin(); it != buf_.end(); ++it) {
        int peek_n = (*it)->peek((char *)p + cnt, n - cnt);
        cnt += peek_n;
        if (cnt == n) {
            break;
        }
    }
    return cnt;
}

int Buffer::consume(void* p, int n) {
    ScopedLock sl(&m_);
    int cnt = 0;
    for (std::list<BufferSegment*>::iterator it = buf_.begin(); it != buf_.end(); ++it) {
        int consume_n = (*it)->consume((char *)p + cnt, n - cnt);
        cnt += consume_n;
        if (cnt == n) {
            break;
        }
    }
    while (!buf_.empty()) {
        BufferSegment* seg = *buf_.begin();
        if (seg->consumed()) {
            buf_.pop_front();
            delete seg;
        } else {
            break;
        }
    }
    return cnt;
}

int Buffer::discard(int n) {
    ScopedLock sl(&m_);
    int cnt = 0;
    for (std::list<BufferSegment*>::iterator it = buf_.begin(); it != buf_.end(); ++it) {
        int discard_n = (*it)->discard(n - cnt);
        cnt += discard_n;
        if (cnt == n) {
            break;
        }
    }
    while (!buf_.empty()) {
        BufferSegment* seg = *buf_.begin();
        if (seg->consumed()) {
            buf_.pop_front();
            delete seg;
        } else {
            break;
        }
    }
    return cnt;
}

int Buffer::size() const {
    ScopedLock sl(const_cast<pthread_mutex_t*>(&m_));
    int sz = 0;
    for (std::list<BufferSegment*>::const_iterator it = buf_.begin(); it != buf_.end(); ++it) {
        sz += (*it)->size();
    }
    return sz;
}

bool Buffer::empty() const {
    ScopedLock sl(const_cast<pthread_mutex_t*>(&m_));
    if (!buf_.empty()) {
        return (*buf_.begin())->size() == 0;
    }
    return true;
}

pthread_mutex_t write_log_m_ = PTHREAD_MUTEX_INITIALIZER;

static inline void write_log(const char* prefix, const char* fmt, va_list va) {
    ScopedLock sl(&write_log_m_);
    printf("%s", prefix);
    vprintf(fmt, va);
    printf("\n");
}

void Log::fatal(const char* fmt, ...) {
    va_list va;
    va_start(va, fmt);
    write_log("F: ", fmt, va);
    va_end(va);
}

void Log::error(const char* fmt, ...) {
    va_list va;
    va_start(va, fmt);
    write_log("E: ", fmt, va);
    va_end(va);
}

void Log::warn(const char* fmt, ...) {
    va_list va;
    va_start(va, fmt);
    write_log("W: ", fmt, va);
    va_end(va);
}

void Log::info(const char* fmt, ...) {
    va_list va;
    va_start(va, fmt);
    write_log("I: ", fmt, va);
    va_end(va);
}

void Log::debug(const char* fmt, ...) {
    va_list va;
    va_start(va, fmt);
    write_log("D: ", fmt, va);
    va_end(va);
}

void* ThreadPool::start_thread_pool(void* ptr) {
    ThreadPool* thpool = (ThreadPool *) ptr;
    thpool->start();
    pthread_exit(NULL);
    return NULL;
}

ThreadPool::ThreadPool(int size /* =? */): size_(size) {
    th_ = new pthread_t[size_];
    for (int i = 0; i < size_; i++) {
        VERIFY(pthread_create(&th_[i], NULL, ThreadPool::start_thread_pool, this) == 0);
    }
}

ThreadPool::~ThreadPool() {
    for (int i = 0; i < size_; i++) {
        jobs_.push(NULL); // death pill
    }
    for (int i = 0; i < size_; i++) {
        pthread_join(th_[i], NULL);
    }
    delete [] th_;
}

void ThreadPool::start() {
    for (;;) {
        Runnable* j = jobs_.pop();
        if (j == NULL) {
            return;
        }
        j->run();
        delete j;
    }
}

int setnonblocking(int fd, bool nonblocking) {
    int ret = fcntl(fd, F_GETFL, 0);
    if (ret != -1) {
        if (nonblocking) {
            ret = fcntl(fd, F_SETFL, ret | O_NONBLOCK);
        } else {
            ret = fcntl(fd, F_SETFL, ret & ~O_NONBLOCK);
        }
    }
    return ret;
}
