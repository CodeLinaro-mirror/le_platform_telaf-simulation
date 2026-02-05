/* Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @page c_tafRadio Radio Service
 *
 * @rst :ref:`API reference <File taf_radio_interface.h>` @endrst
 *
 * <HR>
 *
 * The Radio Service APIs contain phone and network functions and is applied to configure and
 * obtain wireless cellular network information. By default, the radio service is powered on and
 * available by the system.
 *
 * @section taf_radio_binding IPC interfaces binding
 *
 * The functions of this API are provided by the @b tafRadioSvc service.
 *
 * The following example illustrates how to bind to the Radio service.
 * @verbatim

   bindings:
   {
       clientExe.clientComponent.taf_radio -> tafRadioSvc.taf_radio
   }

   @endverbatim
 *
 * @section c_taf_radio_power Power Management
 *
 * Users can power on or power off the radio, but by default it is powered on.
 *
 * - taf_radio_SetRadioPower() / taf_radio_GetRadioPower() -- Sets/gets radio power status.
 *
 * The following example illustrates powering on the radio.
 *
 * @code
 *
 *   le_onoff_t power = LE_OFF;
 *
 *   if (taf_radio_SetRadioPower(LE_ON, phoneId) != LE_OK)
 *       LE_ERROR("Fail to set radio power.");
 *
 *   if (taf_radio_GetRadioPower(&power, phoneId) != LE_OK)
 *       LE_ERROR("Fail to get radio power status.");
 *
 *   if (power != LE_ON)
 *       LE_ERROR("Unexpected power status.");
 *
 *   @endcode
 *
 * @section c_taf_radio_registartion Network Registration
 *
 * Users can register with the network automatically or manually with specific MCC and MNC.
 *
 *  - taf_radio_SetAutomaticRegisterMode() -- Registers with a network automatically.
 *
 *  - taf_radio_SetManualRegisterMode() -- Registers with a network with MCC and MNC manually.
 *
 *  - taf_radio_GetRegisterMode() -- Gets the network register mode.
 *
 *  - taf_radio_SetManualRegisterModeAsync() -- Registers with a network asynchronously with a handler.
 *
 *  - taf_radio_AddNetRegRejectHandler() / taf_radio_RemoveNetRegRejectHandler() -- Adds/removes
 *    handler for network registration rejection.
 *
 *  - taf_radio_GetPlatformSpecificRegistrationErrorCode() -- Gets the network registration error code.
 *
 * The following example illustrates network registration.
 *
 * @code
 *
 *   if (taf_radio_SetManualRegisterMode(mccStr1, mncStr1, phoneId) == LE_OK)
 *       LE_ERROR("Fail to set manual register mode.");
 *
 *   LE_INFO("Platform registration error code: %d.", taf_radio_GetPlatformSpecificRegistrationErrorCode());
 *
 *   bool isManualMode = false;
 *   if (taf_radio_GetRegisterMode(&isManualMode, mccStr2, TAF_RADIO_MCC_BYTES, mncStr2,
 *       TAF_RADIO_MNC_BYTES, phoneId) != LE_OK)
 *       LE_ERROR("Fail to get register mode.");
 *
 *   if (!isManualMode)
 *       LE_ERROR("Unexpected register mode.");
 *
 *   if (strcmp(mccStr1, mccStr2) != 0)
 *       LE_ERROR("Unexpected MCC.");
 *
 *   if (strcmp(mncStr1, mncStr2) != 0)
 *       LE_ERROR("Unexpected MNC.");
 *
 *   @endcode
 ** @section c_taf_radio_service_domain Service Domain
 *
 * Users can get the registration domain for the current serving RAT.
 *
 *  - taf_radio_GetServiceDomain() -- Get the registration domain for the current serving RAT.
 *
 *  - taf_radio_SetServiceDomainPreferences() -- Sets the service domain preferences.
 *
 *  - taf_radio_GetServiceDomainPreferences() -- Gets the network register mode.
 *
 * The following example illustrates getting service domain.
 *
 * @code
 *
 *    taf_radio_ServiceDomainState_t domain;
 *    le_result_t result = taf_radio_GetServiceDomainPreferences(&domain, phoneId);
 *    if (result != LE_OK)
 *       LE_ERROR("Fail to get service domain.");
 *    switch (domain)
 *    {
 *        case TAF_RADIO_SERVICE_DOMAIN_STATE_CS_ONLY:
 *            LE_INFO("Domain : CS Only");
 *            break;
 *        case TAF_RADIO_SERVICE_DOMAIN_STATE_PS_ONLY:
 *            LE_INFO("Domain : PS Only");
 *            break;
 *        case TAF_RADIO_SERVICE_DOMAIN_STATE_CS_AND_PS:
 *            LE_INFO("Domain : CS and PS");
 *            break;
 *        default:
 *            LE_ERROR("Domain : Unknown");
 *    }
 *
 *   @endcode
 *
 * @section c_taf_radio_service_domain Service Domain
 *
 * Users can get the registration domain for the current serving RAT.
 *
 *  - taf_radio_GetServiceDomain() -- Get the registration domain for the current serving RAT.
 *
 *  - taf_radio_SetServiceDomainPreferences() -- Sets the service domain preferences.
 *
 *  - taf_radio_GetServiceDomainPreferences() -- Gets the network register mode.
 *
 * The following example illustrates getting service domain.
 *
 * @code
 *
 *    taf_radio_ServiceDomainState_t domain;
 *    le_result_t result = taf_radio_GetServiceDomainPreferences(&domain, phoneId);
 *    if (result != LE_OK)
 *       LE_ERROR("Fail to get service domain.");
 *    switch (domain)
 *    {
 *        case TAF_RADIO_SERVICE_DOMAIN_STATE_CS_ONLY:
 *            LE_INFO("Domain : CS Only");
 *            break;
 *        case TAF_RADIO_SERVICE_DOMAIN_STATE_PS_ONLY:
 *            LE_INFO("Domain : PS Only");
 *            break;
 *        case TAF_RADIO_SERVICE_DOMAIN_STATE_CS_AND_PS:
 *            LE_INFO("Domain : CS and PS");
 *            break;
 *        default:
 *            LE_ERROR("Domain : Unknown");
 *    }
 *
 *   @endcode
 *
 * @section c_taf_radio_preferred_operator Preferred Operators
 *
 * Users can configure the preferences of operators and get the configuration details
 * with a list.
 *
 *  - taf_radio_GetPreferredOperatorsList() / taf_radio_DeletePreferredOperatorsList() -- Creates/deletes
 *    a preferred operator list reference.
 *
 *  - taf_radio_AddPreferredOperator() / taf_radio_RemovePreferredOperator() -- Adds/removes a
 *    preferred operator.
 *
 *  - taf_radio_GetFirstPreferredOperator() / taf_radio_GetNextPreferredOperator() -- Gets the
 *    first/next preferred operator reference.
 *
 *  - taf_radio_GetPreferredOperatorDetails() -- Gets the operator details.
 *
 * The following example illustrates preferred operator list management.
 *
 * @code
 *
 *   // Add preference.
 *   for (int i = 0; i < PREFERRED_OPERATOR_NUM; i++) {
 *       taf_radio_AddPreferredOperator(prefMccStr[i], prefMncStr[i], prefRatMask, phoneId);
 *   }
 *
 *   // Create list.
 *   listRef = taf_radio_GetPreferredOperatorsList(phoneId);
 *
 *   // Traverse list
 *   opRef = taf_radio_GetFirstPreferredOperator(listRef);
 *   while (opRef != nullptr) {
 *       taf_radio_GetPreferredOperatorDetails(opRef, mccStr, TAF_RADIO_MCC_BYTES, mncStr,
 *           TAF_RADIO_MNC_BYTES, &ratMask);
 *
 *       // Do something with the operator details.
 *
 *       opRef = taf_radio_GetNextPreferredOperator(listRef);
 *   }
 *
 *   // Delete list.
 *   taf_radio_DeletePreferredOperatorsList(listRef);
 *
 *   // Remove preference.
 *   for (int i = 0; i < PREFERRED_OPERATOR_NUM; i++) {
 *       taf_radio_RemovePreferredOperator(prefMccStr[i], prefMncStr[i], phoneId);
 *   }
 *
 *   @endcode
 *
 * @section c_taf_radio_rat Radio Access Technology
 *
 * Users can get RAT in use, and configure RAT preferences.
 *
 *  - taf_radio_GetRadioAccessTechInUse() -- Gets radio access technology in use.
 *
 *  - taf_radio_AddRatChangeHandler() / taf_radio_RemoveRatChangeHandler() -- Adds/removes
 *    RAT change handler.
 *
 *  - taf_radio_SetRatPreferences() / taf_radio_GetRatPreferences() -- Sets/gets RAT peferences.
 *
 * The following example illustrates RAT change indication.
 *
 * @code
 *
 *   // RAT change indication handler.
 *   static void RatChangeHandler(const taf_radio_RatChangeInd_t* indPtr, void* contextPtr)
 *   {
 *       LE_INFO("RAT: %d", indPtr->rat);
 *   }
 *
 *   // Add handler for RAT change.
 *   handlerRef = taf_radio_AddRatChangeHandler((taf_radio_RatChangeHandlerFunc_t)RatChangeHandler, NULL);
 *
 *   // RAT change in some cases.
 *
 *   @endcode
 *
 * @section c_taf_radio_ps Packet-Switched State
 *
 * Users can get the packet-switched (PS) state.
 *
 * - taf_radio_GetPacketSwitchedState() -- Gets the PS state.
 *
 * - taf_radio_AddPacketSwitchedChangeHandler() / taf_radio_RemovePacketSwitchedChangeHandler() -- Adds/removes
 *   PS change handler.
 *
 * The following example illustrates the PS change indication.
 *
 * @code
 *
 *   // PS change indication handler.
 *   static void PsChangeHandler(const taf_radio_NetRegState_t state, void* contextPtr)
 *   {
 *       LE_INFO("PS state: %d", state);
 *   }
 *
 *   // Add handler for PS change.
 *   handlerRef = taf_radio_AddPacketSwitchedChangeHandler(
 *      (taf_radio_PacketSwitchedChangeHandlerFunc_t)PsChangeHandler, NULL);
 *
 *   // PS change in some cases.
 *
 *   @endcode
 *
 * @section c_taf_radio_signal_metrics Signal Metrics
 *
 * Users can measure signal and get metrics for different RATs.
 *
 *  - taf_radio_GetSignalQual() -- Gets the highest signal strength level.
 *
 *  - taf_radio_MeasureSignalMetrics() / taf_radio_DeleteSignalMetrics() -- Creates/deletes the signal
 *    metrics reference.
 *
 *  - taf_radio_GetRatOfSignalMetrics() -- Gets RATs of signal metrics.
 *
 *  - taf_radio_GetGsmSignalMetrics() / taf_radio_GetUmtsSignalMetrics() / taf_radio_GetLteSignalMetrics() /
 *    taf_radio_GetCdmaSignalMetrics() / taf_radio_GetNr5gSignalMetrics() -- Gets
 *    GSM/UMTS/LTE/CDMA/NR5G signal metrics, these APIs should be called after calling
 *    taf_radio_GetRatOfSignalMetrics to get the valid RAT. If the return value is
 *    INVALID_SIGNAL_STRENGTH_VALUE, it indicates that the signal metrics are unknown or not
 *    detectable.
 *
 *  - taf_radio_AddSignalStrengthChangeHandler() / taf_radio_RemoveSignalStrengthChangeHandler() --
 *    Adds/removes signal strength change handler.
 *
 *  - taf_radio_SetSignalStrengthIndThresholds() / taf_radio_SetSignalStrengthIndDelta()
 *    -- Sets signal strength reporting criteria. Either the threshold or delta can be set at a
 *    time.
 *
 *  - taf_radio_SetSignalStrengthIndHysteresis() / taf_radio_SetSignalStrengthIndHysteresisTimer()
 *    -- Sets the optional parameters for signal strength reporting criteria. Hysteresis is
 *    applicable only when threshold is set. Hysteresis timer is applicable for both threshold
 *    and delta.This API needs to be called before taf_radio_SetSignalStrengthIndThresholds() and
 *    taf_radio_SetSignalStrengthIndDelta().
 *
 * The following example illustrates getting LTE signal metrics.
 *
 * @code
 *
 *   metricRef = taf_radio_MeasureSignalMetrics(phoneId);
 *   ratMask = taf_radio_GetRatOfSignalMetrics(metricRef);
 *   if (ratMask & TAF_RADIO_RAT_BIT_MASK_LTE) {
 *       if (taf_radio_GetLteSignalMetrics(metricsRef, &ss, &rsrq, &rsrp, &snr) == LE_OK) {
 *           LE_INFO("LTE signal strength in dBm: %d\n", ss);
 *           LE_INFO("LTE reference signal receive quality in dB : %d\n", rsrq);
 *           LE_INFO("LTE reference signal receive power in dBm : %d\n", rsrp);
 *           LE_INFO("LTE signal-to-noise ratio in 0.1 dB : %d\n", snr);
 *       } else {
 *           LE_ERROR("Fail to get LTE signal metrics.");
 *       }
 *   }
 *
 *   @endcode
 *
 *
 * The following example illustrates setting the signal strength reporting criteria.
 *
 * The signal strength notifications are sent based on the configurations of delta or threshold on
 * the RAT(s) list. Additionally, the hysteresis dB can be applied on top of the threshold list.
 * Furthermore, time hysteresis (hysteresis ms) can be applied either on top of the delta or on the
 * threshold list, or even on top of both the threshold list and the hysteresis dB.
 *
 * For NR5G and LTE only RSRP change will be notified to clients.
 * For other RATs only RSSI change will be notified to clients.
 *
 * - Delta (delta): A notification is sent when the difference between the current
 * signal strength value and the last reported signal strength value crosses the specified
 * delta.
 * The value should be a non-zero positive integer, in units of 0.1dBm. For example, to set a
 * delta of 10dBm, the value should be 100.
 *
 * The default values for delta is as follows.
 * Measurement type      : value
 * RSSI_DELTA            : 50 (in dBm)
 * RSRP_DELTA            : 60 (in dBm)
 *
 * - Threshold (lowerRange/upperRange): A notification is sent when the current signal strength
 * crosses over or under any of the thresholds specified.
 * For example, to set thresholds at -95 dBm and -80 dBm, the threshold list values are -950,
 * -800, since the listed values are in units of 0.1 dBm.
 *
 * - Hysteresis dB (optional): Prevents the generation of multiple notifications when
 * the signal strength is close to a threshold value and experiencing frequent small changes.
 * With a non-zero hysteresis, the signal strength indicators should cross over or under by
 * more than the hysteresis value for a notification to be sent.
 * To apply hysteresis, the value should be a non-zero positive integer, in units of 0.1 dBm.
 * For example, to set a hysteresis dB of 10 dBm, the value should be 100.
 *
 * - Hysteresis ms (optional): Time hystersis can be applied to avoid multiple
 * notifications even when all the other criteria for a notification are met. The time
 * hystersis can be applied on top of any other criteria (delta, threshold, threshold and
 * hysteresis).
 *
 * If the hysteresis(dB or ms) value is set to 0, the signal strength notification criteria just
 * considers the threshold or delta. Once configured, the hysteresis value for a signal strength
 * type is retained, until explicitly reconfigured to 0 again or device reboot.
 * This configuration is a global setting. The signal strength setting does not persist through
 * device reboot and needs to be configured again. Default signal strength configuration is set
 * after a device reboot.
 *
 *
 * @code
 *
 *   // Set the optional hysteresis time parameter. Applicable to delta and threshold.
 *   // Is a global variable and once set will be applicable to all RATs.
 *   taf_radio_SetSignalStrengthIndHysteresisTimer(hysteresis ms,phoneId);
 *   // Set the optional hysteresis dBm parameter. Applicable to threshold.
 *   // Is applicable to a sigType.
 *   taf_radio_SetSignalStrengthIndHysteresis(sigType,hysteresis dB,phoneId);
 *
 *   // Call below API to set the signal strength for threshold criteria.
 *   taf_radio_SetSignalStrengthIndThresholds(sigType,lowerRange,upperRange,phoneId);
 *   // Call below API to set the signal strength for delta criteria.
 *   taf_radio_SetSignalStrengthIndDelta(sigType,delta,phoneId);
 *
 *
 *   @endcode
 *
 * @section c_taf_radio_serving_cell Serving Cellular Network Information
 *
 * Users can get serving cellular network information.
 *
 *  - taf_radio_GetServingCellId() -- Gets the cell ID.
 *    @b WARNING: Only applicable for GSM/LTE/WCDMA/TDSCDMA.
 *
 *  - taf_radio_GetServingCellLocAreaCode() -- Gets the location area code.
 *     @b WARNING: Only applicable for  GSM/WCDMA/TDSCDMA.
 *
 *  - taf_radio_GetServingCellLteTracAreaCode() -- Gets the tracking area code.
 *     @b WARNING: Only applicable for LTE.
 *
 *  - taf_radio_GetServingCellEarfcn() -- Gets the E-UTRA absolute radio frequency channel number.
 *     @b WARNING: Only applicable for LTE.
 *
 *  - taf_radio_GetServingCellTimingAdvance() -- Gets the timing advance.
 *     @b WARNING: Only applicable for GSM/LTE.
 *
 *  - taf_radio_GetPhysicalServingLteCellId() -- Gets the physical cell ID.
 *     @b WARNING: Only applicable for LTE.
 *
 *  - taf_radio_GetServingCellGsmBsic() -- Gets the base station ID.
 *     @b WARNING: Only applicable for GSM.
 *
 *  - taf_radio_GetServingCellScramblingCode() -- Gets the primary scrambling code.
 *     @b WARNING: Only applicable for WCDMA.
 *
 *  - taf_radio_GetServingCellBandInfo() -- Gets 2G/3G band information.
 *     @b WARNING: Only applicable for GSM/WCDMA/TDSCDMA.
 *
 *  - taf_radio_GetServingCellLteBandInfo() -- Gets LTE band information.
 *     @b WARNING: Only applicable for LTE.
 *
 *  - taf_radio_GetServingCellNrBandInfo() -- Gets NR band information.
 *     @b WARNING: Only applicable for NR.
 *
 *  - taf_radio_GetNrIconType() -- Gets NR icon type.
 *     @b WARNING: Applicable for NR (basic or uwb) and also appliable for LTE and LTE will return NONE.
 *
 * @section c_taf_radio_network Network Information
 *
 * Users can use the following APIs to get the current network and discover nearby networks
 * by scanning.
 *
 *  - taf_radio_GetCurrentNetworkName() -- Gets the current network name.
 *
 *  - taf_radio_GetCurrentNetworkMccMnc() -- Gets the current network MCC and MNC.
 *
 *  - taf_radio_PerformCellularNetworkScan() / taf_radio_DeleteCellularNetworkScan() -- Creates/deletes
 *    a list reference for network scanning.
 *
 *  - taf_radio_PerformCellularNetworkScanAsync() -- Asynchronously performs a network scan with a handler.
 *
 *  - taf_radio_GetFirstCellularNetworkScan() / taf_radio_GetNextCellularNetworkScan() -- Gets
 *    the first/next operator reference in the network scan list.
 *
 *  - taf_radio_GetCellularNetworkMccMnc() -- Gets MCC and MNC of a network scan operator.
 *
 *  - taf_radio_GetCellularNetworkName() -- Gets the name of a network scan operator.
 *
 *  - taf_radio_IsCellularNetworkInUse() -- Checks if using the network scan operator's network.
 *
 *  - taf_radio_IsCellularNetworkAvailable() -- Checks if the network is available.
 *
 *  - taf_radio_IsCellularNetworkHome() -- Checks if it is the home network.
 *
 *  - taf_radio_IsCellularNetworkForbidden() -- Checks if the network is forbidden.
 *
 * When users fail to register to the current network, they may scan and find the nearby networks, and register to
 * an available network with MCC and MNC manually. The following example illustrates network scanning.
 *
 * @code
 *
 *   // Create list and perform network scan.
 *   listRef = taf_radio_PerformCellularNetworkScan(phoneId);
 *
 *   // Traverse list
 *   opRef = taf_radio_GetFirstCellularNetworkScan(listRef);
 *   while (opRef != nullptr) {
 *       if (taf_radio_GetCellularNetworkName(opRef, name, TAF_RADIO_NETWORK_NAME_MAX_LEN) != LE_OK) {
 *           LE_ERROR("Fail to get network name.");
 *           break;
 *       }
 *
 *       if (taf_radio_GetCellularNetworkMccMnc(opRef, mcc, TAF_RADIO_MCC_BYTES,
 *           mnc, TAF_RADIO_MNC_BYTES) != LE_OK) {
 *           LE_ERROR("Fail to get network MCC and MNC.");
 *           break;
 *       }
 *
 *       inUse = taf_radio_IsCellularNetworkInUse(opRef);
 *       available = taf_radio_IsCellularNetworkAvailable(opRef);
 *       fobbiden = taf_radio_IsCellularNetworkForbidden(opRef);
 *       home = taf_radio_IsCellularNetworkHome(opRef);
 *
 *       // Do something with the network scan information.
 *
 *       opRef = taf_radio_GetNextCellularNetworkScan(listRef);
 *   }
 *
 *   // Delete list.
 *   taf_radio_DeleteCellularNetworkScan(listRef);
 *
 *   @endcode
 *
 * @section c_taf_radio_ratCapability RAT Capability Information
 *
 * Users can get the number of SIMs and RAT capabilities in given slot.
 *
 *
 *  - taf_radio_GetHardwareSimConfig() --
 *    Gets the maximum number of SIMs that can be supported simultaneously and the maximum number
 *    of SIMs that can be simultaneously active.If the maximum active SIM number is
 *    less than the maximum number, any combination of the SIMs can be active and the remaining
 *    can be in standby.
 *
 *  - taf_radio_GetHardwareSimRatCapabilities() --
 *    simRatCapMask - Gets the supported SIM RAT capabilities which intersects with the device
 *    supported RAT capabilities and the device supported RAT capabilities in given slot.
 *    deviceRatCapMask - Gets the supported device RAT capabilities supported in given slot.
 *
 * The following example illustrates the API usage.
 *
 * @code
 *
 *
 *   // Gets sim count
 *   uint8_t totalSimCount = 0;
 *   uint8_t maxActiveSims = 0;
 *   le_result_t result = taf_radio_GetHardwareSimConfig(&totalSimCount,&maxActiveSims);
 *
 *   // Gets RAT Capabilities
 *   taf_radio_RatBitMask_t deviceRatCapMask;
 *   taf_radio_RatBitMask_t simRatCapMask;
 *   result = taf_radio_GetHardwareSimRatCapabilities(&deviceRatCapMask,&simRatCapMask,phoneId);
 *
 *   @endcode
 *
 * @section c_taf_radio_ims IMS (IP Multimedia Subsystem) Information
 *
 * Users can set and get the IMS registration status, service status and configurations in given slot.
 *
 *  - taf_radio_GetImsRegStatus() -- Gets the IMS registration status in given slot.
 *
 *  - taf_radio_GetImsSvcStatus() -- Gets the IMS service status (including Voice over IMS and SMS) in given slot.
 *
 *  - taf_radio_SetImsSvcCfg() -- Sets the IMS service configurations (including IMS, Voice over IMS, Voice
 *     over NR, SMS over IMS, RTT) in given slot.
 *    @b WARNING: Disabling Voice over NR will result in the user equipment (UE) defaulting to support NR5G voice
 *       over EPS fallback. To enable Voice over NR, both IMS and Voice over IMS must be enabled.
 *
 *  - taf_radio_GetImsSvcCfg() -- Gets the IMS service (IMS, Voice over IMS, Voice over NR, SMS over IMS, RTT)
 *    configurations in given slot.
 *
 *  - taf_radio_SetImsUserAgent() -- Sets the IMS SIP User Agent configuration in given slot.
 *
 *  - taf_radio_GetImsUserAgent() -- Gets the IMS SIP User Agent configuration in given slot.
 *
 *  - taf_radio_GetImsPdpError() -- Gets the IMS PDP error in given slot.
 *
 *  - taf_radio_AddImsRegStatusChangeHandler() / taf_radio_RemoveImsRegStatusChangeHandler() -- Adds/removes
 *    handler for IMS registration status change.
 *
 *  - taf_radio_AddImsStatusChangeHandler() / taf_radio_RemoveImsStatusChangeHandler() -- Adds/removes
 *    handler for IMS status change.
 *
 * The following example illustrates the API usage.
 *
 * @code
 *
 *   // Sets IMS Voice over NR configuration.
 *   bool enable = false;
 *   taf_radio_ImsRef_t imsRef = taf_radio_GetIms(phoneId);
 *   le_result_t result = taf_radio_SetImsSvcCfg(imsRef, TAF_RADIO_IMS_SVC_TYPE_VONR, enable);
 *   if (result == LE_OK)
 *   {
 *       LE_INFO("VoNR has been successfully disabled.");
 *   } else {
 *       LE_ERROR("Failed to disable VoNR.");
 *   }
 *
 *   // Gets IMS Voice over NR configuration.
 *   result = taf_radio_GetImsSvcCfg(imsRef, TAF_RADIO_IMS_SVC_TYPE_VONR, &enable);
 *   if (result == LE_OK)
 *   {
 *       if (enable)
 *       {
 *           LE_INFO("VoNR is enabled");
 *       } else {
 *           LE_INFO("VoNR is disabled");
 *       }
 *   } else {
 *       LE_ERROR("Failed to get the configuration of VoNR.");
 *   }
 *   @endcode
 * <HR>
 *
 */


