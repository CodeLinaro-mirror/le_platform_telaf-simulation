/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file  WiFiSecurityManager.hpp
 * @brief WiFiSecurityManager provides support for detecting, monitoring and
 *        generating security analysis reports for WiFi connections.
 */

#ifndef TELUX_SEC_WIFISECURITYMANAGER_HPP
#define TELUX_SEC_WIFISECURITYMANAGER_HPP

#include <cstdint>
#include <memory>
#include <vector>
#include <string>

#include <telux/sec/WCSListener.hpp>

namespace telux {
namespace sec {

/**
 * Provides support for detecting, monitoring, and generating security reports for
 * Wi-Fi APs.
 */
class IWiFiSecurityManager {

 public:

    /**
     * Gets the security service status.
     *
     * @returns @ref telux::common::ServiceStatus::SERVICE_AVAILABLE if the security
     *          service is ready for use,
     *          @ref telux::common::ServiceStatus::SERVICE_UNAVAILABLE if the security
     *          service is temporarily unavailable (possibly undergoing initialization),
     *          @ref telux::common::ServiceStatus::SERVICE_FAILED if the security
     *          service needs re-initialization
     */
   virtual telux::common::ServiceStatus getServiceStatus() = 0;

   /**
    * Registers the given listener to receive Wi-Fi connection security reports. These
    * reports will be received by @ref IWiFiReportListener::onReportAvailable().
    *
    * On platforms with access control enabled, the caller needs to have the TELUX_SEC_WCS_REPORT
    * permission to successfully invoke this API.
    *
    * @param [in] reportListener Receives security reports.
    *
    * @returns @ref telux::common::ErrorCode::SUCCESS, if the listener is registered,
    *          otherwise, an appropriate error code.
    *
    * @note Eval: This is a new API and is being evaluated. It is subject to change and
    *             could break backwards compatibility.
    */
   virtual telux::common::ErrorCode registerListener(
      std::weak_ptr<IWiFiReportListener> reportListener) = 0;

   /**
    * Unregisters the given listener registered previously with @ref registerListener().
    *
    * On platforms with access control enabled, the caller needs to have the TELUX_SEC_WCS_REPORT
    * permission to successfully invoke this API.
    *
    * @param [in] reportListener Listener to unregister.
    *
    * @returns @ref telux::common::ErrorCode::SUCCESS, if the listener is deregistered,
    *          otherwise, an appropriate error code.
    *
    * @note Eval: This is a new API and is being evaluated. It is subject to change and
    *             could break backwards compatibility.
    */
   virtual telux::common::ErrorCode deregisterListener(
      std::weak_ptr<IWiFiReportListener> reportListener) = 0;

    /**
     * Registers the given listener to get notified when the security service status changes.
     * The @ref IServiceStatusListener::onServiceStatusChange() method receives the new status.
     *
     * @param [in] listener Invoked to pass the new service status
     *
     * @returns @ref telux::common::ErrorCode::SUCCESS if the listener is registered,
     *          otherwise, an appropriate error code
     */
    virtual telux::common::ErrorCode registerListener(
      std::weak_ptr<telux::common::IServiceStatusListener> listener) = 0;

    /**
     * Unregisters the given, previously registered listener.
     *
     * @param [in] listener Listener to unregister
     *
     * @returns @ref telux::common::ErrorCode::SUCCESS if the listener is deregistered,
     *          otherwise, an appropriate error code
     */
    virtual telux::common::ErrorCode deregisterListener(
      std::weak_ptr<telux::common::IServiceStatusListener> listener) = 0;

   /**
    * Lists all the trusted APs.
    *
    * On platforms with access control enabled, the caller needs to have the TELUX_SEC_WCS_INFO
    * permission to successfully invoke this API.
    *
    * @param [out] trustedAPList List of trusted APs ( @ref ApInfo ).
    *
    * @returns @ref telux::common::ErrorCode::SUCCESS, if the list is retrived otherwise,
    *          an appropriate error code.
    *
    * @note Eval: This is a new API and is being evaluated. It is subject to change and
    *             could break backwards compatibility.
    */
   virtual telux::common::ErrorCode getTrustedApList(std::vector<ApInfo>& trustedAPList) = 0;

   /**
    * Removes the given AP from the saved list of trusted APs. If the device connects to the same
    * AP again, @ref IWiFiReportListener::isTrustedAP() will be invoked again.
    *
    * On platforms with access control enabled, the caller needs to have the TELUX_SEC_WCS_CONFIG
    * permission to successfully invoke this API.
    *
    * @param [in] apInfo AP to distrust ( @ref ApInfo ).
    *
    * @returns @ref telux::common::ErrorCode::SUCCESS, if the AP is distrusted otherwise,
    *          an appropriate error code.
    *
    * @note Eval: This is a new API and is being evaluated. It is subject to change and
    *             could break backwards compatibility.
    */
   virtual telux::common::ErrorCode removeApFromTrustedList(ApInfo apInfo) = 0;

   /**
    * IWiFiSecurityManager destructor; cleans up as applicable.
    */
   virtual ~IWiFiSecurityManager() {};
};

/** @} */  // end_addtogroup telematics_sec_mgmt

}  // End of namespace sec
}  // End of namespace telux

#endif // TELUX_SEC_WIFISECURITYMANAGER_HPP
