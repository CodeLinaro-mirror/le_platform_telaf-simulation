/*
 *  Copyright (c) 2017-2020, The Linux Foundation. All rights reserved.
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
 * @file       Phone.hpp
 * @brief      Phone class is the primary interface to get phone informations
 *             like radio state, signal strength, turn on/off radio power,
 *             voice radio tech and voice service state.
 */

#ifndef TELUX_TEL_PHONE_HPP
#define TELUX_TEL_PHONE_HPP

#include <memory>
#include <string>

#include <telux/common/CommonDefines.hpp>
#include <telux/tel/Call.hpp>
#include <telux/tel/CellInfo.hpp>
#include <telux/tel/ECallDefines.hpp>
#include <telux/tel/PhoneDefines.hpp>
#include <telux/tel/PhoneManager.hpp>
#include <telux/tel/VoiceServiceInfo.hpp>

namespace telux {
namespace tel {

/** @addtogroup telematics_phone
 * @{ */

class ISignalStrengthCallback;
class IVoiceServiceStateCallback;

/**
 * This function is called with the response to requestVoiceRadioTechnology API.
 *
 * The callback can be invoked from multiple different threads.
 * The implementation should be thread safe.
 *
 * @param [in] radioTech  Pointer to radio technology
 * @param [in] error      Return code for whether the operation
 *                        succeeded or failed
 *                        - @ref telux::common::ErrorCode::SUCCESS
 *                        - @ref telux::common::ErrorCode::RADIO_NOT_AVAILABLE
 *                        - @ref telux::common::ErrorCode::GENERIC_FAILURE
 *
 * @deprecated Use IVoiceServiceStateCallback instead
 */
using VoiceRadioTechResponseCb
   = std::function<void(telux::tel::RadioTechnology radioTech, telux::common::ErrorCode error)>;

/**
 * This function is called with the response to requestCellInfo API.
 *
 * The callback can be invoked from multiple different threads.
 * The implementation should be thread safe.
 *
 * @param [out] cellInfoList  vector of shared pointers to cell info object
 * @param [out] error         Return code for whether the operation
 *                            succeeded or failed
 */
using CellInfoCallback = std::function<void(std::vector<std::shared_ptr<CellInfo>> cellInfoList,
                                            telux::common::ErrorCode error)>;

/**
 * This function is called with the response to requestECallOperatingMode API.
 *
 * The callback can be invoked from multiple different threads.
 * The implementation should be thread safe.
 *
 * @param [out] eCallMode    @ref ECallMode
 * @param [out] error        Return code for whether the operation succeeded or failed
 */
using ECallGetOperatingModeCallback
   = std::function<void(ECallMode eCallMode, telux::common::ErrorCode error)>;

/**
 * This function is called with the response to requestOperatorName API.
 *
 * The callback can be invoked from multiple different threads.
 * The implementation should be thread safe.
 *
 * @param [out] operatorLongName   Current registered operator long name
 * @param [out] operatorShortName  Current registered operator short name
 * @param [out] error              Return code for whether the operation succeeded or failed
 *
 * @deprecated Use OperatorInfoCallback API instead.
 */
using OperatorNameCallback
   = std::function<void(std::string operatorLongName, std::string operatorShortName,
       telux::common::ErrorCode error)>;
/**
 * This function is called with the response to requestOperatorInfo API.
 *
 * The callback can be invoked from multiple different threads.
 * The implementation should be thread safe.
 *
 * @param [out] plmnInfo           @ref PlmnInfo
 * @param [out] error              Return code for whether the operation succeeded or failed
 *
 */
using OperatorInfoCallback
   = std::function<void(PlmnInfo plmnInfo, telux::common::ErrorCode error)>;

/**
 * @brief This class allows getting system information and registering for system events.
 * Each Phone instance is associated with a single SIM. So on a dual SIM device you
 * would have 2 Phone instances.
 */
class IPhone {
public:
   /**
    * Get the Phone ID corresponding to phone.
    *
    * @param [out] phoneId   Unique identifier for the phone
    *
    * @returns Status of getPhoneId i.e. success or suitable error code.
    */
   virtual telux::common::Status getPhoneId(int &phoneId) = 0;

   /**
    * Get Radio state of device.
    *
    * @returns @ref RadioState
    *
    * @deprecated Use IPhoneManager::requestOperatingMode() API instead
    */
   virtual RadioState getRadioState() = 0;