#include "legato.h"
#include "interfaces.h"

#include "jansson.h"
#include <arpa/inet.h>

typedef struct
{
    uint8_t phoneId;
    taf_radio_ImsRegStatus_t status;
    taf_radio_ImsIndBitMask_t bitmask;
    taf_radio_ImsRef_t imsRef;
} taf_RadioImsStatus_t;

typedef struct Header_ {
    uint16_t length;
    uint16_t type;
} Header_t;


#define RADIO_EVENT_NODE "/tmp/radio_event"
static int RadioEventNodeFd = -1;
static int DummyFd = -1;
static le_fdMonitor_Ref_t RadioEventMonitor;
static le_mem_PoolRef_t ImsEvtPool = NULL;
static le_event_Id_t ImsEventId;

static taf_radio_ImsSvcStatus_t ImsStatus = TAF_RADIO_IMS_SVC_STATUS_UNKNOWN;

//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for EVENT 'taf_radio_NetRegReject'
 *
 * Event to report network registration rejection.
 */
//--------------------------------------------------------------------------------------------------
taf_radio_NetRegRejectHandlerRef_t taf_radio_AddNetRegRejectHandler
(
    taf_radio_NetRegRejectHandlerFunc_t handlerPtr,
        ///< [IN] Handler for network registration rejection.
    void* contextPtr
        ///< [IN]
)
{
    return NULL;
}
//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'taf_radio_NetRegReject'
 */
