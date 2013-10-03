#pragma once

#include <list>
#include <map>
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

using base::i32;
using base::i64;
using base::NoCopy;
using base::Lockable;
using base::SpinLock;
using base::Mutex;
using base::ScopedLock;
using base::CondVar;
using base::Log;
using base::RefCounted;
using base::Queue;
using base::Counter;
using base::Timer;
using base::Rand;
using base::ThreadPool;

// use spinlock for short critical section
typedef SpinLock ShortLock;

// use mutex for long critical section
typedef Mutex LongLock;

int set_nonblocking(int fd, bool nonblocking);

int find_open_port();

std::string get_host_name();

}

