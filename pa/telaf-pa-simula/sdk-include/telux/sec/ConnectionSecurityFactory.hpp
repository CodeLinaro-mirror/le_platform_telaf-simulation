/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file  ConnectionSecurityFactory.hpp
 * @brief ConnectionSecurityFactory allows creation of managers dealing with
 *        connection security.
 */

#ifndef TELUX_SEC_CONNECTIONSECURITYFACTORY_HPP
#define TELUX_SEC_CONNECTIONSECURITYFACTORY_HPP

#include <telux/sec/CellularSecurityManager.hpp>
#include <telux/sec/WiFiSecurityManager.hpp>

namespace telux {
namespace sec {

/** @addtogroup telematics_sec_mgmt
 * @{ */

/**
 * @brief ConnectionConnectionSecurityFactory allows creation of CellularSecurityManager
 * and WiFiSecurityManager.
 */
class ConnectionSecurityFactory {

 public:

    /**
     * Gets the ConnectionSecurityFactory instance.
     */
    static ConnectionSecurityFactory &getInstance();

    /**
     * Provides an ICellularSecurityManager instance that detects and monitors
     * security threats and generates security scan reports for cellular connections.
     *
     * @param[out] ec telux::common::ErrorCode::SUCCESS if ICellularSecurityManager
     *                is created successfully, otherwise, an appropriate error code
     *
     * @returns ICellularSecurityManager instance or nullptr, if an error occurred
     *
     */
    virtual std::shared_ptr<ICellularSecurityManager> getCellularSecurityManager(
        telux::common::ErrorCode &ec) = 0;

    /**
     * Provides an IWiFiSecurityManager instance that detects and monitors
     * security threats and generates security analysis reports for WiFi connections.
     *
     * @param [in] callback Callback to receive the WiFiSecurityManager initialization status.
     *
     * @returns IWiFiSecurityManager instance or nullptr, if an error occurred
     *
     * @note Eval: This is a new API and is being evaluated. It is subject
     *             to change and could break backwards compatibility.
     */
    virtual std::shared_ptr<IWiFiSecurityManager> getWiFiSecurityManager(
        telux::common::InitResponseCb callback) = 0;

    /**
     * Provides an IWiFiSecurityManager instance that detects and monitors
     * security threats and generates security analysis reports for Wi-Fi connections.
     *
     * @param[out] ec telux::common::ErrorCode::SUCCESS if IWiFiSecurityManager
     *                is created successfully, otherwise, an appropriate error code
     *
     * @returns IWiFiSecurityManager instance or nullptr, if an error occurred
     *
     * @deprected use the getWiFiSecurityManager(telux::common::InitResponseCb callback) API instead
     */
    virtual std::shared_ptr<IWiFiSecurityManager> getWiFiSecurityManager(
        telux::common::ErrorCode &ec) = 0;

#ifndef TELUX_DOXY_SKIP
 protected:
    ConnectionSecurityFactory();
    virtual ~ConnectionSecurityFactory();
#endif

 private:
    ConnectionSecurityFactory(const ConnectionSecurityFactory &) = delete;
    ConnectionSecurityFactory &operator=(const ConnectionSecurityFactory &) = delete;
};

/** @} */ /* end_addtogroup telematics_sec_mgmt */

}  // End of namespace sec
}  // End of namespace telux

#endif // TELUX_SEC_CONNECTIONSECURITYFACTORY_HPP
