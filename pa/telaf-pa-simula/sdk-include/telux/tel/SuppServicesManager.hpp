/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file       SuppServicesManager.hpp
 *
 * @brief      ISuppServicesManager is the interface to provide supplementary services like
 *             call forwarding, call waiting.
 */

#include <bitset>
#include <vector>
#include <memory>

#include <telux/common/CommonDefines.hpp>
#include <telux/tel/SuppServicesListener.hpp>

#ifndef TELUX_TEL_SUPPSERVICESMANAGER_HPP
#define TELUX_TEL_SUPPSERVICESMANAGER_HPP

namespace telux {
namespace tel {

/** @addtogroup telematics_supp_services
 * @{ */

/**
 * Defines supplementary services status.
 */
enum class SuppServicesStatus {
    UNKNOWN = -1,    /**< Supplementary service status unknown */
    ENABLED = 1,     /**< Supplementary service is enabled */
    DISABLED = 2     /**< Supplementary service is disabled */
};

/**
 * Defines supplementary services provision status.
 */
enum class SuppSvcProvisionStatus {
    UNKNOWN = -1,                   /**< Supplementary service provision status unknown */
    NOT_PROVISIONED = 0,            /**< Supplementary service is not provisioned */
    PROVISIONED = 1,                /**< Supplementary service is provisioned */
    PRESENTATION_RESTRICTED = 2,    /**< Supplementary service is presentation restricted */
    PRESENTATION_ALLOWED = 3,       /**< Supplementary service is presentation allowed */
};

/**
 * Defines call forwarding operation.
 */
enum class ForwardOperation {
    UNKNOWN = -1,     /**< Status unknown */
    ACTIVATE = 1,     /**< To activate call forwarding */
    DEACTIVATE = 2,   /**< To deactivate call forwarding */
    REGISTER = 3,     /**< To register for call forwarding */
    ERASE = 4         /**< To erase the previous registration */
};

/**
 * Defines reasons for call forwarding.
 */
enum class ForwardReason {
    UNCONDITIONAL = 1,      /**< Unconditional call forwarding */
    BUSY = 2,               /**< Forward when the device is busy on another call*/
    NOREPLY = 3,            /**< Forward when there is no reply */
    NOT_REACHABLE = 4,      /**< Forward when the device is unreachable */
    NOT_LOGGED_IN = 23      /**< Forward when the device is not logged in */
};

/**
 * Defines service class for telephony
 */
enum class ServiceClassType {
    NONE = 0x00,    /**< Service class not provided */
    VOICE = 0x01,   /**< Service class voice */
};

/**
 * 8 bit mask that denotes which of the service class to be used. @ref telux::tel::ServiceClassType
 */
using ServiceClass = std::bitset<8>;

/**
 * Represents parameters for forwarding.
 */
struct ForwardInfo {
    SuppServicesStatus status;   /**< Status of the supplemetary service */
    ServiceClass serviceClass;   /**< Service class */
    std::string number = "";     /**< Phone number to which the call to be forwarded */
    uint8_t noReplyTimer = 0;    /**< No reply timer */
};

/**
 * Represents parameters required for forwarding request.
 */
struct ForwardReq {
    ForwardOperation operation;    /**< Type of operation for forwarding */
    ForwardReason reason;          /**< Reason for call forwarding @ref telux::tel::ForwardReason */
    ServiceClass serviceClass;     /**< Service Class for operation @ref telux::tel::ServiceClass */
    std::string number = "";       /**< Number to which call has to be forwarded. This parameter is required only for registration purpose only. @ref telux::tel::ForwardOperation::REGISTER */
    uint8_t noReplyTimer = 0;      /**< Timer for no reply operation. Required only for no reply forward reason. @ref telux::tel::ForwardReason::NOREPLY. */
};

/**
 * Represents the cause for supplementary services failure.
 */
enum class FailureCause {
    UNAVAILABLE = 0xFFFF,
    OFFLINE = 0x00,
    CDMA_LOCK = 0x14,
    NO_SRV = 0x15,
    FADE = 0x16,
    INTERCEPT = 0x17,
    REORDER = 0x18,
    REL_NORMAL = 0x19,
    REL_SO_REJ = 0x1A,
    INCOM_CALL = 0x1B,
    ALERT_STOP = 0x1C,
    CLIENT_END = 0x1D,
    ACTIVATION = 0x1E,
    MC_ABORT = 0x1F,
    MAX_ACCESS_PROBE = 0x20,
    PSIST_N = 0x21,
    UIM_NOT_PRESENT = 0x22,
    ACC_IN_PROG = 0x23,
    ACC_FAIL = 0x24,
    RETRY_ORDER = 0x25,
    CCS_NOT_SUPPORTED_BY_BS = 0x26,
    NO_RESPONSE_FROM_BS = 0x27,
    REJECTED_BY_BS = 0x28,
    INCOMPATIBLE = 0x29,
    ACCESS_BLOCK = 0x2A,
    ALREADY_IN_TC = 0x2B,
    EMERGENCY_FLASHED = 0x2C,
    USER_CALL_ORIG_DURING_GPS = 0x2D,
    USER_CALL_ORIG_DURING_SMS = 0x2E,
    USER_CALL_ORIG_DURING_DATA = 0x2F,
    REDIR_OR_HANDOFF = 0x30,
    ACCESS_BLOCK_ALL = 0x31,
    OTASP_SPC_ERR = 0x32,
    IS707B_MAX_ACC = 0x33,
    ACC_FAIL_REJ_ORD = 0x34,
    ACC_FAIL_RETRY_ORD = 0x35,
    TIMEOUT_T42 = 0x36,
    TIMEOUT_T40 = 0x37,
    SRV_INIT_FAIL = 0x38,
    T50_EXP = 0x39,
    T51_EXP = 0x3A,
    RL_ACK_TIMEOUT = 0x3B,
    BAD_FL = 0x3C,
    TRM_REQ_FAIL = 0x3D,
    TIMEOUT_T41 = 0x3E,
    INCOM_REJ = 0x66,
    SETUP_REJ = 0x67,
    NETWORK_END = 0x68,
    NO_FUNDS = 0x69,
    NO_GW_SRV = 0x6A,
    NO_CDMA_SRV = 0x6B,
    NO_FULL_SRV = 0x6C,
    MAX_PS_CALLS = 0x6D,
    UNKNOWN_SUBSCRIBER = 0x6E,
    ILLEGAL_SUBSCRIBER = 0x6F,
    BEARER_SERVICE_NOT_PROVISIONED = 0x70,
    TELE_SERVICE_NOT_PROVISIONED = 0x71,
    ILLEGAL_EQUIPMENT = 0x72,
    CALL_BARRED = 0x73,
    ILLEGAL_SS_OPERATION = 0x74,
    SS_ERROR_STATUS = 0x75,
    SS_NOT_AVAILABLE = 0x76,
    SS_SUBSCRIPTION_VIOLATION = 0x77,
    SS_INCOMPATIBILITY = 0x78,
    FACILITY_NOT_SUPPORTED = 0x79,
    ABSENT_SUBSCRIBER = 0x7A,
    SHORT_TERM_DENIAL = 0x7B,
    LONG_TERM_DENIAL = 0x7C,
    SYSTEM_FAILURE = 0x7D,
    DATA_MISSING = 0x7E,
    UNEXPECTED_DATA_VALUE = 0x7F,
    PWD_REGISTRATION_FAILURE = 0x80,
    NEGATIVE_PWD_CHECK = 0x81,
    NUM_OF_PWD_ATTEMPTS_VIOLATION = 0x82,
    POSITION_METHOD_FAILURE = 0x83,
    UNKNOWN_ALPHABET = 0x84,
    USSD_BUSY = 0x85,
    REJECTED_BY_USER = 0x86,
    REJECTED_BY_NETWORK = 0x87,
    DEFLECTION_TO_SERVED_SUBSCRIBER = 0x88,
    SPECIAL_SERVICE_CODE = 0x89,
    INVALID_DEFLECTED_TO_NUMBER = 0x8A,
    MPTY_PARTICIPANTS_EXCEEDED = 0x8B,
    RESOURCES_NOT_AVAILABLE = 0x8C,
    UNASSIGNED_NUMBER = 0x8D,
    NO_ROUTE_TO_DESTINATION = 0x8E,
    CHANNEL_UNACCEPTABLE = 0x8F,
    OPERATOR_DETERMINED_BARRING = 0x90,
    NORMAL_CALL_CLEARING = 0x91,
    USER_BUSY = 0x92,
    NO_USER_RESPONDING = 0x93,
    USER_ALERTING_NO_ANSWER = 0x94,
    CALL_REJECTED = 0x95,
    NUMBER_CHANGED = 0x96,
    PREEMPTION = 0x97,
    DESTINATION_OUT_OF_ORDER = 0x98,
    INVALID_NUMBER_FORMAT = 0x99,
    FACILITY_REJECTED = 0x9A,
    RESP_TO_STATUS_ENQUIRY = 0x9B,
    NORMAL_UNSPECIFIED = 0x9C,
    NO_CIRCUIT_OR_CHANNEL_AVAILABLE = 0x9D,
    NETWORK_OUT_OF_ORDER = 0x9E,
    TEMPORARY_FAILURE = 0x9F,
    SWITCHING_EQUIPMENT_CONGESTION = 0xA0,
    ACCESS_INFORMATION_DISCARDED = 0xA1,
    REQUESTED_CIRCUIT_OR_CHANNEL_NOT_AVAILABLE = 0xA2,
    RESOURCES_UNAVAILABLE_OR_UNSPECIFIED = 0xA3,
    QOS_UNAVAILABLE = 0xA4,
    REQUESTED_FACILITY_NOT_SUBSCRIBED = 0xA5,
    INCOMING_CALLS_BARRED_WITHIN_CUG = 0xA6,
    BEARER_CAPABILITY_NOT_AUTH = 0xA7,
    BEARER_CAPABILITY_UNAVAILABLE = 0xA8,
    SERVICE_OPTION_NOT_AVAILABLE = 0xA9,
    ACM_LIMIT_EXCEEDED = 0xAA,
    BEARER_SERVICE_NOT_IMPLEMENTED = 0xAB,
    REQUESTED_FACILITY_NOT_IMPLEMENTED = 0xAC,
    ONLY_DIGITAL_INFORMATION_BEARER_AVAILABLE = 0xAD,
    SERVICE_OR_OPTION_NOT_IMPLEMENTED = 0xAE,
    INVALID_TRANSACTION_IDENTIFIER = 0xAF,
    USER_NOT_MEMBER_OF_CUG = 0xB0,
    INCOMPATIBLE_DESTINATION = 0xB1,
    INVALID_TRANSIT_NW_SELECTION = 0xB2,
    SEMANTICALLY_INCORRECT_MESSAGE = 0xB3,
    INVALID_MANDATORY_INFORMATION = 0xB4,
    MESSAGE_TYPE_NON_IMPLEMENTED = 0xB5,
    MESSAGE_TYPE_NOT_COMPATIBLE_WITH_PROTOCOL_STATE = 0xB6,
    INFORMATION_ELEMENT_NON_EXISTENT = 0xB7,
    CONDITONAL_IE_ERROR = 0xB8,
    MESSAGE_NOT_COMPATIBLE_WITH_PROTOCOL_STATE = 0xB9,
    RECOVERY_ON_TIMER_EXPIRED = 0xBA,
    PROTOCOL_ERROR_UNSPECIFIED = 0xBB,
    INTERWORKING_UNSPECIFIED = 0xBC,
    OUTGOING_CALLS_BARRED_WITHIN_CUG = 0xBD,
    NO_CUG_SELECTION = 0xBE,
    UNKNOWN_CUG_INDEX = 0xBF,
    CUG_INDEX_INCOMPATIBLE = 0xC0,
    CUG_CALL_FAILURE_UNSPECIFIED = 0xC1,
    CLIR_NOT_SUBSCRIBED = 0xC2,
    CCBS_POSSIBLE = 0xC3,
    CCBS_NOT_POSSIBLE = 0xC4,
    IMSI_UNKNOWN_IN_HLR = 0xC5,
    ILLEGAL_MS = 0xC6,
    IMSI_UNKNOWN_IN_VLR = 0xC7,
    IMEI_NOT_ACCEPTED = 0xC8,
    ILLEGAL_ME = 0xC9,
    PLMN_NOT_ALLOWED = 0xCA,
    LOCATION_AREA_NOT_ALLOWED = 0xCB,
    ROAMING_NOT_ALLOWED_IN_THIS_LOCATION_AREA = 0xCC,
    NO_SUITABLE_CELLS_IN_LOCATION_AREA = 0xCD,
    NETWORK_FAILURE = 0xCE,
    MAC_FAILURE = 0xCF,
    SYNCH_FAILURE = 0xD0,
    NETWORK_CONGESTION = 0xD1,
    GSM_AUTHENTICATION_UNACCEPTABLE = 0xD2,
    SERVICE_NOT_SUBSCRIBED = 0xD3,
    SERVICE_TEMPORARILY_OUT_OF_ORDER = 0xD4,
    CALL_CANNOT_BE_IDENTIFIED = 0xD5,
    INCORRECT_SEMANTICS_IN_MESSAGE = 0xD6,
    MANDATORY_INFORMATION_INVALID = 0xD7,
    ACCESS_STRATUM_FAILURE = 0xD8,
    INVALID_SIM = 0xD9,
    WRONG_STATE = 0xDA,
    ACCESS_CLASS_BLOCKED = 0xDB,
    NO_RESOURCES = 0xDC,
    INVALID_USER_DATA = 0xDD,
    TIMER_T3230_EXPIRED = 0xDE,
    NO_CELL_AVAILABLE = 0xDF,
    ABORT_MSG_RECEIVED = 0xE0,
    RADIO_LINK_LOST = 0xE1,
    TIMER_T303_EXPIRED = 0xE2,
    CNM_MM_REL_PENDING = 0xE3,
    ACCESS_STRATUM_REJ_RR_REL_IND = 0xE4,
    ACCESS_STRATUM_REJ_RR_RANDOM_ACCESS_FAILURE = 0xE5,
    ACCESS_STRATUM_REJ_RRC_REL_IND = 0xE6,
    ACCESS_STRATUM_REJ_RRC_CLOSE_SESSION_IND = 0xE7,
    ACCESS_STRATUM_REJ_RRC_OPEN_SESSION_FAILURE = 0xE8,
    ACCESS_STRATUM_REJ_LOW_LEVEL_FAIL = 0xE9,
    ACCESS_STRATUM_REJ_LOW_LEVEL_FAIL_REDIAL_NOT_ALLOWED = 0xEA,
    ACCESS_STRATUM_REJ_LOW_LEVEL_IMMED_RETRY = 0xEB,
    ACCESS_STRATUM_REJ_ABORT_RADIO_UNAVAILABLE = 0xEC,
    SERVICE_OPTION_NOT_SUPPORTED = 0xED,
    ACCESS_STRATUM_REJ_CONN_EST_FAILURE_ACCESS_BARRED = 0xEE,
    ACCESS_STRATUM_REJ_CONN_REL_NORMAL = 0xEF,
    ACCESS_STRATUM_REJ_UL_DATA_CNF_FAILURE_CONN_REL = 0xF0,
    BAD_REQ_WAIT_INVITE = 0x12C,
    BAD_REQ_WAIT_REINVITE = 0x12D,
    INVALID_REMOTE_URI = 0x12E,
    REMOTE_UNSUPP_MEDIA_TYPE = 0x12F,
    PEER_NOT_REACHABLE = 0x130,
    NETWORK_NO_RESP_TIME_OUT = 0x131,
    NETWORK_NO_RESP_HOLD_FAIL = 0x132,
    DATA_CONNECTION_LOST = 0x133,
    UPGRADE_DOWNGRADE_REJ = 0x134,
    SIP_403_FORBIDDEN = 0x135,
    NO_NETWORK_RESP = 0x136,
    UPGRADE_DOWNGRADE_FAILED = 0x137,
    UPGRADE_DOWNGRADE_CANCELLED = 0x138,
    SSAC_REJECT = 0x139,
    THERMAL_EMERGENCY = 0x13A,
    FAILURE_1XCSFB_SOFT = 0x13B,
    FAILURE_1XCSFB_HARD = 0x13C,
    CONNECTION_EST_FAILURE = 0x13D,
    CONNECTION_FAILURE = 0x13E,
    RRC_CONN_REL_NO_MT_SETUP = 0x13F,
    ESR_FAILURE = 0x140,
    MT_CSFB_NO_RESPONSE_FROM_NW = 0x141,
    BUSY_EVERYWHERE = 0x142,
    ANSWERED_ELSEWHERE = 0x143,
    RLF_DURING_CC_DISCONNECT = 0x144,
    TEMP_REDIAL_ALLOWED = 0x145,
    PERM_REDIAL_NOT_NEEDED = 0x146,
    MERGED_TO_CONFERENCE = 0x147,
    LOW_BATTERY = 0x148,
    CALL_DEFLECTED = 0x149,
    RTP_RTCP_TIMEOUT = 0x14A,
    RINGING_RINGBACK_TIMEOUT = 0x14B,
    REG_RESTORATION = 0x14C,
    CODEC_ERROR = 0x14D,
    UNSUPPORTED_SDP = 0x14E,
    RTP_FAILURE = 0x14F,
    QoS_FAILURE = 0x150,
    MULTIPLE_CHOICES = 0x151,
    MOVED_PERMANENTLY = 0x152,
    MOVED_TEMPORARILY = 0x153,
    USE_PROXY = 0x154,
    ALTERNATE_SERVICE = 0x155,
    ALTERNATE_EMERGENCY_CALL = 0x156,
    UNAUTHORIZED = 0x157,
    PAYMENT_REQUIRED = 0x158,
    METHOD_NOT_ALLOWED = 0x159,
    NOT_ACCEPTABLE = 0x15A,
    PROXY_AUTHENTICATION_REQUIRED = 0x15B,
    GONE = 0x15C,
    REQUEST_ENTITY_TOO_LARGE = 0x15D,
    REQUEST_URI_TOO_LARGE = 0x15E,
    UNSUPPORTED_URI_SCHEME = 0x15F,
    BAD_EXTENSION = 0x160,
    EXTENSION_REQUIRED = 0x161,
    INTERVAL_TOO_BRIEF = 0x162,
    CALL_OR_TRANS_DOES_NOT_EXIST = 0x163,
    LOOP_DETECTED = 0x164,
    TOO_MANY_HOPS = 0x165,
    ADDRESS_INCOMPLETE = 0x166,
    AMBIGUOUS = 0x167,
    REQUEST_TERMINATED = 0x168,
    NOT_ACCEPTABLE_HERE = 0x169,
    REQUEST_PENDING = 0x16A,
    UNDECIPHERABLE = 0x16B,
    SERVER_INTERNAL_ERROR = 0x16C,
    NOT_IMPLEMENTED = 0x16D,
    BAD_GATEWAY = 0x16E,
    SERVER_TIME_OUT = 0x16F,
    VERSION_NOT_SUPPORTED = 0x170,
    MESSAGE_TOO_LARGE = 0x171,
    DOES_NOT_EXIST_ANYWHERE = 0x172,
    SESS_DESCR_NOT_ACCEPTABLE = 0x173,
    SRVCC_END_CALL = 0x174,
    INTERNAL_ERROR = 0x175,
    SERVER_UNAVAILABLE = 0x176,
    PRECONDITION_FAILURE = 0x177,
    DRVCC_IN_PROG = 0x178,
    DRVCC_END_CALL = 0x179,
    CS_HARD_FAILURE = 0x17A,
    CS_ACQ_FAILURE = 0x17B,
    REJECTED_ELSEWHERE = 0x180,
    CALL_PULLED = 0x181,
    CALL_PULL_OUT_OF_SYNC = 0x182,
    HOLD_RESUME_FAILED = 0x183,
    HOLD_RESUME_CANCELED = 0x184,
    REINVITE_COLLISION = 0x185,
    REDIAL_SECONDARY_LINE_CS = 0x186,
    REDIAL_SECONDARY_LINE_PS = 0x187,
    REDIAL_SECONDARY_LINE_CS_AUTO = 0x188,
    REDIAL_SECONDARY_LINE_PS_AUTO = 0x189
};

/**
 * This function is called with the response to setCallWaitingPref and setForwardingPref APIs.
 *
 * The callback can be invoked from multiple different threads.
 * The implementation should be thread safe.
 *
 * @param [in] error         -  Return code which indicates whether the operation
 *                              succeeded or not @ref telux::common::ErrorCode
 * @param [in] failureCause  -  Failure cause populated only in case of errors
 *                              @ref telux::tel::FailureCause.
 *
 */
using SetSuppSvcPrefCallback
    = std::function<void(telux::common::ErrorCode error, FailureCause failureCause)>;

/**
 * This function is called with the response to requestCallWaitingPrefEx API.
 *
 * The callback can be invoked from multiple different threads.
 * The implementation should be thread safe.
 *
 * @param [in] suppSvcStatus    -  Call waiting status @ref telux::tel::SuppServicesStatus
 * @param [in] failureCause     -  Failure cause populated only in case of errors
 *                                 @ref telux::tel::FailureCause.
 * @param [in] error            -  Return code which indicates whether the operation
 *                                 succeeded or not @ref telux::common::ErrorCode
 *
 */
using GetCallWaitingPrefExCb
    = std::function<void(SuppServicesStatus suppSvcStatus, FailureCause failureCause,
        telux::common::ErrorCode error)>;

/**
 * This function is called with the response to requestForwardingPrefEx API.
 *
 * The callback can be invoked from multiple different threads.
 * The implementation should be thread safe.
 *
 * @param [in] forwardInfoList     - List of forward info @ref telux::tel::ForwardInfo.
 *                                   Multiple info are received when different service class are
 *                                   forwarded to different numbers.
 * @param [in] failureCause        - Returns failure cause in case the request fails.
 * @param [in] error               - Return code which indicates whether the operation
 *                                   succeeded or not @ref telux::common::ErrorCode
 *
 */
using GetForwardingPrefExCb
    = std::function<void(std::vector<ForwardInfo> forwardInfoList, FailureCause failureCause,
        telux::common::ErrorCode error)>;

/**
 * This function is called with the response to the requestOirPref API.
 *
 * The callback can be invoked from multiple different threads.
 * The implementation should be thread safe.
 *
 * @param [in] suppSvcStatus       - OIR status @ref telux::tel::SuppServicesStatus
 * @param [in] provisionStatus     - Provision status @ref telux::tel::SuppSvcProvisionStatus.
 * @param [in] failureCause        - Returns failure cause in case the request fails.
 * @param [in] error               - Return code which indicates whether the operation
 *                                   succeeded or not @ref telux::common::ErrorCode
 *
 */
using GetOirPrefCb
    = std::function<void(SuppServicesStatus suppSvcStatus, SuppSvcProvisionStatus
        provisionStatus, FailureCause failureCause, telux::common::ErrorCode error)>;


// Deprecated Callbacks

/**
 * This function is called with the response to requestCallWaitingPref API.
 *
 * The callback can be invoked from multiple different threads.
 * The implementation should be thread safe.
 *
 * @param [in] suppSvcStatus    -  Call waiting status @ref telux::tel::SuppServicesStatus
 * @param [in] provisionStatus  -  Provision status @ref telux::tel::SuppSvcProvisionStatus.
 * @param [in] failureCause     -  Failure cause populated only in case of errors
 *                                 @ref telux::tel::FailureCause.
 * @param [in] error            -  Return code which indicates whether the operation
 *                                 succeeded or not @ref telux::common::ErrorCode
 *
 * @deprecated Use GetCallWaitingPrefExCb callback.
 *
 */
using GetCallWaitingPrefCb
    = std::function<void(SuppServicesStatus suppSvcStatus, SuppSvcProvisionStatus provisionStatus,
        FailureCause failureCause, telux::common::ErrorCode error)>;

/**
 * This function is called with the response to requestForwardingPref API.
 *
 * The callback can be invoked from multiple different threads.
 * The implementation should be thread safe.
 *
 * @param [in] forwardInfoList     - List of forward info @ref telux::tel::ForwardInfo.
 *                                   Multiple info are received when different service class are
 *                                   forwarded to different numbers.
 * @param [in] provisionStatus     - Provision status @ref telux::tel::SuppSvcProvisionStatus.
 * @param [in] failureCause        - Returns failure cause in case the request fails.
 * @param [in] error               - Return code which indicates whether the operation
 *                                   succeeded or not @ref telux::common::ErrorCode
 *
 * @deprecated Use GetForwardingPrefExCb callback.
 *
 */
using GetForwardingPrefCb
    = std::function<void(std::vector<ForwardInfo> forwardInfoList, SuppSvcProvisionStatus
        provisionStatus, FailureCause failureCause, telux::common::ErrorCode error)>;

/**
 * @brief ISuppServicesManager is the interface to provide supplementary services like call
 * forwarding and call waiting.
 */
class ISuppServicesManager {
public:

