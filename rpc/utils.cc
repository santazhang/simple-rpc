#include <utility>

#include <fcntl.h>
#include <stdlib.h>

#include "utils.h"

namespace rpc {

void* ThreadPool::start_thread_pool(void* args) {
    std::pair<ThreadPool*, int>* thread_args = (std::pair<ThreadPool*, int>*) args;
    ThreadPool* threadpool = thread_args->first;
    int tid = thread_args->second;
    delete thread_args;

    threadpool->run_thread(tid);

    pthread_exit(NULL);
    return NULL;
}

ThreadPool::ThreadPool(int n /* =... */)
        : n_(n) {
    th_ = new pthread_t[n_];
    q_ = new Queue<Runnable*> [n_];

    for (int i = 0; i < n_; i++) {
        std::pair<ThreadPool*, int>* thread_args = new std::pair<ThreadPool*, int>;
        thread_args->first = this;
        thread_args->second = i;
        Pthread_create(&th_[i], NULL, ThreadPool::start_thread_pool, thread_args);
    }
}

ThreadPool::~ThreadPool() {
    for (int i = 0; i < n_; i++) {
        q_[i].push(NULL);	// NULL is used as a termination token
    }
    for (int i = 0; i < n_; i++) {
        Pthread_join(th_[i], NULL);
    }
    delete[] th_;
    delete[] q_;
}

void ThreadPool::run_async(Runnable* r) {
    verify(r != NULL);
    // NULL is used to terminate a thread

    // Randomly select a thread for the job.
    // There could be better schedule policy.
    int queue_id = rand() % n_;
    q_[queue_id].push(r);
}

void ThreadPool::run_thread(int tid) {
    for (;;) {
        Runnable* j = q_[tid].pop();
        if (j == NULL) {
            return;
        }
        j->run();
        delete j;
    }
}

int Log::level = Log::DEBUG;
FILE* Log::fp = stdout;
pthread_mutex_t Log::m = PTHREAD_MUTEX_INITIALIZER;

void Log::set_level(int level) {
    Pthread_mutex_lock(&Log::m);
    Log::level = level;
    Pthread_mutex_unlock(&Log::m);
}

void Log::set_file(FILE* fp) {
    verify(fp != NULL);
    Pthread_mutex_lock(&Log::m);
    Log::fp = fp;
    Pthread_mutex_unlock(&Log::m);
}

void Log::log_v(int level, const char* fmt, va_list args) {
    static char indicator[] = { 'F', 'E', 'W', 'I', 'D' };
    assert(level <= Log::DEBUG);
    if (level <= Log::level) {
        Pthread_mutex_lock(&Log::m);
        fprintf(Log::fp, "%c: ", indicator[level]);
        vfprintf(Log::fp, fmt, args);
        fprintf(Log::fp, "\n");
        fflush(Log::fp);
        Pthread_mutex_unlock(&Log::m);
    }
}

void Log::log(int level, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    log_v(level, fmt, args);
    va_end(args);
}

void Log::fatal(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    log_v(Log::FATAL, fmt, args);
    va_end(args);
}

void Log::error(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    log_v(Log::ERROR, fmt, args);
    va_end(args);
}

void Log::warn(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    log_v(Log::WARN, fmt, args);
    va_end(args);
}

void Log::info(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    log_v(Log::INFO, fmt, args);
    va_end(args);
}

void Log::debug(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    log_v(Log::DEBUG, fmt, args);
    va_end(args);
}

int set_nonblocking(int fd, bool nonblocking) {
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

}
