// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear
//
// Log.hpp - lightweight logging for sml-pa simulation components.
//
// Usage:
//   LOG_INFO("[ModemBridge] connected rc=%d", rc);
//
// Runtime level control via env var SML_PA_LOG_LEVEL: DEBUG | INFO | WARN | ERROR
// Default: INFO
//
// Optional log file: SML_PA_LOG_FILE=/path/to/file
// When set, output goes to both stderr and the file (append mode).

#ifndef TELUX_COMMON_SIMULA_LOG_HPP
#define TELUX_COMMON_SIMULA_LOG_HPP

#include <atomic>
#include <cstdarg>
#include <cstdio>

namespace telux {
namespace common {
namespace simula {

enum class LogLevel : int
{
    DBG  = 0,
    INFO = 1,
    WARN = 2,
    ERR  = 3
};

extern std::atomic<LogLevel> g_log_level;

void log_impl(LogLevel level, const char* file, int line, const char* fmt, ...);

}  // namespace simula
}  // namespace common
}  // namespace telux

#define LOG_DEBUG(fmt, ...) \
    ::telux::common::simula::log_impl( \
        ::telux::common::simula::LogLevel::DBG,  __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...) \
    ::telux::common::simula::log_impl( \
        ::telux::common::simula::LogLevel::INFO, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...) \
    ::telux::common::simula::log_impl( \
        ::telux::common::simula::LogLevel::WARN, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) \
    ::telux::common::simula::log_impl( \
        ::telux::common::simula::LogLevel::ERR,  __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#endif  // TELUX_COMMON_SIMULA_LOG_HPP