    /**
     * This status indicates whether the ISuppServicesManager object is in a usable state.
     *
     * @returns SERVICE_AVAILABLE   - If ISuppServicesManager manager is ready for service.
     *          SERVICE_UNAVAILABLE - If ISuppServicesManager manager is temporarily unavailable.
     *          SERVICE_FAILED      - If ISuppServicesManager manager encountered an irrecoverable
     *                                failure.
     *
     */
    virtual telux::common::ServiceStatus getServiceStatus() = 0;

    /**
     * Enable/disable call waiting on device.
     *
     * On platforms with Access control enabled, Caller needs to have TELUX_TEL_SUPP_SERVICES
     * permissions to invoke this API successfully.
     *
     * @param [in] suppSvcStatus -  Call waiting preference @ref telux::tel::SuppServicesStatus.
     * @param [in] callback      -  Callback function to get the response of setCallWaitingPref
     *
     * @returns Status of setCallWaitingPref i.e. success or suitable error code.
     *
     */
    virtual telux::common::Status setCallWaitingPref(SuppServicesStatus suppSvcStatus,
        SetSuppSvcPrefCallback callback = nullptr) = 0;

    /**
     * This API queries the preference for call waiting.
     *
     * On platforms with Access control enabled, Caller needs to have TELUX_TEL_SUPP_SERVICES
     * permissions to invoke this API successfully.
     *
     * @param [in] callback  -  Callback function to get the response of call waiting
     *                          preference.
     * @returns Status of requestCallWaitingPref i.e. success or suitable error code.
     *
     */
    virtual telux::common::Status requestCallWaitingPref(GetCallWaitingPrefExCb callback) = 0;