//--------------------------------------------------------------------------------------------------
void taf_radio_RemoveNetRegRejectHandler
(
    taf_radio_NetRegRejectHandlerRef_t handlerRef
        ///< [IN]
)
{
}
//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for EVENT 'taf_radio_RatChange'
 *
 * Event to report RAT change.
 */
//--------------------------------------------------------------------------------------------------
taf_radio_RatChangeHandlerRef_t taf_radio_AddRatChangeHandler
(
    taf_radio_RatChangeHandlerFunc_t handlerPtr,
        ///< [IN] Handler for RAT change.
    void* contextPtr
        ///< [IN]
)
{
    return NULL;
}
//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'taf_radio_RatChange'
 */
//--------------------------------------------------------------------------------------------------
void taf_radio_RemoveRatChangeHandler
(
    taf_radio_RatChangeHandlerRef_t handlerRef
        ///< [IN]
)
{
}
//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for EVENT 'taf_radio_NetRegStateEvent'
 *
 * Event to report network registration state.
 */
//--------------------------------------------------------------------------------------------------
taf_radio_NetRegStateEventHandlerRef_t taf_radio_AddNetRegStateEventHandler
(
    taf_radio_NetRegStateHandlerFunc_t handlerPtr,
        ///< [IN] Handler for network registration state.
    void* contextPtr
        ///< [IN]
)
{
    return NULL;
}
//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'taf_radio_NetRegStateEvent'
 */
//--------------------------------------------------------------------------------------------------
void taf_radio_RemoveNetRegStateEventHandler
(
    taf_radio_NetRegStateEventHandlerRef_t handlerRef
        ///< [IN]
)
{
}
//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for EVENT 'taf_radio_PacketSwitchedChange'
 *
 * Event to report packet switched state changes.
 */
//--------------------------------------------------------------------------------------------------
taf_radio_PacketSwitchedChangeHandlerRef_t taf_radio_AddPacketSwitchedChangeHandler
(
    taf_radio_PacketSwitchedChangeHandlerFunc_t handlerPtr,
        ///< [IN] Handler for packet switched state changes.
    void* contextPtr
        ///< [IN]
)
{
    return NULL;
}
//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'taf_radio_PacketSwitchedChange'
 */
//--------------------------------------------------------------------------------------------------
void taf_radio_RemovePacketSwitchedChangeHandler
(
    taf_radio_PacketSwitchedChangeHandlerRef_t handlerRef
        ///< [IN]
)
{
}
//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for EVENT 'taf_radio_NrIconType'
 *
 * Event to report NR icon type changes.
 */
//--------------------------------------------------------------------------------------------------
taf_radio_NrIconTypeHandlerRef_t taf_radio_AddNrIconTypeHandler
(
    taf_radio_NrIconTypeHandlerFunc_t handlerPtr,
        ///< [IN] Handler for NR icon type changes.
    void* contextPtr
        ///< [IN]
)
{
    return NULL;
}
//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'taf_radio_NrIconType'
 */
//--------------------------------------------------------------------------------------------------
void taf_radio_RemoveNrIconTypeHandler
(
    taf_radio_NrIconTypeHandlerRef_t handlerRef
        ///< [IN]
)
{
}
//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for EVENT 'taf_radio_SignalStrengthChange'
 *
 * Event to report signal strength change.
 */
//--------------------------------------------------------------------------------------------------
taf_radio_SignalStrengthChangeHandlerRef_t taf_radio_AddSignalStrengthChangeHandler
(
    taf_radio_Rat_t rat,
        ///< [IN] Radio Access Technology.
    taf_radio_SignalStrengthChangeHandlerFunc_t handlerPtr,
        ///< [IN] Handler for signal strength change.
    void* contextPtr
        ///< [IN]
)
{
    return NULL;
}
//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'taf_radio_SignalStrengthChange'
 */
//--------------------------------------------------------------------------------------------------
void taf_radio_RemoveSignalStrengthChangeHandler
(
    taf_radio_SignalStrengthChangeHandlerRef_t handlerRef
        ///< [IN]
)
{
}
//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for EVENT 'taf_radio_ImsRegStatusChange'
 *
 * Event to report IMS registration status.
 */
//--------------------------------------------------------------------------------------------------
taf_radio_ImsRegStatusChangeHandlerRef_t taf_radio_AddImsRegStatusChangeHandler
(
    taf_radio_ImsRegStatusChangeHandlerFunc_t handlerPtr,
        ///< [IN] Handler for IMS registration status change.
    void* contextPtr
        ///< [IN]
)
{
    return NULL;
}
//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'taf_radio_ImsRegStatusChange'
 */
//--------------------------------------------------------------------------------------------------
void taf_radio_RemoveImsRegStatusChangeHandler
(
    taf_radio_ImsRegStatusChangeHandlerRef_t handlerRef
        ///< [IN]
)
{
}
//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for EVENT 'taf_radio_OpModeChange'
 *
 * Event to report operating mode changes.
 */
//--------------------------------------------------------------------------------------------------
taf_radio_OpModeChangeHandlerRef_t taf_radio_AddOpModeChangeHandler
(
    taf_radio_OpModeChangeHandlerFunc_t handlerPtr,
        ///< [IN] Handler for operating mode changes.
    void* contextPtr
        ///< [IN]
)
{
    return NULL;
}
//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'taf_radio_OpModeChange'
 */
//--------------------------------------------------------------------------------------------------
void taf_radio_RemoveOpModeChangeHandler
(
    taf_radio_OpModeChangeHandlerRef_t handlerRef
        ///< [IN]
)
{
}
//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for EVENT 'taf_radio_NetStatusChange'
 *
 * Event to report network status changes.
 */
//--------------------------------------------------------------------------------------------------
taf_radio_NetStatusChangeHandlerRef_t taf_radio_AddNetStatusChangeHandler
(
    taf_radio_NetStatusHandlerFunc_t handlerPtr,
        ///< [IN] Handler for network status changes.
    void* contextPtr
        ///< [IN]
)
{
    return NULL;
}
//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'taf_radio_NetStatusChange'
 */
//--------------------------------------------------------------------------------------------------
void taf_radio_RemoveNetStatusChangeHandler
(
    taf_radio_NetStatusChangeHandlerRef_t handlerRef
        ///< [IN]
)
{
}

static void LayerImsStateHandler
(
    void* reportPtr,       ///< [IN] Report pointer.
    void* layerHandlerFunc ///< [IN] Layered function.
)
{
    if (reportPtr == NULL)
    {
        LE_INFO("Null (reportPtr)");
        return;
    }

    taf_radio_ImsStatusChangeHandlerFunc_t handlerFunc =
        (taf_radio_ImsStatusChangeHandlerFunc_t)layerHandlerFunc;

    taf_RadioImsStatus_t* statusPtr = (taf_RadioImsStatus_t*)reportPtr;
    if (handlerFunc)
    {
        LE_INFO("[simulation]: callback !");
        handlerFunc(statusPtr->imsRef, statusPtr->bitmask, statusPtr->phoneId,
            le_event_GetContextPtr());
    }

    le_mem_Release(reportPtr);
}
//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for EVENT 'taf_radio_ImsStatusChange'
 *
 * Event to report IMS status.
 */
//--------------------------------------------------------------------------------------------------
taf_radio_ImsStatusChangeHandlerRef_t taf_radio_AddImsStatusChangeHandler
(
    taf_radio_ImsStatusChangeHandlerFunc_t handlerPtr,
        ///< [IN] Handler for IMS status change.
    void* contextPtr
        ///< [IN]
)
{
    LE_INFO("[simulation]: add ims status change hdlr");
    le_event_HandlerRef_t handlerRef =
        le_event_AddLayeredHandler("Ims-Evt-Layer-Hdlr",
                                    ImsEventId, LayerImsStateHandler,
                                    (void*)handlerPtr);
    le_event_SetContextPtr(handlerRef, contextPtr);

    return (taf_radio_ImsStatusChangeHandlerRef_t)(handlerRef);
}
//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'taf_radio_ImsStatusChange'
 */
//--------------------------------------------------------------------------------------------------
void taf_radio_RemoveImsStatusChangeHandler
(
    taf_radio_ImsStatusChangeHandlerRef_t handlerRef
        ///< [IN]
)
{
    LE_INFO("[simulation]: remove ims status change hdlr");
    le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);
}
//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for EVENT 'taf_radio_CellInfoChange'
 *
 * Event to report cell info change.
 */
//--------------------------------------------------------------------------------------------------
taf_radio_CellInfoChangeHandlerRef_t taf_radio_AddCellInfoChangeHandler
(
    taf_radio_CellInfoChangeHandlerFunc_t handlerPtr,
        ///< [IN] Handler for registered cell info change.
    void* contextPtr
        ///< [IN]
)
{
    return NULL;
}
//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'taf_radio_CellInfoChange'
 */
//--------------------------------------------------------------------------------------------------
void taf_radio_RemoveCellInfoChangeHandler
(
    taf_radio_CellInfoChangeHandlerRef_t handlerRef
        ///< [IN]
)
{
}
//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for EVENT 'taf_radio_CAInfo'
 *
 * Event to report Carrier Aggregation(CA) information.
 */
//--------------------------------------------------------------------------------------------------
taf_radio_CAInfoHandlerRef_t taf_radio_AddCAInfoHandler
(
    taf_radio_Rat_t rat,
        ///< [IN] Radio Access Technology.
    taf_radio_CAInfoHandlerFunc_t handlerPtr,
        ///< [IN] Handler for CA information.
    void* contextPtr
        ///< [IN]
)
{
    return NULL;
}
//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'taf_radio_CAInfo'
 */
//--------------------------------------------------------------------------------------------------
void taf_radio_RemoveCAInfoHandler
(
    taf_radio_CAInfoHandlerRef_t handlerRef
        ///< [IN]
)
{
}
//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for EVENT 'taf_radio_ConnectionStatus'
 *
 * Event to report connection status.
 */
//--------------------------------------------------------------------------------------------------
taf_radio_ConnectionStatusHandlerRef_t taf_radio_AddConnectionStatusHandler
(
    taf_radio_ConnectionStatusHandlerFunc_t handlerPtr,
        ///< [IN] Handler for connection status.
    void* contextPtr
        ///< [IN]
)
{
    return NULL;
}
//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'taf_radio_ConnectionStatus'
 */
