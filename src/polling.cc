#include <unistd.h>
#include <errno.h>
#include <sys/select.h>
#include <pthread.h>

#include "polling.h"

using namespace std;

void* PollMgr::start_poll_loop(void* p) {
    PollMgr* poll = (PollMgr *) p;
    poll->poll_loop();
    pthread_exit(NULL);
}

PollMgr::PollMgr() {
    pthread_mutex_init(&m_, NULL);
    VERIFY(pipe(wakeup_) == 0);
    setnonblocking(wakeup_[0], true);
    setnonblocking(wakeup_[1], true);
    //Log::debug("wakeup pipe: r=%d, w=%d", wakeup_[0], wakeup_[1]);
    stop_flag_ = false;
    VERIFY(pthread_create(&th_, NULL, PollMgr::start_poll_loop, this) == 0);
}

PollMgr::~PollMgr() {
    Log::info("destroying poll loop...");

    stop_flag_ = true;
    char ch = 's';
    pthread_mutex_lock(&m_);
    VERIFY(::write(wakeup_[1], &ch, 1) == 1);
    pthread_mutex_unlock(&m_);
    pthread_join(th_, NULL);

    for (map<int, Pollable*>::iterator it = poll_.begin(); it != poll_.end(); ++it) {
        delete it->second;
        poll_.erase(it->first);
    }

    close(wakeup_[0]);
    close(wakeup_[1]);
    pthread_mutex_destroy(&m_);
}

void PollMgr::poll_loop() {
    Log::info("poll loop running...");

    fd_set read_fds, write_fds;
    while (!stop_flag_) {
        FD_ZERO(&read_fds);
        FD_ZERO(&write_fds);
        int fdmax = ::max(wakeup_[0], wakeup_[1]);

        FD_SET(wakeup_[0], &read_fds);

        // lock up to protect mode_
        pthread_mutex_lock(&m_);
        for (map<int, Pollable*>::iterator it = poll_.begin(); it != poll_.end(); ++it) {
            if (it->second == NULL) {
                continue;
            }
            fdmax = ::max(fdmax, it->first);
            int mode = it->second->poll_mode();
            mode_[it->first] = mode;
            if (mode & Pollable::READ) {
                FD_SET(it->first, &read_fds);
            }
            if (mode & Pollable::WRITE) {
                FD_SET(it->first, &write_fds);
            }
        }
        pthread_mutex_unlock(&m_);

        timeval tv;
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        int n_ready = select(fdmax + 1, &read_fds, &write_fds, NULL, &tv);
        if (n_ready == 0) {
            continue;
        }

        for (int fd = 0; fd <= fdmax; fd++) {
            if (FD_ISSET(fd, &read_fds)) {
                if (fd == wakeup_[0]) {
                    // lockup wakeup_, prevent lost wakeup notifications
                    ScopedLock sl(&m_);
                    const int buf_size = 1024;
                    char buf[buf_size];
                    VERIFY(::read(fd, buf, buf_size) >= 1);
                } else if (poll_[fd]) {
                    poll_[fd]->handle_read();
                }
            }
            if (FD_ISSET(fd, &write_fds)) {
                if (poll_[fd]) {
                    poll_[fd]->handle_write();
                }
            }
        }

        ScopedLock sl(&m_);
        // dont need to do any thing real, since each round, the fd_set will be re-filled
        pending_update_.clear();

        for (set<int>::iterator it = pending_remove_.begin(); it != pending_remove_.end(); ++it) {
            map<int, Pollable*>::iterator rm_it = poll_.find(*it);
            if (rm_it != poll_.end()) {
                delete rm_it->second;
                poll_.erase(*it);
            }
        }
        pending_remove_.clear();

        for (set<Pollable*>::iterator it = pending_add_.begin(); it != pending_add_.end(); ++it) {
            poll_[(*it)->fd()] = *it;
        }
        pending_add_.clear();
    }
}


void PollMgr::add(Pollable* p) {
    //Log::debug("add pollable for fd=%d", p->fd());
    ScopedLock sl(&m_);
    pending_add_.insert(p);
    char ch = 'a';
    ::write(wakeup_[1], &ch, 1);
}

void PollMgr::remove(int fd) {
    //Log::debug("remove pollable for fd=%d", fd);
    ScopedLock sl(&m_);
    pending_remove_.insert(fd);
    char ch = 'r';
    ::write(wakeup_[1], &ch, 1);
}

void PollMgr::update_poll_mode(int fd, int mode) {
    if (mode & Pollable::WRITE) {
        ScopedLock sl(&m_);
        if (mode == mode_[fd]) {
            return;
        }
        pending_update_.insert(fd);
        char ch = 'u';
        ::write(wakeup_[1], &ch, 1);
    }
}
