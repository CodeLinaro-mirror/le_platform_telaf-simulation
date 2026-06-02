/*
 *  Copyright (c) 2017-2018,2020 The Linux Foundation. All rights reserved.
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
 * ​​​​​Changes from Qualcomm Technologies, Inc. are provided under the following license:
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file       CallListener.hpp
 * @brief      Interface for Call listener object. Client needs to implement this interface
 *             to get access to Call related notifications like call state changes and ecall
 *             state change.
 *
 *             The methods in listener can be invoked from multiple different threads. The
 *             implementation should be thread safe.
 */

#ifndef TELUX_TEL_CALLLISTENER_HPP
#define TELUX_TEL_CALLLISTENER_HPP

#include <vector>
#include <memory>

#include <telux/common/CommonDefines.hpp>

#include <telux/tel/Call.hpp>
#include <telux/tel/PhoneDefines.hpp>
#include <telux/tel/Phone.hpp>
#include <telux/tel/ECallDefines.hpp>

namespace telux {

namespace tel {

/** @addtogroup telematics_call
 * @{ */

class ICall;

/**
 * @brief A listener class for monitoring changes in call,
 * including call state change and ECall state change.
 * Override the methods for the state that you wish to receive updates for.
 *
 * The methods in listener can be invoked from multiple different threads. The implementation
 * should be thread safe.
 */
class ICallListener : public common::IServiceStatusListener {
public:
   /**
    * This function is called when device receives an incoming/waiting call.
    *
    * On platforms with access control enabled, the caller needs to have TELUX_TEL_CALL_INFO_READ
    * permission to receive this notification.
    *
    * @param [in] call -  Pointer to ICall instance
    */
   virtual void onIncomingCall(std::shared_ptr<ICall> call) {
   }

   /**
    * This function is called when there is a change in call attributes
    *
    * On platforms with access control enabled, the caller needs to have TELUX_TEL_CALL_INFO_READ
    * permission to receive this notification.
    *
    * @param [in] call -  Pointer to ICall instance
    */
   virtual void onCallInfoChange(std::shared_ptr<ICall> call) {
   }

   /**
    * This function is called when device completes MSD Transmission.
    *
    * On platforms with access control enabled, the caller needs to have TELUX_TEL_ECALL_MGMT
    * permission to receive this notification.
    *
    * @param [in] phoneId - Unique Id of phone on which MSD Transmission Status is being reported
    * @param [in] errorCode - Indicates MSD Transmission status i.e. success or failure
    *
    * @deprecated Use another onECallMsdTransmissionStatus() API with argument
    * @ref ECallMsdTransmissionStatus
    */
   virtual void onECallMsdTransmissionStatus(int phoneId, telux::common::ErrorCode errorCode) {
   }

   /**
    * This function is called when there is Minimum Set of Data (MSD) transmission.
    * The MSD transmission happens at call connect and also when the modem or client
    * responds to MSD pull request from PSAP.
    *
    *
    * On platforms with access control enabled, the caller needs to have TELUX_TEL_ECALL_MGMT
    * permission to receive this notification.
    *
    * @param [in] phoneId - Unique Id of phone on which MSD Transmission Status is being reported
    * @param [in] msdTransmissionStatus - Indicates MSD Transmission status
    * @ref ECallMsdTransmissionStatus
    *
    */
   virtual void onECallMsdTransmissionStatus(
      int phoneId, telux::tel::ECallMsdTransmissionStatus msdTransmissionStatus) {
   }

   /**
    * This function is called when MSD update is requested by PSAP.
    *
    * Client is expected to update the MSD using @ref telux::tel::ICallManager::updateECallMsd
    * upon receiving this notification. Modem updates its internal cache and responds to PSAP
    * with the new MSD.
    * In situations, where the client fails to update the MSD, modem will time out and send the
    * outdated MSD from its cache.
    *
    * On platforms with access control enabled, the caller needs to have TELUX_TEL_ECALL_MGMT
    * permission to receive this notification.
    *
    * @param [in] phoneId - Unique Id of phone on which MSD update request is received.
    *
    */
   virtual void OnMsdUpdateRequest(int phoneId) {
   }

