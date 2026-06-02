/*
 *  Copyright (c) 2019-2020, The Linux Foundation. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *    * Neither the name of The Linux Foundation nor the names of its
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 *  ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 *  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 *  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 *  OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 *  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * Changes from Qualcomm Technologies, Inc. are provided under the following license:
 *
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */


/**
 * @file       ThermalShutdownManager.hpp
 *
 * @brief      When the chipset temperature reaches a critical level, automatic shutdown of entire
 *             system is triggered to avoid permanent damage. However, applications like eCall may
 *             require to prevent such shutdown during their operation.
 *             When a client disables automatic shutdown, the system internally starts a timer for
 *             auto-enablement. Upon timer expiry, the system notifies the clients and enables the
 *             automatic shutdown after the notified imminent duration.
 *             IThermalShutdownManager class provides APIs to enable/disable automatic thermal
 *             shutdown and receive relevant notifications.
 */

#ifndef TELUX_THERM_THERMALSHUTDOWNMANAGER_HPP
#define TELUX_THERM_THERMALSHUTDOWNMANAGER_HPP

#include <future>

#include <telux/common/CommonDefines.hpp>
#include <telux/therm/ThermalDefines.hpp>
#include <telux/therm/ThermalShutdownListener.hpp>

namespace telux {
namespace therm {

/** @addtogroup telematics_therm_shutdown_management
 * @{ */

/**
 * This function is called with the response to getAutoShutdownMode API.
 *
 * The callback can be invoked from multiple different threads.
 * The implementation should be thread safe.
 *
 * @param [in] mode  AutoShutdownMode
 */
using GetAutoShutdownModeResponseCb = std::function<void(AutoShutdownMode mode)>;

/**
 * @brief   IThermalShutdownManager class provides interface to enable/disable automatic thermal
 *          shutdown. Additionally it facilitates to register for notifications when the automatic
 *          shutdown mode changes.
 */
class IThermalShutdownManager {
public:
    /**
     * Checks the status of thermal shutdown management service and if the other APIs are ready for
     * use and returns the result.
     *
     * @returns  True if the services are ready otherwise false.
     *
     * @deprecated use getServiceStatus()
     */
    virtual bool isReady() = 0;

    /**
    * This status indicates whether the object is in a usable state.
    *
    * @returns  @ref telux::common::ServiceStatus
    *
    */
    virtual telux::common::ServiceStatus getServiceStatus() = 0;

    /**
     * Wait for thermal shutdown management service to be ready.
     *
     * @returns  A future that caller can wait on to be notified when thermal shutdown management
     *           service is ready.
     *
     * @deprecated The callback mechanism introduced in the
     * @ref ThermalFactory::getThermalShutdownManager with initialization callback along with
     * @ref getServiceStatus API will provide the similar mechanism as @ref onReady and
     * @ref isReady. This API will soon be removed from further releases.
     */
    virtual std::future<bool> onReady() = 0;

    /**
     * Register a listener for updates on automatic shutdown mode changes
     *
     * @param [in] listener Pointer of IThermalShutdownListener object that processes the
     *                      notification
     *
     * @returns Status of registerListener i.e success or suitable status code.
     */
    virtual telux::common::Status registerListener(std::weak_ptr<IThermalShutdownListener> listener)
                                        = 0;

    /**
     * Remove a previously registered listener.
     *
     * @param [in] listener Previously registered IThermalShutdownListener that needs to be removed
     *
     * @returns Status of deregisterListener, success or suitable status code
     */
    virtual telux::common::Status deregisterListener(
                                        std::weak_ptr<IThermalShutdownListener> listener) = 0;

    /**
     * Set automatic thermal shutdown mode.
     * When set to DISABLE mode successfully, it remains in DISABLE mode briefly and automatically
     * changes to ENABLE mode after notifying the clients.
     *
     * @param [in] mode    desired AutoShutdownMode to be set
     * @param [in] callback Optional callback to get the response of the command
     * @param [in] timeout Optional timeout(in seconds) for which auto-shutdown remains disabled.
     *
     * @returns Status of setAutoShutdownMode i.e. success or suitable status code.
     */
    virtual telux::common::Status setAutoShutdownMode(AutoShutdownMode mode,
                                        telux::common::ResponseCallback callback = nullptr,
                                        uint32_t timeout = DEFAULT_TIMEOUT) = 0;

    /**
     * Get automatic thermal shutdown mode.
     *
     * @param [in] callback GetAutoShutdownModeResponseCb to get response of the request
     *
     * @returns Status of getAutoShutdownMode i.e. success or suitable status code.
     */
    virtual telux::common::Status getAutoShutdownMode(GetAutoShutdownModeResponseCb callback) = 0;

    /**
     * Destructor of IThermalShutdownManager
     */
    virtual ~IThermalShutdownManager(){};
};
/** @} */ /* end_addtogroup telematics_therm_shutdown_management */

}  // end of namespace therm
}  // end of namespace telux

#endif // TELUX_THERM_THERMALSHUTDOWNMANAGER_HPP
