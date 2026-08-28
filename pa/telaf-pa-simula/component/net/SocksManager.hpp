// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear
//
// SocksManager.hpp - SimulaSocksManager, 2-state readiness shell.
//
// The smallest net manager: ISocksManager has exactly one operational method
// (enableSocks). It still gets the full readiness shell so a client calling
// enableSocks before the subsystem is up gets NOTREADY rather than a silently
// dropped request.

#ifndef TELUX_DATA_NET_SIMULA_SOCKS_MANAGER_HPP
#define TELUX_DATA_NET_SIMULA_SOCKS_MANAGER_HPP

#include "../common/IModemBridge.hpp"
#include "../common/InitCallbackGate.hpp"

#include <atomic>
#include <chart/active_object.hpp>
#include <functional>
#include <memory>
#include <mutex>
#include <telux/data/net/SocksManager.hpp>
#include <vector>

namespace telux::data::net::simula {

class SimulaSocksManager final
    : public telux::data::net::ISocksManager
    , private chart::ActiveObject
{
public:
    SimulaSocksManager(
      telux::data::OperationType opType, common::simula::IModemBridge& bridge,
      telux::common::InitResponseCb initCb = nullptr
    );
    ~SimulaSocksManager() override;

    SimulaSocksManager(const SimulaSocksManager&) = delete;
    SimulaSocksManager& operator=(const SimulaSocksManager&) = delete;

    void start();

    // Registers an additional InitResponseCb after construction. The target PA
    // calls the factory getter twice -- second call carries the callback that
    // unblocks its 30s promise wait -- so this must be honoured, not dropped.
    void addInitCallback(telux::common::InitResponseCb cb);

    // telux::data::net::ISocksManager
    telux::common::ServiceStatus getServiceStatus() override;
    bool isSubsystemReady() override;
    std::future<bool> onSubsystemReady() override;
    telux::common::Status enableSocks(
      bool enable, telux::common::ResponseCallback callback = nullptr
    ) override;
    telux::common::Status registerListener(std::weak_ptr<ISocksListener> listener) override;
    telux::common::Status deregisterListener(std::weak_ptr<ISocksListener> listener) override;
    telux::data::OperationType getOperationType() override;

private:
    friend chart::Status SocksNotReady_St(chart::Hsm*, chart::Event const*);
    friend chart::Status SocksReady_St(chart::Hsm*, chart::Event const*);
    friend chart::Status SocksOperating_St(chart::Hsm*, chart::Event const*);

    void handleInd_(std::string_view topic, const common::simula::Envelope& env);
    void broadcastToListeners_(
      std::function<void(const std::shared_ptr<ISocksListener>&)> invoke
    );
    bool isReadyDerived_() const;
    void publishStatus_(telux::common::ServiceStatus s);
    void unsubscribeFromBridge_();

    telux::data::OperationType op_type_;
    common::simula::IModemBridge& bridge_;
    common::simula::InitCallbackGate init_gate_;
    common::simula::IModemBridge::ConnectivityToken conn_token_{ 0 };
    std::atomic<telux::common::ServiceStatus> last_status_{
        telux::common::ServiceStatus::SERVICE_UNAVAILABLE
    };

    std::mutex listeners_mutex_;
    std::vector<std::weak_ptr<ISocksListener>> listeners_;
};

}  // namespace telux::data::net::simula

#endif  // TELUX_DATA_NET_SIMULA_SOCKS_MANAGER_HPP
