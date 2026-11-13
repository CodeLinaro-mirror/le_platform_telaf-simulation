/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*
 * ABI STABILITY REQUIREMENT
 * This header is part of the prop interface.
 * All declarations must use C-style linkage only (no C++ classes,
 * templates, references, or overloaded functions) to guarantee ABI
 * stability across independently compiled shared libraries.
 */

#ifndef TAF_PROP_COMMON_H
#define TAF_PROP_COMMON_H

#include <stdarg.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Result codes returned by all PROP API functions.
 *
 * ## POSIX errno mapping strategy
 *
 * When a PROP implementation calls a POSIX function that fails, it reads
 * `errno` and applies the following two-tier strategy:
 *
 * **Tier 1 — mapped errno:**
 *   If the errno value has a direct mapping in this enum (see the
 *   "maps from" annotations on each enumerator), the corresponding
 *   `taf_prop_result_t` value is returned and `*underlyingErrPtr` is set
 *   to `TAF_PROP_UNDERLYING_ERR_NONE`.
 *
 * **Tier 2 — unmapped errno:**
 *   If the errno value does NOT appear in the mapping table, the function
 *   returns `TAF_PROP_SYSTEM_ERROR` and writes the raw `errno` value into
 *   `*underlyingErrPtr` (when the caller passes a non-NULL pointer).
 *   The caller should inspect `*underlyingErrPtr` to determine the exact
 *   system-level failure. `strerror((int)*underlyingErrPtr)` can be used
 *   to obtain a human-readable description for debugging.
 *
 * ## Prop component failures (QMI, MRCD, QCMAP, …)
 *
 * When a prop subsystem API (e.g. QMI, MRCD, QCMAP) fails, the
 * implementation returns `TAF_PROP_FAULT`. The prop error codes from
 * these subsystems are intentionally NOT propagated to callers because they
 * are not part of the open-source interface and cannot be exposed to OSS
 * layers. `*underlyingErrPtr` is set to `TAF_PROP_UNDERLYING_ERR_NONE` in
 * this case; detailed diagnostics are available only via the component's
 * internal log output.
 *
 * ## underlyingErrPtr contract (summary)
 *
 * | Return value            | *underlyingErrPtr (when non-NULL)          |
 * |-------------------------|--------------------------------------------|
 * | TAF_PROP_SYSTEM_ERROR   | raw errno from the failing POSIX call      |
 * | any other value         | TAF_PROP_UNDERLYING_ERR_NONE (0xFFFFFFFFU) |
 *
 * Passing NULL for `underlyingErrPtr` is always safe; the implementation
 * checks for NULL before writing.
 */
typedef enum
{
    /// Success
    TAF_PROP_OK = 0,
    /// Resource not found [maps from -ENOENT (-2)]
    TAF_PROP_NOT_FOUND = -1,
    /// Operation not possible in current state
    TAF_PROP_NOT_POSSIBLE = -2,
    /// Value out of allowed range [maps from -ERANGE (-34)]
    TAF_PROP_OUT_OF_RANGE = -3,
    /// Memory allocation failure [maps from -ENOMEM (-12)]
    TAF_PROP_NO_MEMORY = -4,
    /// Operation not permitted [maps from -EPERM (-1) / -EACCES (-13)]
    TAF_PROP_NOT_PERMITTED = -5,
    /**
     * General fault. Returned in two cases:
     *  1. A POSIX call failed with -EFAULT (-14).
     *  2. A prop subsystem API (QMI, MRCD, QCMAP, NAD) failed.
     *     Prop error codes are not propagated to callers because they
     *     cannot be exposed to OSS layers; see internal logs for details.
     * In both cases *underlyingErrPtr is set to TAF_PROP_UNDERLYING_ERR_NONE.
     */
    TAF_PROP_FAULT = -6,
    /// Communication error
    TAF_PROP_COMM_ERROR = -7,
    /// Operation timed out [maps from -ETIMEDOUT (-110)]
    TAF_PROP_TIMEOUT = -8,
    /// Buffer or value overflow [maps from -ENOSPC (-28) / -EOVERFLOW (-75)]
    TAF_PROP_OVERFLOW = -9,
    /// Buffer or value underflow
    TAF_PROP_UNDERFLOW = -10,
    /// Operation would block [maps from -EAGAIN (-11) / -EWOULDBLOCK (-11)]
    TAF_PROP_WOULD_BLOCK = -11,
    /// Deadlock detected [maps from -EDEADLK (-35)]
    TAF_PROP_DEADLOCK = -12,
    /// Data format error [maps from -EILSEQ (-84) / -EBADMSG (-74)]
    TAF_PROP_FORMAT_ERROR = -13,
    /// Duplicate entry [maps from -EEXIST (-17)]
    TAF_PROP_DUPLICATE = -14,
    /// Invalid parameter [maps from -EINVAL (-22)]
    TAF_PROP_BAD_PARAMETER = -15,
    /// Resource is closed [maps from -EBADF (-9) / -EPIPE (-32)]
    TAF_PROP_CLOSED = -16,
    /// Resource is busy [maps from -EBUSY (-16)]
    TAF_PROP_BUSY = -17,
    /// Feature not supported [maps from -ENOTSUP (-95) / -EOPNOTSUPP (-95)]
    TAF_PROP_UNSUPPORTED = -18,
    /// I/O error [maps from -EIO (-5)]
    TAF_PROP_IO_ERROR = -19,
    /// Function not implemented [maps from -ENOSYS (-38)]
    TAF_PROP_NOT_IMPLEMENTED = -20,
    /// Resource unavailable [maps from -ENODEV (-19) / -ENXIO (-6)]
    TAF_PROP_UNAVAILABLE = -21,
    /// Operation or session terminated [maps from -ECANCELED (-125)]
    TAF_PROP_TERMINATED = -22,
    /// Operation already in progress
    /// [maps from -EINPROGRESS (-115) / -EALREADY (-114)]
    TAF_PROP_IN_PROGRESS = -23,
    /// Operation suspended
    TAF_PROP_SUSPENDED = -24,
    /**
     * A POSIX system call failed with an errno value that is not mapped to
     * any other enumerator in this enum (Tier 2 error — see class doc above).
     *
     * When this value is returned, the raw `errno` from the failing call is
     * written into `*underlyingErrPtr` (if the caller passed a non-NULL
     * pointer). The caller must inspect `*underlyingErrPtr` to determine the
     * exact system-level failure. `strerror((int)*underlyingErrPtr)` can be
     * used to obtain a human-readable description for debugging.
     */
    TAF_PROP_SYSTEM_ERROR = -1000,
} taf_prop_result_t;