    /**
     * To set call forwarding preference.
     *
     * On platforms with Access control enabled, Caller needs to have TELUX_TEL_SUPP_SERVICES
     * permissions to invoke this API successfully.
     *
     * @param [in] forwardReq      -  Parameters for call forwarding operation.
     *                                @ref telux::tel::ForwardReq
     * @param [in] callback        -  Callback function to get response of setForwardingPref API.
     *
     * @returns Status of setForwardingPref i.e. success or suitable error code.
     *
     */
    virtual telux::common::Status setForwardingPref(ForwardReq forwardReq,
        SetSuppSvcPrefCallback callback = nullptr) = 0;

    /**
     * This API queries preference for call forwarding supplementary service. If active, returns
     * for which service classes and call forwarding number it is active.
     * There is an option to configure for which service class the request is made, if the option
     * is not configured it assumes that the request is made for all service classes.
     *
     * On platforms with Access control enabled, Caller needs to have TELUX_TEL_SUPP_SERVICES
     * permissions to invoke this API successfully.
     *
     * @param [in] serviceClass -  Service class @ref telux::tel::ServiceClass.
     * @param [in] callback     -  Callback function to get the response of request call forwarding
     *                             preference.
     *
     * @returns Status of requestForwardingPref i.e. success or suitable error code.
     *
     */
    virtual telux::common::Status requestForwardingPref(ServiceClass serviceClass,
        ForwardReason reason, GetForwardingPrefExCb callback) = 0;

