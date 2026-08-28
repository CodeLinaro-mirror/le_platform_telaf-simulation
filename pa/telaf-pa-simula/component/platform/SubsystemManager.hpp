/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef SUBSYSTEM_MANAGER_HPP
#define SUBSYSTEM_MANAGER_HPP

#include <memory>
#include <mutex>
#include <vector>

#include "telux/platform/SubsystemManager.hpp"
#include "telux/platform/SubsystemFactory.hpp"

namespace telux {
namespace platform {

using namespace telux::common;

class SimulaSubsystemManager : public ISubsystemManager {
public:
    explicit SimulaSubsystemManager(InitResponseCb cb);

    ~SimulaSubsystemManager() override;

    ServiceStatus getServiceStatus() override;

    ErrorCode registerListener(
        std::weak_ptr<ISubsystemListener> listener,
        std::vector<SubsystemInfo> subsystems) override;

    ErrorCode deRegisterListener(std::weak_ptr<ISubsystemListener> listener) override;

private:
    ServiceStatus serviceStatus_;
    std::mutex listeners_mutex_;
    std::vector<std::weak_ptr<ISubsystemListener>> listeners_;
};

}  // namespace platform
}  // namespace telux

#endif  // SUBSYSTEM_MANAGER_HPP
