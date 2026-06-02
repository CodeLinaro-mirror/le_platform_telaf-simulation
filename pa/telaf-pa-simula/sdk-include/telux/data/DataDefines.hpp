/*
 *  Copyright (c) 2018-2021, The Linux Foundation. All rights reserved.
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
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file       DataDefines.hpp
 * @brief      DataDefines contains enumerations and variables used for data services
 *
 */

#ifndef TELUX_DATA_DATADEFINES_HPP
#define TELUX_DATA_DATADEFINES_HPP

#include <string>
#include <vector>
#include <bitset>
#include <cstdint>

#include <telux/common/CommonDefines.hpp>
#include <telux/common/ConnectivityDefines.hpp>
namespace telux {
namespace data {

/** @addtogroup telematics_data
 * @{ */

/**
 * Default data profile id.
 */
#define PROFILE_ID_MAX 0x7FFFFFFF

/**
 * Max filters in one flow
 */
#define MAX_QOS_FILTERS 16

/**
 * Preferred IP family for the connection
 */
enum class IpFamilyType {
    UNKNOWN = -1,
    IPV4 = 0x04,   /**< IPv4 data connection */
    IPV6 = 0x06,   /**< IPv6 data connection */
    IPV4V6 = 0x0A, /**< IPv4 and IPv6 data connection */
};

/**
 * Network type
 */
enum class NetworkType {
    UNKNOWN = -1,
    LAN = 1,   /**< LAN network type */
    WAN = 2,   /**< WAN network type */
};

/**
 * Specifies operation
 */
enum class Operation {
    UNKNOWN = -1,    /**< UNKNOWN operation */
    DISABLE = 0,     /**< DISABLE operation */
    ENABLE  = 1,     /**< ENABLE operation  */
};

/**
 * Technology Preference
 */
enum class TechPreference {
    UNKNOWN = -1,
    TP_3GPP,  /**< UMTS, LTE */
    TP_3GPP2, /**< CDMA */
    TP_ANY,   /**< ANY (3GPP or 3GPP2)  */
};

/**
 * Authentication protocol preference type to be used for PDP context.
 */
enum class AuthProtocolType {
    AUTH_NONE = 0,
    AUTH_PAP = 1,  /**< Password Authentication Protocol */
    AUTH_CHAP = 2, /**< Challenge Handshake Authentication Protocol */
    AUTH_PAP_CHAP = 3,
};

/**
 * Defines the supported filtering mode of the packet data session.
 */
enum class DataRestrictModeType {
    UNKNOWN = -1,
    DISABLE,
    ENABLE,
};

/* Specifies the link state
 */
enum class LinkState {
   UNKNOWN = -1, /**< link is UNKNOWN */
   UP   = 1,   /**< link is UP   */
   DOWN = 2,   /**< link is DOWN */
};

/**
 * Defines the supported powersave filtering mode and autoexit for the packet data session.
 * @ref DataRestrictModeType
 */
struct DataRestrictMode {
    DataRestrictModeType filterMode; 
   /**< Disable or enable data filter mode. When disabled all the
        data packets will be forwarded from modem to the apps.
        When enabled only the data matching the filters will be 
        forwarded from modem to the apps. */
    DataRestrictModeType filterAutoExit; 
     /**< Disable or enable autoexit feature. When enabled, once
          an incoming packet matching the filter is received,
          filter mode will we disable automatically and any
          packet will be allowed to be forwarded from modem to apps.*/
};

/**
 * Used to define the Port number and range (number of ports following port value)
 * Ex- for ports ranging from 1000-3000
 * port = 1000 and range= 2000
 *
 * for single port 5000
 * port = 5000 and range= 0
 */
struct PortInfo {
    uint16_t port = 0;  /**< Port. */
    uint16_t range = 0; /**< Range. */
};

/**
 * Specifies APN types that can be set while creating or modifying a profile
 */
enum ApnMaskType {
    APN_MASK_TYPE_DEFAULT     = (1 << 0),   /**< APN type for default/internet traffic  */
    APN_MASK_TYPE_IMS         = (1 << 1),   /**< APN type for the IP multimedia subsystem  */
    APN_MASK_TYPE_MMS         = (1 << 2),   /**< APN type for the multimedia messaging service  */
    APN_MASK_TYPE_DUN         = (1 << 3),   /**< APN type for the dial up network  */
    APN_MASK_TYPE_SUPL        = (1 << 4),   /**< APN type for secure user plane location  */
    APN_MASK_TYPE_HIPRI       = (1 << 5),   /**< APN type for high priority mobile data  */
    APN_MASK_TYPE_FOTA        = (1 << 6),   /**< APN type for over the air administration  */
    APN_MASK_TYPE_CBS         = (1 << 7),   /**< APN type for carrier branded services  */
    APN_MASK_TYPE_IA          = (1 << 8),   /**< APN type for initial attach  */
    APN_MASK_TYPE_EMERGENCY   = (1 << 9),   /**< APN type for emergency  */
    APN_MASK_TYPE_UT          = (1 << 10),  /**< APN type for UT  */
    APN_MASK_TYPE_MCX         = (1 << 11),  /**< APN type for mission critical service  */
};

/**
 * 16 bit mask to set apn types paramater.
 * ApnMaskType enum are used to set apn types.
 */
using ApnTypes = std::bitset<16>;

/**
 * This type represents if the emergency call
 * can be performed on a particular profile.
 * When telux::common::Status telux::data::IDataProfileManager::createProfile
 * or telux::common::Status telux::data::IDataProfileManager::modifyProfile
 * are invoked and the emergency capability is set to UNSPECIFIED, the implementation
 * of the corresponding APIs will default it to NOT_ALLOWED.
 */
enum class EmergencyCapability {
    UNSPECIFIED = 0,  /**< Emergency capability is not specified */
    ALLOWED,          /**< Emergency call is allowed on this profile */
    NOT_ALLOWED,      /**< Emergency call is not allowed on this profile */
};

/**
 * Profile Parameters used for profile creation, query and modify
 */
struct ProfileParams {
    std::string profileName;                                 /**< Profile Name */
    std::string apn;                                         /**< APN name */
    std::string userName;                                    /**< APN user name (if any) */
    std::string password;                                    /**< APN password (if any) */
    TechPreference techPref = TechPreference::UNKNOWN;       /**< Technology preference, default is TechPreference::UNKNOWN */
    AuthProtocolType authType = AuthProtocolType::AUTH_NONE; /**< Authentication protocol type, default is AuthProtocolType::AUTH_NONE */
    IpFamilyType ipFamilyType = IpFamilyType::UNKNOWN;       /**< Preferred IP family for the call,
                                                                  default is
                                                                  IpFamilyType::UNKNOWN */
    ApnTypes apnTypes;                                       /**< APN Types @ref ApnMaskType */
    EmergencyCapability emergencyAllowed =
      telux::data::EmergencyCapability::UNSPECIFIED;         /**< Emergency services are allowed if
                                                               this field is set to ALLOWED*/
    bool clatEnabled = false;                                /**< Enable or disable CLAT */
};

/**
 * Data transfer statistics structure.
 */
struct DataCallStats {
    uint64_t packetsTx = 0;               /**< Number of packets transmitted */
    uint64_t packetsRx = 0;               /**< Number of packets received */
    uint64_t bytesTx = 0;                 /**< Number of bytes transmitted */
    uint64_t bytesRx = 0;                 /**< Number of bytes received */
    uint64_t packetsDroppedTx = 0;        /**< Number of transmit packets dropped */
    uint64_t packetsDroppedRx = 0;        /**< Number of receive packets dropped */
};

/**
 * Data call event status
 */
enum class DataCallStatus {
    INVALID = 0x00,    /**<  Invalid  */
    NET_CONNECTED,     /**< Call is connected */
    NET_NO_NET,        /**< Call is disconnected */
    NET_IDLE,          /**< Call is in idle state */
    NET_CONNECTING,    /**< Call is in connecting state */
    NET_DISCONNECTING, /**< Call is in disconnecting state */
    NET_RECONFIGURED,  /**< Interface is reconfigured, IP Address got changed */
    NET_NEWADDR,       /**< A new IP address was added on an existing call */
    NET_DELADDR,       /**< An IP address was removed from the existing interface */
};

/**
 * IP address information structure
 */
struct IpAddrInfo {
    std::string ifAddress;           /**< Interface IP address. */
    unsigned int ifMask = 0;         /**< Subnet mask.          */
    std::string gwAddress;           /**< Gateway IP address.   */
    unsigned int gwMask = 0;         /**< Subnet mask.          */
    std::string primaryDnsAddress;   /**< Primary DNS address.  */
    std::string secondaryDnsAddress; /**< Secondary DNS address.*/
};

/**
 * Bearer technology types (returned with getCurrentBearerTech).
 */
enum class DataBearerTechnology {
    UNKNOWN, /**< Unknown bearer. */
    // CDMA related data bearer technologies
    CDMA_1X,                /**< 1X technology. */
    EVDO_REV0,              /**< CDMA Rev 0. */
    EVDO_REVA,              /**< CDMA Rev A. */
    EVDO_REVB,              /**< CDMA Rev B. */
    EHRPD,                  /**< EHRPD. */
    FMC,                    /**< Fixed mobile convergence. */
    HRPD,                   /**< HRPD */
    BEARER_TECH_3GPP2_WLAN, /**< IWLAN */

