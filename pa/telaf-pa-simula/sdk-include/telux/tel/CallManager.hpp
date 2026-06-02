/*
 *  Copyright (c) 2017-2018,2020-2021 The Linux Foundation. All rights reserved.
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
 * @file  CallManager.hpp
 * @brief Call Manager is the primary interface for performing call related
 *        operations. Allows to conference calls, swap calls, make normal
 *        voice call and emergency call, send and update MSD pdu. Registers
 *        the listener and notify about incoming call, call info change and
 *        eCall MSD transmission status change to listener.
 *
 */

#ifndef TELUX_TEL_CALLMANAGER_HPP
#define TELUX_TEL_CALLMANAGER_HPP

#include <memory>
#include <string>
#include <vector>

#include <telux/tel/Call.hpp>
#include <telux/tel/CallListener.hpp>
#include <telux/tel/ECallDefines.hpp>
#include <telux/tel/PhoneDefines.hpp>

#include <telux/common/CommonDefines.hpp>

namespace telux {

namespace tel {

/** @addtogroup telematics_call
 * @{ */

class IMakeCallCallback;

/**
 * This function is called with the response to make normal call and
 * emergency call.
 *
 * The callback can be invoked from multiple different threads.
 * The implementation should be thread safe.
 *
 * @param [out] error  @ref ErrorCode
 * @param [out] call   Pointer to Call object or nullptr in case of failure
 *
 */
using MakeCallCallback
   = std::function<void(telux::common::ErrorCode error, std::shared_ptr<ICall> call)>;

/**
 * This function is called with response to request for eCall High Level Application Protocol(HLAP)
 * timers status.
 *
 * The callback can be invoked from multiple different threads.
 * The implementation should be thread safe.
 *
 * @param [out] error         @ref ErrorCode
 * @param [out] phoneId       Represents the phone corresponding to which the response is being
 *                            reported.
 * @param [out] timersStatus  @ref ECallHlapTimerStatus
 *
 */
using ECallHlapTimerStatusCallback = std::function<void(telux::common::ErrorCode error, int phoneId,
                                                        ECallHlapTimerStatus timersStatus)>;

/**
 * This function is called with response to request for ECBM(requestEcbm API).
 *
 * The callback can be invoked from multiple different threads.
 * The implementation should be thread safe.
 *
 * @param [out] ecbMode       Indicates the status of the ECBM.
 *                            @ref EcbMode
 * @param [out] error         @ref ErrorCode
 *
 */
using EcbmStatusCallback
    = std::function<void(telux::tel::EcbMode ecbMode, telux::common::ErrorCode error)>;

/**
 * This function is called with the response to request for the HLAP timer configuration.
 *
 * The callback can be invoked from multiple different threads.
 * The implementation should be thread safe.
 *
 * @param [out] error         @ref ErrorCode
 * @param [out] timeDuration  Represents the time duration for the HLAP timer.
 *
 */
using ECallHlapTimerCallback
   = std::function<void(telux::common::ErrorCode error, uint32_t timeDuration)>;

/**
 * @brief Call Manager is the primary interface for call related operations
 *        Allows to conference calls, swap calls, make normal voice call and
 *        emergency call, send and update MSD pdu.
 */
class ICallManager {
public:
   /**
    * This status indicates whether the ICallManager object is in a usable state.
    *
    * @returns SERVICE_AVAILABLE    -  If CallManager is ready for service.
    *          SERVICE_UNAVAILABLE  -  If CallManager is temporarily unavailable.
    *          SERVICE_FAILED       -  If CallManager encountered an irrecoverable failure.
    *
    */
   virtual telux::common::ServiceStatus getServiceStatus() = 0;

   /**
    * Initiate a voice call. This API can also be used for e911/e112 type of regular emergency call.
    * This is not meant for an automotive eCall.
    * Regular voice call will be blocked by device while eCall is in progress.
    *
    * On platforms with access control enabled, the caller needs to have TELUX_TEL_CALL_MGMT
    * permission to successfully invoke this API.
    *
    * @param [in] phoneId      Represents phone corresponding to which on make
    *                          call operation is performed
    * @param [in] dialNumber   String representing the dialing number
    * @param [in] callback     Optional callback pointer to get the response of
    *                          makeCall request.
    *                          Possible(not exhaustive) error codes for callback response
    *                          - @ref telux::common::ErrorCode::SUCCESS
    *                          - @ref telux::common::ErrorCode::RADIO_NOT_AVAILABLE
    *                          - @ref telux::common::ErrorCode::DIAL_MODIFIED_TO_USSD
    *                          - @ref telux::common::ErrorCode::DIAL_MODIFIED_TO_SS
    *                          - @ref telux::common::ErrorCode::DIAL_MODIFIED_TO_DIAL
    *                          - @ref telux::common::ErrorCode::INVALID_ARGUMENTS
    *                          - @ref telux::common::ErrorCode::NO_MEMORY
    *                          - @ref telux::common::ErrorCode::INVALID_STATE
    *                          - @ref telux::common::ErrorCode::NO_RESOURCES
    *                          - @ref telux::common::ErrorCode::INTERNAL_ERR
    *                          - @ref telux::common::ErrorCode::FDN_CHECK_FAILURE
    *                          - @ref telux::common::ErrorCode::MODEM_ERR
    *                          - @ref telux::common::ErrorCode::NO_SUBSCRIPTION
    *                          - @ref telux::common::ErrorCode::NO_NETWORK_FOUND
    *                          - @ref telux::common::ErrorCode::INVALID_CALL_ID
    *                          - @ref telux::common::ErrorCode::DEVICE_IN_USE
    *                          - @ref telux::common::ErrorCode::MODE_NOT_SUPPORTED
    *                          - @ref telux::common::ErrorCode::ABORTED
    *                          - @ref telux::common::ErrorCode::GENERIC_FAILURE
    *
    * @returns Status of makeCall i.e. success or suitable status code.
    */
   virtual telux::common::Status makeCall(int phoneId, const std::string &dialNumber,
                                          std::shared_ptr<IMakeCallCallback> callback = nullptr)
      = 0;
  /**
    * Initiate a real time text (RTT) voice call. This API can also be used for e911/e112 emergency
    * calls. This is not meant to originate an automotive eCall.
    * During an ongoing eCall, regular RTT voice calls cannot be originated by the device.
    * To enable RTT calls, the RTT service must be enabled first using the
    * @ref telux::tel::IImsSettingsManager::setServiceConfig method.
    *
    * On platforms with access control enabled, the caller needs to have TELUX_TEL_CALL_MGMT
    * permission to successfully invoke this API.
    *
    * @param [in] phoneId      Represents the phone corresponding to which the
    *                          call operation is performed
    * @param [in] dialNumber   String representing the dialing number
    * @param [in] callback     Optional callback pointer to get the response of
    *                          makeRttCall request.
    *                          Possible(not exhaustive) error codes for callback response
    *                          - @ref telux::common::ErrorCode::SUCCESS
    *                          - @ref telux::common::ErrorCode::RADIO_NOT_AVAILABLE
    *                          - @ref telux::common::ErrorCode::DIAL_MODIFIED_TO_USSD
    *                          - @ref telux::common::ErrorCode::DIAL_MODIFIED_TO_SS
    *                          - @ref telux::common::ErrorCode::DIAL_MODIFIED_TO_DIAL
    *                          - @ref telux::common::ErrorCode::INVALID_ARGUMENTS
    *                          - @ref telux::common::ErrorCode::NO_MEMORY
    *                          - @ref telux::common::ErrorCode::INVALID_STATE
    *                          - @ref telux::common::ErrorCode::NO_RESOURCES
    *                          - @ref telux::common::ErrorCode::INTERNAL_ERR
    *                          - @ref telux::common::ErrorCode::FDN_CHECK_FAILURE
    *                          - @ref telux::common::ErrorCode::MODEM_ERR
    *                          - @ref telux::common::ErrorCode::NO_SUBSCRIPTION
    *                          - @ref telux::common::ErrorCode::NO_NETWORK_FOUND
    *                          - @ref telux::common::ErrorCode::INVALID_CALL_ID
    *                          - @ref telux::common::ErrorCode::DEVICE_IN_USE
    *                          - @ref telux::common::ErrorCode::MODE_NOT_SUPPORTED
    *                          - @ref telux::common::ErrorCode::ABORTED
    *                          - @ref telux::common::ErrorCode::GENERIC_FAILURE
    *
    * @returns Status of makeRttCall i.e. success or suitable status code.
    *
    * @note    Eval: This is a new API and is being evaluated. It is subject to change and
    *          could break backward compatibility.
    */
   virtual telux::common::Status makeRttCall(int phoneId, const std::string &dialNumber,
                                          std::shared_ptr<IMakeCallCallback> callback = nullptr)
      = 0;

