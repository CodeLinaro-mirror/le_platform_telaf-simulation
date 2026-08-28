/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "SubsystemManager.hpp"
#include "Log.hpp"

#include <algorithm>

namespace telux {
namespace platform {

using namespace telux::common;

class SimulaSubsystemFactory : public SubsystemFactory
{
public:
    static SubsystemFactory& getInstance()
    {
        LOG_DEBUG("[SimulaSubsystemFactory] getInstance() called");
        static SimulaSubsystemFactory instance;
        return instance;
    }

    std::shared_ptr<ISubsystemManager> getSubsystemManager(InitResponseCb initCallback = nullptr) override
    {
        LOG_INFO("[SimulaSubsystemFactory] getSubsystemManager() called");
        auto mgr = std::make_shared<SimulaSubsystemManager>(initCallback);
        LOG_DEBUG(
          "[SimulaSubsystemFactory] SimulaSubsystemManager instance created: %p",
          static_cast<void*>(mgr.get())
        );
        return mgr;
    }

private:
    SimulaSubsystemFactory() = default;
    ~SimulaSubsystemFactory() override = default;
};

SubsystemFactory&
SubsystemFactory::getInstance()
{
    LOG_DEBUG("[SubsystemFactory] getInstance() called - returning SimulaSubsystemFactory");
    return SimulaSubsystemFactory::getInstance();
}

SubsystemFactory::SubsystemFactory() = default;
SubsystemFactory::~SubsystemFactory() = default;

// ---------------------------------------------------------------------------
// SimulaSubsystemManager

SimulaSubsystemManager::SimulaSubsystemManager(InitResponseCb cb)
    : serviceStatus_(ServiceStatus::SERVICE_AVAILABLE)
{
    LOG_INFO("[SimulaSubsystemManager] Constructor called");
    if (cb)
    {
        LOG_DEBUG("[SimulaSubsystemManager] Invoking init callback with SERVICE_AVAILABLE");
        cb(ServiceStatus::SERVICE_AVAILABLE);
    }
}

SimulaSubsystemManager::~SimulaSubsystemManager()
{
    LOG_DEBUG("[SimulaSubsystemManager] Destructor called");
}

ServiceStatus
SimulaSubsystemManager::getServiceStatus()
{
    LOG_DEBUG("[SimulaSubsystemManager] getServiceStatus() = %d", static_cast<int>(serviceStatus_));
    return serviceStatus_;
}

ErrorCode
SimulaSubsystemManager::registerListener(
  std::weak_ptr<ISubsystemListener> listener, std::vector<SubsystemInfo>
)
{
    LOG_DEBUG("[SimulaSubsystemManager] registerListener() called");
    std::lock_guard<std::mutex> lk(listeners_mutex_);
    listeners_.push_back(std::move(listener));
    return ErrorCode::SUCCESS;
}

ErrorCode
SimulaSubsystemManager::deRegisterListener(std::weak_ptr<ISubsystemListener> listener)
{
    LOG_DEBUG("[SimulaSubsystemManager] deRegisterListener() called");
    std::lock_guard<std::mutex> lk(listeners_mutex_);
    auto target = listener.lock();
    listeners_.erase(
      std::remove_if(
        listeners_.begin(), listeners_.end(),
        [&](const std::weak_ptr<ISubsystemListener>& w) {
            auto sp = w.lock();
            return !sp || (target && sp == target);
        }
      ),
      listeners_.end()
    );
    return ErrorCode::SUCCESS;
}

}  // namespace platform
}  // namespace telux
