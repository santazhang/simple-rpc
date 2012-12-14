#pragma once

#include <list>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>

#ifdef NDEBUG
#define VERIFY(expr, ...) do { if (!(expr)) { printf(__VA_ARGS__); abort(); } } while (0)
#else
#define VERIFY(expr, ...) assert(expr)
#endif

typedef int32_t i32;
typedef int64_t i64;

class NoCopy {
    NoCopy(const NoCopy&);
    const NoCopy& operator =(const NoCopy&);
protected:
    NoCopy() {}
    virtual ~NoCopy() = 0;
};
inline NoCopy::~NoCopy() {}

class ScopedLock {
    pthread_mutex_t* m_;
public:
    ScopedLock(pthread_mutex_t* m): m_(m) {
        pthread_mutex_lock(m_);
    }

    ~ScopedLock() {
        pthread_mutex_unlock(m_);
    }
};

struct BufferSegment;
class Buffer {
    std::list<BufferSegment*> buf_;
    pthread_mutex_t m_;
public:
    Buffer() {
        pthread_mutex_init(&m_, NULL);
    }
    Buffer(const Buffer& b);
    ~Buffer();

    const Buffer& operator =(const Buffer& b);

    void write(const void* p, int n);
    int peek(void* p, int n) const;
    int consume(void* p, int n);
    int discard(int n);
    int size() const;
    bool empty() const;
};

class Log {
private:
    // make sure that Log cannot be instantiated
    virtual ~Log() = 0;
public:
    static void fatal(const char*, ...);
    static void error(const char*, ...);
    static void warn(const char*, ...);
    static void info(const char*, ...);
    static void debug(const char*, ...);
};

template<class T>
class SmartPtr {
    T* p_;
    int* n_;
    pthread_mutex_t* m_;

    void decr_ref() {
        pthread_mutex_lock(m_);
        (*n_)--;
        pthread_mutex_unlock(m_);
        if (*n_ == 0) {
            pthread_mutex_destroy(m_);
            delete p_;
        }
    }
public:
    SmartPtr(T* p): p_(p) {
        n_ = new int;
        *n_ = 1;
        m_ = new pthread_mutex_t;
        pthread_mutex_init(m_, NULL);
    }
    SmartPtr(const SmartPtr<T>& sp): p_(sp.p_), n_(sp.n_), m_(sp.m_) {
        ScopedLock sl(m_);
        (*n_)++;
    }
    ~SmartPtr() {
        decr_ref();
    }

    const SmartPtr& operator =(const SmartPtr<T>& sp) {
        if (this != &sp) {
            decr_ref();
            p_ = sp.p_;
            n_ = sp.n_;
            m_ = sp.m_;
            ScopedLock sl(m_);
            (*n_)++;
        }
        return *this;
    }

    T& operator *() {
        return *p_;
    }

    T* operator ->() {
        return p_;
    }
};


struct Runnable {
    virtual ~Runnable() = 0;
    virtual void run() = 0;
};
inline Runnable::~Runnable() {}


template<class T>
class Queue {
    std::list<T> q_;
    pthread_cond_t not_empty_;
    pthread_mutex_t m_;
public:
    Queue() {
        pthread_mutex_init(&m_, NULL);
        pthread_cond_init(&not_empty_, NULL);
    }
    ~Queue() {
        pthread_cond_destroy(&not_empty_);
        pthread_mutex_destroy(&m_);
    }

    void push(T e) {
        {
            ScopedLock sl(&m_);
            q_.push_back(e);
        }
        pthread_cond_signal(&not_empty_);
    }

    T pop() {
        ScopedLock sl(&m_);
        while (q_.empty()) {
            pthread_cond_wait(&not_empty_, &m_);
        }
        T e = q_.front();
        q_.pop_front();
        return e;
    }
};


class ThreadPool: public NoCopy {
    Queue<Runnable*> jobs_;
    pthread_t* th_;
    int size_;

    static void* start_thread_pool(void*);
    void start();

public:
    ThreadPool(int size = 64);
    ~ThreadPool();

    void run_async(Runnable* j) {
        jobs_.push(j);
    }
};

class Counter {
    i64 x_;
    pthread_mutex_t m_;
public:
    Counter(): x_(0) {
        pthread_mutex_init(&m_, NULL);
    }
    ~Counter() {
        pthread_mutex_destroy(&m_);
    }
    i64 next() {
        ScopedLock sl(&m_);
        return x_++;
    }
};

int setnonblocking(int fd, bool nonblocking);