   /**
    * Initiate an European(EU) or ERA-GLONASS automotive eCall.
    * Regular voice calls will be blocked by device while eCall is in progress.
    * MSD encoding for optional ERA-GLONASS additional data is not supported as per spec
    * GOST R 54620 / GOST R 33464.
    *
    * On platforms with Access control enabled, Caller needs to have TELUX_TEL_ECALL_MGMT permission
    * to invoke this API successfully.
    *
    * @param [in] phoneId      Represents phone corresponding to which make
    *                          eCall operation is performed
    * @param [in] eCallMsdData The structure containing required fields to
    *                          create eCall Minimum Set of Data (MSD)
    * @param [in] category     @ref ECallCategory
    * @param [in] variant      @ref ECallVariant
    * @param [in] callback     Optional callback pointer to get the response of
    *                          makeECall request.
    *                          Possible(not exhaustive) error codes for callback response
    *                          - @ref telux::common::ErrorCode::SUCCESS
    *                          - @ref telux::common::ErrorCode::RADIO_NOT_AVAILABLE
    *                          - @ref telux::common::ErrorCode::NO_MEMORY
    *                          - @ref telux::common::ErrorCode::MODEM_ERR
    *                          - @ref telux::common::ErrorCode::INTERNAL_ERR
    *                          - @ref telux::common::ErrorCode::INVALID_STATE
    *                          - @ref telux::common::ErrorCode::INVALID_CALL_ID
    *                          - @ref telux::common::ErrorCode::INVALID_ARGUMENTS
    *                          - @ref telux::common::ErrorCode::OPERATION_NOT_ALLOWED
    *                          - @ref telux::common::ErrorCode::GENERIC_FAILURE
    *
    * @returns Status of makeECall i.e. success or suitable status code.
    */
   virtual telux::common::Status makeECall(int phoneId, const ECallMsdData &eCallMsdData,
                                           int category, int variant,
                                           std::shared_ptr<IMakeCallCallback> callback = nullptr)
      = 0;

