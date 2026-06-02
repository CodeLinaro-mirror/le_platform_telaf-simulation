/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file  WCSListener.hpp
 * @brief Defines the listener classes and methods to receive asynchronous events.
 */

#ifndef TELUX_SEC_WCSLISTENER_HPP
#define TELUX_SEC_WCSLISTENER_HPP

#include <telux/common/SDKListener.hpp>
#include <telux/sec/WCSDefines.hpp>

namespace telux {
namespace sec {

/** @addtogroup telematics_sec_mgmt
 * @{ */

/**
 * Receives security analysis reports for the Wi-Fi APs detected while
 * scanning for APs in the vicinity and provides a listener for deauthentication
 * attacks.
 * It is recommended that the client should not perform any blocking/sleeping operation
 * from within methods in this class to ensure all the information is provided for attack scans.
 * Also the implementation should be thread safe.
 */
class IWiFiReportListener : public telux::common::ISDKListener {

 public:
    /**
     * Notifies that the implementation completed a threat analysis and that the report is available.
     * This analysis is performed at various triggers. For example, when a scan for APs is triggered
     * the implementation performs an analysis and provides a report for every AP it sees in the
     * vicinity.
     *
     * @param[in] report @ref WiFiSecurityReport result of the Wi-Fi security analysis.
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to change and
     *             could break backwards compatibility.
     */
    virtual void onReportAvailable(WiFiSecurityReport report) { }

    /**
     * Notifies that a deauthentication attack is identified.
     *
     * @param[in] deauthenticationInfo @ref DeauthenticationInfo security analysis information.
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to change and
     *             could break backwards compatibility.
     */
    virtual void onDeauthenticationAttack(DeauthenticationInfo deauthenticationInfo) { }

    /**
     * Gets user's confirmation that the given AP is trusted. This is called only once
     * when the device connects to this AP for the first time. If the application
     * trusts the given AP, it should set 'isTrusted' to true, otherwise it should be set to false.
     * This information is critical for attack scans and without the user's input security analysis
     * reports will be blocked.
     *
     * Once the user confirms that an AP is trusted, this information is saved internally
     * and used later to detect threats like evil twin attacks.
     *
     * On platforms with access control enabled, the caller needs to have the TELUX_SEC_WCS_CONFIG
     * permission to successfully invoke this API.
     *
     * @param[in] accessPoint @ref ApInfo provides information about an AP.
     *
     * @param[out] isTrusted True if trusted; false otherwise.
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to change and
     *             could break backwards compatibility.
     */
    virtual void isTrustedAP(ApInfo accessPoint, bool& isTrusted) { }

    /**
     * IWiFiReportListener destructor.
     */
    virtual ~IWiFiReportListener() { }
};

/** @} */ /* end_addtogroup telematics_sec_mgmt */

}  // End of namespace sec
}  // End of namespace telux

#endif // TELUX_SEC_WCSLISTENER_HPP
