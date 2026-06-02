/*
 *  Copyright (c) 2017-2021 The Linux Foundation. All rights reserved.
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
 * @file       CardManager.hpp
 * @brief      Card Manager is a primary interface that is aware of all the UICC cards on a device.
 *             It provide APIs to enumerate cards, retrieve number of slots, get card state.
 */

#ifndef TELUX_TEL_CARDMANAGER_HPP
#define TELUX_TEL_CARDMANAGER_HPP

#include <future>
#include <memory>
#include <string>
#include <vector>

#include "telux/tel/CardApp.hpp"
#include "telux/tel/CardDefines.hpp"
#include "telux/tel/PhoneDefines.hpp"
#include "telux/tel/CardFileHandler.hpp"
#include "telux/common/CommonDefines.hpp"

namespace telux {
namespace tel {

/** @addtogroup telematics_card
 * @{ */

// Forward declarations
class ICardChannelCallback;
class ICardCommandCallback;
class ICardListener;
class ICard;
class ICardFileHandler;

/**
 * This function is called with the response to requestEid API.
 *
 * The callback can be invoked from multiple different threads.
 * The implementation should be thread safe.
 *
 * @param [in] eid        eUICC identifier.
 * @param [in] error      Return code which indicates whether the operation
 *                        succeeded or not.  @ref ErrorCode
 */
using EidResponseCallback
    = std::function<void(const std::string &eid, telux::common::ErrorCode error)>;

/**
 * This function is called with the response to requestLastRefreshEvent API.
 *
 * The callback can be invoked from multiple different threads.
 * The implementation should be thread safe.
 *
 * @param [in] stage            Card refresh stage @ref telux::tel:RefreshStage
 * @param [in] mode             Card refresh mode @ref telux::tel:RefreshMode
 * @param [in] efFiles          List of the elementary file path and identifier
 * @param [in] refreshParams    Session type @ref telux::tel::RefreshParams.
 *                              Client provides the session type, application
 *                              identifier to listen for the corresponding refresh
 *                              event.
 * @param [in] error            Return code which indicates whether the operation
 *                              succeeded or not. @ref ErrorCode
 *
 * @note   Eval: This is a new API and is being evaluated. It is subject to
 *         change and could break backwards compatibility.
 */
using refreshLastEventResponseCallback
    = std::function<void(RefreshStage stage, RefreshMode mode, std::vector<IccFile> efFiles,
    RefreshParams refreshParams, telux::common::ErrorCode error)>;

/**
 * ICardManager provide APIs for slot count, retrieve slot ids, get card state and get card.
 */
class ICardManager {
 public:
    /**
     * Checks the status of telephony subsystems and returns the result.
     *
     * @deprecated Use ICardManager::getServiceStatus() API.
     *
     * @returns If true then CardManager is ready for service.
     */
    virtual bool isSubsystemReady() = 0;

    /**
     * Wait for telephony subsystem to be ready.
     *
     * @deprecated Use InitResponseCb in PhoneFactory::getCardManager instead,
     *             to get notified about subsystem readiness.
     * @returns A future that caller can wait on to be notified
     * when card manager is ready.
     */
    virtual std::future<bool> onSubsystemReady() = 0;

    /**
    * This status indicates whether the ICardManager object is in a usable state.
    *
    * @returns SERVICE_AVAILABLE    - If Card Manager is ready for service.
    *          SERVICE_UNAVAILABLE  - If Card Manager is temporarily unavailable.
    *          SERVICE_FAILED       - If Card Manager encountered an irrecoverable
    *                                 failure.
    *
    */
    virtual telux::common::ServiceStatus getServiceStatus() = 0;

    /**
     * Get SIM slot count.
     *
     * @param [out] count  SIM slot count.
     *
     * @returns Status of getSlotCount i.e. success or suitable status code.
     */
    virtual telux::common::Status getSlotCount(int &count) = 0;

    /**
     * Get list of SIM slots.
     *
     * @param [out] slotIds  List of SIM slot ids.
     *
     * @returns Status of getSlotIds i.e. success or suitable status code.
     */
    virtual telux::common::Status getSlotIds(std::vector<int> &slotIds) = 0;

    /**
     * Get the Card corresponding to SIM slot.
     *
     * @param [in] slotId    Slot id corresponding to the card.
     * @param [out] status   Status of getCard i.e. success or suitable status code.
     *
     * @returns Pointer to ICard object.
     */
    virtual std::shared_ptr<ICard> getCard(
        int slotId = DEFAULT_SLOT_ID, telux::common::Status *status = nullptr)
        = 0;