   /**
    * Note that the API OnTpsMsdUpdateRequest is an alias for OnMsdUpdateRequest and
    * OnTpsMsdUpdateRequest would deprecated and eventually removed. As of now, it is retained
    * for backward compatibility.
    */
   #define OnTpsMsdUpdateRequest OnMsdUpdateRequest

   /**
    * This function is called when the eCall High Level Application Protocol(HLAP) timers status
    * is changed.
    *
    * On platforms with access control enabled, the caller needs to have TELUX_TEL_ECALL_MGMT
    * permission to receive this notification.
    *
    * @param [in] phoneId - Unique Id of phone on which HLAP timer status is being reported
    * @param [in] timersStatus - Indicates the HLAP timer event
    *                            @ref ECallHlapTimerEvents
    *
    */
   virtual void onECallHlapTimerEvent(int phoneId, ECallHlapTimerEvents timersStatus) {
   }

    /**
    * This function is called whenever there is a scan failure after one round of network scan
    * during origination of emergency call or at any time during the emergency call.
    *
    * During origination of an ecall or in between an ongoing ecall, if the UE is in an area of
    * no/poor coverage and loses service, the modem will perform network scan and try to register
    * on any available network.
    * If the scan completes successfully and the device finds a suitable cell, the ecall will be
    * placed and the call state changes to the active state.
    * If the network scan fails then this function will be invoked after one round of network scan.
    *
    * On platforms with access control enabled, the caller needs to have TELUX_TEL_ECALL_MGMT
    * permission to receive this notification.
    *
    * @param [in] phoneId - Unique Id of phone on which network scan failure reported.
    *
    */
   virtual void onEmergencyNetworkScanFail(int phoneId) {
   }

   /**
    * This function is called whenever emergency callback mode(ECBM) changes.
    *
    * @param [in] mode   - Indicates the status of the ECBM.
    *                      @ref EcbMode
    *
    */
   virtual void onEcbmChange(telux::tel::EcbMode mode) {
   }

   /**
    * When the network doesn't play an in-band ringback tone for an alerting call, an application
    * can play the ringback tone locally based on this notification. This function is called when
    * the ringback tone needs to be started or stopped.
    *
    * On platforms with access control enabled, the caller needs to have TELUX_TEL_CALL_INFO_READ
    * permission to receive this notification.
    *
    * @param [in] isAlerting - true to start playing ringback tone, false to stop playing ringback
    * tone.
    * @param [in] phoneId - Unique Id of phone on which local ringback tone need to be triggered.
    *
    * @note    Eval: This is a new API and is being evaluated. It is subject to change and
    *          could break backward compatibility.
    */
   virtual void onRingbackTone(bool isAlerting, int phoneId) {
   }

   /**
    * This function is called when a modification request is triggered by the other party to
    * change the call from a normal voice call to a real time text (RTT) call. This API shall not
    * be invoked when other party sends the modification request to change the call from real time
    * text (RTT) to normal voice call.
    *
    * On platforms with access control enabled, the caller needs to have TELUX_TEL_CALL_INFO_READ
    * permission to receive this notification.
    *
    * @param [in] rttMode - @ref telux::tel::RttMode::FULL to indicate a upgrade request,
    * @param [in] callId - Unique CallId on which upgrade request was triggered.
    * @param [in] phoneId - Unique Id of phone on which upgrade request was triggered.
    *
    * @note    Eval: This is a new API and is being evaluated. It is subject to change and
    *          could break backward compatibility.
    */
   virtual void onModifyCallRequest(RttMode rttMode, int callId , int phoneId) {
   }

