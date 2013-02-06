#pragma once

#include <list>

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <pthread.h>
#include <inttypes.h>

/**
 * Use assert() when the test is only intended for debugging.
 * Use verify() when the test is crucial for both debug and release binary.
 */
#ifdef NDEBUG
#define verify(expr, ...) do { if (!(expr)) { printf(__VA_ARGS__); abort(); } } while (0)
#else
#define verify(expr, ...) assert(expr)
#endif

#define Pthread_mutex_init(m, attr) verify(pthread_mutex_init(m, attr) == 0)
#define Pthread_mutex_lock(m) verify(pthread_mutex_lock(m) == 0)
#define Pthread_mutex_unlock(m) verify(pthread_mutex_unlock(m) == 0)
#define Pthread_mutex_destroy(m) verify(pthread_mutex_destroy(m) == 0)
#define Pthread_cond_init(c, attr) verify(pthread_cond_init(c, attr) == 0)
#define Pthread_cond_destroy(c) verify(pthread_cond_destroy(c) == 0)
#define Pthread_cond_signal(c) verify(pthread_cond_signal(c) == 0)
#define Pthread_cond_wait(c, m) verify(pthread_cond_wait(c, m) == 0)
#define Pthread_create(th, attr, func, arg) verify(pthread_create(th, attr, func, arg) == 0)
#define Pthread_join(th, attr) verify(pthread_join(th, attr) == 0)

namespace rpc {

typedef int32_t i32;
typedef int64_t i64;

class Log {
    static int level;
    static FILE* fp;
    static pthread_mutex_t m;

    static void log_v(int level, const char* fmt, va_list args);
public:

    enum {
        FATAL = 0, ERROR = 1, WARN = 2, INFO = 3, DEBUG = 4
    };

    static void set_file(FILE* fp);
    static void set_level(int level);
    static void log(int level, const char* fmt, ...);

    static void fatal(const char* fmt, ...);
    static void error(const char* fmt, ...);
    static void warn(const char* fmt, ...);
    static void info(const char* fmt, ...);
    static void debug(const char* fmt, ...);
};

class NoCopy {
    NoCopy(const NoCopy&);
    const NoCopy& operator =(const NoCopy&);
protected:
    NoCopy() {
    }
    virtual ~NoCopy() = 0;
};
inline NoCopy::~NoCopy() {
}

class Runnable {
public:
    virtual ~Runnable() {
    }
    virtual void run() = 0;
};

/**
 * Note: All sub class of RefCounted *MUST* have protected destructor!
 * This prevents accidentally deleting the object.
 * You are only allowed to cleanup with release() call.
 * This is thread safe.
 */
class RefCounted: public NoCopy {
    pthread_mutex_t m_;
    int refcnt_;

protected:

    virtual ~RefCounted() {
        Pthread_mutex_destroy(&m_);
    }

public:

    RefCounted()
            : refcnt_(1) {
        Pthread_mutex_init(&m_, NULL);
    }

    RefCounted* ref_copy() {
        Pthread_mutex_lock(&m_);
        refcnt_++;
        Pthread_mutex_unlock(&m_);
        return this;
    }

    void release() {
        Pthread_mutex_lock(&m_);
        refcnt_--;
        verify(refcnt_ >= 0);
        bool should_delete = (refcnt_ == 0);
        Pthread_mutex_unlock(&m_);
        if (should_delete) {
            delete this;
        }
    }
};

/**
 * Thread safe queue.
 */
template<class T>
class Queue {
    std::list<T> q_;
    pthread_cond_t not_empty_;
    pthread_mutex_t m_;

public:

    Queue() {
        Pthread_mutex_init(&m_, NULL);
        Pthread_cond_init(&not_empty_, NULL);
    }

    ~Queue() {
        Pthread_cond_destroy(&not_empty_);
        Pthread_mutex_destroy(&m_);
    }

    void push(const T& e) {
        Pthread_mutex_lock(&m_);
        q_.push_back(e);
        Pthread_cond_signal(&not_empty_);
        Pthread_mutex_unlock(&m_);
    }

    T pop() {
        Pthread_mutex_lock(&m_);
        while (q_.empty()) {
            Pthread_cond_wait(&not_empty_, &m_);
        }
        T e = q_.front();
        q_.pop_front();
        Pthread_mutex_unlock(&m_);
        return e;
    }
};

class ThreadPool {
    int n_;
    pthread_t* th_;
    Queue<Runnable*>* q_;

    static void* start_thread_pool(void*);
    void run_thread(int tid);

public:
    ThreadPool(int n = 64);
    ~ThreadPool();

    // NOTE: Runnable* will be deleted after execution.
    void run_async(Runnable*);
};

class Counter {
    i64 next_;
    pthread_mutex_t m_;
public:
    Counter()
            : next_(0) {
        Pthread_mutex_init(&m_, NULL);
    }
    ~Counter() {
        Pthread_mutex_destroy(&m_);
    }
    i64 next() {
        Pthread_mutex_lock(&m_);
        i64 r = next_++;
        Pthread_mutex_unlock(&m_);
        return r;
    }
};

int set_nonblocking(int fd, bool nonblocking);

}