/**
 * Sentinel value written to the optional `underlyingErrPtr` output parameter
 * when the function does NOT return `TAF_PROP_SYSTEM_ERROR`.
 *
 * ## Full contract for `underlyingErrPtr`
 *
 * Every PROP API function that can fail due to a POSIX system call accepts an
 * optional `uint32_t* underlyingErrPtr` output parameter.  The rules are:
 *
 *  - **TAF_PROP_SYSTEM_ERROR returned:**
 *    The POSIX call failed with an errno that is not mapped to any named
 *    `taf_prop_result_t` enumerator.  The raw `errno` value is written into
 *    `*underlyingErrPtr` so the caller can inspect the exact system error.
 *    Use `strerror((int)*underlyingErrPtr)` to obtain a human-readable
 *    description for debugging purposes.
 *
 *  - **Any other value returned (including TAF_PROP_FAULT):**
 *    `*underlyingErrPtr` is set to `TAF_PROP_UNDERLYING_ERR_NONE`
 *    (0xFFFFFFFFU) to indicate that no underlying error code is available to
 *    the caller.  This covers success (`TAF_PROP_OK`), all mapped POSIX error
 *    codes, and prop subsystem failures (QMI, MRCD, QCMAP, …) whose
 *    error codes cannot be exposed to OSS layers.
 *
 *  - **NULL pointer:**
 *    Passing NULL is always safe.  The implementation checks for NULL before
 *    writing, so callers that do not need the underlying error may pass NULL.
 *
 * Example usage:
 * @code
 *   uint32_t sysErr;
 *   taf_prop_result_t rc = taf_prop_file_Open(path, flags, mode, &fd, &sysErr);
 *   if (rc == TAF_PROP_SYSTEM_ERROR) {
 *       // sysErr holds the raw errno value
 *       TAF_PROP_ERROR("open failed: %s", strerror((int)sysErr));
 *   }
 * @endcode
 */
#define TAF_PROP_UNDERLYING_ERR_NONE 0xFFFFFFFFU

#define TAF_PROP_SHARED __attribute__((visibility("default")))

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
 * Provided by PA at runtime. PA-prop / PA-noship must not call syslog/DLT directly.
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
TAF_PROP_SHARED void taf_prop_common_LogBind(const taf_prop_common_LogVtable_t* vt);

TAF_PROP_SHARED void taf_prop_common_LogSetlevel(taf_prop_common_LogLevel_t level);

TAF_PROP_SHARED void taf_prop_common_LogMessage
(
    taf_prop_common_LogLevel_t level,
    const char* file,
    const char* func,
    int line,
    const char* fmt,
    ...
);

#define TAF_PROP_DEBUG(fmt, ...) taf_prop_common_LogMessage( \
    TAF_PROP_COMMON_LOG_LEVEL_DEBUG,  __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__)
#define TAF_PROP_INFO(fmt, ...) taf_prop_common_LogMessage( \
    TAF_PROP_COMMON_LOG_LEVEL_INFO,   __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__)
#define TAF_PROP_NOTICE(fmt, ...) taf_prop_common_LogMessage( \
    TAF_PROP_COMMON_LOG_LEVEL_NOTICE, __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__)
#define TAF_PROP_WARN(fmt, ...) taf_prop_common_LogMessage( \
    TAF_PROP_COMMON_LOG_LEVEL_WARN,   __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__)
#define TAF_PROP_ERROR(fmt, ...) taf_prop_common_LogMessage( \
    TAF_PROP_COMMON_LOG_LEVEL_ERROR,  __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__)
#define TAF_PROP_CRIT(fmt, ...) taf_prop_common_LogMessage( \
    TAF_PROP_COMMON_LOG_LEVEL_CRIT,   __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__)
#define TAF_PROP_ALERT(fmt, ...) taf_prop_common_LogMessage( \
    TAF_PROP_COMMON_LOG_LEVEL_ALERT,  __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__)
#define TAF_PROP_EMERG(fmt, ...) taf_prop_common_LogMessage( \
    TAF_PROP_COMMON_LOG_LEVEL_EMERG,  __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__)

#define TAF_PROP_FATAL(fmt, ...) \
    do { TAF_PROP_EMERG(fmt, ##__VA_ARGS__); exit(EXIT_FAILURE); } while (0)
#define TAF_PROP_FATAL_IF(condition, fmt, ...) \
    do { if (condition) { TAF_PROP_FATAL(fmt, ##__VA_ARGS__); } } while (0)
#define TAF_PROP_ASSERT(condition) \
    TAF_PROP_FATAL_IF(!(condition), "Assert Failed: '%s'", #condition)

#ifdef __cplusplus
}
#endif

#endif // TAF_PROP_COMMON_H
