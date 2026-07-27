// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef TELUX_TEL_SIMULA_SUBSCRIPTION_MANAGER_HPP
#define TELUX_TEL_SIMULA_SUBSCRIPTION_MANAGER_HPP

#include "../common/IModemBridge.hpp"
#include "Subscription.hpp"

#include <atomic>
#include <chart/active_object.hpp>
#include <memory>
#include <mutex>
#include <telux/tel/SubscriptionManager.hpp>
#include <vector>

namespace telux::tel::simula {

class SimulaSubscriptionManager final
    : public telux::tel::ISubscriptionManager
    , private chart::ActiveObject
{
public:
    SimulaSubscriptionManager(
      int slotId,
      common::simula::IModemBridge& bridge,
      telux::common::InitResponseCb initCb = nullptr
    );
    ~SimulaSubscriptionManager() override;

    SimulaSubscriptionManager(const SimulaSubscriptionManager&) = delete;
    SimulaSubscriptionManager& operator=(const SimulaSubscriptionManager&) = delete;

    // Starts the active object and bridge subscriptions.
    void start();

    // Queues a readiness callback or invokes it immediately when ready.
    void addInitCallback(telux::common::InitResponseCb cb);

    // telux::tel::ISubscriptionManager
    bool isSubsystemReady() override;
    std::future<bool> onSubsystemReady() override;
    telux::common::ServiceStatus getServiceStatus() override;
    std::shared_ptr<telux::tel::ISubscription> getSubscription(
      int slotId = DEFAULT_SLOT_ID,
      telux::common::Status* status = nullptr
    ) override;
    std::vector<std::shared_ptr<telux::tel::ISubscription>> getAllSubscriptions(
      telux::common::Status* status = nullptr
    ) override;
    telux::common::Status registerListener(
      std::weak_ptr<telux::tel::ISubscriptionListener> listener
    ) override;
    telux::common::Status removeListener(
      std::weak_ptr<telux::tel::ISubscriptionListener> listener
    ) override;

private:
    friend chart::Status SubNotReady_St(chart::Hsm*, chart::Event const*);
    friend chart::Status SubReady_St(chart::Hsm*, chart::Event const*);

    void handleSubInd_(std::string_view topic, const common::simula::Envelope& env);
    // Pulls and joins ICCID/IMSI through the normal update path.
    void resyncSubInfo_();
    void broadcastToListeners_(
      std::function<void(const std::shared_ptr<telux::tel::ISubscriptionListener>&)> invoke
    );
    // Invokes and clears queued readiness callbacks.
    void fireInitCallbacks_(telux::common::ServiceStatus status);

    // Mirror of start()'s bridge registrations, ending in a drain() fence.
    // Called from the dtor before any member teardown -- see the dtor.
    void unsubscribeFromBridge_();

    common::simula::IModemBridge& bridge_;
    // Handle for the connectivity observer registered in start(), withdrawn
    // in the dtor. 0 until start() runs, which unsubscribe_connectivity
    // treats as a no-op.
    common::simula::IModemBridge::ConnectivityToken conn_token_{ 0 };
    int slotId_;

    // Guarded because callbacks are added off the AO thread.
    std::mutex init_cbs_mutex_;
    std::vector<telux::common::InitResponseCb> init_cbs_;
    bool init_reported_{ false };

    std::atomic<bool> ready_flag_{ false };
    std::atomic<telux::common::ServiceStatus> last_status_{
        telux::common::ServiceStatus::SERVICE_UNAVAILABLE
    };

    // AO-thread-only subscription object.
    std::shared_ptr<SimulaSubscription> subscription_;

    std::mutex listeners_mutex_;
    std::vector<std::weak_ptr<telux::tel::ISubscriptionListener>> listeners_;
};

}  // namespace telux::tel::simula

#endif  // TELUX_TEL_SIMULA_SUBSCRIPTION_MANAGER_HPP
