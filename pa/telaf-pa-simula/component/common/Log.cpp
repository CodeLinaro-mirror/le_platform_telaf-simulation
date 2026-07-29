// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "Log.hpp"

#include <chart/spy_bootstrap.hpp>
#include <chart/spy.hpp>

#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <map>
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

constexpr const char* kConfPath = "/etc/sml_pa.conf";

// Parses KEY=VALUE lines from kConfPath ('#' comments, blank lines ignored).
// Read once and cached; missing file yields an empty map (no error).
const std::map<std::string, std::string>&
conf_file()
{
    static const std::map<std::string, std::string> conf = []() {
        std::map<std::string, std::string> result;
        std::ifstream f(kConfPath);
        std::string line;
        while (std::getline(f, line)) {
            auto hash = line.find('#');
            if (hash != std::string::npos)
                line.erase(hash);
            auto eq = line.find('=');
            if (eq == std::string::npos)
                continue;
            std::string key = line.substr(0, eq);
            std::string val = line.substr(eq + 1);
            auto trim = [](std::string& s) {
                const char* ws = " \t\r\n";
                auto b = s.find_first_not_of(ws);
                auto e = s.find_last_not_of(ws);
                s = (b == std::string::npos) ? "" : s.substr(b, e - b + 1);
            };
            trim(key);
            trim(val);
            if (!key.empty())
                result[key] = val;
        }
        return result;
    }();
    return conf;
}

// Looks up key in the environment first, falling back to the conf file.
// Returns nullptr if neither has it.
const char*
setting(const char* env_name, const char* conf_key)
{
    const char* v = std::getenv(env_name);
    if (v && v[0] != '\0')
        return v;
    const auto& conf = conf_file();
    auto it = conf.find(conf_key);
    if (it != conf.end() && !it->second.empty())
        return it->second.c_str();
    return nullptr;
}

// Optional file sink — opened once at process start from SML_PA_LOG_FILE
// (or LOG_FILE in /etc/sml_pa.conf). nullptr means syslog-only.
FILE* g_log_file = []() -> FILE* {
    const char* path = setting("SML_PA_LOG_FILE", "LOG_FILE");
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
    const char* v = setting("SML_PA_LOG_LEVEL", "LOG_LEVEL");
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

    // Optional secondary sink: file (set SML_PA_LOG_FILE or LOG_FILE in
    // /etc/sml_pa.conf to enable).
    if (g_log_file) {
        std::fprintf(g_log_file, "%s\n", msg);
        std::fflush(g_log_file);
    }
}

// Routes chart::Spy live output into syslog at LOG_DEBUG priority.
// Each '\n'-terminated line from Spy becomes one syslog entry, prefixed with
// a caller-supplied tag so spy lines and trace lines are grep-separable:
//   [sml-pa spy]   — verbose visit / HOOK / IGNORED lines
//   [sml-pa trace] — transition lines (On and Verbose modes)
class SpySyslogBuf : public std::streambuf {
public:
    explicit SpySyslogBuf(const char* prefix) : prefix_(prefix) {}

protected:
    int overflow(int c) override {
        if (c == traits_type::eof())
            return traits_type::not_eof(c);
        if (c == '\n') {
            if (!buf_.empty()) {
                std::lock_guard<std::mutex> lk(g_log_mutex);
                syslog(LOG_DEBUG, "%s %s", prefix_, buf_.c_str());
                if (g_log_file) {
                    std::fprintf(g_log_file, "%s %s\n", prefix_, buf_.c_str());
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
    const char* prefix_;
    std::string buf_;
};

namespace {
struct SpyWire {
    SpyWire()
        : spy_buf_("[sml-pa spy]"), trace_buf_("[sml-pa trace]")
    {
        spy_os_.reset(new std::ostream(&spy_buf_));
        trace_os_.reset(new std::ostream(&trace_buf_));
        chart::Spy::live_sink(spy_os_.get());
        chart::Spy::live_trace_sink(trace_os_.get());
        // Signal / state-handler names and Spy enable knob. Idempotent; runs
        // once at process start. Must happen AFTER the live sinks are attached
        // so no dispatch line is lost between enable and sink hookup.
        chart::init_from_env("TELAF_CHART_SPY", "/etc/sml_pa.conf", "LOG_SPY");
    }
    SpySyslogBuf spy_buf_;
    SpySyslogBuf trace_buf_;
    std::unique_ptr<std::ostream> spy_os_;
    std::unique_ptr<std::ostream> trace_os_;
} g_spy_wire;
}

}  // namespace simula
}  // namespace common
}  // namespace telux
