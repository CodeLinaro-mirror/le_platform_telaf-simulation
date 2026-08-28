// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear
//
// DataSettingsManager.hpp - SimulaDataSettingsManager.
//
// Note: this lives under component/net/ with the other net managers even
// though telux::data::IDataSettingsManager is in namespace telux::data (not
// telux::data::net). It belongs here because it is part of the same taf_net
// surface and shares the net domain's registries and signal range; splitting
// it out by namespace alone would scatter one wire domain across two dirs.
//
// The synchronous-API mirror
// --------------------------
// Six of these methods return telux::common::ErrorCode directly instead of
// taking a ResponseCallback, so they must produce a value before returning.
// Blocking on an MQTT round-trip inside them risks deadlock (a client may call
// them from a listener callback, i.e. from the ListenerDispatchAO thread).
//
// They are therefore answered from ippt_mirror_ / ip_config_mirror_, which hold
// ONLY settled MPSS-derived state: every entry arrived on a retained
// net_settings ippt_config_sync / ip_config_sync indication. Writes issue
// dedicated MPSS RPCs and do NOT update the mirror, not even optimistically --
// otherwise a write MPSS rejected or dropped would leave the PA answering
// subsequent getIp*() calls from a value the modem never accepted, making the
// cache an authoritative source (invariant (d) forbids exactly that). A
// rejected/timed-out write therefore leaves the last MPSS-published value in
// place and is logged; MPSS remains ground truth, the mirror is a cache.

#ifndef TELUX_DATA_SIMULA_DATA_SETTINGS_MANAGER_HPP
#define TELUX_DATA_SIMULA_DATA_SETTINGS_MANAGER_HPP

#include "../common/IModemBridge.hpp"
#include "../common/InitCallbackGate.hpp"

#include <atomic>
#include <chart/active_object.hpp>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <nlohmann/json.hpp>
#include <telux/data/DataSettingsManager.hpp>
#include <tuple>
#include <vector>