    /**
     * Power on the SIM card.
     *
     * On platforms with access control enabled, caller needs to have TELUX_TEL_CARD_POWER
     * permission to invoke this API successfully.
     *
     * @param [in] slotId      Slot identifier corresponding to the card which needs to be
     *                         powered up.
     * @param [in] callback    Optional callback pointer to get the result of cardPowerUp
     *
     * @returns Status of cardPowerUp i.e. success or suitable status code.
     *
     */
    virtual telux::common::Status cardPowerUp(SlotId slotId,
        telux::common::ResponseCallback callback = nullptr)
        = 0;

    /**
     * Power off the SIM card.
     * When the SIM card is powered down, the card state is absent and the SIM IO operations,
     * PIN management API's like unlock card by pin, change card pin will fail.
     *
     * On platforms with access control enabled, caller needs to have TELUX_TEL_CARD_POWER
     * permission to invoke this API successfully.
     *
     * @param [in] slotId      Slot identifier corresponding to the card which needs to be
     *                         powered down.
     * @param [in] callback    Optional callback pointer to get the result of CardPowerDown
     *
     * @returns Status of cardPowerDown i.e. success or suitable status code.
     *
     */
    virtual telux::common::Status cardPowerDown(SlotId slotId,
        telux::common::ResponseCallback callback = nullptr)
        = 0;

    /**
     * Register and deregister for refresh events from card and optionally allow client to
     * participate in voting. The client is notified to participate in voting through
     * @ref telux::tel::ICardListener::onRefreshEvent with
     * @ref telux::tel::RefreshStage::WAITING_FOR_VOTES. The client must then invoke the
     * allowCardRefresh API to permit the refresh. For the refresh procedure to continue, all
     * clients participating in the voting must allow the refresh, if any client disallows it,
     * the refresh process will fail and be communicated to the card.
     * The API also allow to register for file change notification triggered due to change in
     * EFs in the card application.
     * This API can be invoked multiple times to register with different session types, as
     * specified in @ref telux::tel::SessionType. If the API is invoked twice with the same
     * session type, the new values will overwrite the previous ones.
     *
     * On platforms with access control enabled, the caller must have the TELUX_TEL_CARD_REFRESH
     * and TELUX_TEL_CARD_REFRESH_VOTING permission to successfully invoke this API.
     *
     * @param [in] slotId           Slot identifier corresponding to the card which needs to be
     *                              refreshed.
     * @param [in] isRegister       If true register for refresh events to be received through
     *                              @ref telux::tel::ICardListener::onRefreshEvent, otherwise,
     *                              deregister for refresh events that will not be delivered.
     * @param [in] doVoting         If true, then participate in voting to allow refresh
     *                              procedure otherwise do not participate.
     * @param [in] efFiles          List of the elementary file path and identifier, and
     *                              this parameter only needs to be set to get refresh events
     *                              for refresh modes such as @ref telux::tel::RefreshMode::INIT,
     *                              @ref telux::tel::RefreshMode::FCN and
     *                              @ref telux::tel::RefreshMode::INIT_FULL_FCN.
     * @param [in] refreshParams    Session type @ref telux::tel::RefreshParams.
     *                              Client provides the session type, application
     *                              identifier to listen for the corresponding refresh
     *                              event.
     * @param [in] callback         Optional callback pointer to get the result of
     *                              setupRefreshConfig
     *
     * @returns Status of setupRefreshConfig i.e. success or suitable status code.
     *
     * @note   Eval: This is a new API and is being evaluated. It is subject to
     *         change and could break backwards compatibility.
     */
    virtual telux::common::Status setupRefreshConfig(
        SlotId slotId, bool isRegister, bool doVoting, std::vector<IccFile> efFiles,
        RefreshParams refreshParams, common::ResponseCallback callback = nullptr) = 0;

