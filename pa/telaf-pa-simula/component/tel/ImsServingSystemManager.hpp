// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear


#ifndef TELUX_TEL_SIMULA_IMS_SERVING_SYSTEM_MANAGER_HPP
#define TELUX_TEL_SIMULA_IMS_SERVING_SYSTEM_MANAGER_HPP

#include "../common/IModemBridge.hpp"

#include <chart/active_object.hpp>
#include <memory>
#include <mutex>
#include <telux/tel/ImsServingSystemManager.hpp>
#include <vector>

namespace telux::tel::simula {

class SimulaImsServingSystemManager final
    : public telux::tel::IImsServingSystemManager
    , private chart::ActiveObject
{
public:
    SimulaImsServingSystemManager(
      int slotId,
      common::simula::IModemBridge& bridge,
      telux::common::InitResponseCb initCb = nullptr
    );
    ~SimulaImsServingSystemManager() override;

    SimulaImsServingSystemManager(const SimulaImsServingSystemManager&) = delete;
    SimulaImsServingSystemManager& operator=(const SimulaImsServingSystemManager&) = delete;

    void start();

    void setInitCallback(telux::common::InitResponseCb cb);

    telux::common::ServiceStatus getServiceStatus() override;

    telux::common::Status
    requestRegistrationInfo(telux::tel::ImsRegistrationInfoCb callback) override;
    telux::common::Status requestServiceInfo(telux::tel::ImsServiceInfoCb callback) override;
    telux::common::Status requestPdpStatus(telux::tel::ImsPdpStatusInfoCb callback) override;

    telux::common::Status
    registerListener(std::weak_ptr<telux::tel::IImsServingSystemListener> listener) override;
    telux::common::Status
    deregisterListener(std::weak_ptr<telux::tel::IImsServingSystemListener> listener) override;

private:
    friend chart::Status ImsServNotReady_St(chart::Hsm*, chart::Event const*);
    friend chart::Status ImsServReady_St(chart::Hsm*, chart::Event const*);

    void handleReadinessInd_(std::string_view topic, const common::simula::Envelope& env);
    void handleRegStatusInd_(std::string_view topic, const common::simula::Envelope& env);
    void handleServiceInfoInd_(std::string_view topic, const common::simula::Envelope& env);
    void handlePdpStatusInd_(std::string_view topic, const common::simula::Envelope& env);
    void broadcastToListeners_(
      std::function<void(const std::shared_ptr<telux::tel::IImsServingSystemListener>&)> invoke
    );
    bool isReadyDerived_() const;
    void publishStatus_(telux::common::ServiceStatus s);
    void unsubscribeFromBridge_();

    common::simula::IModemBridge& bridge_;
    common::simula::IModemBridge::ConnectivityToken conn_token_{ 0 };
    int slotId_;
    std::vector<telux::common::InitResponseCb> init_cbs_;
    std::atomic<telux::common::ServiceStatus> last_status_{
        telux::common::ServiceStatus::SERVICE_UNAVAILABLE
    };

    std::mutex listeners_mutex_;
    std::vector<std::weak_ptr<telux::tel::IImsServingSystemListener>> listeners_;
};

}  // namespace telux::tel::simula

#endif  // TELUX_TEL_SIMULA_IMS_SERVING_SYSTEM_MANAGER_HPP
