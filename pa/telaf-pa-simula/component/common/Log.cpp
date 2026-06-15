// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "Log.hpp"

#include <chart/spy.hpp>

#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <mutex>
#include <memory>
#include <ostream>
#include <streambuf>
#include <string>
#include <syslog.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>

namespace telux {
namespace common {
namespace simula {

namespace {

const char*
basename_(const char* p)
{
    const char* s = std::strrchr(p, '/');
    return s ? s + 1 : p;
}

// Optional file sink — opened once at process start from SML_PA_LOG_FILE.
// nullptr means syslog-only.
FILE* g_log_file = []() -> FILE* {
    const char* path = std::getenv("SML_PA_LOG_FILE");
    if (!path || path[0] == '\0')
        return nullptr;
    return std::fopen(path, "a");  // nullptr on failure → syslog-only
}();

std::mutex g_log_mutex;

constexpr int
to_syslog_priority(LogLevel level)
{
    switch (level) {
        case LogLevel::DBG:  return LOG_DEBUG;
        case LogLevel::INFO: return LOG_INFO;
        case LogLevel::WARN: return LOG_WARNING;
        case LogLevel::ERR:  return LOG_ERR;
    }
    return LOG_INFO;
}

}  // anonymous namespace

std::atomic<LogLevel> g_log_level{ []() -> LogLevel {
    const char* v = std::getenv("SML_PA_LOG_LEVEL");
    if (!v)
        return LogLevel::INFO;
    if (std::strcmp(v, "DEBUG") == 0)
        return LogLevel::DBG;
    if (std::strcmp(v, "WARN") == 0)
        return LogLevel::WARN;
    if (std::strcmp(v, "ERROR") == 0)
        return LogLevel::ERR;
    return LogLevel::INFO;
}() };

void
log_impl(LogLevel level, const char* file, int line, const char* fmt, ...)
{
    if (level < g_log_level.load(std::memory_order_relaxed))
        return;

    static const char* tags[] = { "DBG", "INF", "WRN", "ERR" };

    struct timespec ts{};
    clock_gettime(CLOCK_MONOTONIC, &ts);
    double t = static_cast<double>(ts.tv_sec) + static_cast<double>(ts.tv_nsec) * 1e-9;

    pid_t tid = static_cast<pid_t>(syscall(SYS_gettid));

    va_list args;
    va_start(args, fmt);
    char body[1024];
    std::vsnprintf(body, sizeof(body), fmt, args);
    va_end(args);

    // Full message: level tag + timestamp + tid + location + body.
    char msg[1280];
    std::snprintf(msg, sizeof(msg),
        "[sml-pa %s %10.3f tid=%-6d] %s:%d %s",
        tags[static_cast<int>(level)],
        t,
        static_cast<int>(tid),
        basename_(file),
        line,
        body);

    std::lock_guard<std::mutex> lk(g_log_mutex);

    // Primary sink: syslog — correct facility/level visible in logread.
    syslog(to_syslog_priority(level), "%s", msg);

    // Optional secondary sink: file (set SML_PA_LOG_FILE to enable).
    if (g_log_file) {
        std::fprintf(g_log_file, "%s\n", msg);
        std::fflush(g_log_file);
    }
}

// Routes chart::Spy live output into syslog at LOG_DEBUG priority.
// Each '\n'-terminated line from Spy becomes one syslog entry.
class SpySyslogBuf : public std::streambuf {
protected:
    int overflow(int c) override {
        if (c == traits_type::eof())
            return traits_type::not_eof(c);
        if (c == '\n') {
            if (!buf_.empty()) {
                std::lock_guard<std::mutex> lk(g_log_mutex);
                syslog(LOG_DEBUG, "%s", buf_.c_str());
                if (g_log_file) {
                    std::fprintf(g_log_file, "%s\n", buf_.c_str());
                    std::fflush(g_log_file);
                }
                buf_.clear();
            }
        } else {
            buf_ += static_cast<char>(c);
        }
        return c;
    }

private:
    std::string buf_;
};

namespace {
struct SpyWire {
    SpyWire() {
        os_.reset(new std::ostream(&buf_));
        chart::Spy::live_sink(os_.get());
        chart::Spy::live_trace_sink(os_.get());
    }
    SpySyslogBuf buf_;
    std::unique_ptr<std::ostream> os_;
} g_spy_wire;
}

}  // namespace simula
}  // namespace common
}  // namespace telux
