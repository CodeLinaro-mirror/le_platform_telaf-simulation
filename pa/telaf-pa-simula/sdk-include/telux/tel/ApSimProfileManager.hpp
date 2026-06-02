/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file       ApSimProfileManager.hpp
 *
 * @brief      ApSimProfileManager is the primary interface to allow the modem software to interact
 *             with a Local Profile Assistant (LPA) running on the application processor (AP).
 *             The LPA can use the provided APIs to handle requests from the modem for operations
 *             such as retrieving profile details and enabling or disabling profile on the eUICC.
 *
 */

#ifndef TELUX_TEL_APSIMPROFILEMANAGER_HPP
#define TELUX_TEL_APSIMPROFILEMANAGER_HPP

#include <memory>
#include <string>
#include <vector>

#include <telux/common/CommonDefines.hpp>

namespace telux {
namespace tel {

// Forward declaration
class IApSimProfileListener;

/** @addtogroup telematics_rsp
 * @{ */

/**
 * Indicates Application Protocol Data Unit (APDU) exchange status.
 */
enum class ApduExchangeStatus {
    SUCCESS = 0,      /**< APDU exchange is success */
    FAILURE,          /**< APDU exchange is failed */
};

/**
 * @brief IApSimProfileManager is the primary interface that enables the modem to interact with
 * a Local Profile Assistant (LPA) running on the application processor (AP). The modem initiates
 * profile-related operations such as retrieving profiles and enabling or disabling profile. When
 * an AP-based LPA is enabled, LPA on AP will respond to the modem’s notifications regarding these
 * profile-related operation request. The LPA on AP can use the APIs in this class to handle
 * requests from the modem for various SIM profile operations, including:
 *
 * 1. Retrieving profile details
 * 2. Enabling profile on the eUICC
 * 3. Disabling profile on the eUICC
 *
 */
class IApSimProfileManager {
 public:

    /**
     * This status indicates whether the IApSimProfileManager object is in a usable state.
     *
     * @returns SERVICE_AVAILABLE    - If ApSimProfile Manager is ready for service.
     *          SERVICE_UNAVAILABLE  - If ApSimProfile Manager is temporarily unavailable.
     *          SERVICE_FAILED       - If ApSimProfile Manager encountered an irrecoverable
     *                                 failure.
     *
     * @note   Eval: This is a new API and is being evaluated. It is subject to change and
     *         could break backwards compatibility.
     *
     */
    virtual telux::common::ServiceStatus getServiceStatus() = 0;

    /**
     * Sends the list of ICCIDs for the profiles requested by the modem. This API should be
     * called in response to notification @ref telux::tel::IApSimProfileListener::
     * onRetrieveProfileListRequest received to retrieve profiles information request.
     * The LPA on the AP retrieves the list of ICCIDs for the profiles by exchanging the APDUs
     * with the card using logical channel @ref telux::tel::ICard::transmitApduLogicalChannel
     * and sends them back to the modem.
     *
     * On platforms with access control enabled, the caller needs to have TELUX_TEL_SIM_PROFILE_OPS
     * permission to invoke this API successfully.
     *
     * @param [in] slotId           Logical slot identifier corresponding to the card.
     * @param [in] result           Status indicating whether the LPA on AP was able to service the
     *                              request from the modem to retrieve the ICCIDs.
     * @param [in] referenceId      Serves as a token, the LPA on the AP must pass the same
     *                              reference ID provided in the telux::tel::IApSimProfileListener
     *                              ::onRetrieveProfileListRequest notification. This identifies
     *                              the specific notification request to which the profile
     *                              operation response pertains.
     * @param [in] profileIccIds    List of ICCIDs for the profiles.
     * @param [in] callback         Optional callback function to get the result of profile list
     *                              response send to modem.
     *
     * @returns  Status of sendRetrieveProfileListResponse i.e. success or suitable error code.
     *
     * @note   Eval: This is a new API and is being evaluated. It is subject to change and
     *         could break backwards compatibility.
     */
    virtual telux::common::Status sendRetrieveProfileListResponse(SlotId slotId,
        ApduExchangeStatus result, uint32_t referenceId, std::vector<std::string> profileIccIds,
        common::ResponseCallback callback = nullptr) = 0;

