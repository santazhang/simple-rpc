#include <sstream>

#include "buffer.h"

using namespace std;

namespace rpc {

#ifdef RPC_STATISTICS

// -1, 0~15, 16~31, 32~63, 64~127, 128~255, 256~511, 512~1023, 1024~2047, 2048~4095, 4096~8191, 8192~
static Counter g_marshal_in_stat[12];
static Counter g_marshal_in_stat_cumulative[12];
static Counter g_marshal_out_stat[12];
static Counter g_marshal_out_stat_cumulative[12];
static uint64_t g_marshal_stat_report_time = 0;
static const uint64_t g_marshal_stat_report_interval = 1000 * 1000 * 1000;

static void stat_marshal_report() {
    Log::info("* MARSHAL:     -1 0~15 16~31 32~63 64~127 128~255 256~511 512~1023 1024~2047 2048~4095 4096~8191 8192~");
    {
        ostringstream ostr;
        for (size_t i = 0; i < arraysize(g_marshal_in_stat); i++) {
            i64 v = g_marshal_in_stat[i].peek_next();
            g_marshal_in_stat_cumulative[i].next(v);
            ostr << " " << v;
            g_marshal_in_stat[i].reset();
        }
        Log::info("* MARSHAL IN: %s", ostr.str().c_str());
    }
    {
        ostringstream ostr;
        for (size_t i = 0; i < arraysize(g_marshal_in_stat); i++) {
            ostr << " " << g_marshal_in_stat_cumulative[i].peek_next();
        }
        Log::info("* MARSHAL IN (cumulative): %s", ostr.str().c_str());
    }
    {
        ostringstream ostr;
        for (size_t i = 0; i < arraysize(g_marshal_out_stat); i++) {
            i64 v = g_marshal_out_stat[i].peek_next();
            g_marshal_out_stat_cumulative[i].next(v);
            ostr << " " << v;
            g_marshal_out_stat[i].reset();
        }
        Log::info("* MARSHAL OUT:%s", ostr.str().c_str());
    }
    {
        ostringstream ostr;
        for (size_t i = 0; i < arraysize(g_marshal_in_stat); i++) {
            ostr << " " << g_marshal_out_stat_cumulative[i].peek_next();
        }
        Log::info("* MARSHAL OUT (cumulative): %s", ostr.str().c_str());
    }
}

void stat_marshal_in(int fd, const void* buf, size_t nbytes, ssize_t ret) {
    if (ret == -1) {
        g_marshal_in_stat[0].next();
    } else if (ret < 16) {
        g_marshal_in_stat[1].next();
    } else if (ret < 32) {
        g_marshal_in_stat[2].next();
    } else if (ret < 64) {
        g_marshal_in_stat[3].next();
    } else if (ret < 128) {
        g_marshal_in_stat[4].next();
    } else if (ret < 256) {
        g_marshal_in_stat[5].next();
    } else if (ret < 512) {
        g_marshal_in_stat[6].next();
    } else if (ret < 1024) {
        g_marshal_in_stat[7].next();
    } else if (ret < 2048) {
        g_marshal_in_stat[8].next();
    } else if (ret < 4096) {
        g_marshal_in_stat[9].next();
    } else if (ret < 8192) {
        g_marshal_in_stat[10].next();
    } else {
        g_marshal_in_stat[11].next();
    }

    uint64_t now = base::rdtsc();
    if (now - g_marshal_stat_report_time > g_marshal_stat_report_interval) {
        stat_marshal_report();
        g_marshal_stat_report_time = now;
    }
}

void stat_marshal_out(int fd, const void* buf, size_t nbytes, ssize_t ret) {
    if (ret == -1) {
        g_marshal_out_stat[0].next();
    } else if (ret < 16) {
        g_marshal_out_stat[1].next();
    } else if (ret < 32) {
        g_marshal_out_stat[2].next();
    } else if (ret < 64) {
        g_marshal_out_stat[3].next();
    } else if (ret < 128) {
        g_marshal_out_stat[4].next();
    } else if (ret < 256) {
        g_marshal_out_stat[5].next();
    } else if (ret < 512) {
        g_marshal_out_stat[6].next();
    } else if (ret < 1024) {
        g_marshal_out_stat[7].next();
    } else if (ret < 2048) {
        g_marshal_out_stat[8].next();
    } else if (ret < 4096) {
        g_marshal_out_stat[9].next();
    } else if (ret < 8192) {
        g_marshal_out_stat[10].next();
    } else {
        g_marshal_out_stat[11].next();
    }

    uint64_t now = base::rdtsc();
    if (now - g_marshal_stat_report_time > g_marshal_stat_report_interval) {
        stat_marshal_report();
        g_marshal_stat_report_time = now;
    }
}

#endif // RPC_STATISTICS

/**
 * 8kb minimum chunk size.
 * NOTE: this value directly affects how many read/write syscall will be issued.
 */
const size_t raw_bytes::min_size = 8192;

}   // namespace rpc
