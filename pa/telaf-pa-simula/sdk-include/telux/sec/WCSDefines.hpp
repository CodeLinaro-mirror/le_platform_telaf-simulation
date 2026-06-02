/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file  WCSDefines.hpp
 * @brief Defines various enumerations and data types used with telsdk
 *        security APIs.
 */

#ifndef TELUX_SEC_WCSDEFINES_HPP
#define TELUX_SEC_WCSDEFINES_HPP

#include <cstdint>
#include <cstring>
#include <vector>
#include <string>

#include <telux/common/CommonDefines.hpp>

namespace telux {
namespace sec {

/** @addtogroup telematics_sec_mgmt
 * @{ */

/**
 *  Security analysis result for a given access point (AP).
 */
enum class AnalysisResult {

    /**
     *  There was no result for this AP because either the device is moving
     *  or the AP is on the fringes of signal strength.
     */
    NO_RESULT,

    /**
     *  This is the first time this AP is used for a connection and no previous
     *  references exist.
     */
    NEW_ASSOCIATION,

    /**
     *  The AP appears safe.
     */
    NO_THREAT_DETECTED,

    /**
     *  The AP is not safe.
     */
    MALICIOUS
};

/**
 *  Machine learning algorithm threat analysis result per AP.
 */
struct MLAlgorithmAnalysis {

    /**
     *  Higher threat scores indicate a higher possibility that
     *  the AP is malicious; range is 0 to 100.
     */
    uint32_t threatScore;

    /**
     *  Result of the security analysis for a given AP.
     */
    AnalysisResult result;
};

/**
 *  Summoning attack threat analysis result.
 */
struct SummoningAnalysis {

    /**
     *  Result of the security analysis for a given AP.
     */
    AnalysisResult result;
};

/**
 *  Represents the security report for a Wi-Fi AP.
 */
struct WiFiSecurityReport {

    /**
     *  Network interface name of the AP.
     */
    std::string ssid;

    /**
     *  MAC address of the AP.
     */
    std::string bssid;

    /**
     *  True if the device is connected to this AP.
     */
    bool isConnectedToAP;

    /**
     *  True if devices can connect to this AP without authentication.
     */
    bool isOpenAP;

    /**
     *  Machine learning algorithm threat analysis result.
     */
    MLAlgorithmAnalysis mlAlgorithmAnalysis;

    /**
     *  Summoning attack threat analysis result.
     */
    SummoningAnalysis summoningAnalysis;
};

/**
 *  Represents information about a deauthentication attack.
 */
struct DeauthenticationInfo {

    /**
     *  Reason code why disassociation or deauthentication occurred
     *  as specified by the IEEE 802.11 standard.
     */
    int deauthenticationReason;

    /**
     *  True if the AP initiated the disconnection.
     */
    bool didAPInitiateDisconnect;

    /**
     *  Higher threat scores indicate a higher possibility that this
     *  is a deauthentication attack; range is 0 to 100.
     */
    uint32_t threatScore;
};

/**
 *  Represents a WiFi access point.
 */
struct ApInfo {
    /**
     *  Network interface name of the AP.
     */
    std::string ssid;

    /**
     *  MAC address of the AP.
     */
    std::string bssid;
};

/** @} */ /* end_addtogroup telematics_sec_mgmt */

}  // End of namespace sec
}  // End of namespace telux

#endif // TELUX_SEC_WCSDEFINES_HPP
