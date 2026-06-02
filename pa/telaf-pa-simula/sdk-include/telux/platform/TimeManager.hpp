/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */


/**
 * @file       TimeManager.hpp
 * @brief      TimeManager provides APIs to register and deregister
 *             a listener for time reports.
 */

#ifndef TELUX_PLATFORM_TIMEMANAGER_HPP
#define TELUX_PLATFORM_TIMEMANAGER_HPP

#include <memory>
#include <bitset>

#include <telux/common/CommonDefines.hpp>
#include <telux/platform/TimeListener.hpp>

namespace telux {

namespace platform {
/** @addtogroup telematics_platform_time
 * @{ */

/**
 * Defines supported utc report types.
 */
enum SupportedTimeType {
    GNSS_UTC_TIME = 0,    /**< GNSS UTC time derived from location fix. */
    CV2X_UTC_TIME  = 1,   /**< UTC time derived from injected UTC when the
                               vehicle has selected a roadside unit as the
                               synchronization reference for V2X communication. */
    MAX_SUPPORTED_TIME_TYPES
};

/**
 * Bit mask that denotes which of the time types defined in
 * @ref SupportedTimeType are supported.
 */
using TimeTypeMask = std::bitset<MAX_SUPPORTED_TIME_TYPES>;

/**
 * @brief   ITimeManager provides interface to retrieve time information.
 */
class ITimeManager {
 public:
    /**
     * This status indicates whether the object is in a usable state.
     *
     * @returns @ref telux::common::ServiceStatus indicating the current status of the device info
     *          service.
     *
     */
    virtual telux::common::ServiceStatus getServiceStatus() = 0;

    /**
     * Registers the listener for time updates.
     * This will result in frequent notifications and will result in
     * wakeups when system is suspended. If wakeups are not desired
     * then deregister should be called.
     *
     * @param [in] listener      - pointer to implemented listener.
     * @param [in] mask          - mask to indicate which times the client is
     *                             interested in registering for.
     *
     * @returns status of the registration request.
     *
     */
    virtual telux::common::Status registerListener(std::weak_ptr<ITimeListener> listener,
        TimeTypeMask mask) = 0;

    /**
     * Deregisters the previously registered listener for time updates.
     *
     * @param [in] listener      - pointer to registered listener that needs to be removed.
     * @param [in] mask          - mask to indicate which times the client has registering for.
     *
     * @returns status of the deregistration request.
     *
     */
    virtual telux::common::Status deregisterListener(std::weak_ptr<ITimeListener> listener,
        TimeTypeMask mask) = 0;

    /**
     * Destructor of ITimeManager
     */
    virtual ~ITimeManager(){};
};

/** @} */ /* end_addtogroup telematics_platform_time */
}  // end of namespace platform

}  // end of namespace telux

#endif // TELUX_PLATFORM_TIMEMANAGER_HPP
