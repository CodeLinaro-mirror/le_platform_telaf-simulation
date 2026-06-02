/*
 *  Copyright (c) 2020, The Linux Foundation. All rights reserved.
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
* @file       Cv2xThrottleManager.hpp
*
* @brief      Cv2xThrottleManager class encapsulate ThrottleManager client interface APIs
*
* @note    Eval: This is a new API and is being evaluated. It is subject to change
*          and could break backwards compatibility.
*
*/

#ifndef TELUX_CV2X_CV2XTHROTTLEMANAGER_HPP
#define TELUX_CV2X_CV2XTHROTTLEMANAGER_HPP

#include <future>
#include <memory>

#include <telux/common/SDKListener.hpp>
#include <telux/common/CommonDefines.hpp>

namespace telux {
namespace cv2x {
/** @addtogroup telematics_cv2x_cpp
 *  * @{ */

/**
 * This function is called as a response to @ref ICv2xThrottleManager::setVerificationLoad
 *
 * @param [in] error     - SUCCESS if verification load was set successfully
 *                       - SUCCESS
 *                       - GENERIC_FAILURE
 *
 */
using setVerificationLoadCallback = std::function<void (telux::common::ErrorCode error)>;

/**
 * @brief Listener class for getting filter rate update notification.
 *
 */
class ICv2xThrottleManagerListener : public telux::common::ISDKListener {
public:
   /**
    * This API is invoked to advise the client to adjust the incoming message filtering rate
    * by @p rate messages/second.
    * If the @p rate is positive, it indicates the client to filter @p rate more messages/second
    * If the @p rate is negative, it indicates the client to filter @p rate less messages/second
    *
    * @param [in] rate the reported filter rate adjustment value.
    *
    *
    */
    virtual void onFilterRateAdjustment(int rate) {}
    /**
    * This API is invoked when the service status changes for example when a subsytem restart (SSR)
    * occurs
    *
    * @param [in] status - @ref telux::common::ServiceStatus
    *
    */
    virtual void onServiceStatusChange(telux::common::ServiceStatus status) {}
    /**
    * This API is invoked when capacity of underlying message verification module is tampered
    *
    * @param [in] state - mvm state, true if mvm is in good state, otherwise false.
    *
    * @note    Eval: This is a new API and is being evaluated. It is subject to change
    *          and could break backwards compatibility.
    */
    virtual void onSanityStateUpdate(bool state) {}

   /**
    * Destructor of ICv2xThrottleManagerListener
    */
    virtual ~ICv2xThrottleManagerListener() {}
};

/**
 * @brief ThrottleManager provides throttle manager client interface.
 *
 * ThrottleManager provides APIs that allows applications to specify the incoming verification load
 * on the system. This is used to make decisions on how to optimally use the resources
 * available in the system. The API also provides feedback to clients on the suggested filtering
 * that needs to be done when the incoming message verification rate exceeds the instantaneous
 * system capacity.
 *
 */
class ICv2xThrottleManager {
public:

    /**
     * This status indicates whether the object is in a usable state.
     *
     * @returns SERVICE_AVAILABLE    -  If location manager is ready for service.
     *          SERVICE_UNAVAILABLE  -  If location manager is temporarily unavailable.
     *          SERVICE_FAILED       -  If location manager encountered an irrecoverable failure.
     *
     */
    virtual telux::common::ServiceStatus getServiceStatus() = 0;

    /**
     * Registers a listener to receive the updated filer rate adjustment data.
     *
     * @param[in] listener - Listener that implement ICv2xThrottleManagerListener interface.
     */
    virtual telux::common::Status registerListener(
            std::weak_ptr<ICv2xThrottleManagerListener> listener) = 0;

    /**
     * Deregister a ICv2xThrottleManagerListener.
     *
     * @param[in] listener - Previously registered Cv2xThrottleManagerListener that is
     *            deregistered.
     */
    virtual telux::common::Status deregisterListener(
            std::weak_ptr<ICv2xThrottleManagerListener> listener) = 0;

    /**
     * Set current measured/average verification load.
     *
     * @param[in] load - current measured verification load(verification/second).
     *
     * @param[in] cb   - callack for indicating the result of set verfication load.
     *
     */
    virtual telux::common::Status setVerificationLoad(int load, setVerificationLoadCallback cb) = 0;

    virtual ~ICv2xThrottleManager() {}
};

/** @} */ /* end_addtogroup telematics_cv2x_cpp */

} // namespace cv2x

} // namespace telux

#endif // TELUX_CV2X_CV2XTHROTTLEMANAGER_HPP