    /**
     * Allow or disallow the initiation of the card refresh procedure. This function enables
     * the client to vote on whether it is acceptable to start the refresh procedure. The refresh
     * will only commence once all registered clients (on HLOS or modem) have voted in favor of
     * starting.
     * This API should only be used after the client receives the card refresh notification via
     * @ref telux::tel::ICardListener::onRefreshEvent, which indicates the stage of waiting for
     * approval to refresh (@ref telux::tel::RefreshStage::WAITING_FOR_VOTES). This API must be
     * called within a specified time frame (default is 10 seconds) using allowRefresh(true) after
     * receiving the notification, otherwise, the modem will consider the refresh as failed, and
     * the client will be notified of the failure through the card refresh failure notification
     * via @ref telux::tel::ICardListener::onRefreshEvent and
     * @ref telux::tel::RefreshStage::ENDED_WITH_FAILURE after the timer in modem expires.
     *
     * On platforms with access control enabled, the caller must have the
     * TELUX_TEL_CARD_REFRESH_VOTING permission to successfully invoke this API.
     *
     * @param [in] slotId           Slot identifier corresponding to the card which needs to be
     *                              refreshed.
     * @param [in] allowRefresh     If true, allow the SIM refresh otherwise, disallow it.
     * @param [in] refreshParams    Session type @ref telux::tel::RefreshParams.
     *                              Client provides the session type, application
     *                              identifier to listen for the corresponding refresh
     *                              event.
     * @param [in] callback         Optional callback pointer to get the result of
     *                              allowCardRefresh
     *
     * @returns Status of allowCardRefresh i.e. success or suitable status code.
     *
     * @note   Eval: This is a new API and is being evaluated. It is subject to
     *         change and could break backwards compatibility.
     */
    virtual telux::common::Status allowCardRefresh(SlotId slotId, bool allowRefresh,
        RefreshParams refreshParams, common::ResponseCallback callback = nullptr) = 0;

    /**
     * Indicates that the card refresh procedure is completed from the client application's
     * perspective to the modem.
     * This API should only be used after the client receives the card refresh notification
     * via @ref telux::tel::ICardListener::onRefreshEvent, which indicates the stage of
     * starting the refresh procedure(@ref telux::tel::RefreshStage::STARTING) and the
     * client has invalidated the cache or reread the cache for the session type. This API
     * must be called within a specified time frame (default is 120 seconds) after receiving
     * the notification, otherwise, the modem will consider the refresh as failed, and the
     * client will be notified of the failure through the card refresh failure notification
     * via @ref telux::tel::ICardListener::onRefreshEvent and
     * @ref telux::tel::RefreshStage::ENDED_WITH_FAILURE after the timer in modem expires.
     *
     * Below table describes describes the session type and refresh mode in which the client
     * needs to call this API after the stage of starting the refresh
     * (@ref telux::tel::RefreshStage::STARTING).
     ********************************************************
     *  Mode  *                          Stage
     ********************************************************
     *        *WAIT      *                                  *
     *        *FOR_VOTES *           STARTING               *
     ********************************************************
     * FCN    * Vote if  * Reread the files (EFs) being     *
     *        * it is OK * refreshed and then invoke the    *
     *        *    to    * @ref telux::tel::ICardManager    *
     *        * continue * ::confirmRefreshHandlingCompleted*
     ********** with the ************************************
     * Init   * refresh. * Provisioning session: Invalidate *
     *        *          * all cached values.               *
     *        *          * Nonprovisioning session: Reread  *
     *        *          * the files (EFs), and then invoke *
     *        *          * the @ref telux::tel::ICardManager*
     *        *          * ::confirmRefreshHandlingCompleted*
     **********          ************************************
     * Init + *          * Provisioning session:Invalidate  *
     * FCN    *          * cached values of files (EFs) in  *
     *        *          * the FCN list.                    *
     *        *          * Nonprovisioning session: Reread  *
     *        *          * the files (EFs) in the FCN list, *
     *        *          * and then invoke the              *
     *        *          * @ref telux::tel::ICardManager    *
     *        *          * ::confirmRefreshHandlingCompleted*
     **********          ************************************
     * Init + *          * Provisioning session: Invalidate *
     * Full   *          * all cached values.               *
     * FCN    *          * Nonprovisioning session: Reread  *
     *        *          * the files (EFs), and then invoke *
     *        *          * the @ref telux::tel::ICardManager*
     *        *          * ::confirmRefreshHandlingCompleted*
     **********          ************************************
     * App    *          * Provisioning session: Invalidate *
     * reset  *          * all cached values.               *
     *        *          * Nonprovisioning session: invoke  *
     *        *          * the @ref telux::tel::ICardManager*
     *        *          * ::confirmRefreshHandlingCompleted*
     *        *          * and wait for End Stage.          *
     *        *          * Provisioning session: Wait for   *
     *        *          * the application state to be Ready*
     *        *          * or End Stage.                    *
     **********          ************************************
     * 3G     *          * Delete all cached values.        *
     * session*          * Nonprovisioning session: invoke  *
     * reset  *          * the @ref telux::tel::ICardManager*
     *        *          * ::confirmRefreshHandlingCompleted*
     ********************************************************
     *
     * On platforms with access control enabled, the caller must have the TELUX_TEL_CARD_REFRESH
     * permission to successfully invoke this API.
     *
     * @param [in] slotId           Slot identifier corresponding to the card which needs to be
     *                              refreshed.
     * @param [in] isCompleted      If true, the refresh handling is completed; otherwise, it
     *                              is not completed due to an error in invalidating the cache
     *                              or rereading the files.
     * @param [in] refreshParams    Session type @ref telux::tel::RefreshParams.
     *                              Client provides the session type, application
     *                              identifier to listen for the corresponding refresh
     *                              event.
     * @param [in] callback         Optional callback pointer to get the result of
     *                              confirmRefreshHandlingCompleted
     *
     * @returns Status of confirmRefreshHandlingCompleted i.e. success or suitable status code.
     *
     * @note   Eval: This is a new API and is being evaluated. It is subject to
     *         change and could break backwards compatibility.
     */
    virtual telux::common::Status confirmRefreshHandlingCompleted(SlotId slotId,
        bool isCompleted, RefreshParams refreshParams,
        common::ResponseCallback callback = nullptr) = 0;