    /**
     * Activate/Deactivate originating identification restriction preference on the device.
     * If the OIR service was activated, the original call number will be restricted to the
     * target when a call is dialed to a subscriber.
     *
     * On platforms with access control enabled, the caller must have TELUX_TEL_SUPP_SERVICES
     * permissions to invoke this API successfully.
     *
     * @param [in] serviceClass  -  Service class @ref telux::tel::ServiceClass.
     * @param [in] suppSvcStatus -  OIR Status @ref telux::tel::SuppServicesStatus.
     * @param [in] callback      -  Callback function to get the response of setOIRPref
     *
     * @returns Status of setOirPref i.e. success or suitable error code.
     *
     */
    virtual telux::common::Status setOirPref(ServiceClass serviceClass,
        SuppServicesStatus suppSvcStatus, SetSuppSvcPrefCallback callback = nullptr) = 0;

    /**
     * This API queries the originating identification restriction preference.
     *
     * On platforms with access control enabled, the caller must have TELUX_TEL_SUPP_SERVICES
     * permissions to invoke this API successfully.
     *
     * @param [in] serviceClass  -  Service class @ref telux::tel::ServiceClass.
     * @param [in] callback      -  Callback function to get the response of requestOIRPref
     *
     * @returns Status of requestOirPref i.e. success or suitable error code.
     *
     */
    virtual telux::common::Status requestOirPref(ServiceClass serviceClass,
        GetOirPrefCb callback) = 0;

