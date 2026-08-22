// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "DataProfileManager.hpp"

#include "../common/EventCast.hpp"
#include "../common/ListenerDispatchAO.hpp"
#include "Signals.hpp"
#include "generated/cpp/topics.h"

#include <chart/event.hpp>
#include <chart/hsm.hpp>
#include <chart/spy.hpp>
#include <future>
#include <nlohmann/json.hpp>
#include <telux/data/DataProfile.hpp>

namespace telux::data::simula {

using namespace DataSignals;

namespace {

using common::simula::Envelope;

std::string
authTypeToWire(telux::data::AuthProtocolType t)
{
    switch (t)
    {
        case telux::data::AuthProtocolType::AUTH_NONE:     return "AUTH_NONE";
        case telux::data::AuthProtocolType::AUTH_PAP:      return "AUTH_PAP";
        case telux::data::AuthProtocolType::AUTH_CHAP:     return "AUTH_CHAP";
        case telux::data::AuthProtocolType::AUTH_PAP_CHAP: return "AUTH_PAP_CHAP";
        default: return "AUTH_NONE";
    }
}

telux::data::AuthProtocolType
wireToAuthType(const std::string& s)
{
    if (s == "AUTH_PAP")      return telux::data::AuthProtocolType::AUTH_PAP;
    if (s == "AUTH_CHAP")     return telux::data::AuthProtocolType::AUTH_CHAP;
    if (s == "AUTH_PAP_CHAP") return telux::data::AuthProtocolType::AUTH_PAP_CHAP;
    return telux::data::AuthProtocolType::AUTH_NONE;
}

// Profile queries: UNKNOWN means "not specified" — the profile just doesn't
// constrain IP family, so MPSS reports the stored value verbatim.
std::string
ipFamilyToWire(telux::data::IpFamilyType f)
{
    switch (f)
    {
        case telux::data::IpFamilyType::IPV4:   return "IPV4";
        case telux::data::IpFamilyType::IPV6:   return "IPV6";
        case telux::data::IpFamilyType::IPV4V6: return "IPV4V6";
        default: return "UNKNOWN";
    }
}

telux::data::IpFamilyType
wireToIpFamily(const std::string& s)
{
    if (s == "IPV4")   return telux::data::IpFamilyType::IPV4;
    if (s == "IPV6")   return telux::data::IpFamilyType::IPV6;
    if (s == "IPV4V6") return telux::data::IpFamilyType::IPV4V6;
    return telux::data::IpFamilyType::UNKNOWN;
}

std::string
techPrefToWire(telux::data::TechPreference t)
{
    switch (t)
    {
        case telux::data::TechPreference::TP_3GPP:  return "TP_3GPP";
        case telux::data::TechPreference::TP_3GPP2: return "TP_3GPP2";
        case telux::data::TechPreference::TP_ANY:   return "TP_ANY";
        default: return "UNKNOWN";
    }
}

telux::data::TechPreference
wireToTechPref(const std::string& s)
{
    if (s == "TP_3GPP")  return telux::data::TechPreference::TP_3GPP;
    if (s == "TP_3GPP2") return telux::data::TechPreference::TP_3GPP2;
    if (s == "TP_ANY")   return telux::data::TechPreference::TP_ANY;
    return telux::data::TechPreference::UNKNOWN;
}

std::string
emergencyToWire(telux::data::EmergencyCapability c)
{
    switch (c)
    {
        case telux::data::EmergencyCapability::ALLOWED:     return "ALLOWED";
        case telux::data::EmergencyCapability::NOT_ALLOWED: return "NOT_ALLOWED";
        default: return "UNSPECIFIED";
    }
}

telux::data::EmergencyCapability
wireToEmergency(const std::string& s)
{
    if (s == "ALLOWED")     return telux::data::EmergencyCapability::ALLOWED;
    if (s == "NOT_ALLOWED") return telux::data::EmergencyCapability::NOT_ALLOWED;
    return telux::data::EmergencyCapability::UNSPECIFIED;
}

// ApnTypes is a 16-bit bitset (DataDefines.hpp's ApnMaskType). No per-bit
// wire schema exists yet -- encoding the raw integer value is the simplest
// lossless wire form until individual bits need to be named on the wire.
nlohmann::json
profileParamsToWire(const telux::data::ProfileParams& p)
{
    nlohmann::json j = nlohmann::json::object();
    j["profileName"] = p.profileName;
    j["apn"] = p.apn;
    j["userName"] = p.userName;
    j["password"] = p.password;
    j["techPref"] = techPrefToWire(p.techPref);
    j["authType"] = authTypeToWire(p.authType);
    j["ipFamilyType"] = ipFamilyToWire(p.ipFamilyType);
    j["apnTypes"] = static_cast<unsigned long>(p.apnTypes.to_ulong());
    j["emergencyAllowed"] = emergencyToWire(p.emergencyAllowed);
    j["clatEnabled"] = p.clatEnabled;
    return j;
}

std::shared_ptr<telux::data::DataProfile>
wireToDataProfile(const nlohmann::json& j)
{
    return std::make_shared<telux::data::DataProfile>(
      j.value("id", telux::data::DataProfile::PROFILE_ID_INVALID),
      j.value("profileName", std::string()),
      j.value("apn", std::string()),
      j.value("userName", std::string()),
      j.value("password", std::string()),
      wireToIpFamily(j.value("ipFamilyType", std::string())),
      wireToTechPref(j.value("techPref", std::string())),
      wireToAuthType(j.value("authType", std::string())),
      telux::data::ApnTypes(j.value("apnTypes", 0ul)),
      wireToEmergency(j.value("emergencyAllowed", std::string())),
      j.value("clatEnabled", false)
    );
}

std::vector<std::shared_ptr<telux::data::DataProfile>>
wireToDataProfileList(const nlohmann::json& data)
{
    std::vector<std::shared_ptr<telux::data::DataProfile>> out;
    if (data.contains("profiles") && data["profiles"].is_array())
    {
        for (const auto& j : data["profiles"])
            out.push_back(wireToDataProfile(j));
    }
    return out;
}

constexpr auto kRpcTimeout = std::chrono::seconds(30);

struct RpcResultPld
{
    std::optional<Envelope> rsp;
};

struct StateIndPld
{
    Envelope env;
};

struct RequestProfileListPld
{
    std::shared_ptr<telux::data::IDataProfileListCallback> cb;
};

struct CreateProfilePld
{
    telux::data::ProfileParams params;
    std::shared_ptr<telux::data::IDataCreateProfileCallback> cb;
};

struct DeleteProfilePld
{
    uint8_t profileId;
    telux::data::TechPreference techPreference;
    std::shared_ptr<telux::common::ICommandResponseCallback> cb;
};

struct ModifyProfilePld
{
    uint8_t profileId;
    telux::data::ProfileParams params;
    std::shared_ptr<telux::common::ICommandResponseCallback> cb;
};

struct QueryProfilePld
{
    telux::data::ProfileParams params;
    std::shared_ptr<telux::data::IDataProfileListCallback> cb;
};

struct RequestProfilePld
{
    uint8_t profileId;
    telux::data::TechPreference techPreference;
    std::shared_ptr<telux::data::IDataProfileCallback> cb;
};

using telux::common::simula::event_cast;

}  // namespace

chart::Status
ProfileNotReady_St(chart::Hsm*, chart::Event const*);
chart::Status
ProfileReady_St(chart::Hsm*, chart::Event const*);

SimulaDataProfileManager::SimulaDataProfileManager(
  SlotId slotId,
  common::simula::IModemBridge& bridge,
  telux::common::InitResponseCb initCb
)
    : chart::ActiveObject("DataProfileManager")
    , bridge_(bridge)
    , slotId_(slotId)
    , init_cb_(std::move(initCb))
{}

SimulaDataProfileManager::~SimulaDataProfileManager()
{
    // Withdraw from the bridge before anything else: the callbacks below
    // capture raw `this` and the bridge holds its own copies. unsubscribe_*
    // is only a queued removal, so the drain() fence inside is what actually
    // makes "no callback can reach us" true.
    unsubscribeFromBridge_();
    stop();
}

void
SimulaDataProfileManager::unsubscribeFromBridge_()
{
    bridge_.unsubscribe_event(topics::data::subsys_ready_profile::ind);
    bridge_.unsubscribe_event(topics::data::profile_changed::ind);
    bridge_.unsubscribe_connectivity(conn_token_);
    conn_token_ = 0;
    bridge_.drain();
}

void
SimulaDataProfileManager::start()
{
    if (running())
        return;
    set_instrument(std::make_unique<chart::SpyInstrument>(name()));
    start_at(ProfileNotReady_St);

    bridge_.subscribe_event(
      topics::data::subsys_ready_profile::ind,
      "data.subsys_ready_profile.ind",
      [this](std::string_view topic, const Envelope& env) { handleReadinessInd_(topic, env); }
    );
    bridge_.subscribe_event(
      topics::data::profile_changed::ind,
      "data.profile_changed.ind",
      [this](std::string_view topic, const Envelope& env) { handleProfileChangedInd_(topic, env); }
    );
    conn_token_ = bridge_.subscribe_connectivity([this](bool operational) {
        auto pld = std::make_shared<bool>(operational);
        post_fifo({ BridgeConnectivityChanged_Signal, pld });
    });
}

void
SimulaDataProfileManager::handleReadinessInd_(std::string_view /*topic*/, const Envelope& env)
{
    auto pld = std::make_shared<StateIndPld>();
    pld->env = env;
    post_fifo({ ReadinessEvt_Signal, pld });
}

void
SimulaDataProfileManager::handleProfileChangedInd_(std::string_view /*topic*/, const Envelope& env)
{
    auto pld = std::make_shared<StateIndPld>();
    pld->env = env;
    post_fifo({ ProfileChangedEvt_Signal, pld });
}

void
SimulaDataProfileManager::broadcastToListeners_(
  std::function<void(const std::shared_ptr<telux::data::IDataProfileListener>&)> invoke
)
{
    auto task = std::make_shared<common::simula::DispatchTask>();
    task->debug_tag = "DataProfileManager::broadcastToListeners_";
    {
        std::lock_guard<std::mutex> lk(listeners_mutex_);
        for (auto& weak : listeners_)
        {
            if (auto sp = weak.lock())
                task->listeners.push_back(sp);
        }
    }
    if (task->listeners.empty())
        return;
    task->invoker = [invoke](std::shared_ptr<void> raw) {
        invoke(std::static_pointer_cast<telux::data::IDataProfileListener>(raw));
    };
    common::simula::ListenerDispatchAO::instance().enqueue(std::move(task));
}

// ---------------------------------------------------------------------------
// telux::data::IDataProfileManager

telux::common::ServiceStatus
SimulaDataProfileManager::getServiceStatus()
{
    return last_status_.load();
}

bool
SimulaDataProfileManager::isReadyDerived_() const
{
    // Chart-derived readiness: sole authority is the chart's current-state
    // pointer. ProfileReady_St == Ready, everything else == NotReady.
    return const_cast<SimulaDataProfileManager*>(this)->current_state() == ProfileReady_St;
}

void
SimulaDataProfileManager::publishStatus_(telux::common::ServiceStatus s)
{
    last_status_.store(s);
}

bool
SimulaDataProfileManager::isSubsystemReady()
{
    return isReadyDerived_();
}

std::future<bool>
SimulaDataProfileManager::onSubsystemReady()
{
    std::promise<bool> p;
    p.set_value(isReadyDerived_());
    return p.get_future();
}

telux::common::Status
SimulaDataProfileManager::requestProfileList(
  std::shared_ptr<telux::data::IDataProfileListCallback> callback
)
{
    if (!isReadyDerived_())
        return telux::common::Status::NOTREADY;
    auto pld = std::make_shared<RequestProfileListPld>();
    pld->cb = std::move(callback);
    post_fifo({ RequestProfileList_Signal, pld });
    return telux::common::Status::SUCCESS;
}

telux::common::Status
SimulaDataProfileManager::createProfile(
  const telux::data::ProfileParams& profileParams,
  std::shared_ptr<telux::data::IDataCreateProfileCallback> callback
)
{
    if (!isReadyDerived_())
        return telux::common::Status::NOTREADY;
    auto pld = std::make_shared<CreateProfilePld>();
    pld->params = profileParams;
    pld->cb = std::move(callback);
    post_fifo({ CreateProfile_Signal, pld });
    return telux::common::Status::SUCCESS;
}

telux::common::Status
SimulaDataProfileManager::deleteProfile(
  uint8_t profileId,
  telux::data::TechPreference techPreference,
  std::shared_ptr<telux::common::ICommandResponseCallback> callback
)
{
    if (!isReadyDerived_())
        return telux::common::Status::NOTREADY;
    auto pld = std::make_shared<DeleteProfilePld>();
    pld->profileId = profileId;
    pld->techPreference = techPreference;
    pld->cb = std::move(callback);
    post_fifo({ DeleteProfile_Signal, pld });
    return telux::common::Status::SUCCESS;
}

telux::common::Status
SimulaDataProfileManager::modifyProfile(
  uint8_t profileId,
  const telux::data::ProfileParams& profileParams,
  std::shared_ptr<telux::common::ICommandResponseCallback> callback
)
{
    if (!isReadyDerived_())
        return telux::common::Status::NOTREADY;
    auto pld = std::make_shared<ModifyProfilePld>();
    pld->profileId = profileId;
    pld->params = profileParams;
    pld->cb = std::move(callback);
    post_fifo({ ModifyProfile_Signal, pld });
    return telux::common::Status::SUCCESS;
}

telux::common::Status
SimulaDataProfileManager::queryProfile(
  const telux::data::ProfileParams& profileParams,
  std::shared_ptr<telux::data::IDataProfileListCallback> callback
)
{
    if (!isReadyDerived_())
        return telux::common::Status::NOTREADY;
    auto pld = std::make_shared<QueryProfilePld>();
    pld->params = profileParams;
    pld->cb = std::move(callback);
    post_fifo({ QueryProfile_Signal, pld });
    return telux::common::Status::SUCCESS;
}

telux::common::Status
SimulaDataProfileManager::requestProfile(
  uint8_t profileId,
  telux::data::TechPreference techPreference,
  std::shared_ptr<telux::data::IDataProfileCallback> callback
)
{
    if (!isReadyDerived_())
        return telux::common::Status::NOTREADY;
    auto pld = std::make_shared<RequestProfilePld>();
    pld->profileId = profileId;
    pld->techPreference = techPreference;
    pld->cb = std::move(callback);
    // On the wire this reuses kReqQueryProfile ("get one profile by id" is
    // "query filtered down to one result" at the RPC layer -- no separate
    // topic exists for this real-SDK method), but posts its own signal so
    // the AO-side handler doesn't have to guess RequestProfilePld vs
    // QueryProfilePld from an untyped payload.
    post_fifo({ RequestProfile_Signal, pld });
    return telux::common::Status::SUCCESS;
}

int
SimulaDataProfileManager::getSlotId()
{
    // IDataProfileManager::getSlotId() returns plain int (unlike
    // IServingSystemManager's, which returns SlotId) -- verified against
    // the real header, not a typo.
    return static_cast<int>(slotId_);
}

telux::common::Status
SimulaDataProfileManager::registerListener(std::weak_ptr<telux::data::IDataProfileListener> listener)
{
    std::lock_guard<std::mutex> lk(listeners_mutex_);
    listeners_.push_back(std::move(listener));
    return telux::common::Status::SUCCESS;
}

telux::common::Status
SimulaDataProfileManager::deregisterListener(
  std::weak_ptr<telux::data::IDataProfileListener> listener
)
{
    std::lock_guard<std::mutex> lk(listeners_mutex_);
    auto target = listener.lock();
    listeners_.erase(
      std::remove_if(
        listeners_.begin(),
        listeners_.end(),
        [&](const std::weak_ptr<telux::data::IDataProfileListener>& w) {
            auto sp = w.lock();
            return !sp || (target && sp == target);
        }
      ),
      listeners_.end()
    );
    return telux::common::Status::SUCCESS;
}

// ---------------------------------------------------------------------------
// State handlers

// Composite parent shell for {ProfileNotReady, ProfileReady}. No cross-child
// deduplicable behavior exists in this Manager today (both children handle
// BridgeConnectivityChanged_Signal differently by design, and RPC-timeout
// handling is per-signal-callback rather than state-level), so this is a
chart::Status
ProfileNotReady_St(chart::Hsm* h, chart::Event const* e)
{
    auto* self = static_cast<SimulaDataProfileManager*>(h);
    switch (e->sig)
    {
        case chart::Entry_Signal:
        case chart::Exit_Signal:
            return chart::Status::HANDLED;
        case ReadinessEvt_Signal:
        {
            auto pld = event_cast<StateIndPld>(*e);
            if (pld->env.data && pld->env.data->value("status", std::string()) == "AVAILABLE")
                return self->to(ProfileReady_St);
            return chart::Status::HANDLED;
        }
        case BridgeConnectivityChanged_Signal:
            return chart::Status::HANDLED;
        // API calls landed here synchronously return NOTREADY before ever
        // posting -- reachable only if stale-queued during a transition.
        case RequestProfileList_Signal:
        case CreateProfile_Signal:
        case DeleteProfile_Signal:
        case ModifyProfile_Signal:
        case QueryProfile_Signal:
        case RequestProfile_Signal:
            return chart::Status::HANDLED;
        default:
            return self->super(&chart::Hsm::top);
    }
}

chart::Status
ProfileReady_St(chart::Hsm* h, chart::Event const* e)
{
    auto* self = static_cast<SimulaDataProfileManager*>(h);
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
                  [](const std::shared_ptr<telux::data::IDataProfileListener>& l) {
                      l->onServiceStatusChange(telux::common::ServiceStatus::SERVICE_AVAILABLE);
                  }
                );
            }
            return chart::Status::HANDLED;
        }
        case chart::Exit_Signal:
            self->broadcastToListeners_(
              [](const std::shared_ptr<telux::data::IDataProfileListener>& l) {
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
                return self->to(ProfileNotReady_St);
            }
            if (state == "FAILED")
            {
                self->publishStatus_(telux::common::ServiceStatus::SERVICE_FAILED);
                return self->to(ProfileNotReady_St);
            }
            return chart::Status::HANDLED;
        }

        case BridgeConnectivityChanged_Signal:
        {
            auto pld = event_cast<bool>(*e);
            if (pld && !*pld)
            {
                self->publishStatus_(telux::common::ServiceStatus::SERVICE_UNAVAILABLE);
                return self->to(ProfileNotReady_St);
            }
            return chart::Status::HANDLED;
        }

        case ProfileChangedEvt_Signal:
        {
            auto pld = event_cast<StateIndPld>(*e);
            if (!pld->env.data)
                return chart::Status::HANDLED;
            int profileId = pld->env.data->value("profileId", -1);
            auto techPref = wireToTechPref(pld->env.data->value("techPref", std::string()));
            auto eventName = pld->env.data->value("event", std::string());
            telux::data::ProfileChangeEvent event = telux::data::ProfileChangeEvent::MODIFY_PROFILE_EVENT;
            if (eventName == "CREATE")
                event = telux::data::ProfileChangeEvent::CREATE_PROFILE_EVENT;
            else if (eventName == "DELETE")
                event = telux::data::ProfileChangeEvent::DELETE_PROFILE_EVENT;
            self->broadcastToListeners_(
              [profileId, techPref, event](const std::shared_ptr<telux::data::IDataProfileListener>& l) {
                  l->onProfileUpdate(profileId, techPref, event);
              }
            );
            return chart::Status::HANDLED;
        }

        case RequestProfileList_Signal:
        {
            auto pld = event_cast<RequestProfileListPld>(*e);
            // request_profile_list.req schema is strict (no filter fields):
            // MPSS lists all stored profiles unfiltered, slot is not read.
            nlohmann::json data = nlohmann::json::object();
            auto req = common::simula::makeRequestEnvelope(self->bridge_.currentPaId(), std::move(data));
            auto cb = pld->cb;
            self->bridge_.send_request(
              topics::data::request_profile_list::req,
              "data.request_profile_list.rsp",
              req,
              [cb](std::optional<Envelope> rsp) {
                  if (!cb)
                      return;
                  if (!rsp || rsp->error || !rsp->data)
                  {
                      cb->onProfileListResponse({}, telux::common::ErrorCode::OPERATION_TIMEOUT);
                      return;
                  }
                  cb->onProfileListResponse(
                    wireToDataProfileList(*rsp->data), telux::common::ErrorCode::SUCCESS
                  );
              },
              kRpcTimeout
            );
            return chart::Status::HANDLED;
        }

        case CreateProfile_Signal:
        {
            auto pld = event_cast<CreateProfilePld>(*e);
            nlohmann::json data = profileParamsToWire(pld->params);
            data["slot"] = static_cast<int>(self->slotId_);
            auto req = common::simula::makeRequestEnvelope(self->bridge_.currentPaId(), std::move(data));
            auto cb = pld->cb;
            self->bridge_.send_request(
              topics::data::create_profile::req,
              "data.create_profile.rsp",
              req,
              [cb](std::optional<Envelope> rsp) {
                  if (!cb)
                      return;
                  if (!rsp || rsp->error || !rsp->data)
                  {
                      cb->onResponse(
                        telux::data::DataProfile::PROFILE_ID_INVALID,
                        telux::common::ErrorCode::OPERATION_TIMEOUT
                      );
                      return;
                  }
                  cb->onResponse(
                    rsp->data->value("profileId", telux::data::DataProfile::PROFILE_ID_INVALID),
                    telux::common::ErrorCode::SUCCESS
                  );
              },
              kRpcTimeout
            );
            return chart::Status::HANDLED;
        }

        case DeleteProfile_Signal:
        {
            auto pld = event_cast<DeleteProfilePld>(*e);
            nlohmann::json data = nlohmann::json::object();
            data["profileId"] = pld->profileId;
            data["techPref"] = techPrefToWire(pld->techPreference);
            data["slot"] = static_cast<int>(self->slotId_);
            auto req = common::simula::makeRequestEnvelope(self->bridge_.currentPaId(), std::move(data));
            auto cb = pld->cb;
            self->bridge_.send_request(
              topics::data::delete_profile::req,
              "data.delete_profile.rsp",
              req,
              [cb](std::optional<Envelope> rsp) {
                  if (!cb)
                      return;
                  if (!rsp || rsp->error)
                  {
                      cb->commandResponse(telux::common::ErrorCode::OPERATION_TIMEOUT);
                      return;
                  }
                  cb->commandResponse(telux::common::ErrorCode::SUCCESS);
              },
              kRpcTimeout
            );
            return chart::Status::HANDLED;
        }

        case ModifyProfile_Signal:
        {
            auto pld = event_cast<ModifyProfilePld>(*e);
            nlohmann::json data = profileParamsToWire(pld->params);
            data["profileId"] = pld->profileId;
            data["slot"] = static_cast<int>(self->slotId_);
            auto req = common::simula::makeRequestEnvelope(self->bridge_.currentPaId(), std::move(data));
            auto cb = pld->cb;
            self->bridge_.send_request(
              topics::data::modify_profile::req,
              "data.modify_profile.rsp",
              req,
              [cb](std::optional<Envelope> rsp) {
                  if (!cb)
                      return;
                  if (!rsp || rsp->error)
                  {
                      cb->commandResponse(telux::common::ErrorCode::OPERATION_TIMEOUT);
                      return;
                  }
                  cb->commandResponse(telux::common::ErrorCode::SUCCESS);
              },
              kRpcTimeout
            );
            return chart::Status::HANDLED;
        }

        case QueryProfile_Signal:
        {
            auto pld = event_cast<QueryProfilePld>(*e);
            // query_profile.req is a strict whitelist (additionalProperties:
            // false) of filter fields — profileName, apn, techPref,
            // ipFamilyType. ProfileParams is the create/modify superset with
            // default-constructed empties for unused fields, so
            // profileParamsToWire() would emit userName/password/authType/
            // apnTypes/emergencyAllowed/clatEnabled and the request would
            // fail schema validation on MPSS -- queryProfile would never
            // succeed. Emit only fields the SDK caller actually populated.
            const auto& pp = pld->params;
            nlohmann::json data = nlohmann::json::object();
            if (!pp.profileName.empty()) data["profileName"] = pp.profileName;
            if (!pp.apn.empty())         data["apn"]         = pp.apn;
            if (pp.techPref != telux::data::TechPreference::UNKNOWN)
                data["techPref"] = techPrefToWire(pp.techPref);
            if (pp.ipFamilyType != telux::data::IpFamilyType::UNKNOWN)
                data["ipFamilyType"] = ipFamilyToWire(pp.ipFamilyType);
            auto req =
              common::simula::makeRequestEnvelope(self->bridge_.currentPaId(), std::move(data));
            auto cb = pld->cb;
            self->bridge_.send_request(
              topics::data::query_profile::req,
              "data.query_profile.rsp",
              req,
              [cb](std::optional<Envelope> rsp) {
                  if (!cb)
                      return;
                  if (!rsp || rsp->error || !rsp->data)
                  {
                      cb->onProfileListResponse({}, telux::common::ErrorCode::OPERATION_TIMEOUT);
                      return;
                  }
                  cb->onProfileListResponse(
                    wireToDataProfileList(*rsp->data), telux::common::ErrorCode::SUCCESS
                  );
              },
              kRpcTimeout
            );
            return chart::Status::HANDLED;
        }

        case RequestProfile_Signal:
        {
            auto pld = event_cast<RequestProfilePld>(*e);
            // query_profile.req schema is strict: profileId + techPref only, no slot.
            nlohmann::json data = nlohmann::json::object();
            data["profileId"] = pld->profileId;
            data["techPref"] = techPrefToWire(pld->techPreference);
            auto req =
              common::simula::makeRequestEnvelope(self->bridge_.currentPaId(), std::move(data));
            auto cb = pld->cb;
            self->bridge_.send_request(
              topics::data::query_profile::req,
              "data.query_profile.rsp",
              req,
              [cb](std::optional<Envelope> rsp) {
                  if (!cb)
                      return;
                  if (!rsp || rsp->error || !rsp->data)
                  {
                      cb->onResponse(nullptr, telux::common::ErrorCode::OPERATION_TIMEOUT);
                      return;
                  }
                  auto list = wireToDataProfileList(*rsp->data);
                  cb->onResponse(
                    list.empty() ? nullptr : list.front(), telux::common::ErrorCode::SUCCESS
                  );
              },
              kRpcTimeout
            );
            return chart::Status::HANDLED;
        }

        default:
            return self->super(&chart::Hsm::top);
    }
}

CHART_NAMED_STATE(ProfileNotReady_St,  "DataProfileManager::NotReady");
CHART_NAMED_STATE(ProfileReady_St,     "DataProfileManager::Ready");

}  // namespace telux::data::simula
