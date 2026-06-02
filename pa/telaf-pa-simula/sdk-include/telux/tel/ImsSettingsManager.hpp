/*
 *  Copyright (c) 2021, The Linux Foundation. All rights reserved.
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
 * @file       ImsSettingsManager.hpp
 * @brief      ImsSettingsManager allows to set or get IMS service conﬁguration parameters.
 *             For example to enable or disable the IMS service, VOIMS service.
 *
 */

#ifndef TELUX_TEL_IMSSETTINGSMANAGER_HPP
#define TELUX_TEL_IMSSETTINGSMANAGER_HPP

#include <memory>
#include <bitset>

#include <telux/common/SDKListener.hpp>
#include <telux/common/CommonDefines.hpp>

namespace telux {
namespace tel {

// Forward declaration
class IImsSettingsListener;

/** @addtogroup telematics_ims_settings
 * @{ */

/**
 * Defines the IMS service configuration parameters
 */
enum ImsServiceConfigType {
    IMSSETTINGS_VOIMS = 1,       /**< Voice calling support on LTE */
    IMSSETTINGS_IMS_SERVICE = 2, /**< IMS Normal Registration configuration */
    IMSSETTINGS_SMS = 3,         /**< SMS support on IMS */
    IMSSETTINGS_RTT = 4,         /**< RTT support on IMS */

};

/**
 * 32 bit mask that denotes which of the IMS settings configuration parameters
 * defined in ImsServiceConfigType enum are configured currently.
 * For example, if the client selects the VOIMS configuration, 1st bit position is set.
 */
using ImsServiceConfigValidity = std::bitset<32>;

/**
 * Defines the selected IMS service configuration parameters and their corresponding value
 */
struct ImsServiceConfig {
    ImsServiceConfigValidity configValidityMask;   /**< Indicates the configurations type. Bit set to 1 denotes the config is valid. Example: configValidityMask = 0x01 denotes ImsServiceConfig::voImsEnabled has a valid value */
    bool voImsEnabled;                             /**< Enable/Disable VOIMS service */
    bool imsServiceEnabled;                        /**< Enable/Disable IMS service */
    bool smsEnabled;                               /**< Enable/Disable SMS service */
    bool rttEnabled;                               /**< Enable/Disable RTT service */
};

/**
 * This function is called in the response to requestServiceConfig API.
 *
 * The callback can be invoked from multiple different threads.
 * The implementation should be thread safe.
 *
 * @param [in] slotId         Slot for which the IMS service configuration is intended.
 * @param [in] config         Indicates which configuration is valid and whether the configuration
 *                            is enabled or disabled. @ref ImsServiceConfig.
 * @param [in] error          Return code which indicates whether the operation
 *                            succeeded or not @ref telux::common::ErrorCode.
 *
 */
using ImsServiceConfigCb
   = std::function<void(SlotId slotId, ImsServiceConfig config, telux::common::ErrorCode error)>;

/**
 * This function is called in the response to requestSipUserAgent API.
 *
 * The callback can be invoked from multiple different threads.
 * The implementation should be thread safe.
 *
 * @param [in] slotId         Slot for which the IMS service configuration is intended.
 * @param [in] sipUserAgent   Indicates the configured Sip UserAgent.
 * @param [in] error          Return code which indicates whether the operation
 *                            succeeded or not @ref telux::common::ErrorCode.
 *
 */
using ImsSipUserAgentConfigCb
   = std::function<void(SlotId slotId, std::string sipUserAgent, telux::common::ErrorCode error)>;

/**
 * This function is called in the response to requestVonrStatus API.
 *
 * The callback can be invoked from multiple different threads.
 * The implementation should be thread safe.
 *
 * @param [in] slotId         Logical slot for which the status of the IMS voice over NR
 *                            service is requested.
 * @param [in] isEnable       Indicates the IMS voice over NR service is enabled or
 *                            disabled.
 * @param [in] error          Return code which indicates whether the operation
 *                            succeeded or not @ref telux::common::ErrorCode.
 *
 * @note   Eval: This is a new API and is being evaluated. It is subject to
 *         change and could break backwards compatibility.
 */
using ImsVonrStatusCb
   = std::function<void(SlotId slotId, bool isEnable, telux::common::ErrorCode error)>;

/**
 * @brief      ImsSettingsManager allows IMS settings. For example enabling or disabling
 *             IMS service, VOIMS service.
 *
 */
class IImsSettingsManager {
public:
   /**
    * This status indicates whether the IImsSettingsManager object is in a usable state.
    *
    * @returns SERVICE_AVAILABLE    -  If IMS settings manager is ready for service.
    *          SERVICE_UNAVAILABLE  -  If IMS settings manager is temporarily unavailable.
    *          SERVICE_FAILED       -  If IMS settings manager encountered an irrecoverable failure.
    *
    */
   virtual telux::common::ServiceStatus getServiceStatus() = 0;

