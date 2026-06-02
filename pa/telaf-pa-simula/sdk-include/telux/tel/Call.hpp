/*
 *  Copyright (c) 2017,2021 The Linux Foundation. All rights reserved.
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
 * @file       Call.hpp
 * @brief      Phone Call class is the primary interface to process Call requests.
 */

#ifndef TELUX_TEL_CALL_HPP
#define TELUX_TEL_CALL_HPP

#include <memory>
#include <string>

#include <telux/common/CommonDefines.hpp>
#include <telux/tel/PhoneDefines.hpp>

namespace telux {

namespace tel {

/** @addtogroup telematics_call
 * @{ */

class IPhone;

/**
 * @brief ICall represents a call in progress. An ICall cannot be directly created by the client,
 *        rather it  is returned as a result of instantiating a call or from the PhoneListener
 *        when receiving an incoming call.
 */
class ICall {
public:
   /**
    * Allows the client to answer the call. This is only applicable for CallState::INCOMING and
    * CallState::WAITING calls during a normal voice call.
    * If a Waiting call is being answered and the existing call is Active, then existing call
    * will move to Hold state.If the existing call is on Hold already, then it will remain on Hold.
    * The waiting call state transition from Waiting to Active.
    * Answering an incoming RTT call is supported.
    * However, if an RTT call is waiting, there is no support to answer such a call currently.
    *
    * On platforms with access control enabled, the caller needs to have TELUX_TEL_CALL_MGMT
    * permission to successfully invoke this API.
    *
    * @param [in] callback - optional callback pointer to get the response of answer request
    * below are possible error codes for callback response
    *        - @ref telux::common::ErrorCode::SUCCESS
    *        - @ref telux::common::ErrorCode::RADIO_NOT_AVAILABLE
    *        - @ref telux::common::ErrorCode::NO_MEMORY
    *        - @ref telux::common::ErrorCode::MODEM_ERR
    *        - @ref telux::common::ErrorCode::INTERNAL_ERR
    *        - @ref telux::common::ErrorCode::INVALID_STATE
    *        - @ref telux::common::ErrorCode::INVALID_CALL_ID
    *        - @ref telux::common::ErrorCode::INVALID_ARGUMENTS
    *        - @ref telux::common::ErrorCode::OPERATION_NOT_ALLOWED
    *        - @ref telux::common::ErrorCode::GENERIC_FAILURE
    * @param [in] mode - @ref telux::tel::RttMode::DISABLED - To answer incoming call as a normal
    *                    voice call.
    *                    @ref telux::tel::RttMode::FULL - To answer incoming call as a real time
    *                    text (RTT) call.
    *
    * @returns Status of answer function i.e. success or suitable error code.
    */
   virtual telux::common::Status
      answer(std::shared_ptr<telux::common::ICommandResponseCallback> callback = nullptr,
        RttMode mode = RttMode::DISABLED)
      = 0;

   /**
    * Puts the ongoing call on hold.
    *
    * On platforms with access control enabled, the caller needs to have TELUX_TEL_CALL_MGMT
    * permission to successfully invoke this API.
    *
    * @param [in] callback - optional callback pointer to get the response of hold request
    * below are possible error codes for callback response
    *        - @ref telux::common::ErrorCode::SUCCESS
    *        - @ref telux::common::ErrorCode::RADIO_NOT_AVAILABLE
    *        - @ref telux::common::ErrorCode::NO_MEMORY
    *        - @ref telux::common::ErrorCode::MODEM_ERR
    *        - @ref telux::common::ErrorCode::INTERNAL_ERR
    *        - @ref telux::common::ErrorCode::INVALID_STATE
    *        - @ref telux::common::ErrorCode::INVALID_CALL_ID
    *        - @ref telux::common::ErrorCode::INVALID_ARGUMENTS
    *        - @ref telux::common::ErrorCode::OPERATION_NOT_ALLOWED
    *        - @ref telux::common::ErrorCode::GENERIC_FAILURE
    *
    *  @returns Status of hold function i.e. success or suitable error code.
    */
   virtual telux::common::Status
      hold(std::shared_ptr<telux::common::ICommandResponseCallback> callback = nullptr)
      = 0;

