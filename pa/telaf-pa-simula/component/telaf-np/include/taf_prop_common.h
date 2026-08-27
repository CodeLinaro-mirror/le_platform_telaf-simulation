/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef TAF_PROP_COMMON_H
#define TAF_PROP_COMMON_H

#include <stdint.h>
#include <stdarg.h>
#include <syslog.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int32_t prop_result_t;

#define PROP_SHARED __attribute__((visibility("default")))

typedef enum {
    TAF_PROP_OK = 0,
    TAF_PROP_NOT_FOUND = -1,
    TAF_PROP_NOT_POSSIBLE = -2,
    TAF_PROP_OUT_OF_RANGE = -3,
    TAF_PROP_NO_MEMORY = -4,
    TAF_PROP_NOT_PERMITTED = -5,
    TAF_PROP_FAULT = -6,
    TAF_PROP_COMM_ERROR = -7,
    TAF_PROP_TIMEOUT = -8,
    TAF_PROP_OVERFLOW = -9,
    TAF_PROP_UNDERFLOW = -10,
    TAF_PROP_WOULD_BLOCK = -11,
    TAF_PROP_DEADLOCK = -12,
    TAF_PROP_FORMAT_ERROR = -13,
    TAF_PROP_DUPLICATE = -14,
    TAF_PROP_BAD_PARAMETER = -15,
    TAF_PROP_CLOSED = -16,
    TAF_PROP_BUSY = -17,
    TAF_PROP_UNSUPPORTED = -18,
    TAF_PROP_IO_ERROR = -19,
    TAF_PROP_NOT_IMPLEMENTED = -20,
    TAF_PROP_UNAVAILABLE = -21,
    TAF_PROP_TERMINATED = -22,
    TAF_PROP_IN_PROGRESS = -23,
    TAF_PROP_SUSPENDED = -24
} prop_result_enum_t;

typedef enum
{
    TAF_PROP_COMMON_LOG_LEVEL_DEBUG = 0,
    TAF_PROP_COMMON_LOG_LEVEL_INFO = 1,
    TAF_PROP_COMMON_LOG_LEVEL_NOTICE = 2,
    TAF_PROP_COMMON_LOG_LEVEL_WARN = 3,
    TAF_PROP_COMMON_LOG_LEVEL_ERROR = 4,
    TAF_PROP_COMMON_LOG_LEVEL_CRIT = 5,
    TAF_PROP_COMMON_LOG_LEVEL_ALERT = 6,
    TAF_PROP_COMMON_LOG_LEVEL_EMERG = 7
} taf_prop_common_LogLevel_t;

/* ===== Injected logging interface (vtable) =====
 * Provided by PA at runtime. PA-noship must not call syslog/DLT directly.
 */
typedef struct
{
    /* ABI guard for compatibility */
    unsigned int abi_version;
    unsigned int size;

    /* printf-style logging using va_list */
    void (*log_vprintf)(taf_prop_common_LogLevel_t level,
                        const char* file,
                        const char* func,
                        int line,
                        const char* fmt,
                        va_list ap);

    /* optional: allow PA to push policy (can be NULL) */
    void (*set_level)(taf_prop_common_LogLevel_t level);

} taf_prop_common_LogVtable_t;

/* Called by PA to inject/clear the vtable. */
PROP_SHARED void taf_prop_common_LogBind(const taf_prop_common_LogVtable_t* vt);

PROP_SHARED void taf_prop_common_LogSetlevel
(
    taf_prop_common_LogLevel_t level
);

PROP_SHARED void taf_prop_common_LogMessage
(
    taf_prop_common_LogLevel_t level,
    const char* file,
    const char* func,
    int line,
    const char* fmt,
    ...
);

#define PROP_DEBUG(fmt, ...) taf_prop_common_LogMessage(TAF_PROP_COMMON_LOG_LEVEL_DEBUG, __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__)
#define PROP_INFO(fmt, ...) taf_prop_common_LogMessage(TAF_PROP_COMMON_LOG_LEVEL_INFO,  __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__)
#define PROP_NOTICE(fmt, ...) taf_prop_common_LogMessage(TAF_PROP_COMMON_LOG_LEVEL_NOTICE,  __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__)
#define PROP_WARN(fmt, ...) taf_prop_common_LogMessage(TAF_PROP_COMMON_LOG_LEVEL_WARN,  __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__)
#define PROP_ERROR(fmt, ...) taf_prop_common_LogMessage(TAF_PROP_COMMON_LOG_LEVEL_ERROR,   __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__)
#define PROP_CRIT(fmt, ...) taf_prop_common_LogMessage(TAF_PROP_COMMON_LOG_LEVEL_CRIT,  __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__)
#define PROP_ALERT(fmt, ...) taf_prop_common_LogMessage(TAF_PROP_COMMON_LOG_LEVEL_ALERT,  __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__)
#define PROP_EMERG(fmt, ...) taf_prop_common_LogMessage(TAF_PROP_COMMON_LOG_LEVEL_EMERG,  __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__)

#define PROP_FATAL(fmt, ...) { PROP_EMERG(fmt, ##__VA_ARGS__); exit(EXIT_FAILURE); }
#define PROP_FATAL_IF(condition, fmt, ...) if (condition) { PROP_FATAL(fmt, ##__VA_ARGS__) }
#define PROP_ASSERT(condition) PROP_FATAL_IF(!(condition), "Assert Failed: '%s'", #condition)

#ifdef __cplusplus
}
#endif

#endif // TAF_PROP_COMMON_H
