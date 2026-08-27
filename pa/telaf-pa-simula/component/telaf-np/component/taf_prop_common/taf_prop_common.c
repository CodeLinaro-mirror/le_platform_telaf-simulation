/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "taf_prop_common.h"
#include <stdlib.h>

#define MAX_MSG_SIZE 1024

static taf_prop_common_LogLevel_t gLogLevel = TAF_PROP_COMMON_LOG_LEVEL_INFO;
static const taf_prop_common_LogVtable_t* gLogVtable = NULL;

static const char* LogLevelToStr
(
    taf_prop_common_LogLevel_t level
)
{
    switch (level)
    {
        case TAF_PROP_COMMON_LOG_LEVEL_DEBUG:
            return " DBUG";
        case TAF_PROP_COMMON_LOG_LEVEL_INFO:
            return " INFO";
        case TAF_PROP_COMMON_LOG_LEVEL_NOTICE:
            return "-NTC-";
        case TAF_PROP_COMMON_LOG_LEVEL_WARN:
            return "-WRN-";
        case TAF_PROP_COMMON_LOG_LEVEL_ERROR:
            return "=ERR=";
        case TAF_PROP_COMMON_LOG_LEVEL_CRIT:
            return "*CRT*";
        case TAF_PROP_COMMON_LOG_LEVEL_ALERT:
            return "*ALT*";
        case TAF_PROP_COMMON_LOG_LEVEL_EMERG:
            return "*EMR*";
        default:
            break;
    }

    return " INFO";
}

static int LogLevelToSyslog
(
    taf_prop_common_LogLevel_t level
)
{
    switch (level)
    {
        case TAF_PROP_COMMON_LOG_LEVEL_DEBUG:
            return LOG_DEBUG;
        case TAF_PROP_COMMON_LOG_LEVEL_INFO:
            return LOG_INFO;
        case TAF_PROP_COMMON_LOG_LEVEL_NOTICE:
            return LOG_NOTICE;
        case TAF_PROP_COMMON_LOG_LEVEL_WARN:
            return LOG_WARNING;
        case TAF_PROP_COMMON_LOG_LEVEL_ERROR:
            return LOG_ERR;
        case TAF_PROP_COMMON_LOG_LEVEL_CRIT:
            return LOG_CRIT;
        case TAF_PROP_COMMON_LOG_LEVEL_ALERT:
            return LOG_ALERT;
        case TAF_PROP_COMMON_LOG_LEVEL_EMERG:
            return LOG_EMERG;
        default:
            break;
    }

    return LOG_INFO;
}

/* ===== Public API ===== */

void taf_prop_common_LogBind(const taf_prop_common_LogVtable_t* vt)
{
    if (vt)
    {
        /* Basic ABI check */
        if (vt->abi_version != 1 || vt->size < sizeof(taf_prop_common_LogVtable_t))
        {
            /* Reject incompatible vtable */
            return;
        }
    }
    gLogVtable = vt;
}

void taf_prop_common_LogSetlevel
(
    taf_prop_common_LogLevel_t level
)
{
    gLogLevel = level;

    /* If vtable has set_level callback, notify PA */
    if (gLogVtable && gLogVtable->set_level)
    {
        gLogVtable->set_level(level);
    }
}

void taf_prop_common_LogMessage
(
    taf_prop_common_LogLevel_t level,
    const char* file,
    const char* func,
    int line,
    const char* fmt,
    ...
)
{
    va_list ap;
    va_start(ap, fmt);

    /* Use injected vtable if available, otherwise fallback to syslog */
    if (gLogVtable && gLogVtable->log_vprintf)
    {
        gLogVtable->log_vprintf(level, file, func, line, fmt, ap);
    }

    va_end(ap);
}
