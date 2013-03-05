#ifdef __APPLE__
#define USE_KQUEUE
#endif

#ifdef USE_KQUEUE
#include <sys/event.h>
#else
#include <sys/epoll.h>
#endif

#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "utils.h"
#include "polling.h"

using namespace std;

namespace rpc {

class PollMgr::PollThread {

    // guard mode_ and poll_set_
    pthread_mutex_t m_;
    std::map<int, int> mode_;
    std::set<Pollable*> poll_set_;
    int poll_fd_;

    std::set<Pollable*> pending_remove_;
    pthread_mutex_t pending_remove_m_;

    pthread_t th_;
    bool stop_flag_;

    static void* start_poll_loop(void* arg) {
        PollThread* thiz = (PollThread *) arg;
        thiz->poll_loop();
        pthread_exit(NULL);
        return NULL;
    }

    void poll_loop();

public:

    PollThread()
            : stop_flag_(false) {
        Pthread_mutex_init(&m_, NULL);
        Pthread_mutex_init(&pending_remove_m_, NULL);

#ifdef USE_KQUEUE
        poll_fd_ = kqueue();
#else
        poll_fd_ = epoll_create(10);    // arg ignored, any value > 0 will do
#endif

        verify(poll_fd_ != -1);

        Pthread_create(&th_, NULL, PollMgr::PollThread::start_poll_loop, this);
    }

    ~PollThread() {
        Pthread_mutex_lock(&m_);
        set<Pollable*> poll_set_copy = poll_set_;
        Pthread_mutex_unlock(&m_);

        for (set<Pollable*>::iterator it = poll_set_copy.begin(); it != poll_set_copy.end(); ++it) {
            remove(*it);
        }

        stop_flag_ = true;
        Pthread_join(th_, NULL);
        Pthread_mutex_destroy(&m_);
        Pthread_mutex_destroy(&pending_remove_m_);
    }

    void add(Pollable*);
    void remove(Pollable*);
    void update_mode(Pollable*, int new_mode);
};

PollMgr::PollMgr(int n_threads /* =... */)
        : n_(n_threads) {
    poll_threads_ = new PollThread[n_];
    //Log::debug("rpc::PollMgr: start with %d thread", n_);
}

PollMgr::~PollMgr() {
    delete[] poll_threads_;
    //Log::debug("rpc::PollMgr: destroyed");
}

void PollMgr::PollThread::poll_loop() {
    while (!stop_flag_) {
        const int max_nev = 100;

#ifdef USE_KQUEUE

        struct kevent evlist[max_nev];
        struct timespec timeout;
        timeout.tv_sec = 0;
        timeout.tv_nsec = 50 * 1000 * 1000; // 0.05 sec

        int nev = kevent(poll_fd_, NULL, 0, evlist, max_nev, &timeout);

        for (int i = 0; i < nev; i++) {
            Pollable* poll = (Pollable *) evlist[i].udata;
            verify(poll != NULL);

            if (evlist[i].filter == EVFILT_READ) {
                poll->handle_read();
            }
            if (evlist[i].filter == EVFILT_WRITE) {
                poll->handle_write();
            }

            // handle error after handle IO, so that we can at least process something
            if (evlist[i].flags & EV_EOF) {
                poll->handle_error();
            }
        }

#else

        struct epoll_event evlist[max_nev];
        int timeout = 50; // milli, 0.05 sec

        int nev = epoll_wait(poll_fd_, evlist, max_nev, timeout);

        if (stop_flag_) {
            break;
        }

        for (int i = 0; i < nev; i++) {
            Pollable* poll = (Pollable *) evlist[i].data.ptr;
            verify(poll != NULL);

            if (evlist[i].events & EPOLLIN) {
                poll->handle_read();
            }
            if (evlist[i].events & EPOLLOUT) {
                poll->handle_write();
            }

            // handle error after handle IO, so that we can at least process something
            if (evlist[i].events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)) {
                poll->handle_error();
            }
        }

#endif

        // after each poll loop, remove uninterested pollables
        Pthread_mutex_lock(&pending_remove_m_);
        list<Pollable*> remove_poll(pending_remove_.begin(), pending_remove_.end());
        pending_remove_.clear();
        Pthread_mutex_unlock(&pending_remove_m_);

        for (list<Pollable*>::iterator it = remove_poll.begin(); it != remove_poll.end(); ++it) {
            Pollable* poll = *it;
            int fd = poll->fd();

            Pthread_mutex_lock(&m_);
            if (mode_.find(fd) == mode_.end()) {
                // NOTE: only remove the fd when it is not immediately added again
                // if the same fd is used again, mode_ will contains its info
#ifdef USE_KQUEUE

                struct kevent ev;

                bzero(&ev, sizeof(ev));
                ev.ident = fd;
                ev.flags = EV_DELETE;
                ev.filter = EVFILT_READ;
                kevent(poll_fd_, &ev, 1, NULL, 0, NULL);

                bzero(&ev, sizeof(ev));
                ev.ident = fd;
                ev.flags = EV_DELETE;
                ev.filter = EVFILT_WRITE;
                kevent(poll_fd_, &ev, 1, NULL, 0, NULL);

#else
                struct epoll_event ev;
                memset(&ev, 0, sizeof(ev));

                epoll_ctl(poll_fd_, EPOLL_CTL_DEL, fd, &ev);
#endif
            }
            Pthread_mutex_unlock(&m_);

            poll->release();
        }
    }

    // when stopping, release anything registered in pollmgr
    for (set<Pollable*>::iterator it = poll_set_.begin(); it != poll_set_.end(); ++it) {
        (*it)->release();
    }

    close(poll_fd_);
}

