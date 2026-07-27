// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear
// Factory for the simulated card, subscription, and multi-SIM managers.

#ifndef TELUX_TEL_SIMULA_SIM_FACTORY_HPP
#define TELUX_TEL_SIMULA_SIM_FACTORY_HPP

#include "../common/IModemBridge.hpp"
#include "CardManager.hpp"
#include "MultiSimManager.hpp"
#include "SubscriptionManager.hpp"

#include <map>
#include <memory>
#include <mutex>
#include <telux/tel/PhoneFactory.hpp>

namespace telux::tel::simula {

class SimulaSimFactory
{
public:
    SimulaSimFactory();
    ~SimulaSimFactory();

    SimulaSimFactory(const SimulaSimFactory&) = delete;
    SimulaSimFactory& operator=(const SimulaSimFactory&) = delete;

    std::shared_ptr<telux::tel::ICardManager> getCardManager(telux::common::InitResponseCb clientCallback);
    std::shared_ptr<telux::tel::ISubscriptionManager> getSubscriptionManager(
      telux::common::InitResponseCb clientCallback
    );
    std::shared_ptr<telux::tel::IMultiSimManager> getMultiSimManager(
      telux::common::InitResponseCb clientCallback
    );

private:
    std::mutex mutex_;
    common::simula::IModemBridge& bridge_;
    // Managers are keyed by the single simulated slot.
    std::map<int, std::shared_ptr<SimulaCardManager>> card_managers_;
    std::map<int, std::shared_ptr<SimulaSubscriptionManager>> subscription_managers_;
    std::map<int, std::shared_ptr<SimulaMultiSimManager>> multi_sim_managers_;
};

}  // namespace telux::tel::simula

#endif  // TELUX_TEL_SIMULA_SIM_FACTORY_HPP