   /**
    * Initiate an automotive Third Party Service(TPS) eCall over CS based RAT only
    * (i.e. not IMS), to the specified phone number with Minimum Set of Data(MSD) at call connect.
    * It will be treated like a regular voice call by the UE and the network.
    * During this request, if the device was registered over a PS based RAT, it will attempt to
    * fallback to a CS based RAT. If this attempt fails, the call will end with a failure.
    *
    * It is the responsibility of application to make sure that another call is not dialed while
    * Third Party Service eCall is in progress.
    *
    * On platforms with Access control enabled, Caller needs to have TELUX_TEL_ECALL_MGMT
    * permission to invoke this API successfully.
    *
    * @param [in] phoneId      Represents phone corresponding to which make
    *                          eCall operation is performed
    * @param [in] dialNumber   String representing the dialing number
    * @param [in] eCallMsdData The structure containing required fields to
    *                          create eCall Minimum Set of Data (MSD)
    * @param [in] category     @ref ECallCategory
    * @param [in] callback     Optional callback pointer to get the response of
    *                          makeECall request.
    *                          Possible(not exhaustive) error codes for callback response
    *                          - @ref telux::common::ErrorCode::SUCCESS
    *                          - @ref telux::common::ErrorCode::RADIO_NOT_AVAILABLE
    *                          - @ref telux::common::ErrorCode::NO_MEMORY
    *                          - @ref telux::common::ErrorCode::MODEM_ERR
    *                          - @ref telux::common::ErrorCode::INTERNAL_ERR
    *                          - @ref telux::common::ErrorCode::INVALID_STATE
    *                          - @ref telux::common::ErrorCode::INVALID_CALL_ID
    *                          - @ref telux::common::ErrorCode::INVALID_ARGUMENTS
    *                          - @ref telux::common::ErrorCode::OPERATION_NOT_ALLOWED
    *                          - @ref telux::common::ErrorCode::GENERIC_FAILURE
    *
    * @returns Status of makeECall i.e. success or suitable status code.
    *
    */
   virtual telux::common::Status makeECall(int phoneId, const std::string dialNumber,
                                           const ECallMsdData &eCallMsdData, int category,
                                           std::shared_ptr<IMakeCallCallback> callback = nullptr)
      = 0;
   /**
    * Initiate an automotive Third Party Service(TPS) eCall over IMS only, to the specified phone
    * number with Minimum Set of Data(MSD) at call connect. It will be treated like a regular voice
    * call over IMS by the UE and the network.
    * During this request, if the device was not registered over IMS for voice service, the request
    * will fail.
    *
    * Application is expected to dial only one Third Party Service eCall per subscription.
    * It is the responsibility of application to make sure that another call is not dialed while
    * Third Party Service eCall is in progress.
    *
    * On platforms with access control enabled, the caller needs to have TELUX_TEL_ECALL_MGMT
    * permission to successfully invoke this API.
    *
    * @param [in] phoneId      Represents phone corresponding to which make
    *                          eCall operation is performed
    * @param [in] dialNumber   String representing the dialing number
    * @param [in] msdPdu       Encoded MSD(Minimum Set of Data) PDU as per spec EN
    *                          15722 2015 or GOST R 54620-2011/33464-2015
    *                          Max size 255 bytes
    * @param [in] header       Optional SIP headers intended to be sent in the
    *                          SIP invite message to the network for PSAP
    *                          - @ref telux::tel::CustomSipHeader
    * @param [in] callback     Optional callback function to get the response of
    *                          makeECall request.
    *
    * @returns Status of makeECall i.e. success or suitable status code.
    *
    */
   virtual telux::common::Status makeECall(int phoneId, const std::string dialNumber,
      const std::vector<uint8_t> &msdPdu,
      CustomSipHeader header = {telux::tel::CONTENT_HEADER,""},
      MakeCallCallback callback = nullptr) = 0;
   /**
    * Initiate an European(EU) or ERA-GLONASS automotive eCall with raw MSD pdu.
    * Regular voice calls will be blocked by device while eCall is in progress.
    * MSD encoding for optional ERA-GLONASS additional data is not supported as per spec
    * GOST R 54620 / GOST R 33464.
    *
    * On platforms with access control enabled, the caller needs to have TELUX_TEL_ECALL_MGMT
    * permission to successfully invoke this API.
    *
    * @param [in] phoneId   Represents phone corresponding to which on make eCall
    *                       operation is performed
    * @param [in] msdPdu    Encoded MSD(Minimum Set of Data) PDU as per spec EN
    *                       15722 2015 or GOST R 54620-2011/33464-2015
    * @param [in] category  @ref ECallCategory
    * @param [in] variant   @ref ECallVariant
    * @param [in] callback  Callback function to get the response of makeECall
    *                       request.
    *                       Possible(not exhaustive) error codes for callback response
    *                       - @ref telux::common::ErrorCode::SUCCESS
    *                       - @ref telux::common::ErrorCode::RADIO_NOT_AVAILABLE
    *                       - @ref telux::common::ErrorCode::NO_MEMORY
    *                       - @ref telux::common::ErrorCode::MODEM_ERR
    *                       - @ref telux::common::ErrorCode::INTERNAL_ERR
    *                       - @ref telux::common::ErrorCode::INVALID_STATE
    *                       - @ref telux::common::ErrorCode::INVALID_CALL_ID
    *                       - @ref telux::common::ErrorCode::INVALID_ARGUMENTS
    *                       - @ref telux::common::ErrorCode::OPERATION_NOT_ALLOWED
    *                       - @ref telux::common::ErrorCode::GENERIC_FAILURE
    *
    * @returns Status of makeECall i.e. success or suitable status code.
    */
   virtual telux::common::Status makeECall(int phoneId, const std::vector<uint8_t> &msdPdu,
                                           int category, int variant,
                                           MakeCallCallback callback = nullptr)
      = 0;

   /**
    * Initiate an automotive eCall with raw MSD pdu, to the specified phone number for TPS eCall
    * over CS based RAT only (i.e. not IMS).
    * It will be treated like a regular voice call by the UE and the network.
    * During this request, if the device was registered over a PS based RAT, it will attempt to
    * fallback to a CS based RAT. If this attempt fails, the call will end with a failure.
    *
    * It is the responsibility of application to make sure that another call is not dialed while
    * Third Party Service eCall is in progress.
    *
    * On platforms with access control enabled, the caller needs to have TELUX_TEL_ECALL_MGMT
    * permission to successfully invoke this API.
    *
    * @param [in] phoneId   Represents phone corresponding to which on make eCall
    *                       operation is performed
    * @param [in] dialNumber   String representing the dialing number
    * @param [in] msdPdu    Encoded MSD(Minimum Set of Data) PDU as per spec EN
    *                       15722 2015 or GOST R 54620-2011/33464-2015
    * @param [in] category  @ref ECallCategory
    * @param [in] callback  Callback function to get the response of makeECall
    *                       request.
    *                       Possible(not exhaustive) error codes for callback response
    *                       - @ref telux::common::ErrorCode::SUCCESS
    *                       - @ref telux::common::ErrorCode::RADIO_NOT_AVAILABLE
    *                       - @ref telux::common::ErrorCode::NO_MEMORY
    *                       - @ref telux::common::ErrorCode::MODEM_ERR
    *                       - @ref telux::common::ErrorCode::INTERNAL_ERR
    *                       - @ref telux::common::ErrorCode::INVALID_STATE
    *                       - @ref telux::common::ErrorCode::INVALID_CALL_ID
    *                       - @ref telux::common::ErrorCode::INVALID_ARGUMENTS
    *                       - @ref telux::common::ErrorCode::OPERATION_NOT_ALLOWED
    *                       - @ref telux::common::ErrorCode::GENERIC_FAILURE
    *
    * @returns Status of makeECall i.e. success or suitable status code.
    *
    */
   virtual telux::common::Status makeECall(int phoneId, const std::string dialNumber,
                                           const std::vector<uint8_t> &msdPdu, int category,
                                           MakeCallCallback callback = nullptr)
      = 0;

