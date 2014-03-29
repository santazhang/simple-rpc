#pragma once

#include <list>
#include <map>
#include <unordered_map>
#include <functional>

#include <sys/types.h>
#include <sys/time.h>
#include <stdarg.h>
#include <stdio.h>
#include <assert.h>
#include <pthread.h>
#include <inttypes.h>

#include "base/all.h"

namespace rpc {

using base::i8;
using base::i16;
using base::i32;
using base::i64;
using base::v32;
using base::v64;
using base::NoCopy;
using base::Lockable;
using base::SpinLock;
using base::Mutex;
using base::ScopedLock;
using base::CondVar;
using base::Log;

//using base::RefCounted;

/**
 * Note: All sub class of RefCounted *MUST* have protected destructor!
 * This prevents accidentally deleting the object.
 * You are only allowed to cleanup with release() call.
 * This is thread safe.
 */
class RefCounted: public NoCopy {
    volatile int refcnt_;
protected:
    virtual ~RefCounted() = 0;
public:
    RefCounted(): refcnt_(1) {}
    int ref_count() {
        return refcnt_;
    }
    virtual RefCounted* ref_copy() {
        __sync_add_and_fetch(&refcnt_, 1);
        return this;
    }
    virtual int release() {
        int r = __sync_sub_and_fetch(&refcnt_, 1);
        verify(r >= 0);
        if (r == 0) {
            delete this;
        }
        return r;
    }
};
inline RefCounted::~RefCounted() {}

using base::Queue;
using base::Counter;
using base::Timer;
using base::Rand;
using base::ThreadPool;
using base::insert_into_map;

int set_nonblocking(int fd, bool nonblocking);

int find_open_port();

std::string get_host_name();

}
