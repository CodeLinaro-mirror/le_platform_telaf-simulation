/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file       FsManager.hpp
 * @brief      FsManager provides APIs related to File System(FS) management such as notifying
 *             the backup/restore operations.
 */

#ifndef TELUX_PLATFORM_FSMANAGER_HPP
#define TELUX_PLATFORM_FSMANAGER_HPP

#include <memory>

#include <telux/common/CommonDefines.hpp>
#include <telux/platform/FsDefines.hpp>
#include <telux/platform/FsListener.hpp>

namespace telux {

namespace platform {
/** @addtogroup telematics_platform_filesystem
 * @{ */

/**
 * @brief   IFsManager provides interface to to control and get notified about file system
 *          operations. This includes Embedded file system (EFS) operations.
 */
class IFsManager {
 public:
    /**
     * This status indicates whether the object is in a usable state.
     *
     * @returns @ref telux::common::ServiceStatus indicating the current status of the file system
     *          service.
     *
     */
    virtual telux::common::ServiceStatus getServiceStatus() = 0;

    /**
     * Registers the listener for FileSystem Manager indications.
     *
     * @param [in] listener      - pointer to implemented listener.
     *
     * @returns status of the registration request.
     *
     */
    virtual telux::common::Status registerListener(std::weak_ptr<IFsListener> listener) = 0;

    /**
     * Deregisters the previously registered listener.
     *
     * @param [in] listener      - pointer to registered listener that needs to be removed.
     *
     * @returns status of the deregistration request.
     *
     */
    virtual telux::common::Status deregisterListener(std::weak_ptr<IFsListener> listener) = 0;

    /**
     * Request to trigger an EFS backup. If the request is successful, the status of EFS backup
     * is notified via @ref telux::platform::IFsListener::OnEfsBackupEvent.
     *
     * On platforms with Access control enabled, Caller needs to have TELUX_PLATFORM_FS_OPS_CTRL
     * permission to invoke this API successfully.
     *
     * @returns The status of the request - @ref telux::common::Status
     *
     */
    virtual telux::common::Status startEfsBackup() = 0;

    /**
     * The Filesystem Manager performs periodic operations which might be resource intensive.
     * Such operations are not desired during other crucial events like an eCall. To avoid
     * performing such operations during such events, the client is recommended to invoke
     * this API before it initiates an eCall. This allows the filesystem manager to prepare
     * the system to restrict any resource intensive operations like filesystem scrubbing
     * during the eCall.
     *
     * On platforms with Access control enabled, Caller needs to have TELUX_TEL_ECALL_MGMT
     * permission to invoke this API successfully.
     *
     * @note - The client would need to periodically invoke this API to ensure that the timer
     *         gets reset so that operations do not get re-enabled.
     *
     * @returns - @ref telux::common::Status
     *
     */
    virtual telux::common::Status prepareForEcall() = 0;

    /**
     * Once ecall complete, the client should invoke this API to re-enable filesystem
     * operations like filesystem scrubbing.If the API invocation results in
     * @ref telux::common::Status::NOTREADY,indicating that the sub-system is not ready,
     * the client should retry.
     *
     *
     * On platforms with Access control enabled, Caller needs to have TELUX_TEL_ECALL_MGMT
     * permission to invoke this API successfully.
     *
     * @returns - @ref telux::common::Status
     *
     */
    virtual telux::common::Status eCallCompleted() = 0;

    /**
     * This API should be invoked to allow the filesystem manager to perform operations
     * like prepare the filesystem for an OTA. In addition to this preparation, any
     * on-going operations like scrubbing is stopped.
     *
     * On platforms with Access control enabled, Caller needs to have TELUX_PLATFORM_OTA_MGMT
     * permission to invoke this API successfully.
     *
     * @param [in] otaOperation  - @ref telux::platform::OtaOperation.
     *
     * @param [out] responseCb   - @ref telux::common::ResponseCallback
     * The callback method to be invoked upon completion of OTA preparation and the response
     * is indicated asynchronously.
     *
     * @returns - @ref telux::common::Status
     *
     */
    virtual telux::common::Status prepareForOta(
        OtaOperation otaOperation, telux::common::ResponseCallback responseCb)
        = 0;

    /**
     * This API should be invoked upon completion of OTA, this will allow the filesystem
     * manager to perform post OTA verifications and re-enable operations that were
     * disabled for performing the OTA, like scrubbing.
     *
     * On platforms with Access control enabled, Caller needs to have TELUX_PLATFORM_OTA_MGMT
     * permission to invoke this API successfully.
     *
     * @param [in] operationStatus  - @ref telux::platform::OperationStatus
     * The status of the OTA operation that the client attempted.
     *
     * @param [out] responseCb   - @ref telux::common::ResponseCallback
     * The callback method to be invoked upon completion of OTA related filesystem
     * verifications and the response is indicated asynchronously.
     *
     * @returns - @ref telux::common::Status
     *
     */
    virtual telux::common::Status otaCompleted(
        OperationStatus operationStatus, telux::common::ResponseCallback responseCb)
        = 0;

    /**
     * This API should be invoked when the client decides to mirror the active partition
     * to the inactive partition.
     *
     * On platforms with Access control enabled, Caller needs to have TELUX_PLATFORM_OTA_MGMT
     * permission to invoke this API successfully.
     *
     * @param [out] responseCb   - @ref telux::common::ResponseCallback
     * The callback method to be invoked when the mirroring operation is completed and
     * the response is indicated asynchronously.
     *
     * @returns - @ref telux::common::Status
     *
     */
    virtual telux::common::Status startAbSync(telux::common::ResponseCallback responseCb) = 0;

    /**
     * Destructor of IFsManager
     */
    virtual ~IFsManager(){};
};

/** @} */ /* end_addtogroup telematics_platform_filesystem */
}  // end of namespace platform

}  // end of namespace telux

#endif // TELUX_PLATFORM_FSMANAGER_HPP