//--------------------------------------------------------------------------------------------------
void taf_radio_RemoveConnectionStatusHandler
(
    taf_radio_ConnectionStatusHandlerRef_t handlerRef
        ///< [IN]
)
{
}
//--------------------------------------------------------------------------------------------------
/**
 * Sets the radio power state.
 *
 * @return
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------

static le_onoff_t power_status = LE_OFF;

le_result_t taf_radio_SetRadioPower
(
    le_onoff_t power,
        ///< [IN] Power state.
    uint8_t phoneId
        ///< [IN] Phone ID.
)
{
    // Z:FIXME
    power_status = power;
    return LE_OK;
}
//--------------------------------------------------------------------------------------------------
/**
 *  Gets the radio power state.
 *
 * @return
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_GetRadioPower
(
    le_onoff_t* powerPtr,
        ///< [OUT] Power state.
    uint8_t phoneId
        ///< [IN] Phone ID.
)
{
    *powerPtr = power_status;
    return LE_OK;
}
//--------------------------------------------------------------------------------------------------
/**
 *  Registers to network using automatic mode.
 *
 * @return
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_TIMEOUT -- Response time out.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_SetAutomaticRegisterMode
(
    uint8_t phoneId
        ///< [IN] Phone ID.
)
{
    // Z:FIXME
    return LE_OK;
}
//--------------------------------------------------------------------------------------------------
/**
 *  Registers to network using manual mode.
 *
 * @return
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_TIMEOUT -- Response time out.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_SetManualRegisterMode
(
    const char* LE_NONNULL mcc,
        ///< [IN] MCC.
    const char* LE_NONNULL mnc,
        ///< [IN] MNC.
    uint8_t phoneId
        ///< [IN] Phone ID.
)
{
    return LE_NOT_IMPLEMENTED;
}
//--------------------------------------------------------------------------------------------------
/**
 *  Registers to network asynchronously using manual mode.
 */
//--------------------------------------------------------------------------------------------------
void taf_radio_SetManualRegisterModeAsync
(
    const char* LE_NONNULL mcc,
        ///< [IN] MCC.
    const char* LE_NONNULL mnc,
        ///< [IN] MNC.
    taf_radio_ManualSelectionHandlerFunc_t handlerPtr,
        ///< [IN] Handler for manual selection.
    void* contextPtr,
        ///< [IN]
    uint8_t phoneId
        ///< [IN] Phone ID.
)
{
}
//--------------------------------------------------------------------------------------------------
/**
 *  Gets the network registration mode.
 *
 * @return
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_TIMEOUT -- Response time out.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_GetRegisterMode
(
    bool* isManualPtrPtr,
        ///< [OUT] True if manual, false if automatic.
    char* mccPtr,
        ///< [OUT] MCC.
    size_t mccPtrSize,
        ///< [IN]
    char* mncPtr,
        ///< [OUT] MNC.
    size_t mncPtrSize,
        ///< [IN]
    uint8_t phoneId
        ///< [IN] Phone ID.
)
{
    // Z:FIXME
    *isManualPtrPtr = false;
    return LE_OK;
}
//--------------------------------------------------------------------------------------------------
/**
 *  Gets platform-specific network registration error code.
 *
 * @return
 * Platform-specific network registration error code.
 */
//--------------------------------------------------------------------------------------------------
int32_t taf_radio_GetPlatformSpecificRegistrationErrorCode
(
    void
)
{
    return 0;
}
//--------------------------------------------------------------------------------------------------
/**
 * Gets 2G/3G band capabilities.
 *
 * @return
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_GetBandCapabilities
(
    taf_radio_BandBitMask_t* bandMaskPtrPtr,
        ///< [OUT] 2G/3G band capabilities.
    uint8_t phoneId
        ///< [IN] Phone ID.
)
{
    return LE_NOT_IMPLEMENTED;
}
//--------------------------------------------------------------------------------------------------
/**
 * Gets LTE band capabilities.
 *
 * @return
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_GetLteBandCapabilities
(
    uint64_t* bandMaskPtrPtr,
        ///< [OUT] LTE band capabilities.
    size_t* bandMaskPtrSizePtr,
        ///< [INOUT]
    uint8_t phoneId
        ///< [IN] Phone ID.
)
{
    return LE_NOT_IMPLEMENTED;
}
//--------------------------------------------------------------------------------------------------
/**
 * Sets 2G/3G band preferences.
 *
 * @return
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_SetBandPreferences
(
    taf_radio_BandBitMask_t bandMask,
        ///< [IN] 2G/3G band preferences.
    uint8_t phoneId
        ///< [IN] Phone ID.
)
{
    return LE_NOT_IMPLEMENTED;
}
//--------------------------------------------------------------------------------------------------
/**
 * Gets 2G/3G band preferences.
 *
 * @return
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_GetBandPreferences
(
    taf_radio_BandBitMask_t* bandMaskPtrPtr,
        ///< [OUT] 2G/3G band preferences.
    uint8_t phoneId
        ///< [IN] Phone ID.
)
{
    return LE_NOT_IMPLEMENTED;
}
//--------------------------------------------------------------------------------------------------
/**
 * Sets LTE band preferences.
 *
 * @return
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_SetLteBandPreferences
(
    const uint64_t* bandMaskPtr,
        ///< [IN] LTE band preferences.
    size_t bandMaskSize,
        ///< [IN]
    uint8_t phoneId
        ///< [IN] Phone ID.
)
{
    return LE_NOT_IMPLEMENTED;
}
//--------------------------------------------------------------------------------------------------
/**
 * Gets LTE band preferences.
 *
 * @return
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_GetLteBandPreferences
(
    uint64_t* bandMaskPtr,
        ///< [OUT] LTE band preferences.
    size_t* bandMaskSizePtr,
        ///< [INOUT]
    uint8_t phoneId
        ///< [IN] Phone ID.
)
{
    return LE_NOT_IMPLEMENTED;
}
//--------------------------------------------------------------------------------------------------
/**
 *  Adds a preferred operator.
 *
 * @return
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_TIMEOUT -- Response timed out.
 *  - LE_DUPLICATE -- Duplicate preferred operator.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_AddPreferredOperator
(
    const char* LE_NONNULL mcc,
        ///< [IN] MCC.
    const char* LE_NONNULL mnc,
        ///< [IN] MNC.
    taf_radio_RatBitMask_t ratMask,
        ///< [IN] RAT bitmask, applicable for GSM/UMTS/LTE/NR5G.
    uint8_t phoneId
        ///< [IN] Phone ID.
)
{
    return LE_NOT_IMPLEMENTED;
}
//--------------------------------------------------------------------------------------------------
/**
 *  Removes a preferred operator.
 *
 * @return
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_TIMEOUT -- Response timed out.
 *  - LE_NOT_FOUND -- Preferred operator not found.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_RemovePreferredOperator
(
    const char* LE_NONNULL mcc,
        ///< [IN] MCC.
    const char* LE_NONNULL mnc,
        ///< [IN] MNC.
    uint8_t phoneId
        ///< [IN] Phone ID.
)
{
    return LE_NOT_IMPLEMENTED;
}
//--------------------------------------------------------------------------------------------------
/**
 *  Deletes the preferred operators list.
 *
 * @return
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_NOT_FOUND -- Preferred operator not found.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_DeletePreferredOperatorsList
(
    taf_radio_PreferredOperatorListRef_t preferredOperatorListRef
        ///< [IN] The preferred operators list reference.
)
{
    return LE_NOT_IMPLEMENTED;
}
//--------------------------------------------------------------------------------------------------
/**
 *  Creates a preferred operators list.
 *
 * @return
 *  - Non-null pointer -- The preferred operators list reference.
 *  - Null pointer -- Internal error or no preferred operators list.
 *
 * @b NOTE: At most 1 preferred operator list per slot.
 */
//--------------------------------------------------------------------------------------------------
taf_radio_PreferredOperatorListRef_t taf_radio_GetPreferredOperatorsList
(
    uint8_t phoneId
        ///< [IN] Phone ID.
)
{
    return NULL;
}
//--------------------------------------------------------------------------------------------------
/**
 *  Gets the first preferred operator.
 *
 * @return
 *  - Non-null pointer -- The first preferred operator reference.
 *  - Null pointer -- Internal error or empty list.
 */
//--------------------------------------------------------------------------------------------------
taf_radio_PreferredOperatorRef_t taf_radio_GetFirstPreferredOperator
(
    taf_radio_PreferredOperatorListRef_t preferredOperatorListRef
        ///< [IN] The preferred operators list reference.
)
{
    return NULL;
}
//--------------------------------------------------------------------------------------------------
/**
 *  Gets the next preferred operator based on the current position in the list.
 *
 * @return
 *  - Non-null pointer -- The next preferred operator reference.
 *  - Null pointer -- Internal error or empty list.
 */
//--------------------------------------------------------------------------------------------------
taf_radio_PreferredOperatorRef_t taf_radio_GetNextPreferredOperator
(
    taf_radio_PreferredOperatorListRef_t preferredOperatorListRef
        ///< [IN] The preferred operators list reference.
)
{
    return NULL;
}
//--------------------------------------------------------------------------------------------------
/**
 *  Gets the details of the given preferred operator.
 *
 * @return
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_NOT_FOUND -- Preferred operator not found.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_GetPreferredOperatorDetails
(
    taf_radio_PreferredOperatorRef_t preferredOperatorRef,
        ///< [IN] The preferred operator reference.
    char* mccPtr,
        ///< [OUT] MCC.
    size_t mccPtrSize,
        ///< [IN]
    char* mncPtr,
        ///< [OUT] MNC.
    size_t mncPtrSize,
        ///< [IN]
    taf_radio_RatBitMask_t* ratMaskPtr
        ///< [OUT] RAT bitmask.
)
{
    return LE_NOT_IMPLEMENTED;
}
//--------------------------------------------------------------------------------------------------
/**
 *  Gets the RAT in use.
 *
 * @return
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_TIMEOUT -- Response timed out.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_GetRadioAccessTechInUse
(
    taf_radio_Rat_t* ratPtr,
        ///< [OUT] RAT in use.
    uint8_t phoneId
        ///< [IN] Phone ID.
)
{
    return LE_NOT_IMPLEMENTED;
}
//--------------------------------------------------------------------------------------------------
/**
 *  Sets the RAT preferences.
 *
 * @return
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_SetRatPreferences
(
    taf_radio_RatBitMask_t ratMask,
        ///< [IN] Bitmask for RAT prefences.
    uint8_t phoneId
        ///< [IN] Phone ID.
)
{
    return LE_NOT_IMPLEMENTED;
}
//--------------------------------------------------------------------------------------------------
/**
 *  Gets the RAT preferences.
 *
 * @return
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_TIMEOUT -- Response timed out.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_GetRatPreferences
(
    taf_radio_RatBitMask_t* ratMaskPtrPtr,
        ///< [OUT] Bitmask for RAT prefences.
    uint8_t phoneId
        ///< [IN] Phone ID.
)
{
    return LE_NOT_IMPLEMENTED;
}
//--------------------------------------------------------------------------------------------------
/**
 *  Gets the network registration state.
 *
 * @return
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_TIMEOUT -- Response timed out.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_GetNetRegState
(
    taf_radio_NetRegState_t* statePtr,
        ///< [OUT] Network registration state.
    uint8_t phoneId
        ///< [IN] Phone ID.
)
{
    return LE_NOT_IMPLEMENTED;
}
//--------------------------------------------------------------------------------------------------
/**
 *  Gets the packet switch state.
 *
 * @return
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_TIMEOUT -- Response timed out.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_GetPacketSwitchedState
(
    taf_radio_NetRegState_t* statePtr,
        ///< [OUT] Packet switch state.
    uint8_t phoneId
        ///< [IN] Phone ID.
)
{
    *statePtr = TAF_RADIO_NET_REG_STATE_HOME;
    return LE_OK;
}
//--------------------------------------------------------------------------------------------------
/**
 *  Gets the registration domain for the current serving RAT.
 *
 * @return
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_GetServiceDomain
(
    taf_radio_ServiceDomainState_t* domainPtr,
        ///< [OUT] Service domain.
    uint8_t phoneId
        ///< [IN] Phone ID.
)
{
    return LE_NOT_IMPLEMENTED;
}
//--------------------------------------------------------------------------------------------------
/**
 *  Sets the service domain preferences.
 *
 * @return
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_TIMEOUT -- Response timed out.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_SetServiceDomainPreferences
(
    taf_radio_ServiceDomainState_t domain,
        ///< [IN] Service domain preferences.
    uint8_t phoneId
        ///< [IN] Phone ID.
)
{
    return LE_NOT_IMPLEMENTED;
}
//--------------------------------------------------------------------------------------------------
/**
 *  Gets the service domain preferences.
 *
 * @return
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_TIMEOUT -- Response timed out.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_GetServiceDomainPreferences
(
    taf_radio_ServiceDomainState_t* domainPtrPtr,
        ///< [OUT] Service domain preferences.
    uint8_t phoneId
        ///< [IN] Phone ID.
)
{
    return LE_NOT_IMPLEMENTED;
}
//--------------------------------------------------------------------------------------------------
/**
 * Sets signal strength indication thresholds.
 *
 * @return
 *  - LE_OK -- Succeeded.
 *  - LE_BAD_PARAMETER -- Bad parameters
 *  - LE_FAULT -- Failed.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_SetSignalStrengthIndThresholds
(
    taf_radio_SigType_t sigType,
        ///< [IN] Signal type.
    int32_t lowerRangeThreshold,
        ///< [IN] Lower range threshold in 0.1 dBm.
    int32_t upperRangeThreshold,
        ///< [IN] Upper range threshold in 0.1 dBm.
    uint8_t phoneId
        ///< [IN] Phone ID.
)
{
    return LE_NOT_IMPLEMENTED;
}
//--------------------------------------------------------------------------------------------------
/**
 * Sets signal strength indication delta.
 *
 * @return
 *  - LE_OK -- Succeeded.
 *  - LE_BAD_PARAMETER -- Bad parameters
 *  - LE_FAULT -- Failed.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_SetSignalStrengthIndDelta
(
    taf_radio_SigType_t sigType,
        ///< [IN] Signal type.
    uint16_t delta,
        ///< [IN] Delta in uints of 0.1 dBm
    uint8_t phoneId
        ///< [IN] Phone ID.
)
{
    return LE_NOT_IMPLEMENTED;
}
//--------------------------------------------------------------------------------------------------
/**
 *  Gets the signal quality.
 *
 * @return
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_TIMEOUT -- Response timed out.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_GetSignalQual
(
    uint32_t* qualityPtr,
        ///< [OUT] Signal qulity level, ranges from 1 to 5, 0 is unknown.
    uint8_t phoneId
        ///< [IN] Phone ID.
)
{
    return LE_NOT_IMPLEMENTED;
}
//--------------------------------------------------------------------------------------------------
/**
 *  Creates signal metrics.
 *
 * @return
 *  - Non-null pointer -- The signal metrics reference.
 *  - Null pointer -- Internal error.
 */
