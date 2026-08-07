// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "Phone.hpp"

#include "../common/Log.hpp"
#include "generated/cpp/topics.h"

#include <nlohmann/json.hpp>

namespace telux::tel::simula {

using common::simula::Envelope;

namespace {

constexpr auto kRpcTimeout = std::chrono::seconds(30);

std::string
operatingModeToWire(telux::tel::OperatingMode mode)
{
    switch (mode)
    {
        case telux::tel::OperatingMode::ONLINE:              return "ONLINE";
        case telux::tel::OperatingMode::AIRPLANE:            return "AIRPLANE";
        case telux::tel::OperatingMode::FACTORY_TEST:        return "FACTORY_TEST";
        case telux::tel::OperatingMode::OFFLINE:             return "OFFLINE";
        case telux::tel::OperatingMode::RESETTING:           return "RESETTING";
        case telux::tel::OperatingMode::SHUTTING_DOWN:       return "SHUTTING_DOWN";
        case telux::tel::OperatingMode::PERSISTENT_LOW_POWER: return "PERSISTENT_LOW_POWER";
        default:
            LOG_WARN(
              "[Phone] operatingModeToWire: unrecognized OperatingMode=%d, defaulting to ONLINE",
              static_cast<int>(mode)
            );
            return "ONLINE";
    }
}

telux::tel::VoiceServiceState
wireToVoiceServiceState(const std::string& s)
{
    if (s == "NOT_REG_AND_NOT_SEARCHING") return telux::tel::VoiceServiceState::NOT_REG_AND_NOT_SEARCHING;
    if (s == "REG_HOME")                  return telux::tel::VoiceServiceState::REG_HOME;
    if (s == "NOT_REG_AND_SEARCHING")     return telux::tel::VoiceServiceState::NOT_REG_AND_SEARCHING;
    if (s == "REG_DENIED")                return telux::tel::VoiceServiceState::REG_DENIED;
    if (s == "REG_ROAMING")               return telux::tel::VoiceServiceState::REG_ROAMING;
    if (s != "UNKNOWN")
        LOG_WARN(
          "[Phone] wireToVoiceServiceState: unrecognized value=%s, defaulting to UNKNOWN",
          s.c_str()
        );
    return telux::tel::VoiceServiceState::UNKNOWN;
}

telux::tel::RadioTechnology
wireToRadioTechnology(const std::string& s)
{
    if (s == "RADIO_TECH_GPRS")     return telux::tel::RadioTechnology::RADIO_TECH_GPRS;
    if (s == "RADIO_TECH_EDGE")     return telux::tel::RadioTechnology::RADIO_TECH_EDGE;
    if (s == "RADIO_TECH_UMTS")     return telux::tel::RadioTechnology::RADIO_TECH_UMTS;
    if (s == "RADIO_TECH_LTE")      return telux::tel::RadioTechnology::RADIO_TECH_LTE;
    if (s == "RADIO_TECH_GSM")      return telux::tel::RadioTechnology::RADIO_TECH_GSM;
    if (s == "RADIO_TECH_TD_SCDMA") return telux::tel::RadioTechnology::RADIO_TECH_TD_SCDMA;
    if (s == "RADIO_TECH_LTE_CA")   return telux::tel::RadioTechnology::RADIO_TECH_LTE_CA;
    if (s == "RADIO_TECH_NR5G")     return telux::tel::RadioTechnology::RADIO_TECH_NR5G;
    if (s != "RADIO_TECH_UNKNOWN")
        LOG_WARN(
          "[Phone] wireToRadioTechnology: unrecognized value=%s, defaulting to RADIO_TECH_UNKNOWN",
          s.c_str()
        );
    return telux::tel::RadioTechnology::RADIO_TECH_UNKNOWN;
}

telux::tel::CellType
wireToCellType(const std::string& s)
{
    if (s == "GSM")     return telux::tel::CellType::GSM;
    if (s == "CDMA")    return telux::tel::CellType::CDMA;
    if (s == "LTE")     return telux::tel::CellType::LTE;
    if (s == "WCDMA")   return telux::tel::CellType::WCDMA;
    if (s == "TDSCDMA") return telux::tel::CellType::TDSCDMA;
    if (s == "NR5G")    return telux::tel::CellType::NR5G;
    if (s != "NB1_NTN")
        LOG_WARN(
          "[Phone] wireToCellType: unrecognized value=%s, defaulting to NB1_NTN",
          s.c_str()
        );
    return telux::tel::CellType::NB1_NTN;
}

telux::common::BoolValue
wireToBoolValue(bool b)
{
    return b ? telux::common::BoolValue::STATE_TRUE : telux::common::BoolValue::STATE_FALSE;
}

std::shared_ptr<telux::tel::SignalStrength>
wireToSignalStrength(const nlohmann::json& data)
{
    const auto& g = data.at("gsm");
    auto gsm = std::make_shared<telux::tel::GsmSignalStrengthInfo>(
      g.value("gsmSignalStrength", 0), g.value("gsmBitErrorRate", 0), 0
    );
    const auto& w = data.at("wcdma");
    auto wcdma = std::make_shared<telux::tel::WcdmaSignalStrengthInfo>(
      w.value("signalStrength", 0), w.value("bitErrorRate", 0), w.value("ecio", 0), w.value("rscp", 0)
    );
    const auto& l = data.at("lte");
    auto lte = std::make_shared<telux::tel::LteSignalStrengthInfo>(
      l.value("lteSignalStrength", 0), l.value("lteRsrp", 0), l.value("lteRsrq", 0),
      l.value("lteRssnr", 0), l.value("lteCqi", 0), l.value("timingAdvance", 0)
    );
    const auto& n = data.at("nr5g");
    auto nr5g = std::make_shared<telux::tel::Nr5gSignalStrengthInfo>(
      n.value("rsrp", 0), n.value("rsrq", 0), n.value("rssnr", 0)
    );
    return std::make_shared<telux::tel::SignalStrength>(
      lte, gsm, nullptr, wcdma, nullptr, nr5g, nullptr
    );
}


std::string
ratToWireSigType(telux::tel::RadioTechnology radioTech)
{
    switch (radioTech)
    {
        case telux::tel::RadioTechnology::RADIO_TECH_GSM:  return "GSM_RSSI";
        case telux::tel::RadioTechnology::RADIO_TECH_UMTS: return "WCDMA_RSSI";
        case telux::tel::RadioTechnology::RADIO_TECH_LTE:  return "LTE_RSRP";
        case telux::tel::RadioTechnology::RADIO_TECH_NR5G: return "NR5G_RSRP";
        default:                                             return "LTE_RSRP";
    }
}

}  // namespace

// Declared in Phone.hpp (external linkage, unlike the helpers above) so
// PhoneManager.cpp's cell_info.ind handler can convert the indication's cell
// list the same way this file's own requestCellInfo() response handler does
// -- see cell_info_ind.json's description: this payload is defined to carry
// the whole refreshed list, mirroring the SDK's
// onCellInfoListChanged(vector<CellInfo>) callback contract.
std::shared_ptr<telux::tel::CellInfo>
wireToCellInfo(const nlohmann::json& j)
{
    auto cellType = wireToCellType(j.value("cellType", std::string()));
    int registered = j.value("registered", false) ? 1 : 0;
    switch (cellType)
    {
        case telux::tel::CellType::GSM:
        {

            if (!j.contains("gsm"))
            {
                LOG_WARN("[Phone] wireToCellInfo: missing \"gsm\" sub-object for cellType=GSM");
                return nullptr;
            }
            const auto& g = j.at("gsm");
            telux::tel::GsmCellIdentity id(
              g.value("mcc", std::string()), g.value("mnc", std::string()),
              g.value("lac", 0), g.value("cid", 0), g.value("arfcn", 0), g.value("bsic", 0)
            );
            telux::tel::GsmSignalStrengthInfo ss(
              g.value("signalStrength", 0), g.value("bitErrorRate", 0), 0
            );
            return std::make_shared<telux::tel::GsmCellInfo>(registered, id, ss);
        }
        case telux::tel::CellType::WCDMA:
        {
            if (!j.contains("wcdma"))
            {
                LOG_WARN("[Phone] wireToCellInfo: missing \"wcdma\" sub-object for cellType=WCDMA");
                return nullptr;
            }
            const auto& w = j.at("wcdma");
            telux::tel::WcdmaCellIdentity id(
              w.value("mcc", std::string()), w.value("mnc", std::string()),
              w.value("lac", 0), w.value("cid", 0), w.value("psc", 0), w.value("uarfcn", 0)
            );
            telux::tel::WcdmaSignalStrengthInfo ss(
              w.value("signalStrength", 0), w.value("bitErrorRate", 0),
              w.value("ecio", 0), w.value("rscp", 0)
            );
            return std::make_shared<telux::tel::WcdmaCellInfo>(registered, id, ss);
        }
        case telux::tel::CellType::LTE:
        {
            if (!j.contains("lte"))
            {
                LOG_WARN("[Phone] wireToCellInfo: missing \"lte\" sub-object for cellType=LTE");
                return nullptr;
            }
            const auto& l = j.at("lte");
            telux::tel::LteCellIdentity id(
              l.value("mcc", std::string()), l.value("mnc", std::string()),
              l.value("ci", 0), l.value("pci", 0), l.value("tac", 0), l.value("earfcn", 0)
            );
            telux::tel::LteSignalStrengthInfo ss(
              l.value("signalStrength", 0), l.value("rsrp", 0), l.value("rsrq", 0),
              l.value("rssnr", 0), l.value("cqi", 0), l.value("timingAdvance", 0)
            );
            return std::make_shared<telux::tel::LteCellInfo>(registered, id, ss);
        }
        case telux::tel::CellType::NR5G:
        {
            if (!j.contains("nr5g"))
            {
                LOG_WARN("[Phone] wireToCellInfo: missing \"nr5g\" sub-object for cellType=NR5G");
                return nullptr;
            }
            const auto& n = j.at("nr5g");
            telux::tel::Nr5gCellIdentity id(
              n.value("mcc", std::string()), n.value("mnc", std::string()),
              static_cast<uint64_t>(n.value("ci", 0)), static_cast<uint32_t>(n.value("pci", 0)),
              static_cast<int32_t>(n.value("tac", 0)), static_cast<int32_t>(n.value("arfcn", 0))
            );
            telux::tel::Nr5gSignalStrengthInfo ss(
              n.value("rsrp", 0), n.value("rsrq", 0), n.value("rssnr", 0)
            );
            return std::make_shared<telux::tel::Nr5gCellInfo>(registered, id, ss);
        }
        case telux::tel::CellType::CDMA:
        case telux::tel::CellType::TDSCDMA:
            LOG_WARN(
              "[Phone] wireToCellInfo: no wire representation for cellType=%s",
              j.value("cellType", std::string()).c_str()
            );
            return nullptr;
        default:
            return nullptr;
    }
}

SimulaPhone::SimulaPhone(int phoneId, common::simula::IModemBridge& bridge)
    : bridge_(bridge)
    , phoneId_(phoneId)
{}

SimulaPhone::~SimulaPhone() = default;

// ---------------------------------------------------------------------------
// telux::tel::IPhone

telux::common::Status
SimulaPhone::getPhoneId(int& phoneId)
{
    // The one field this forwarder can answer without a round trip -- it is
    // handed in at construction by SimulaPhoneManager::getPhone().
    phoneId = phoneId_;
    return telux::common::Status::SUCCESS;
}

telux::tel::RadioState
SimulaPhone::getRadioState()
{

    return telux::tel::RadioState::RADIO_STATE_ON;
}

telux::common::Status
SimulaPhone::requestVoiceRadioTechnology(telux::tel::VoiceRadioTechResponseCb callback)
{
    LOG_DEBUG(
      "[Phone] requestVoiceRadioTechnology phoneId=%d hasCallback=%d",
      phoneId_,
      callback ? 1 : 0
    );
    // Deprecated API, superseded by requestVoiceServiceState -- backed by
    // the same wire RPC, extracting just the radioTech field.
    nlohmann::json data = nlohmann::json::object();
    data["slot"] = phoneId_;
    auto req = common::simula::makeRequestEnvelope(bridge_.currentPaId(), std::move(data));
    LOG_DEBUG(
      "[Phone] requestVoiceRadioTechnology send_request topic=%s corrId=%s",
      topics::radio::request_voice_service_state::req,
      req.corrId.c_str()
    );
    bridge_.send_request(
      topics::radio::request_voice_service_state::req,
      "radio.request_voice_service_state.rsp",
      req,
      [callback, corrId = req.corrId](std::optional<Envelope> rsp) {
          if (!callback)
              return;
          if (!rsp || rsp->error || !rsp->data)
          {
              LOG_WARN(
                "[Phone] requestVoiceRadioTechnology response failed corrId=%s",
                corrId.c_str()
              );
              callback(telux::tel::RadioTechnology::RADIO_TECH_UNKNOWN,
                        telux::common::ErrorCode::OPERATION_TIMEOUT);
              return;
          }
          LOG_DEBUG(
            "[Phone] requestVoiceRadioTechnology response ok corrId=%s",
            corrId.c_str()
          );
          callback(
            wireToRadioTechnology(rsp->data->value("radioTech", std::string())),
            telux::common::ErrorCode::SUCCESS
          );
      },
      kRpcTimeout
    );
    return telux::common::Status::SUCCESS;
}

telux::tel::ServiceState
SimulaPhone::getServiceState()
{
    // Deprecated synchronous accessor; no cached local state (see class
    // header comment) -- IN_SERVICE matches the default seed
    // (voiceServiceState=REG_HOME).
    return telux::tel::ServiceState::IN_SERVICE;
}

telux::common::Status
SimulaPhone::requestVoiceServiceState(std::weak_ptr<telux::tel::IVoiceServiceStateCallback> callback)
{
    LOG_DEBUG(
      "[Phone] requestVoiceServiceState phoneId=%d listenerAlive=%d",
      phoneId_,
      callback.lock() ? 1 : 0
    );
    nlohmann::json data = nlohmann::json::object();
    data["slot"] = phoneId_;
    auto req = common::simula::makeRequestEnvelope(bridge_.currentPaId(), std::move(data));
    LOG_DEBUG(
      "[Phone] requestVoiceServiceState send_request topic=%s corrId=%s",
      topics::radio::request_voice_service_state::req,
      req.corrId.c_str()
    );
    bridge_.send_request(
      topics::radio::request_voice_service_state::req,
      "radio.request_voice_service_state.rsp",
      req,
      [callback, corrId = req.corrId](std::optional<Envelope> rsp) {
          auto cb = callback.lock();
          if (!cb)
              return;
          if (!rsp || rsp->error || !rsp->data)
          {
              LOG_WARN(
                "[Phone] requestVoiceServiceState response failed corrId=%s",
                corrId.c_str()
              );
              cb->voiceServiceStateResponse(nullptr, telux::common::ErrorCode::OPERATION_TIMEOUT);
              return;
          }
          LOG_DEBUG(
            "[Phone] requestVoiceServiceState response ok corrId=%s",
            corrId.c_str()
          );
          auto info = std::make_shared<telux::tel::VoiceServiceInfo>(
            wireToVoiceServiceState(rsp->data->value("voiceServiceState", std::string())),
            static_cast<telux::tel::VoiceServiceDenialCause>(rsp->data->value("denialCause", -1)),
            wireToRadioTechnology(rsp->data->value("radioTech", std::string()))
          );
          cb->voiceServiceStateResponse(info, telux::common::ErrorCode::SUCCESS);
      },
      kRpcTimeout
    );
    return telux::common::Status::SUCCESS;
}

telux::common::Status
SimulaPhone::setRadioPower(
  bool enable,
  std::shared_ptr<telux::common::ICommandResponseCallback> callback
)
{
    LOG_DEBUG(
      "[Phone] setRadioPower phoneId=%d enable=%d hasCallback=%d",
      phoneId_,
      enable ? 1 : 0,
      callback ? 1 : 0
    );

    nlohmann::json data = nlohmann::json::object();
    data["mode"] = operatingModeToWire(
      enable ? telux::tel::OperatingMode::ONLINE : telux::tel::OperatingMode::AIRPLANE
    );
    data["slot"] = phoneId_;
    auto req = common::simula::makeRequestEnvelope(bridge_.currentPaId(), std::move(data));
    LOG_DEBUG(
      "[Phone] setRadioPower send_request topic=%s corrId=%s",
      topics::radio::set_operating_mode::req,
      req.corrId.c_str()
    );
    bridge_.send_request(
      topics::radio::set_operating_mode::req,
      "radio.set_operating_mode.rsp",
      req,
      [callback, corrId = req.corrId](std::optional<Envelope> rsp) {
          if (!callback)
              return;
          if (!rsp || rsp->error)
          {
              LOG_WARN("[Phone] setRadioPower response failed corrId=%s", corrId.c_str());
              callback->commandResponse(telux::common::ErrorCode::OPERATION_TIMEOUT);
              return;
          }
          LOG_DEBUG("[Phone] setRadioPower response ok corrId=%s", corrId.c_str());
          callback->commandResponse(telux::common::ErrorCode::SUCCESS);
      },
      kRpcTimeout
    );
    return telux::common::Status::SUCCESS;
}

telux::common::Status
SimulaPhone::requestCellInfo(telux::tel::CellInfoCallback callback)
{
    LOG_DEBUG("[Phone] requestCellInfo phoneId=%d hasCallback=%d", phoneId_, callback ? 1 : 0);
    nlohmann::json data = nlohmann::json::object();
    data["slot"] = phoneId_;
    auto req = common::simula::makeRequestEnvelope(bridge_.currentPaId(), std::move(data));
    LOG_DEBUG(
      "[Phone] requestCellInfo send_request topic=%s corrId=%s",
      topics::radio::request_cell_info::req,
      req.corrId.c_str()
    );
    bridge_.send_request(
      topics::radio::request_cell_info::req,
      "radio.request_cell_info.rsp",
      req,
      [callback, corrId = req.corrId](std::optional<Envelope> rsp) {
          if (!callback)
              return;
          std::vector<std::shared_ptr<telux::tel::CellInfo>> cells;
          if (!rsp || rsp->error || !rsp->data)
          {
              LOG_WARN("[Phone] requestCellInfo response failed corrId=%s", corrId.c_str());
              callback(cells, telux::common::ErrorCode::OPERATION_TIMEOUT);
              return;
          }
          for (const auto& c : rsp->data->value("cells", nlohmann::json::array()))
          {
              auto cell = wireToCellInfo(c);
              if (cell)
                  cells.push_back(cell);
          }
          LOG_DEBUG(
            "[Phone] requestCellInfo response ok corrId=%s cells=%zu",
            corrId.c_str(),
            cells.size()
          );
          callback(cells, telux::common::ErrorCode::SUCCESS);
      },
      kRpcTimeout
    );
    return telux::common::Status::SUCCESS;
}

telux::common::Status
SimulaPhone::setCellInfoListRate(uint32_t /*timeInterval*/, telux::common::ResponseCallback callback)
{
    LOG_DEBUG(
      "[Phone] setCellInfoListRate phoneId=%d hasCallback=%d (no wire RPC, immediate ack)",
      phoneId_,
      callback ? 1 : 0
    );

    if (callback)
        callback(telux::common::ErrorCode::SUCCESS);
    return telux::common::Status::SUCCESS;
}

telux::common::Status
SimulaPhone::requestSignalStrength(std::shared_ptr<telux::tel::ISignalStrengthCallback> callback)
{
    LOG_DEBUG(
      "[Phone] requestSignalStrength phoneId=%d hasCallback=%d",
      phoneId_,
      callback ? 1 : 0
    );
    nlohmann::json data = nlohmann::json::object();
    data["slot"] = phoneId_;
    auto req = common::simula::makeRequestEnvelope(bridge_.currentPaId(), std::move(data));
    LOG_DEBUG(
      "[Phone] requestSignalStrength send_request topic=%s corrId=%s",
      topics::radio::request_signal_strength::req,
      req.corrId.c_str()
    );
    bridge_.send_request(
      topics::radio::request_signal_strength::req,
      "radio.request_signal_strength.rsp",
      req,
      [callback, corrId = req.corrId](std::optional<Envelope> rsp) {
          if (!callback)
              return;
          if (!rsp || rsp->error || !rsp->data)
          {
              LOG_WARN("[Phone] requestSignalStrength response failed corrId=%s", corrId.c_str());
              callback->signalStrengthResponse(nullptr, telux::common::ErrorCode::OPERATION_TIMEOUT);
              return;
          }
          LOG_DEBUG("[Phone] requestSignalStrength response ok corrId=%s", corrId.c_str());
          callback->signalStrengthResponse(
            wireToSignalStrength(*rsp->data), telux::common::ErrorCode::SUCCESS
          );
      },
      kRpcTimeout
    );
    return telux::common::Status::SUCCESS;
}

telux::common::Status
SimulaPhone::setECallOperatingMode(telux::tel::ECallMode /*eCallMode*/, telux::common::ResponseCallback callback)
{
    LOG_DEBUG(
      "[Phone] setECallOperatingMode phoneId=%d hasCallback=%d (not supported)",
      phoneId_,
      callback ? 1 : 0
    );
    // eCall is out of scope for the requested radio API list.
    if (callback)
        callback(telux::common::ErrorCode::NOT_SUPPORTED);
    return telux::common::Status::NOTSUPPORTED;
}

telux::common::Status
SimulaPhone::requestECallOperatingMode(telux::tel::ECallGetOperatingModeCallback callback)
{
    LOG_DEBUG(
      "[Phone] requestECallOperatingMode phoneId=%d hasCallback=%d (not supported)",
      phoneId_,
      callback ? 1 : 0
    );
    if (callback)
        callback(telux::tel::ECallMode::NONE, telux::common::ErrorCode::NOT_SUPPORTED);
    return telux::common::Status::NOTSUPPORTED;
}

telux::common::Status
SimulaPhone::requestOperatorName(telux::tel::OperatorNameCallback callback)
{
    LOG_DEBUG("[Phone] requestOperatorName phoneId=%d hasCallback=%d", phoneId_, callback ? 1 : 0);
    // Deprecated name-only path; the PA uses requestOperatorInfo, but both
    // read the same wire RPC.
    nlohmann::json data = nlohmann::json::object();
    data["slot"] = phoneId_;
    auto req = common::simula::makeRequestEnvelope(bridge_.currentPaId(), std::move(data));
    LOG_DEBUG(
      "[Phone] requestOperatorName send_request topic=%s corrId=%s",
      topics::radio::request_operator_info::req,
      req.corrId.c_str()
    );
    bridge_.send_request(
      topics::radio::request_operator_info::req,
      "radio.request_operator_info.rsp",
      req,
      [callback, corrId = req.corrId](std::optional<Envelope> rsp) {
          if (!callback)
              return;
          if (!rsp || rsp->error || !rsp->data)
          {
              LOG_WARN("[Phone] requestOperatorName response failed corrId=%s", corrId.c_str());
              callback("", "", telux::common::ErrorCode::OPERATION_TIMEOUT);
              return;
          }
          LOG_DEBUG("[Phone] requestOperatorName response ok corrId=%s", corrId.c_str());
          callback(
            rsp->data->value("longName", std::string()),
            rsp->data->value("shortName", std::string()),
            telux::common::ErrorCode::SUCCESS
          );
      },
      kRpcTimeout
    );
    return telux::common::Status::SUCCESS;
}

telux::common::Status
SimulaPhone::requestOperatorInfo(telux::tel::OperatorInfoCallback callback)
{
    LOG_DEBUG("[Phone] requestOperatorInfo phoneId=%d hasCallback=%d", phoneId_, callback ? 1 : 0);
    // Backs taf_pa_radio_GetCurrNetworkName, i.e. taf_radio_GetCurrentNetworkName
    // (shortName) / GetCurrentNetworkLongName (longName).
    nlohmann::json data = nlohmann::json::object();
    data["slot"] = phoneId_;
    auto req = common::simula::makeRequestEnvelope(bridge_.currentPaId(), std::move(data));
    LOG_DEBUG(
      "[Phone] requestOperatorInfo send_request topic=%s corrId=%s",
      topics::radio::request_operator_info::req,
      req.corrId.c_str()
    );
    bridge_.send_request(
      topics::radio::request_operator_info::req,
      "radio.request_operator_info.rsp",
      req,
      [callback, corrId = req.corrId](std::optional<Envelope> rsp) {
          if (!callback)
              return;
          telux::tel::PlmnInfo info{};
          if (!rsp || rsp->error || !rsp->data)
          {
              LOG_WARN("[Phone] requestOperatorInfo response failed corrId=%s", corrId.c_str());
              callback(info, telux::common::ErrorCode::OPERATION_TIMEOUT);
              return;
          }
          info.longName = rsp->data->value("longName", std::string());
          info.shortName = rsp->data->value("shortName", std::string());
          info.plmn = rsp->data->value("plmn", std::string());
          info.isHome = wireToBoolValue(rsp->data->value("isHome", false));
          LOG_DEBUG("[Phone] requestOperatorInfo response ok corrId=%s", corrId.c_str());
          callback(info, telux::common::ErrorCode::SUCCESS);
      },
      kRpcTimeout
    );
    return telux::common::Status::SUCCESS;
}

telux::common::Status
SimulaPhone::configureSignalStrength(
  std::vector<telux::tel::SignalStrengthConfig> /*signalStrengthConfig*/,
  telux::common::ResponseCallback callback
)
{
    LOG_DEBUG(
      "[Phone] configureSignalStrength(deprecated) phoneId=%d hasCallback=%d (immediate ack)",
      phoneId_,
      callback ? 1 : 0
    );
 
    if (callback)
        callback(telux::common::ErrorCode::SUCCESS);
    return telux::common::Status::SUCCESS;
}

telux::common::Status
SimulaPhone::configureSignalStrength(
  std::vector<telux::tel::SignalStrengthConfigEx> signalStrengthConfigEx,
  uint16_t hysteresisMs,
  telux::common::ResponseCallback callback
)
{
    LOG_DEBUG(
      "[Phone] configureSignalStrength phoneId=%d configCount=%zu hysteresisMs=%u hasCallback=%d",
      phoneId_,
      signalStrengthConfigEx.size(),
      static_cast<unsigned int>(hysteresisMs),
      callback ? 1 : 0
    );
 
    nlohmann::json data = nlohmann::json::object();
    if (!signalStrengthConfigEx.empty())
    {
        const auto& cfg = signalStrengthConfigEx.front();
        data["ratSigType"] = ratToWireSigType(cfg.radioTech);
        if (!cfg.sigConfigData.empty())
        {
            const auto& sd = cfg.sigConfigData.front();
            if (cfg.configTypeMask.test(
                  static_cast<size_t>(telux::tel::SignalStrengthConfigExType::DELTA)
                ))
                data["delta"] = sd.delta;
            if (cfg.configTypeMask.test(
                  static_cast<size_t>(telux::tel::SignalStrengthConfigExType::THRESHOLD)
                ))
            {
                nlohmann::json thresholds = nlohmann::json::array();
                for (auto t : sd.thresholdList)
                    thresholds.push_back(t);
                data["thresholds"] = std::move(thresholds);
                if (cfg.configTypeMask.test(
                      static_cast<size_t>(telux::tel::SignalStrengthConfigExType::HYSTERESIS_DB)
                    ))
                    data["hysteresisDb"] = sd.hysteresisDb;
            }
        }
    }
    else
    {
        data["ratSigType"] = "LTE_RSRP";
    }
    if (hysteresisMs != 0)
        data["hysteresisMs"] = hysteresisMs;
    data["slot"] = phoneId_;

    auto req = common::simula::makeRequestEnvelope(bridge_.currentPaId(), std::move(data));
    LOG_DEBUG(
      "[Phone] configureSignalStrength send_request topic=%s corrId=%s",
      topics::radio::configure_signal_strength::req,
      req.corrId.c_str()
    );
    bridge_.send_request(
      topics::radio::configure_signal_strength::req,
      "radio.configure_signal_strength.rsp",
      req,
      [callback, corrId = req.corrId](std::optional<Envelope> rsp) {
          if (!callback)
              return;
          if (!rsp || rsp->error)
          {
              LOG_WARN(
                "[Phone] configureSignalStrength response failed corrId=%s",
                corrId.c_str()
              );
              callback(telux::common::ErrorCode::OPERATION_TIMEOUT);
              return;
          }
          LOG_DEBUG("[Phone] configureSignalStrength response ok corrId=%s", corrId.c_str());
          callback(telux::common::ErrorCode::SUCCESS);
      },
      kRpcTimeout
    );
    return telux::common::Status::SUCCESS;
}

}  // namespace telux::tel::simula
