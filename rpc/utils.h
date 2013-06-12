#pragma once

#include <list>
#include <functional>

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <pthread.h>
#include <inttypes.h>

#include "callback.h"

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
#define Pthread_cond_broadcast(c) verify(pthread_cond_broadcast(c) == 0)
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

typedef Callback<void> Runnable;

#define RUNNABLE_CLASS1(cls, type1, arg1, run_func_body) \
    class cls: public rpc::Runnable { \
    public: \
        cls(type1 arg1): arg1(arg1) {} \
        void run() { run_func_body; } \
    private: \
        type1 arg1; \
    };

#define RUNNABLE_CLASS2(cls, type1, arg1, type2, arg2, run_func_body) \
    class cls: public rpc::Runnable { \
    public: \
        cls(type1 arg1, type2 arg2): arg1(arg1), arg2(arg2) {} \
        void run() { run_func_body; } \
    private: \
        type1 arg1; \
        type2 arg2; \
    };

#define RUNNABLE_CLASS3(cls, type1, arg1, type2, arg2, type3, arg3, run_func_body) \
    class cls: public rpc::Runnable { \
    public: \
        cls(type1 arg1, type2 arg2, type3 arg3): arg1(arg1), arg2(arg2), arg3(arg3) {} \
        void run() { run_func_body; } \
    private: \
        type1 arg1; \
        type2 arg2; \
        type3 arg3; \
    };

#define RUNNABLE_CLASS4(cls, type1, arg1, type2, arg2, type3, arg3, type4, arg4, run_func_body) \
    class cls: public rpc::Runnable { \
    public: \
        cls(type1 arg1, type2 arg2, type3 arg3, type4 arg4): arg1(arg1), arg2(arg2), arg3(arg3), arg4(arg4) {} \
        void run() { run_func_body; } \
    private: \
        type1 arg1; \
        type2 arg2; \
        type3 arg3; \
        type4 arg4; \
    };

/**
 * Note: All sub class of RefCounted *MUST* have protected destructor!
 * This prevents accidentally deleting the object.
 * You are only allowed to cleanup with release() call.
 * This is thread safe.
 */
class RefCounted: public NoCopy {
    int refcnt_;

protected:

    virtual ~RefCounted() {}

public:

    RefCounted(): refcnt_(1) {
    }

    int ref_count() {
        return refcnt_;
    }

    RefCounted* ref_copy() {
        __sync_add_and_fetch(&refcnt_, 1);
        return this;
    }

    void release() {
        int r = __sync_sub_and_fetch(&refcnt_, 1);
        verify(r >= 0);
        if (r == 0) {
            delete this;
        }
    }
};

/**
 * Thread safe queue.
 */
template<class T>
class Queue: public NoCopy {
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

    void pop_all(std::list<T>* fill) {
        Pthread_mutex_lock(&m_);
        while (q_.empty()) {
            Pthread_cond_wait(&not_empty_, &m_);
        }
        *fill = q_;
        q_.clear();
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

class ThreadPool: public RefCounted {
    int n_;
    pthread_t* th_;
    Queue<Runnable*>* q_;

    static void* start_thread_pool(void*);
    void run_thread(int tid);

protected:
    ~ThreadPool();

public:
    ThreadPool(int n = 64);

    // NOTE: Runnable* will be deleted after execution.
    void run_async(Runnable*);
    void run_async(const std::function<void()>&);
};

class Counter: public NoCopy {
    i64 next_;
    pthread_mutex_t m_;
public:
    Counter(i64 start = 0)
            : next_(start) {
        Pthread_mutex_init(&m_, NULL);
    }
    ~Counter() {
        Pthread_mutex_destroy(&m_);
    }
    i64 next(i64 step = 1) {
        Pthread_mutex_lock(&m_);
        i64 r = next_;
        next_ += step;
        Pthread_mutex_unlock(&m_);
        return r;
    }
    void reset(i64 start = 0) {
        Pthread_mutex_lock(&m_);
        next_ = start;
        Pthread_mutex_unlock(&m_);
    }
};

class Mutex {
public:
    Mutex()         { Pthread_mutex_init(&m_, NULL); }
    ~Mutex()        { Pthread_mutex_destroy(&m_); }

    void lock()     { Pthread_mutex_lock(&m_); }
    void unlock()   { Pthread_mutex_unlock(&m_); }

private:
    friend class ConditionVar;

    pthread_mutex_t m_;

    // Non-copyable, non-assignable
    Mutex(Mutex &);
    Mutex& operator=(Mutex&);
};

class ScopedLock {
public:
    explicit ScopedLock(Mutex* lock) : m_(lock) { m_->lock(); }
    ~ScopedLock()   { m_->unlock(); }

private:
    Mutex* m_;

    // Non-copyable, non-assignable
    ScopedLock(ScopedLock&);
    ScopedLock& operator=(ScopedLock&);
};

class ConditionVar {
public:
    ConditionVar()          { Pthread_cond_init(&cv_, NULL); }
    ~ConditionVar()         { Pthread_cond_destroy(&cv_); }

    void wait(Mutex* mutex) { Pthread_cond_wait(&cv_, &(mutex->m_)); }
    void signal()           { Pthread_cond_signal(&cv_); }
    void signalAll()        { Pthread_cond_broadcast(&cv_); }

    void timedWait(Mutex* mutex, const struct timespec* timeout) {
        pthread_cond_timedwait(&cv_, &(mutex->m_), timeout);
    }


private:
    pthread_cond_t cv_;

    // Non-copyable, non-assignable
    ConditionVar(ConditionVar&);
    ConditionVar& operator=(ConditionVar&);
};


// A microsecond precision timer based on the gettimeofday() call
// (which should be low overhead).
//
// Usage:
//
//   Timer t;
//   t.start();
//   ... event we want to clock
//   t.end();
//
//   std::cout << "elapsed time in seconds" << t.elapsed();
//
class Timer {
public:
  Timer();

  void start();
  void end();
  void reset();
  double elapsed() const;

private:
  struct timeval start_;
  struct timeval end_;
};

int set_nonblocking(int fd, bool nonblocking);

}