    /**
     * Provides ability to retrieve content similar to that previously received on
     * telux::tel::ICardListener::onRefreshEvent.
     *
     * On platforms with access control enabled, the caller must have the TELUX_TEL_CARD_REFRESH
     * permission to successfully invoke this API.
     *
     * @param [in] slotId           Slot identifier corresponding to the card which needs to be
     *                              refreshed.
     * @param [in] refreshParams    Session type @ref telux::tel::RefreshParams.
     *                              Client provides the session type, application
     *                              identifier to listen for the corresponding refresh
     *                              event.
     * @param [in] callback         Callback function to get the result of request the last event
     *                              of card refresh.
     *
     * @returns Status of requestLastRefreshEvent i.e. success or suitable status code.
     *
     * @note   Eval: This is a new API and is being evaluated. It is subject to
     *         change and could break backwards compatibility.
     */
    virtual telux::common::Status requestLastRefreshEvent(SlotId slotId,
        RefreshParams refreshParams, refreshLastEventResponseCallback callback) = 0;

    /**
     * Register a listener for card events.
     *
     * @param [in] listener    Pointer to ICardListener object that processes the notification.
     *
     * @returns Status of registerListener i.e. success or suitable status code.
     */
    virtual telux::common::Status registerListener(std::shared_ptr<ICardListener> listener) = 0;

    /**
     * Remove a previously added listener.
     *
     * @param [in] listener    Pointer to ICardListener object that needs to be removed.
     *
     * @returns Status of removeListener i.e. success or suitable status code.
     */
    virtual telux::common::Status removeListener(std::shared_ptr<ICardListener> listener) = 0;

    virtual ~ICardManager(){};

};  // end of ICardManager

/**
 *@brief ICard represents currently inserted UICC or eUICC
 */
class ICard {
 public:
    /**
     * Get the card state for the slot id.
     *
     * @param [out] cardState  @ref CardState - state of the card.
     *
     * @returns Status of getCardState i.e. success or suitable status code.
     */
    virtual telux::common::Status getState(CardState &cardState) = 0;
    /**
     * Get card applications.
     *
     * @param [out] status   Status of getApplications i.e. success or suitable status code.
     *
     * @returns List of card applications.
     */
    virtual std::vector<std::shared_ptr<ICardApp>> getApplications(
        telux::common::Status *status = nullptr)
        = 0;

    /**
     * Open a logical channel to the SIM.
     *
     * On platforms with access control enabled, caller needs to have TELUX_TEL_CARD_OPS permission
     * to invoke this API successfully.
     *
     * @param [in] applicationId   Application Id.
     * @param [in] callback        Optional callback pointer to get the response of open logical
     *                             channel request.
     *
     * @returns Status of openLogicalChannel i.e. success or suitable status code.
     */
    virtual telux::common::Status openLogicalChannel(
        std::string applicationId, std::shared_ptr<ICardChannelCallback> callback = nullptr)
        = 0;

