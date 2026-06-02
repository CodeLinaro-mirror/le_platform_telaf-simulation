/*
 *  Copyright (c) 2017-2018, 2020-2021 The Linux Foundation. All rights reserved.
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
 * @file      PhoneDefines.hpp
 * @brief     PhoneDefines contains enumerations and variables used for
 *            telephony subsystems.
 */
#ifndef TELUX_TEL_PHONEDEFINES_HPP
#define TELUX_TEL_PHONEDEFINES_HPP

#include <array>
#include <memory>
#include <string>
#include <bitset>
#include <vector>

#include <telux/common/CommonDefines.hpp>

#define DEFAULT_PHONE_ID 1
#define INVALID_PHONE_ID -1
#define THRESHOLD_LIST_MAX 10

namespace telux {

namespace tel {

/** @addtogroup telematics_call
 * @{ */

/**
 * Defines type of call like incoming, outgoing and none.
 */
enum class CallDirection {
   INCOMING,
   OUTGOING,
   NONE,
};

/**
 * Defines the states a call can be in
 */
enum class CallState {
   CALL_IDLE = -1, /**< idle call, default state of a newly created call object */
   CALL_ACTIVE,    /**< active call*/
   CALL_ON_HOLD,   /**< on hold call */
   CALL_DIALING,   /**< out going call, in dialing state and not yet connected,
                        MO Call only */
   CALL_INCOMING,  /**< incoming call, not yet answered  */
   CALL_WAITING,   /**< waiting call*/
   CALL_ALERTING,  /**< alerting call, MO Call only */
   CALL_ENDED,     /**<  call ended / disconnected */
};

/**
 * Defines call type
 */
enum class CallType {
   UNKNOWN = -1,       /**< Unknown; information is not available */
   VOICE_CALL,         /**< Normal voice call or TPS eCall */
   VOICE_IP_CALL,      /**< Normal Voice over IP (VoIP) call or TPS eCall over IP */
   EMERGENCY_CALL,     /**< Non-automotive emergency call, automotive eCall or NGeCall */
   EMERGENCY_IP_CALL,  /**< Non-automotive emergency Voice over IP (VoIP) call */
};

/**
 * Reason for the recently terminated call (either normally ended or failed)
 */
enum class CallEndCause {
   UNOBTAINABLE_NUMBER = 1,              /**< Unassigned(unallocated) number */
   NO_ROUTE_TO_DESTINATION = 3,          /**< No route  to destination */
   CHANNEL_UNACCEPTABLE = 6,             /**< Channel unacceptable */
   OPERATOR_DETERMINED_BARRING = 8,      /**< Operator determined barring */
   NORMAL = 16,                          /**< Normal call barring */
   BUSY = 17,                            /**< User busy */
   NO_USER_RESPONDING = 18,              /**< No user responding */
   NO_ANSWER_FROM_USER = 19,             /**< User alerting, no answer */
   NOT_REACHABLE = 20,                   /**< Not reachable */
   CALL_REJECTED = 21,                   /**< Call rejected */
   NUMBER_CHANGED = 22,                  /**< Number changed */
   PREEMPTION = 25,                      /**< Pre-emption */
   DESTINATION_OUT_OF_ORDER = 27,        /**< Destination out of order */
   INVALID_NUMBER_FORMAT = 28,           /**< Invalid number format (incomplete number) */
   FACILITY_REJECTED = 29,               /**< Facility rejected */
   RESP_TO_STATUS_ENQUIRY = 30,          /**< Response to STATUS ENQUIRY */
   NORMAL_UNSPECIFIED = 31,              /**< Normal, unspecified */
   CONGESTION = 34,                      /**< No circuit/channel available */
   NETWORK_OUT_OF_ORDER = 38,            /**< Network out of order */
   TEMPORARY_FAILURE = 41,               /**< Temporary failure */
   SWITCHING_EQUIPMENT_CONGESTION = 42,  /**< Switching equipment congestion */
   ACCESS_INFORMATION_DISCARDED = 43,    /**< Access information discarded */
   REQUESTED_CIRCUIT_OR_CHANNEL_NOT_AVAILABLE = 44,
                                         /**< Requested circuit/channel not available */
   RESOURCES_UNAVAILABLE_OR_UNSPECIFIED = 47,/**< Resource unavailable, unspecified */
   QOS_UNAVAILABLE = 49,                 /**< Quality of service unavailable */
   REQUESTED_FACILITY_NOT_SUBSCRIBED = 50,/**< Requested facility not subscribed */
   INCOMING_CALLS_BARRED_WITHIN_CUG = 55,/**< Incoming calls barred within the CUG */
   BEARER_CAPABILITY_NOT_AUTHORIZED = 57,/**< Bearer capability not authorized */
   BEARER_CAPABILITY_UNAVAILABLE = 58,   /**< Bearer capability not presently available */
   SERVICE_OPTION_NOT_AVAILABLE = 63,    /**< Service or option not available, unspecified */
   BEARER_SERVICE_NOT_IMPLEMENTED = 65,  /**< Bearer service not implemented */
   ACM_LIMIT_EXCEEDED = 68,              /**< ACM equal to or greater than ACMmax */
   REQUESTED_FACILITY_NOT_IMPLEMENTED = 69,/**< Requested facility not implemented */
   ONLY_DIGITAL_INFORMATION_BEARER_AVAILABLE = 70,
                                         /**< Only restricted digital information
                                              bearer capability is available */
   SERVICE_OR_OPTION_NOT_IMPLEMENTED = 79,/**< Service or option not implemented, unspecified */
   INVALID_TRANSACTION_IDENTIFIER = 81,  /**< Invalid transaction identifier value */
   USER_NOT_MEMBER_OF_CUG = 87,          /**< User not member of CUG */
   INCOMPATIBLE_DESTINATION = 88,        /**< Incompatible destination */
   INVALID_TRANSIT_NW_SELECTION = 91,    /**< Invalid transit network selection */
   SEMANTICALLY_INCORRECT_MESSAGE = 95,  /**< Semantically incorrect message */
   INVALID_MANDATORY_INFORMATION = 96,   /**< Invalid mandatory information */
   MESSAGE_TYPE_NON_IMPLEMENTED = 97,    /**< Message type non-existent or not implemented */
   MESSAGE_TYPE_NOT_COMPATIBLE_WITH_PROTOCOL_STATE = 98,
                                         /**< Message type not compatible with protocol state */
   INFORMATION_ELEMENT_NON_EXISTENT = 99,/**< Information element non-existent or
                                              not implemented */
   CONDITIONAL_IE_ERROR = 100,           /**< Conditional IE error */
   MESSAGE_NOT_COMPATIBLE_WITH_PROTOCOL_STATE = 101,
                                         /**< Message not compatible with protocol state */
   RECOVERY_ON_TIMER_EXPIRED = 102,      /**< Recovery on timer expiry */
   PROTOCOL_ERROR_UNSPECIFIED = 111,     /**< Protocol error, unspecified */
   INTERWORKING_UNSPECIFIED = 127,       /**< Interworking, unspecified */
   CALL_BARRED = 240,                    /**< Call barred */
   FDN_BLOCKED = 241,                    /**< FDN blocked */
   IMSI_UNKNOWN_IN_VLR = 242,            /**< Incorrect IMSI */
   IMEI_NOT_ACCEPTED = 243,              /**< IMEI not accepted */
   DIAL_MODIFIED_TO_USSD = 244,          /**< DIAL request modified to USSD */
   DIAL_MODIFIED_TO_SS = 245,            /**< DIAL request modified to SS */
   DIAL_MODIFIED_TO_DIAL = 246,          /**< DIAL request modified to DIAL with different data */
   RADIO_OFF = 247,                      /**< Radio is OFF */
   OUT_OF_SERVICE = 248,                 /**< No cellular coverage */
   NO_VALID_SIM = 249,                   /**< No valid SIM is present */
   RADIO_INTERNAL_ERROR = 250,           /**< Internal error at Modem */
   NETWORK_RESP_TIMEOUT = 251,           /**< No response from network */
   NETWORK_REJECT = 252,                 /**< Explicit network reject */
   RADIO_ACCESS_FAILURE = 253,           /**< RRC connection failure. Eg.RACH */
   RADIO_LINK_FAILURE = 254,             /**< Radio Link Failure */
   RADIO_LINK_LOST = 255,                /**< Radio link lost due to poor coverage */
   RADIO_UPLINK_FAILURE = 256,           /**< Radio uplink failure */
   RADIO_SETUP_FAILURE = 257,            /**< RRC connection setup failure */
   RADIO_RELEASE_NORMAL = 258,           /**< RRC connection release, normal */
   RADIO_RELEASE_ABNORMAL = 259,         /**< RRC connection release, abnormal */
   ACCESS_CLASS_BLOCKED = 260,           /**< Access class barring */
   NETWORK_DETACH = 261,                 /**< Explicit network detach */
   EMERGENCY_TEMP_FAILURE = 325,         /**< Emergency redial temporary failure */
   EMERGENCY_PERM_FAILURE = 326,         /**< Emergency redial permanent failure */
   HO_NOT_FEASIBLE = 382,                /**< Hand over not feasible */
   USER_BUSY = 501,                      /**< User busy */
   USER_REJECT = 502,                    /**< User reject */
   LOW_BATTERY = 503,                    /**< Battery is low */
   BLACKLISTED_CALL_ID = 504,            /**< Blacklisted caller id */
   CS_RETRY_REQUIRED = 505,              /**< Retry CS call, VoLTE service can't be provided by
                                              the network or remote end */
   CDMA_LOCKED_UNTIL_POWER_CYCLE = 1000, /**< MS is locked until next power cycle */
   CDMA_DROP = 1001,                     /**< Drop call */
   CDMA_INTERCEPT = 1002,                /**< INTERCEPT order received, MS state idle entered */
   CDMA_REORDER = 1003,                  /**< MS has been redirected, call is cancelled */
   CDMA_SO_REJECT = 1004,                /**< Service option rejection */
   CDMA_RETRY_ORDER = 1005,              /**< Requested service is rejected, retry delay is set */
   CDMA_ACCESS_FAILURE = 1006,           /**< Unable to obtain access to the CDMA system */
   CDMA_PREEMPTED = 1007,                /**< Not a preempted call */
   CDMA_NOT_EMERGENCY = 1008,            /**< For non-emergency number dialed
                                              during emergency callback mode */
   CDMA_ACCESS_BLOCKED = 1009,           /**< CDMA network access probes blocked */
   NETWORK_UNAVAILABLE = 1010,           /**< Network unavailable */
   FEATURE_UNAVAILABLE = 1011,           /**< Feature unavailable */
   SIP_ERROR = 1012,                     /**< SIP internal error */
   MISC = 1013,                          /**< SIP miscellaneous error */
   ANSWERED_ELSEWHERE = 1014,            /**< MT call has ended due to a release from the network
                                              because the call was answered elsewhere */
   PULL_OUT_OF_SYNC = 1015,              /**< MultiEndpoint - call pull request has failed */
   CAUSE_CALL_PULLED = 1016,             /**< MultiEndpoint - call has been pulled from primary
                                              to secondary */
   SIP_REDIRECTED = 2001,                /**< Request is redirected */
   SIP_BAD_REQUEST = 2002,               /**< Bad request */
   SIP_FORBIDDEN = 2003,                 /**< Forbidden */
   SIP_NOT_FOUND = 2004,                 /**< Remote URI not found */
   SIP_NOT_SUPPORTED = 2005,             /**< Not supported */
   SIP_REQUEST_TIMEOUT = 2006,           /**< Request timed out */
   SIP_TEMPORARILY_UNAVAILABLE = 2007,   /**< Temporarily unavailable */
   SIP_BAD_ADDRESS = 2008,               /**< Address incomplete */
   SIP_BUSY = 2009,                      /**< User busy */
   SIP_REQUEST_CANCELLED = 2010,         /**< Request(call) rejected */
   SIP_NOT_ACCEPTABLE = 2011,            /**< Not acceptable */
   SIP_NOT_REACHABLE = 2012,             /**< Not reachable */
   SIP_SERVER_INTERNAL_ERROR = 2013,     /**< Server internal error */
   SIP_SERVER_NOT_IMPLEMENTED = 2014,    /**< Server not implemented */
   SIP_SERVER_BAD_GATEWAY = 2015,        /**< Server bad gateway */
   SIP_SERVICE_UNAVAILABLE = 2016,       /**< Service unavailable */
   SIP_SERVER_TIMEOUT = 2017,            /**< Server time out */
   SIP_SERVER_VERSION_UNSUPPORTED = 2018,/**< Server version not supported */
   SIP_SERVER_MESSAGE_TOOLARGE = 2019,   /**< Server message is too large */
   SIP_SERVER_PRECONDITION_FAILURE = 2020,/**< Server pre-condition failure */
   SIP_USER_REJECTED = 2021,             /**< User(call) rejected */
   SIP_GLOBAL_ERROR = 2022,              /**< Global error */
   MEDIA_INIT_FAILED = 3001,             /**< Media resource initialization failure */
   MEDIA_NO_DATA = 3002,                 /**< RTP timeout(no audio/video traffic in the session) */
   MEDIA_NOT_ACCEPTABLE = 3003,          /**< Media is not supported */
   MEDIA_UNSPECIFIED_ERROR = 3004,       /**< Media unspecified error */
   HOLD_RESUME_FAILED = 3005,            /**< Resume failed for hold call */
   HOLD_RESUME_CANCELED = 3006,          /**< Resume cancelled for hold call */
   HOLD_REINVITE_COLLISION = 3007,       /**< Reinvite collision for hold call */
   SIP_ALTERNATE_EMERGENCY_CALL = 3008,  /**< Alternate emergency call */
   NO_CSFB_IN_CS_ROAM = 3009,            /**< CS fallback in roaming not allowed */
   SRV_NOT_REGISTERED = 3010,            /**< Service not registered */
   CALL_TYPE_NOT_ALLOWED = 3011,         /**< Call type not allowed */
   EMRG_CALL_ONGOING = 3012,             /**< Emergency call is in progress */
   CALL_SETUP_ONGOING = 3013,            /**< Call setup is in progress */
   MAX_CALL_LIMIT_REACHED = 3014,        /**< Maximum call limit reached */
   UNSUPPORTED_SIP_HDRS = 3015,          /**< Unsupported sip header */
   CALL_TRANSFER_ONGOING = 3016,         /**< Call transfer is in progress */
   PRACK_TIMEOUT = 3017,                 /**< Memory allocation failure or RTP open failure */
   QOS_FAILURE = 3018,                   /**< Call failed due to lack of dedicated bearer */
   ONGOING_HANDOVER = 3019,              /**< Call rejected due to pending handover */
   VT_WITH_TTY_NOT_ALLOWED = 3020,       /**< TTY and VT are not supported together */
   CALL_UPGRADE_ONGOING = 3021,          /**< Upgrade request is in progress */
   CONFERENCE_WITH_TTY_NOT_ALLOWED = 3022,/**< Call from conference server received when TTY is
                                               ON */
   CALL_CONFERENCE_ONGOING = 3023,       /**< Conference call is ongoing */
   VT_WITH_AVPF_NOT_ALLOWED = 3024,      /**< VT call with AVPF */
   ENCRYPTION_CALL_ONGOING = 3025,       /**< Encrypted call could not coexist with other calls */
   CALL_ONGOING_CW_DISABLED = 3026,      /**< Call waiting disabled during incoming call */
   CALL_ON_OTHER_SUB = 3027,             /**< Active call on other subscription */
   ONE_X_COLLISION = 3028,               /**< CDMA collision */
   UI_NOT_READY = 3029,                  /**< UI is not ready during the incoming call */
   CS_CALL_ONGOING = 3030,               /**< CS call ongoing when incoming call is received */
   REJECTED_ELSEWHERE = 3031,            /**< One of the devices (interconnected endpoints)
                                              rejected the call */
   USER_REJECTED_SESSION_MODIFICATION = 3032,/**< Upgrade/downgrade rejected */
   USER_CANCELLED_SESSION_MODIFICATION = 3033,/**< Upgrade/downgrade cancelled */
   SESSION_MODIFICATION_FAILED = 3034,   /**< Upgrade/downgrade failed */
   SIP_UNAUTHORIZED = 3035,              /**< Unauthorized */
   SIP_PAYMENT_REQUIRED = 3036,          /**< Payment required */
   SIP_METHOD_NOT_ALLOWED = 3037,        /**< Method requested in the address line was not allowed
                                              for the address identified by the request-URI */
   SIP_PROXY_AUTHENTICATION_REQUIRED = 3038,/**< Client must first authenticate with a proxy */
   SIP_REQUEST_ENTITY_TOO_LARGE = 3039,  /**< Request entity body is larger than what the server
                                              is willing to process */
   SIP_REQUEST_URI_TOO_LARGE = 3040,     /**< Server is refusing to service because the request-URI
                                              is longer than the server willing to interpret */
   SIP_EXTENSION_REQUIRED = 3041,        /**< Extension to process a request is not listed in the
                                              supported header field in the request */
   SIP_INTERVAL_TOO_BRIEF = 3042,        /**< Expiration time of the resource refreshed by the
                                              request is too short */
   SIP_CALL_OR_TRANS_DOES_NOT_EXIST = 3043,/**< Request received by a UAS does not match any
                                                existing dialog or transaction */
   SIP_LOOP_DETECTED = 3044,             /**< Server detected a loop */
   SIP_TOO_MANY_HOPS = 3045,             /**< Request received has Max-Forwards header field at 0
                                              */
   SIP_AMBIGUOUS = 3046,                 /**< Requested URI was ambiguous */
   SIP_REQUEST_PENDING = 3047,           /**< Request was received by a UAS that had a pending
                                              request within the same dialog */
   SIP_UNDECIPHERABLE = 3048,            /**< Request has an encrypted MIME body for which the
                                              recipient does not possess an appropriate decryption
                                              key */
   RETRY_ON_IMS_WITHOUT_RTT = 3049,      /**< Call should be tried on IMS with RTT disabled */
   MAX_PS_CALLS = 3050,                  /**< Maximum PS calls exceeded */
   SIP_MULTIPLE_CHOICES = 3051,          /**< Multiple choices */
   SIP_MOVED_PERMANENTLY = 3052,         /**< Moved permanently */
   SIP_MOVED_TEMPORARILY = 3053,         /**< Moved temporarily */
   SIP_USE_PROXY = 3054,                 /**< Use proxy */
   SIP_ALTERNATE_SERVICE = 3055,         /**< Alternate service */
   SIP_UNSUPPORTED_URI_SCHEME = 3056,    /**< Unsupported URI scheme */
   SIP_REMOTE_UNSUPP_MEDIA_TYPE = 3057,  /**< Unsupported media type */
   SIP_BAD_EXTENSION = 3058,             /**< Bad extension */
   DSDA_CONCURRENT_CALL_NOT_POSSIBLE = 3059,/**< Concurrent call is not possible */
   EPSFB_FAILURE = 3060,                 /**< Call ended due to evolved packet system fallback
                                              (EPSFB) failure */
   TWAIT_EXPIRED = 3061,                 /**< Call ended due to twait timer expired */
   TCP_CONNECTION_REQ = 3062,            /**< Call ended due to TCP connection */
   THERMAL_EMERGENCY = 3100,             /**< Thermal emergency */
   ERROR_UNSPECIFIED = 0xffff,           /**< Error unspecified */
};

/**
 * Defines the real time text (RTT) mode of a call.
 */
enum class RttMode {
   UNKNOWN = -1,  /*< RTT mode data is unknown */
   DISABLED = 0,  /*< RTT mode is not enabled */
   FULL = 1,      /*< RTT call being used by both the parties*/
};

/** @} */ /* end_addtogroup telematics_call */

/** @addtogroup telematics_phone
 * @{ */

/**
 * Defines the radio state
 */
enum class RadioState {
   RADIO_STATE_OFF = 0,         /**< Radio is explicitly powered off */
   RADIO_STATE_UNAVAILABLE = 1, /**< Radio unavailable (eg, resetting or not booted) */
   RADIO_STATE_ON = 10,         /**< Radio is on */
};

/**
 * Defines the service states
 *
 * @deprecated Use requestVoiceServiceState() API or  to know the status of phone
 */
enum class ServiceState {
   EMERGENCY_ONLY, /**< Only emergency calls allowed */
   IN_SERVICE,     /**< Normal operation, device is registered with a carrier and
                        online */
   OUT_OF_SERVICE, /**< Device is not registered with any carrier */
   RADIO_OFF,      /**< Device radio is off - Airplane mode for example */
};

/**
 * Defines all available radio access technologies
 */
enum class RadioTechnology {
   RADIO_TECH_UNKNOWN,  /**< Network type is unknown */
   RADIO_TECH_GPRS,     /**< Network type is GPRS */
   RADIO_TECH_EDGE,     /**< Network type is EDGE */
   RADIO_TECH_UMTS,     /**< Network type is UMTS */
   RADIO_TECH_IS95A,    /**< Network type is IS95A */
   RADIO_TECH_IS95B,    /**< Network type is IS95B */
   RADIO_TECH_1xRTT,    /**< Network type is 1xRTT */
   RADIO_TECH_EVDO_0,   /**< Network type is EVDO revision 0 */
   RADIO_TECH_EVDO_A,   /**< Network type is EVDO revision A */
   RADIO_TECH_HSDPA,    /**< Network type is HSDPA */
   RADIO_TECH_HSUPA,    /**< Network type is HSUPA */
   RADIO_TECH_HSPA,     /**< Network type is HSPA */
   RADIO_TECH_EVDO_B,   /**< Network type is EVDO revision B*/
   RADIO_TECH_EHRPD,    /**< Network type is eHRPD */
   RADIO_TECH_LTE,      /**< Network type is LTE */
   RADIO_TECH_HSPAP,    /**< Network type is HSPA+ */
   RADIO_TECH_GSM,      /**< Network type is GSM, Only supports voice */
   RADIO_TECH_TD_SCDMA, /**< Network type is TD SCDMA */
   RADIO_TECH_IWLAN,    /**< Network type is TD IWLAN */
   RADIO_TECH_LTE_CA,   /**< Network type is LTE CA */
   RADIO_TECH_NR5G,     /**< Network type is NR5G */
   RADIO_TECH_NB1_NTN,  /**< Network type is NB-IoT(NB1) Non Terrestrial Network(NTN) */
};

/**
 * Defines all available RAT capabilities for each subscription
 */
enum class RATCapability {
   AMPS,     /**< AMPS mode */
   CDMA,     /**< CDMA mode */
   HDR,      /**< HDR mode */
   GSM,      /**< GSM mode */
   WCDMA,    /**< WCDMA mode */
   LTE,      /**< LTE mode */
   TDS,      /**< TD-SCDMA mode */
   NR5G,     /**< NR5G NSA mode */
   NR5GSA,   /**< NR5G SA mode */
   NB1_NTN,  /**< NB-IoT(NB1) Non Terrestrial Network(NTN) mode */
};

using RATCapabilitiesMask = std::bitset<16>;

/**
 * Defines all voice support available on device
 */
enum class VoiceServiceTechnology {
   VOICE_TECH_GW_CSFB,
   VOICE_TECH_1x_CSFB,
   VOICE_TECH_VOLTE,
};

using VoiceServiceTechnologiesMask = std::bitset<16>;

/**
 * Structure contains slotID and RAT capabilities corresponding to slot.
 */
struct SimRatCapability {
   int slotId;  /**< Number of slotid's */
   RATCapabilitiesMask capabilities;  /**< This field indicates RATCapabilitiesMask */
};

/**
 * For Device max subcription capability
 */
using DeviceRatCapability = SimRatCapability;

/**
 * Structure contains information about device capability.
 */
struct CellularCapabilityInfo {
    VoiceServiceTechnologiesMask voiceServiceTechs;  /**< Indicates voice support capabilities */
    int simCount;                                    /**< The maximum number of SIMs that can be supported simultaneously */
    int maxActiveSims;                               /**< The maximum number of SIMs that can be simultaneously active. If this number is less than numberofSims, it implies that any combination of the SIMs can be active and the remaining can be in standby. */
    std::vector<SimRatCapability> simRatCapabilities; /**< A Sim inserted in a slot allows for certain rat capabilities. And the UE's HW allows for certain rat capabilities. This field lists the intersection of capabilities allowed by the Sim and the HW. The capabilities are indexed based on slotId. */
    std::vector<DeviceRatCapability> deviceRatCapability; /**< This field lists the Rat capabilities supported by the HW on a given Sim slot. The capabilities are indexed based on slotId. */
};

/**
 * Defines operating modes of the device.
 */
enum class OperatingMode {
   ONLINE = 0,           /**< Online mode */
   AIRPLANE,             /**< Low Power mode i.e temporarily disabled RF */
   FACTORY_TEST,         /**< Special mode for manufacturer use*/
   OFFLINE,              /**< Device has deactivated RF and partially shutdown */
   RESETTING,            /**< Device is in process of power cycling */
   SHUTTING_DOWN,        /**< Device is in process of shutting down */
   PERSISTENT_LOW_POWER, /**< Persists low power mode even on reset*/
};

/**
 * Emergency callback mode
 */
enum class EcbMode {
   NORMAL = 0, /**< Device is not in emergency callback mode(ECBM) */
   EMERGENCY,  /**< Device is in emergency callback mode(ECBM) */
};

/**
 * Defines the radio SignalStrength types for delta or threshold.
 * @deprecated Use SignalStrengthMeasurementType with RadioTechnology.
 */
enum class RadioSignalStrengthType {
   GSM_RSSI,     /**< GSM received signal strength indicator.*/
   WCDMA_RSSI,   /**< WCDMA received signal strength indicator.*/
   LTE_RSSI,     /**< LTE received signal strength indicator.*/
   LTE_SNR,      /**< LTE signal-to-noise ratio.*/
   LTE_RSRQ,     /**< LTE reference signal received quality.*/
   LTE_RSRP,     /**< LTE reference signal received power.*/
   NR5G_SNR,     /**< NR5G signal-to-noise ratio.*/
   NR5G_RSRP,    /**< NR5G reference signal received power.*/
   NR5G_RSRQ,    /**< NR5G reference signal received quality.*/
};

/**
 * Defines the SignalStrength configuration parameters.
 * @deprecated Use SignalStrengthConfigExType.
 */
enum class SignalStrengthConfigType {
    DELTA = 1,       /**< Signal strength delta provided. */
    THRESHOLD = 2,   /**< Signal strength threshold provided. */
};

/**
 * Defines different configuration types to configure the signal strength notification. Each value
 * represents a corresponding bit for the SignalStrengthConfigMask bitset.
 */
enum SignalStrengthConfigExType {
    DELTA = 1,          /**< Signal strength delta provided. */
    THRESHOLD = 2,      /**< Signal strength threshold list provided. */
    HYSTERESIS_DB = 3,  /**< Signal strength hysteresis delta provided. */
};

/**
 * 8-bit mask that denotes which signal strength config type is used for the signal strength
 * configuration.
 */
using SignalStrengthConfigMask = std::bitset<8>;

/**
 * Defines different signal strength measurement types.
 */
enum class SignalStrengthMeasurementType {
   RSSI,  /**< Received signal strength indicator. */
   ECIO,  /**< Energy per chip to interference power ratio. */
   SINR,  /**< Signal-to-interference-plus-noise ratio. */
   IO,    /**< Interference power ratio. */
   RSRQ,  /**< Reference signal received quality. */
   RSRP,  /**< Reference signal received power. */
   SNR,   /**< Signal-to-noise ratio. */
   RSCP,  /**< Received signal code power. */
};

/**
 * Defines the SignalStrength threshold parameters.
 * @deprecated Use the thresholdList field from SignalStrengthConfigData.
 */
struct SignalStrengthThreshold {
   int32_t lowerRangeThreshold;     /**< Lower threshold for the selected radio technology. */
   int32_t upperRangeThreshold;     /**< Upper threshold for the selected radio technology. */
};

/**
 * Defines the SignalStrength notification configuration parameters and their corresponding values.
 * @deprecated Use SignalStrengthConfigEx.
 */
struct SignalStrengthConfig {
   SignalStrengthConfigType configType;     /**< Signal strength configuration type. */
   RadioSignalStrengthType ratSigType;      /**< Radio signal strength type. */