   /**
    * Initiate an European(EU) or ERA-GLONASS automotive eCall without transmitting
    * Minimum Set of Data (MSD) at call connect.
    * Regular voice calls will be blocked by device while eCall is in progress.
    * MSD encoding for optional ERA-GLONASS additional data is not supported as per spec
    * GOST R 54620 / GOST R 33464.
    *
    * On platforms with access control enabled, the caller needs to have TELUX_TEL_ECALL_MGMT
    * permission to successfully invoke this API.
    *
    * @param [in] phoneId      Represents phone corresponding to which make
    *                          eCall operation is performed
    * @param [in] category     @ref ECallCategory
    * @param [in] variant      @ref ECallVariant
    * @param [in] callback     Optional callback function to get the response of
    *                          makeECall request.
    *                          Possible(not exhaustive) error codes for callback response
    *                          - @ref telux::common::ErrorCode::SUCCESS
    *                          - @ref telux::common::ErrorCode::RADIO_NOT_AVAILABLE
    *                          - @ref telux::common::ErrorCode::NO_MEMORY
    *                          - @ref telux::common::ErrorCode::MODEM_ERR
    *                          - @ref telux::common::ErrorCode::INTERNAL_ERR
    *                          - @ref telux::common::ErrorCode::INVALID_STATE
    *                          - @ref telux::common::ErrorCode::INVALID_CALL_ID
    *                          - @ref telux::common::ErrorCode::INVALID_ARGUMENTS
    *                          - @ref telux::common::ErrorCode::OPERATION_NOT_ALLOWED
    *                          - @ref telux::common::ErrorCode::GENERIC_FAILURE
    *
    * @returns Status of makeECall i.e. success or suitable status code.
    */
   virtual telux::common::Status makeECall(int phoneId, int category, int variant,
                                           MakeCallCallback callback = nullptr)
      = 0;

   /**
    * Initiate an automotive eCall to the specified phone number for TPS eCall over CS
    * based RAT only (i.e. not IMS), without transmitting Minimum Set of Data(MSD) at call
    * connect.
    * It will be treated like a regular voice call by the UE and the network.
    * During this request, if the device was registered over a PS based RAT, it will attempt to
    * fallback to a CS based RAT. If this attempt fails, the call will end with a failure.
    *
    * It is the responsibility of application to make sure that another call is not dialed while
    * Third Party Service eCall is in progress.
    *
    * On platforms with access control enabled, the caller needs to have TELUX_TEL_ECALL_MGMT
    * permission to successfully invoke this API.
    *
    * @param [in] phoneId      Represents phone corresponding to which make
    *                          eCall operation is performed
    * @param [in] dialNumber   String representing the dialing number
    * @param [in] category     @ref ECallCategory
    * @param [in] callback     Optional callback function to get the response of
    *                          makeECall request.
    *                          Possible(not exhaustive) error codes for callback response
    *                          - @ref telux::common::ErrorCode::SUCCESS
    *                          - @ref telux::common::ErrorCode::RADIO_NOT_AVAILABLE
    *                          - @ref telux::common::ErrorCode::NO_MEMORY
    *                          - @ref telux::common::ErrorCode::MODEM_ERR
    *                          - @ref telux::common::ErrorCode::INTERNAL_ERR
    *                          - @ref telux::common::ErrorCode::INVALID_STATE
    *                          - @ref telux::common::ErrorCode::INVALID_CALL_ID
    *                          - @ref telux::common::ErrorCode::INVALID_ARGUMENTS
    *                          - @ref telux::common::ErrorCode::OPERATION_NOT_ALLOWED
    *                          - @ref telux::common::ErrorCode::GENERIC_FAILURE
    *
    * @returns Status of makeECall i.e. success or suitable status code.
    */
   virtual telux::common::Status makeECall(int phoneId, const std::string dialNumber, int category,
                                           MakeCallCallback callback = nullptr)
      = 0;

   /**
    * Update the eCall MSD in modem to be sent to Public Safety Answering Point
    * (PSAP) when requested.
    *
    * On platforms with access control enabled, the caller needs to have TELUX_TEL_ECALL_MGMT
    * permission to successfully invoke this API.
    *
    * @param [in] phoneId   Represents phone corresponding to which
    *                       updateECallMsd operation is performed
    * @param [in] eCallMsd  The data structure represents the Minimum Set of Data
    *                       (MSD)
    * @param [in] callback  Optional callback pointer to get the response of
    *                       updateECallMsd.
    *
    * @returns Status of updateECallMsd i.e. success or suitable error code.
    */
   virtual telux::common::Status
      updateECallMsd(int phoneId, const ECallMsdData &eCallMsd,
                     std::shared_ptr<telux::common::ICommandResponseCallback> callback = nullptr)
      = 0;

   /**
    * This API could be used to explicitly send MSD to PSAP in response to MSD pull request
    * from the PSAP.The modem will not automatically update MSD to the Public Safety Answering
    * Point(PSAP) @ref- telux::tel::ICallListener::OnMsdUpdateRequest.
    *
    * On platforms with access control enabled, the caller needs to have TELUX_TEL_ECALL_MGMT
    * permission to successfully invoke this API.
    *
    * @param [in] phoneId   Represents phone corresponding to which
    *                       updateECallMsd operation is performed
    * @param [in] msdPdu    Encoded MSD(Minimum Set of Data) PDU as per spec EN
    *                       15722 2015 or GOST R 54620-2011/33464-2015
    *                       For Third Party Service(TPS) eCall over IMS technology:
    *                       Maximum length allowed for MSD is 255 bytes
    *                       For all other types of eCall:
    *                       Maximum length allowed for MSD is 140 bytes
    * @param [in] callback  Callback function to get the response of
    *                       updateECallMsd.
    *
    * @returns Status of updateECallMsd i.e. success or suitable error code.
    */
   virtual telux::common::Status updateECallMsd(int phoneId, const std::vector<uint8_t> &msdPdu,
                                                telux::common::ResponseCallback callback)
      = 0;