    /**
     * Close a previously opened logical channel to the SIM.
     *
     * On platforms with access control enabled, caller needs to have TELUX_TEL_CARD_OPS permission
     * to invoke this API successfully.
     *
     * @param [in] channelId   The channel ID to be closed.
     * @param [in] callback    Optional callback pointer to get the response of close logical
     *                         channel request.
     *
     * @returns Status of closeLogicalChannel i.e. success or suitable status code.
     */
    virtual telux::common::Status closeLogicalChannel(
        int channelId, std::shared_ptr<telux::common::ICommandResponseCallback> callback = nullptr)
        = 0;

    /**
     * Transmit an APDU to the ICC card over a logical channel.
     *
     * On platforms with access control enabled, caller needs to have TELUX_TEL_CARD_OPS permission
     * to invoke this API successfully.
     *
     * @param [in] channel       Channel Id of the channel to use for communication.
     *                           Has to be greater than zero.
     * @param [in] cla           Class of the APDU command.
     * @param [in] instruction   Instruction of the APDU command.
     * @param [in] p1            Instruction Parameter 1 value of the APDU command.
     * @param [in] p2            Instruction Parameter 2  value of the APDU command.
     * @param [in] p3            Number of bytes present in the data field of the APDU command.
     *                           If p3 is negative, a 4 byte APDU is sent to the SIM.
     * @param [in] data          Data to be sent with the APDU.
     * @param [in] callback      Optional callback pointer to get the response of
     *                           transmit APDU request.
     *
     * @returns Status of transmitApduLogicalChannel i.e. success or suitable status code.
     */
    virtual telux::common::Status transmitApduLogicalChannel(int channel, uint8_t cla,
        uint8_t instruction, uint8_t p1, uint8_t p2, uint8_t p3, std::vector<uint8_t> data,
        std::shared_ptr<ICardCommandCallback> callback = nullptr)
        = 0;

    /**
     * Exchange APDUs with the SIM on a basic channel.
     *
     * On platforms with access control enabled, caller needs to have TELUX_TEL_CARD_OPS permission
     * to invoke this API successfully.
     *
     * @param [in] cla           Class of the APDU command.
     * @param [in] instruction   Instruction of the APDU command.
     * @param [in] p1            Instruction Param1 value of the APDU command.
     * @param [in] p2            Instruction Param1  value of the APDU command.
     * @param [in] p3            Number of bytes present in the data field of the APDU command.
     *                           If p3 is negative, a 4 byte APDU is sent to the SIM.
     * @param [in] data          Data to be sent with the APDU.
     * @param [in] callback      Optional callback pointer to get the response of
     *                           transmit APDU request.
     *
     * @returns Status of transmitApduBasicChannel i.e. success or suitable status code.
     */
    virtual telux::common::Status transmitApduBasicChannel(uint8_t cla, uint8_t instruction,
        uint8_t p1, uint8_t p2, uint8_t p3, std::vector<uint8_t> data,
        std::shared_ptr<ICardCommandCallback> callback = nullptr)
        = 0;

    /**
     * Performs SIM IO operation, This is similar to the TS 27.007 "restricted SIM" operation
     * where it assumes all of the EF selection will be done by the callee.
     *
     * On platforms with access control enabled, caller needs to have TELUX_TEL_CARD_OPS permission
     * to invoke this API successfully.
     *
     * @param [in] fileId    Elementary File Identifier
     * @param [in] command   APDU Command for SIM IO operation
     * @param [in] p1        Instruction Param1 value of the APDU command
     * @param [in] p2        Instruction Param2 value of the APDU command
     * @param [in] p3        Number of bytes present in the data field of APDU command.
     *                       If p3 is negative, a 4 byte APDU is sent to the SIM.
     * @param [in] filePath  Path of the file
     * @param [in] data      Data to be sent with the APDU,
     *                       send empty or null string in case no data
     * @param [in] pin2      Pin value of the SIM. Invalid attempt of PIN2 value will lock the SIM.
     *                       send empty or null string in case of no Pin2 value
     * @param [in] aid       Application identifier, send empty or null string in case of no aid
     * @param [in] callback  Optional callback pointer to get the response of SIM IO request
     *
     * @returns - Status of exchangeSimIO i.e. success or suitable status code
     */
    virtual telux::common::Status exchangeSimIO(uint16_t fileId, uint8_t command, uint8_t p1,
        uint8_t p2, uint8_t p3, std::string filePath, std::vector<uint8_t> data, std::string pin2,
        std::string aid, std::shared_ptr<ICardCommandCallback> callback = nullptr)
        = 0;

