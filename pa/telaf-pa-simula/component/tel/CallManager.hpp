// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear
//


#ifndef TELUX_TEL_SIMULA_CALL_MANAGER_HPP
#define TELUX_TEL_SIMULA_CALL_MANAGER_HPP

#include "../common/IModemBridge.hpp"

#include <chart/active_object.hpp>
#include <map>
#include <memory>
#include <mutex>
#include <telux/tel/CallManager.hpp>
#include <vector>

namespace telux::tel::simula {

class SimulaCall;

class SimulaCallManager final
    : public telux::tel::ICallManager
    , private chart::ActiveObject
{
public:
    explicit SimulaCallManager(
      common::simula::IModemBridge& bridge, telux::common::InitResponseCb initCb = nullptr
    );
    ~SimulaCallManager() override;

    SimulaCallManager(const SimulaCallManager&) = delete;
    SimulaCallManager& operator=(const SimulaCallManager&) = delete;

    void start();
    void setInitCallback(telux::common::InitResponseCb cb);

    // telux::tel::ICallManager
    telux::common::ServiceStatus getServiceStatus() override;

    // Generic voice calling -- out of scope.
    telux::common::Status makeCall(
      int phoneId, const std::string& dialNumber,
      std::shared_ptr<telux::tel::IMakeCallCallback> callback = nullptr
    ) override;
    telux::common::Status makeRttCall(
      int phoneId, const std::string& dialNumber,
      std::shared_ptr<telux::tel::IMakeCallCallback> callback = nullptr
    ) override;

    // makeECall -- all 8 ICallManager overloads.
    telux::common::Status makeECall(
      int phoneId, const telux::tel::ECallMsdData& eCallMsdData, int category, int variant,
      std::shared_ptr<telux::tel::IMakeCallCallback> callback = nullptr
    ) override;
    telux::common::Status makeECall(
      int phoneId, const std::string dialNumber, const telux::tel::ECallMsdData& eCallMsdData,
      int category, std::shared_ptr<telux::tel::IMakeCallCallback> callback = nullptr
    ) override;
    telux::common::Status makeECall(
      int phoneId, const std::string dialNumber, const std::vector<uint8_t>& msdPdu,
      telux::tel::CustomSipHeader header = { telux::tel::CONTENT_HEADER, "" },
      telux::tel::MakeCallCallback callback = nullptr
    ) override;
    telux::common::Status makeECall(
      int phoneId, const std::vector<uint8_t>& msdPdu, int category, int variant,
      telux::tel::MakeCallCallback callback = nullptr
    ) override;
    telux::common::Status makeECall(
      int phoneId, const std::string dialNumber, const std::vector<uint8_t>& msdPdu, int category,
      telux::tel::MakeCallCallback callback = nullptr
    ) override;
    telux::common::Status makeECall(
      int phoneId, int category, int variant, telux::tel::MakeCallCallback callback = nullptr
    ) override;
    telux::common::Status makeECall(
      int phoneId, const std::string dialNumber, int category,
      telux::tel::MakeCallCallback callback = nullptr
    ) override;
    telux::common::Status makeECall(
      int phoneId, const std::string dialNumber, const std::vector<uint8_t>& msdPdu,
      telux::tel::MakeCallCallback callback = nullptr
    ) override;

    telux::common::Status updateECallMsd(
      int phoneId, const telux::tel::ECallMsdData& eCallMsd,
      std::shared_ptr<telux::common::ICommandResponseCallback> callback = nullptr
    ) override;
    telux::common::Status updateECallMsd(
      int phoneId, const std::vector<uint8_t>& msdPdu, telux::common::ResponseCallback callback
    ) override;
    telux::common::Status requestECallHlapTimerStatus(
      int phoneId, telux::tel::ECallHlapTimerStatusCallback callback
    ) override;
    std::vector<std::shared_ptr<telux::tel::ICall>> getInProgressCalls() override;

    telux::common::Status conference(
      std::shared_ptr<telux::tel::ICall> call1, std::shared_ptr<telux::tel::ICall> call2,
      std::shared_ptr<telux::common::ICommandResponseCallback> callback = nullptr
    ) override;
    telux::common::Status swap(
      std::shared_ptr<telux::tel::ICall> callToHold, std::shared_ptr<telux::tel::ICall> callToActivate,
      std::shared_ptr<telux::common::ICommandResponseCallback> callback = nullptr
    ) override;
    telux::common::Status hangupForegroundResumeBackground(
      int phoneId, telux::common::ResponseCallback callback = nullptr
    ) override;
    telux::common::Status hangupWaitingOrBackground(
      int phoneId, telux::common::ResponseCallback callback = nullptr
    ) override;
    telux::common::Status
      requestEcbm(int phoneId, telux::tel::EcbmStatusCallback callback) override;
    telux::common::Status
      exitEcbm(int phoneId, telux::common::ResponseCallback callback = nullptr) override;

    telux::common::Status requestNetworkDeregistration(
      int phoneId, telux::common::ResponseCallback callback = nullptr
    ) override;
    telux::common::Status updateEcallHlapTimer(
      int phoneId, telux::tel::HlapTimerType type, uint32_t timeDuration,
      telux::common::ResponseCallback callback = nullptr
    ) override;
    telux::common::Status requestEcallHlapTimer(
      int phoneId, telux::tel::HlapTimerType type, telux::tel::ECallHlapTimerCallback callback
    ) override;
    telux::common::Status setECallConfig(telux::tel::EcallConfig config) override;
    telux::common::Status getECallConfig(telux::tel::EcallConfig& config) override;

    telux::common::ErrorCode
      encodeECallMsd(telux::tel::ECallMsdData eCallMsdData, std::vector<uint8_t>& data) override;
    telux::common::Status encodeEuroNcapOptionalAdditionalData(
      telux::tel::ECallOptionalEuroNcapData optionalEuroNcapData, std::vector<uint8_t>& data
    ) override;
    telux::common::Status sendRtt(
      int phoneId, std::string message, telux::common::ResponseCallback callback = nullptr
    ) override;
    telux::common::Status configureECallRedial(
      telux::tel::RedialConfigType config, const std::vector<int>& timeGap,
      telux::common::ResponseCallback callback = nullptr
    ) override;
    telux::common::Status restartECallHlapTimer(
      int phoneId, telux::tel::EcallHlapTimerId timerId, int duration,
      telux::common::ResponseCallback callback = nullptr
    ) override;
    telux::common::ErrorCode
      getECallRedialConfig(std::vector<int>& callOrigTimeGap, std::vector<int>& callDropTimeGap)
        override;
    telux::common::Status updateECallPostTestRegistrationTimer(
      int phoneId, uint32_t timer, telux::common::ResponseCallback callback = nullptr
    ) override;
    telux::common::ErrorCode getECallPostTestRegistrationTimer(int phoneId, uint32_t& timer)
      override;
    telux::common::Status setEmergencyMode(
      int phoneId, bool emergencyModeEnabled, bool antennaSwitchEnabled = false,
      telux::common::ResponseCallback callback = nullptr
    ) override;

    telux::common::Status registerListener(std::shared_ptr<telux::tel::ICallListener> listener)
      override;
    telux::common::Status removeListener(std::shared_ptr<telux::tel::ICallListener> listener)
      override;

private:
    friend chart::Status CallMgrNotReady_St(chart::Hsm*, chart::Event const*);
    friend chart::Status CallMgrReady_St(chart::Hsm*, chart::Event const*);

    bool isReadyDerived_() const;
    void unsubscribeFromBridge_();
    void ensureStarted_() { if (!running()) start(); }

    void handleReadinessInd_(std::string_view topic, const common::simula::Envelope& env);
    void handleCallStateInd_(std::string_view topic, const common::simula::Envelope& env);
    void handleMsdStatusInd_(std::string_view topic, const common::simula::Envelope& env);
    void handleHlapTimerEventInd_(std::string_view topic, const common::simula::Envelope& env);
    void handleRedialInd_(std::string_view topic, const common::simula::Envelope& env);

    void doMakeECall_(
      int phoneId, const std::string& type, bool isMsdTransmitted,
      const std::string& remotePartyNumber, const std::string& dialNumber,
      std::function<void(telux::common::ErrorCode, std::shared_ptr<telux::tel::ICall>)> notify
    );
    void broadcastToListeners_(
      std::function<void(const std::shared_ptr<telux::tel::ICallListener>&)> invoke
    );
    std::shared_ptr<SimulaCall> findOrCreateCall_(
      int phoneId, int callIndex, telux::tel::CallDirection direction,
      const std::string& remotePartyNumber
    );

    common::simula::IModemBridge& bridge_;
    common::simula::IModemBridge::ConnectivityToken conn_token_{ 0 };

    std::vector<telux::common::InitResponseCb> init_cbs_;

    std::mutex calls_mutex_;
    std::map<int, std::shared_ptr<SimulaCall>> calls_;  // keyed by phoneId

    std::mutex hlap_events_mutex_;
    std::map<int, telux::tel::ECallHlapTimerEvents> hlapEvents_;  // keyed by phoneId

    std::mutex listeners_mutex_;
    std::vector<std::shared_ptr<telux::tel::ICallListener>> listeners_;
};

}  // namespace telux::tel::simula

#endif  // TELUX_TEL_SIMULA_CALL_MANAGER_HPP
