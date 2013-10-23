#!/bin/bash

export GPERFTOOLS_ROOT=../gperftools
export GPERFTOOLS_REPORT=report.prof
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$GPERFTOOLS_ROOT/.libs
export DYLD_FALLBACK_LIBRARY_PATH=$DYLD_FALLBACK_LIBRARY_PATH:$GPERFTOOLS_ROOT/.libs

env LD_PRELOAD=$GPERFTOOLS_ROOT/.libs/libprofiler.so \
    DYLD_INSERT_LIBRARIES=$GPERFTOOLS_ROOT/.libs/libprofiler.dylib \
    CPUPROFILE=$GPERFTOOLS_REPORT ./build/testharness $@

$GPERFTOOLS_ROOT/src/pprof --text build/testharness $GPERFTOOLS_REPORT