    // UMTS related data bearer technologies
    WCDMA,                 /**< WCDMA. */
    GPRS,                  /**< GPRS. */
    HSDPA,                 /**< HSDPA. */
    HSUPA,                 /**< HSUPA. */
    EDGE,                  /**< EDGE. */
    LTE,                   /**< LTE. */
    HSDPA_PLUS,            /**< HSDPA+. */
    DC_HSDPA_PLUS,         /**< DC HSDPA+. */
    HSPA,                  /**< HSPA */
    BEARER_TECH_64_QAM,    /**< 64 QAM. */
    TDSCDMA,               /**< TD-SCDMA. */
    GSM,                   /**< GSM */
    BEARER_TECH_3GPP_WLAN, /**< IWLAN */
    BEARER_TECH_5G,        /**< 5G */
};

using EndReasonType = telux::common::EndReasonType;
using MobileIpReasonCode = telux::common::MobileIpReasonCode;
using InternalReasonCode = telux::common::InternalReasonCode;
using CallManagerReasonCode = telux::common::CallManagerReasonCode;
using SpecReasonCode = telux::common::SpecReasonCode;
using PPPReasonCode = telux::common::PPPReasonCode;
using EHRPDReasonCode = telux::common::EHRPDReasonCode;
using Ipv6ReasonCode = telux::common::Ipv6ReasonCode;
using HandoffReasonCode = telux::common::HandoffReasonCode;
using DataCallEndReason = telux::common::DataCallEndReason;


/**
 * Event due to which change in profile happened.
 */
enum class ProfileChangeEvent {
    CREATE_PROFILE_EVENT = 1, /**< Profile was created */
    DELETE_PROFILE_EVENT,     /**< Profile was deleted */
    MODIFY_PROFILE_EVENT,     /**< Profile was modified */
};

/**
 * This applies in architectures where the modem is attached to an External Application
 * Processor(EAP). An API, like start/stop data call, INatManager, IFirewallManager can be
 * invoked from the EAP or from the modems Internal Application Processor (IAP). This type
 * specifies where the operation should be carried out.
 */
enum class OperationType {
    DATA_LOCAL = 0, /**< Perform the operation on the processor where the API is invoked.*/
    DATA_REMOTE,    /**< Perform the operation on the application processor other than where
                           the API is invoked. */
};

/**
 * Direction of firewall rule
 */
enum class Direction {
    UPLINK = 1,   /**< Uplink Direction */
    DOWNLINK = 2, /**< Downlink Direction */
};

/**
 * Internet (IP) protocol numbers found in IPv4 or IPv6 headers
 * the protocol numbers are defined by Internet Assigned Numbers Authority (IANA)
 */
using IpProtocol = uint8_t;

/**
 * Traffic class number
 */
using TrafficClass = uint8_t;

/**
 * Default IP Protocol number in IPv4 or IPv6 headers.
 */
#define IP_PROT_UNKNOWN 0xFF

/**
 * Peripheral Interface type
 */
enum class InterfaceType {
    UNKNOWN = 0,    /**< UNKNOWN interface                                       */
    WLAN = 1,       /**< Wireless Local Area Network (WLAN)                      */
    ETH = 2,        /**< Ethernet (ETH)                                          */
    ECM = 3,        /**< Ethernet Control Model (ECM)                            */
    RNDIS = 4,      /**< Remote Network Driver Interface Specification (RNDIS)   */
    MHI = 5,        /**< Modem Host Interface (MHI)                              */
    VMTAP0 = 6,     /**< Represents Virtio interface available in a VM           */
    VMTAP1 = 7,     /**< Represents Virtio interface available in a VM           */
    ETH2 = 8,       /**< Ethernet network interface card (ETH NIC2)              */
    AP_PRIMARY = 9,       /**< Primary WLAN access point                         */
    AP_SECONDARY = 10,    /**< Secondary WLAN access point                       */
    AP_TERTIARY = 11,     /**< Tertiary WLAN access point                        */
    AP_QUATERNARY = 12,   /**< Quaternary WLAN access point                      */
};

/**
 * Specifies backhaul types
 */
enum class BackhaulType {
    ETH           = 0  ,    /** Ethernet Backhaul        */
    USB           = 1  ,    /** USB Backhaul             */
    WLAN          = 2  ,    /** WLAN Backhaul            */
    WWAN          = 3  ,    /** WWAN Backhaul with default profile ID set by */
                            /** @ref telux::data::DataConnectionManager::setDefaultProfile  */
    BLE           = 4  ,    /** Bluetooth Backhaul       */
    MAX_SUPPORTED = 5  ,    /** Max Supported Backhauls  */
};

/**
 * Encapsulate backhaul configuration parameters
 */
struct BackhaulInfo {
    BackhaulType backhaul;              /** Backhaul type to apply configuration on.          */
    SlotId slotId = DEFAULT_SLOT_ID;    /** Slot ID on which the profile ID is available.    */
                                        /** Needed only for WWAN backhaul                     */
    int profileId = -1;                 /** Profile ID to apply configuration on              */
                                        /** Needed only for WWAN backhaul                     */
    int vlanId = -1;                    /** Vlan ID should be provided only if vlan is treated as
                                            backhaul.
                                            e.g. if the backhaul is Vlan over Ethernet (ETH) with
                                            Vlan ID 4, Vlan ID should be set to 4 and backhaul type
                                            should be set to ETH */
};

enum class IpAssignType {
    UNKNOWN    = -1,    /** UNKNOW IP Type */
    STATIC_IP  = 0,     /** STATIC IP */
    DYNAMIC_IP = 1,     /** DYNAMIC IP */
};

/**
 * Specifies IP assign operation
 */
enum class IpAssignOperation {
    UNKNOWN     = -1,   /** UNKNOWN IP assign operation   */
    DISABLE     = 0,    /** DISABLE IP assignment     */
    ENABLE      = 1,    /** ENABLE IP assignment      */
    RECONFIGURE = 2,    /** RECONFIGURE IP assignment */
};

/**
 * Specifies IP configuration parameters
 */
struct IpConfigParams {
    InterfaceType ifType;                              /**< Interfaces (i.e. ETH, ECM and RNDIS) */
    IpFamilyType ipFamilyType = IpFamilyType::UNKNOWN; /**< Preferred IP family, default is IpFamilyType::UNKNOWN */
    uint32_t vlanId = -1;                              /**< Vlan ID should be provided only if vlan is treated as backhaul. e.g. if the backhaul is Vlan over Ethernet (ETH) with Vlan ID 4, Vlan ID should be set to 4 and interface type should be set to ETH */
};

/**
 * Specifies WAN config
 */
struct IpConfig {
    IpAssignType ipType;      /**< IP type assignment, STATIC_IP:  STATIC IP assignment DYNAMIC_IP: DYNAMIC IP assignment */
    IpAssignOperation ipOpr;  /**< IP assign operation, DISABLE: if @ref telux::data::DataCallStatus::NET_NO_NET ENABLE:  if @ref telux::data::DataCallStatus::NET_CONNECTED RECONFIG: if @ref telux::data::DataCallStatus::NET_RECONFIGURED */
    IpAddrInfo ipAddr;        /**< IP configuration, needed only for STATIC type IP */
};

/**
 * State of Service
 */
enum class ServiceState {
    INACTIVE = 0,   /**< Service is inactive */
    ACTIVE   = 1,   /**< Service is Active */
};

/**
 * Structure for vlan configuration
 */
struct VlanConfig {
    InterfaceType iface;       /**< PHY interfaces (i.e. ETH, ECM and RNDIS)                     */
    int16_t vlanId;            /**< Vlan identifier (i.e 1-4094)                                 */
    bool isAccelerated;        /**< is acceleration allowed                                      */
    uint8_t priority = 0;      /**< Vlan priority - A 3-bit field which refers to the IEEE 802.1p class of service to traffic priority level. Don't care = 0   */
    NetworkType nwType = NetworkType::LAN;        /**< Network type */
    bool createBridge = true;                     /**< TRUE:  create VLAN with bridge, FALSE: create VLAN without bridge */
};

/**
 * QOS flow state change type
 */
enum class QosFlowStateChangeEvent {
    UNKNOWN = -1,  /**< UNKNOWN state */
    ACTIVATED = 0, /**< Flow activated */
    MODIFIED = 1,  /**< Flow modified */
    DELETED = 2,   /**< Flow deleted */
};

/**
 * QOS Flow identifier
 */
using QosFlowId = uint32_t;

/**
 * QOS flow IP traffic class type
 */
enum class IpTrafficClassType {
    UNKNOWN = -1,       /**< UNKNOWN type */
    CONVERSATIONAL = 0, /**< Conversational IP Traffic class */
    STREAMING = 1,      /**< Streaming IP Traffic class */
    INTERACTIVE = 2,    /**< Interactive IP Traffic class */
    BACKGROUND = 3,     /**< Background IP Traffic class */
};

/**
 * QOS Flow data min max rate bits per seconds
 */
struct FlowDataRate {
    uint64_t maxRate;      /**< QOS Flow maximum data rate */
    uint64_t minRate;      /**< QOS Flow minimum data rate */
};

/**
 * Specifies QOS IP Flow parameter mask
 */
enum QosIPFlowMaskType {
    MASK_IP_FLOW_NONE = 0,                        /**< No parameters set  */
    MASK_IP_FLOW_TRF_CLASS = 1,                   /**< Traffic class */
    MASK_IP_FLOW_DATA_RATE_MIN_MAX = 2,           /**< Data rate min/max */
};

/**
 * 16 bit mask that denotes which of the flow paramaters defined in
 * QosIPFlowMaskType enum are used for TFT @QosIPFlowInfo.
 */
using QosIPFlowMask = std::bitset<16>;

/**
 * QOS Flow IP info
 */
struct QosIPFlowInfo {
    QosIPFlowMask mask;                     /**< Valid parameters of QosIPFlowInfo @ref QosIPFlowMaskType */
    IpTrafficClassType tfClass;             /**< IP Traffic class type @ref IpTrafficClassType */
    FlowDataRate dataRate;                  /**< Flow data rate @ref FlowDataRate */
};

/**
 * Specifies QOS Flow parameter mask
 */
enum QosFlowMaskType {
    MASK_FLOW_NONE = 0,           /**< No parameters set  */
    MASK_FLOW_TX_GRANTED = 1,     /**< TX Granted flow set */
    MASK_FLOW_RX_GRANTED = 2,     /**< RX Granted flow set */
    MASK_FLOW_TX_FILTERS = 3,     /**< TX filters set */
    MASK_FLOW_RX_FILTERS = 4,     /**< RX filters set */
};

/**
 * 16 bit mask that denotes which of the flow paramaters defined in
 * QosFlowMaskType enum are used for TFT @TrafficFlowTemplate.
 */
using QosFlowMask = std::bitset<16>;

/**
 * Possible DDS switch types.
 */

enum class DdsType {
    PERMANENT = 0, /**< Permanently switch the DDS SIM slot. Persistent across reboots; used to stop data on current DDS SIM and start on the other in DSDS mode. */
    TEMPORARY = 1  /**< Temporarily switch the DDS SIM slot. Not persistent across reboots; used for temporary data activity on non-DDS SIM during a voice call. */
};

/**
 * Specifies the DDS switch information.
 */
struct DdsInfo {
    DdsType type;   /**< Specifies DDS switch type */
    SlotId slotId;  /**< Specifies which slot is the DDS */
};

/** @} */ /* end_addtogroup telematics_data */
}
}

#endif // TELUX_DATA_DATADEFINES_HPP
