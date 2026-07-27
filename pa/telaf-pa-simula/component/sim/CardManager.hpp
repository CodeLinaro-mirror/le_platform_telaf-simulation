// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef TELUX_TEL_SIMULA_CARD_MANAGER_HPP
#define TELUX_TEL_SIMULA_CARD_MANAGER_HPP

#include "../common/IModemBridge.hpp"
#include "SimCard.hpp"

#include <atomic>
#include <chart/active_object.hpp>
#include <memory>
#include <mutex>
#include <telux/tel/CardManager.hpp>
#include <vector>

namespace telux::tel::simula {

class SimulaCardManager final
    : public telux::tel::ICardManager
    , private chart::ActiveObject
    , public std::enable_shared_from_this<SimulaCardManager>
{
public:
    SimulaCardManager(
      int slotId,
      common::simula::IModemBridge& bridge,
      telux::common::InitResponseCb initCb = nullptr
    );
    ~SimulaCardManager() override;

    SimulaCardManager(const SimulaCardManager&) = delete;
    SimulaCardManager& operator=(const SimulaCardManager&) = delete;

    // Starts the active object and bridge subscriptions.
    void start();

    // Called on the AO thread when getCard() would change nullness.
    using PresenceChangedCb = std::function<void(bool present)>;
    void setPresenceChangedCb(PresenceChangedCb cb);

    // Queues a readiness callback or invokes it immediately when ready.
    void addInitCallback(telux::common::InitResponseCb cb);

    // telux::tel::ICardManager
    bool isSubsystemReady() override;
    std::future<bool> onSubsystemReady() override;
    telux::common::ServiceStatus getServiceStatus() override;
    telux::common::Status getSlotCount(int& count) override;
    telux::common::Status getSlotIds(std::vector<int>& slotIds) override;
    std::shared_ptr<telux::tel::ICard> getCard(
      int slotId = DEFAULT_SLOT_ID,
      telux::common::Status* status = nullptr
    ) override;
    telux::common::Status cardPowerUp(
      SlotId slotId,
      telux::common::ResponseCallback callback = nullptr
    ) override;
    telux::common::Status cardPowerDown(
      SlotId slotId,
      telux::common::ResponseCallback callback = nullptr
    ) override;
    telux::common::Status setupRefreshConfig(
      SlotId slotId,
      bool isRegister,
      bool doVoting,
      std::vector<telux::tel::IccFile> efFiles,
      telux::tel::RefreshParams refreshParams,
      telux::common::ResponseCallback callback = nullptr
    ) override;
    telux::common::Status allowCardRefresh(
      SlotId slotId,
      bool allowRefresh,
      telux::tel::RefreshParams refreshParams,
      telux::common::ResponseCallback callback = nullptr
    ) override;
    telux::common::Status confirmRefreshHandlingCompleted(
      SlotId slotId,
      bool isCompleted,
      telux::tel::RefreshParams refreshParams,
      telux::common::ResponseCallback callback = nullptr
    ) override;
    telux::common::Status requestLastRefreshEvent(
      SlotId slotId,
      telux::tel::RefreshParams refreshParams,
      telux::tel::refreshLastEventResponseCallback callback
    ) override;
    telux::common::Status registerListener(std::shared_ptr<telux::tel::ICardListener> listener) override;
    telux::common::Status removeListener(std::shared_ptr<telux::tel::ICardListener> listener) override;

private:
    friend chart::Status CardNotReady_St(chart::Hsm*, chart::Event const*);
    friend chart::Status CardReady_St(chart::Hsm*, chart::Event const*);

    void handleCardInd_(std::string_view topic, const common::simula::Envelope& env);
    // Pulls current card state through the normal state-update path.
    void resyncCardState_();
    void broadcastToListeners_(
      std::function<void(const std::shared_ptr<telux::tel::ICardListener>&)> invoke
    );
    // Invokes and clears queued readiness callbacks.
    void fireInitCallbacks_(telux::common::ServiceStatus status);

    // True when getCard() should return a non-null card.
    bool cardIsPresent_() const;

    // Notifies only when card presence changes.
    void notifyIfPresenceChanged_();

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

    // AO-thread-only card object.
    std::shared_ptr<SimulaCard> card_;

    // Last presence value reported through presence_cb_.
    bool last_present_{ false };
    std::mutex presence_cb_mutex_;
    PresenceChangedCb presence_cb_;

    std::mutex listeners_mutex_;
    std::vector<std::weak_ptr<telux::tel::ICardListener>> listeners_;
};

}  // namespace telux::tel::simula

#endif  // TELUX_TEL_SIMULA_CARD_MANAGER_HPP