   /**
    * Request for status of eCall High Level Application Protocol(HLAP) timers that are maintained
    * by the UE state machine. This does not retrieve status of timers maintained by the PSAP.
    * The provided timers are as per EN 16062:2015 standard.
    *
    * On platforms with access control enabled, the caller needs to have TELUX_TEL_ECALL_MGMT
    * permission to successfully invoke this API.
    *
    * @param [in] phoneId   Represents phone corresponding on which requestECallHlapTimerStatus
    *                       operation is performed
    * @param [in] callback  Callback function to get the response of requestECallHlapTimerStatus
    *
    * @returns Status of requestECallHlapTimerStatus i.e. success or suitable error code.
    *
    */
   virtual telux::common::Status requestECallHlapTimerStatus(int phoneId,
                                                         ECallHlapTimerStatusCallback callback) = 0;

   /**
    * Get in-progress calls.
    *
    * On platforms with access control enabled, the caller needs to have TELUX_TEL_CALL_INFO_READ
    * permission to successfully invoke this API.
    *
    * @returns List of active calls.
    */
   virtual std::vector<std::shared_ptr<ICall>> getInProgressCalls() = 0;

   /**
    * Merge two calls in a conference.
    *
    * On platforms with access control enabled, the caller needs to have TELUX_TEL_CALL_MGMT
    * permission to successfully invoke this API.
    *
    * @param [in] call1     Call object to conference.
    * @param [in] call2     Call object to conference.
    * @param [in] callback  Optional callback pointer to get the result of
    *                       conference function
    *
    * @returns Status of conference i.e. success or suitable error code.
    */
   virtual telux::common::Status
      conference(std::shared_ptr<ICall> call1, std::shared_ptr<ICall> call2,
                 std::shared_ptr<telux::common::ICommandResponseCallback> callback = nullptr)
      = 0;

   /**
    * Swap calls to make one active and put the another on hold.
    *
    * On platforms with access control enabled, the caller needs to have TELUX_TEL_CALL_MGMT
    * permission to successfully invoke this API.
    *
    * @param [in] callToHold      Active call object to swap to hold state.
    * @param [in] callToActivate  Hold call object to swap to active state.
    * @param [in] callback        Optional callback pointer to get the result of
    *                             swap function
    *
    * @returns Status of swap i.e. success or suitable error code.
    */
   virtual telux::common::Status
      swap(std::shared_ptr<ICall> callToHold, std::shared_ptr<ICall> callToActivate,
           std::shared_ptr<telux::common::ICommandResponseCallback> callback = nullptr)
      = 0;

   /**
    * Hangup all the foreground call(s) if any and accept the background call as the active call.
    * The foreground call here could be active call, incoming call or multiple active calls in case
    * of conference and background call could be held call or waiting call.
    *
    * If a call(s) is active, the active call(s) will be terminated or if a call is waiting, the
    * waiting call will be accepted and becomes active.  Otherwise, if a held call is present, the
    * held call becomes active.
    * In case of hold and waiting calls, the hold call will still be on hold and waiting call will
    * be accepted.
    * In case of hold, active and waiting scenario, the hold call will still be on hold, active
    * call will be ended and waiting call will be accepted.
    * Answering a waiting RTT call during the above scenarios is not supported.
    *
    * On platforms with access control enabled, the caller needs to have TELUX_TEL_CALL_MGMT
    * permission to successfully invoke this API.
    *
    * @param [in] phoneId - Represents phone corresponding to which this operation is performed.
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
    * @returns Status of hangupForegroundResumeBackground i.e. success or suitable error code.
    *
    */
   virtual telux::common::Status hangupForegroundResumeBackground(int phoneId,
      common::ResponseCallback callback = nullptr)
      = 0;

   /**
    * Hangup all the waiting or background call(s).
    * The background call here could be waiting call, hold call or multiple hold calls in case
    * of conference.
    *
    * If a call(s) is hold, the hold call(s) will be terminated or if a call is waiting, the
    * waiting call will be be terminated as well.
    * In case of hold, active and waiting scenario, the active call will still be on active, hold
    * and waiting call will be ended.
    *
    * On platforms with access control enabled, the caller needs to have TELUX_TEL_CALL_MGMT
    * permission to successfully invoke this API.
    *
    * @param [in] phoneId - Represents phone corresponding to which this operation is performed.
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
    * @returns Status of hangupWaitingOrBackground i.e. success or suitable error code.
    *
    */
   virtual telux::common::Status hangupWaitingOrBackground(int phoneId,
      common::ResponseCallback callback = nullptr)
      = 0;

   /**
    * Request for emergency callback mode
    *
    * @param [in] phoneId      Represents the phone corresponding to which the emergency callback
    *                          mode(ECBM) status is requested.
    * @param [in] callback     Callback pointer to get the result of ECBM status request
    *
    * @returns Status of requestEcbm i.e. success or suitable error code.
    *
    */
   virtual telux::common::Status requestEcbm(int phoneId, EcbmStatusCallback callback) = 0;

   /**
    * Exit emergency callback mode.
    *
    * On platforms with access control enabled, the caller needs to have TELUX_TEL_EMERGENCY_OPS
    * permission to successfully invoke this API.
    *
    * @param [in] phoneId      Represents the phone corresponding to which the emergency callback
    *                          mode(ECBM) exit is requested.
    * @param [in] callback      Optional callback pointer to get the result of exit ECBM request
    *
    * @returns Status of exitEcbm i.e. success or suitable error code.
    *
    */
   virtual telux::common::Status exitEcbm(int phoneId, common::ResponseCallback callback = nullptr)
      = 0;

   /**
    * Deregister from the network after an eCall when the modem is in eCall-only mode.
    * This is typically done after the T9 eCall HLAP timer has expired to stop the T10 eCall HLAP
    * timer and deregister from the serving network.
    *
    * On platforms with access control enabled, the caller needs to have TELUX_TEL_ECALL_MGMT
    * permission to successfully invoke this API.
    *
    * @param [in] phoneId   Represents the phone corresponding to which the network deregistration
    *                       will be performed.
    * @param [in] callback  Callback function to get the response of the request. The response is
    *                       sent after the operation is complete.
    *
    * @returns Status of requestNetworkDeregistration request, i.e., success or suitable error code.
    *
    */
    virtual telux::common::Status requestNetworkDeregistration(int phoneId,
        common::ResponseCallback callback = nullptr) = 0;