   /**
    * Request the IMS service configurations
    *
    * @param [in] slotId      Slot for which the IMS service configurations is requested.
    * @param [in] callback    Callback function to get the response of request
    *                         IMS service configurations.
    *
    * @returns Status of requestServiceConfig i.e. success or suitable error code.
    *
    */
   virtual telux::common::Status requestServiceConfig(SlotId slotId,
      ImsServiceConfigCb callback) = 0;

    /**
    * Request the IMS SIP User Agent configuration
    *
    * @param [in] slotId      Slot for which the IMS SIP User Agent configuration is requested.
    * @param [in] callback    Callback function to get the response of request
    *                         IMS SIP User Agent configurations.
    *
    * @returns Status of requestSipUserAgent i.e. success or suitable error code.
    *
    * @note   Eval: This is a new API and is being evaluated. It is subject to
    *         change and could break backwards compatibility.
    *
    */
   virtual telux::common::Status requestSipUserAgent(SlotId slotId,
      ImsSipUserAgentConfigCb callback) = 0;

    /**
    * Set the IMS SIP user agent
    *
    * On platforms with Access control enabled, Caller needs to have TELUX_TEL_IMS_SETTINGS
    * permission to invoke this API successfully.
    *
    * @param [in] slotId         Slot for which the IMS SIP User Agent configuration is intended.
    * @param [in] userAgent      Configure User Agent Client(UAC) orignating the request.
    * @param [in] callback       Callback function to get the response of set IMS SIP User Agent
    *                            configuration request.
    *
    * @returns Status of setSipUserAgent i.e. success or suitable error code.
    *
    * @note   Eval: This is a new API and is being evaluated. It is subject to
    *         change and could break backwards compatibility.
    *
    */
   virtual telux::common::Status setSipUserAgent(SlotId slotId,
      std::string userAgent, common::ResponseCallback callback = nullptr) = 0;

   /**
    * To configure the IMS service configurations.
    * Also specifiy whether configuration needs to be enabled or disabled.
    *
    * On platforms with Access control enabled, Caller needs to have TELUX_TEL_IMS_SETTINGS
    * permission to invoke this API successfully.
    *
    * @param [in] slotId         Slot for which the IMS service configuration is intended.
    * @param [in] config         Indicates which configuration are configured currently and whether
    *                            the config is enabled or disabled. @ref ImsServiceConfig.
    * @param [in] callback       Callback function to get the response of set IMS service
    *                            configuration request.
    *
    * @returns Status of setServiceConfig i.e. success or suitable error code.
    *
    */
   virtual telux::common::Status setServiceConfig(SlotId slotId,
      ImsServiceConfig config, common::ResponseCallback callback = nullptr) = 0;

