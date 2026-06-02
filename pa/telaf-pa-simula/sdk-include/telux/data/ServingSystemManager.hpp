/*
 *  Copyright (c) 2020-2021, The Linux Foundation. All rights reserved.
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
 * @file       data/ServingSystemManager.hpp
 *
 * @brief      Serving System Manager class provides the interface to access network and modem
 *             low level services.
 */

#ifndef TELUX_DATA_SERVINGSYSTEMMANAGER_HPP
#define TELUX_DATA_SERVINGSYSTEMMANAGER_HPP

#include <future>
#include <memory>

#include <telux/common/SDKListener.hpp>
#include <telux/data/DataDefines.hpp>
#include <telux/common/CommonDefines.hpp>

namespace telux {
namespace data {

/** @addtogroup telematics_data
 * @{ */

//Forward Declaration
class IServingSystemListener;

/**
 * @brief Dedicated Radio Bearer (DRB) status.
 */
enum class DrbStatus {
    ACTIVE  ,   /**< At least one of the physical links across all PDNs is UP */
    DORMANT ,   /**< All the Physlinks across all PDNs are DOWN */
    UNKNOWN ,   /**< No PDN is active */
};

/**
 * @brief Roaming Type.
 */
enum class RoamingType {
    UNKNOWN       ,      /**< Device roaming mode is unknown */
    DOMESTIC      ,      /**< Device is in Domestic roaming network            */
    INTERNATIONAL ,      /**< Device is in International roaming network       */
};

/**
 * @brief Roaming Status
 */
struct RoamingStatus {
   bool isRoaming;          /**< True: Roaming on, False: Roaming off                 */
   RoamingType type;        /**< International/Domestic. Valid only if roaming is on  */
};

/**
 * @brief Data Service State. Indicates whether data service is ready to setup a data call or not.
 */
enum class DataServiceState {
    UNKNOWN        ,        /**< Service State not available */
    IN_SERVICE     ,        /**< Service Available           */
    OUT_OF_SERVICE ,        /**< Service Not Available       */
};

/**
 * @brief Data Network RATs.
 */
enum class NetworkRat {
    UNKNOWN    ,    /**< UNKNOWN   */
    CDMA_1X    ,    /**< CDMA_1X   */
    CDMA_EVDO  ,    /**< CDMA_EVDO */
    GSM        ,    /**< GSM       */
    WCDMA      ,    /**< WCDMA     */
    LTE        ,    /**< LTE       */
    TDSCDMA    ,    /**< TDSCDMA   */
    NR5G       ,    /**< NR5G      */
};

/**
 * @brief Data Service Status Info.
 */
struct ServiceStatus {
    DataServiceState serviceState;
    NetworkRat networkRat;
};

/**
 * @brief NR icon type.
 */
enum class NrIconType {
    NONE  ,      /**< Unspecified       */
    BASIC ,      /**< 5G basic         */
    UWB   ,      /**< 5G ultrawide band */
};

/**
 * @brief LTE attach failure information.
 */
struct LteAttachFailureInfo {
    std::vector<uint8_t> plmnId;
    /**< PLMN ID that was rejected during the attach */
    DataCallEndReason rejectReason;

    std::vector<uint8_t> primaryPlmnId;
    /**< Primary PLMN for the shared network */
};

/**
 * This function is called in response to requestServiceStatus API.
 *
 * The callback can be invoked from multiple different threads.
 * The implementation should be thread safe.
 *
 * @param [in] serviceStatus       Current service status @ref telux::data::ServiceStatus
 * @param [in] error               Return code for whether the operation succeeded or failed.
 *
 */
using RequestServiceStatusResponseCb
    = std::function<void(ServiceStatus serviceStatus, telux::common::ErrorCode error)>;

/**
 * This function is called in response to requestRoamingStatus API.
 *
 * The callback can be invoked from multiple different threads.
 * The implementation should be thread safe.
 *
 * @param [in] roamingStatus       Current roaming status @ref telux::data::RoamingStatus
 * @param [in] error               Return code for whether the operation succeeded or failed.
 *
*/
using RequestRoamingStatusResponseCb
    = std::function<void(RoamingStatus roamingStatus, telux::common::ErrorCode error)>;

/**
 * This function is called in response to RequestNrIconType API.
 *
 * The callback can be invoked from multiple different threads.
 * The implementation should be thread safe.
 *
 * @param [in] type                Current NR icon type @ref telux::data::NrIconType
 * @param [in] error               Return code for whether the operation succeeded or failed.
 *
*/
using RequestNrIconTypeResponseCb
    = std::function<void(NrIconType type, telux::common::ErrorCode error)>;

/**
 * @brief Serving System Manager class provides APIs related to the serving system for data
 *        functionality. For example, ability to query or be notified about the state of
 *        the platform's WWAN PS data serving information
 */
class IServingSystemManager {
public:
    /**
     * Checks the status of serving manager object and returns the result.
     *
     * @returns SERVICE_AVAILABLE    -  If serving manager object is ready for service.
     *          SERVICE_UNAVAILABLE  -  If serving manager object is temporarily unavailable.
     *          SERVICE_FAILED       -  If serving manager object encountered an irrecoverable
     *                                  failure.
     *
     */
    virtual telux::common::ServiceStatus getServiceStatus() = 0;

    /**
     * Get the dedicated radio bearer (DRB) status
     *
     * @returns current DrbStatus @ref DrbStatus.
     *
     */
    virtual DrbStatus getDrbStatus() = 0;