    /**
     * Sends a response to the modem request received for enabling or disabling the profile.
     * This API should be called in response to notification @ref telux::tel::
     * IApSimProfileListener::onProfileOperationRequest received to perform operation on the
     * the profile. The LPA on the AP enables or disables profile by exchanging APDUs with the
     * card using logical channel @ref telux::tel::ICard::transmitApduLogicalChannel
     *
     * On platforms with access control enabled, the caller needs to have TELUX_TEL_SIM_PROFILE_OPS
     * permission to invoke this API successfully.
     *
     * @param [in] slotId           Logical slot identifier corresponding to the card.
     * @param [in] result           Status indicating whether the LPA on AP was able to service the
     *                              request from the modem to enable or disable the profile.
     * @param [in] referenceId      Serves as a token, the LPA on the AP must pass the same
     *                              reference ID provided in the telux::tel::IApSimProfileListener
     *                              ::onProfileOperationRequest notification. This identifies the
     *                              specific notification request to which the profile operation
     *                              response pertains.
     * @param [in] callback         Optional callback function to get the result of profile
     *                              operation response send to the modem.
     *
     * @returns Status of sendProfileOperationResponse i.e. success or suitable error code.
     *
     * @note   Eval: This is a new API and is being evaluated. It is subject to change and
     *         could break backwards compatibility.
     */
    virtual telux::common::Status sendProfileOperationResponse(SlotId slotId,
        ApduExchangeStatus result, uint32_t referenceId, common::ResponseCallback callback
        = nullptr) = 0;

    /**
     * Register a listener to listen for requests to retrieve profile list, enable or disable
     * profile on eUICC triggered by the modem.
     *
     * On platforms with access control enabled, the caller needs to have TELUX_TEL_SIM_PROFILE_OPS
     * permission to invoke this API successfully.
     *
     * @param [in] listener    Pointer of IApSimProfileListener object that processes the
     *                         notification.
     *
     * @returns Status of registerListener success or suitable status code.
     *
     * @note   Eval: This is a new API and is being evaluated. It is subject to change and
     *         could break backwards compatibility.
     */
    virtual telux::common::Status registerListener(std::weak_ptr<IApSimProfileListener> listener)
        = 0;

    /**
     * De-register the listener.
     *
     * On platforms with access control enabled, the caller needs to have TELUX_TEL_SIM_PROFILE_OPS
     * permission to invoke this API successfully.
     *
     * @param [in] listener    Pointer of IApSimProfileListener object that needs to be removed.
     *
     * @returns Status of deregisterListener success or suitable status code.
     *
     * @note   Eval: This is a new API and is being evaluated. It is subject to change and
     *         could break backwards compatibility.
     */
    virtual telux::common::Status deregisterListener(std::weak_ptr<IApSimProfileListener> listener)
        = 0;

    /**
     * Destructor for IApSimProfileManager
     */
    virtual ~IApSimProfileManager() {
    }
};  // end of IApSimProfileManager

/**
 * @brief Listener class that receives requests for profile-related operations from the modem. When
 *        one of the listener’s APIs is invoked, the LPA on the AP is expected to perform the
 *        operation by exchanging the APDUs with the card.
 *
 *        The listener method can be invoked from multiple different threads.
 *        Client needs to make sure that implementation is thread-safe.
 */
class IApSimProfileListener : public telux::common::IServiceStatusListener {
public:

    /**
     * This function is called when available profiles information is requested by the modem.
     * @note  AP has to respond within the timer(30 seconds) expires for the profile switch.
     *
     * Below are the sequence of steps to be followed.
     * 1.Receive Notification: The LPA on AP receives the onRetrieveProfileListRequest
     * notification.
     * 2.Fetch ICCIDs: After receiving the request from the modem, the LPA on the AP retrieves
     * the list of ICCIDs for the profiles by exchanging the APDUs with the card using logical
     * channel. Follow the sequence below for exchanging the APDUs.
     * 2.1 Open the logical channel by providing application identifier(AID) @ref telux::tel::
     *     ICard::openLogicalChannel. To retrieve AID, @ref telux::tel::ICard::getApplications(),
     *     this will return card applications, from card application get the AID @ref
     *     telux::tel::ICardApp::getAppId()
     * 2.2 Exchange the APDUs @ref telux::tel::ICard::transmitApduLogicalChannel
     * 2.3 Close the channel once APDU exchange is complete @ref telux::tel::ICard::
     *     closeLogicalChannel
     * 3.Send Response: Upon receiving a successful result for the APDUs exchange, the LPA on the
     * AP sends a response to the modem using @ref telux::tel::IApSimProfileManager::
     * sendRetrieveProfileListResponse
     * 4.Acknowledge Response: The modem should acknowledge the LPA's response by sending
     * the result (status of sendRetrieveProfileListResponse i.e. success or suitable error code)
     * back to the LPA on the AP in a callback.
     *
     * @param [in] slotId              Logical slot on which profiles need to be requested.
     * @param [in] referenceId         Acts as a token, and the LPA on the AP needs to pass the
     *                                 same reference id in a subsequent profiles list response.
     *                                 @ref telux::tel::IApSimProfileManager::
     *                                 sendRetrieveProfileListResponse
     *
     * @note   Eval: This is a new API and is being evaluated. It is subject to change and
     *         could break backwards compatibility.
     */
    virtual void onRetrieveProfileListRequest(SlotId slotId, uint32_t referenceId) {
    }

    /**
     * This function is called when profile needs to be enabled or disabled on the card based
     * on the ICCID. The LPA on the AP is expected to provide same reference identifier when
     * sending profile operation response using @ref telux::tel::IApSimProfileManager::
     * sendProfileOperationResponse
     *
     * @note  AP has to respond within the timer(30 seconds) expires for the profile switch.
     *
     * Below are the sequence of steps to be followed.
     * 1.Receive Notification: The LPA on AP receives the onProfileOperationRequest notification.
     * 2.Enable/disable profile: After receiving the request from the modem, the LPA on the AP
     * enables or disables profile by exchanging APDUs with the card using logical channel.
     * Follow the sequence below for exchanging the APDUs.
     * 2.1 Open the logical channel by providing application identifier(AID) @ref telux::tel::
     *     ICard::openLogicalChannel. To retrieve AID, @ref telux::tel::ICard::getApplications(),
     *     this will return card applications, from card application get the AID @ref
     *     telux::tel::ICardApp::getAppId()
     * 2.2 Exchange the APDUs @ref telux::tel::ICard::transmitApduLogicalChannel
     * 2.3 Close the channel once APDU exchange is complete @ref telux::tel::ICard::
     *     closeLogicalChannel
     * 3.Send Response: Upon receiving a successful result for the APDUs exchange, the LPA on the AP
     * sends a response to the modem using @ref telux::tel::IApSimProfileManager::
     * sendProfileOperationResponse
     * 4.Acknowledge Response: The modem should acknowledge the LPA’s response by sending
     * the result (status of sendProfileOperationResponse i.e. success or suitable error code) back
     * to the LPA on the AP in a callback.
     *
     * @param [in] slotId              Logical slot on which profile to be modified.
     * @param [in] referenceId         Acts as a token, and the LPA on the AP needs to pass the
     *                                 same reference id in a subsequent profile operation
     *                                 response @ref telux::tel::IApSimProfileManager::
                                       sendProfileOperationResponse
     * @param [in] iccid               ICCID for the profile to enable or disable.
     * @param [in] isEnable            Indicates whether the profile should be enabled or disabled.
     *                                 true - Enable and false - Disable.
     *
     * @note   Eval: This is a new API and is being evaluated. It is subject to change and
     *         could break backwards compatibility.
     */
    virtual void onProfileOperationRequest(SlotId slotId, uint32_t referenceId,
        std::string iccid, bool isEnable) {
    }

    /**
     * Destructor of IApSimProfileListener
     */
    virtual ~IApSimProfileListener() {
    }
};

/** @} */ /* end_addtogroup telematics_rsp */
} // End of namespace tel
} // End of namespace telux

#endif // TELUX_TEL_APSIMPROFILEMANAGER_HPP