   /**
    * Resumes this call from on-hold state to active state
    *
    * On platforms with Access control enabled, Caller needs to have TELUX_TEL_CALL_MGMT permission
    * to invoke this API successfully.
    *
    * @param [in] callback - optional callback pointer to get the response of resume request
    * below are possible error codes for callback response
    *        - @ref telux::common::ErrorCode::SUCCESS
    *        - @ref telux::common::ErrorCode::RADIO_NOT_AVAILABLE
    *        - @ref telux::common::ErrorCode::NO_MEMORY
    *        - @ref telux::common::ErrorCode::MODEM_ERR
    *        - @ref telux::common::ErrorCode::INTERNAL_ERR
    *        - @ref telux::common::ErrorCode::INVALID_STATE
    *        - @ref telux::common::ErrorCode::INVALID_CALL_ID
    *        - @ref telux::common::ErrorCode::INVALID_ARGUMENTS
    *        - @ref telux::common::ErrorCode::OPERATION_NOT_ALLOWED
    *        - @ref telux::common::ErrorCode::GENERIC_FAILURE
    *
    *  @returns Status of resume function i.e. success or suitable error code.
    */
   virtual telux::common::Status
      resume(std::shared_ptr<telux::common::ICommandResponseCallback> callback = nullptr)
      = 0;

   /**
    * Reject the incoming/waiting call. Only applicable for CallState::INCOMING and
    * CallState::WAITING calls.
    *
    * On platforms with access control enabled, the caller needs to have TELUX_TEL_CALL_MGMT
    * permission to successfully invoke this API.
    *
    * @param [in] callback - optional callback pointer to get the response of reject request
    * below are possible error codes for callback response
    *        - @ref telux::common::ErrorCode::SUCCESS
    *        - @ref telux::common::ErrorCode::RADIO_NOT_AVAILABLE
    *        - @ref telux::common::ErrorCode::NO_MEMORY
    *        - @ref telux::common::ErrorCode::MODEM_ERR
    *        - @ref telux::common::ErrorCode::INTERNAL_ERR
    *        - @ref telux::common::ErrorCode::INVALID_STATE
    *        - @ref telux::common::ErrorCode::INVALID_CALL_ID
    *        - @ref telux::common::ErrorCode::INVALID_ARGUMENTS
    *        - @ref telux::common::ErrorCode::OPERATION_NOT_ALLOWED
    *        - @ref telux::common::ErrorCode::GENERIC_FAILURE
    *
    * @returns Status of reject function i.e. success or suitable error code.
    */
   virtual telux::common::Status
      reject(std::shared_ptr<telux::common::ICommandResponseCallback> callback = nullptr)
      = 0;

   /**
    * Reject the call and  send an SMS to caller. Only applicable for CallState::INCOMING
    * and CallState::WAITING calls.
    *
    * On platforms with access control enabled, the caller needs to have TELUX_TEL_CALL_MGMT
    * permission to successfully invoke this API.
    *
    * @param [in] rejectSMS SMS string used to send in response to a call rejection.
    * @param [in] callback - optional callback pointer to get the response of rejectwithSMS request
    * below are possible error codes for callback response
    *        - @ref telux::common::ErrorCode::SUCCESS
    *        - @ref telux::common::ErrorCode::RADIO_NOT_AVAILABLE
    *        - @ref telux::common::ErrorCode::NO_MEMORY
    *        - @ref telux::common::ErrorCode::MODEM_ERR
    *        - @ref telux::common::ErrorCode::INTERNAL_ERR
    *        - @ref telux::common::ErrorCode::INVALID_STATE
    *        - @ref telux::common::ErrorCode::INVALID_CALL_ID
    *        - @ref telux::common::ErrorCode::INVALID_ARGUMENTS
    *        - @ref telux::common::ErrorCode::OPERATION_NOT_ALLOWED
    *        - @ref telux::common::ErrorCode::GENERIC_FAILURE
    *
    *  @deprecated This API not being supported
    *
    *  @returns Status of success for call reject() or suitable error code.
    *
    */
   virtual telux::common::Status
      reject(const std::string &rejectSMS,
             std::shared_ptr<telux::common::ICommandResponseCallback> callback = nullptr)
      = 0;