   /**
    * Request for Radio technology type (3GPP/3GPP2) used for voice.
    *
    * @param [in] callback  callback pointer to get the response of radio power
    *                       request @ref telux::tel::VoiceRadioTechResponseCb
    *
    * @returns Status of requestVoiceRadioTechnology i.e. success or suitable
    * error code @ref telux::common::Status.
    *
    * @deprecated Use requestVoiceServiceState() API to get VoiceServiceInfo which
    *             has API to get radio technology i.e VoiceServiceInfo::getRadioTechnology()
    */
   virtual telux::common::Status requestVoiceRadioTechnology(VoiceRadioTechResponseCb callback) = 0;

   /**
    * Get service state of the phone.
    *
    * @returns    @ref ServiceState
    *
    * @deprecated Use requestVoiceServiceState() API
    */
   virtual ServiceState getServiceState() = 0;

   /**
    * Request for voice service state to get the information of phone serving
    * states
    *
    * @param [in] callback  callback pointer to get the response of voice
    *                       service state @ref telux::tel::IVoiceServiceStateCallback.
    *
    * @returns Status of requestVoiceServiceState i.e. success or suitable error
    * code @ref telux::common::Status.
    */
   virtual telux::common::Status
      requestVoiceServiceState(std::weak_ptr<IVoiceServiceStateCallback> callback)
      = 0;

   /**
    * Set the radio power on or off.
    *
    * On platforms with Access control enabled, Caller needs to have TELUX_TEL_PHONE_MGMT permission
    * to invoke this API successfully.
    *
    * @param [in] enable    Flag that determines whether to turn radio on or off
    * @param [in] callback  Optional callback pointer to get the response of set
    *                       radio power request
    *
    * @returns Status of setRadioPower i.e. success or suitable error code.
    *
    * @deprecated Use IPhoneManager::setOperatingMode() API instead
    */
   virtual telux::common::Status setRadioPower(
      bool enable, std::shared_ptr<telux::common::ICommandResponseCallback> callback = nullptr)
      = 0;

   /**
    * Get the cell information about current serving cell and neighboring cells.
    *
    * On platforms with Access control enabled, Caller needs to have TELUX_TEL_PRIVATE_INFO_READ
    * permission to invoke this API successfully.
    *
    * @param [in] callback    Callback to get the response of cell info request
    *                         @ref telux::tel::CellInfoCallback
    *
    * @returns Status of requestCellInfo i.e. success or suitable error
    *
    */
   virtual telux::common::Status requestCellInfo(CellInfoCallback callback) = 0;

   /**
    * Set the minimum time in milliseconds between when the cell info list should
    * be received.
    *
    * On platforms with Access control enabled, Caller needs to have TELUX_TEL_PHONE_CONFIG
    * permission to invoke this API successfully.
    *
    * @param [in] timeInterval  Value of 0 means receive cell info list when any
    *                           info changes. Value of INT_MAX means never
    *                           receive cell info list even on change.
    *                           Default value is 0
    * @param [in] callback      Callback to get the response for set cell info
    *                           list rate.
    *
    * @returns Status of setCellInfoListRate i.e. success or suitable error
    *
    */
   virtual telux::common::Status setCellInfoListRate(uint32_t timeInterval,
                                                     common::ResponseCallback callback)
      = 0;

   /**
    * Get current signal strength of the associated network.
    *
    * @param [in] callback   Optional callback pointer to get the response of
    *                        signal strength request
    *
    * @returns Status of requestSignalStrength i.e. success or suitable error
    * code.
    */
   virtual telux::common::Status
      requestSignalStrength(std::shared_ptr<ISignalStrengthCallback> callback = nullptr)
      = 0;

   /**
    * Sets the eCall operating mode
    *
    * On platforms with Access control enabled, Caller needs to have TELUX_TEL_ECALL_CONFIG
    * permission to invoke this API successfully.
    *
    * @param [in] eCallMode - @ref ECallMode
    * @param [in] callback - Callback function to get the response for set eCall operating mode
    * request.
    *
    * @returns Status of setECallOperatingMode i.e. success or suitable error
    */
   virtual telux::common::Status setECallOperatingMode(ECallMode eCallMode,
                                                       telux::common::ResponseCallback callback)
      = 0;

   /**
    * Get the eCall operating mode
    *
    * @param [in] callback - Callback function to get the response of eCall operating mode request
    *
    * @returns Status of requestECallOperatingMode i.e. success or suitable error
    */
   virtual telux::common::Status requestECallOperatingMode(ECallGetOperatingModeCallback callback)
      = 0;

   /**
    * Get current registered operator name.
    * This API returns PLMN name if available. If not then it returns the SPN configured in the
    * SIM card.
    *
    * On platforms with Access control enabled, Caller needs to have TELUX_TEL_PRIVATE_INFO_READ
    * permission to invoke this API successfully.
    *
    * @param [in] callback - Callback function to get the response of operator name request
    *
    * @returns Status of requestOperatorName i.e. success or suitable error
    *
    * @deprecated Use IPhone::requestOperatorInfo(OperatorInfoCallback callback) API instead.
    *
    */
   virtual telux::common::Status requestOperatorName(OperatorNameCallback callback) = 0;

