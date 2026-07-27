// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear
// Single-slot multi-SIM manager; readiness follows bridge connectivity.

#ifndef TELUX_TEL_SIMULA_MULTI_SIM_MANAGER_HPP
#define TELUX_TEL_SIMULA_MULTI_SIM_MANAGER_HPP

#include "../common/IModemBridge.hpp"

#include <atomic>
#include <chart/active_object.hpp>
#include <memory>
#include <mutex>
#include <telux/tel/MultiSimManager.hpp>
#include <vector>

namespace telux::tel::simula {

class SimulaMultiSimManager final
    : public telux::tel::IMultiSimManager
    , private chart::ActiveObject
{
public:
    SimulaMultiSimManager(
      int slotId,
      common::simula::IModemBridge& bridge,
      telux::common::InitResponseCb initCb = nullptr
    );
    ~SimulaMultiSimManager() override;

    SimulaMultiSimManager(const SimulaMultiSimManager&) = delete;
    SimulaMultiSimManager& operator=(const SimulaMultiSimManager&) = delete;

    // Broadcasts a card-presence change so the PA refreshes its card cache.
    void notifySlotStatusChanged(bool cardPresent);

    // Starts the active object and bridge connectivity subscription.
    void start();

    // Queues a readiness callback or invokes it immediately when ready.
    void addInitCallback(telux::common::InitResponseCb cb);

    // telux::tel::IMultiSimManager
    bool isSubsystemReady() override;
    std::future<bool> onSubsystemReady() override;
    telux::common::ServiceStatus getServiceStatus() override;
    telux::common::Status getSlotCount(int& count) override;
    telux::common::Status requestHighCapability(HighCapabilityCallback callback) override;
    telux::common::Status setHighCapability(
      int slotId,
      common::ResponseCallback callback = nullptr
    ) override;
    telux::common::Status switchActiveSlot(
      SlotId slotId,
      common::ResponseCallback callback = nullptr
    ) override;
    telux::common::Status requestSlotStatus(SlotStatusCallback callback) override;
    telux::common::Status registerListener(std::weak_ptr<telux::tel::IMultiSimListener> listener) override;
    telux::common::Status deregisterListener(std::weak_ptr<telux::tel::IMultiSimListener> listener) override;

private:
    friend chart::Status MultiSimNotReady_St(chart::Hsm*, chart::Event const*);
    friend chart::Status MultiSimReady_St(chart::Hsm*, chart::Event const*);

    void broadcastToListeners_(
      std::function<void(const std::shared_ptr<telux::tel::IMultiSimListener>&)> invoke
    );

    // Fetches card state; an RPC failure reports UNKNOWN on an active slot.
    void fetchSlotStatus_(SlotStatusCallback callback);

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

    // Active simulated slot.
    std::atomic<int> active_slot_;

    std::mutex listeners_mutex_;
    std::vector<std::weak_ptr<telux::tel::IMultiSimListener>> listeners_;
};

}  // namespace telux::tel::simula

#endif  // TELUX_TEL_SIMULA_MULTI_SIM_MANAGER_HPP
