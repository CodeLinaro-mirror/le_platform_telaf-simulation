// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "SimFactory.hpp"

#include "../common/ListenerDispatchAO.hpp"
#include "../common/ModemBridge.hpp"
#include "CardManager.hpp"
#include "MultiSimManager.hpp"

namespace telux::tel::simula {

SimulaSimFactory::SimulaSimFactory()
    : bridge_(common::simula::ModemBridge::instance())
{
    bridge_.start();
    // Listener callbacks are dispatched by this shared active object.
    common::simula::ListenerDispatchAO::instance().start();
}

SimulaSimFactory::~SimulaSimFactory() = default;

std::shared_ptr<telux::tel::ICardManager>
SimulaSimFactory::getCardManager(telux::common::InitResponseCb clientCallback)
{
    std::lock_guard<std::mutex> lk(mutex_);
    auto it = card_managers_.find(DEFAULT_SLOT_ID);
    if (it != card_managers_.end())
    {
        it->second->addInitCallback(std::move(clientCallback));
        return it->second;
    }
    auto mgr = std::make_shared<SimulaCardManager>(DEFAULT_SLOT_ID, bridge_, std::move(clientCallback));
    card_managers_.emplace(DEFAULT_SLOT_ID, mgr);
    // SimFactory wires card presence changes to the multi-SIM cache refresh.
    auto it_ms = multi_sim_managers_.find(DEFAULT_SLOT_ID);
    if (it_ms != multi_sim_managers_.end())
    {
        auto ms = it_ms->second;
        mgr->setPresenceChangedCb(
          [ms](bool present) { ms->notifySlotStatusChanged(present); }
        );
    }
    mgr->start();
    return mgr;
}

std::shared_ptr<telux::tel::ISubscriptionManager>
SimulaSimFactory::getSubscriptionManager(telux::common::InitResponseCb clientCallback)
{
    std::lock_guard<std::mutex> lk(mutex_);
    auto it = subscription_managers_.find(DEFAULT_SLOT_ID);
    if (it != subscription_managers_.end())
    {
        it->second->addInitCallback(std::move(clientCallback));
        return it->second;
    }
    auto mgr =
      std::make_shared<SimulaSubscriptionManager>(DEFAULT_SLOT_ID, bridge_, std::move(clientCallback));
    subscription_managers_.emplace(DEFAULT_SLOT_ID, mgr);
    mgr->start();
    return mgr;
}

std::shared_ptr<telux::tel::IMultiSimManager>
SimulaSimFactory::getMultiSimManager(telux::common::InitResponseCb clientCallback)
{
    std::lock_guard<std::mutex> lk(mutex_);
    auto it = multi_sim_managers_.find(DEFAULT_SLOT_ID);
    if (it != multi_sim_managers_.end())
    {
        it->second->addInitCallback(std::move(clientCallback));
        return it->second;
    }
    auto mgr =
      std::make_shared<SimulaMultiSimManager>(DEFAULT_SLOT_ID, bridge_, std::move(clientCallback));
    multi_sim_managers_.emplace(DEFAULT_SLOT_ID, mgr);
    mgr->start();
    // Wire an existing card manager to the new multi-SIM manager.
    auto it_cm = card_managers_.find(DEFAULT_SLOT_ID);
    if (it_cm != card_managers_.end())
    {
        auto cm = it_cm->second;
        cm->setPresenceChangedCb(
          [mgr](bool present) { mgr->notifySlotStatusChanged(present); }
        );
    }
    return mgr;
}

}  // namespace telux::tel::simula