    /**
    * Register a listener for supplementary services events.
    *
    * @param [in] listener   Pointer to ISuppServicesListener object that
    *                        processes the notification.
    *
    *
    * @returns Status of registerListener i.e. success or suitable status code.
    */
    virtual telux::common::Status registerListener(std::weak_ptr<ISuppServicesListener> listener)
      = 0;

   /**
    * Remove a previously added listener.
    *
    * @param [in] listener   Pointer to ISuppServicesListener object that needs
    *                        to be removed.
    *
    *
    * @returns Status of removeListener i.e. success or suitable status code.
    */
   virtual telux::common::Status removeListener(std::weak_ptr<ISuppServicesListener> listener) = 0;

    /**
     * Destructor for ISupplementaryServicesManager
     */
    virtual ~ISuppServicesManager() {
    }


    // Deprecated APIs

    /**
     * This API queries the preference for call waiting.
     *
     * @param [in] callback  -  Callback function to get the response of requestCallWaitingPref.
     *
     * @returns Status of requestCallWaitingPref i.e. success or suitable error code.
     *
     * @deprecated This API is not being supported instead use requestCallWaitingPref(
     *             GetCallWaitingPrefExCb) API.
     *
     */
    virtual telux::common::Status requestCallWaitingPref(GetCallWaitingPrefCb callback) = 0;

    /**
     * This API queries preference for call forwarding supplementary service. If active, returns
     * for which service classes and call forwarding number it is active. It also returns the
     * provision status of the supplemetary service.
     * There is an option to configure for which service class the request is made, if the option
     * is not configured it assumes that the request is made for all service classes.
     *
     * @param [in] serviceClass -  Service class @ref telux::tel::ServiceClass.
     * @param [in] callback     -  Callback function to get the response of request call forwarding
     *                             preference.
     *
     * @returns Status of requestForwardingPref i.e. success or suitable error code.
     *
     * @deprecated This API is not being supported instead use requestForwardingPref(
     *             ServiceClass serviceClass, ForwardReason reason,
     *             GetForwardingPrefExCb callback) API.
     *
     */
    virtual telux::common::Status requestForwardingPref(ServiceClass serviceClass,
        ForwardReason reason, GetForwardingPrefCb callback) = 0;

}; // end of ISuppServicesManager

/** @} */ /* end_addtogroup telematics_supp_services */

} // namespace tel
} // namespace telux

#endif // TELUX_TEL_SUPPSERVICESMANAGER_HPP
