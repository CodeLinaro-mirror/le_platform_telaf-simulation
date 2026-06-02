/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file    FsListener.hpp
 *
 * @brief   FsListener provides callback methods for listening to restore indications.
 *          Client need to implement these methods. The methods in listener can be invoked
 *          from multiple threads.So the client needs to make sure that the implementation
 *          is thread-safe.
 */

#ifndef TELUX_PLATFORM_FSLISTENER_HPP
#define TELUX_PLATFORM_FSLISTENER_HPP

#include <cstdint>
#include <telux/common/CommonDefines.hpp>
#include <telux/platform/FsDefines.hpp>

namespace telux {

namespace platform {

/** @addtogroup telematics_platform_filesystem
 * @{ */

/**
 * @brief Listener class for getting notifications related to EFS backup/restore operations.
 *        The client needs to implement these methods as briefly as possible and avoid blocking
 *        calls in it. The methods in this class can be invoked from multiple different threads.
 *        Client needs to make sure that the implementation is thread-safe.
 */
class IFsListener : public common::IServiceStatusListener {
 public:
    /**
     * This function is called when a EFS restore operation is detected.
     *
     * On platforms with Access control enabled, the client needs to have
     * TELUX_PLATFORM_LISTEN_FS_EVENTS permission to receive this event.
     *
     * @param [in] event    Event related data.  @ref telux::platform::EfsEventInfo.
     *
     */
    virtual void OnEfsRestoreEvent(EfsEventInfo event) {
    }

    /**
     * This function is called when a EFS backup operation is detected.
     *
     * On platforms with Access control enabled, the client needs to have
     * TELUX_PLATFORM_LISTEN_FS_EVENTS permission to receive this event.
     *
     * @param [in] event    Event related data.  @ref telux::platform::EfsEventInfo.
     *
     */
    virtual void OnEfsBackupEvent(EfsEventInfo event) {
    }

    /**
     * When the client is about to make an eCall it is expected to invoke prepareForEcall.
     * This starts a timer within the FsManager which represents the max duration of the eCall.
     * After which the filesystem operations will resume. This API will be invoked to let the
     * client know that resumption of Fs operations is imminent. If the eCall has not yet ended,
     * the client should call prepareForEcall again to reset the timer, which will continue to
     * suspend the FS operations.
     *
     * @param [in] timeLeftToStart    The time in seconds after which filesystem operations
     *                                shall re-enable.
     *
     */
    virtual void OnFsOperationImminentEvent(uint32_t timeLeftToStart) {
    }

    /**
     * Destructor of IFsListener
     */
    virtual ~IFsListener() {
    }
};

/** @} */ /* end_addtogroup telematics_platform_filesystem */

}  // end of namespace platform

}  // end of namespace telux

#endif // TELUX_PLATFORM_FSLISTENER_HPP
