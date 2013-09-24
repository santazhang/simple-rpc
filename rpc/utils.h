#pragma once

#include <list>
#include <map>
#include <functional>
#include <random>

#include <sys/types.h>
#include <sys/time.h>
#include <stdarg.h>
#include <stdio.h>
#include <assert.h>
#include <pthread.h>
#include <inttypes.h>

#include "b0/b0all.h"

namespace rpc {

using b0::i32;
using b0::i64;
using b0::NoCopy;
using b0::Lockable;
using b0::SpinLock;
using b0::Mutex;
using b0::ScopedLock;
using b0::CondVar;
using b0::Log;
using b0::RefCounted;

// choice between spinlock & mutex:
// * when n_thread > n_core, use mutex
// * on virtual machines, use mutex


// use spinlock for short critical section
typedef SpinLock ShortLock;

// use mutex for long critical section
typedef Mutex LongLock;


/**
 * Thread safe queue.
 */
template<class T>
class Queue: public NoCopy {
    std::list<T>* q_;
    pthread_cond_t not_empty_;
    pthread_mutex_t m_;

public:

    Queue(): q_(new std::list<T>) {
        Pthread_mutex_init(&m_, NULL);
        Pthread_cond_init(&not_empty_, NULL);
    }

    ~Queue() {
        Pthread_cond_destroy(&not_empty_);
        Pthread_mutex_destroy(&m_);
        delete q_;
    }

    void push(const T& e) {
        Pthread_mutex_lock(&m_);
        q_->push_back(e);
        Pthread_cond_signal(&not_empty_);
        Pthread_mutex_unlock(&m_);
    }

    void pop_many(std::list<T>* out, int count) {
        Pthread_mutex_lock(&m_);
        if (q_->empty()) {
          Pthread_mutex_unlock(&m_);
          return;
        }

        while (count > 0) {
          out->push_back(q_->front());
          q_->pop_front();
          --count;
        }

        Pthread_mutex_unlock(&m_);
    }

    T pop() {
        Pthread_mutex_lock(&m_);
        while (q_->empty()) {
            Pthread_cond_wait(&not_empty_, &m_);
        }
        T e = q_->front();
        q_->pop_front();
        Pthread_mutex_unlock(&m_);
        return e;
    }
};

class Rand {
    std::mt19937 rand_;
public:
    Rand() {
        struct timeval now;
        gettimeofday(&now, NULL);
        rand_.seed(now.tv_sec + now.tv_usec + (long long) pthread_self() + (long long) this);
    }
    std::mt19937::result_type next() {
        return rand_();
    }
    std::mt19937::result_type operator() () {
        return rand_();
    }
};

class ThreadPool: public RefCounted {
    int n_;
    Rand rand_engine_;
    pthread_t* th_;
    Queue<std::function<void()>*>* q_;
    bool should_stop_;

    static void* start_thread_pool(void*);
    void run_thread(int id_in_pool);

protected:
    ~ThreadPool();

public:
    ThreadPool(int n = 64);

    // return 0 when queuing ok, otherwise EPERM
    int run_async(const std::function<void()>&);
};

using b0::Counter;
using b0::Timer;

int set_nonblocking(int fd, bool nonblocking);

int find_open_port();
std::string get_host_name();

inline uint64_t rdtsc() {
    uint32_t hi, lo;
    __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
    return (((uint64_t) hi) << 32) | ((uint64_t) lo);
}


}