   /**
    * Hangup the call if the call state is either active, hold, dialing, waiting or alerting.
    *
    * On platforms with access control enabled, the caller needs to have TELUX_TEL_CALL_MGMT
    * permission to successfully invoke this API.
    *
    * @param [in] callback - optional callback pointer to get the response of hangup request
    * below are possible error codes for callback response
    *        - @ref telux::common::ErrorCode::SUCCESS
    *        - @ref telux::common::ErrorCode::RADIO_NOT_AVAILABLE
    *        - @ref telux::common::ErrorCode::NO_MEMORY
    *        - @ref telux::common::ErrorCode::MODEM_ERR
    *        - @ref telux::common::ErrorCode::INTERNAL_ERR
    *        - @ref telux::common::ErrorCode::INVALID_STATE
    *        - @ref telux::common::ErrorCode::INVALID_CALL_ID
    *        - @ref telux::common::ErrorCode::INVALID_ARGUMENTS
    *        - @ref telux::common::ErrorCode::OPERATION_NOT_ALLOWED
    *        - @ref telux::common::ErrorCode::GENERIC_FAILURE
    *
    * @returns Status of hangup i.e. success or suitable error code.
    */
   virtual telux::common::Status
      hangup(std::shared_ptr<telux::common::ICommandResponseCallback> callback = nullptr)
      = 0;

   /**
    * Play a DTMF tone and stop it. The interval for which the tone is played is dependent on the
    * system implementation. If continuous DTMF tone is playing, it will be stopped.
    * This API is used to play DTMF tone on TX path so that it is heard on far end. For DTMF
    * playback on local device on the RX path use @ref telux::audio::IAudioVoiceStream::playDtmfTone
    *
    * On platforms with access control enabled, the caller needs to have TELUX_TEL_CALL_MGMT
    * permission to successfully invoke this API.
    *
    * @param [in] tone - a single character with one of 12 values: 0-9, *, #.
    *
    * @param [in] callback - Optional callback pointer to get the result of
    * playDtmfTones function
    *
    * @returns Status of playDtmfTones i.e. success or suitable error code.
    */
   virtual telux::common::Status playDtmfTone(
      char tone, std::shared_ptr<telux::common::ICommandResponseCallback> callback = nullptr)
      = 0;

   /**
    * Starts a continuous DTMF tone. To terminate the continous DTMF tone,stopDtmfTone API needs to
    * be invoked explicitly. This API is used to play DTMF tone on TX path so that it is heard on
    * far end. For DTMF playback on local device on the RX path use
    * @ref telux::audio::IAudioVoiceStream::playDtmfTone
    *
    * On platforms with access control enabled, the caller needs to have TELUX_TEL_CALL_MGMT
    * permission to successfully invoke this API.
    *
    * @param [in] tone - a single character with one of 12 values: 0-9, *, #.
    * @param [in] callback - Optional callback pointer to get the result of
    * startDtmfTone function.
    *
    * @returns Status of startDtmfTone i.e. success or suitable error code.
    */
   virtual telux::common::Status startDtmfTone(
      char tone, std::shared_ptr<telux::common::ICommandResponseCallback> callback = nullptr)
      = 0;

