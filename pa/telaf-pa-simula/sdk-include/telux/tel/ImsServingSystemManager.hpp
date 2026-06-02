/*
 *  Copyright (c) 2021 The Linux Foundation. All rights reserved.
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
 * @file  ImsServingSystemManager.hpp
 * @brief IMS Serving System Manager is the primary interface for performing
 *        IMS related operations. Allows to query IMS registration status.
 *        The IMS registration status change can be notified via the registered
 *        listeners.
 */

#ifndef TELUX_TEL_IMSSERVINGSYSTEMMANAGER_HPP
#define TELUX_TEL_IMSSERVINGSYSTEMMANAGER_HPP

#include <memory>
#include <string>

#include <telux/common/CommonDefines.hpp>
#include <telux/common/ConnectivityDefines.hpp>
#include <telux/tel/PhoneDefines.hpp>

namespace telux {

namespace tel {

class IImsServingSystemListener;
/**
 * Defines the IMS registration status parameters
 */
enum class RegistrationStatus {
    UNKOWN_STATE = -1,      /**< Unknown status for IMS */
    NOT_REGISTERED = 0,     /**< Not registered status for IMS */
    REGISTERING = 1,        /**< Registering status for IMS */
    REGISTERED = 2,         /**< Registered status for IMS */
    LIMITED_REGISTERED = 3, /**< Limited registration status for IMS */
};

/**
 * Defines the IMS registration status parameters and the error code value
 */
struct ImsRegistrationInfo {
    RegistrationStatus imsRegStatus; /**< The status of the IMS registration with the network */
    RadioTechnology rat; /**< The RAT is returned when IMS registration is being attempted or is successful */
    /** An error code is returned when the IMS registration status is RegistrationStatus::NOT_REGISTERED. Values(Defined in SIP-RFC3261 section 13.2.2.2 and section 13.2.2.3): \n
                                  - 3xx - Redirection responses
                                  - 4xx - Client failure responses
                                  - 5xx - Server failure responses
                                  - 6xx - Global failure responses
                                */
    int errorCode;
    std::string errorString; /**< Registration failure error string when the IMS is not registered. */
};

/**
 * Defines the cellular service status parameters.
 */
enum class CellularServiceStatus {
    UNKNOWN = -1,             /**< Unknown service status */
    NO_SERVICE = 0,           /**< Unavailable service status */
    LIMITED_SERVICE = 1,      /**< Emergency service status */
    FULL_SERVICE = 2,         /**< Available service status */
};

/**
 * Represents the status for supporting various services over IMS.
 */
struct ImsServiceInfo {
    CellularServiceStatus sms;    /**< SMS service status over IMS */
    CellularServiceStatus voice;  /**< Voice service status over IMS */
};

/**
 * Defines the cellular PDP failure error code.
 */
enum class PdpFailureCode {
    OTHER_FAILURE = 0,                 /**< Generic failure reason. */
    OPTION_UNSUBSCRIBED = 1,           /**< Option is unsubscribed. */
    UNKNOWN_PDP = 2,                   /**< PDP was unknown. */
    REASON_NOT_SPECIFIED = 3,          /**< Reason not specified. */
    CONNECTION_BRINGUP_FAILURE = 4,    /**< Connection bring-up failure. */
    CONNECTION_IKE_AUTH_FAILURE = 5,   /**< IKE authentication failure. */
    USER_AUTH_FAILED = 6,              /**< User authentication failed. */
};

/**
 * Represents the IMS PDP status information.
 */
struct ImsPdpStatusInfo {
    bool isPdpConnected;             /**< PDP connection status. */
    PdpFailureCode failureCode;      /**< PDP failure code. */
    telux::common::DataCallEndReason failureReason; /**< PDP call end reason type and its cause. */
    std::string apnName;             /**< IMS registration APN name. */
};

/**
 * This function is called in the response to requestRegistrationInfo API.
 *
 * The callback can be invoked from multiple different threads.
 * The implementation should be thread safe.
 *
 * @param [in] status         Indicates the IMS registration status and the error code
 *                            @ref telux::tel::ImsRegistrationInfo.
 * @param [in] error          Return code which indicates whether the operation
 *                            succeeded or not @ref telux::common::ErrorCode.
 *
 */
using ImsRegistrationInfoCb
   = std::function<void(ImsRegistrationInfo status, telux::common::ErrorCode error)>;

/**
 * This function is called in response to the requestServiceInfo API.
 *
 * The callback can be invoked from multiple different threads.
 * The implementation should be thread safe.
 *
 * @param [in] service        Indicates the IMS service information
 *                            @ref telux::tel::ImsServiceInfo.
 * @param [in] error          Return code which indicates whether the operation
 *                            succeeded or not @ref telux::common::ErrorCode.
 *
 */
using ImsServiceInfoCb
   = std::function<void(ImsServiceInfo service, telux::common::ErrorCode error)>;

/**
 * This function is called in response to the requestPdpStatus API.
 *
 * The callback can be invoked from multiple different threads.
 * The implementation should be thread safe.
 *
 * @param [in] status         Indicates the IMS PDP status information.
 *                            @ref telux::tel::ImsPdpStatusInfo
 * @param [in] error          Return code which indicates whether the operation
 *                            succeeded or not. @ref telux::common::ErrorCode
 *
 * @note Eval: This is a new API and is being evaluated. It is subject to change and
 *             could break backwards compatibility.
 */
using ImsPdpStatusInfoCb
   = std::function<void(ImsPdpStatusInfo status, telux::common::ErrorCode error)>;

/**
 * @brief IMS Serving System Manager is the primary interface for IMS related operations
 *        Allows to query IMS registration status.
 */
class IImsServingSystemManager {
public:
   /**
    * This status indicates whether the IImsServingSystemManager object is in a usable state.
    *
    * @returns SERVICE_AVAILABLE    -  If IMS Serving System manager is ready for service.
    *          SERVICE_UNAVAILABLE  -  If IMS Serving System manager is temporarily unavailable.
    *          SERVICE_FAILED       -  If IMS Serving System manager encountered an irrecoverable
    *                                  failure.
    */
   virtual telux::common::ServiceStatus getServiceStatus() = 0;