    /**
     * Get associated slot id for ICard
     *
     * @returns SlotId
     */
    virtual int getSlotId() = 0;

    /**
     * Request eUICC identifier (EID) of eUICC card.
     *
     * On platforms with access control enabled, caller needs to have TELUX_TEL_PRIVATE_INFO_READ
     * permission to invoke this API successfully.
     *
     * @param [in] callback          Callback function to get the result of request EID.
     *
     * @returns  Status of request EID i.e. success or suitable error code.
     *
     * @dependencies Card should be eUICC capable
     */
    virtual telux::common::Status requestEid(EidResponseCallback callback) = 0;

    /**
     * Get file handler for reading or writing to EF on SIM.
     *
     *
     * @returns ICardFileHandler
     */
    virtual std::shared_ptr<ICardFileHandler> getFileHandler() = 0;

   /**
    * Checks whether the NTN profile is activated on a given slot.
    *
    * @returns If true NTN profile is activated or else not-activated.
    *
    * @note   Eval: This is a new API and is being evaluated. It is subject to
    *         change and could break backwards compatibility.
    */
   virtual bool isNtnProfileActive() = 0;

   virtual ~ICard() {};
};

/**
 * Interface for Card callback object. Client needs to implement this interface to get
 * single shot responses for commands like open logical channel and close logical channel.
 *
 * The methods in callback can be invoked from multiple different threads. The implementation
 * should be thread safe.
 */
class ICardChannelCallback : public telux::common::ICommandCallback {
 public:
    /**
     * This function is called with the response to the open logical channel operation.
     *
     * @param [in] channel  Channel Id for the logical channel.
     * @param [in] result   @ref IccResult of open logical channel.
     * @param [in] error    @ref telux::common::ErrorCode of the request.
     *
     */
    virtual void onChannelResponse(int channel, IccResult result, telux::common::ErrorCode error)
        = 0;
};

class ICardCommandCallback : public telux::common::ICommandCallback {
 public:
    /**
     * This function is called when SIM Card transmit APDU over Logical, Basic Channel and
     * Exchange Sim IO.
     *
     * @param [in] result   @ref IccResult of transmit APDU command
     * @param [in] error    @ref telux::common::ErrorCode of the request,
     *                      Possible error codes are
     *                      - @ref telux::common::ErrorCode::SUCCESS
     *                      - @ref telux::common::ErrorCode::INTERNAL
     *                      - @ref telux::common::ErrorCode::NO_MEMORY
     *                      - @ref telux::common::ErrorCode::INVALID_ARG
     *                      - @ref telux::common::ErrorCode::MISSING_ARG
     */
    virtual void onResponse(IccResult result, telux::common::ErrorCode error) = 0;
};

/**
 * Interface for SIM Card Listener object. Client needs to implement this interface to get
 * access to card services notifications on card state change.
 *
 * The methods in listener can be invoked from multiple different threads. The implementation
 * should be thread safe.
 */
class ICardListener : public common::IServiceStatusListener {
 public:
    /**
     * This function is called when info of card gets updated.
     *
     * @param [in] slotId   Slot identifier.
     */
    virtual void onCardInfoChanged(int slotId) {
    }

