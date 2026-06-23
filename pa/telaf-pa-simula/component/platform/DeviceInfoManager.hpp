/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef DEVICE_INFO_MANAGER_HPP
#define DEVICE_INFO_MANAGER_HPP

#include <memory>

#include "telux/platform/DeviceInfoManager.hpp"
#include "telux/platform/DeviceInfoListener.hpp"

namespace telux {
namespace platform {

using namespace telux::common;

class DeviceInfoManager : public IDeviceInfoManager {
public:
    DeviceInfoManager(InitResponseCb cb);

    ~DeviceInfoManager() override;

    ServiceStatus getServiceStatus() override;

    Status registerListener(
        std::weak_ptr<IDeviceInfoListener> listener) override;

    Status deregisterListener(
        std::weak_ptr<IDeviceInfoListener> listener) override;

    Status getPlatformVersion(
        PlatformVersion &pv) override;

    Status getIMEI(
        std::string &imei) override;

private:
    ServiceStatus serviceStatus_;
    InitResponseCb callback_;
};

}  // namespace platform
}  // namespace telux

#endif  // DEVICE_INFO_MANAGER_HPP