   /**
    * Stop the currently playing continuous DTMF tone.
    *
    * On platforms with access control enabled, the caller needs to have TELUX_TEL_CALL_MGMT
    * permission to successfully invoke this API.
    *
    * @param [in] callback - Optional callback pointer to get the result of
    *                        stopDtmfTone function.
    *
    * @returns Status of stopDtmfTone i.e. success or suitable error code.
    */
   virtual telux::common::Status
      stopDtmfTone(std::shared_ptr<telux::common::ICommandResponseCallback> callback = nullptr)
      = 0;

   /**
    * Get the current state of the call, such as ringing, in progress etc.
    *
    * On platforms with access control enabled, the caller needs to have TELUX_TEL_CALL_INFO_READ
    * permission to successfully invoke this API.
    *
    * @returns CallState - enumeration representing call State
    */
   virtual CallState getCallState() = 0;

   /**
    * Get the unique index of the call assigned by Telephony subsystem
    *
    * On platforms with access control enabled, the caller needs to have TELUX_TEL_CALL_MGMT
    * permission to successfully invoke this API.
    *
    * @returns Call Index
    */
   virtual int getCallIndex() = 0;

   /**
    * Get the direction of the call
    *
    * On platforms with access control enabled, the caller needs to have TELUX_TEL_CALL_MGMT
    * permission to successfully invoke this API.
    *
    * @returns CallDirection - enumeration representing call direction
    *                          i.e. INCOMING/ OUTGOING
    */
   virtual CallDirection getCallDirection() = 0;

   /**
    * Get the dailing number
    *
    * On platforms with access control enabled, the caller needs to have
    * TELUX_TEL_CALL_PRIVATE_INFO permission to successfully invoke this API.
    *
    * @returns Phone Number to which the call was dialed out.
    *          Empty string in case of INCOMING call direction.
    */
   virtual std::string getRemotePartyNumber() = 0;

   /**
    * Get the cause of the termination of the call.
    *
    * On platforms with access control enabled, the caller needs to have TELUX_TEL_CALL_INFO_READ
    * permission to successfully invoke this API.
    *
    * @returns Enum representing call end cause.
    */
   virtual CallEndCause getCallEndCause() = 0;

   /**
    * Get the SIP error code for the termination of the IMS call.
    * Refer RFC3261 Section 21 for error description.
    *
    * On platforms with access control enabled, the caller needs to have TELUX_TEL_CALL_INFO_READ
    * permission to successfully invoke this API.
    *
    * @returns integer representing SIP error code.
    *
    * @note    Eval: This is a new API and is being evaluated. It is subject to change and
    *          could break backward compatibility.
    */
   virtual int getSipErrorCode() = 0;

   /**
    * Get id of the phone object which represents the network/SIM on which
    * the call is in progress.
    *
    * On platforms with access control enabled, the caller needs to have TELUX_TEL_CALL_INFO_READ
    * permission to successfully invoke this API.
    *
    * @returns Phone Id.
    */
   virtual int getPhoneId() = 0;

   /**
    *  To check if call is in multi party call(conference) or not
    *
    * On platforms with access control enabled, the caller needs to have TELUX_TEL_CALL_INFO_READ
    * permission to successfully invoke this API.
    *
    * @returns True if call is in conference otherwise false.
    *
    */
   virtual bool isMultiPartyCall() = 0;

   /**
    * Retrieves the RTT mode of the call.
    *
    * On platforms with access control enabled, the caller needs to have TELUX_TEL_CALL_INFO_READ
    * permission to successfully invoke this API.
    *
    * @returns RttMode - enumeration representing RTT mode @ref telux::tel::RttMode
    *
    * @note Eval: This is a new API and is being evaluated. It is subject to change and
    *             could break backwards compatibility.
    */
   virtual RttMode getRttMode() = 0;

   /**
    * Retrieves the local RTT capability of the call, indicating whether a local
    * device can support a RTT call. Users can upgrade to a RTT call using
    * @ref telux::tel::ICall::modify.
    *
    * On platforms with access control enabled, the caller needs to have TELUX_TEL_CALL_INFO_READ
    * permission to successfully invoke this API.
    *
    * @returns RttMode - enumeration representing RTT mode @ref telux::tel::RttMode
    *
    * @note Eval: This is a new API and is being evaluated. It is subject to change and
    *             could break backwards compatibility.
    */
   virtual RttMode getLocalRttCapability() = 0;