    /**
     * Queries the current serving network status
     *
     * @param [in] callback          callback to get response for requestServiceStatus
     *
     * @returns Status of requestServiceStatus i.e. success or suitable status code.
     *          if requestServiceStatus returns failure, callback will not be invoked.
     */
    virtual telux::common::Status requestServiceStatus(RequestServiceStatusResponseCb callback) = 0;

    /**
     * Queries the current roaming status
     *
     * @param [in] callback          callback to get response for requestRoamingStatus
     *
     * @returns Status of requestRoamingStatus i.e. success or suitable status code.
     *
     */
    virtual telux::common::Status requestRoamingStatus(RequestRoamingStatusResponseCb callback) = 0;

    /**
     * Queries the NR icon type to be displayed based on the serving system that the
     * device has acquired service on.
     *
     * @param [in] callback         callback to get response for requestNrIconType
     *
     * @returns Status of requestNrIconType i.e. success or suitable status code.
     *
     */
    virtual telux::common::Status requestNrIconType(RequestNrIconTypeResponseCb callback) = 0;

    /**
     * Request modem switch to dormant state: Certain network operations can only be performed when
     * modem is in dormant state. This API provides an ability for clients to request modem to
     * immediately transition to dormant state for such scenarios.
     *
     * Clients must ensure no data calls are in process of bring up/tear down and there is no
     * traffic on any active data calls when this API is called.
     *
     * @param [in] callback              optional callback to get the response of makeDormancy
     *
     * @returns
     * telux::common::ErrorCode::SUCCESS if request is honored by network.
     * telux::common::ErrorCode::INVALID_STATE is returned if:
     *  - There is no active data calls
     *  - Any Data calls is going through bring up/tear down
     *  - There is data traffic on any active data calls
     * If API fails, application is responsible for re-attempting operation at later time once the
     * above conditions are met.
     *
     * On platforms with Access control enabled, Caller needs to have TELUX_DATA_SERVICE_MGMT
     * permission to invoke this API successfully.
     *
     */
    virtual telux::common::Status makeDormant(
        telux::common::ResponseCallback callback = nullptr) = 0;

   /**
    * Register a listener for specific updates from serving system.
    *
    * @param [in] listener     Pointer of IServingSystemListener object that
    *                          processes the notification
    *
    * @returns Status of registerListener i.e success or suitable status code.
    */
   virtual telux::common::Status registerListener(std::weak_ptr<IServingSystemListener> listener)
      = 0;

   /**
    * Deregister the previously added listener.
    *
    * @param [in] listener     Previously registered IServingSystemListener that
    *                          needs to be removed
    *
    * @returns Status of removeListener i.e. success or suitable status code
    */
   virtual telux::common::Status deregisterListener(std::weak_ptr<IServingSystemListener> listener)
      = 0;

   /**
    * Get associated slot id for the Serving System Manager.
    *
    * @returns SlotId
    *
    *
    */
   virtual SlotId getSlotId() = 0;

   /**
    * Destructor of IServingSystemManager
    */
   virtual ~IServingSystemManager() {};
};

/**
 * @brief Listener class for data serving system change notification.
 *
 * The listener method can be invoked from multiple different threads.
 * Client needs to make sure that implementation is thread-safe.
 * Note: Some APIs of this listener support an auto-suppress feature, where the invocation of the
 * API will be suppressed, to prevent unnecessary wakeups and save power, when the system is in a
 * suspended state, enabling the auto suppress feature is controlled using a platform configuration
 * in tel.conf. If the platform is configured to suppress an API, that API will not be invoked
 * during suspend. In this case, if a state change or event occurs in the modem, the client will
 * not know about it via a listener indication. If the client is interested, it can get the latest
 * state explicitly on resume.
 */
class IServingSystemListener : public telux::common::ISDKListener {
public:
    /**
     * This function is called when telux::common::ServiceStatus status changes.
     * telux::common::ServiceStatus indicate whether this sub system ready to provide service.
     *
     * @param [in] status - @ref ServiceStatus
     */
    virtual void onServiceStatusChange(telux::common::ServiceStatus status) {};

   /**
    * This function is called whenever Drb status is changed.
    *
    * @param [in] status      @ref DrbStatus
    *
    * This API supports the auto-suppress feature.
    */
   virtual void onDrbStatusChanged(DrbStatus status) {};

   /**
    * This function is called whenever telux::data:ServiceStatus state is changed.
    * telux::data:ServiceStatus indicate packet switch domain network status.
    *
    * @param [in] status      @ref ServiceStatus
    *
    * This API supports the auto-suppress feature.
    */
   virtual void onServiceStateChanged(ServiceStatus status) {};

   /**
    * This function is called whenever roaming status is changed.
    *
    * @param [in] status      @ref RoamingStatus
    *
    * This API supports the auto-suppress feature.
    */
   virtual void onRoamingStatusChanged(RoamingStatus status) {};

   /**
    * This function is called whenever NR icon type is changed.
    *
    * @param [in] type      @ref NrIconType
    *
    * This API supports the auto-suppress feature.
    */
   virtual void onNrIconTypeChanged(NrIconType type) {};

   /**
    * This function is called whenever LTE attach failed.
    *
    * @param [in] type      @ref LteAttachFailureInfo
    *
    * This API supports the auto-suppress feature.
    */
   virtual void onLteAttachFailure(const telux::data::LteAttachFailureInfo info) {};

   /**
    * Destructor of IServingSystemListener
    */
   virtual ~IServingSystemListener() {};
};

/** @} */ /* end_addtogroup telematics_data */
}
}

#endif // TELUX_DATA_SERVINGSYSTEMMANAGER_HPP