//--------------------------------------------------------------------------------------------------
taf_radio_MetricsRef_t taf_radio_MeasureSignalMetrics
(
    uint8_t phoneId
        ///< [IN] Phone ID.
)
{
    return NULL;
}
//--------------------------------------------------------------------------------------------------
/**
 *  Deletes signal metrics.
 *
 * @return
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_NOT_FOUND -- Preferred operator not found.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_DeleteSignalMetrics
(
    taf_radio_MetricsRef_t MetricsRef
        ///< [IN] The signal metrics reference.
)
{
    return LE_NOT_IMPLEMENTED;
}
//--------------------------------------------------------------------------------------------------
/**
 *  Gets RAT bitmask of signal metrics.
 *
 * @return
 *  - taf_radio_RatBitMask_t -- RAT bitmask of signal metrics.
 */
//--------------------------------------------------------------------------------------------------
taf_radio_RatBitMask_t taf_radio_GetRatOfSignalMetrics
(
    taf_radio_MetricsRef_t MetricsRef
        ///< [IN] The signal metrics reference.
)
{
    return (taf_radio_RatBitMask_t) 0;
}
//--------------------------------------------------------------------------------------------------
/**
 *  Gets GSM signal metrics.
 *
 * @return
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_NOT_FOUND -- Reference not found.
 *  - LE_UNAVAILABLE -- GSM unavailable.
 *  - LE_OK -- Succeeded.
 *
 * @deprecated The parameter @ber is deprecated.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_GetGsmSignalMetrics
(
    taf_radio_MetricsRef_t MetricsRef,
        ///< [IN] The signal metrics reference.
    int32_t* rssiPtr,
        ///< [OUT] Received signal strength indicator in dBm.
    uint32_t* berPtr
        ///< [OUT] Bit error rate, valid from 0 to 7.
)
{
    return LE_NOT_IMPLEMENTED;
}
//--------------------------------------------------------------------------------------------------
/**
 *  Gets UMTS signal metrics.
 *
 * @return
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_NOT_FOUND -- Reference not found.
 *  - LE_UNAVAILABLE -- UMTS unavailable.
 *  - LE_OK -- Succeeded.
 *
 * @deprecated The parameter @ber is deprecated.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_GetUmtsSignalMetrics
(
    taf_radio_MetricsRef_t MetricsRef,
        ///< [IN] The signal metrics reference.
    int32_t* ssPtr,
        ///< [OUT] Signal strength in dBm.
    uint32_t* berPtr,
        ///< [OUT] WCDMA bit error rate, valid from 0 to 7, 0x7FFFFFFF is unavailable.
    int32_t* rscpPtr
        ///< [OUT] Receive signal channel power in dBm.
)
{
    return LE_NOT_IMPLEMENTED;
}
//--------------------------------------------------------------------------------------------------
/**
 *  Gets LTE signal metrics.
 *
 * @return
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_NOT_FOUND -- Reference not found.
 *  - LE_UNAVAILABLE -- LTE unavailable.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_GetLteSignalMetrics
(
    taf_radio_MetricsRef_t MetricsRef,
        ///< [IN] The signal metrics reference.
    int32_t* ssPtr,
        ///< [OUT] Signal strength in dBm.
    int32_t* rsrqPtr,
        ///< [OUT] Reference signal receive quality in dB.
    int32_t* rsrpPtr,
        ///< [OUT] Reference signal receive power in dBm.
    int32_t* snrPtr
        ///< [OUT] Signal-to-noise ratio in units of 0.1 dB.
)
{
    return LE_NOT_IMPLEMENTED;
}
//--------------------------------------------------------------------------------------------------
/**
 *  Gets CDMA signal metrics.
 *
 * @return
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_NOT_FOUND -- Reference not found.
 *  - LE_UNAVAILABLE -- CDMA unavailable.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_GetCdmaSignalMetrics
(
    taf_radio_MetricsRef_t MetricsRef,
        ///< [IN] The signal metrics reference.
    int32_t* ssPtr,
        ///< [OUT] Signal strength in dBm.
    int32_t* ecioPtr,
        ///< [OUT] CDMA Ec/Io in dB.
    int32_t* snrPtr,
        ///< [OUT] EVDO signal-to-noise ratio in dB.
    int32_t* ioPtr
        ///< [OUT] EVDO Ec/Io in dB.
)
{
    return LE_NOT_IMPLEMENTED;
}
//--------------------------------------------------------------------------------------------------
/**
 *  Gets NR5G signal metrics.
 *
 * @return
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_NOT_FOUND -- Reference not found.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_GetNr5gSignalMetrics
(
    taf_radio_MetricsRef_t MetricsRef,
        ///< [IN] The signal metrics reference.
    int32_t* rsrqPtr,
        ///< [OUT] Reference Signal Receive Quality in dB.
    int32_t* rsrpPtr,
        ///< [OUT] Reference Signal Receive Power in dBm.
    int32_t* snrPtr
        ///< [OUT] Signal-to-Noise Ratio in units of 0.1 dB.
)
{
    return LE_NOT_IMPLEMENTED;
}
//--------------------------------------------------------------------------------------------------
/**
 *  Gets cell ID.
 *
 * @return
 *  - UINT32_MAX -- Internal error.
 *  - Others -- Cell ID.
 */
//--------------------------------------------------------------------------------------------------
uint32_t taf_radio_GetServingCellId
(
    uint8_t phoneId
        ///< [IN] Phone ID.
)
{
    return 0;
}
//--------------------------------------------------------------------------------------------------
/**
 *  Gets NR cell ID.
 *
 * @return
 *  - UINT64_MAX -- Internal error.
 *  - Others -- Cell ID.
 */
//--------------------------------------------------------------------------------------------------
uint64_t taf_radio_GetServingNrCellId
(
    uint8_t phoneId
        ///< [IN] Phone ID.
)
{
    return 0;
}
//--------------------------------------------------------------------------------------------------
/**
 *  Gets the location area code.
 *
 * @return
 *  - UINT32_MAX -- Internal error.
 *  - Others -- Location area code.
 */
//--------------------------------------------------------------------------------------------------
uint32_t taf_radio_GetServingCellLocAreaCode
(
    uint8_t phoneId
        ///< [IN] Phone ID.
)
{
    return 0;
}
//--------------------------------------------------------------------------------------------------
/**
 *  Gets the LTE tracking area code.
 *
 * @return
 *  - UINT16_MAX -- Internal error.
 *  - Others -- LTE tracking area code.
 */
//--------------------------------------------------------------------------------------------------
uint16_t taf_radio_GetServingCellLteTracAreaCode
(
    uint8_t phoneId
        ///< [IN] Phone ID.
)
{
    return 0;
}
//--------------------------------------------------------------------------------------------------
/**
 *  Gets the NR tracking area code.
 *
 * @return
 *  - INT32_MAX -- Internal error.
 *  - Others -- NR tracking area code.
 */
//--------------------------------------------------------------------------------------------------
int32_t taf_radio_GetServingCellNrTracAreaCode
(
    uint8_t phoneId
        ///< [IN] Phone ID.
)
{
    return 0;
}
//--------------------------------------------------------------------------------------------------
/**
 *  Gets E-UTRA absolute radio frequency channel number.
 *
 * @return
 *  - UINT16_MAX -- Internal error.
 *  - Others -- E-UTRA absolute radio frequency channel number.
 */
//--------------------------------------------------------------------------------------------------
uint32_t taf_radio_GetServingCellEarfcn
(
    uint8_t phoneId
        ///< [IN] Phone ID.
)
{
    return 0;
}
//--------------------------------------------------------------------------------------------------
/**
 *  Gets NR absolute radio frequency channel number.
 *
 * @return
 *  - INT32_MAX -- Internal error.
 *  - Others -- Absolute radio frequency channel number.
 */
//--------------------------------------------------------------------------------------------------
int32_t taf_radio_GetServingCellNrArfcn
(
    uint8_t phoneId
        ///< [IN] Phone ID.
)
{
    return 0;
}
//--------------------------------------------------------------------------------------------------
/**
 *  Gets timing advance.
 *
 * @return
 *  - UINT32_MAX -- Internal error.
 *  - Others -- Timing advance.
 */
//--------------------------------------------------------------------------------------------------
uint32_t taf_radio_GetServingCellTimingAdvance
(
    uint8_t phoneId
        ///< [IN] Phone ID.
)
{
    return 0;
}
//--------------------------------------------------------------------------------------------------
/**
 *  Gets physical LTE cell ID.
 *
 * @return
 *  - UINT16_MAX -- Internal error.
 *  - Others -- Physical LTE cell ID.
 */
//--------------------------------------------------------------------------------------------------
uint16_t taf_radio_GetPhysicalServingLteCellId
(
    uint8_t phoneId
        ///< [IN] Phone ID.
)
{
    return 0;
}
//--------------------------------------------------------------------------------------------------
/**
 *  Gets physical NR5G cell ID.
 *
 * @return
 *  - UINT32_MAX -- Internal error.
 *  - Others -- Physical NR5G cell ID.
 */
//--------------------------------------------------------------------------------------------------
uint32_t taf_radio_GetPhysicalServingNrCellId
(
    uint8_t phoneId
        ///< [IN] Phone ID.
)
{
    return 0;
}
//--------------------------------------------------------------------------------------------------
/**
 *  Gets the GSM base station identity code.
 *
 * @return
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_TIMEOUT -- Response time out.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_GetServingCellGsmBsic
(
    uint8_t* bsicPtr,
        ///< [OUT] GSM base station identity code.
    uint8_t phoneId
        ///< [IN] Phone ID.
)
{
    return LE_NOT_IMPLEMENTED;
}
//--------------------------------------------------------------------------------------------------
/**
 *  Gets the primary scrambling code.
 *
 * @return
 *  - UINT16_MAX -- Internal error.
 *  - Others -- Primary scrambling code.
 *
 * @b NOTE: Only applicable for WCDMA.
 */
//--------------------------------------------------------------------------------------------------
uint16_t taf_radio_GetServingCellScramblingCode
(
    uint8_t phoneId
        ///< [IN] Phone ID.
)
{
    return 0;
}
//--------------------------------------------------------------------------------------------------
/**
 *  Gets the current network's short name.
 *
 * @return
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_FAULT -- Failed.
 *  - LE_TIMEOUT -- Time out.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_GetCurrentNetworkName
(
    char* nameStr,
        ///< [OUT] Current network's short name.
    size_t nameStrSize,
        ///< [IN]
    uint8_t phoneId
        ///< [IN] Phone ID.
)
{
    return LE_NOT_IMPLEMENTED;
}
//--------------------------------------------------------------------------------------------------
/**
 *  Gets the current MCC and MNC.
 *
 * @return
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_GetCurrentNetworkMccMnc
(
    char* mccStr,
        ///< [OUT] MCC.
    size_t mccStrSize,
        ///< [IN]
    char* mncStr,
        ///< [OUT] MNC.
    size_t mncStrSize,
        ///< [IN]
    uint8_t phoneId
        ///< [IN] Phone ID.
)
{
    return LE_NOT_IMPLEMENTED;
}
//--------------------------------------------------------------------------------------------------
/**
 *  Performs celluar network scan and gets the network scan information list reference.
 *
 * @return
 *  - Non-null pointer -- The network scan information list reference.
 *  - Null pointer -- Internal error.
 */