void PollMgr::PollThread::add(Pollable* poll) {
    poll->ref_copy();   // increase ref count

    int poll_mode = poll->poll_mode();
    int fd = poll->fd();

    Pthread_mutex_lock(&m_);

    // verify not exists
    verify(poll_set_.find(poll) == poll_set_.end());
    verify(mode_.find(fd) == mode_.end());

    // register pollable
    poll_set_.insert(poll);
    mode_[fd] = poll_mode;

    Pthread_mutex_unlock(&m_);

#ifdef USE_KQUEUE

    struct kevent ev;
    if (poll_mode & Pollable::READ) {
        bzero(&ev, sizeof(ev));
        ev.ident = fd;
        ev.flags = EV_ADD;
        ev.filter = EVFILT_READ;
        ev.udata = poll;
        verify(kevent(poll_fd_, &ev, 1, NULL, 0, NULL) == 0);
    }
    if (poll_mode & Pollable::WRITE) {
        bzero(&ev, sizeof(ev));
        ev.ident = fd;
        ev.flags = EV_ADD;
        ev.filter = EVFILT_WRITE;
        ev.udata = poll;
        verify(kevent(poll_fd_, &ev, 1, NULL, 0, NULL) == 0);
    }

#else

    struct epoll_event ev;
    memset(&ev, 0, sizeof(ev));

    ev.data.ptr = poll;
    ev.events = EPOLLIN | EPOLLET | EPOLLRDHUP; // EPOLLERR and EPOLLHUP are included by default
    if (poll_mode & Pollable::WRITE) {
        ev.events |= EPOLLOUT;
    }
    verify(epoll_ctl(poll_fd_, EPOLL_CTL_ADD, fd, &ev) == 0);

#endif
}

void PollMgr::PollThread::remove(Pollable* poll) {
    bool found = false;
    Pthread_mutex_lock(&m_);
    set<Pollable*>::iterator it = poll_set_.find(poll);
    if (it != poll_set_.end()) {
        found = true;
        assert(mode_.find(poll->fd()) != mode_.end());
        poll_set_.erase(poll);
        mode_.erase(poll->fd());
    } else {
        assert(mode_.find(poll->fd()) == mode_.end());
    }
    Pthread_mutex_unlock(&m_);

    if (found) {
        Pthread_mutex_lock(&pending_remove_m_);
        pending_remove_.insert(poll);
        Pthread_mutex_unlock(&pending_remove_m_);
    }
}

void PollMgr::PollThread::update_mode(Pollable* poll, int new_mode) {
    int fd = poll->fd();

    Pthread_mutex_lock(&m_);

    if (poll_set_.find(poll) == poll_set_.end()) {
        Pthread_mutex_unlock(&m_);
        return;
    }

    map<int, int>::iterator it = mode_.find(fd);
    verify(it != mode_.end());
    int old_mode = it->second;
    it->second = new_mode;

    if (new_mode != old_mode) {

#ifdef USE_KQUEUE

        struct kevent ev;
        if ((new_mode & Pollable::READ) && !(old_mode & Pollable::READ)) {
            // add READ
            bzero(&ev, sizeof(ev));
            ev.ident = fd;
            ev.udata = poll;
            ev.flags = EV_ADD;
            ev.filter = EVFILT_READ;
            verify(kevent(poll_fd_, &ev, 1, NULL, 0, NULL) == 0);
        }
        if (!(new_mode & Pollable::READ) && (old_mode & Pollable::READ)) {
            // del READ
            bzero(&ev, sizeof(ev));
            ev.ident = fd;
            ev.udata = poll;
            ev.flags = EV_DELETE;
            ev.filter = EVFILT_READ;
            verify(kevent(poll_fd_, &ev, 1, NULL, 0, NULL) == 0);
        }
        if ((new_mode & Pollable::WRITE) && !(old_mode & Pollable::WRITE)) {
            // add WRITE
            bzero(&ev, sizeof(ev));
            ev.ident = fd;
            ev.udata = poll;
            ev.flags = EV_ADD;
            ev.filter = EVFILT_WRITE;
            verify(kevent(poll_fd_, &ev, 1, NULL, 0, NULL) == 0);
        }
        if (!(new_mode & Pollable::WRITE) && (old_mode & Pollable::WRITE)) {
            // del WRITE
            bzero(&ev, sizeof(ev));
            ev.ident = fd;
            ev.udata = poll;
            ev.flags = EV_DELETE;
            ev.filter = EVFILT_WRITE;
            verify(kevent(poll_fd_, &ev, 1, NULL, 0, NULL) == 0);
        }

#else

        struct epoll_event ev;
        memset(&ev, 0, sizeof(ev));

        ev.data.ptr = poll;
        ev.events = EPOLLET | EPOLLRDHUP;
        if (new_mode & Pollable::READ) {
            ev.events |= EPOLLIN;
        }
        if (new_mode & Pollable::WRITE) {
            ev.events |= EPOLLOUT;
        }
        verify(epoll_ctl(poll_fd_, EPOLL_CTL_MOD, fd, &ev) == 0);

#endif

    }

    Pthread_mutex_unlock(&m_);
}

void PollMgr::add(Pollable* poll) {
    int fd = poll->fd();
    if (fd >= 0) {
        int tid = fd % n_;
        poll_threads_[tid].add(poll);
    }
}

void PollMgr::remove(Pollable* poll) {
    int fd = poll->fd();
    if (fd >= 0) {
        int tid = fd % n_;
        poll_threads_[tid].remove(poll);
    }
}

void PollMgr::update_mode(Pollable* poll, int new_mode) {
    int fd = poll->fd();
    if (fd >= 0) {
        int tid = fd % n_;
        poll_threads_[tid].update_mode(poll, new_mode);
    }
}

}