   /**
    * Retrieves the peer RTT capability of the call, indicating whether a peer device
    * can support a RTT call.
    *
    * On platforms with access control enabled, the caller needs to have TELUX_TEL_CALL_INFO_READ
    * permission to successfully invoke this API.
    *
    * @returns RttMode - enumeration representing RTT mode @ref telux::tel::RttMode
    *
    * @note Eval: This is a new API and is being evaluated. It is subject to change and
    *             could break backwards compatibility.
    */
   virtual RttMode getPeerRttCapability() = 0;

   /**
    * Request to upgrade the call from a normal voice call to a RTT call or downgrade the call from
    * a RTT call to a normal voice call.
    *
    * On platforms with access control enabled, the caller needs to have TELUX_TEL_CALL_MGMT
    * permission to successfully invoke this API.
    *
    * @param [in] mode -     Parameter to send desired real time text mode for a call
    *                        @ref telux::tel::RttMode::DISABLED, to send request to
    *                             downgrade a RTT call to normal voice call.
    *                        @ref telux::tel::RttMode::FULL, to send upgrade normal voice
    *                             call to a RTT call.
    * @param [in] callback - optional callback pointer to get the response of modify
    * request below are possible error codes for callback response
    *        - @ref telux::common::ErrorCode::SUCCESS
    *        - @ref telux::common::ErrorCode::MODEM_ERR
    *        - @ref telux::common::ErrorCode::GENERIC_FAILURE
    *
    * @returns Status of modify i.e. success or suitable error code.
    *
    * @note Eval: This is a new API and is being evaluated. It is subject to change and
    *             could break backwards compatibility.
    */
   virtual telux::common::Status
      modify(RttMode mode,
        std::shared_ptr<telux::common::ICommandResponseCallback> callback = nullptr)
      = 0;

   /**
    *
    * Accepts or rejects a modify call request triggered by a remote party after the user receives
    * a notification to modify the call using @ref ICallListener::onModifyCallRequest.
    *
    * On platforms with access control enabled, the caller needs to have TELUX_TEL_CALL_MGMT
    * permission to successfully invoke this API.
    *
    *
    * @param [in] modifyResponseType -  Send the response to accept or reject the modify request
    *                                   Accept corresponds to true and reject corresponds to false
    * @param [in] callback -            Optional, callback pointer to get the response of
    *                                   the respondToModifyRequest request.
    *
    * Below are possible error codes for callback response
    *        - @ref telux::common::ErrorCode::SUCCESS
    *        - @ref telux::common::ErrorCode::MODEM_ERR
    *        - @ref telux::common::ErrorCode::GENERIC_FAILURE
    *
    * @returns Status of respondToModifyRequest i.e. success or suitable error code.
    *
    * @note Eval: This is a new API and is being evaluated. It is subject to change and
    *             could break backwards compatibility.
    */
   virtual telux::common::Status
      respondToModifyRequest(bool modifyResponseType,
        std::shared_ptr<telux::common::ICommandResponseCallback> callback = nullptr)
      = 0;

   /**
    * Gets the current type of the call, such as emergency call, voice call, etc.
    *
    * On platforms with access control enabled, the caller needs to have TELUX_TEL_CALL_INFO_READ
    * permission to successfully invoke this API.
    *
    * @returns CallType - enumeration representing call type @ref telux::tel::CallType
    *
    * @note   Eval: This is a new API and is being evaluated. It is subject to
    *         change and could break backwards compatibility.
    */
   virtual CallType getCallType() = 0;

   virtual ~ICall() {
   }
};
/** @} */ /* end_addtogroup telematics_call */

}  // End of namespace tel

}  // End of namespace telux

#endif // TELUX_TEL_CALL_HPP