   /**
    * Request IMS registration information.
    *
    * @param [in] callback     Callback pointer to get the response of
    *                          requestRegistrationInfo.
    *
    * @returns Status of requestRegistrationInfo i.e. success or suitable status code.
    *
    */
    virtual telux::common::Status
        requestRegistrationInfo(ImsRegistrationInfoCb callback) = 0;

   /**
    * Request IMS service information, such as SMS and voice service status over IMS.
    *
    * @param [in] callback     Callback pointer to get the response of
    *                          requestServiceInfo.
    *
    * @returns Status of requestServiceInfo i.e., success or suitable status code.
    *
    */
    virtual telux::common::Status
        requestServiceInfo(ImsServiceInfoCb callback) = 0;

   /**
    * Request PDP status information, such as PDP connection status, failure cause
    * and error code for IMS PDP failure.
    *
    * @param [in] callback     Callback pointer to get the response of
    *                          requestPdpStatus.
    *
    * @returns Status of requestPdpStatus i.e., success or suitable status code.
    *
    * @note Eval: This is a new API and is being evaluated. It is subject to change and
    *             could break backwards compatibility.
    */
    virtual telux::common::Status requestPdpStatus(ImsPdpStatusInfoCb callback) = 0;

   /**
    * Add a listener to listen for specific events in the IMS Serving System subsystem.
    *
    * @param [in] listener  Pointer to IImsServingSystemListener object that processes the
    *                       notification
    *
    * @returns Status of registerListener i.e. success or suitable error code.
    *
    */
   virtual telux::common::Status
      registerListener(std::weak_ptr<telux::tel::IImsServingSystemListener> listener) = 0;

   /**
    * Remove a previously added listener.
    *
    * @param [in] listener  Listener to be removed.
    *
    * @returns Status of deregisterListener i.e. success or suitable error code.
    *
    */
   virtual telux::common::Status
       deregisterListener(std::weak_ptr<telux::tel::IImsServingSystemListener> listener) = 0;

   virtual ~IImsServingSystemManager(){};
};

/**
 * @brief A listener class for monitoring changes in IMS Serving System manager,
 * including IMS registration status change.
 * Override the methods for the state that you wish to receive updates for.
 *
 * The methods in listener can be invoked from multiple different threads. The implementation
 * should be thread safe.
 */
class IImsServingSystemListener : public common::IServiceStatusListener {
public:
    /**
     * This function is called when service status changes.
     *
     * @param [in] status - @ref telux::common::ServiceStatus
     *
     */
    virtual void onServiceStatusChange(telux::common::ServiceStatus status) {
    }

   /**
    * This function is called whenever any IMS service configuration is changed.
    *
    * @param [in] status        Indicates which registration status is the IMS service
    *                           changed to. @ref telux::tel::ImsRegistrationInfo.
    *
    */
    virtual void onImsRegStatusChange(ImsRegistrationInfo status) {
    }

   /**
    * This function is called whenever any IMS service information is changed.
    *
    * @param [in] service        Indicates which IMS service information has changed.
    *                            @ref telux::tel::ImsServiceInfo.
    *
    */
    virtual void onImsServiceInfoChange(ImsServiceInfo service) {
    }

    /**
    * This function is called whenever any IMS PDP status information is changed.
    *
    * @param [in] status         Indicates which IMS PDP information has changed.
    *                            @ref telux::tel::ImsPdpStatusInfo.
    *
    * @note Eval: This is a new API and is being evaluated. It is subject to change and
    *             could break backwards compatibility.
    */
    virtual void onImsPdpStatusInfoChange(ImsPdpStatusInfo status) {
    }

    virtual ~IImsServingSystemListener() {
    }
};
}  // End of namespace tel

}  // End of namespace telux

#endif // TELUX_TEL_IMSSERVINGSYSTEMMANAGER_HPP
