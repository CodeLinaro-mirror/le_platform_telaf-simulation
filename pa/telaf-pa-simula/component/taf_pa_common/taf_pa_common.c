/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "taf_pa_common.h"

#define MAX_MSG_SIZE 1024

taf_pa_common_LogLevel_t gLogLevel = TAF_PA_COMMON_LOG_LEVEL_INFO;

static const char* LogLevelToStr
(
    taf_pa_common_LogLevel_t level
)
{
    switch (level)
    {
        case TAF_PA_COMMON_LOG_LEVEL_DEBUG:
            return " DBUG";
        case TAF_PA_COMMON_LOG_LEVEL_INFO:
            return " INFO";
        case TAF_PA_COMMON_LOG_LEVEL_NOTICE:
            return "-NTC-";
        case TAF_PA_COMMON_LOG_LEVEL_WARN:
            return "-WRN-";
        case TAF_PA_COMMON_LOG_LEVEL_ERROR:
            return "=ERR=";
        case TAF_PA_COMMON_LOG_LEVEL_CRIT:
            return "*CRT*";
        case TAF_PA_COMMON_LOG_LEVEL_ALERT:
            return "*ALT*";
        case TAF_PA_COMMON_LOG_LEVEL_EMERG:
            return "*EMR*";
        default:
            break;
    }

    return " INFO";
}

static int LogLevelToSyslog
(
    taf_pa_common_LogLevel_t level
)
{
    switch (level)
    {
        case TAF_PA_COMMON_LOG_LEVEL_DEBUG:
            return LOG_DEBUG;
        case TAF_PA_COMMON_LOG_LEVEL_INFO:
            return LOG_INFO;
        case TAF_PA_COMMON_LOG_LEVEL_NOTICE:
            return LOG_NOTICE;
        case TAF_PA_COMMON_LOG_LEVEL_WARN:
            return LOG_WARNING;
        case TAF_PA_COMMON_LOG_LEVEL_ERROR:
            return LOG_ERR;
        case TAF_PA_COMMON_LOG_LEVEL_CRIT:
            return LOG_CRIT;
        case TAF_PA_COMMON_LOG_LEVEL_ALERT:
            return LOG_ALERT;
        case TAF_PA_COMMON_LOG_LEVEL_EMERG:
            return LOG_EMERG;
        default:
            break;
    }

    return LOG_INFO;
}

void taf_pa_common_LogSetlevel
(
    taf_pa_common_LogLevel_t level
)
{
    gLogLevel = level;
}

void taf_pa_common_LogMessage
(
    taf_pa_common_LogLevel_t level,
    const char* file,
    const char* func,
    int line,
    const char* fmt,
    ...
)
{
    if (level < gLogLevel)
        return;

    const char* base = strrchr(file, '/');
    if (!base)
        base = strrchr(file, '\\');
    base = base ? base + 1 : file;

    char log[MAX_MSG_SIZE];
    snprintf(log, sizeof(log), "%s | %s %s() %d | %s", LogLevelToStr(level), base,
        (func ? func : "?"), line, fmt);

    va_list ap;
    va_start(ap, fmt);
    vsyslog(LogLevelToSyslog(level), log, ap);
    va_end(ap);
}