namespace telux::data::simula {

class SimulaDataSettingsManager final
    : public telux::data::IDataSettingsManager
    , private chart::ActiveObject
{
public:
    SimulaDataSettingsManager(
      telux::data::OperationType opType,
      common::simula::IModemBridge& bridge,
      telux::common::InitResponseCb initCb = nullptr
    );
    // Not 'override': IDataSettingsManager declares no virtual destructor (unlike
    // the net INatManager/IVlanManager interfaces). Deletion through a base
    // pointer is therefore never safe here, so the factory must keep handing out
    // shared_ptr<SimulaDataSettingsManager>-derived deleters, which it does.
    ~SimulaDataSettingsManager();

    SimulaDataSettingsManager(const SimulaDataSettingsManager&) = delete;
    SimulaDataSettingsManager& operator=(const SimulaDataSettingsManager&) = delete;

    void start();

    // Registers an additional InitResponseCb after construction. The target PA
    // calls the factory getter twice -- second call carries the callback that
    // unblocks its 30s promise wait -- so this must be honoured, not dropped.
    void addInitCallback(telux::common::InitResponseCb cb);
    // Factory uses this to honour the SDK's second-getter callback contract.
    bool isSubsystemReady() const;

    // telux::data::IDataSettingsManager
    telux::common::ServiceStatus getServiceStatus() override;
    telux::common::Status restoreFactorySettings(
      OperationType operationType,
      telux::common::ResponseCallback callback = nullptr,
      bool isRebootNeeded = true
    ) override;
    telux::common::Status setBackhaulPreference(
      std::vector<BackhaulType> backhaulPref,
      telux::common::ResponseCallback callback = nullptr
    ) override;
    telux::common::Status requestBackhaulPreference(
      RequestBackhaulPrefResponseCb callback
    ) override;
    telux::common::Status setBandInterferenceConfig(
      bool enable,
      std::shared_ptr<BandInterferenceConfig> config = nullptr,
      telux::common::ResponseCallback callback = nullptr
    ) override;
    telux::common::Status requestBandInterferenceConfig(
      RequestBandInterferenceConfigResponseCb callback
    ) override;
    telux::common::Status setWwanConnectivityConfig(
      SlotId slotId,
      bool allow,
      telux::common::ResponseCallback callback = nullptr
    ) override;
    telux::common::Status requestWwanConnectivityConfig(
      SlotId slotId,
      requestWwanConnectivityConfigResponseCb callback
    ) override;
    bool isDeviceDataUsageMonitoringEnabled() override;
    telux::common::Status
    setMacSecState(bool enable, telux::common::ResponseCallback callback = nullptr) override;
    telux::common::Status requestMacSecState(RequestMacSecSateResponseCb callback) override;
    telux::common::Status switchBackHaul(
      BackhaulInfo source,
      BackhaulInfo dest,
      bool applyToAll = false,
      telux::common::ResponseCallback callback = nullptr
    ) override;
    telux::common::ErrorCode
    setIpPassThroughConfig(const IpptParams& ipptParms, const IpptConfig& config) override;
    telux::common::ErrorCode setIpPassThroughNatConfig(bool enableNat = true) override;
    telux::common::ErrorCode getIpPassThroughNatConfig(bool& isNatEnabled) override;
    telux::common::ErrorCode
    getIpPassThroughConfig(const IpptParams& ipptParms, IpptConfig& config) override;
    telux::common::ErrorCode
    setIpConfig(const IpConfigParams& ipConfigParams, const IpConfig& ipConfig) override;
    telux::common::ErrorCode
    getIpConfig(const IpConfigParams& ipConfigParams, IpConfig& ipConfig) override;
    telux::common::Status registerListener(std::weak_ptr<IDataSettingsListener> listener) override;
    telux::common::Status deregisterListener(
      std::weak_ptr<IDataSettingsListener> listener
    ) override;
    telux::common::Status
    requestDdsSwitch(DdsInfo request, telux::common::ResponseCallback callback = nullptr) override;
    telux::common::Status requestCurrentDds(RequestCurrentDdsResponseCb callback) override;

private:
    friend chart::Status SetNotReady_St(chart::Hsm*, chart::Event const*);
    friend chart::Status SetReady_St(chart::Hsm*, chart::Event const*);
    friend chart::Status SetOperating_St(chart::Hsm*, chart::Event const*);

    void handleReadyInd_(std::string_view topic, const common::simula::Envelope& env);
    void handleWwanInd_(std::string_view topic, const common::simula::Envelope& env);
    void handleDdsInd_(std::string_view topic, const common::simula::Envelope& env);
    void handleIpptSyncInd_(std::string_view topic, const common::simula::Envelope& env);
    void handleIpConfigSyncInd_(std::string_view topic, const common::simula::Envelope& env);
    void broadcastToListeners_(
      std::function<void(const std::shared_ptr<IDataSettingsListener>&)> invoke
    );
    // Sole authority for readiness: derived from the chart's current-state
    // pointer, exactly as the sibling net managers do it. last_status_ is NOT
    // consulted -- it only records the FAILED-vs-UNAVAILABLE reason the 2-state
    // chart cannot express.
    bool isReadyDerived_() const;
    void publishStatus_(telux::common::ServiceStatus s);
    void unsubscribeFromBridge_();

    // Whole-snapshot mirror replacement from a retained *_sync indication.
    // Called only from the chart handlers (AO thread); shared by NotReady and
    // Ready so those two paths cannot drift apart.
    void applyIpptSync_(const nlohmann::json& data);
    void applyIpConfigSync_(const nlohmann::json& data);

    // Write-through helper for the six synchronous ErrorCode methods.
    telux::common::ErrorCode
    publishMirrorWrite_(std::string_view topic, std::string_view schemaId, nlohmann::json data);

    telux::data::OperationType op_type_;
    common::simula::IModemBridge& bridge_;
    common::simula::InitCallbackGate init_gate_;
    common::simula::IModemBridge::ConnectivityToken conn_token_{ 0 };
    std::atomic<telux::common::ServiceStatus> last_status_{
        telux::common::ServiceStatus::SERVICE_UNAVAILABLE
    };

    // Mirrors for the synchronous APIs. Guarded by their own mutex because the
    // sync methods read them on the *caller's* thread, not the AO thread.
    std::mutex mirror_mutex_;
    bool ippt_nat_enabled_{ true };
    // Keyed profileId/vlanId/slotId, matching the MPSS-side key.
    std::map<std::tuple<int, int, int>, IpptConfig> ippt_mirror_;
    // Keyed ifType/ipFamily/vlanId.
    std::map<std::tuple<int, int, int>, IpConfig> ip_config_mirror_;

    std::mutex listeners_mutex_;
    std::vector<std::weak_ptr<IDataSettingsListener>> listeners_;
};

}  // namespace telux::data::simula

#endif  // TELUX_DATA_SIMULA_DATA_SETTINGS_MANAGER_HPP
