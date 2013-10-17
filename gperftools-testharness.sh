#!/bin/bash

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:../gperftools/.libs
export DYLD_FALLBACK_LIBRARY_PATH=$DYLD_FALLBACK_LIBRARY_PATH:../gperftools/.libs

env LD_PRELOAD=../gperftools/.libs/libprofiler.so \
    DYLD_INSERT_LIBRARIES=../gperftools/.libs/libprofiler.dylib \
    CPUPROFILE=report.prof ./build/testharness $@

../gperftools/src/pprof --text build/testharness report.prof