   /**
    * Enable or disable IMS voice over NR service. If voice over NR is disabled, then
    * the UE will fallback to use voice over LTE if the network supports it.
    * For voice over NR/LTE, ensure that voImsEnabled in telux::tel::ImsServiceConfig
    * is turned on.
    *
    * On platforms with Access control enabled, Caller needs to have TELUX_TEL_IMS_SETTINGS
    * permission to invoke this API successfully.
    *
    * @param [in] slotId         Logical slot for toggling IMS voice over NR service.
    * @param [in] isEnable       Indicates whether the IMS voice over NR service is enabled
    *                            or disabled. If voice over NR is disabled, then IMS voice
    *                            over LTE is enabled
    * @param [in] callback       Callback function to get the response for toggling IMS voice
    *                            over NR service.
    *
    * @returns Status of toggleVonr i.e. success or suitable error code.
    *
    * @note   Eval: This is a new API and is being evaluated. It is subject to
    *         change and could break backwards compatibility.
    */
   virtual telux::common::Status toggleVonr(SlotId slotId,
      bool isEnable, common::ResponseCallback callback = nullptr) = 0;


   /**
    * Request the status of the IMS voice over NR service.
    *
    * On platforms with Access control enabled, Caller needs to have TELUX_TEL_IMS_SETTINGS
    * permission to invoke this API successfully.
    *
    * @param [in] slotId      Logical slot for which the IMS voice over NR service is requested.
    * @param [in] callback    Callback function to get the response of request IMS
    *                         voice over NR service.
    *
    * @returns Status of requestVonrStatus i.e. success or suitable error code.
    *
    * @note   Eval: This is a new API and is being evaluated. It is subject to
    *         change and could break backwards compatibility.
    *
    */
   virtual telux::common::Status requestVonrStatus(SlotId slotId, ImsVonrStatusCb callback) = 0;

   /**
    * Register a listener for specific events in the IMS settings subsystem.
    *
    * @param [in] listener  Pointer to IImsSettingsListener object that processes the
    *                       notification
    *
    * @returns Status of registerListener i.e. success or suitable error code.
    *
    */
   virtual telux::common::Status registerListener(std::weak_ptr<IImsSettingsListener> listener) = 0;

   /**
    * Deregister the previously added listener.
    *
    * @param [in] listener    Pointer to IImsSettingsListener object that needs to be
    *                         deregistered.
    *
    * @returns Status of deregisterListener i.e. success or suitable error code.
    *
    */
   virtual telux::common::Status deregisterListener(std::weak_ptr<IImsSettingsListener> listener)
       = 0;

   virtual ~IImsSettingsManager(){};
};

/**
 * @brief Listener class for getting IMS service configuration change notifications.
 *        The listener method can be invoked from multiple different threads.
 *        Client needs to make sure that implementation is thread-safe.
 */
class IImsSettingsListener : public telux::common::ISDKListener {
public:

   /**
    * This function is called whenever any IMS service configuration is changed.
    *
    * @param [in] slotId        SIM corresponding to slot identifier for which the IMS service
    *                           configuration has changed.
    * @param [in] config        Indicates which configuration is valid and whether the config
    *                           is enabled or disabled. @ref ImsServiceConfig.
    *
    */
   virtual void onImsServiceConfigsChange(SlotId slotId, ImsServiceConfig config) {}

   /**
    * This function is called whenever any IMS SIP user agent is changed.
    *
    * @param [in] slotId        SIM corresponding to slot identifier for which the IMS SIP user
    *                           agent is changed.
    * @param [in] sipUserAgent  Indicates the configured SIP User Agent.
    *
    * @note   Eval: This is a new API and is being evaluated. It is subject to
    *         change and could break backwards compatibility.
    *
    */
   virtual void onImsSipUserAgentChange(SlotId slotId, std::string sipUserAgent) {}

   /**
    * This function is called when IImsSettingsManager service status changes.
    *
    * @param [in] status - @ref telux::common::ServiceStatus
    *
    */
   virtual void onServiceStatusChange(telux::common::ServiceStatus status) {}

   /**
    * Destructor of IImsSettingsListener
    */
   virtual ~IImsSettingsListener() {
   }
};

/** @} */ /* end_addtogroup telematics_ims_settings */
}
}

#endif // TELUX_TEL_IMSSETTINGSMANAGER_HPP
