// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "DataConnectionManager.hpp"

#include "../common/EventCast.hpp"
#include "../common/ListenerDispatchAO.hpp"
#include "Signals.hpp"
#include "generated/cpp/topics.h"

#include <arpa/inet.h>

#include <atomic>
#include <chart/event.hpp>
#include <chart/hsm.hpp>
#include <chart/spy.hpp>
#include <future>

namespace telux::data::simula {

using namespace DataSignals;

namespace {

using common::simula::Envelope;
using common::simula::event_cast;

// Connection bringup: default to dual-stack so MPSS allocates both address
// families unless the caller explicitly constrains to v4-only or v6-only.
std::string
ipFamilyToWire(telux::data::IpFamilyType f)
{
    switch (f)
    {
        case telux::data::IpFamilyType::IPV4:
            return "IPV4";
        case telux::data::IpFamilyType::IPV6:
            return "IPV6";
        default:
            return "IPV4V6";
    }
}

std::string
opTypeToWire(telux::data::OperationType t)
{
    return t == telux::data::OperationType::DATA_REMOTE ? "DATA_REMOTE" : "DATA_LOCAL";
}

// Wire encoding for the eight-way EndReasonType tagged union: only the two
// sub-codes MPSS itself produces are decoded (CE_INTERNAL/NW_INITIATED_
// TERMINATION on drop, CE_3GPP_SPEC_DEFINED/REGULAR_DEACTIVATION on clean
// stop) -- every other tagged-union member is unreachable from this wire
// contract and stays at the struct's
// documented default {type: CE_UNKNOWN} unless a future payload needs it.
telux::data::DataCallStatus
wireToCallStatus(const std::string& s)
{
    if (s == "CONNECTED")
        return telux::data::DataCallStatus::NET_CONNECTED;
    if (s == "NO_NET")
        return telux::data::DataCallStatus::NET_NO_NET;
    if (s == "CONNECTING")
        return telux::data::DataCallStatus::NET_CONNECTING;
    if (s == "DISCONNECTING")
        return telux::data::DataCallStatus::NET_DISCONNECTING;
    if (s == "RECONFIGURED")
        return telux::data::DataCallStatus::NET_RECONFIGURED;
    if (s == "NEWADDR")
        return telux::data::DataCallStatus::NET_NEWADDR;
    if (s == "DELADDR")
        return telux::data::DataCallStatus::NET_DELADDR;
    return telux::data::DataCallStatus::NET_IDLE;
}

// Mirrors MPSS's RAT_TO_BEARER_TECH wire strings (connection.py) into
// telux::data::DataBearerTechnology. Unrecognized/absent -> UNKNOWN.
telux::data::DataBearerTechnology
wireToBearerTech(const std::string& s)
{
    if (s == "GSM")            return telux::data::DataBearerTechnology::GSM;
    if (s == "WCDMA")          return telux::data::DataBearerTechnology::WCDMA;
    if (s == "LTE")            return telux::data::DataBearerTechnology::LTE;
    if (s == "BEARER_TECH_5G") return telux::data::DataBearerTechnology::BEARER_TECH_5G;
    if (s == "CDMA_1X")        return telux::data::DataBearerTechnology::CDMA_1X;
    if (s == "EVDO_REV0")      return telux::data::DataBearerTechnology::EVDO_REV0;
    return telux::data::DataBearerTechnology::UNKNOWN;
}

// Decodes the optional `end_reason: {type, code}` object. Pinned values:
// CE_3GPP_SPEC_DEFINED/REGULAR_DEACTIVATION=clean stop,
// CE_INTERNAL/NW_INITIATED_TERMINATION=network drop. `type` is a plain
// int-to-enum cast (EndReasonType's underlying values ARE the wire ints);
// only the two sub-code members MPSS can actually populate are decoded --
// any other type value leaves the union's active member unset (harmless,
// since a client must only read the member matching `type`).
telux::common::DataCallEndReason
decodeEndReason(const nlohmann::json& obj)
{
    telux::common::DataCallEndReason reason{};
    auto type = static_cast<telux::common::EndReasonType>(obj.value("type", 0xFF));
    reason.type = type;
    int code = obj.value("code", 0);
    if (type == telux::common::EndReasonType::CE_3GPP_SPEC_DEFINED)
        reason.specCode = static_cast<telux::common::SpecReasonCode>(code);
    else if (type == telux::common::EndReasonType::CE_INTERNAL)
        reason.internalCode = static_cast<telux::common::InternalReasonCode>(code);
    return reason;
}

// Converts a dotted-quad IPv4 subnet mask string ("255.255.255.0") into the
// network-byte-order unsigned int IpAddrInfo::ifMask expects. Returns 0 (its
// documented default) if the string isn't a valid IPv4 address.
unsigned int
subnetMaskToIfMask(const std::string& s)
{
    in_addr addr{};
    if (inet_pton(AF_INET, s.c_str(), &addr) != 1)
        return 0;
    return static_cast<unsigned int>(addr.s_addr);
}

// Applies the optional `ipv4`/`ipv6` address blocks from a call_state
// indication payload. Field names are snake_case per the payload-schema
// convention.
//
// IpAddrInfo has a single `unsigned int ifMask` field with no v6-specific
// equivalent (telux/data/DataDefines.hpp) -- ipv6's `prefix_len` (an integer
// like 64, not a mask string) has no natural encoding into that field, so
// it is intentionally not applied; ifMask stays at its documented default
// (0) for the v6 IpAddrInfo. Only ipv4's `subnet_mask` populates ifMask.
void
applyAddrInfo(SimulaDataCall& call, const nlohmann::json& data, telux::data::DataCallStatus status)
{
    if (data.contains("ipv4"))
    {
        const auto& a = data["ipv4"];
        telux::data::IpFamilyInfo info{};
        info.status = status;
        info.addr.ifAddress = a.value("if_address", std::string());
        info.addr.gwAddress = a.value("gw_address", std::string());
        info.addr.primaryDnsAddress = a.value("primary_dns_address", std::string());
        info.addr.secondaryDnsAddress = a.value("secondary_dns_address", std::string());
        if (a.contains("subnet_mask"))
            info.addr.ifMask = subnetMaskToIfMask(a.value("subnet_mask", std::string()));
        call.setIpv4(info);
    }
    if (data.contains("ipv6"))
    {
        const auto& a = data["ipv6"];
        telux::data::IpFamilyInfo info{};
        info.status = status;
        info.addr.ifAddress = a.value("if_address", std::string());
        info.addr.gwAddress = a.value("gw_address", std::string());
        info.addr.primaryDnsAddress = a.value("primary_dns_address", std::string());
        info.addr.secondaryDnsAddress = a.value("secondary_dns_address", std::string());
        call.setIpv6(info);
    }
}

struct RpcResultPld
{
    std::optional<Envelope> rsp;
};

struct StateIndPld
{
    Envelope env;
};

struct StopReqPld
{
    telux::data::DataCallResponseCb cb;
};

constexpr auto kRpcTimeout = std::chrono::seconds(30);
constexpr auto kBringupTimeout = std::chrono::seconds(30);
// TeardownTimeout isn't pinned (only BringupTimeout=30s is an explicit
// decision) -- reusing the same value for symmetry until a real requirement
// pins a different one.
constexpr auto kTeardownTimeout = std::chrono::seconds(30);

// Decodes the wire's `infos: [ {slot,profileId,ul{throughput,maxThroughput,
// queueSize},dl{throughput}} ]` (opaque array-of-object per codegen's
// simplification) into telux::data::ThroughputInfo. Shared by
// request_throughput_info's rsp and the throughput_info ind -- same shape.
std::vector<telux::data::ThroughputInfo>
decodeThroughputInfos(const nlohmann::json& infos)
{
    std::vector<telux::data::ThroughputInfo> out;
    for (const auto& item : infos)
    {
        telux::data::ThroughputInfo info{};
        info.slot = static_cast<SlotId>(item.value("slot", 0));
        info.profileId = item.value("profileId", 0);
        if (item.contains("ul"))
        {
            const auto& ul = item["ul"];
            info.ulThroughput.throughput = ul.value("throughput", 0);
            info.ulThroughput.maxThroughput = ul.value("maxThroughput", 0);
            info.ulThroughput.queueSize = ul.value("queueSize", 0);
        }
        if (item.contains("dl"))
            info.dlThroughput.throughput = item["dl"].value("throughput", 0);
        out.push_back(info);
    }
    return out;
}

// Decodes the wire's flat throttle-status shape (`{apn, profileIds, ipv4Time,
// ipv6Time, isBlocked, mcc, mnc}`) into telux::data::APNThrottleInfo. Shared
// by request_throttle_status's rsp and the throttle_status ind -- both carry
// this one shape 1:1 (one profile/APN's throttle state per message).
telux::data::APNThrottleInfo
decodeThrottleInfo(const nlohmann::json& data)
{
    telux::data::APNThrottleInfo info{};
    info.apn = data.value("apn", std::string());
    if (data.contains("profileIds"))
    {
        for (const auto& pid : data["profileIds"])
            info.profileIds.push_back(pid.get<int>());
    }
    info.ipv4Time = data.value("ipv4Time", 0);
    info.ipv6Time = data.value("ipv6Time", 0);
    info.isBlocked = data.value("isBlocked", false);
    info.mcc = data.value("mcc", std::string());
    info.mnc = data.value("mnc", std::string());
    return info;
}

telux::data::QosFlowStateChangeEvent
wireToQosStateChange(const std::string& s)
{
    if (s == "ACTIVATED")   return telux::data::QosFlowStateChangeEvent::ACTIVATED;
    if (s == "MODIFIED")    return telux::data::QosFlowStateChangeEvent::MODIFIED;
    if (s == "DEACTIVATED") return telux::data::QosFlowStateChangeEvent::DELETED;
    return telux::data::QosFlowStateChangeEvent::UNKNOWN;
}

// Decodes the wire's qos_status shape into a TrafficFlowTemplate. Only
// qosId/stateChange/mask and (when present) txGranted/rxGranted.dataRate are
// populated -- the wire carries no filter data, so txFilters/rxFilters stay
// empty (*FiltersLength = 0), matching IDataConnectionListener::
// onTrafficFlowTemplateChange's real contract when the modem reports no
// filters for a flow.
std::shared_ptr<telux::data::TrafficFlowTemplate>
decodeTrafficFlowTemplate(const nlohmann::json& data)
{
    auto tft = std::make_shared<telux::data::TrafficFlowTemplate>();
    tft->qosId = data.value("qosId", 0);
    tft->stateChange = wireToQosStateChange(data.value("stateChange", std::string()));
    tft->mask = telux::data::QosFlowMask(static_cast<unsigned long>(data.value("mask", 0)));
    tft->txFiltersLength = 0;
    tft->rxFiltersLength = 0;
    if (data.contains("txGranted"))
    {
        const auto& g = data["txGranted"];
        tft->txGrantedFlow.dataRate.maxRate = g.value("maxRate", 0);
        tft->txGrantedFlow.dataRate.minRate = g.value("minRate", 0);
    }
    if (data.contains("rxGranted"))
    {
        const auto& g = data["rxGranted"];
        tft->rxGrantedFlow.dataRate.maxRate = g.value("maxRate", 0);
        tft->rxGrantedFlow.dataRate.minRate = g.value("minRate", 0);
    }
    return tft;
}

}  // namespace

// ============================================================================
// SimulaDataCall
// ============================================================================

SimulaDataCall::SimulaDataCall(
  common::simula::IModemBridge& bridge,
  int profileId,
  SlotId slotId,
  telux::data::IpFamilyType ipFamilyType,
  telux::data::OperationType operationType,
  std::string interfaceName
)
    : bridge_(bridge)
    , profileId_(profileId)
    , slotId_(slotId)
    , ipFamilyType_(ipFamilyType)
    , operationType_(operationType)
    , interfaceName_(std::move(interfaceName))
{}

const std::string&
SimulaDataCall::getInterfaceName()
{
    std::lock_guard<std::mutex> lk(m_);
    return interfaceName_;
}

telux::data::DataCallEndReason
SimulaDataCall::getDataCallEndReason()
{
    std::lock_guard<std::mutex> lk(m_);
    return endReason_;
}

telux::data::DataCallStatus
SimulaDataCall::getDataCallStatus()
{
    std::lock_guard<std::mutex> lk(m_);
    return status_;
}

telux::data::IpFamilyInfo
SimulaDataCall::getIpv4Info()
{
    std::lock_guard<std::mutex> lk(m_);
    return ipv4_;
}

telux::data::IpFamilyInfo
SimulaDataCall::getIpv6Info()
{
    std::lock_guard<std::mutex> lk(m_);
    return ipv6_;
}

telux::data::TechPreference
SimulaDataCall::getTechPreference()
{
    std::lock_guard<std::mutex> lk(m_);
    return techPreference_;
}

std::list<telux::data::IpAddrInfo>
SimulaDataCall::getIpAddressInfo()
{
    std::lock_guard<std::mutex> lk(m_);
    std::list<telux::data::IpAddrInfo> out;
    if (ipFamilyType_ == telux::data::IpFamilyType::IPV4 ||
        ipFamilyType_ == telux::data::IpFamilyType::IPV4V6)
    {
        out.push_back(ipv4_.addr);
    }
    if (ipFamilyType_ == telux::data::IpFamilyType::IPV6 ||
        ipFamilyType_ == telux::data::IpFamilyType::IPV4V6)
    {
        out.push_back(ipv6_.addr);
    }
    return out;
}

telux::data::IpFamilyType
SimulaDataCall::getIpFamilyType()
{
    std::lock_guard<std::mutex> lk(m_);
    return ipFamilyType_;
}

int
SimulaDataCall::getProfileId()
{
    std::lock_guard<std::mutex> lk(m_);
    return profileId_;
}

SlotId
SimulaDataCall::getSlotId()
{
    std::lock_guard<std::mutex> lk(m_);
    return slotId_;
}

telux::data::OperationType
SimulaDataCall::getOperationType()
{
    std::lock_guard<std::mutex> lk(m_);
    return operationType_;
}

telux::common::Status
SimulaDataCall::requestTrafficFlowTemplate(
  telux::data::IpFamilyType /*ipFamilyType*/,
  telux::data::TrafficFlowTemplateCb /*callback*/
)
{
    // Not part of the wire-schema scope (only start/stop/list_data_call +
    // query_profile are covered). Real SDK convention per
    // DataCallResponseCb's doc comment: on failure, the
    // callback is never invoked.
    return telux::common::Status::NOTSUPPORTED;
}

telux::common::Status
SimulaDataCall::requestDataCallStatistics(telux::data::StatisticsResponseCb /*callback*/)
{
    return telux::common::Status::NOTSUPPORTED;
}

telux::common::Status
SimulaDataCall::resetDataCallStatistics(telux::common::ResponseCallback /*callback*/)
{
    return telux::common::Status::NOTSUPPORTED;
}

telux::common::Status
SimulaDataCall::requestDataCallBitRate(telux::data::requestDataCallBitRateResponseCb callback)
{
    // Bitrate is a live query tied to the call's *current* RAT -- gate on
    // NET_CONNECTED rather than dispatching a doomed RPC MPSS would reject
    // anyway.
    if (getDataCallStatus() != telux::data::DataCallStatus::NET_CONNECTED)
    {
        if (callback)
        {
            telux::data::BitRateInfo info{};
            callback(info, telux::common::ErrorCode::INVALID_STATE);
        }
        return telux::common::Status::SUCCESS;
    }
    // IModemBridge::send_request is documented thread-safe from any thread
    // (IModemBridge.hpp), and this call's fields used below are immutable
    // after construction -- no need to hop onto the owning Session's AO
    // thread just to dispatch this RPC.
    nlohmann::json data = nlohmann::json::object();
    data["profileId"] = getProfileId();
    auto req = common::simula::makeRequestEnvelope(bridge_.currentPaId(), std::move(data));
    bridge_.send_request(
      topics::data::request_data_call_bitrate::req,
      "data.request_data_call_bitrate.rsp",
      req,
      [callback](std::optional<Envelope> rsp) {
          if (!callback)
              return;
          telux::data::BitRateInfo info{};
          if (!rsp || rsp->error || !rsp->data)
          {
              callback(info, telux::common::ErrorCode::OPERATION_TIMEOUT);
              return;
          }
          info.maxTxRate = rsp->data->value("maxTxRate", 0);
          info.maxRxRate = rsp->data->value("maxRxRate", 0);
          callback(info, telux::common::ErrorCode::SUCCESS);
      },
      kRpcTimeout
    );
    return telux::common::Status::SUCCESS;
}

telux::data::DataBearerTechnology
SimulaDataCall::getCurrentBearerTech()
{
    std::lock_guard<std::mutex> lk(m_);
    return bearerTech_;
}

void
SimulaDataCall::setStatus(telux::data::DataCallStatus status)
{
    std::lock_guard<std::mutex> lk(m_);
    status_ = status;
    // A teardown transition applies to every IP family. The real SDK reports a
    // per-family IpFamilyInfo.status alongside the overall status, and the DCS
    // layer notifies its clients *per family, only on a per-family change*
    // (tafDcsProfileManagerImpl.cpp paSessionStateChangeEvtHandler:
    // `if (curIpv4State != ipv4ConnState) ...`). On connect the per-family
    // statuses are set to NET_CONNECTED by applyAddrInfo, but the teardown
    // paths only ever set this overall status_ -- leaving ipv4_/ipv6_ stuck at
    // NET_CONNECTED, so DCS sees no per-family change and the client's
    // SessionState listener never observes DISCONNECTED. Cascade the going-down
    // states into the per-family statuses (addresses left intact) so the
    // transition is visible per family.
    if (status == telux::data::DataCallStatus::NET_DISCONNECTING ||
        status == telux::data::DataCallStatus::NET_NO_NET)
    {
        ipv4_.status = status;
        ipv6_.status = status;
    }
    // Mirrors MPSS's own bearer_tech clearing on teardown (connection.py's
    // CallSession.disconnected()/force_drop() both set bearer_tech=None) --
    // getCurrentBearerTech() must not keep reporting the last-known RAT
    // after the call is gone.
    if (status == telux::data::DataCallStatus::NET_NO_NET)
        bearerTech_ = telux::data::DataBearerTechnology::UNKNOWN;
}

void
SimulaDataCall::setEndReason(telux::common::DataCallEndReason reason)
{
    std::lock_guard<std::mutex> lk(m_);
    endReason_ = reason;
}

void
SimulaDataCall::setIpv4(telux::data::IpFamilyInfo info)
{
    std::lock_guard<std::mutex> lk(m_);
    ipv4_ = std::move(info);
}

void
SimulaDataCall::setIpv6(telux::data::IpFamilyInfo info)
{
    std::lock_guard<std::mutex> lk(m_);
    ipv6_ = std::move(info);
}

void
SimulaDataCall::setInterfaceName(std::string name)
{
    std::lock_guard<std::mutex> lk(m_);
    interfaceName_ = std::move(name);
}

void
SimulaDataCall::setTechPreference(telux::data::TechPreference tech)
{
    std::lock_guard<std::mutex> lk(m_);
    techPreference_ = tech;
}

void
SimulaDataCall::setBearerTech(telux::data::DataBearerTechnology bearer)
{
    std::lock_guard<std::mutex> lk(m_);
    bearerTech_ = bearer;
}

// ============================================================================
// SimulaDataCallSession
// ============================================================================
//
// State machine for DataCallSession. Chart-style: one free function per
// state, switch-on-signal, default branch reports super-state via
// self->super(...).

chart::Status
Idle_St(chart::Hsm*, chart::Event const*);
chart::Status
Active_St(chart::Hsm*, chart::Event const*);
chart::Status
SendingStartReq_St(chart::Hsm*, chart::Event const*);
chart::Status
WaitingForUp_St(chart::Hsm*, chart::Event const*);
chart::Status
Connected_St(chart::Hsm*, chart::Event const*);
chart::Status
SendingStopReq_St(chart::Hsm*, chart::Event const*);
chart::Status
WaitingForDown_St(chart::Hsm*, chart::Event const*);
chart::Status
Reconciling_St(chart::Hsm*, chart::Event const*);
chart::Status
Terminal_St(chart::Hsm*, chart::Event const*);

SimulaDataCallSession::SimulaDataCallSession(
  common::simula::IModemBridge& bridge,
  SlotId slotId,
  int profileId,
  telux::data::IpFamilyType ipFamilyType,
  telux::data::OperationType operationType,
  std::string interfaceName,
  std::function<void(std::shared_ptr<telux::data::IDataCall>)> notifyListeners,
  std::function<void(int profileId)> onFinished
)
    : chart::ActiveObject("DataCallSession")
    , bridge_(bridge)
    , slotId_(slotId)
    , profileId_(profileId)
    , operationType_(operationType)
    , call_(std::make_shared<SimulaDataCall>(
        bridge, profileId, slotId, ipFamilyType, operationType, std::move(interfaceName)
      ))
    , notifyListeners_(std::move(notifyListeners))
    , onFinished_(std::move(onFinished))
    , bringup_timeout_(this, BringupTimeout_Signal)
    , teardown_timeout_(this, TeardownTimeout_Signal)
{}

SimulaDataCallSession::~SimulaDataCallSession()
{
    stop();
}

void
SimulaDataCallSession::start(telux::data::DataCallResponseCb startCb)
{
    startCb_ = std::move(startCb);
    set_instrument(std::make_unique<chart::SpyInstrument>(name()));
    start_at(Idle_St);
    post_fifo({ Start_Signal, nullptr });
}

void
SimulaDataCallSession::requestStop(telux::data::DataCallResponseCb stopCb)
{
    auto pld = std::make_shared<StopReqPld>();
    pld->cb = std::move(stopCb);
    post_fifo({ Stop_Signal, pld });
}

void
SimulaDataCallSession::deliverStateInd(const common::simula::Envelope& env)
{
    auto pld = std::make_shared<StateIndPld>();
    pld->env = env;
    post_fifo({ StateInd_Signal, pld });
}

chart::Status
Idle_St(chart::Hsm* h, chart::Event const* e)
{
    auto* self = static_cast<SimulaDataCallSession*>(h);
    switch (e->sig)
    {
        case chart::Entry_Signal:
        case chart::Exit_Signal:
            return chart::Status::HANDLED;
        case Start_Signal:
            return self->to(SendingStartReq_St);
        default:
            return self->super(&chart::Hsm::top);
    }
}

// Composite parent for the six mid-lifecycle call states
// (SendingStartReq..Reconciling). Structural per D4 shape; child-specific
// Stop_Signal handling is retained (SendingStartReq defers, WaitingForUp
// and Connected consume-and-transition, tail states silently drop via
// IGNORED bubble), because moving Stop deferral wholesale to Active would
// leak a StopReqPld cb into deferred_ from states that can never recall
// it (SendingStopReq/WaitingForDown/Reconciling only transition forward).
chart::Status
Active_St(chart::Hsm* h, chart::Event const* e)
{
    auto* self = static_cast<SimulaDataCallSession*>(h);
    switch (e->sig)
    {
        case chart::Entry_Signal:
        case chart::Exit_Signal:
            return chart::Status::HANDLED;
        // Only the tail states (SendingStopReq / WaitingForDown / Reconciling)
        // bubble Stop_Signal here -- SendingStartReq defers it, WaitingForUp
        // and Connected consume-and-transition. A Stop landing while a prior
        // stop is already in flight is the stop-side analogue of the
        // Manager's OP_IN_PROGRESS check on startDataCall: fire the caller's
        // cb here rather than silently dropping it (a caller waiting on that
        // cb would otherwise hang forever, since no tail state ever recalls
        // the deferred queue).
        case Stop_Signal:
        {
            auto pld = event_cast<StopReqPld>(*e);
            if (pld && pld->cb)
                pld->cb(self->call_, telux::common::ErrorCode::OP_IN_PROGRESS);
            return chart::Status::HANDLED;
        }
        default:
            return self->super(&chart::Hsm::top);
    }
}

chart::Status
SendingStartReq_St(chart::Hsm* h, chart::Event const* e)
{
    auto* self = static_cast<SimulaDataCallSession*>(h);
    switch (e->sig)
    {
        case chart::Entry_Signal:
        {
            nlohmann::json data = nlohmann::json::object();
            data["profileId"] = self->profileId_;
            data["ipFamily"] = ipFamilyToWire(self->call_->getIpFamilyType());
            data["ifname"] = self->call_->getInterfaceName();
            data["opType"] = opTypeToWire(self->operationType_);
            data["slot"] = static_cast<int>(self->slotId_);

            auto req =
              common::simula::makeRequestEnvelope(self->bridge_.currentPaId(), std::move(data));
            self->bridge_.send_request(
              topics::data::start_data_call::req,
              "data.start_data_call.rsp",
              req,
              [weak = self->weak_from_this()](std::optional<Envelope> rsp) {
                  auto self = weak.lock();
                  if (!self)
                      return;
                  auto pld = std::make_shared<RpcResultPld>();
                  pld->rsp = std::move(rsp);
                  self->post_fifo({ StartRsp_Signal, pld });
              },
              kRpcTimeout
            );
            return chart::Status::HANDLED;
        }
        // Stop arriving mid-request: MPSS hasn't admitted the call yet, so
        // we can't cancel an in-flight RPC. Stash for replay once
        // WaitingForUp's entry recalls the deferred queue. Kept child-local
        // rather than pushed to Active_St because the tail states
        // (SendingStopReq/WaitingForDown/Reconciling) can
        // never recall — deferring Stop there would leak the StopReqPld cb.
        case Stop_Signal:
            chart::defer(self->deferred_, *e);
            return chart::Status::HANDLED;
        case StartRsp_Signal:
        {
            auto pld = event_cast<RpcResultPld>(*e);
            if (!pld->rsp)
            {
                if (self->startCb_)
                    self->startCb_(nullptr, telux::common::ErrorCode::OPERATION_TIMEOUT);
                self->onFinished_(self->profileId_);
                return self->to(Terminal_St);
            }
            if (pld->rsp->error)
            {
                auto code = common::simula::parseErrorCode(
                  pld->rsp->error->value("code", std::string())
                );
                if (self->startCb_)
                    self->startCb_(nullptr, code);
                self->onFinished_(self->profileId_);
                return self->to(Terminal_St);
            }
            self->call_->setStatus(telux::data::DataCallStatus::NET_CONNECTING);
            // Adopt the interface name MPSS assigned from its ifname pool
            // (start req carries an empty `ifname`, so the name is only known
            // once MPSS replies). Without this, getInterfaceName() stays empty
            // and every ifname-derived query (GetMtu's SIOCGIFMTU ioctl, host
            // interface lookups) fails even though the call is up.
            if (pld->rsp->data)
            {
                auto ifname = pld->rsp->data->value("ifname", std::string());
                if (!ifname.empty())
                    self->call_->setInterfaceName(std::move(ifname));
            }
            if (self->startCb_)
                self->startCb_(self->call_, telux::common::ErrorCode::SUCCESS);
            return self->to(WaitingForUp_St);
        }
        default:
            return self->super(Active_St);
    }
}

chart::Status
WaitingForUp_St(chart::Hsm* h, chart::Event const* e)
{
    auto* self = static_cast<SimulaDataCallSession*>(h);
    switch (e->sig)
    {
        case chart::Entry_Signal:
            self->bringup_timeout_.arm_one_shot(kBringupTimeout);
            chart::recall_all(self->deferred_, *self);
            return chart::Status::HANDLED;
        case chart::Exit_Signal:
            self->bringup_timeout_.disarm();
            return chart::Status::HANDLED;
        case StateInd_Signal:
        {
            auto pld = event_cast<StateIndPld>(*e);
            if (!pld->env.data)
                return chart::Status::HANDLED;
            auto status = wireToCallStatus(pld->env.data->value("status", std::string()));
            if (status == telux::data::DataCallStatus::NET_CONNECTED)
            {
                self->call_->setStatus(status);
                // The connected indication is authoritative for the assigned
                // ifname too; adopt it in case the start rsp was ever missing
                // it (start req carries an empty ifname -- MPSS fills it in).
                auto ifname = pld->env.data->value("ifname", std::string());
                if (!ifname.empty())
                    self->call_->setInterfaceName(std::move(ifname));
                applyAddrInfo(*self->call_, *pld->env.data, status);
                if (pld->env.data->contains("bearer_tech"))
                    self->call_->setBearerTech(
                      wireToBearerTech(pld->env.data->value("bearer_tech", std::string()))
                    );
                return self->to(Connected_St);
            }
            if (status == telux::data::DataCallStatus::NET_NO_NET)
            {
                self->call_->setStatus(status);
                if (pld->env.data->contains("end_reason"))
                    self->call_->setEndReason(decodeEndReason((*pld->env.data)["end_reason"]));
                self->notifyListeners_(self->call_);
                return self->to(Terminal_St);
            }
            return chart::Status::HANDLED;
        }
        case BringupTimeout_Signal:
        {
            telux::common::DataCallEndReason reason{};
            self->call_->setEndReason(reason);
            self->call_->setStatus(telux::data::DataCallStatus::NET_NO_NET);
            self->notifyListeners_(self->call_);
            return self->to(Terminal_St);
        }
        case Stop_Signal:
        {
            // Capture stopCb_ here, not in SendingStopReq_St's Entry_Signal
            // handler -- chart's dispatch synthesizes a payload-less Event
            // for ENTRY actions (see hsm.hpp Hsm::dispatch), so the
            // triggering Stop_Signal's payload is only reachable from the
            // handler that actually receives it.
            auto pld = event_cast<StopReqPld>(*e);
            if (pld)
                self->stopCb_ = std::move(pld->cb);
            return self->to(SendingStopReq_St);
        }
        default:
            return self->super(Active_St);
    }
}

chart::Status
Connected_St(chart::Hsm* h, chart::Event const* e)
{
    auto* self = static_cast<SimulaDataCallSession*>(h);
    switch (e->sig)
    {
        case chart::Entry_Signal:
            self->notifyListeners_(self->call_);
            return chart::Status::HANDLED;
        case StateInd_Signal:
        {
            auto pld = event_cast<StateIndPld>(*e);
            if (!pld->env.data)
                return chart::Status::HANDLED;
            auto status = wireToCallStatus(pld->env.data->value("status", std::string()));
            switch (status)
            {
                case telux::data::DataCallStatus::NET_RECONFIGURED:
                case telux::data::DataCallStatus::NET_NEWADDR:
                case telux::data::DataCallStatus::NET_DELADDR:
                    self->call_->setStatus(status);
                    applyAddrInfo(*self->call_, *pld->env.data, status);
                    self->notifyListeners_(self->call_);
                    return chart::Status::HANDLED;
                case telux::data::DataCallStatus::NET_CONNECTED:
                    // A RAT change re-sends call_state with status still
                    // CONNECTED ("bearer transport: re-sent on RAT change")
                    // -- re-stamp bearer_tech without treating this as a
                    // fresh connect (no re-notify; nothing else
                    // about the call changed).
                    if (pld->env.data->contains("bearer_tech"))
                        self->call_->setBearerTech(
                          wireToBearerTech(pld->env.data->value("bearer_tech", std::string()))
                        );
                    return chart::Status::HANDLED;
                case telux::data::DataCallStatus::NET_NO_NET:
                    self->call_->setStatus(status);
                    if (pld->env.data->contains("end_reason"))
                        self->call_->setEndReason(decodeEndReason((*pld->env.data)["end_reason"]));
                    self->notifyListeners_(self->call_);
                    return self->to(Terminal_St);
                default:
                    return chart::Status::HANDLED;
            }
        }
        case Stop_Signal:
        {
            auto pld = event_cast<StopReqPld>(*e);
            if (pld)
                self->stopCb_ = std::move(pld->cb);
            return self->to(SendingStopReq_St);
        }
        default:
            return self->super(Active_St);
    }
}

chart::Status
SendingStopReq_St(chart::Hsm* h, chart::Event const* e)
{
    auto* self = static_cast<SimulaDataCallSession*>(h);
    switch (e->sig)
    {
        case chart::Entry_Signal:
        {
            // stopCb_ is captured by whichever state's Stop_Signal handler
            // transitioned us here (WaitingForUp_St or Connected_St above)
            // -- chart's dispatch synthesizes a payload-less Event for
            // ENTRY actions (see hsm.hpp Hsm::dispatch), so this handler
            // never sees the original Stop_Signal's payload itself.
            nlohmann::json data = nlohmann::json::object();
            data["profileId"] = self->profileId_;
            data["ipFamily"] = ipFamilyToWire(self->call_->getIpFamilyType());
            data["ifname"] = self->call_->getInterfaceName();
            data["opType"] = opTypeToWire(self->operationType_);
            data["slot"] = static_cast<int>(self->slotId_);

            auto req =
              common::simula::makeRequestEnvelope(self->bridge_.currentPaId(), std::move(data));
            self->bridge_.send_request(
              topics::data::stop_data_call::req,
              "data.stop_data_call.rsp",
              req,
              [weak = self->weak_from_this()](std::optional<Envelope> rsp) {
                  auto self = weak.lock();
                  if (!self)
                      return;
                  auto p = std::make_shared<RpcResultPld>();
                  p->rsp = std::move(rsp);
                  self->post_fifo({ StopRsp_Signal, p });
              },
              kRpcTimeout
            );
            return chart::Status::HANDLED;
        }
        case StopRsp_Signal:
        {
            auto pld = event_cast<RpcResultPld>(*e);
            if (!pld->rsp || pld->rsp->error)
            {
                // Request-not-delivered-or-refused is not the same as
                // "still connected" -- verify with MPSS before landing
                // anywhere (Reconciling rationale).
                return self->to(Reconciling_St);
            }
            self->call_->setStatus(telux::data::DataCallStatus::NET_DISCONNECTING);
            if (self->stopCb_)
                self->stopCb_(self->call_, telux::common::ErrorCode::SUCCESS);
            return self->to(WaitingForDown_St);
        }
        default:
            return self->super(Active_St);
    }
}

chart::Status
WaitingForDown_St(chart::Hsm* h, chart::Event const* e)
{
    auto* self = static_cast<SimulaDataCallSession*>(h);
    switch (e->sig)
    {
        case chart::Entry_Signal:
            self->teardown_timeout_.arm_one_shot(kTeardownTimeout);
            return chart::Status::HANDLED;
        case chart::Exit_Signal:
            self->teardown_timeout_.disarm();
            return chart::Status::HANDLED;
        case StateInd_Signal:
        {
            auto pld = event_cast<StateIndPld>(*e);
            if (!pld->env.data)
                return chart::Status::HANDLED;
            auto status = wireToCallStatus(pld->env.data->value("status", std::string()));
            if (status == telux::data::DataCallStatus::NET_NO_NET)
            {
                self->call_->setStatus(status);
                if (pld->env.data->contains("end_reason"))
                    self->call_->setEndReason(decodeEndReason((*pld->env.data)["end_reason"]));
                self->notifyListeners_(self->call_);
                return self->to(Terminal_St);
            }
            return chart::Status::HANDLED;
        }
        case TeardownTimeout_Signal:
            // Best-effort: force terminal and report NO_NET even though
            // MPSS never confirmed teardown (WaitingForDown
            // --TeardownTimeout--> Terminal edge).
            self->call_->setStatus(telux::data::DataCallStatus::NET_NO_NET);
            self->notifyListeners_(self->call_);
            return self->to(Terminal_St);
        default:
            return self->super(Active_St);
    }
}

chart::Status
Reconciling_St(chart::Hsm* h, chart::Event const* e)
{
    auto* self = static_cast<SimulaDataCallSession*>(h);
    switch (e->sig)
    {
        case chart::Entry_Signal:
        {
            nlohmann::json data = nlohmann::json::object();
            data["opType"] = opTypeToWire(self->operationType_);
            data["slot"] = static_cast<int>(self->slotId_);

            auto req =
              common::simula::makeRequestEnvelope(self->bridge_.currentPaId(), std::move(data));
            self->bridge_.send_request(
              topics::data::list_data_call::req,
              "data.list_data_call.rsp",
              req,
              [weak = self->weak_from_this()](std::optional<Envelope> rsp) {
                  auto self = weak.lock();
                  if (!self)
                      return;
                  auto p = std::make_shared<RpcResultPld>();
                  p->rsp = std::move(rsp);
                  self->post_fifo({ ListRsp_Signal, p });
              },
              kRpcTimeout
            );
            return chart::Status::HANDLED;
        }
        case ListRsp_Signal:
        {
            auto pld = event_cast<RpcResultPld>(*e);
            // list RPC itself failed/timed out: can't confirm either way.
            // Conservative call -- assume disconnected rather than leak a
            // call record client code can never clean up.
            if (!pld->rsp || pld->rsp->error)
            {
                self->call_->setStatus(telux::data::DataCallStatus::NET_NO_NET);
                if (self->stopCb_)
                    self->stopCb_(nullptr, telux::common::ErrorCode::GENERIC_FAILURE);
                self->notifyListeners_(self->call_);
                return self->to(Terminal_St);
            }
            bool still_up = false;
            if (pld->rsp->data && pld->rsp->data->contains("calls"))
            {
                for (const auto& c : (*pld->rsp->data)["calls"])
                {
                    if (c.value("profileId", -1) == self->profileId_ &&
                        wireToCallStatus(c.value("status", std::string())) ==
                          telux::data::DataCallStatus::NET_CONNECTED)
                    {
                        still_up = true;
                        break;
                    }
                }
            }
            if (still_up)
            {
                self->call_->setStatus(telux::data::DataCallStatus::NET_CONNECTED);
                if (self->stopCb_)
                    self->stopCb_(nullptr, telux::common::ErrorCode::GENERIC_FAILURE);
                return self->to(Connected_St);
            }
            self->call_->setStatus(telux::data::DataCallStatus::NET_NO_NET);
            if (self->stopCb_)
                self->stopCb_(self->call_, telux::common::ErrorCode::SUCCESS);
            self->notifyListeners_(self->call_);
            return self->to(Terminal_St);
        }
        default:
            return self->super(Active_St);
    }
}

chart::Status
Terminal_St(chart::Hsm* h, chart::Event const* e)
{
    auto* self = static_cast<SimulaDataCallSession*>(h);
    switch (e->sig)
    {
        case chart::Entry_Signal:
            // No unconditional notifyListeners_ here -- every path that
            // transitions to Terminal_St after the call was genuinely
            // connected (or genuinely torn down) already notified at the
            // point of transition above. A path that never got the call
            // admitted in the first place (SendingStartReq_St's error/
            // timeout branches) must NOT notify, matching the real SDK's
            // documented DataCallResponseCb contract: on failure,
            // onDataCallInfoChanged is not called.
            self->onFinished_(self->profileId_);
            return chart::Status::HANDLED;
        default:
            // Terminal: swallow everything else. The Manager drops its
            // shared_ptr to this Session on SessionFinished_Signal (posted
            // by onFinished_ above); the AO worker thread then winds down
            // once this object's destructor runs stop().
            return chart::Status::HANDLED;
    }
}

// ============================================================================
// SimulaDataConnectionManager
// ============================================================================
//
// 2-state readiness shell.

namespace {

struct StartDataCallPld
{
    telux::data::DataCallParams params;
    telux::data::DataCallResponseCb cb;
};

struct StopDataCallPld
{
    telux::data::DataCallParams params;
    telux::data::DataCallResponseCb cb;
};

struct SessionFinishedPld
{
    int profileId;
};

struct RequestDataCallListPld
{
    telux::data::OperationType operationType;
    telux::data::DataCallListResponseCb cb;
};

struct GetDefaultProfilePld
{
    telux::data::OperationType operationType;
    telux::data::DefaultProfileIdResponseCb cb;
};

struct SetDefaultProfilePld
{
    telux::data::OperationType operationType;
    uint8_t profileId;
    telux::common::ResponseCallback cb;
};

struct RequestThrottledApnInfoPld
{
    telux::data::ThrottleInfoCb cb;
};

// setThroughputInterval/getLastThroughputInfo have synchronous ErrorCode
// signatures (no callback param) but the underlying RPC is async over MQTT
// -- these payloads carry a promise the RPC callback fulfils, so the public
// API method (running on an arbitrary client thread) can block on the
// matching future rather than fabricating a result.
struct SetThroughputIntervalPld
{
    uint32_t reportInterval;
    std::shared_ptr<std::promise<telux::common::ErrorCode>> result;
};

struct GetLastThroughputInfoPld
{
    std::shared_ptr<std::promise<std::pair<telux::common::ErrorCode,
                                            std::vector<telux::data::ThroughputInfo>>>>
      result;
};

}  // namespace

chart::Status
NotReady_St(chart::Hsm*, chart::Event const*);
chart::Status
Ready_St(chart::Hsm*, chart::Event const*);
chart::Status
Operating_St(chart::Hsm*, chart::Event const*);

SimulaDataConnectionManager::SimulaDataConnectionManager(
  SlotId slotId,
  common::simula::IModemBridge& bridge,
  telux::common::InitResponseCb initCb
)
    : chart::ActiveObject("DataConnectionManager")
    , bridge_(bridge)
    , slotId_(slotId)
    , init_cb_(std::move(initCb))
{}

SimulaDataConnectionManager::~SimulaDataConnectionManager()
{
    // Withdraw from the bridge FIRST. Every callback registered in start()
    // captures raw `this`; the bridge holds its own copies and would happily
    // invoke them against freed memory after we return. unsubscribe_* only
    // *queues* the removal, so drain() is mandatory: it fences against the
    // bridge worker's FIFO, guaranteeing that every already-queued indication
    // (and the removals themselves) have finished running before we tear
    // anything down.
    unsubscribeFromBridge_();

    // Tear down Sessions first: each Session's dtor joins its own AO worker
    // thread, so no Session thread can still be running when our
    // listeners_mutex_/listeners_ destructors run below. Without this,
    // reverse-member-order teardown destroys listeners_mutex_ while a
    // Session worker is mid-notifyListeners_ -> broadcastDataCallInfoChanged
    // -> lock_guard(listeners_mutex_), which is UAF on the mutex (and on
    // *this via the captured mgr pointer). stop() after, to join our own AO.
    active_sessions_.clear();
    stop();
}

void
SimulaDataConnectionManager::unsubscribeFromBridge_()
{
    bridge_.unsubscribe_event(topics::data::subsys_ready_data::ind);
    bridge_.unsubscribe_event(topics::data::call_state::ind);
    bridge_.unsubscribe_event(topics::data::throughput_info::ind);
    bridge_.unsubscribe_event(topics::data::qos_status::ind);
    bridge_.unsubscribe_event(topics::data::hw_accel_state::ind);
    bridge_.unsubscribe_event(topics::data::throttle_status::ind);
    bridge_.unsubscribe_connectivity(conn_token_);
    conn_token_ = 0;
    bridge_.drain();
}

void
SimulaDataConnectionManager::start()
{
    if (running())
        return;
    set_instrument(std::make_unique<chart::SpyInstrument>(name()));
    start_at(NotReady_St);

    bridge_.subscribe_event(
      topics::data::subsys_ready_data::ind,
      "data.subsys_ready_data.ind",
      [this](std::string_view topic, const Envelope& env) { handleCallStateInd_(topic, env); }
    );
    bridge_.subscribe_event(
      topics::data::call_state::ind,
      "data.call_state.ind",
      [this](std::string_view topic, const Envelope& env) { handleCallStateInd_(topic, env); }
    );
    bridge_.subscribe_event(
      topics::data::throughput_info::ind,
      "data.throughput_info.ind",
      [this](std::string_view topic, const Envelope& env) { handleCallStateInd_(topic, env); }
    );
    bridge_.subscribe_event(
      topics::data::qos_status::ind,
      "data.qos_status.ind",
      [this](std::string_view topic, const Envelope& env) { handleCallStateInd_(topic, env); }
    );
    bridge_.subscribe_event(
      topics::data::hw_accel_state::ind,
      "data.hw_accel_state.ind",
      [this](std::string_view topic, const Envelope& env) { handleCallStateInd_(topic, env); }
    );
    bridge_.subscribe_event(
      topics::data::throttle_status::ind,
      "data.throttle_status.ind",
      [this](std::string_view topic, const Envelope& env) { handleCallStateInd_(topic, env); }
    );
    conn_token_ = bridge_.subscribe_connectivity([this](bool operational) {
        auto pld = std::make_shared<bool>(operational);
        post_fifo({ BridgeConnectivityChanged_Signal, pld });
    });
}

void
SimulaDataConnectionManager::handleCallStateInd_(std::string_view topic, const Envelope& env)
{
    // Fires on the bridge's own worker thread (see IModemBridge::EventCallback's
    // doc comment) -- never touch active_sessions_ here. Just forward.
    auto pld = std::make_shared<StateIndPld>();
    pld->env = env;
    if (topic == topics::data::subsys_ready_data::ind)
        post_fifo({ ReadinessEvt_Signal, pld });
    else if (topic == topics::data::throughput_info::ind)
        post_fifo({ ThroughputInfoEvt_Signal, pld });
    else if (topic == topics::data::qos_status::ind)
        post_fifo({ QosStatusEvt_Signal, pld });
    else if (topic == topics::data::hw_accel_state::ind)
        post_fifo({ HwAccelEvt_Signal, pld });
    else if (topic == topics::data::throttle_status::ind)
        post_fifo({ ThrottleStatusEvt_Signal, pld });
    else
        post_fifo({ StateInd_Signal, pld });
}

void
SimulaDataConnectionManager::broadcastToListeners_(
  telux::data::DataConnectionIndicationsType bit,
  std::function<void(const std::shared_ptr<telux::data::IDataConnectionListener>&)> invoke
)
{
    auto task = std::make_shared<common::simula::DispatchTask>();
    task->debug_tag = "DataConnectionManager::broadcastToListeners_";
    {
        std::lock_guard<std::mutex> lk(listeners_mutex_);
        for (auto& [weak, indications] : listeners_)
        {
            if (!indications.test(bit))
                continue;
            if (auto sp = weak.lock())
                task->listeners.push_back(sp);
        }
    }
    if (task->listeners.empty())
        return;
    task->invoker = [invoke](std::shared_ptr<void> raw) {
        invoke(std::static_pointer_cast<telux::data::IDataConnectionListener>(raw));
    };
    common::simula::ListenerDispatchAO::instance().enqueue(std::move(task));
}

void
SimulaDataConnectionManager::broadcastDataCallInfoChanged(
  std::shared_ptr<telux::data::IDataCall> call
)
{
    broadcastToListeners_(
      telux::data::DataConnectionIndicationsType::DEFAULT,
      [call](const std::shared_ptr<telux::data::IDataConnectionListener>& l) {
          l->onDataCallInfoChanged(call);
      }
    );
}

// ---------------------------------------------------------------------------
// telux::data::IDataConnectionManager

telux::common::ServiceStatus
SimulaDataConnectionManager::getServiceStatus()
{
    return last_status_.load();
}

bool
SimulaDataConnectionManager::isReadyDerived_() const
{
    // Chart-derived readiness: sole authority is the chart's current-state
    // pointer. Ready_St == Ready, everything else (NotReady_St and the
    // Operating_St composite bookkeeping ancestor) == NotReady.
    return const_cast<SimulaDataConnectionManager*>(this)->current_state() == Ready_St;
}

void
SimulaDataConnectionManager::publishStatus_(telux::common::ServiceStatus s)
{
    last_status_.store(s);
}

bool
SimulaDataConnectionManager::isSubsystemReady()
{
    return isReadyDerived_();
}

std::future<bool>
SimulaDataConnectionManager::onSubsystemReady()
{
    // Deprecated in the real SDK in favor of InitResponseCb (see
    // DataConnectionManager.hpp's doc comment on this method) -- not worth
    // building out a promise/future plumbing path for a deprecated API.
    std::promise<bool> p;
    p.set_value(isReadyDerived_());
    return p.get_future();
}

telux::common::Status
SimulaDataConnectionManager::setDefaultProfile(
  telux::data::OperationType operationType,
  uint8_t profileId,
  telux::common::ResponseCallback callback
)
{
    if (!isReadyDerived_())
        return telux::common::Status::NOTREADY;
    auto pld = std::make_shared<SetDefaultProfilePld>();
    pld->operationType = operationType;
    pld->profileId = profileId;
    pld->cb = std::move(callback);
    post_fifo({ SetDefaultProfile_Signal, pld });
    return telux::common::Status::SUCCESS;
}

telux::common::Status
SimulaDataConnectionManager::getDefaultProfile(
  telux::data::OperationType operationType,
  telux::data::DefaultProfileIdResponseCb callback
)
{
    if (!isReadyDerived_())
        return telux::common::Status::NOTREADY;
    auto pld = std::make_shared<GetDefaultProfilePld>();
    pld->operationType = operationType;
    pld->cb = std::move(callback);
    post_fifo({ GetDefaultProfile_Signal, pld });
    return telux::common::Status::SUCCESS;
}

telux::common::Status
SimulaDataConnectionManager::setRoamingMode(
  bool /*enable*/,
  uint8_t /*profileId*/,
  telux::data::OperationType /*operationType*/,
  telux::common::ResponseCallback /*callback*/
)
{
    return telux::common::Status::NOTSUPPORTED;
}

telux::common::Status
SimulaDataConnectionManager::requestRoamingMode(
  uint8_t /*profileId*/,
  telux::data::OperationType /*operationType*/,
  telux::data::requestRoamingModeResponseCb /*callback*/
)
{
    return telux::common::Status::NOTSUPPORTED;
}

telux::common::Status
SimulaDataConnectionManager::startDataCall(
  const telux::data::DataCallParams& dataCallParams,
  telux::data::DataCallResponseCb callback
)
{
    if (!isReadyDerived_())
        return telux::common::Status::NOTREADY;
    auto pld = std::make_shared<StartDataCallPld>();
    pld->params = dataCallParams;
    pld->cb = std::move(callback);
    post_fifo({ StartDataCall_Signal, pld });
    return telux::common::Status::SUCCESS;
}

telux::common::Status
SimulaDataConnectionManager::stopDataCall(
  const telux::data::DataCallParams& dataCallParams,
  telux::data::DataCallResponseCb callback
)
{
    if (!isReadyDerived_())
        return telux::common::Status::NOTREADY;
    auto pld = std::make_shared<StopDataCallPld>();
    pld->params = dataCallParams;
    pld->cb = std::move(callback);
    post_fifo({ StopDataCall_Signal, pld });
    return telux::common::Status::SUCCESS;
}

telux::common::Status
SimulaDataConnectionManager::registerListener(
  std::weak_ptr<telux::data::IDataConnectionListener> listener,
  telux::data::DataConnectionIndications indicationList
)
{
    std::lock_guard<std::mutex> lk(listeners_mutex_);
    listeners_.emplace_back(std::move(listener), indicationList);
    return telux::common::Status::SUCCESS;
}

telux::common::Status
SimulaDataConnectionManager::deregisterListener(
  std::weak_ptr<telux::data::IDataConnectionListener> listener,
  telux::data::DataConnectionIndications indicationList
)
{
    std::lock_guard<std::mutex> lk(listeners_mutex_);
    auto target = listener.lock();
    for (auto it = listeners_.begin(); it != listeners_.end();)
    {
        auto sp = it->first.lock();
        if (!sp || (target && sp == target))
        {
            // Deregister only the requested indication bits (real-SDK-mirrored
            // semantics: a listener registered for DEFAULT+THROUGHPUT that
            // deregisters only DEFAULT stays
            // registered for THROUGHPUT).
            it->second &= ~indicationList;
            if (!sp || it->second.none())
                it = listeners_.erase(it);
            else
                ++it;
        }
        else
        {
            ++it;
        }
    }
    return telux::common::Status::SUCCESS;
}

int
SimulaDataConnectionManager::getSlotId()
{
    return static_cast<int>(slotId_);
}

telux::common::Status
SimulaDataConnectionManager::requestDataCallList(
  telux::data::OperationType operationType,
  telux::data::DataCallListResponseCb callback
)
{
    if (!isReadyDerived_())
        return telux::common::Status::NOTREADY;
    // Answered from this Manager's own active_sessions_ (this process's own
    // bookkeeping), not via a `list_data_call` RPC to MPSS -- that RPC is
    // Reconciling_St's tool for verifying a *specific* profileId's real
    // state after an uncertain stop, a different concern from "what has
    // this process itself started". Filtering by operationType here mirrors
    // requestDataCallList's real-SDK contract.
    auto pld = std::make_shared<RequestDataCallListPld>();
    pld->operationType = operationType;
    pld->cb = std::move(callback);
    post_fifo({ RequestDataCallList_Signal, pld });
    return telux::common::Status::SUCCESS;
}

telux::common::Status
SimulaDataConnectionManager::startDataCall(
  int profileId,
  telux::data::IpFamilyType ipFamilyType,
  telux::data::DataCallResponseCb callback,
  telux::data::OperationType operationType,
  std::string /*apn*/
)
{
    telux::data::DataCallParams params;
    params.profileId = profileId;
    params.ipFamilyType = ipFamilyType;
    params.operationType = operationType;
    return startDataCall(params, std::move(callback));
}

telux::common::Status
SimulaDataConnectionManager::stopDataCall(
  int profileId,
  telux::data::IpFamilyType ipFamilyType,
  telux::data::DataCallResponseCb callback,
  telux::data::OperationType operationType,
  std::string /*apn*/
)
{
    telux::data::DataCallParams params;
    params.profileId = profileId;
    params.ipFamilyType = ipFamilyType;
    params.operationType = operationType;
    return stopDataCall(params, std::move(callback));
}

telux::common::Status
SimulaDataConnectionManager::requestThrottledApnInfo(telux::data::ThrottleInfoCb callback)
{
    if (!isReadyDerived_())
        return telux::common::Status::NOTREADY;
    auto pld = std::make_shared<RequestThrottledApnInfoPld>();
    pld->cb = std::move(callback);
    post_fifo({ RequestThrottledApnInfo_Signal, pld });
    return telux::common::Status::SUCCESS;
}

telux::common::ErrorCode
SimulaDataConnectionManager::setThroughputInterval(uint32_t reportInterval)
{
    // Real signature has no callback param (telux::data::
    // IDataConnectionManager::setThroughputInterval) but the RPC is async
    // over MQTT -- block the calling thread on a promise/future the AO's
    // RPC callback fulfils, capped at kRpcTimeout so a stuck bridge can't
    // hang the caller forever. No existing sync-signature RPC bridge
    // precedent in this file to follow; onSubsystemReady's promise/future
    // (above) is the closest analog, though that one never actually blocks.
    if (!isReadyDerived_())
        return telux::common::ErrorCode::SUBSYSTEM_UNAVAILABLE;
    auto pld = std::make_shared<SetThroughputIntervalPld>();
    pld->reportInterval = reportInterval;
    pld->result = std::make_shared<std::promise<telux::common::ErrorCode>>();
    auto future = pld->result->get_future();
    post_fifo({ SetThroughputInterval_Signal, pld });
    if (future.wait_for(kRpcTimeout) != std::future_status::ready)
        return telux::common::ErrorCode::OPERATION_TIMEOUT;
    return future.get();
}

telux::common::ErrorCode
SimulaDataConnectionManager::getLastThroughputInfo(std::vector<telux::data::ThroughputInfo>& info)
{
    if (!isReadyDerived_())
        return telux::common::ErrorCode::SUBSYSTEM_UNAVAILABLE;
    auto pld = std::make_shared<GetLastThroughputInfoPld>();
    pld->result = std::make_shared<
      std::promise<std::pair<telux::common::ErrorCode, std::vector<telux::data::ThroughputInfo>>>>();
    auto future = pld->result->get_future();
    post_fifo({ GetLastThroughputInfo_Signal, pld });
    if (future.wait_for(kRpcTimeout) != std::future_status::ready)
        return telux::common::ErrorCode::OPERATION_TIMEOUT;
    auto [error, infos] = future.get();
    if (error == telux::common::ErrorCode::SUCCESS)
        info = std::move(infos);
    return error;
}

// ---------------------------------------------------------------------------
// State handlers

// Composite parent for {NotReady, Ready}. Owns the readiness-agnostic
// SessionFinished_Signal handling that both children previously duplicated:
// an in-flight Session that reaches Terminal_St after the Manager degraded
// out of Ready still gets its slot in active_sessions_ cleaned up here.
chart::Status
Operating_St(chart::Hsm* h, chart::Event const* e)
{
    auto* self = static_cast<SimulaDataConnectionManager*>(h);
    switch (e->sig)
    {
        case chart::Entry_Signal:
        case chart::Exit_Signal:
            return chart::Status::HANDLED;
        case SessionFinished_Signal:
        {
            auto pld = event_cast<SessionFinishedPld>(*e);
            self->active_sessions_.erase(pld->profileId);
            return chart::Status::HANDLED;
        }
        default:
            return self->super(&chart::Hsm::top);
    }
}

chart::Status
NotReady_St(chart::Hsm* h, chart::Event const* e)
{
    auto* self = static_cast<SimulaDataConnectionManager*>(h);
    switch (e->sig)
    {
        case chart::Entry_Signal:
        case chart::Exit_Signal:
            return chart::Status::HANDLED;
        case ReadinessEvt_Signal:
        {
            auto pld = event_cast<StateIndPld>(*e);
            if (pld->env.data && pld->env.data->value("status", std::string()) == "AVAILABLE")
                return self->to(Ready_St);
            return chart::Status::HANDLED;
        }
        case BridgeConnectivityChanged_Signal:
            // Already NotReady; nothing to degrade from.
            return chart::Status::HANDLED;
        // StartDataCall_Signal/StopDataCall_Signal/RequestDataCallList_Signal/
        // GetDefaultProfile_Signal/SetDefaultProfile_Signal/
        // RequestThrottledApnInfo_Signal land here only if a stale call got
        // queued while transitioning (the public API methods synchronously
        // return NOTREADY before ever posting). SessionFinished_Signal is a
        // real case though: a Session already in flight when the bridge
        // drops keeps running (Ready_St's Exit_Signal doesn't tear down
        // active_sessions_) and still reports back once it reaches
        // Terminal_St.
        case StartDataCall_Signal:
        case StopDataCall_Signal:
        case RequestDataCallList_Signal:
        case GetDefaultProfile_Signal:
        case SetDefaultProfile_Signal:
        case RequestThrottledApnInfo_Signal:
            return chart::Status::HANDLED;
        // SetThroughputInterval_Signal/GetLastThroughputInfo_Signal carry a
        // promise the caller is blocked on (setThroughputInterval/
        // getLastThroughputInfo have synchronous ErrorCode signatures) --
        // unlike the callback-based signals above, dropping these silently
        // would hang the caller for the full kRpcTimeout instead of
        // returning promptly.
        case SetThroughputInterval_Signal:
        {
            auto pld = event_cast<SetThroughputIntervalPld>(*e);
            pld->result->set_value(telux::common::ErrorCode::SUBSYSTEM_UNAVAILABLE);
            return chart::Status::HANDLED;
        }
        case GetLastThroughputInfo_Signal:
        {
            auto pld = event_cast<GetLastThroughputInfoPld>(*e);
            pld->result->set_value({ telux::common::ErrorCode::SUBSYSTEM_UNAVAILABLE, {} });
            return chart::Status::HANDLED;
        }
        case SessionFinished_Signal:
            // Handled by Operating_St (parent) — a Session that finishes
            // after the Manager degraded out of Ready still needs
            // active_sessions_ cleanup, and the cleanup is state-agnostic.
            return chart::Status::UNHANDLED;
        default:
            return self->super(Operating_St);
    }
}

chart::Status
Ready_St(chart::Hsm* h, chart::Event const* e)
{
    auto* self = static_cast<SimulaDataConnectionManager*>(h);
    switch (e->sig)
    {
        case chart::Entry_Signal:
        {
            self->publishStatus_(telux::common::ServiceStatus::SERVICE_AVAILABLE);
            if (!self->init_cb_fired_)
            {
                self->init_cb_fired_ = true;
                if (self->init_cb_)
                    self->init_cb_(telux::common::ServiceStatus::SERVICE_AVAILABLE);
            }
            else
            {
                self->broadcastToListeners_(
                  telux::data::DataConnectionIndicationsType::DEFAULT,
                  [](const std::shared_ptr<telux::data::IDataConnectionListener>& l) {
                      l->onServiceStatusChange(telux::common::ServiceStatus::SERVICE_AVAILABLE);
                  }
                );
            }
            return chart::Status::HANDLED;
        }
        case chart::Exit_Signal:
            self->broadcastToListeners_(
              telux::data::DataConnectionIndicationsType::DEFAULT,
              [](const std::shared_ptr<telux::data::IDataConnectionListener>& l) {
                  l->onServiceStatusChange(telux::common::ServiceStatus::SERVICE_UNAVAILABLE);
              }
            );
            return chart::Status::HANDLED;

        case ReadinessEvt_Signal:
        {
            auto pld = event_cast<StateIndPld>(*e);
            if (!pld->env.data)
                return chart::Status::HANDLED;
            auto state = pld->env.data->value("status", std::string());
            if (state == "UNAVAILABLE")
            {
                self->publishStatus_(telux::common::ServiceStatus::SERVICE_UNAVAILABLE);
                return self->to(NotReady_St);
            }
            if (state == "FAILED")
            {
                self->publishStatus_(telux::common::ServiceStatus::SERVICE_FAILED);
                return self->to(NotReady_St);
            }
            return chart::Status::HANDLED;
        }

        case BridgeConnectivityChanged_Signal:
        {
            auto pld = event_cast<bool>(*e);
            if (pld && !*pld)
            {
                self->publishStatus_(telux::common::ServiceStatus::SERVICE_UNAVAILABLE);
                return self->to(NotReady_St);
            }
            return chart::Status::HANDLED;
        }

        case StartDataCall_Signal:
        {
            auto pld = event_cast<StartDataCallPld>(*e);
            int profileId = pld->params.profileId;
            auto it = self->active_sessions_.find(profileId);
            if (it != self->active_sessions_.end() &&
                it->second->dataCall()->getDataCallStatus() ==
                  telux::data::DataCallStatus::NET_CONNECTED)
            {
                // Real contract's scenario 2: already up -> immediate
                // SUCCESS, no onDataCallInfoChanged (rationale for keeping
                // this check out of Session, which always runs the full
                // Idle->... lifecycle with no
                // shortcut state).
                if (pld->cb)
                    pld->cb(it->second->dataCall(), telux::common::ErrorCode::SUCCESS);
                return chart::Status::HANDLED;
            }
            if (it != self->active_sessions_.end())
            {
                // A session for this profileId is already mid-lifecycle
                // (not yet Connected) -- real SDK's OP_IN_PROGRESS case.
                if (pld->cb)
                    pld->cb(nullptr, telux::common::ErrorCode::OP_IN_PROGRESS);
                return chart::Status::HANDLED;
            }

            auto* mgr = self;
            auto session = std::make_shared<SimulaDataCallSession>(
              mgr->bridge_,
              mgr->slotId_,
              profileId,
              pld->params.ipFamilyType,
              pld->params.operationType,
              pld->params.interfaceName,
              [mgr](std::shared_ptr<telux::data::IDataCall> call) {
                  mgr->broadcastDataCallInfoChanged(std::move(call));
              },
              [mgr](int finishedProfileId) {
                  auto p = std::make_shared<SessionFinishedPld>();
                  p->profileId = finishedProfileId;
                  mgr->post_fifo({ SessionFinished_Signal, p });
              }
            );
            self->active_sessions_.emplace(profileId, session);
            session->start(pld->cb);
            return chart::Status::HANDLED;
        }

        case StopDataCall_Signal:
        {
            auto pld = event_cast<StopDataCallPld>(*e);
            auto it = self->active_sessions_.find(pld->params.profileId);
            if (it == self->active_sessions_.end())
            {
                if (pld->cb)
                    pld->cb(nullptr, telux::common::ErrorCode::INVALID_STATE);
                return chart::Status::HANDLED;
            }
            it->second->requestStop(pld->cb);
            return chart::Status::HANDLED;
        }

        case StateInd_Signal:
        {
            auto pld = event_cast<StateIndPld>(*e);
            if (!pld->env.data)
                return chart::Status::HANDLED;
            int profileId = pld->env.data->value("profileId", -1);
            auto it = self->active_sessions_.find(profileId);
            if (it != self->active_sessions_.end())
                it->second->deliverStateInd(pld->env);
            return chart::Status::HANDLED;
        }

        case SessionFinished_Signal:
            // Bubble to Operating_St, which owns active_sessions_ cleanup.
            return chart::Status::UNHANDLED;

        case RequestDataCallList_Signal:
        {
            auto pld = event_cast<RequestDataCallListPld>(*e);
            std::vector<std::shared_ptr<telux::data::IDataCall>> out;
            for (auto& [profileId, session] : self->active_sessions_)
            {
                (void)profileId;
                if (session->dataCall()->getOperationType() == pld->operationType)
                    out.push_back(session->dataCall());
            }
            if (pld->cb)
                pld->cb(out, telux::common::ErrorCode::SUCCESS);
            return chart::Status::HANDLED;
        }

        case GetDefaultProfile_Signal:
        {
            auto pld = event_cast<GetDefaultProfilePld>(*e);
            nlohmann::json data = nlohmann::json::object();
            data["slot"] = static_cast<int>(self->slotId_);
            data["opType"] = opTypeToWire(pld->operationType);
            auto req = common::simula::makeRequestEnvelope(self->bridge_.currentPaId(), std::move(data));
            auto cb = pld->cb;
            auto slotId = self->slotId_;
            self->bridge_.send_request(
              topics::data::get_default_profile::req,
              "data.get_default_profile.rsp",
              req,
              [cb, slotId](std::optional<Envelope> rsp) {
                  if (!cb)
                      return;
                  if (!rsp || rsp->error || !rsp->data)
                  {
                      cb(telux::data::DataProfile::PROFILE_ID_INVALID, slotId,
                         telux::common::ErrorCode::OPERATION_TIMEOUT);
                      return;
                  }
                  cb(
                    rsp->data->value("profileId", telux::data::DataProfile::PROFILE_ID_INVALID),
                    slotId,
                    telux::common::ErrorCode::SUCCESS
                  );
              },
              kRpcTimeout
            );
            return chart::Status::HANDLED;
        }

        case SetDefaultProfile_Signal:
        {
            // Mirrors GetDefaultProfile_Signal above exactly -- same
            // chart-signal -> RPC -> single-round-trip callback shape, just
            // a ResponseCallback (ErrorCode-only)
            // instead of DefaultProfileIdResponseCb.
            auto pld = event_cast<SetDefaultProfilePld>(*e);
            nlohmann::json data = nlohmann::json::object();
            data["slot"] = static_cast<int>(self->slotId_);
            data["opType"] = opTypeToWire(pld->operationType);
            data["profileId"] = static_cast<int>(pld->profileId);
            auto req = common::simula::makeRequestEnvelope(self->bridge_.currentPaId(), std::move(data));
            auto cb = pld->cb;
            self->bridge_.send_request(
              topics::data::set_default_profile::req,
              "data.set_default_profile.rsp",
              req,
              [cb](std::optional<Envelope> rsp) {
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
              },
              kRpcTimeout
            );
            return chart::Status::HANDLED;
        }

        case RequestThrottledApnInfo_Signal:
        {
            // The real requestThrottledApnInfo(ThrottleInfoCb) takes no
            // profileId and returns every throttled APN in one shot
            // (DataConnectionManager.hpp's doc comment), but MPSS's
            // request_throttle_status RPC is profileId-scoped -- there is
            // no wildcard "give me all" request on the wire. Approximate
            // by querying throttle status for every profileId this Manager
            // currently has a session for and aggregating the (successful)
            // results; a profile with no throttle preset on the MPSS side
            // errors that RPC and is simply omitted from the aggregate
            // (not itself an overall failure).
            auto pld = event_cast<RequestThrottledApnInfoPld>(*e);
            std::vector<int> profileIds;
            for (auto& entry : self->active_sessions_)
                profileIds.push_back(entry.first);
            if (profileIds.empty())
            {
                if (pld->cb)
                    pld->cb({}, telux::common::ErrorCode::SUCCESS);
                return chart::Status::HANDLED;
            }
            auto results = std::make_shared<std::vector<telux::data::APNThrottleInfo>>();
            auto remaining = std::make_shared<std::atomic<int>>(static_cast<int>(profileIds.size()));
            auto cb = pld->cb;
            auto resultsMutex = std::make_shared<std::mutex>();
            for (int profileId : profileIds)
            {
                nlohmann::json data = nlohmann::json::object();
                data["profileId"] = profileId;
                auto req =
                  common::simula::makeRequestEnvelope(self->bridge_.currentPaId(), std::move(data));
                self->bridge_.send_request(
                  topics::data::request_throttle_status::req,
                  "data.request_throttle_status.rsp",
                  req,
                  [results, remaining, cb, resultsMutex](std::optional<Envelope> rsp) {
                      if (rsp && !rsp->error && rsp->data)
                      {
                          std::lock_guard<std::mutex> lk(*resultsMutex);
                          results->push_back(decodeThrottleInfo(*rsp->data));
                      }
                      if (remaining->fetch_sub(1) == 1 && cb)
                          cb(*results, telux::common::ErrorCode::SUCCESS);
                  },
                  kRpcTimeout
                );
            }
            return chart::Status::HANDLED;
        }

        case SetThroughputInterval_Signal:
        {
            auto pld = event_cast<SetThroughputIntervalPld>(*e);
            nlohmann::json data = nlohmann::json::object();
            data["intervalMs"] = static_cast<int>(pld->reportInterval);
            auto req = common::simula::makeRequestEnvelope(self->bridge_.currentPaId(), std::move(data));
            auto result = pld->result;
            self->bridge_.send_request(
              topics::data::set_throughput_interval::req,
              "data.set_throughput_interval.rsp",
              req,
              [result](std::optional<Envelope> rsp) {
                  if (!rsp || rsp->error)
                  {
                      result->set_value(telux::common::ErrorCode::OPERATION_TIMEOUT);
                      return;
                  }
                  result->set_value(telux::common::ErrorCode::SUCCESS);
              },
              kRpcTimeout
            );
            return chart::Status::HANDLED;
        }

        case GetLastThroughputInfo_Signal:
        {
            auto pld = event_cast<GetLastThroughputInfoPld>(*e);
            nlohmann::json data = nlohmann::json::object();
            auto req = common::simula::makeRequestEnvelope(self->bridge_.currentPaId(), std::move(data));
            auto result = pld->result;
            self->bridge_.send_request(
              topics::data::request_throughput_info::req,
              "data.request_throughput_info.rsp",
              req,
              [result](std::optional<Envelope> rsp) {
                  if (!rsp || rsp->error || !rsp->data)
                  {
                      result->set_value({ telux::common::ErrorCode::OPERATION_TIMEOUT, {} });
                      return;
                  }
                  auto infos = decodeThroughputInfos(rsp->data->value("infos", nlohmann::json::array()));
                  result->set_value({ telux::common::ErrorCode::SUCCESS, std::move(infos) });
              },
              kRpcTimeout
            );
            return chart::Status::HANDLED;
        }

        case ThroughputInfoEvt_Signal:
        {
            auto pld = event_cast<StateIndPld>(*e);
            if (!pld->env.data)
                return chart::Status::HANDLED;
            auto infos = decodeThroughputInfos(pld->env.data->value("infos", nlohmann::json::array()));
            self->broadcastToListeners_(
              telux::data::DataConnectionIndicationsType::THROUGHPUT,
              [infos](const std::shared_ptr<telux::data::IDataConnectionListener>& l) {
                  l->onThroughputInfoAvailable(infos);
              }
            );
            return chart::Status::HANDLED;
        }

        case HwAccelEvt_Signal:
        {
            auto pld = event_cast<StateIndPld>(*e);
            if (!pld->env.data)
                return chart::Status::HANDLED;
            auto state = pld->env.data->value("state", std::string()) == "ACTIVE"
              ? telux::data::ServiceState::ACTIVE
              : telux::data::ServiceState::INACTIVE;
            self->broadcastToListeners_(
              telux::data::DataConnectionIndicationsType::DEFAULT,
              [state](const std::shared_ptr<telux::data::IDataConnectionListener>& l) {
                  l->onHwAccelerationChanged(state);
              }
            );
            return chart::Status::HANDLED;
        }

        case ThrottleStatusEvt_Signal:
        {
            auto pld = event_cast<StateIndPld>(*e);
            if (!pld->env.data)
                return chart::Status::HANDLED;
            std::vector<telux::data::APNThrottleInfo> list{ decodeThrottleInfo(*pld->env.data) };
            self->broadcastToListeners_(
              telux::data::DataConnectionIndicationsType::DEFAULT,
              [list](const std::shared_ptr<telux::data::IDataConnectionListener>& l) {
                  l->onThrottledApnInfoChanged(list);
              }
            );
            return chart::Status::HANDLED;
        }

        case QosStatusEvt_Signal:
        {
            auto pld = event_cast<StateIndPld>(*e);
            if (!pld->env.data)
                return chart::Status::HANDLED;
            int profileId = pld->env.data->value("profileId", -1);
            auto it = self->active_sessions_.find(profileId);
            if (it == self->active_sessions_.end())
            {
                // No matching session for this profileId's QoS event --
                // onTrafficFlowTemplateChange requires a dataCall to
                // attribute the change to; drop rather than fabricate one.
                return chart::Status::HANDLED;
            }
            auto dataCall = it->second->dataCall();
            auto tft = decodeTrafficFlowTemplate(*pld->env.data);
            auto tftChange = std::make_shared<telux::data::TftChangeInfo>();
            tftChange->tft = tft;
            tftChange->stateChange = tft->stateChange;
            std::vector<std::shared_ptr<telux::data::TftChangeInfo>> tfts{ tftChange };
            self->broadcastToListeners_(
              telux::data::DataConnectionIndicationsType::DEFAULT,
              [dataCall, tfts](const std::shared_ptr<telux::data::IDataConnectionListener>& l) {
                  l->onTrafficFlowTemplateChange(dataCall, tfts);
              }
            );
            return chart::Status::HANDLED;
        }

        default:
            return self->super(Operating_St);
    }
}

CHART_NAMED_STATE(Idle_St,            "DataCallSession::Idle");
CHART_NAMED_STATE(Active_St,          "DataCallSession::Active");
CHART_NAMED_STATE(SendingStartReq_St, "DataCallSession::SendingStartReq");
CHART_NAMED_STATE(WaitingForUp_St,    "DataCallSession::WaitingForUp");
CHART_NAMED_STATE(Connected_St,       "DataCallSession::Connected");
CHART_NAMED_STATE(SendingStopReq_St,  "DataCallSession::SendingStopReq");
CHART_NAMED_STATE(WaitingForDown_St,  "DataCallSession::WaitingForDown");
CHART_NAMED_STATE(Reconciling_St,     "DataCallSession::Reconciling");
CHART_NAMED_STATE(Terminal_St,        "DataCallSession::Terminal");
CHART_NAMED_STATE(NotReady_St,        "DataConnectionManager::NotReady");
CHART_NAMED_STATE(Ready_St,           "DataConnectionManager::Ready");
CHART_NAMED_STATE(Operating_St,       "DataConnectionManager::Operating");

}  // namespace telux::data::simula

