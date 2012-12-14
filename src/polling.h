#pragma once

#include <map>
#include <set>

#include "utils.h"

class Pollable: public NoCopy {
public:
    enum {
        READ = 0x1,
        WRITE = 0x2,
    };

    virtual ~Pollable() = 0;

    virtual void handle_read() = 0;
    virtual void handle_write() = 0;
    virtual int fd() = 0;
    virtual int poll_mode() = 0;
};
inline Pollable::~Pollable() {}

class PollMgr: public NoCopy {

    static void* start_poll_loop(void*);
    void poll_loop();

    pthread_t th_;
    bool stop_flag_;
    int wakeup_[2];
    std::map<int, Pollable*> poll_;
    std::map<int, int> mode_;
    std::set<int> pending_update_;
    std::set<Pollable*> pending_add_;
    std::set<int> pending_remove_;
    pthread_mutex_t m_;
public:
    PollMgr();
    virtual ~PollMgr();

    virtual void add(Pollable*);
    virtual void remove(int fd);
    virtual void update_poll_mode(int fd, int mode);
};
