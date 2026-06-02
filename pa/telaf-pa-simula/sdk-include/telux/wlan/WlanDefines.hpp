/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file       WlanDefines.hpp
 * @brief      WlanDefines contains enumerations and variables used for wlan services
 *
 */

#ifndef TELUX_WLAN_WLANDEFINES_HPP
#define TELUX_WLAN_WLANDEFINES_HPP

#include <string>
#include <vector>
#include <bitset>

namespace telux {
namespace wlan {

/** @addtogroup telematics_wlan
 * @{ */

/**
 * Radio Band Types:
 */
enum class BandType {
    BAND_5GHZ   = 1,
    BAND_2GHZ   = 2,
    BAND_6GHZ   = 3,
};

/**
 * Connection Status
 */
enum class ConnectionStatus {
    UNKNOWN              = 0,           /**< Device connection is unknown    */
    CONNECTED            = 1,           /**< Device is connected             */
    DISCONNECTED         = 2,           /**< Device is disconnected          */
};

/**
 * Identifiers for Ap, Sta, P2p
 */
enum class Id {
    PRIMARY     = 1,
    SECONDARY   = 2,
    TERTIARY    = 3,
    QUATERNARY  = 4,
};

/**
 * AP Types:
 */
enum class ApType {
    UNKNOWN      = 0,
    PRIVATE      = 1,
    GUEST        = 2,
};

/**
 * Station Interface Status
 */
enum class StaInterfaceStatus {
    UNKNOWN              = 0,           /**< Station interface is unknown                  */
    CONNECTING           = 1,           /**< Station interface is connecting               */
    CONNECTED            = 2,           /**< Station interface is connected                */
    DISCONNECTED         = 3,           /**< Station interface is disconnected             */
    ASSOCIATION_FAILED   = 4,           /**< Station is unable to associate with AP        */
    IP_ASSIGNMENT_FAILED = 5,           /**< Station in unable to get IP address via DHCP  */
};

/**
 * Station Connection Status
 */
enum class StaConnectionStatus {
    UNKNOWN              = 0,           /**< Station connection status is unknown              */
    SUCCESS              = 1,           /**< Station connection attempt was successful         */
    INCORRECT_PSK        = 2,           /**< Station connection attempt failed with incorrect
                                             password/passkey */
    AP_NOT_FOUND         = 3,           /**< Station connection attempt failed with AP not in
                                             range */
};

/**
 * AP Info - captures ap type (private/guest)
 */
struct ApInfo {
    BandType        apRadio;            /**< Radio type (2.4/5.0/6.0 GHz) */
    ApType          apType;             /**< Ap type (private/guest) */
};

/**
 * Ap Network Info
 */
struct ApNetInfo {
    ApInfo          info;               /**< Ap information (AP type)              */
    std::string     ssid;               /**< SSID associated with this network     */
};

/**
 * AP Status for enabled Networks
 */
struct ApStatus {
    Id              id;              /**< AP id                                 */
    std::string     name;            /**< AP network interface name             */
    std::string     ipv4Address;     /**< Local AP IP V4 address                */
    std::string     macAddress;      /**< AP MAC address                        */
    std::vector<ApNetInfo> network;  /**< Settings for AP info                  */
};

/**
 * Station Status
 */
struct StaStatus {
    Id                  id;               /**< Station Id                       */
    std::string         name;             /**< Network interface name           */
    std::string         ipv4Address;      /**< Public IP V4 address             */
    std::string         ipv6Address;      /**< Public IP V6 address             */
    std::string         macAddress;       /**< MAC address                      */
    StaInterfaceStatus  status;           /**< Interface status                 */
    StaConnectionStatus connectionStatus; /**< Station connection status        */
};

/**
 * This applies in architectures where the modem is attached to an External Application
 * Processor(EAP). An API that sets or configure Wlan can be invoked from the EAP or from
 * the modems Internal Application Processor (IAP). This type  specifies where the operation
 * should be carried out.
 */
enum class OperationType {
    WLAN_LOCAL = 0, /**< Perform the operation on the processor where the API is invoked.*/
    WLAN_REMOTE,    /**< Perform the operation on the application processor other than where
                           the API is invoked. */
};

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
 * Service operations to be performed
 */
enum class ServiceOperation {
    STOP      = 0x00,      /**<  Stop service       */
    START     = 0x01,      /**<  Start service      */
    RESTART   = 0x02,      /**<  Restart service    */
};

/** @} */ /* end_addtogroup telematics_wlan */
}
}

#endif // TELUX_WLAN_WLANDEFINES_HPP