   /**
    * Get current registered operator information.
    * This API returns PLMN information about the network the device is currently camped on. If
    * this information is not available then it returns the SPN in the SIM card.
    *
    * On platforms with Access control enabled, Caller needs to have TELUX_TEL_PRIVATE_INFO_READ
    * permission to invoke this API successfully.
    *
    * @param [in] callback - Callback function to get the response of operator information request
    *
    * @returns Status of requestOperatorInfo i.e. success or suitable error
    *
    * @note Eval: This is a new API and is being evaluated. It is subject to change and
    *             could break backwards compatibility.
    */
   virtual telux::common::Status requestOperatorInfo(OperatorInfoCallback callback) = 0;

   /**
    * Configures SignalStrength notification.
    *
    * This API configures SignalStrength notifications based on the RAT(s) delta
    * or threshold provided for SignalStrength.
    *
    * - Delta (unsigned 2 bytes): The value should be a non-zero positive integer, in units
    * of 0.1dBm. For example to set a delta of 10dBm, the delta value should be 100.
    * A notification is sent when the difference between the current value and the last reported
    * value crosses the specified delta.
    *
    * -Threshold (signed 4 bytes): For example to set threshold at -95dBm and -80dBm, the threshold
    * list values are -950, -800, since the the list values are in units of 0.1 dBm.
    * A notification is sent when the current signal strength crosses one of the registered
    * thresholds.
    *
    * The threshold range list is as follows. See SignalStrength.hpp for more details.
    * - GSM_RSSI  : -113 to -51 (in dBm)
    * - WCDMA_RSSI: -113 to -51 (in dBm)
    * - LTE_RSSI  : -113 to -51 (in dBm)
    * - LTE_SNR   : -200 to 300 (in dB)
    * - LTE_RSRQ  : -20 to -3   (in dB)
    * - LTE_RSRP  : -140 to -44 (in dBm)
    * - NR5G_SNR  : -200 to 300 (in dB)
    * - NR5G_RSRP : -140 to -44 (in dBm)
    * - NR5G_RSRQ : -20 to -3   (in dB)
    *
    * This configuration is a global setting. The signal strength setting does not persist through
    * device reboot and needs to be configured again. Default signal strength configuration is set
    * after a device reboot.
    *
    * On platforms with access control enabled, the caller needs to have the TELUX_TEL_PHONE_MGMT
    * permission to successfully invoke this API.
    *
    * @param [in] signalStrengthConfig     Signal strength configuration.
    * @param [in] callback                 Callback function to get the SignalStrength
    *                                      configuration response.
    *
    *
    * @returns Status of configureSignalStrength, i.e., success or the suitable error code.
    *
    * @deprecated Use configureSignalStrength(std::vector<SignalStrengthConfigEx>).
    */
   virtual telux::common::Status configureSignalStrength(
        std::vector<SignalStrengthConfig> signalStrengthConfig, telux::common::ResponseCallback
        callback = nullptr) = 0;

