// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "DataSettingsManager.hpp"

#include "../common/EventCast.hpp"
#include "../common/ListenerDispatchAO.hpp"
#include "../common/Log.hpp"
#include "generated/cpp/topics.h"
#include "Signals.hpp"
#include "WireEnums.hpp"

// chart/spy.hpp is required directly, not transitively: CHART_NAMED_STATE and
// chart::SpyInstrument both live there, and the only chart header reaching this
// TU otherwise is active_object.hpp (-> instrument.hpp -> hsm.hpp), which does
// not pull in spy.hpp. Same explicit include set the sibling net managers use.
#include <algorithm>
#include <chart/event.hpp>
#include <chart/hsm.hpp>
#include <chart/spy.hpp>
#include <chrono>
#include <optional>
#include <string>

namespace telux::data::simula {

using namespace telux::data::net::simula::NetSignals;

namespace {
using common::simula::Envelope;
using telux::data::net::simula::wire::fromBackhaul;
using telux::data::net::simula::wire::fromBackhaulInfo;
using telux::data::net::simula::wire::fromIfaceType;
using telux::data::net::simula::wire::fromIpFamily;
using telux::data::net::simula::wire::toBackhaul;
using telux::data::net::simula::wire::toIfaceType;
using telux::data::net::simula::wire::toIpFamily;

constexpr auto kRpcTimeout = std::chrono::seconds(30);

// Bridge callbacks always run on the bridge worker. They carry wire input into
// this manager's AO; no manager state is changed on that worker thread.
struct IndPld
{
    Envelope env;
};

void
complete(const telux::common::ResponseCallback& cb, std::optional<Envelope> rsp)
{
    if (!cb)
        return;
    if (!rsp)
    {
        cb(telux::common::ErrorCode::OPERATION_TIMEOUT);
        return;
    }
    if (rsp->error)
    {
        cb(common::simula::parseErrorCode(rsp->error->value("code", std::string())));
        return;
    }
    cb(telux::common::ErrorCode::SUCCESS);
}

std::string
fromDds(DdsType t)
{
    return t == DdsType::TEMPORARY ? "TEMPORARY" : "PERMANENT";
}
DdsType
toDds(const std::string& s)
{
    return s == "TEMPORARY" ? DdsType::TEMPORARY : DdsType::PERMANENT;
}
std::string
fromOperation(Operation op)
{
    if (op == Operation::ENABLE)
        return "ENABLE";
    if (op == Operation::DISABLE)
        return "DISABLE";
    return "UNKNOWN";
}
Operation
toOperation(const std::string& s)
{
    if (s == "ENABLE")
        return Operation::ENABLE;
    if (s == "DISABLE")
        return Operation::DISABLE;
    return Operation::UNKNOWN;
}
std::string
fromIpType(IpAssignType t)
{
    if (t == IpAssignType::STATIC_IP)
        return "STATIC_IP";
    if (t == IpAssignType::DYNAMIC_IP)
        return "DYNAMIC_IP";
    return "UNKNOWN";
}
IpAssignType
toIpType(const std::string& s)
{
    if (s == "STATIC_IP")
        return IpAssignType::STATIC_IP;
    if (s == "DYNAMIC_IP")
        return IpAssignType::DYNAMIC_IP;
    return IpAssignType::UNKNOWN;
}
std::string
fromIpOperation(IpAssignOperation op)
{
    if (op == IpAssignOperation::ENABLE)
        return "ENABLE";
    if (op == IpAssignOperation::DISABLE)
        return "DISABLE";
    if (op == IpAssignOperation::RECONFIGURE)
        return "RECONFIGURE";
    return "UNKNOWN";
}
IpAssignOperation
toIpOperation(const std::string& s)
{
    if (s == "ENABLE")
        return IpAssignOperation::ENABLE;
    if (s == "DISABLE")
        return IpAssignOperation::DISABLE;
    if (s == "RECONFIGURE")
        return IpAssignOperation::RECONFIGURE;
    return IpAssignOperation::UNKNOWN;
}

nlohmann::json
fromIppt(const IpptParams& p, const IpptConfig& c)
{
    nlohmann::json j = { { "profileId", p.profileId },
                         { "vlanId", p.vlanId },
                         { "slot", static_cast<int>(p.slotId) },
                         { "ipptOpr", fromOperation(c.ipptOpr) } };
    if (c.devConfig.nwInterface != InterfaceType::UNKNOWN)
        j["nwInterface"] = fromIfaceType(c.devConfig.nwInterface);
    if (!c.devConfig.macAddr.empty())
        j["macAddr"] = c.devConfig.macAddr;
    return j;
}

IpptConfig
toIppt(const nlohmann::json& j)
{
    IpptConfig c{};
    c.ipptOpr = toOperation(j.value("ipptOpr", std::string()));
    c.devConfig.nwInterface = toIfaceType(j.value("nwInterface", std::string()));
    c.devConfig.macAddr = j.value("macAddr", std::string());
    return c;
}

nlohmann::json
fromIpConfig(const IpConfigParams& p, const IpConfig& c)
{
    nlohmann::json j = { { "ifType", fromIfaceType(p.ifType) },
                         { "ipFamily", fromIpFamily(p.ipFamilyType) },
                         { "ipType", fromIpType(c.ipType) },
                         { "ipOpr", fromIpOperation(c.ipOpr) } };
    if (p.vlanId != static_cast<uint32_t>(-1))
        j["vlanId"] = p.vlanId;
    if (!c.ipAddr.ifAddress.empty())
        j["ifAddress"] = c.ipAddr.ifAddress;
    if (c.ipAddr.ifMask)
        j["ifMask"] = c.ipAddr.ifMask;
    if (!c.ipAddr.gwAddress.empty())
        j["gwAddress"] = c.ipAddr.gwAddress;
    if (!c.ipAddr.primaryDnsAddress.empty())
        j["primaryDnsAddress"] = c.ipAddr.primaryDnsAddress;
    if (!c.ipAddr.secondaryDnsAddress.empty())
        j["secondaryDnsAddress"] = c.ipAddr.secondaryDnsAddress;
    return j;
}

IpConfig
toIpConfig(const nlohmann::json& j)
{
    IpConfig c{};
    c.ipType = toIpType(j.value("ipType", std::string()));
    c.ipOpr = toIpOperation(j.value("ipOpr", std::string()));
    c.ipAddr.ifAddress = j.value("ifAddress", std::string());
    c.ipAddr.ifMask = j.value("ifMask", 0u);
    c.ipAddr.gwAddress = j.value("gwAddress", std::string());
    c.ipAddr.primaryDnsAddress = j.value("primaryDnsAddress", std::string());
    c.ipAddr.secondaryDnsAddress = j.value("secondaryDnsAddress", std::string());
    return c;
}
}  // namespace

// Forward declarations: start_at() below needs SetNotReady_St, and the Ready/
// NotReady handlers reference each other plus their composite parent. The chart
// is the SOLE owner of readiness (invariant h) -- isReadyDerived_() compares
// current_state() against SetReady_St, and last_status_ only records the
// FAILED-vs-UNAVAILABLE reason the 2-state chart cannot express.
chart::Status
SetNotReady_St(chart::Hsm*, chart::Event const*);
chart::Status
SetReady_St(chart::Hsm*, chart::Event const*);
chart::Status
SetOperating_St(chart::Hsm*, chart::Event const*);

SimulaDataSettingsManager::SimulaDataSettingsManager(
  OperationType opType,
  common::simula::IModemBridge& bridge,
  telux::common::InitResponseCb initCb
)
    : chart::ActiveObject("DataSettingsManager")
    , op_type_(opType)
    , bridge_(bridge)
    , init_gate_(std::move(initCb))
{}

SimulaDataSettingsManager::~SimulaDataSettingsManager()
{
    unsubscribeFromBridge_();
    stop();
}

void
SimulaDataSettingsManager::addInitCallback(telux::common::InitResponseCb cb)
{
    init_gate_.add(std::move(cb));
}

void
SimulaDataSettingsManager::start()
{
    if (running())
        return;
    // Instrumentation must be attached before start_at(), exactly as the
    // sibling net managers do it -- otherwise the first transitions of this
    // chart are invisible to TELAF_CHART_SPY.
    set_instrument(std::make_unique<chart::SpyInstrument>(name()));
    start_at(SetNotReady_St);
    bridge_.subscribe_event(
      topics::net_settings::subsys_ready_net_settings::ind,
      "net_settings.subsys_ready_net_settings.ind",
      [this](std::string_view t, const Envelope& e) { handleReadyInd_(t, e); }
    );
    bridge_.subscribe_event(
      topics::net_settings::wwan_connectivity_changed::ind,
      "net_settings.wwan_connectivity_changed.ind",
      [this](std::string_view t, const Envelope& e) { handleWwanInd_(t, e); }
    );
    bridge_.subscribe_event(
      topics::net_settings::dds_changed::ind,
      "net_settings.dds_changed.ind",
      [this](std::string_view t, const Envelope& e) { handleDdsInd_(t, e); }
    );
    bridge_.subscribe_event(
      topics::net_settings::ippt_config_sync::ind,
      "net_settings.ippt_config_sync.ind",
      [this](std::string_view t, const Envelope& e) { handleIpptSyncInd_(t, e); }
    );
    bridge_.subscribe_event(
      topics::net_settings::ip_config_sync::ind,
      "net_settings.ip_config_sync.ind",
      [this](std::string_view t, const Envelope& e) { handleIpConfigSyncInd_(t, e); }
    );
    conn_token_ = bridge_.subscribe_connectivity([this](bool operational) {
        post_fifo({ BridgeConnectivityChanged_Signal, std::make_shared<bool>(operational) });
    });
}

void
SimulaDataSettingsManager::unsubscribeFromBridge_()
{
    bridge_.unsubscribe_event(topics::net_settings::subsys_ready_net_settings::ind);
    bridge_.unsubscribe_event(topics::net_settings::wwan_connectivity_changed::ind);
    bridge_.unsubscribe_event(topics::net_settings::dds_changed::ind);
    bridge_.unsubscribe_event(topics::net_settings::ippt_config_sync::ind);
    bridge_.unsubscribe_event(topics::net_settings::ip_config_sync::ind);
    bridge_.unsubscribe_connectivity(conn_token_);
    bridge_.drain();
}

void
SimulaDataSettingsManager::handleReadyInd_(std::string_view, const Envelope& env)
{
    auto p = std::make_shared<IndPld>();
    p->env = env;
    post_fifo({ ReadinessEvt_Signal, p });
}
void
SimulaDataSettingsManager::handleWwanInd_(std::string_view, const Envelope& env)
{
    auto p = std::make_shared<IndPld>();
    p->env = env;
    post_fifo({ WwanConnectivityEvt_Signal, p });
}
void
SimulaDataSettingsManager::handleDdsInd_(std::string_view, const Envelope& env)
{
    auto p = std::make_shared<IndPld>();
    p->env = env;
    post_fifo({ DdsChangedEvt_Signal, p });
}
void
SimulaDataSettingsManager::handleIpptSyncInd_(std::string_view, const Envelope& env)
{
    auto p = std::make_shared<IndPld>();
    p->env = env;
    post_fifo({ IpptConfigSyncEvt_Signal, p });
}
void
SimulaDataSettingsManager::handleIpConfigSyncInd_(std::string_view, const Envelope& env)
{
    auto p = std::make_shared<IndPld>();
    p->env = env;
    post_fifo({ IpConfigSyncEvt_Signal, p });
}

void
SimulaDataSettingsManager::broadcastToListeners_(
  std::function<void(const std::shared_ptr<IDataSettingsListener>&)> invoke
)
{
    auto task = std::make_shared<common::simula::DispatchTask>();
    task->debug_tag = "DataSettingsManager::broadcastToListeners_";
    {
        std::lock_guard<std::mutex> lk(listeners_mutex_);
        for (auto it = listeners_.begin(); it != listeners_.end();)
        {
            if (auto listener = it->lock())
            {
                task->listeners.push_back(listener);
                ++it;
            }
            else
            {
                it = listeners_.erase(it);
            }
        }
    }
    if (task->listeners.empty())
        return;
    task->invoker = [invoke = std::move(invoke)](std::shared_ptr<void> raw) {
        invoke(std::static_pointer_cast<IDataSettingsListener>(raw));
    };
    common::simula::ListenerDispatchAO::instance().enqueue(std::move(task));
}
bool
SimulaDataSettingsManager::isReadyDerived_() const
{
    return const_cast<SimulaDataSettingsManager*>(this)->current_state() == SetReady_St;
}

bool
SimulaDataSettingsManager::isSubsystemReady() const
{
    return isReadyDerived_();
}

void
SimulaDataSettingsManager::publishStatus_(telux::common::ServiceStatus s)
{
    last_status_.store(s);
}

telux::common::ServiceStatus
SimulaDataSettingsManager::getServiceStatus()
{
    return last_status_.load();
}

// Callback-shaped APIs share this simple asynchronous send pattern.
telux::common::Status
SimulaDataSettingsManager::restoreFactorySettings(
  OperationType op,
  telux::common::ResponseCallback cb,
  bool reboot
)
{
    if (!isReadyDerived_())
        return telux::common::Status::NOTREADY;
    auto r = common::simula::makeRequestEnvelope(
      bridge_.currentPaId(),
      { { "opType", op == OperationType::DATA_REMOTE ? "DATA_REMOTE" : "DATA_LOCAL" },
        { "isRebootNeeded", reboot } }
    );
    bridge_.send_request(
      topics::net_settings::restore_factory_settings::req,
      "net_settings.restore_factory_settings.rsp",
      r,
      [cb](auto x) { complete(cb, std::move(x)); },
      kRpcTimeout
    );
    return telux::common::Status::SUCCESS;
}
telux::common::Status
SimulaDataSettingsManager::setBackhaulPreference(
  std::vector<BackhaulType> p,
  telux::common::ResponseCallback cb
)
{
    if (!isReadyDerived_())
        return telux::common::Status::NOTREADY;
    nlohmann::json a = nlohmann::json::array();
    for (auto x : p)
        a.push_back(fromBackhaul(x));
    auto r = common::simula::makeRequestEnvelope(bridge_.currentPaId(), { { "backhaulPref", a } });
    bridge_.send_request(
      topics::net_settings::set_backhaul_pref::req,
      "net_settings.set_backhaul_pref.rsp",
      r,
      [cb](auto x) { complete(cb, std::move(x)); },
      kRpcTimeout
    );
    return telux::common::Status::SUCCESS;
}
telux::common::Status
SimulaDataSettingsManager::requestBackhaulPreference(RequestBackhaulPrefResponseCb cb)
{
    if (!isReadyDerived_())
        return telux::common::Status::NOTREADY;
    auto r = common::simula::makeRequestEnvelope(bridge_.currentPaId(), nlohmann::json::object());
    bridge_.send_request(
      topics::net_settings::request_backhaul_pref::req,
      "net_settings.request_backhaul_pref.rsp",
      r,
      // Concrete std::optional<Envelope> parameter rather than `auto`: inside a
      // generic lambda every `x->...` expression is type-dependent, so the
      // member template call below would need `v.template get<std::string>()`
      // to parse. Spelling the RpcCallback parameter type (as
      // publishMirrorWrite_ already does) removes the dependency instead of
      // papering over it with `template`.
      [cb](std::optional<Envelope> x) {
          std::vector<BackhaulType> o;
          if (!x || x->error || !x->data)
          {
              if (cb)
                  cb(
                    o,
                    !x ? telux::common::ErrorCode::OPERATION_TIMEOUT
                       : telux::common::ErrorCode::GENERIC_FAILURE
                  );
              return;
          }
          // is_string() guard: a non-string element would make get<std::string>()
          // throw type_error inside a bridge callback that swallows exceptions,
          // dropping `cb` entirely and leaving the caller waiting forever. An
          // unexpected element decodes to UNKNOWN via toBackhaul instead.
          for (const auto& v : x->data->value("backhaulPref", nlohmann::json::array()))
              o.push_back(toBackhaul(v.is_string() ? v.get<std::string>() : std::string()));
          if (cb)
              cb(o, telux::common::ErrorCode::SUCCESS);
      },
      kRpcTimeout
    );
    return telux::common::Status::SUCCESS;
}

// The remaining callback APIs are intentionally delegated to their exact wire RPCs.
telux::common::Status
SimulaDataSettingsManager::setBandInterferenceConfig(
  bool e,
  std::shared_ptr<BandInterferenceConfig> c,
  telux::common::ResponseCallback cb
)
{
    if (!isReadyDerived_())
        return telux::common::Status::NOTREADY;
    nlohmann::json d = { { "enable", e } };
    if (c)
        d["config"] = { { "priority", c->priority == BandPriority::WLAN ? "WLAN" : "N79" },
                        { "wlanWaitTimeInSec", c->wlanWaitTimeInSec },
                        { "n79WaitTimeInSec", c->n79WaitTimeInSec } };
    auto r = common::simula::makeRequestEnvelope(bridge_.currentPaId(), d);
    bridge_.send_request(
      topics::net_settings::set_band_interference::req,
      "net_settings.set_band_interference.rsp",
      r,
      [cb](auto x) { complete(cb, std::move(x)); },
      kRpcTimeout
    );
    return telux::common::Status::SUCCESS;
}
telux::common::Status
SimulaDataSettingsManager::requestBandInterferenceConfig(RequestBandInterferenceConfigResponseCb cb)
{
    if (!isReadyDerived_())
        return telux::common::Status::NOTREADY;
    auto r = common::simula::makeRequestEnvelope(bridge_.currentPaId(), nlohmann::json::object());
    bridge_.send_request(
      topics::net_settings::request_band_interference::req,
      "net_settings.request_band_interference.rsp",
      r,
      [cb](auto x) {
          if (!cb)
              return;
          if (!x || x->error || !x->data)
          {
              cb(
                false,
                nullptr,
                !x ? telux::common::ErrorCode::OPERATION_TIMEOUT
                   : telux::common::ErrorCode::GENERIC_FAILURE
              );
              return;
          }
          bool e = x->data->value("isEnabled", false);
          std::shared_ptr<BandInterferenceConfig> c;
          if (e && x->data->contains("config"))
          {
              auto q = (*x->data)["config"];
              c = std::make_shared<BandInterferenceConfig>();
              c->priority = q.value("priority", std::string()) == "WLAN" ? BandPriority::WLAN
                                                                         : BandPriority::N79;
              c->wlanWaitTimeInSec = q.value("wlanWaitTimeInSec", 30u);
              c->n79WaitTimeInSec = q.value("n79WaitTimeInSec", 30u);
          }
          cb(e, c, telux::common::ErrorCode::SUCCESS);
      },
      kRpcTimeout
    );
    return telux::common::Status::SUCCESS;
}
telux::common::Status
SimulaDataSettingsManager::setWwanConnectivityConfig(
  SlotId s,
  bool a,
  telux::common::ResponseCallback cb
)
{
    if (!isReadyDerived_())
        return telux::common::Status::NOTREADY;
    auto r =
      common::simula::makeRequestEnvelope(bridge_.currentPaId(), { { "slot", s }, { "allow", a } });
    bridge_.send_request(
      topics::net_settings::set_wwan_connectivity::req,
      "net_settings.set_wwan_connectivity.rsp",
      r,
      [cb](auto x) { complete(cb, std::move(x)); },
      kRpcTimeout
    );
    return telux::common::Status::SUCCESS;
}
telux::common::Status
SimulaDataSettingsManager::requestWwanConnectivityConfig(
  SlotId s,
  requestWwanConnectivityConfigResponseCb cb
)
{
    if (!isReadyDerived_())
        return telux::common::Status::NOTREADY;
    auto r = common::simula::makeRequestEnvelope(bridge_.currentPaId(), { { "slot", s } });
    bridge_.send_request(
      topics::net_settings::request_wwan_connectivity::req,
      "net_settings.request_wwan_connectivity.rsp",
      r,
      [cb, s](auto x) {
          if (!cb)
              return;
          if (!x || x->error || !x->data)
          {
              cb(
                s,
                false,
                !x ? telux::common::ErrorCode::OPERATION_TIMEOUT
                   : telux::common::ErrorCode::GENERIC_FAILURE
              );
              return;
          }
          cb(
            static_cast<SlotId>(x->data->value("slot", int(s))),
            x->data->value("isAllowed", false),
            telux::common::ErrorCode::SUCCESS
          );
      },
      kRpcTimeout
    );
    return telux::common::Status::SUCCESS;
}
bool
SimulaDataSettingsManager::isDeviceDataUsageMonitoringEnabled()
{
    return false;
}
telux::common::Status
SimulaDataSettingsManager::setMacSecState(bool e, telux::common::ResponseCallback cb)
{
    if (!isReadyDerived_())
        return telux::common::Status::NOTREADY;
    auto r = common::simula::makeRequestEnvelope(bridge_.currentPaId(), { { "enable", e } });
    bridge_.send_request(
      topics::net_settings::set_macsec_state::req,
      "net_settings.set_macsec_state.rsp",
      r,
      [cb](auto x) { complete(cb, std::move(x)); },
      kRpcTimeout
    );
    return telux::common::Status::SUCCESS;
}
telux::common::Status
SimulaDataSettingsManager::requestMacSecState(RequestMacSecSateResponseCb cb)
{
    if (!isReadyDerived_())
        return telux::common::Status::NOTREADY;
    auto r = common::simula::makeRequestEnvelope(bridge_.currentPaId(), nlohmann::json::object());
    bridge_.send_request(
      topics::net_settings::request_macsec_state::req,
      "net_settings.request_macsec_state.rsp",
      r,
      [cb](auto x) {
          if (!cb)
              return;
          if (!x || x->error || !x->data)
          {
              cb(
                false,
                !x ? telux::common::ErrorCode::OPERATION_TIMEOUT
                   : telux::common::ErrorCode::GENERIC_FAILURE
              );
              return;
          }
          cb(x->data->value("enabled", false), telux::common::ErrorCode::SUCCESS);
      },
      kRpcTimeout
    );
    return telux::common::Status::SUCCESS;
}
telux::common::Status
SimulaDataSettingsManager::switchBackHaul(
  BackhaulInfo s,
  BackhaulInfo d,
  bool all,
  telux::common::ResponseCallback cb
)
{
    if (!isReadyDerived_())
        return telux::common::Status::NOTREADY;
    auto r = common::simula::makeRequestEnvelope(
      bridge_.currentPaId(),
      { { "source", fromBackhaulInfo(s) }, { "dest", fromBackhaulInfo(d) }, { "applyToAll", all } }
    );
    bridge_.send_request(
      topics::net_settings::switch_backhaul::req,
      "net_settings.switch_backhaul.rsp",
      r,
      [cb](auto x) { complete(cb, std::move(x)); },
      kRpcTimeout
    );
    return telux::common::Status::SUCCESS;
}

// ---------------------------------------------------------------------------
// The six synchronous ErrorCode APIs.
//
// Reads answer from the mirror, which holds ONLY settled MPSS-derived state:
// every entry in it arrived on a retained ippt_config_sync / ip_config_sync
// indication. Writes never touch the mirror -- not even optimistically. That is
// invariant (d): if the PA seeded its own cache on the way out, a write MPSS
// rejected/dropped would leave the PA answering later getIp*() calls from a
// value the modem never accepted, i.e. the cache would have become an
// authoritative source. Instead the write is a pure request; the mirror only
// moves when MPSS republishes its retained truth, so a rejected write simply
// leaves the last known-good value in place.
//
// Because the SDK signature must return before the round-trip completes, the
// response cannot be reported to the caller. It is not discarded either: the
// completion is logged, so a silently-rejected write is diagnosable from the PA
// log rather than presenting as an unexplained "my setting didn't stick".
telux::common::ErrorCode
SimulaDataSettingsManager::publishMirrorWrite_(
  std::string_view t,
  std::string_view schema,
  nlohmann::json d
)
{
    if (!isReadyDerived_())
        return telux::common::ErrorCode::DEVICE_NOT_READY;
    auto r = common::simula::makeRequestEnvelope(bridge_.currentPaId(), std::move(d));
    const std::string topic_for_log(t);
    bridge_.send_request(
      t,
      schema,
      r,
      [topic_for_log](std::optional<Envelope> rsp) {
          if (!rsp)
          {
              LOG_WARN(
                "[DataSettingsManager] sync write %s timed out -- mirror keeps "
                "last MPSS-published value",
                topic_for_log.c_str()
              );
          }
          else if (rsp->error)
          {
              LOG_WARN(
                "[DataSettingsManager] sync write %s rejected by MPSS (%s) -- mirror "
                "keeps last MPSS-published value",
                topic_for_log.c_str(),
                rsp->error->value("code", std::string("?")).c_str()
              );
          }
          // On success MPSS republishes the retained *_sync indication, which is
          // what actually advances the mirror -- nothing to do here.
      },
      kRpcTimeout
    );
    return telux::common::ErrorCode::SUCCESS;
}
telux::common::ErrorCode
SimulaDataSettingsManager::setIpPassThroughConfig(const IpptParams& p, const IpptConfig& c)
{
    return publishMirrorWrite_(
      topics::net_settings::set_ippt_config::req,
      "net_settings.set_ippt_config.rsp",
      fromIppt(p, c)
    );
}
telux::common::ErrorCode
SimulaDataSettingsManager::setIpPassThroughNatConfig(bool e)
{
    return publishMirrorWrite_(
      topics::net_settings::set_ippt_nat_config::req,
      "net_settings.set_ippt_nat_config.rsp",
      { { "enableNat", e } }
    );
}
telux::common::ErrorCode
SimulaDataSettingsManager::getIpPassThroughNatConfig(bool& e)
{
    if (!isReadyDerived_())
        return telux::common::ErrorCode::DEVICE_NOT_READY;
    std::lock_guard<std::mutex> lk(mirror_mutex_);
    e = ippt_nat_enabled_;
    return telux::common::ErrorCode::SUCCESS;
}
telux::common::ErrorCode
SimulaDataSettingsManager::getIpPassThroughConfig(const IpptParams& p, IpptConfig& c)
{
    if (!isReadyDerived_())
        return telux::common::ErrorCode::DEVICE_NOT_READY;
    std::lock_guard<std::mutex> lk(mirror_mutex_);
    auto i = ippt_mirror_.find({ p.profileId, p.vlanId, int(p.slotId) });
    if (i == ippt_mirror_.end())
        return telux::common::ErrorCode::INVALID_ARGUMENTS;
    c = i->second;
    return telux::common::ErrorCode::SUCCESS;
}
// Rejects IPV4V6 locally because the SDK documents setIpConfig as per-family;
// accepting it here would hide a client bug that fails on real hardware.
telux::common::ErrorCode
SimulaDataSettingsManager::setIpConfig(const IpConfigParams& p, const IpConfig& c)
{
    if (p.ipFamilyType == IpFamilyType::IPV4V6)
        return telux::common::ErrorCode::REQUEST_NOT_SUPPORTED;
    return publishMirrorWrite_(
      topics::net_settings::set_ip_config::req,
      "net_settings.set_ip_config.rsp",
      fromIpConfig(p, c)
    );
}
telux::common::ErrorCode
SimulaDataSettingsManager::getIpConfig(const IpConfigParams& p, IpConfig& c)
{
    if (!isReadyDerived_())
        return telux::common::ErrorCode::DEVICE_NOT_READY;
    std::lock_guard<std::mutex> lk(mirror_mutex_);
    auto i = ip_config_mirror_.find({ int(p.ifType), int(p.ipFamilyType), int(p.vlanId) });
    if (i == ip_config_mirror_.end())
        return telux::common::ErrorCode::INVALID_ARGUMENTS;
    c = i->second;
    return telux::common::ErrorCode::SUCCESS;
}

// ---------------------------------------------------------------------------
// Mirror replacement. Called only from the chart (AO thread) on a retained
// *_sync indication. Whole-snapshot replace, not merge: the indication carries
// MPSS's complete table, so merging would resurrect entries MPSS has deleted.
// Shared by NotReady and Ready so the two paths cannot drift apart.
void
SimulaDataSettingsManager::applyIpptSync_(const nlohmann::json& data)
{
    std::lock_guard<std::mutex> lk(mirror_mutex_);
    ippt_nat_enabled_ = data.value("natEnabled", true);
    ippt_mirror_.clear();
    for (const auto& entry : data.value("entries", nlohmann::json::array()))
        ippt_mirror_[{ entry.value("profileId", -1),
                       entry.value("vlanId", -1),
                       entry.value("slot", 1) }] = toIppt(entry);
}

void
SimulaDataSettingsManager::applyIpConfigSync_(const nlohmann::json& data)
{
    std::lock_guard<std::mutex> lk(mirror_mutex_);
    ip_config_mirror_.clear();
    for (const auto& entry : data.value("entries", nlohmann::json::array()))
        ip_config_mirror_[{ static_cast<int>(toIfaceType(entry.value("ifType", std::string()))),
                            static_cast<int>(toIpFamily(entry.value("ipFamily", std::string()))),
                            entry.value("vlanId", -1) }] = toIpConfig(entry);
}

telux::common::Status
SimulaDataSettingsManager::registerListener(std::weak_ptr<IDataSettingsListener> l)
{
    std::lock_guard<std::mutex> lk(listeners_mutex_);
    listeners_.push_back(std::move(l));
    return telux::common::Status::SUCCESS;
}
telux::common::Status
SimulaDataSettingsManager::deregisterListener(std::weak_ptr<IDataSettingsListener> l)
{
    std::lock_guard<std::mutex> lk(listeners_mutex_);
    auto target = l.lock();
    listeners_.erase(
      std::remove_if(
        listeners_.begin(),
        listeners_.end(),
        [&](auto& w) {
            auto p = w.lock();
            return !p || (target && p == target);
        }
      ),
      listeners_.end()
    );
    return telux::common::Status::SUCCESS;
}
telux::common::Status
SimulaDataSettingsManager::requestDdsSwitch(DdsInfo d, telux::common::ResponseCallback cb)
{
    if (!isReadyDerived_())
        return telux::common::Status::NOTREADY;
    auto r = common::simula::makeRequestEnvelope(
      bridge_.currentPaId(),
      { { "slot", d.slotId }, { "ddsType", fromDds(d.type) } }
    );
    bridge_.send_request(
      topics::net_settings::request_dds_switch::req,
      "net_settings.request_dds_switch.rsp",
      r,
      [cb](auto x) { complete(cb, std::move(x)); },
      kRpcTimeout
    );
    return telux::common::Status::SUCCESS;
}
telux::common::Status
SimulaDataSettingsManager::requestCurrentDds(RequestCurrentDdsResponseCb cb)
{
    if (!isReadyDerived_())
        return telux::common::Status::NOTREADY;
    auto r = common::simula::makeRequestEnvelope(bridge_.currentPaId(), nlohmann::json::object());
    bridge_.send_request(
      topics::net_settings::request_current_dds::req,
      "net_settings.request_current_dds.rsp",
      r,
      [cb](auto x) {
          if (!cb)
              return;
          if (!x || x->error || !x->data)
          {
              cb(
                {},
                !x ? telux::common::ErrorCode::OPERATION_TIMEOUT
                   : telux::common::ErrorCode::GENERIC_FAILURE
              );
              return;
          }
          cb(
            { toDds(x->data->value("ddsType", std::string())),
              static_cast<SlotId>(x->data->value("slot", 1)) },
            telux::common::ErrorCode::SUCCESS
          );
      },
      kRpcTimeout
    );
    return telux::common::Status::SUCCESS;
}

// ---------------------------------------------------------------------------
// Explicit lifecycle chart. Bridge callbacks only post events; every state
// transition, retained-mirror update, and listener fan-out happens here on the
// manager AO thread.
chart::Status
SetOperating_St(chart::Hsm* h, chart::Event const* e)
{
    auto* self = static_cast<SimulaDataSettingsManager*>(h);
    switch (e->sig)
    {
        case chart::Entry_Signal:
        case chart::Exit_Signal:
            return chart::Status::HANDLED;
        default:
            return self->super(&chart::Hsm::top);
    }
}

chart::Status
SetNotReady_St(chart::Hsm* h, chart::Event const* e)
{
    auto* self = static_cast<SimulaDataSettingsManager*>(h);
    switch (e->sig)
    {
        case chart::Entry_Signal:
        case chart::Exit_Signal:
            return chart::Status::HANDLED;
        case ReadinessEvt_Signal:
        {
            auto p = common::simula::event_cast<IndPld>(*e);
            if (p->env.data && p->env.data->value("status", std::string()) == "AVAILABLE")
                return self->to(SetReady_St);
            return chart::Status::HANDLED;
        }
        case IpptConfigSyncEvt_Signal:
        {
            // Retained mirrors can arrive before the retained readiness
            // indication. They still seed the cache, but do not make the
            // manager callable until the explicit AVAILABLE transition.
            auto p = common::simula::event_cast<IndPld>(*e);
            if (!p->env.data)
                return chart::Status::HANDLED;
            self->applyIpptSync_(*p->env.data);
            return chart::Status::HANDLED;
        }
        case IpConfigSyncEvt_Signal:
        {
            auto p = common::simula::event_cast<IndPld>(*e);
            if (!p->env.data)
                return chart::Status::HANDLED;
            self->applyIpConfigSync_(*p->env.data);
            return chart::Status::HANDLED;
        }
        case BridgeConnectivityChanged_Signal:
        case WwanConnectivityEvt_Signal:
        case DdsChangedEvt_Signal:
            return chart::Status::HANDLED;
        default:
            return self->super(SetOperating_St);
    }
}

chart::Status
SetReady_St(chart::Hsm* h, chart::Event const* e)
{
    auto* self = static_cast<SimulaDataSettingsManager*>(h);
    switch (e->sig)
    {
        case chart::Entry_Signal:
            self->publishStatus_(telux::common::ServiceStatus::SERVICE_AVAILABLE);
            if (!self->init_gate_.markReadyAndFire(
                  telux::common::ServiceStatus::SERVICE_AVAILABLE))
            {
                self->broadcastToListeners_([](const std::shared_ptr<IDataSettingsListener>& l) {
                    l->onServiceStatusChange(telux::common::ServiceStatus::SERVICE_AVAILABLE);
                });
            }
            return chart::Status::HANDLED;
        case chart::Exit_Signal:
            self->broadcastToListeners_([](const std::shared_ptr<IDataSettingsListener>& l) {
                l->onServiceStatusChange(telux::common::ServiceStatus::SERVICE_UNAVAILABLE);
            });
            return chart::Status::HANDLED;
        case ReadinessEvt_Signal:
        {
            auto p = common::simula::event_cast<IndPld>(*e);
            if (!p->env.data)
                return chart::Status::HANDLED;
            const auto status = p->env.data->value("status", std::string());
            if (status == "UNAVAILABLE" || status == "FAILED")
            {
                self->publishStatus_(
                  status == "FAILED" ? telux::common::ServiceStatus::SERVICE_FAILED
                                     : telux::common::ServiceStatus::SERVICE_UNAVAILABLE
                );
                return self->to(SetNotReady_St);
            }
            return chart::Status::HANDLED;
        }
        case BridgeConnectivityChanged_Signal:
        {
            auto operational = common::simula::event_cast<bool>(*e);
            if (operational && !*operational)
            {
                self->publishStatus_(telux::common::ServiceStatus::SERVICE_UNAVAILABLE);
                return self->to(SetNotReady_St);
            }
            return chart::Status::HANDLED;
        }
        case WwanConnectivityEvt_Signal:
        {
            auto p = common::simula::event_cast<IndPld>(*e);
            if (!p->env.data)
                return chart::Status::HANDLED;
            const auto& d = *p->env.data;
            const auto slot = static_cast<SlotId>(d.value("slot", 1));
            const bool allowed = d.value("isConnectivityAllowed", true);
            self->broadcastToListeners_([slot,
                                         allowed](const std::shared_ptr<IDataSettingsListener>& l) {
                l->onWwanConnectivityConfigChange(slot, allowed);
            });
            return chart::Status::HANDLED;
        }
        case DdsChangedEvt_Signal:
        {
            auto p = common::simula::event_cast<IndPld>(*e);
            if (!p->env.data)
                return chart::Status::HANDLED;
            const auto& d = *p->env.data;
            const DdsInfo info{ toDds(d.value("ddsType", std::string())),
                                static_cast<SlotId>(d.value("slot", 1)) };
            self->broadcastToListeners_([info](const std::shared_ptr<IDataSettingsListener>& l) {
                l->onDdsChange(info);
            });
            return chart::Status::HANDLED;
        }
        case IpptConfigSyncEvt_Signal:
        {
            auto p = common::simula::event_cast<IndPld>(*e);
            if (!p->env.data)
                return chart::Status::HANDLED;
            self->applyIpptSync_(*p->env.data);
            return chart::Status::HANDLED;
        }
        case IpConfigSyncEvt_Signal:
        {
            auto p = common::simula::event_cast<IndPld>(*e);
            if (!p->env.data)
                return chart::Status::HANDLED;
            self->applyIpConfigSync_(*p->env.data);
            return chart::Status::HANDLED;
        }
        default:
            return self->super(SetOperating_St);
    }
}

CHART_NAMED_STATE(SetNotReady_St, "DataSettingsManager::NotReady");
CHART_NAMED_STATE(SetReady_St, "DataSettingsManager::Ready");
CHART_NAMED_STATE(SetOperating_St, "DataSettingsManager::Operating");

}  // namespace telux::data::simula