   /**
    * This function is called when a RTT message is received from a remote party.
    *
    * On platforms with access control enabled, the caller needs to have TELUX_TEL_CALL_INFO_READ
    * permission to receive this notification.
    *
    * @param [in] phoneId  Unique Id of phone on which RTT message is received.
    * @param [in] message  Text message received from device is in UTF8 encoding format.
    *                      It supports the English language.
    *
    * @note    Eval: This is a new API and is being evaluated. It is subject to change and
    *          could break backward compatibility.
    */
   virtual void onRttMessage(int phoneId, std::string message) {}

   /**
    * This function is called to notify the clients whether eCall will be redialed or not by the
    * modem along with the reason for the operation.
    *
    * Note: In situations where the user does not configure retry eCall parameters using
    * @ref telux::tel::configureECallRedial, the default eCall retry parameters will be considered
    * by the modem.
    *
    * Behavior of redial:
    *
    * ERA-GLONASS eCall - During an eCall redial, when AP sends a call termination request using
    * @ref telux::tel::ICall::hangup(), modem will terminate any ongoing redials.
    * European eCall(EU) eCall - During an eCall redial, when AP sends a call termination request
    * using @ref telux::tel::ICall::hangup(), modem will not terminate any ongoing redials.
    *
    * On platforms with access control enabled, the caller needs to have TELUX_TEL_ECALL_MGMT
    * permission to receive this notification.
    *
    * @param [in] phoneId - Unique identifier of phone on which eCall redial information is
    *                       received.
    * @param [in] info - Indicates eCall redial information
    *                    1. Modem performs redial of eCall when its origination has failed or
    *                       it gets dropped before receipt of MSD transmission status.
    *                       In above situation, the contents of info is as follows:
    *                       @ref telux::tel::ECallRedialInfo::willECallRedial is true and
    *                       @ref telux::tel::ECallRedialInfo::reason can either be
    *                           telux::tel::ReasonType::CALL_ORIG_FAILURE or
    *                           telux::tel::ReasonType::CALL_DROP.
    *                    2. Modem does not perform redial when eCall is successfully connected or
    *                       the number of attempts of redial have been exhausted.
    *                       In above situation, the contents of info is as follows:
    *                       @ref telux::tel::ECallRedialInfo::willECallRedial is false and
    *                       @ref telux::tel::ECallRedialInfo::reason can either be
    *                           telux::tel::ReasonType::CALL_CONNECTED or
    *                           telux::tel::ReasonType::MAX_REDIAL_ATTEMPTED.
    *
    * @note    Eval: This is a new API and is being evaluated. It is subject to change and
    *          could break backward compatibility.
    */
   virtual void onECallRedial(int phoneId, ECallRedialInfo info) {}

   /**
     * This function is invoked upon detection of a DTMF tone at the receiving endpoint during PS
     * calls. For CS call, @ref IVoiceListener::onDtmfToneDetection is invoked as DTMF tone is
     * delivered as part of signaling information.
     * @ref IVoiceListener::onDtmfToneDetection is used in conjuction with
     * @ref IAudioVoiceStream::registerListener().
     *
     * On platforms with access control enabled, the caller needs to have TELUX_TEL_CALL_INFO_READ
     * permission to receive this notification.
     *
     * @param [in] phoneId    Represents the phone corresponding to which the DTMF digit
     *                        was received
     * @param [in] dtmfTone   Represents single character.
     *                        Valid values:
     *                        • Digits: 0–9
     *                        • Special characters: '\*' and '#'
     *
     * @note    Eval: This is a new API and is being evaluated. It is subject to change and
     *          could break backward compatibility.
     */
    virtual void onDtmfReceived(int phoneId, char dtmfTone) {
    }

   virtual ~ICallListener() {
   }
};
/** @} */ /* end_addtogroup telematics_call */

}  // End of namespace tel

}  // End of namespace telux

#endif // TELUX_TEL_CALLLISTENER_HPP
