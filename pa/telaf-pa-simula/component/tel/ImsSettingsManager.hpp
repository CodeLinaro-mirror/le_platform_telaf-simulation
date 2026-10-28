// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear


#ifndef TELUX_TEL_SIMULA_IMS_SETTINGS_MANAGER_HPP
#define TELUX_TEL_SIMULA_IMS_SETTINGS_MANAGER_HPP

#include "../common/IModemBridge.hpp"

#include <chart/active_object.hpp>
#include <memory>
#include <mutex>
#include <telux/tel/ImsSettingsManager.hpp>
#include <vector>

namespace telux::tel::simula {

class SimulaImsSettingsManager final
    : public telux::tel::IImsSettingsManager
    , private chart::ActiveObject
{
public:
    explicit SimulaImsSettingsManager(
      common::simula::IModemBridge& bridge,
      telux::common::InitResponseCb initCb = nullptr
    );
    ~SimulaImsSettingsManager() override;

    SimulaImsSettingsManager(const SimulaImsSettingsManager&) = delete;
    SimulaImsSettingsManager& operator=(const SimulaImsSettingsManager&) = delete;

    void start();
    void setInitCallback(telux::common::InitResponseCb cb);

    // telux::tel::IImsSettingsManager
    telux::common::ServiceStatus getServiceStatus() override;

    telux::common::Status
    requestServiceConfig(SlotId slotId, telux::tel::ImsServiceConfigCb callback) override;
    telux::common::Status requestSipUserAgent(
      SlotId slotId, telux::tel::ImsSipUserAgentConfigCb callback
    ) override;
    telux::common::Status setSipUserAgent(
      SlotId slotId, std::string userAgent,
      telux::common::ResponseCallback callback = nullptr
    ) override;
    telux::common::Status setServiceConfig(
      SlotId slotId, telux::tel::ImsServiceConfig config,
      telux::common::ResponseCallback callback = nullptr
    ) override;
    telux::common::Status toggleVonr(
      SlotId slotId, bool isEnable,
      telux::common::ResponseCallback callback = nullptr
    ) override;
    telux::common::Status
    requestVonrStatus(SlotId slotId, telux::tel::ImsVonrStatusCb callback) override;

    telux::common::Status
    registerListener(std::weak_ptr<telux::tel::IImsSettingsListener> listener) override;
    telux::common::Status
    deregisterListener(std::weak_ptr<telux::tel::IImsSettingsListener> listener) override;

private:
    friend chart::Status ImsSettingsNotReady_St(chart::Hsm*, chart::Event const*);
    friend chart::Status ImsSettingsReady_St(chart::Hsm*, chart::Event const*);

    void handleReadinessInd_(std::string_view topic, const common::simula::Envelope& env);
    void handleServiceConfigInd_(std::string_view topic, const common::simula::Envelope& env);
    void handleSipUserAgentInd_(std::string_view topic, const common::simula::Envelope& env);
    void broadcastToListeners_(
      std::function<void(const std::shared_ptr<telux::tel::IImsSettingsListener>&)> invoke
    );
    bool isReadyDerived_() const;
    void unsubscribeFromBridge_();

    void doSetService_(
      int slotId, std::string service, bool enable, telux::common::ResponseCallback cb
    );
    void doRequestService_(
      int slotId, std::string service, std::function<void(bool, telux::common::ErrorCode)> cb
    );

    common::simula::IModemBridge& bridge_;
    common::simula::IModemBridge::ConnectivityToken conn_token_{ 0 };
    std::vector<telux::common::InitResponseCb> init_cbs_;
    std::atomic<telux::common::ServiceStatus> last_status_{
        telux::common::ServiceStatus::SERVICE_UNAVAILABLE
    };

    std::mutex listeners_mutex_;
    std::vector<std::weak_ptr<telux::tel::IImsSettingsListener>> listeners_;
};

}  // namespace telux::tel::simula

#endif  // TELUX_TEL_SIMULA_IMS_SETTINGS_MANAGER_HPP
