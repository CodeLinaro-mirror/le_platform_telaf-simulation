/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file       WlanFactory.hpp
 *
 * @brief      WlanFactory is the central factory to create all wlan managers instances such as
 *             WlanDeviceManager, WlanApInterfaceManager, and WlanStaInterfaceManager
 *
 */

#ifndef TELUX_WLAN_WLANFACTORY_HPP
#define TELUX_WLAN_WLANFACTORY_HPP

#include <map>
#include <memory>

#include <telux/common/CommonDefines.hpp>

#include <telux/wlan/WlanDeviceManager.hpp>
#include <telux/wlan/ApInterfaceManager.hpp>
#include <telux/wlan/StaInterfaceManager.hpp>

namespace telux {
namespace wlan {

/** @addtogroup telematics_wlan
 * @{ */

/**
 *@brief WlanFactory is the central factory to create all wlan classes
 *
 */
class WlanFactory {
 public:
    /**
     * Get Wlan Factory instance.
     */
    static WlanFactory &getInstance();

    /**
     * Get Wlan Device Manager
     *
     * @param [in] clientCallback       Optional callback to get the initialization status of
     *                                  WlanDeviceManager @ref telux::common::InitResponseCb
     * @returns instance of IWlanDeviceManager
     *
     */
    virtual std::shared_ptr<IWlanDeviceManager> getWlanDeviceManager(
       telux::common::InitResponseCb clientCallback = nullptr) = 0;

    /**
     * Get Access Point Interface Manager
     *
     * @returns instance of IApInterfaceManager
     *
     */
    virtual std::shared_ptr<IApInterfaceManager> getApInterfaceManager() = 0;

    /**
     * Get Station Interface Manager
     *
     * @returns instance of IStaInterfaceManager
     *
     */
    virtual std::shared_ptr<IStaInterfaceManager> getStaInterfaceManager() = 0;

 protected:
    WlanFactory();
    virtual ~WlanFactory();

 private:
    WlanFactory(const WlanFactory &) = delete;
    WlanFactory &operator=(const WlanFactory &) = delete;
};

/** @} */ /* end_addtogroup telematics_wlan */
}
}

#endif // TELUX_WLAN_WLANFACTORY_HPP