   /**
    * Set the value of an eCall HLAP timer.
    * Only the T10 Timer is supported currently.
    *
    * On platforms with access control enabled, the caller needs to have TELUX_TEL_ECALL_MGMT
    * permission to successfully invoke this API.
    *
    * @param [in] phoneId      Represents the phone corresponding to which the value of T10 eCall
    *                          HLAP timer updated will be performed.
    * @param [in] type         @ref HlapTimerType
    * @param [in] timeDuration Represents the time duration for the HLAP timer.
    *                          T10 timer is in units of minutes, and the supported range is from
    *                          60 to 720.
    * @param [in] callback     Callback function to get the response of the request. The response is
    *                          sent after the operation is complete.
    *
    * @returns Status of updateEcallHlapTimer i.e., success or suitable error code.
    *
    */
   virtual telux::common::Status updateEcallHlapTimer(int phoneId, HlapTimerType type,
       uint32_t timeDuration, common::ResponseCallback callback = nullptr) = 0;

   /**
    * Get the value of an eCall HLAP timer.
    * Only the T10 Timer is supported currently.
    *
    * On platforms with access control enabled, the caller needs to have TELUX_TEL_ECALL_MGMT
    * permission to successfully invoke this API
    *
    * @param [in] phoneId      Represents the phone corresponding to which the value of eCall HLAP
    *                          timer query will be performed.
    * @param [in] type         @ref HlapTimerType
    * @param [in] callback     Callback function to get the response of the request. The response is
    *                          sent after the operation is complete.
    *
    * @returns Status of requestEcallHlapTimer i.e., success or suitable error code.
    *
    */
   virtual telux::common::Status requestEcallHlapTimer(int phoneId, HlapTimerType type,
       ECallHlapTimerCallback callback) = 0;

   /**
    * Set the configuration related to emergency call.
    * The configuration is persistent and takes effect when the next emergency call is dialed.
    *
    * Minimum value of EcallConfig.t9Timer value should be 3600000. If a lesser value is provided,
    * this API will still succeed but the actual value would be set to 3600000.
    *
    * On platforms with access control enabled, the caller needs to have TELUX_TEL_ECALL_MGMT
    * permission to successfully invoke this API
    *
    * @param [in] config   eCall configuration to be set
    *                      @ref EcallConfig
    *
    * @returns Status of setECallConfig i.e. success or suitable error code.
    *
    */
   virtual telux::common::Status setECallConfig(EcallConfig config) = 0;

   /**
    * Get the configuration related to emergency call.
    *
    * On platforms with access control enabled, the caller needs to have TELUX_TEL_ECALL_MGMT
    * permission to successfully invoke this API.
    *
    * @param [out] config   Parameter to hold the fetched eCall configuration
    *                       @ref EcallConfig
    *
    * @returns Status of getECallConfig i.e. success or suitable error code.
    *
    */
   virtual telux::common::Status getECallConfig(EcallConfig &config) = 0;

   /**
    * Gets encoded bytes of eCall MSD according to EN 15722:2015 (MSD version 2) and
    * EN 15722:2020 (MSD version 3).
    *
    * @param [in] eCallMsdData   eCall MSD data. @ref telux::tel::ECallMsdData
    * @param [out] data          Encoded bytes of eCall MSD.
    *
    * @returns error code for encodeECallMsd i.e. success or suitable error code.
    * Below are possible error codes.
    *        - @ref telux::common::ErrorCode::SUCCESS
    *        - @ref telux::common::ErrorCode::INVALID_ARGUMENTS
    *        - @ref telux::common::ErrorCode::GENERIC_FAILURE
    *
    * @note Eval: This is a new API and is being evaluated. It is subject to change and
    *             could break backwards compatibility.
    */
   virtual telux::common::ErrorCode encodeECallMsd(telux::tel::ECallMsdData eCallMsdData,
       std::vector<uint8_t> &data) = 0;

   /**
    * Gets encoded bytes of optional additional data content as per the Euro NCAP Technical
    * Bulletin TB 040. Client needs to pass this vector of bytes to the data field of the
    * ECallOptionalPdu. @ref telux::tel::ECallOptionalPdu::data
    *
    * @param [in] optionalEuroNcapData   ECall optional additional data as per Euro NCAP
    *                                    Technical Bulletin TB 040.
                                         @ref telux::tel::ECallOptionalEuroNcapData
    * @param [out] data                  Encoded optional additional data.
    *
    * @returns Status of encodeEuroNcapOptionalAdditionalData i.e. success or
    * suitable error code.
    *
    * @note Eval: This is a new API and is being evaluated. It is subject to change and
    *             could break backwards compatibility.
    */
    virtual telux::common::Status encodeEuroNcapOptionalAdditionalData(
        telux::tel::ECallOptionalEuroNcapData optionalEuroNcapData, std::vector<uint8_t> &data)
        = 0;

   /**
    * Sends real Time Text (RTT) to remote party during an active RTT call session.
    *
    * On platforms with access control enabled, the caller needs to have TELUX_TEL_CALL_MGMT
    * permission to successfully invoke this API.
    *
    * @param [in] phoneId        PhoneId to which text is sent.
    * @param [in] message        Text to be sent to a remote party in UTF8 encoding format.
    *                            Maximum length of the message is 127 characters.
    * @param [in] callback       Callback function to get the response of sendRtt request.
    *
    * @returns Status of sendRtt i.e. success or suitable error code.
    *
    */
   virtual telux::common::Status sendRtt(int phoneId,
      std::string message, common::ResponseCallback callback = nullptr) = 0;

