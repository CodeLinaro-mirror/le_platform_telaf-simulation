/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file       FsDefines.hpp
 *
 * @brief      This file contains enumerations and variables used for filesystem susbsystem.
 *
 */

#ifndef TELUX_PLATFORM_FSDEFINES_HPP
#define TELUX_PLATFORM_FSDEFINES_HPP

#include <telux/common/CommonDefines.hpp>

namespace telux {

namespace platform {
/** @addtogroup telematics_platform_filesystem
 * @{ */

/* Enum to denote the EFS backup/restore operation state */
enum class EfsEvent {
    START, /**< Indicating the beginning of Backup/Restore operation */
    END,   /**< Indicating the completion of Backup/Restore operation */
};

/* EfsEventInfo captures the event related data */
struct EfsEventInfo {
    EfsEvent event;                 /**< The event being notified */
    telux::common::ErrorCode error; /**< @ref telux::common::ErrorCode associated with the event */
};

/* Enum to denote status of operations */
enum class OperationStatus {
    UNKNOWN,
    SUCCESS, /*< Indicates a successful operation*/
    FAILURE, /*< Indicates a failed operation*/
};

/* Enum to denote ota operation */
enum class OtaOperation {
    INVALID,
    START,  /*< Used whenever the client is starting an OTA operation*/
    RESUME, /*< Used whenever the client is resuming a previously started OTA operation*/
};

/** @} */ /* end_addtogroup telematics_platform_filesystem */
}  // end of namespace platform

}  // end of namespace telux

#endif // TELUX_PLATFORM_FSDEFINES_HPP
