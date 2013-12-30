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
using base::RefCounted;
using base::Queue;
using base::Counter;
using base::Timer;
using base::Rand;
using base::ThreadPool;

int set_nonblocking(int fd, bool nonblocking);

int find_open_port();

std::string get_host_name();

template<class K, class V>
inline void insert_into_map(std::map<K, V>& dict, const K& key, const V& value) {
    dict.insert(typename std::map<K, V>::value_type(key, value));
}

template<class K, class V>
inline void insert_into_map(std::unordered_map<K, V>& dict, const K& key, const V& value) {
    dict.insert(typename std::unordered_map<K, V>::value_type(key, value));
}

}