//--------------------------------------------------------------------------------------------------
taf_radio_ScanInformationListRef_t taf_radio_PerformCellularNetworkScan
(
    uint8_t phoneId
        ///< [IN] Phone ID.
)
{
    return NULL;
}
//--------------------------------------------------------------------------------------------------
/**
 *  Asynchronously performs a celluar network scan.
 */
//--------------------------------------------------------------------------------------------------
void taf_radio_PerformCellularNetworkScanAsync
(
    taf_radio_CellularNetworkScanHandlerFunc_t handlerPtr,
        ///< [IN] Handler for network scan.
    void* contextPtr,
        ///< [IN]
    uint8_t phoneId
        ///< [IN] Phone ID.
)
{
}
//--------------------------------------------------------------------------------------------------
/**
 *  Gets the reference of the first operator in the network scan information list.
 *
 * @return
 *  - Non-null pointer -- The reference of the first operator in the network scan list.
 *  - Null pointer -- Internal error.
 */
//--------------------------------------------------------------------------------------------------
taf_radio_ScanInformationRef_t taf_radio_GetFirstCellularNetworkScan
(
    taf_radio_ScanInformationListRef_t scanInformationListRef
        ///< [IN] The network scan information list reference.
)
{
    return NULL;
}
//--------------------------------------------------------------------------------------------------
/**
 * Gets the reference of the next operator based on the current position in the network scan
 * information list.
 *
 * @return
 *  - Non-null pointer -- The reference to the next operator in the network scan list.
 *  - Null pointer -- Internal error or the end of the list.
 */
//--------------------------------------------------------------------------------------------------
taf_radio_ScanInformationRef_t taf_radio_GetNextCellularNetworkScan
(
    taf_radio_ScanInformationListRef_t scanInformationListRef
        ///< [IN] The network scan information list reference.
)
{
    return NULL;
}
//--------------------------------------------------------------------------------------------------
/**
 *  Deletes the network scan information list reference.
 *
 * @return
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_NOT_FOUND -- Reference not found.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_DeleteCellularNetworkScan
(
    taf_radio_ScanInformationListRef_t scanInformationListRef
        ///< [IN] The network scan information list reference.
)
{
    return LE_NOT_IMPLEMENTED;
}
//--------------------------------------------------------------------------------------------------
/**
 *  Gets MCC and MNC of a network scan operator.
 *
 * @return
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_NOT_FOUND -- Reference not found.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_GetCellularNetworkMccMnc
(
    taf_radio_ScanInformationRef_t scanInformationRef,
        ///< [IN] The scan information list reference.
    char* mccPtr,
        ///< [OUT] MCC.
    size_t mccPtrSize,
        ///< [IN]
    char* mncPtr,
        ///< [OUT] MNC.
    size_t mncPtrSize
        ///< [IN]
)
{
    return LE_NOT_IMPLEMENTED;
}
//--------------------------------------------------------------------------------------------------
/**
 *  Gets the network name of a network scan operator.
 *
 * @return
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_NOT_FOUND -- Reference not found.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_GetCellularNetworkName
(
    taf_radio_ScanInformationRef_t scanInformationRef,
        ///< [IN] The scan information list reference.
    char* namePtr,
        ///< [OUT] Network name.
    size_t namePtrSize
        ///< [IN]
)
{
    return LE_NOT_IMPLEMENTED;
}
//--------------------------------------------------------------------------------------------------
/**
 *  Gets Radio Access Technology of a network scan operator.
 *
 * @return
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_NOT_FOUND -- Reference not found.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
taf_radio_Rat_t taf_radio_GetCellularNetworkRat
(
    taf_radio_ScanInformationRef_t scanInformationRef
        ///< [IN] The scan information list reference.
)
{
    return (taf_radio_Rat_t) 0;
}
//--------------------------------------------------------------------------------------------------
/**
 *  Checks if a scan operator's network is in use.
 *
 * @return
 *  - True -- Network is in use.
 *  - False -- Invalid reference or network is not in use.
 */
//--------------------------------------------------------------------------------------------------
bool taf_radio_IsCellularNetworkInUse
(
    taf_radio_ScanInformationRef_t scanInformationRef
        ///< [IN] The network scan operator reference.
)
{
    return false;
}
//--------------------------------------------------------------------------------------------------
/**
 *  Checks if the scan operator's network is available.
 *
 * @return
 *  - True -- Network is available.
 *  - False -- Invalid reference or network is available.
 */
//--------------------------------------------------------------------------------------------------
bool taf_radio_IsCellularNetworkAvailable
(
    taf_radio_ScanInformationRef_t scanInformationRef
        ///< [IN] The network scan operator reference.
)
{
    return false;
}
//--------------------------------------------------------------------------------------------------
/**
 *  Checks if scan operator is home network.
 *
 * @return
 *  - True -- It is home network.
 *  - False -- Invalid reference or it is not home network.
 */
//--------------------------------------------------------------------------------------------------
bool taf_radio_IsCellularNetworkHome
(
    taf_radio_ScanInformationRef_t scanInformationRef
        ///< [IN] The network scan operator reference.
)
{
    return false;
}
//--------------------------------------------------------------------------------------------------
/**
 * Checks if scan operator's network is forbidden.
 *
 * @return
 *  - True -- Network is forbidden.
 *  - False -- Invalid reference or network is forbidden.
 */
//--------------------------------------------------------------------------------------------------
bool taf_radio_IsCellularNetworkForbidden
(
    taf_radio_ScanInformationRef_t scanInformationRef
        ///< [IN] The network scan operator reference.
)
{
    return false;
}
//--------------------------------------------------------------------------------------------------
/**
 *  Performs celluar network scan with Physical Cell ID.
 *
 * @return
 *  - Non-null pointer -- The PCI network scan information list reference.
 *  - Null pointer -- Internal error.
 */
//--------------------------------------------------------------------------------------------------
taf_radio_PciScanInformationListRef_t taf_radio_PerformPciNetworkScan
(
    taf_radio_RatBitMask_t ratMask,
        ///< [IN] Radio Access Technology bitmask.
    uint8_t phoneId
        ///< [IN] Phone ID.
)
{
    return NULL;
}
//--------------------------------------------------------------------------------------------------
/**
 *  Asynchronously performs celluar network scan with physical cell ID.
 */
//--------------------------------------------------------------------------------------------------
void taf_radio_PerformPciNetworkScanAsync
(
    taf_radio_RatBitMask_t ratMask,
        ///< [IN] Radio Access Technology bitmask.
    taf_radio_PciNetworkScanHandlerFunc_t handlerPtr,
        ///< [IN] Handler for PCI network scan.
    void* contextPtr,
        ///< [IN]
    uint8_t phoneId
        ///< [IN] Phone ID.
)
{
}
//--------------------------------------------------------------------------------------------------
/**
 *  Gets the first reference in the PCI network scan information list.
 *
 * @return
 *  - Non-null pointer -- The reference of the first PCI network scan information in the list.
 *  - Null pointer -- Internal error.
 */
//--------------------------------------------------------------------------------------------------
taf_radio_PciScanInformationRef_t taf_radio_GetFirstPciScanInfo
(
    taf_radio_PciScanInformationListRef_t scanInformationListRef
        ///< [IN] PCI network scan information list
        ///< reference.
)
{
    return NULL;
}
//--------------------------------------------------------------------------------------------------
/**
 *  Gets the next reference in the PCI network scan information list.
 *
 * @return
 *  - Non-null pointer -- The reference of the next PCI network scan information in the list.
 *  - Null pointer -- Internal error.
 */
//--------------------------------------------------------------------------------------------------
taf_radio_PciScanInformationRef_t taf_radio_GetNextPciScanInfo
(
    taf_radio_PciScanInformationListRef_t scanInformationListRef
        ///< [IN] PCI network scan information list
        ///< reference.
)
{
    return NULL;
}
//--------------------------------------------------------------------------------------------------
/**
 *  Deletes the PCI network scan information list reference.
 *
 * @return
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_NOT_FOUND -- Reference not found.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_DeletePciNetworkScan
(
    taf_radio_PciScanInformationListRef_t scanInformationListRef
        ///< [IN] PCI network scan information list
        ///< reference.
)
{
    return LE_NOT_IMPLEMENTED;
}
//--------------------------------------------------------------------------------------------------
/**
 *  Gets the first reference of PLMN info of PCI network scan information.
 *
 * @return
 *  - Non-null pointer -- The reference of the first PLMN information.
 *  - Null pointer -- Internal error.
 */
//--------------------------------------------------------------------------------------------------
taf_radio_PlmnInformationRef_t taf_radio_GetFirstPlmnInfo
(
    taf_radio_PciScanInformationRef_t pciScanInformationRef
        ///< [IN] PCI network scan information reference.
)
{
    return NULL;
}
//--------------------------------------------------------------------------------------------------
/**
 *  Gets the next reference of PLMN info of PCI network scan information.
 *
 * @return
 *  - Non-null pointer -- The reference of the next PLMN information.
 *  - Null pointer -- Internal error.
 */
//--------------------------------------------------------------------------------------------------
taf_radio_PlmnInformationRef_t taf_radio_GetNextPlmnInfo
(
    taf_radio_PciScanInformationRef_t pciScanInformationRef
        ///< [IN] PCI network scan information reference.
)
{
    return NULL;
}
//--------------------------------------------------------------------------------------------------
/**
 *  Gets the cell ID of PCI network scan information.
 *
 * @return Cell ID.
 */
//--------------------------------------------------------------------------------------------------
uint16_t taf_radio_GetPciScanCellId
(
    taf_radio_PciScanInformationRef_t pciScanInformationRef
        ///< [IN] PCI network scan information reference.
)
{
    return 0;
}
//--------------------------------------------------------------------------------------------------
/**
 *  Gets the global cell ID of PCI network scan information.
 *
 * @return Global Cell ID.
 */
//--------------------------------------------------------------------------------------------------
uint32_t taf_radio_GetPciScanGlobalCellId
(
    taf_radio_PciScanInformationRef_t pciScanInformationRef
        ///< [IN] PCI network scan information reference.
)
{
    return 0;
}
//--------------------------------------------------------------------------------------------------
/**
 *  Gets PLMN network MCC and MNC of PCI network scan information.
 *
 * @return
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_GetPciScanMccMnc
(
    taf_radio_PlmnInformationRef_t plmnRef,
        ///< [IN] PLMN information reference.
    char* mccPtr,
        ///< [OUT] MCC.
    size_t mccPtrSize,
        ///< [IN]
    char* mncPtr,
        ///< [OUT] MNC.
    size_t mncPtrSize
        ///< [IN]
)
{
    return LE_NOT_IMPLEMENTED;
}
//--------------------------------------------------------------------------------------------------
/**
 *  Gets neighbor cells information.
 *
 * @return
 *  - Non-null pointer -- The signal metrics reference.
 *  - Null pointer -- Internal error.
 */
//--------------------------------------------------------------------------------------------------
taf_radio_NeighborCellsRef_t taf_radio_GetNeighborCellsInfo
(
    uint8_t phoneId
        ///< [IN] Phone ID.
)
{
    return NULL;
}
//--------------------------------------------------------------------------------------------------
/**
 * Deletes neighbor cells information.
 *
 * @return
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_NOT_FOUND -- Neighbor cells reference not found.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_DeleteNeighborCellsInfo
(
    taf_radio_NeighborCellsRef_t ngbrCellsRef
        ///< [IN] Neighbor cells reference.
)
{
    return LE_NOT_IMPLEMENTED;
}
//--------------------------------------------------------------------------------------------------
/**
 *  Gets the first neighbor cell information.
 *
 * @return
 *  - Non-null pointer -- The first neighbor cell information reference.
 *  - Null pointer -- Internal error.
 */
//--------------------------------------------------------------------------------------------------
taf_radio_CellInfoRef_t taf_radio_GetFirstNeighborCellInfo
(
    taf_radio_NeighborCellsRef_t ngbrCellsRef
        ///< [IN] Neighbor cells reference.
)
{
    return NULL;
}
//--------------------------------------------------------------------------------------------------
/**
 *  Gets the next neighbor cell information.
 *
 * @return
 *  - Non-null pointer -- The next neighbor cell information reference.
 *  - Null pointer -- Internal error.
 */