   /**
    * Configure eCall redial parameters.
    * Redial of an eCall can be attempted by the modem during an eCall origination failure or when
    * it gets terminated before receipt of the MSD transmission status.
    * The eCall redial parameters should be configured before initiating a regulatory eCall and
    * this configuration is not persistent after modem reset.
    *
    * On platforms with access control enabled, the caller needs to have TELUX_TEL_ECALL_MGMT
    * permission to successfully invoke this API.
    *
    * @param [in] config         Indicates eCall redial configuration
    *                            @ref telux::tel::RedialConfigType
    * @param [in] timeGap        Indicates time gap between successive redial attempts in
    *                            milliseconds.
    *                            Redial attempts can range from 1 to 10 for eCall origination
    *                            failures. For eCall termination before the receipt of MSD
    *                            Transmission status, the range is between 1 and 2 attempts.
    *                            The redial minimum time duration between the successive redial
    *                            attempts is set as per 3GPP TS22.001 annex 6 and the user is
    *                            expected to provide a suitable value of timeGap.
    * ---------------------------------------------------------------------------------------------
    * -----------------------------------ECALL ORIGINATION FAILURE---------------------------------
    * -------------------( @ref telux::tel::RedialConfigType::CALL_ORIG )--------------------------
    * ---------------------------------------------------------------------------------------------
    * Call attempt                                                    Minimum duration between
    *                                                                       call attempt
    *                                                              ( in milliseconds as per
    *                                                                3GPP TS22.001 annex 6 )
    *----------------------------------------------------------------------------------------------
    * Initial call attempt                                                     NA
    *     1                                                                    5000
    *     2                                                                    60000
    *     3                                                                    60000
    *     4                                                                    60000
    *     5 attempt and                                                        180000
    *     subsequent attempts
    * ---------------------------------------------------------------------------------------------
    * -----------------------------------------ECALL DROP -----------------------------------------
    * -----------------------( @ref telux::tel::RedialConfigType::CALL_DROP )----------------------
    * ---------------------------------------------------------------------------------------------
    * Call attempt                                                    Minimum duration between
    *                                                                       call attempt
    *                                                              ( in milliseconds as per
    *                                                                3GPP TS22.001 annex 6 )
    *----------------------------------------------------------------------------------------------
    * Initial call attempt                                                      NA
    *     1                                                                     5000
    *     2                                                                     60000
    *----------------------------------------------------------------------------------------------

    *
    * @param [in] callback       Callback function to get the response of the configureECallRedial
    *                            request.
    *
    * @returns Status of configureECallRedial i.e. success or suitable error code.
    *
    * @note Eval: This is a new API and is being evaluated. It is subject to change and could break
    *             backwards compatibility.
    */
   virtual telux::common::Status configureECallRedial(RedialConfigType config,
        const std::vector<int> &timeGap, common::ResponseCallback callback = nullptr) = 0;

   /**
    * Restart T9 and T10 eCall High Level Application Protocol (HLAP) timers with residual timer
    * duration. Application is expected to maintain residual timer information and resume the
    * timers during events like modem reset or transition of device operating mode from low power
    * mode to online.
    *
    * Notes:
    * 1. Application must restart timer according to eCall operating mode of device.
    *    T10 eCall HLAP timer must be restarted only when eCall operating mode is
    *    @ref telux::tel::ECallMode::ECALL_ONLY.
    * 2. Application must validate the residual timer value before calling the API to prevent
    *    invalid data being processed.
    * 3. T9 eCall HLAP timer cannot be restarted after transition of device operating mode from
    *    low power mode to online.
    *
    * On platforms with access control enabled, caller needs to have TELUX_TEL_ECALL_MGMT
    * permission to invoke this API successfully.
    *
    * @param [in] timerId - Represents the timer which is required to be restarted by
    *                       application. @ref telux::tel::EcallHlapTimerId.
    * @param [in] duration - Remaining time duration in seconds for the timer to run.
    * @param [in] callback - Callback function to get the response of the restartECallHlapTimer
    *                        request.
    *
    * @returns Status of restartECallHlapTimer i.e. success or suitable error code.
    *
    * @note Eval: This is a new API and is being evaluated. It is subject to change and
    *             could break backwards compatibility.
    *
    */
   virtual telux::common::Status restartECallHlapTimer(int phoneId, EcallHlapTimerId timerId,
        int duration, common::ResponseCallback callback = nullptr ) = 0;

   /**
    * Retrieve the configured eCall redial parameters for call origination and call drop failures.
    *
    * On platforms with access control enabled, the caller needs to have TELUX_TEL_ECALL_MGMT
    * permission to successfully invoke this API.
    *
    * @param [out] callOrigTimeGap Gets redial time gap between successive redial attempts in
    *                              milliseconds for call origination failures.
    * @param [out] callDropTimeGap Gets redial time gap between successive redial attempts in
    *                              milliseconds for call drop failures.
    *
    * @returns ErrorCode for getECallRedialConfig i.e. success or suitable error code.
    *
    * @note Eval: This is a new API and is being evaluated. It is subject to change and could break
    *             backwards compatibility.
    */
   virtual telux::common::ErrorCode getECallRedialConfig(std::vector<int> &callOrigTimeGap,
        std::vector<int> &callDropTimeGap) = 0;

   /**
    * Initiate an ERA-GLONASS self test automotive eCall with raw MSD pdu, to the specified
    * phone number over CS based RAT only (i.e. not IMS).
    * It will be treated like a regular voice call by the UE and the network.
    * When an ERA-GLONASS emergency eCall is triggered by user during a self test ECALL, self test
    * eCall will terminate.
    * Application must not trigger ERA-GLONASS self test automotive eCall when T10 HLAP timer
    * @ref telux::tel::ECallHlapTimerStatus::t10 is active @ref telux::tel::HlapTimerStatus::ACTIVE
    * due to a previous activation of automotive emergency eCall.
    *
    * Self test ECall can be triggered in both the eCall operating mode
    * @ref telux::tel::ECallMode::ECALL_ONLY and @ref telux::tel::ECallMode::NORMAL.
    * T9 and T10 HLAP timers will not triggered during a self test ERA-GLONASS eCall.
    *
    * On platforms with access control enabled, the caller needs to have TELUX_TEL_ECALL_MGMT
    * permission to successfully invoke this API.
    *
    * @param [in] phoneId   Represents phone corresponding to which on make eCall
    *                       operation is performed
    * @param [in] dialNumber   String representing the dialing number
    * @param [in] msdPdu    Encoded MSD(Minimum Set of Data) PDU as per spec GOST R 54620/
    *                       GOST R 33464
    * @param [in] callback  Callback function to get the response of makeECall
    *                       request.
    *                       Possible(not exhaustive) error codes for callback response
    *                       - @ref telux::common::ErrorCode::SUCCESS
    *                       - @ref telux::common::ErrorCode::RADIO_NOT_AVAILABLE
    *                       - @ref telux::common::ErrorCode::NO_MEMORY
    *                       - @ref telux::common::ErrorCode::MODEM_ERR
    *                       - @ref telux::common::ErrorCode::INTERNAL_ERR
    *                       - @ref telux::common::ErrorCode::INVALID_STATE
    *                       - @ref telux::common::ErrorCode::INVALID_CALL_ID
    *                       - @ref telux::common::ErrorCode::INVALID_ARGUMENTS
    *                       - @ref telux::common::ErrorCode::OPERATION_NOT_ALLOWED
    *                       - @ref telux::common::ErrorCode::GENERIC_FAILURE
    *
    * @returns Status of makeECall i.e. success or suitable status code.
    *
    */
   virtual telux::common::Status makeECall(int phoneId, const std::string dialNumber,
        const std::vector<uint8_t> &msdPdu, MakeCallCallback callback = nullptr) = 0;

