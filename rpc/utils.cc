#include <utility>

#include <fcntl.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "utils.h"

using namespace std;

namespace rpc {

struct start_thread_pool_args {
    ThreadPool* thrpool;
    int id_in_pool;
};

void* ThreadPool::start_thread_pool(void* args) {
    start_thread_pool_args* t_args = (start_thread_pool_args *) args;
    t_args->thrpool->run_thread(t_args->id_in_pool);
    delete t_args;
    pthread_exit(NULL);
    return NULL;
}

ThreadPool::ThreadPool(int n /* =... */): n_(n), should_stop_(false) {
    th_ = new pthread_t[n_];
    q_ = new Queue<function<void()>*> [n_];

    for (int i = 0; i < n_; i++) {
        start_thread_pool_args* args = new start_thread_pool_args();
        args->thrpool = this;
        args->id_in_pool = i;
        Pthread_create(&th_[i], NULL, ThreadPool::start_thread_pool, args);
    }
}

ThreadPool::~ThreadPool() {
    should_stop_ = true;
    for (int i = 0; i < n_; i++) {
        Pthread_join(th_[i], nullptr);
    }
    // TODO probably need to check if there's jobs not executed yet in queues
    delete[] th_;
    delete[] q_;
}

int ThreadPool::run_async(const std::function<void()>& f) {
    if (should_stop_) {
        return EPERM;
    }
    // Randomly select a thread for the job.
    int queue_id = rand_engine_() % n_;
    q_[queue_id].push(new function<void()>(f));
    return 0;
}

void ThreadPool::run_thread(int id_in_pool) {
    int64_t cycle_last_item_found = rdtsc();
    list<function<void()>*> jobs;
    struct timespec sleep_req;
    sleep_req.tv_nsec = 1;
    sleep_req.tv_sec = 0;

    for (;;) {
        if (sleep_req.tv_nsec > 1) {
          nanosleep(&sleep_req, NULL);
        }

        int64_t cycle_now = rdtsc();

        // How long should a thread wait before stealing from
        // other queues.
        static const int kStealCycles = 10 * 1000 * 1000;

        if (cycle_now - cycle_last_item_found > kStealCycles) {
            // start checking other queues
            // TODO randomize each worker thread's stealing order
            for (auto i = 0; i < n_; ++i) {
                q_[i].pop_many(&jobs, 1);
                if (!jobs.empty()) {
                    break;
                }
            }
        } else {
            q_[id_in_pool].pop_many(&jobs, 1);
        }

        if (jobs.empty()) {
            if (should_stop_) {
                break;
            }
            sleep_req.tv_nsec = std::min(1000000L, sleep_req.tv_nsec * 2);
            continue;
        }

        sleep_req.tv_nsec = 1;

        while (!jobs.empty()) {
            auto& f = jobs.front();
            (*f)();
            delete f;
            jobs.pop_front();
        }

        cycle_last_item_found = rdtsc();
    }
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

int find_open_port() {
  int fd = socket(AF_INET, SOCK_STREAM, 0);

  addrinfo *local_addr;

  addrinfo hints;
  bzero(&hints, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  if (getaddrinfo("localhost", NULL, NULL, &local_addr) != 0) {
    Log_fatal("Failed to getaddrinfo");
  }

  int port = -1;

  for (int i = 1024; i < 65000; ++i) {
    ((sockaddr_in*)local_addr->ai_addr)->sin_port = i;
    if (::bind(fd, local_addr->ai_addr, local_addr->ai_addrlen) != 0) {
      continue;
    }

    sockaddr_in addr;
    socklen_t addrlen;
    if (getsockname(fd, (sockaddr*)&addr, &addrlen) != 0) {
      Log_fatal("Failed to get socket address");
    }

    port = i;
    break;
  }

  freeaddrinfo(local_addr);
  ::close(fd);

  if (port != -1) {
    Log_info("Found open port: %d", port);
    return port;
  }

  Log_fatal("Failed to find open port.");
  return -1;
}

std::string get_host_name() {
  char buffer[1024];
  if (gethostname(buffer, 1024) != 0) {
    Log_fatal("Failed to get hostname.");
  }

  return std::string(buffer);
}

}
