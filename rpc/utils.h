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
using b0::Queue;
using b0::Counter;
using b0::Timer;
using b0::Rand;
using b0::ThreadPool;

// use spinlock for short critical section
typedef SpinLock ShortLock;

// use mutex for long critical section
typedef Mutex LongLock;

int set_nonblocking(int fd, bool nonblocking);

int find_open_port();

std::string get_host_name();

}