   /**
    * Post test registration timer is started upon termination of ERA-GLONASS self-test to ensure
    * UE remains registered on the network for the specified duration. Upon expiry of this timer
    * UE will deregister from the network when UE is in @ref telux::tel::ECallMode::ECALL_ONLY
    * mode. Application must update post test registration timer before triggering self-test eCall
    * to overrride exisiting settings.
    *
    * The update of post test registration timer is not persistent across reboot.
    *
    * On platforms with access control enabled, the caller needs to have TELUX_TEL_ECALL_MGMT
    * permission to successfully invoke this API.
    *
    * @param [in] phoneId   Represents phone corresponding to which on request to update the post
    *                       test registration timer is made.
    * @param [in] timer     Input timer value in minutes.
    *                       Input timer value must be greater than 0 mins. In situations when, AP
    *                       sets the timer value to 0 minutes, UE will interpret it as 2 minutes.
    * @param [in] callback  Callback function to get the response of
    *                       updateECallPostTestRegistrationTimer request.
    *
    * @returns Status of updateECallPostTestRegistrationTimer i.e. success or suitable status code.
    *
    */
    virtual telux::common::Status updateECallPostTestRegistrationTimer(int phoneId, uint32_t timer,
        common::ResponseCallback callback = nullptr) = 0;

   /**
    * Get the post test registration timer. This timer is applicable only for ERA-GLONASS self test
    * eCall when device is in @ref telux::tel::ECallMode::ECALL_ONLY eCall operating mode.
    * Default value of timer is 2 minutes.
    *
    * On platforms with access control enabled, the caller needs to have TELUX_TEL_ECALL_MGMT
    * permission to successfully invoke this API.
    *
    * @param [in] phoneId   Represents phone corresponding to which on request to get the post
    *                       test registration timer is made.
    *                       operation is performed
    * @param [out] timer    Timer value in minutes.
    *
    * @returns ErrorCode of getECallPostTestRegistrationTimer i.e. success or suitable error code.
    *
    */
   virtual telux::common::ErrorCode getECallPostTestRegistrationTimer(int phoneId,
        uint32_t &timer) = 0;

    /**
     * Enable or disable emergency mode configuration for AECS (Automated Emergency Call System)
     * calls.
     *
     * This API enables or disables emergency mode and optionally allows antenna switching for
     * AECS call:
     *   - When emergency mode is enabled on the phone identifier, other subscriptions
     *     in DSDS/DSDA mode will be suspended.
     *   - If antenna switching is enabled, the system will automatically select the best
     *     alternative antenna if the active one becomes damaged.
     *
     * For incoming AECS call from AECSP:
     *   - This API must be called first to enable emergency mode.
     *
     * After the AECS call ends:
     *   - The user should disable emergency mode and turn off antenna switching if previously
     *     enabled.
     *
     * @note Antenna switching cannot be enabled when emergency mode is disabled. i.e., Setting
     *       emergencyModeEnabled = false while antennaSwitchEnabled = true is not supported and
     *       will return @ref telux::common::ErrorCode::INVALID_ARGUMENTS.
     * @note This API is platform-dependent. Ensure the platform supports antenna switching
     *       before enabling antenna switching.
     *
     * On platforms with access control enabled, the caller needs to have TELUX_TEL_ECALL_MGMT
     * permission to successfully invoke this API.
     *
     * @param [in] phoneId          Represents the phone corresponding to which the emergency
     *                              mode is configured.
     * @param [in] emergencyModeEnabled If true, enables emergency mode in SS/DSDS/DSDA;
     *                                  if false, disables emergency mode in SS/DSDS/DSDA.
     * @param [in] antennaSwitchEnabled If true, enables automatic antenna switching mode;
     *                                  if false, disables automatic antenna switching mode.
     * @param [in] callback         Optional callback pointer to get the response of the
     *                              setEmergencyMode request.
     *
     * @returns Status of the setEmergencyMode request; either success or the suitable error code.
     *
     * @note   Eval: This is a new API and is being evaluated. It is subject to
     *         change and could break backwards compatibility.
     *
     */
    virtual telux::common::Status setEmergencyMode(int phoneId, bool emergencyModeEnabled,
        bool antennaSwitchEnabled = false, telux::common::ResponseCallback callback = nullptr)
        = 0;

    /**
     * Add a listener to listen for incoming call, call info change and eCall MSD
     * transmission status change.
     *
     * @param [in] listener  Pointer to ICallListener object which receives event
     *                       corresponding to phone
     *
     * @returns Status of registerListener i.e. success or suitable error code.
     */
    virtual telux::common::Status registerListener(
        std::shared_ptr<telux::tel::ICallListener> listener)
        = 0;

   /**
    * Remove a previously added listener.
    *
    * @param [in] listener  Listener to be removed.
    *
    * @returns Status of removeListener i.e. success or suitable error code.
    */
   virtual telux::common::Status removeListener(std::shared_ptr<telux::tel::ICallListener> listener)
      = 0;

   virtual ~ICallManager(){};
};

/**
 * @brief Interface for Make Call callback object.
 * Client needs to implement this interface to get single shot responses for
 * commands like make call.
 *
 * The methods in callback can be invoked from multiple different threads.
 * The implementation should be thread safe.
 */
class IMakeCallCallback : public telux::common::ICommandCallback {
public:
   /**
    * This function is called with the response to makeCall API.
    *
    * @param [out] error  @ref telux::common::ErrorCode
    * @param [out] call   Pointer to Call object or nullptr in case of failure
    */
   virtual void makeCallResponse(telux::common::ErrorCode error,
                                 std::shared_ptr<ICall> call = nullptr) {
   }

   virtual ~IMakeCallCallback(){};
};

/** @} */ /* end_addtogroup telematics_call */

}  // End of namespace tel

}  // End of namespace telux

#endif // TELUX_TEL_CALLMANAGER_HPP