    /**
     * This function is called when a card refresh notification comes from the card.
     *
     * @param [in] slotId           Slot identifier.
     * @param [in] stage            Card refresh stage @ref telux::tel:RefreshStage
     * @param [in] mode             Card refresh mode @ref telux::tel:RefreshMode
     * @param [in] efFiles          List of the elementary file path and identifier
     * @param [in] refreshParams    Session type @ref telux::tel::RefreshParams.
     *                              Client provides the session type, application
     *                              identifier to listen for the corresponding refresh
     *                              event.
     *
     * Below table describes the expected behavior of a client when it receives a refresh
     * indication after registering for it. The behavior depends on the mode and the stage,
     * as indicated in the refresh indication.The refresh will only commence once all
     * registered clients (on HLOS or modem) have voted in favor of starting. On receiving
     * this refresh stage WAIT_FOR_VOTES, client is expected to call
     * @ref telux::tel::ICardManager::allowCardRefresh to allow refresh procedure to start.
     ***************************************************************************************
     *  Mode  *                          Stage                                             *
     ***************************************************************************************
     *        *WAIT      *                                  *                              *
     *        *FOR_VOTES *           STARTING               *         END SUCCESS          *
     ************************************************************************************* *
     * Reset  *          * Delete all cached values. The    * This event might be missing. *
     *        *          * card is reinitialized and its    * The client should look at    *
     *        *          * status is updated.               * the card status and          *
     *        *          *                                  * application status.          *
     ********** Vote if  *******************************************************************
     * FCN    * it is OK * Reread the files (EFs) being     * No action is required.       *
     *        *    to    * refreshed and then invoke the    *                              *
     *        * continue * @ref telux::tel::ICardManager    *                              *
     *        * with the * ::confirmRefreshHandlingCompleted*                              *
     ********** refresh. *******************************************************************
     * Init   *          * Provisioning session: Invalidate * Provisioning session: Reread *
     *        *          * all cached values.               * all files (EFs) (if not done *
     *        *          * Nonprovisioning session: Reread  * when the application state is*
     *        *          * the files (EFs), and then invoke * back to Ready).              *
     *        *          * the @ref telux::tel::ICardManager*                              *
     *        *          * ::confirmRefreshHandlingCompleted*                              *
     **********          *******************************************************************
     * Init + * In Init +* Provisioning session:Invalidate  * Provisioning session: Reread *
     * FCN    * FCN mode,* cached values of files (EFs) in  * files (EFs) in the FCN list  *
     *        * client   * the FCN list.                    * (if not done when the        *
     *        * receives * Nonprovisioning session: Reread  * application state is back to *
     *        * two indi-* the files (EFs) in the FCN list, * Ready).                      *
     *        * cations  * and then invoke the              *                              *
     *        * both     * @ref telux::tel::ICardManager    *                              *
     *        * requests * ::confirmRefreshHandlingCompleted*                              *
     *        * vote: one*                                  *                              *
     *        * for Init *                                  *                              *
     *        * one for  *                                  *                              *
     *        * FCN.     *                                  *                              *
     **********          *******************************************************************
     * Init + *          * Provisioning session: Invalidate * Provisioning session: Reread *
     * Full   *          * all cached values.               * all files (EFs) (if not done *
     * FCN    *          * Nonprovisioning session: Reread  * when the application state is*
     *        *          * the files (EFs), and then invoke * back to Ready).              *
     *        *          * the @ref telux::tel::ICardManager*                              *
     *        *          * ::confirmRefreshHandlingCompleted*                              *
     **********          *******************************************************************
     * App    *          * Provisioning session: Invalidate * Provisioning session: Reread *
     * reset  *          * all cached values.               * all files (EFs) (if not done *
     *        *          * Nonprovisioning session: invoke  * when the application state is*
     *        *          * the @ref telux::tel::ICardManager* back to Ready)               *
     *        *          * ::confirmRefreshHandlingCompleted* Nonprovisioning session:     *
     *        *          * and wait for End Stage.          * Reread all files (EFs).      *
     *        *          * Provisioning session: Wait for   *                              *
     *        *          * the application state to be Ready*                              *
     *        *          * or End Stage.                    *                              *
     **********          *******************************************************************
     * 3G     *          * Delete all cached values.        * Provisioning session: Reread *
     * session*          * Nonprovisioning session: invoke  * all of the files (EFs)       *
     * reset  *          * the @ref telux::tel::ICardManager* discarded when the refresh   *
     *        *          * ::confirmRefreshHandlingCompleted* was started (if not done when*
     *        *          *                                  * the application state        *
     *        *          *                                  * returned to Ready).          *
     ***************************************************************************************
     *
     * @note   Eval: This is a new API and is being evaluated. It is subject to
     *         change and could break backwards compatibility.
     */
    virtual void onRefreshEvent(
        int slotId, RefreshStage stage, RefreshMode mode, std::vector<IccFile> efFiles,
        RefreshParams refreshParams) {
    }

    virtual ~ICardListener() {
    }
};

/** @} */ /* end_addtogroup telematics_card */

}  // End of namespace tel

}  // End of namespace telux

#endif // TELUX_TEL_CARDMANAGER_HPP
