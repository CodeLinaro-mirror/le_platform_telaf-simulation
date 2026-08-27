// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear
//
// PowerFactory.hpp - entry point implementing telux::power::PowerFactory.
//
// Mirrors component/data/DataFactory.hpp's getInstance()-out-of-line idiom
// (see PowerFactory.cpp). Covers the whole telux::power surface: one
// ITcuActivityManager per ClientType (a client may hold both a MASTER and a
// SLAVE manager, and they must stay distinct SDK clients) and a single
// IWakeupManager.

#ifndef TELUX_POWER_SIMULA_POWER_FACTORY_HPP
#define TELUX_POWER_SIMULA_POWER_FACTORY_HPP

#include "../common/IModemBridge.hpp"
#include "TcuActivityManager.hpp"
#include "WakeupManager.hpp"

#include <memory>
#include <mutex>
#include <telux/power/PowerFactory.hpp>

namespace telux::power::simula {

class SimulaPowerFactory final : public telux::power::PowerFactory
{
public:
    // No getInstance() override here -- telux::power::PowerFactory::
    // getInstance() (the real SDK's non-virtual static method) is defined
    // out-of-line in PowerFactory.cpp to return a SimulaPowerFactory
    // singleton, matching the ABI real client code links against.
    //
    // Ctor/dtor are public (not protected+friend) because getInstance()'s
    // function-local static lives in the enclosing telux::power namespace,
    // not as a member of this class -- there's no single friend
    // declaration that would reach it.
    SimulaPowerFactory();
    ~SimulaPowerFactory() override;

    SimulaPowerFactory(const SimulaPowerFactory&) = delete;
    SimulaPowerFactory& operator=(const SimulaPowerFactory&) = delete;

    std::shared_ptr<telux::power::ITcuActivityManager> getTcuActivityManager(
      telux::power::ClientInstanceConfig config,
      telux::common::InitResponseCb callback = nullptr
    ) override;

    std::shared_ptr<telux::power::IWakeupManager> getWakeupManager(
      telux::common::InitResponseCb callback = nullptr
    ) override;

    // Deprecated overload. Forwards to the ClientInstanceConfig form with
    // procType ignored (simula is always effectively LOCAL_PROC).
    std::shared_ptr<telux::power::ITcuActivityManager> getTcuActivityManager(
      telux::power::ClientType clientType = telux::power::ClientType::SLAVE,
      telux::common::ProcType procType = telux::common::ProcType::LOCAL_PROC,
      telux::common::InitResponseCb callback = nullptr
    ) override;

private:
    std::mutex mutex_;
    common::simula::IModemBridge& bridge_;
    std::shared_ptr<SimulaTcuActivityManager> tcu_activity_manager_master_;
    std::shared_ptr<SimulaTcuActivityManager> tcu_activity_manager_slave_;
    std::shared_ptr<SimulaWakeupManager> wakeup_manager_;
};

}  // namespace telux::power::simula

#endif  // TELUX_POWER_SIMULA_POWER_FACTORY_HPP
