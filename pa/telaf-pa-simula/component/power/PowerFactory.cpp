// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "PowerFactory.hpp"

#include "../common/ListenerDispatchAO.hpp"
#include "../common/ModemBridge.hpp"
#include "TcuActivityManager.hpp"
#include "WakeupManager.hpp"

namespace telux::power::simula {

SimulaPowerFactory::SimulaPowerFactory()
    : bridge_(common::simula::ModemBridge::instance())
{
    bridge_.start();
    // Same rationale as SimulaDataFactory's ctor (DataFactory.cpp:11-22):
    // every manager delivers async listener callbacks by enqueueing onto
    // this shared worker.
    common::simula::ListenerDispatchAO::instance().start();
}

SimulaPowerFactory::~SimulaPowerFactory() = default;

std::shared_ptr<telux::power::ITcuActivityManager>
SimulaPowerFactory::getTcuActivityManager(
  telux::power::ClientInstanceConfig config,
  telux::common::InitResponseCb callback
)
{
    std::lock_guard<std::mutex> lk(mutex_);
    // One instance per role, not one per process. A client may legitimately
    // hold both a MASTER and a SLAVE manager at once (tafPmsPa does exactly
    // that), and the two are distinct SDK clients: the master issues
    // setActivityState and hears onSlaveAckStatusUpdate, the slave hears
    // onTcuActivityStateUpdate and answers sendActivityStateAck. Caching a
    // single instance across both roles merged their listener sets, so every
    // indication reached each registered listener twice.
    auto& slot = config.clientType == telux::power::ClientType::MASTER
                   ? tcu_activity_manager_master_
                   : tcu_activity_manager_slave_;
    if (slot)
    {
        if (callback &&
            slot->getServiceStatus() == telux::common::ServiceStatus::SERVICE_AVAILABLE)
            callback(telux::common::ServiceStatus::SERVICE_AVAILABLE);
        return slot;
    }
    slot = std::make_shared<SimulaTcuActivityManager>(bridge_, config, std::move(callback));
    slot->start();
    return slot;
}

std::shared_ptr<telux::power::IWakeupManager>
SimulaPowerFactory::getWakeupManager(telux::common::InitResponseCb callback)
{
    std::lock_guard<std::mutex> lk(mutex_);
    if (wakeup_manager_)
    {
        if (callback &&
            wakeup_manager_->getServiceStatus() == telux::common::ServiceStatus::SERVICE_AVAILABLE)
            callback(telux::common::ServiceStatus::SERVICE_AVAILABLE);
        return wakeup_manager_;
    }

    wakeup_manager_ = std::make_shared<SimulaWakeupManager>(bridge_, std::move(callback));
    wakeup_manager_->start();
    return wakeup_manager_;
}

std::shared_ptr<telux::power::ITcuActivityManager>
SimulaPowerFactory::getTcuActivityManager(
  telux::power::ClientType clientType,
  telux::common::ProcType /*procType*/,
  telux::common::InitResponseCb callback
)
{
    telux::power::ClientInstanceConfig config;
    config.clientType = clientType;
    return getTcuActivityManager(config, std::move(callback));
}

}  // namespace telux::power::simula

// =============================================================================
// telux::power::PowerFactory::getInstance()
//
// Mirrors telux::data::DataFactory::getInstance() (DataFactory.cpp:217-224):
// must live in the telux::power namespace to satisfy the ABI real client
// code links against.
// =============================================================================

namespace telux::power {

PowerFactory&
PowerFactory::getInstance()
{
    static simula::SimulaPowerFactory instance;
    return instance;
}

PowerFactory::PowerFactory() = default;
PowerFactory::~PowerFactory() = default;

}  // namespace telux::power
