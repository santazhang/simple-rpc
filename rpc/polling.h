#pragma once

#include <map>
#include <set>

#include "utils.h"
#include "marshal.h"

namespace rpc {

class Pollable: public RefCounted {
protected:

    virtual ~Pollable() {
    }

public:

    enum {
        READ = 0x1, WRITE = 0x2
    };

    virtual int fd() = 0;
    virtual int poll_mode() = 0;
    virtual void handle_read() = 0;
    virtual void handle_write() = 0;
    virtual void handle_error() = 0;
};

class PollMgr: public RefCounted {

    class PollThread;

    PollThread* poll_threads_;
    int n_;

#ifdef PERF_TEST
    // for performance reporting
    pthread_t perf_th_;
    static void* start_perf_loop(void *arg);
    void perf_loop();
    bool perf_stop_flag_;
#endif // PERF_TEST

protected:

    // RefCounted object uses protected dtor to prevent accidental deletion
    ~PollMgr();

public:

    PollMgr(int n_threads = 1);

    void add(Pollable*);
    void remove(Pollable*);
    void update_mode(Pollable*, int new_mode);
};

}