   /**< Signal strength data. */
   union {
      uint16_t delta;                       /**< Signal strength delta. */
      SignalStrengthThreshold threshold;    /**< Signal strength threshold. */
   };
};

/**
 *  Represents Operator information
 */
struct PlmnInfo {
   std::string longName;               /**< Represents long Name for Network */
   std::string shortName;              /**< Represents short Name for Network */
   std::string plmn;                   /**< Represents PLMN code for Network, consists of a MCC and MNC.  */
   telux::common::BoolValue isHome;    /**< Represents whether the network is the home network, default state is STATE_UNKNOWN */
};

/**
 * Defines the signal strength configuration data parameters.
 */
struct SignalStrengthConfigData {
   SignalStrengthMeasurementType sigMeasType;       /**< Signal strength measurement type. */
   /**< Signal strength data. */
   union {
      uint16_t delta;                               /**< Signal strength delta. */
      struct {
         std::array<int32_t, THRESHOLD_LIST_MAX> thresholdList;
                                                    /**< Signal strength threshold list. */
         uint16_t hysteresisDb = 0;                 /**< (Optional) Signal strength hysteresis delta; note hysteresis db is not mandatory but hystersis db requires that the threshold list is specified. */
      };
   };
};

/**
 * Defines the signal strength notification configuration parameters.
 */
struct SignalStrengthConfigEx {
   SignalStrengthConfigMask configTypeMask;             /**< Signal strength configuration mask. Both delta and threshold can't be sent in single request. Hysteresis db is applicable only when threshold is configured. */
   RadioTechnology radioTech;                           /**< Radio technology. */
   std::vector<SignalStrengthConfigData> sigConfigData; /**< Signal strength data. */
};

/** @} */ /* end_addtogroup telematics_phone */

}  // End of namespace tel

}  // End of namespace telux

#endif // TELUX_TEL_PHONEDEFINES_HPP
