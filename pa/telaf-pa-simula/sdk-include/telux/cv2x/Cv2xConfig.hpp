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
* @file       Cv2xConfig.hpp
*
* @brief      Cv2xConfig provide operations to update or request cv2x configuration
*
*/

#ifndef TELUX_CV2X_CV2XCONFIG_HPP
#define TELUX_CV2X_CV2XCONFIG_HPP

#include <string>
#include <future>
#include <telux/common/SDKListener.hpp>
#include <telux/common/CommonDefines.hpp>
#include <telux/cv2x/Cv2xRadioTypes.hpp>

namespace telux {

namespace cv2x {
/** @addtogroup telematics_cv2x_cpp
 * @{ */

/**
 *@brief Listeners for ICv2xConfig must implement this interface.
 */
class ICv2xConfigListener : virtual public telux::common::ISDKListener {
public:
    /**
     * Called when CV2X configuration has changed in the below scenarios:
     * 1. The specified configuration source has expired.
     * 2. The active configuration source has changed to the specified
     *    configuration source type due to the expiration of the configuration
     *    source being used.
     * 3. The specified configuration source has been updated.
     *
     * @param [in] info - Information of CV2X configuration event.
     */
    virtual void onConfigChanged(const ConfigEventInfo &info) {};

    /**
     * Destructor for ICv2xConfigListener
     */
    virtual ~ICv2xConfigListener(){}
};

/**
 * @brief      Cv2xConfig provide operations to update or request cv2x configuration
 */
class ICv2xConfig {
public:
    virtual ~ICv2xConfig() {}
    /**
     * Checks if the Cv2x Config Manager is ready.
     *
     * @returns True if Cv2x Config is ready for service, otherwise
     * returns false.
     *
     * @deprecated use getServiceStatus instead
     */
    virtual bool isReady() = 0;

    /**
     * Wait for Cv2x Config to be ready.
     *
     * @returns A future that caller can wait on to be notified
     * when Cv2x Radio Manager is ready.
     *
     * @deprecated the readiness can be notified via the callback passed to
     *             Cv2xFactory::getCv2xConfig.
     *
     */
    virtual std::future<bool> onReady() = 0;

    /**
     * This status indicates whether the Cv2xConfig is in a usable state.
     *
     * @returns SERVICE_AVAILABLE    -  If cv2x config is ready for service.
     *          SERVICE_UNAVAILABLE  -  If cv2x config is temporarily unavailable.
     *          SERVICE_FAILED       -  If cv2x config encountered an irrecoverable failure.
     *
     */
    virtual telux::common::ServiceStatus getServiceStatus() = 0;

    /**
     * Updates CV2X configuration.
     * Requires CV2X TX/RX radio status be Inactive. If CV2X radio status is
     * Active or Suspended, call @ref ICv2xRadioManager::stopCv2x before
     * trying to update configuration.
     * The functionality of V2X configuration expiration is supported by adding an expiration
     * leaf to the V2X configuration file passed in. When the active configuration expires,
     * the system fallbacks to a lower priority V2X configuration @ref ConfigSourceType
     * if existed. If the V2X stauts is active, it changes to suspended when the active V2X
     * configuration expires and then changes to active after the system fallbacks to a lower
     * priority V2X configuration or changes to inactive if no V2X configuration is available.
     *
     * On platforms with access control enabled, the caller needs to have TELUX_CV2X_CONFIG
     * permission to successfully invoke this API.
     *
     * @param [in] configFilePath - Path to config file. This is the fully qualified
     *                              file path including the name of the file.
     * @param [in] cb             - Callback that is invoked when the send is complete.
     *                              This may be null.
     */
    virtual telux::common::Status updateConfiguration(
        const std::string& configFilePath, telux::common::ResponseCallback cb) = 0;
    /**
     * Retrieve active CV2X configuration.
     * The calling application should have write access to the path specified
     * by configFilePath. And if the v2x configuration retrieval request succeed,
     * the file specified by configFilePath will be created and filled with the
     * configuration contents. Otherwise, no file will be created.
     *
     * On platforms with access control enabled, the caller needs to have TELUX_CV2X_CONFIG
     * permission to successfully invoke this API.
     *
     * @param [in] configFilePath - Path to config file. This is the fully qualified
     *                              file path including the name of the file.
     * @param [in] cb             - Callback that is invoked when the configuration retrival
     *                              is complete. This may be null.
     */
    virtual telux::common::Status retrieveConfiguration(
        const std::string& configFilePath, telux::common::ResponseCallback cb) = 0;

     /**
      * Registers a listener for this ICv2xConfig.
      *
      * @param [in] listener - Listener that implements ICv2xConfigListener interface.
      */
     virtual telux::common::Status registerListener(
         std::weak_ptr<ICv2xConfigListener> listener) = 0;

     /**
      * Deregisters a listener for this ICv2xConfig.
      *
      * @param [in] listener - Previously registered ICv2xConfigListener that is to be
      *        deregistered.
      */
     virtual telux::common::Status deregisterListener(
         std::weak_ptr<ICv2xConfigListener> listener) = 0;
};

/** @} */ /* end_addtogroup telematics_cv2x_cpp */

} // namespace cv2x

} // namespace telux




#endif // TELUX_CV2X_CV2XCONFIG_HPP