  /**
    * Configures signal strength notifications based on the RAT(s) delta or threshold list.
    * Additionally, the hysteresis dB can be applied on top of the threshold list. Furthermore,
    * time hysteresis (hysteresis ms) can be applied either on top of the delta or on the
    * threshold list, or even on top of both the threshold list and the hysteresis dB.
    *
    * - Delta (unsigned 2 bytes): A notification is sent when the difference between the current
    * signal strength value and the last reported signal strength value crosses the specified
    * delta.
    * The value should be a non-zero positive integer, in units of 0.1dBm. For example, to set a
    * delta of 10dBm, the value should be 100.
    *
    * - Threshold (signed 4 bytes): A notification is sent when the current signal strength
    * crosses over or under any of the thresholds specified.
    * For example, to set thresholds at -95 dBm and -80 dBm, the threshold list values are -950,
    * -800, since the list values are in units of 0.1 dBm.
    *
    * - Hysteresis dB (unsigned 2 bytes): Prevents the generation of multiple notifications when
    * the signal strength is close to a threshold value and experiencing frequent small changes.
    * With a non-zero hysteresis, the signal strength indicators should cross over or under by
    * more than the hysteresis value for a notification to be sent.
    * To apply hysteresis, the value should be a non-zero positive integer, in units of 0.1 dBm.
    * For example, to set a hysteresis dB of 10 dBm, the value should be 100.
    *
    * - Hysteresis ms (unsigned 2 bytes): Time hystersis can be applied to avoid multiple
    * notifications even when all the other criteria for a notification are met. The time
    * hystersis can be applied on top of any other criteria (delta, threshold, threshold and
    * hysteresis).
    *
    * If the hysteresis(dB or ms) value is set to 0, the signal strength notification criteria just
    * considers the threshold or delta. Once configured, the hysteresis value for a signal strength
    * type is retained, until explicitly reconfigured to 0 again or device reboot.
    *
    * The threshold range list is as follows. See SignalStrength.hpp for more details.
    * - RAT    Measurement type  : value
    * - GSM     RSSI             : -113 to -51 (in dBm)
    * - WCDMA   RSSI             : -113 to -51 (in dBm)
    * - WCDMA   ECIO             : -24 to 0    (in dB)
    * - WCDMA   RSCP             : -120 to -24 (in dBm)
    * - LTE     RSSI             : -113 to -51 (in dBm)
    * - LTE     SNR              : -200 to 300 (in dB)
    * - LTE     RSRQ             : -20 to -3   (in dB)
    * - LTE     RSRP             : -140 to -44 (in dBm)
    * - NR5G    SNR              : -200 to 300 (in dB)
    * - NR5G    RSRP             : -140 to -44 (in dBm)
    * - NR5G    RSRQ             : -20 to -3   (in dB)
    *
    * This configuration is a global setting. The signal strength setting does not persist through
    * device reboot and needs to be configured again. On reboot, the default signal strength
    * configuration is set to delta @ref telux::tel::SignalStrengthConfigExType with default values
    * for all signal measurement types.
    *
    * On platforms with access control enabled, the caller needs to have the TELUX_TEL_PHONE_MGMT
    * permission to successfully invoke this API.
    *
    * @note This API is not supported for @ref telux::tel::RadioTechnology::RADIO_TECH_NB1_NTN
    *
    * @param [in] signalStrengthConfigEx   Signal strength configuration.
    * @param [in] hysteresisMs             (Optional) Signal strength hysteresis timer in
    *                                      milliseconds.
    * @param [in] callback                 Callback function to get the SignalStrength
    *                                      configuration response.
    *
    * @returns Status of configureSignalStrength, i.e., success or the suitable error code.
    *
    * @note Eval: This is a new API and is being evaluated. It is subject to change and
    *             could break backwards compatibility.
    */

   virtual telux::common::Status configureSignalStrength(
       std::vector<SignalStrengthConfigEx> signalStrengthConfigEx, uint16_t hysteresisMs = 0,
       telux::common::ResponseCallback callback = nullptr) = 0;

   virtual ~IPhone(){};
};

/**
 * @brief Interface for Signal strength callback object.
 * Client needs to implement this interface to get single shot responses for
 * commands like get signal strength.
 *
 * The methods in callback can be invoked from multiple different threads.
 * The implementation should be thread safe.
 */
class ISignalStrengthCallback : public telux::common::ICommandCallback {
public:
   /**
    * This function is called with the response to requestSignalStrength API.
    *
    * @param [in] signalStrength   Pointer to signal strength object
    * @param [in] error            Return code for whether the operation
    *                              succeeded or failed
    *                              @ref SUCCESS
    *                              @ref telux::common::ErrorCode::RADIO_NOT_AVAILABLE
    */
   virtual void signalStrengthResponse(std::shared_ptr<SignalStrength> signalStrength,
                                       telux::common::ErrorCode error) {
   }

   virtual ~ISignalStrengthCallback() {
   }
};

/**
 * @brief Interface for voice service state callback object.
 * Client needs to implement this interface to get single shot responses for
 * commands like request voice radio technology.
 *
 * The methods in callback can be invoked from multiple different threads.
 * The implementation should be thread safe.
 */
class IVoiceServiceStateCallback : public telux::common::ICommandCallback {
public:
   /**
    * This function is called with the response to requestVoiceServiceState API.
    *
    * @param [in] serviceInfo   Pointer to voice service info object
    *                           @ref telux::tel::VoiceServiceInfo
    * @param [in] error         Return code for whether the operation
    *                           succeeded or failed
    *                           - @ref telux::common::ErrorCode::SUCCESS
    *                           - @ref telux::common::ErrorCode::RADIO_NOT_AVAILABLE
    *                           - @ref telux::common::ErrorCode::GENERIC_FAILURE
    */
   virtual void voiceServiceStateResponse(const std::shared_ptr<VoiceServiceInfo> &serviceInfo,
                                          telux::common::ErrorCode error) {
   }

   virtual ~IVoiceServiceStateCallback() {
   }
};

/** @} */ /* end_addtogroup telematics_phone
 */
}
}

#endif // TELUX_TEL_PHONE_HPP