//--------------------------------------------------------------------------------------------------
taf_radio_CellInfoRef_t taf_radio_GetNextNeighborCellInfo
(
    taf_radio_NeighborCellsRef_t ngbrCellsRef
        ///< [IN] Neighbor cells reference.
)
{
    return NULL;
}
//--------------------------------------------------------------------------------------------------
/**
 * Gets neighbor Cell ID.
 *
 * @return Cell ID
 */
//--------------------------------------------------------------------------------------------------
uint64_t taf_radio_GetNeighborCellId
(
    taf_radio_CellInfoRef_t ngbrCellInfoRef
        ///< [IN] Cell information reference.
)
{
    return 0;
}
//--------------------------------------------------------------------------------------------------
/**
 * Gets Location Area Code of neighbor cell.
 *
 * @return Location Area Code.
 */
//--------------------------------------------------------------------------------------------------
uint32_t taf_radio_GetNeighborCellLocAreaCode
(
    taf_radio_CellInfoRef_t ngbrCellInfoRef
        ///< [IN] Neighboring cell information reference.
)
{
    return 0;
}
//--------------------------------------------------------------------------------------------------
/**
 * Gets signal strength of neighbor cell.
 *
 * @return signal strength in dBm.
 */
//--------------------------------------------------------------------------------------------------
int32_t taf_radio_GetNeighborCellRxLevel
(
    taf_radio_CellInfoRef_t ngbrCellInfoRef
        ///< [IN] Neighbor cell information reference.
)
{
    return 0;
}
//--------------------------------------------------------------------------------------------------
/**
 * Gets Radio Access Technology of neighbor cell.
 *
 * @return Radio Access Technology.
 */
//--------------------------------------------------------------------------------------------------
taf_radio_Rat_t taf_radio_GetNeighborCellRat
(
    taf_radio_CellInfoRef_t ngbrCellInfoRef
        ///< [IN] Neighbor cell information reference.
)
{
    return (taf_radio_Rat_t) 0;
}
//--------------------------------------------------------------------------------------------------
/**
 * Gets Physical Cell ID of neighbor LTE cell.
 *
 * @return Physical Cell ID, valid from 0 to 503.
 */
//--------------------------------------------------------------------------------------------------
uint16_t taf_radio_GetPhysicalNeighborLteCellId
(
    taf_radio_CellInfoRef_t ngbrCellInfoRef
        ///< [IN] Neighbor cell information reference
)
{
    return 0;
}
//--------------------------------------------------------------------------------------------------
/**
 * Gets Physical Cell ID of neighbor NR5G cell.
 *
 * @return Physical Cell ID.
 */
//--------------------------------------------------------------------------------------------------
uint32_t taf_radio_GetPhysicalNeighborNrCellId
(
    taf_radio_CellInfoRef_t ngbrCellInfoRef
        ///< [IN] Neighbor cell information reference
)
{
    return 0;
}
//--------------------------------------------------------------------------------------------------
/**
 * Gets Base Station Identity Code of neighbor GSM cell.
 *
 * @return
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_NOT_FOUND -- Neighbor cells reference not found.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_GetNeighborCellGsmBsic
(
    taf_radio_CellInfoRef_t ngbrCellInfoRef,
        ///< [IN] Neighbor cell information reference
    uint8_t* bsicPtr
        ///< [OUT] Base Station Identity Code
)
{
    return LE_NOT_IMPLEMENTED;
}
//--------------------------------------------------------------------------------------------------
/**
 * Gets the network status reference.
 *
 * @return
 *  - Non-null pointer -- Network status reference.
 *  - Null pointer -- Internal error.
 */
//--------------------------------------------------------------------------------------------------
taf_radio_NetStatusRef_t taf_radio_GetNetStatus
(
    uint8_t phoneId
        ///< [IN] Phone ID.
)
{
    return NULL;
}
//--------------------------------------------------------------------------------------------------
/**
 * Gets the RAT service status.
 *
 * @return
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 *  - LE_TIMEOUT -- Time out.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_GetRatSvcStatus
(
    taf_radio_NetStatusRef_t netRef,
        ///< [IN] Network status reference.
    taf_radio_RatSvcStatus_t* statusPtr
        ///< [OUT] RAT service status.
)
{
    return LE_NOT_IMPLEMENTED;
}
//--------------------------------------------------------------------------------------------------
/**
 * Gets the CS capabilitiy of LTE network.
 *
 * @return
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 *  - LE_TIMEOUT -- Time out.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_GetLteCsCap
(
    taf_radio_NetStatusRef_t netRef,
        ///< [IN] Network status reference.
    taf_radio_CsCap_t* capabilityPtr
        ///< [OUT] CS capabilitiy of LTE network.
)
{
    return LE_NOT_IMPLEMENTED;
}
//--------------------------------------------------------------------------------------------------
/**
 *  Gets the IMS reference.
 *
 * @return
 *  - Non-null pointer -- IMS reference.
 *  - Null pointer -- Internal error.
 */
