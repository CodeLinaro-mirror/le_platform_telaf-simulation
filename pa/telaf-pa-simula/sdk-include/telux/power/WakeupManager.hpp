/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file WakeupManager.hpp
 * @brief Provides ability to monitor wake up due to QMI communication.
 */

#ifndef TELUX_POWER_WAKEUPMANAGER_HPP
#define TELUX_POWER_WAKEUPMANAGER_HPP

#include <unistd.h>

#include <cstdint>
#include <memory>
#include <string>

#include <telux/common/CommonDefines.hpp>

namespace telux {
namespace power {

/** @addtogroup telematics_power_wakeup_manager
 * @{ */

/**
 * Wakeup initiator.
 */
enum class WakeupType {
    /** Source of wakeup could not be determined. */
    UNKNOWN,

    /** A QMI transaction caused the wakeup */
    QMI
};

/**
 * Details of the QMI message.
 */
struct QmiWakeupInfo {
    /**
     * QMI service identifier.
     */
    uint32_t serviceId;

    /**
     * QMI source node identifier. Represents the node from which the message originated.
     */
    uint32_t sourceNodeId;

    /**
     * QMI destination node identifier. Represents the node to which message will
     * be delivered.
     */
    uint32_t destinationNodeId;

    /**
     * True, if the msgId field is set and has a valid value for this
     * transaction otherwise false.
     */
    bool isMsgIdValid;

    /**
     * QMI message identifier.
     */
    uint32_t msgId;

    /**
     * True, if the pid field is set and has a valid value for this
     * transaction otherwise false.
     */
    bool isPIDValid;

    /**
     * Linux process ID (PID) of the process which received this message.
     */
    pid_t pid;

    /**
     * True, if the processName field is set and has a valid value for
     * this transaction otherwise false.
     */
    bool isProcessNameValid;

    /**
     * Name of the Linux process (upto 16 characters) who received this message.
     */
    std::string processName;
};

/**
 * Details of the activity that caused system wakeup.
 */
struct WakeupInfo {
    /**
     * Details of the QMI message.
     */
    QmiWakeupInfo qmiWakeupInfo;

    /**
     * Wakeup initiator.
     */
    WakeupType wakeupType;
};

/**
 * Receives notification whenever the subsystem is woken up. Currently,
 * only QMI transaction based wakeup is supported.
 */
class IWakeupListener : public telux::common::IServiceStatusListener {
 public:
    /**
     * Invoked whenever the system is woken up and provides the details
     * of the activity that caused wakeup.
     *
     * On platforms with access control enabled, caller needs to have TELUX_POWER_WAKEUP_INFO
     * permission to invoke this API successfully.
     *
     * @param[in] wakeupInfo details of the activity that caused the wake up
     */
    virtual void onWakeup(WakeupInfo wakeupInfo) { }

    /**
     * Destructor for IWakeUpListener.
     */
    virtual ~IWakeupListener() { }
};

/**
 * IWakeupManager is used to monitor application processor wake up due to QMI communication.
 */
class IWakeupManager {
 public:
   /**
    * Registers the given listener to receive activity details upon system wake up via
    * @ref IWakeupListener::onWakeup().
    *
    * @param[in] listener Receives notifications
    *
    * @returns @ref telux::common::ErrorCode::SUCCESS if the listener is registered,
    *          otherwise, an appropriate error code
    *
    * @note Eval: This is a new API and is being evaluated. It is subject
    *             to change and could break backwards compatibility.
    */
   virtual telux::common::ErrorCode registerListener(
        std::weak_ptr<IWakeupListener> listener) = 0;

   /**
    * Deregisters the given listener registered previously with @ref registerListener().
    *
    * @param[in] listener Listener to deregister
    *
    * @returns @ref telux::common::ErrorCode::SUCCESS if the listener is deregistered,
    *          otherwise, an appropriate error code
    *
    * @note Eval: This is a new API and is being evaluated. It is subject
    *             to change and could break backwards compatibility.
    */
   virtual telux::common::ErrorCode deRegisterListener(
        std::weak_ptr<IWakeupListener> listener) = 0;

   /**
    * Gets the wakeup manager's service status.
    *
    * @returns @ref telux::common::ServiceStatus::SERVICE_AVAILABLE if the service is ready
    *          for use, @ref telux::common::ServiceStatus::SERVICE_UNAVAILABLE if the service
    *          is temporarily unavailable (possibly undergoing initialization),
    *          @ref telux::common::ServiceStatus::SERVICE_FAILED if the service needs
    *          re-initialization
    */
   virtual telux::common::ServiceStatus getServiceStatus() = 0;

   /**
    * Performs cleanup and destroys the IWakeUpManager instance.
    */
   virtual ~IWakeupManager() {};
};

/** @} */ /* end_addtogroup telematics_power_wakeup_manager */

}  // End of namespace power
}  // End of namespace telux

#endif // TELUX_POWER_WAKEUPMANAGER_HPP