//--------------------------------------------------------------------------------------------------
taf_radio_ImsRef_t taf_radio_GetIms
(
    uint8_t phoneId
        ///< [IN] Phone ID.
)
{
    return NULL;
}
//--------------------------------------------------------------------------------------------------
/**
 * Gets the IMS registration status.
 *
 * @return
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 *  - LE_TIMEOUT -- Time out.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_GetImsRegStatus
(
    taf_radio_ImsRegStatus_t* statusPtr,
        ///< [OUT] IMS registration status.
    uint8_t phoneId
        ///< [IN] Phone ID.
)
{
    return LE_NOT_IMPLEMENTED;
}
//--------------------------------------------------------------------------------------------------
/**
 * Gets the IMS service status.
 *
 * @return
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 *  - LE_TIMEOUT -- Time out.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_GetImsSvcStatus
(
    taf_radio_ImsRef_t imsRef,
        ///< [IN] IMS reference.
    taf_radio_ImsSvcType_t service,
        ///< [IN] IMS service type.
    taf_radio_ImsSvcStatus_t* statusPtr
        ///< [OUT] IMS service status.
)
{

    LE_ASSERT(statusPtr != NULL);

    *statusPtr = ImsStatus;

    return LE_OK;
}
//--------------------------------------------------------------------------------------------------
/**
 * Gets the PDP error code.
 *
 * @return
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 *  - LE_TIMEOUT -- Time out.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_GetImsPdpError
(
    taf_radio_ImsRef_t imsRef,
        ///< [IN] IMS reference.
    taf_radio_PdpError_t* errorPtr
        ///< [OUT] PDP error code.
)
{
    return LE_NOT_IMPLEMENTED;
}
//--------------------------------------------------------------------------------------------------
/**
 * Sets the IMS service enablement configuration parameters.
 *
 * @return
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 *  - LE_TIMEOUT -- Time out.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_SetImsSvcCfg
(
    taf_radio_ImsRef_t imsRef,
        ///< [IN] IMS reference.
    taf_radio_ImsSvcType_t service,
        ///< [IN] IMS service type.
    bool enable
        ///< [IN] True if enabling service, false if disabling service.
)
{
    return LE_NOT_IMPLEMENTED;
}
//--------------------------------------------------------------------------------------------------
/**
 * Gets the IMS service enablement configuration parameters.
 *
 * @return
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 *  - LE_TIMEOUT -- Time out.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_GetImsSvcCfg
(
    taf_radio_ImsRef_t imsRef,
        ///< [IN] IMS reference.
    taf_radio_ImsSvcType_t service,
        ///< [IN] IMS service type.
    bool* enablePtr
        ///< [OUT] True if service is enabled, false if service is disabled.
)
{
    return LE_NOT_IMPLEMENTED;
}
//--------------------------------------------------------------------------------------------------
/**
 * Sets the user agent.
 *
 * @return
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 *  - LE_TIMEOUT -- Time out.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_SetImsUserAgent
(
    taf_radio_ImsRef_t imsRef,
        ///< [IN] IMS reference.
    const char* LE_NONNULL userAgent
        ///< [IN] User agent string to be sent with SIP message.
)
{
    return LE_NOT_IMPLEMENTED;
}
//--------------------------------------------------------------------------------------------------
/**
 * Gets the user agent.
 *
 * @return
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 *  - LE_TIMEOUT -- Time out.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_GetImsUserAgent
(
    taf_radio_ImsRef_t imsRef,
        ///< [IN] IMS reference.
    char* userAgent,
        ///< [OUT] User agent string to be sent with SIP message.
    size_t userAgentSize
        ///< [IN]
)
{
    return LE_NOT_IMPLEMENTED;
}
//--------------------------------------------------------------------------------------------------
/**
 *  Gets the DCNR and ENDC mode status.
 *
 * @return
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_FAULT -- Failed
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_GetNrDualConnectivityStatus
(
    taf_radio_NREndcAvailability_t* statusEndcPtr,
        ///< [OUT] ENDC availability status.
    taf_radio_NRDcnrRestriction_t* statusDcnrPtr,
        ///< [OUT] DCNR restriction status.
    uint8_t phoneId
        ///< [IN] Phone ID.
)
{
    return LE_NOT_IMPLEMENTED;
}
//--------------------------------------------------------------------------------------------------
/**
 *  Gets the long name of the network.
 *
 * @return
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_FAULT -- Failed.
 *  - LE_TIMEOUT -- Time out.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_GetCurrentNetworkLongName
(
    char* longNameStr,
        ///< [OUT] Long network name.
    size_t longNameStrSize,
        ///< [IN]
    uint8_t phoneId
        ///< [IN] Phone ID.
)
{
    return LE_NOT_IMPLEMENTED;
}
//--------------------------------------------------------------------------------------------------
/**
 *  Gets the details of the total SIM count and maximum active SIM count.
 *
 * @return
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 *  - LE_TIMEOUT -- Time out.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_GetHardwareSimConfig
(
    uint8_t* totalSimCountPtr,
        ///< [OUT] The maximum number of SIMs supported simultaneously.
    uint8_t* maxActiveSimsPtr
        ///< [OUT] The maximum number of SIMs that can be active simultaneously.
)
{
    return LE_NOT_IMPLEMENTED;
}
//--------------------------------------------------------------------------------------------------
/**
 *  Gets the RAT capabilities supported by hardware and SIM based on a given phone ID.
 *
 * @return
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 *  - LE_TIMEOUT -- Time out.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_GetHardwareSimRatCapabilities
(
    taf_radio_RatBitMask_t* deviceRatCapMaskPtr,
        ///< [OUT] Device RAT capability bitmask.
    taf_radio_RatBitMask_t* simRatCapMaskPtr,
        ///< [OUT] SIM RAT capability bitmask.
    uint8_t phoneId
        ///< [IN] Phone ID.
)
{
    return LE_NOT_IMPLEMENTED;
}
//--------------------------------------------------------------------------------------------------
/**
 * Sets the hysteresis in units of 0.1 dBm. which is an optional parameter for signal strength
 * indication.
 *
 * @return
 *  - LE_OK -- Succeeded.
 *  - LE_BAD_PARAMETER -- Bad parameters
 *  - LE_FAULT -- Failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_SetSignalStrengthIndHysteresis
(
    taf_radio_SigType_t sigType,
        ///< [IN] Signal type.
    uint16_t hysteresis,
        ///< [IN] Hysteresis dBm in units of 0.1 dBm.
    uint8_t phoneId
        ///< [IN] Phone ID.
)
{
    return LE_NOT_IMPLEMENTED;
}
//--------------------------------------------------------------------------------------------------
/**
 * Sets the hysteresis time in milliseconds which is an optional parameter for signal strength
 * indication.
 *
 * @return
 *  - LE_OK -- Succeeded.
 *  - LE_BAD_PARAMETER -- Bad parameters
 *  - LE_FAULT -- Failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_SetSignalStrengthIndHysteresisTimer
(
    uint16_t hysteresisTimer,
        ///< [IN] Hysteresis time in milliseconds.
    uint8_t phoneId
        ///< [IN] Phone ID.
)
{
    return LE_NOT_IMPLEMENTED;
}
//--------------------------------------------------------------------------------------------------
/**
 *  Gets the serving cell absolute radio frequency channel number.
 *
 * @return
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 *
 * @b NOTE: Only applicable for GSM.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_GetServingCellArfcn
(
    int32_t* arfcnPtr,
        ///< [OUT] Absolute radio frequency channel number.
    uint8_t phoneId
        ///< [IN] Phone ID.
)
{
    return LE_NOT_IMPLEMENTED;
}
//--------------------------------------------------------------------------------------------------
/**
 *  Gets the serving cell UTRA absolute radio frequency channel number.
 *
 * @return
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 *
 * @b NOTE: Only applicable for UMTS.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_GetServingCellUarfcn
(
    int32_t* uarfcnPtr,
        ///< [OUT] UTRA absolute radio frequency channel number.
    uint8_t phoneId
        ///< [IN] Phone ID.
)
{
    return LE_NOT_IMPLEMENTED;
}
//--------------------------------------------------------------------------------------------------
/**
 * Sets the operating mode.
 *
 * @return
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_SetOperatingMode
(
    taf_radio_OpMode_t mode,
        ///< [IN] Operating mode.
    uint8_t phoneId
        ///< [IN] Phone ID.
)
{
    return LE_NOT_IMPLEMENTED;
}
//--------------------------------------------------------------------------------------------------
/**
 *  Gets the operating mode.
 *
 * @return
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_GetOperatingMode
(
    taf_radio_OpMode_t* modePtr,
        ///< [OUT] Operating mode.
    uint8_t phoneId
        ///< [IN] Phone ID.
)
{
    return LE_NOT_IMPLEMENTED;
}
//--------------------------------------------------------------------------------------------------
/**
 *  Gets routing area code.
 *
 * @return
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_TIMEOUT -- Response time out.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 *
 * @note Only applicable for GSM/WCDMA/TDSCDMA.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_GetServingCellRoutingAreaCode
(
    uint8_t* racPtr,
        ///< [OUT] Routing area code.
    uint8_t phoneId
        ///< [IN] Phone ID.
)
{
    return LE_NOT_IMPLEMENTED;
}
//--------------------------------------------------------------------------------------------------
/**
 *  Gets 2G/3G band information.
 *
 * @return
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_TIMEOUT -- Response time out.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 *
 * @b NOTE: Only applicable for GSM/WCDMA/TDSCDMA.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_GetServingCellBandInfo
(
    taf_radio_BandBitMask_t* bandPtrPtr,
        ///< [OUT] 2G/3G active band.
    taf_radio_RFBandWidth_t* bandWidthPtrPtr,
        ///< [OUT] RF bandwidth information.
    uint8_t phoneId
        ///< [IN] Phone ID.
)
{
    return LE_NOT_IMPLEMENTED;
}
//--------------------------------------------------------------------------------------------------
/**
 *  Gets LTE band information.
 *
 * @return
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_TIMEOUT -- Response time out.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_GetServingCellLteBandInfo
(
    uint32_t* bandPtrPtr,
        ///< [OUT] LTE active band.
    taf_radio_RFBandWidth_t* bandWidthPtrPtr,
        ///< [OUT] RF bandwidth information.
    uint8_t phoneId
        ///< [IN] Phone ID.
)
{
    return LE_NOT_IMPLEMENTED;
}
//--------------------------------------------------------------------------------------------------
/**
 *  Gets NR band information.
 *
 * @return
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_TIMEOUT -- Response time out.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_GetServingCellNrBandInfo
(
    uint32_t* bandPtrPtr,
        ///< [OUT] NR active band.
    taf_radio_RFBandWidth_t* bandWidthPtrPtr,
        ///< [OUT] RF bandwidth information.
    uint8_t phoneId
        ///< [IN] Phone ID.
)
{
    return LE_NOT_IMPLEMENTED;
}
//--------------------------------------------------------------------------------------------------
/**
 *  Gets NR icon type.
 *
 * @return
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_TIMEOUT -- Response time out.
 *  - LE_FAULT -- Failed.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_GetNrIconType
(
    taf_radio_NrIconType_t* typePtr,
        ///< [OUT] NR icon type.
    uint8_t phoneId
        ///< [IN] Phone ID.
)
{
    return LE_NOT_IMPLEMENTED;
}


static void DropAllRest(int fd)
{
    char buffer[128];
    int nRead = -1;
    while(1)
    {
        nRead = le_fd_Read(fd, buffer, sizeof(buffer));
        if (nRead <= 0)
        {
            break;
        }
    }
}

static void FifoEventHandler
(
    int     fd,
    short   events
)
{
    char    buffer[PIPE_BUF];
    ssize_t readSize;

    LE_ASSERT(fd == RadioEventNodeFd);

    if (events & POLLIN)
    {

        Header_t * header = (Header_t *)buffer;

        readSize = le_fd_Read(RadioEventNodeFd, header, sizeof(*header));

        if(-1 == readSize)
        {
            LE_INFO("Can't read fifo %s: %m", RADIO_EVENT_NODE);
            return;
        }

        if (sizeof(Header_t) != readSize) // less than Header size
        {
            LE_INFO("Bad header msg to be parsed. (size: %d)", (int) readSize);
            return;
        }

        header->length = ntohs(header->length);
        header->type = ntohs(header->type);

        if (header->length > sizeof(buffer))
        {
            LE_INFO("Msg length more than buffer size: %d", (int)PIPE_BUF);
            DropAllRest(RadioEventNodeFd);
            return;
        }

        LE_INFO("len: %d, type: %d", header->length, header->type);

        uint8_t * payload = (uint8_t *) buffer + sizeof(*header);

        size_t payloadTotal = 0;
        while(payloadTotal < header->length)
        {
            ssize_t nRead = le_fd_Read(RadioEventNodeFd,
                                    payload + payloadTotal,
                                    header->length - payloadTotal);
            if (nRead == -1)
            {
                if (errno == EINTR)
                {
                    continue;
                }
                else if (errno == EAGAIN)
                {
                    LE_INFO("No more ...");
                    break;
                }
                else {
                    LE_INFO("read error: %m");
                    return;
                }
            }
            else if (nRead == 0)
            {
                LE_INFO("read EOF: %m");
                break;
            }

            payloadTotal += nRead;
        }

        LE_INFO("payloadTotal = %d", (int)payloadTotal);

        switch (header->type)
        {
            case 0x0001: // Json format
            {
                json_error_t error;
                json_t * root = json_loadb((const char *)payload, (header->length - 2), 0, &error);
                if (root == NULL)
                {
                    LE_INFO("JSON error: %s", error.text);
                    return;
                }

                json_t * node = json_object_get(root, "ims_status");
                if (! node)
                {
                    LE_INFO("Not found 'ims_status' item.");
                    json_decref(root);
                    return;
                }

                if (!json_is_integer(node))
                {
                    LE_INFO("Item 'ims_status' is not <INT>");
                    json_decref(root);
                    return;
                }

                int status = json_integer_value(node);

                json_decref(root);

                if (status < TAF_RADIO_IMS_SVC_STATUS_UNAVAILABLE
                ||  status > TAF_RADIO_IMS_SVC_STATUS_FULL_SERVICE)
                {
                    LE_INFO("Bad status picked from json file.");
                    return;
                }

                // To be gotten by another API: taf_radio_GetImsSvcStatus
                // ENUM ImsSvcStatus
                // {
                //     IMS_SVC_STATUS_UNKNOWN = -1, ///< Unknown status.
                //     IMS_SVC_STATUS_UNAVAILABLE,  ///< Unavailable service status.
                //     IMS_SVC_STATUS_LIMITED,      ///< Limited service status.
                //     IMS_SVC_STATUS_FULL_SERVICE  ///< Full service status.
                // };
                ImsStatus = status;

                taf_RadioImsStatus_t* statusPtr =
                        (taf_RadioImsStatus_t*)le_mem_ForceAlloc(ImsEvtPool);
                statusPtr->bitmask = TAF_RADIO_IMS_IND_BIT_MASK_SVC_INFO;
                statusPtr->phoneId = 0; // unused
                statusPtr->imsRef = NULL; // unused
                le_event_Report(ImsEventId, (void*)statusPtr, sizeof(*statusPtr));
                LE_INFO("report -> ims event");
            }
            break;

            case 0x0002: // String format
            {
                LE_INFO("TBD -- String format");
            }
            break;

            case 0x0003: // C struct format
            {
                LE_INFO("TBD -- C struct format");
            }
            break;

            default:
            {
                LE_INFO("Unknown format to be parsed.");
                return;
            }
        }
    }
    else
    {
        LE_INFO("<Unknown Event>");
    }
}

static void TermSigHandler(int sigNum)
{
    if (access(RADIO_EVENT_NODE, F_OK) == 0)
    {
        unlink(RADIO_EVENT_NODE);
    }

    if (RadioEventMonitor)
    {
        le_fdMonitor_Delete(RadioEventMonitor);
    }

    if (RadioEventNodeFd != -1)
    {
        le_fd_Close(RadioEventNodeFd);
    }

    if (DummyFd != -1)
    {
        le_fd_Close(DummyFd);
    }

    _exit(0);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the reference of Carrier Aggregation(CA) information.
 *
 * @return
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_NOT_FOUND -- Reference not found.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_GetCAInformation
(
    uint8_t phoneId,
        ///< [IN] Phone ID.
    taf_radio_Rat_t rat,
        ///< [IN] RAT.
    taf_radio_CAInfoRef_t* infoRefPtr
        ///< [OUT] CA information reference.
)
{
    return LE_NOT_IMPLEMENTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Deletes the reference of Carrier Aggregation(CA) information.
 *
 * @return
 *  - LE_NOT_FOUND -- Reference not found.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_DeleteCAInformation
(
    taf_radio_CAInfoRef_t infoRef
        ///< [IN] CA information reference.
)
{
    return LE_NOT_IMPLEMENTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the Carrier Aggregation(CA) status and active CC count of LTE.
 *
 * @return
 *  - LE_NOT_FOUND -- Reference not found.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_GetLteCAStatus
(
    taf_radio_CAInfoRef_t infoRef,
        ///< [IN] CA information reference.
    taf_radio_CAStatus_t* statusPtr,
        ///< [OUT] CA status
    uint32_t* activeCCNumPtr
        ///< [OUT] Numbers of active component carriers
)
{
    return LE_NOT_IMPLEMENTED;
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets the reference of connection status.
 *
 * @return
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_NOT_FOUND -- Reference not found.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_GetConnStatus
(
    uint8_t phoneId,
        ///< [IN] Phone ID.
    taf_radio_ConnStatusRef_t* statusRefPtr
        ///< [OUT] Connection status reference.
)
{
    return LE_NOT_IMPLEMENTED;
}


//--------------------------------------------------------------------------------------------------
/**
 * Deletes the reference of connection status.
 *
 * @return
 *  - LE_NOT_FOUND -- Reference not found.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_DeleteConnStatus
(
    taf_radio_ConnStatusRef_t statusRef
        ///< [IN] Connection status reference.
)
{
    return LE_NOT_IMPLEMENTED;
}


//--------------------------------------------------------------------------------------------------
/**
 *  Gets the E-UTRA New Radio Dual Connectivity(ENDC) connection status.
 *
 * @return
 *  - LE_NOT_FOUND -- Reference not found.
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_radio_GetEndcConnectionStatus
(
    taf_radio_ConnStatusRef_t statusRef,
        ///< [IN] Reference
    taf_radio_NREndcAvailability_t* statusPtr
        ///< [OUT] ENDC connection status
)
{
    return LE_NOT_IMPLEMENTED;
}


COMPONENT_INIT
{
    mode_t oldMask = umask(0);

    int result = le_fd_MkFifo(RADIO_EVENT_NODE,
                    S_IRUSR | S_IWUSR |
                    S_IRGRP | S_IWGRP |
                    S_IROTH | S_IWOTH);

    if (-1 == result)
    {
        if (errno == EEXIST)
        {
            LE_INFO("%s was already there", RADIO_EVENT_NODE);
        }
        else
        {
            LE_FATAL("Can't create radio event fifo node: %m");
        }
    }

    umask(oldMask);

    LE_ASSERT(signal(SIGTERM, TermSigHandler) != SIG_ERR);

    RadioEventNodeFd = le_fd_Open(RADIO_EVENT_NODE, O_RDONLY | O_NONBLOCK);
    if (RadioEventNodeFd == -1)
    {
        LE_FATAL("Can't open %s: %m", RADIO_EVENT_NODE);
    }

    RadioEventMonitor = le_fdMonitor_Create("FIFO", RadioEventNodeFd, &FifoEventHandler, POLLIN);

    DummyFd = le_fd_Open(RADIO_EVENT_NODE, O_WRONLY | O_NONBLOCK);
    LE_ASSERT(DummyFd != -1);

    LE_FATAL_IF(
        RadioEventMonitor == NULL,
        "Create monitor failed."
    );

    ImsEventId = le_event_CreateId("IMS-Evt", sizeof(taf_RadioImsStatus_t));
    ImsEvtPool = le_mem_CreatePool("IMS-Evt", sizeof(taf_RadioImsStatus_t));

    LE_INFO("[simulation]: radio init done.");
}
