// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "Call.hpp"

#include "../common/Envelope.hpp"
#include "../common/Log.hpp"
#include "generated/cpp/topics.h"

#include <nlohmann/json.hpp>

namespace telux::tel::simula {

using common::simula::Envelope;

namespace {

constexpr auto kRpcTimeout = std::chrono::seconds(30);

// Issues end_ecall/answer_ecall directly

void
sendCallControlRequest(
  common::simula::IModemBridge& bridge,
  const char* topic,
  const char* rspSchemaId,
  int phoneId,
  int callIndex,
  std::shared_ptr<telux::common::ICommandResponseCallback> callback
)
{
    nlohmann::json data = nlohmann::json::object();
    data["phoneId"] = phoneId;
    data["callIndex"] = callIndex;
    auto req = common::simula::makeRequestEnvelope(bridge.currentPaId(), std::move(data));
    LOG_DEBUG(
      "[Call] sendCallControlRequest topic=%s phoneId=%d callIndex=%d corrId=%s",
      topic,
      phoneId,
      callIndex,
      req.corrId.c_str()
    );
    bridge.send_request(
      topic,
      rspSchemaId,
      req,
      [callback, corrId = req.corrId](std::optional<Envelope> rsp) {
          if (!callback)
              return;
          if (!rsp || rsp->error)
          {
              const auto error = rsp && rsp->error
                ? common::simula::parseErrorCode(rsp->error->value("code", std::string()))
                : telux::common::ErrorCode::OPERATION_TIMEOUT;
              LOG_WARN("[Call] request failed corrId=%s error=%d", corrId.c_str(),
                       static_cast<int>(error));
              callback->commandResponse(error);
              return;
          }
          LOG_DEBUG("[Call] request ok corrId=%s", corrId.c_str());
          callback->commandResponse(telux::common::ErrorCode::SUCCESS);
      },
      kRpcTimeout
    );
}

}  // namespace

SimulaCall::SimulaCall(
  int phoneId,
  int callIndex,
  telux::tel::CallDirection direction,
  std::string remotePartyNumber,
  common::simula::IModemBridge& bridge
)
    : bridge_(bridge)
    , phoneId_(phoneId)
    , callIndex_(callIndex)
    , direction_(direction)
    , remotePartyNumber_(std::move(remotePartyNumber))
{}

SimulaCall::~SimulaCall() = default;

void
SimulaCall::setCallState(telux::tel::CallState state)
{
    std::lock_guard<std::mutex> lk(mutex_);
    LOG_DEBUG(
      "[Call] setCallState phoneId=%d callIndex=%d %d -> %d", phoneId_, callIndex_,
      static_cast<int>(state_), static_cast<int>(state)
    );
    state_ = state;
}

void
SimulaCall::setCallEndCause(telux::tel::CallEndCause cause)
{
    std::lock_guard<std::mutex> lk(mutex_);
    LOG_DEBUG(
      "[Call] setCallEndCause phoneId=%d callIndex=%d %d -> %d", phoneId_, callIndex_,
      static_cast<int>(endCause_), static_cast<int>(cause)
    );
    endCause_ = cause;
}

// ---------------------------------------------------------------------------
// telux::tel::ICall

telux::common::Status
SimulaCall::answer(
  std::shared_ptr<telux::common::ICommandResponseCallback> callback,
  telux::tel::RttMode /*mode*/
)
{
    LOG_DEBUG("[Call] answer phoneId=%d callIndex=%d", phoneId_, callIndex_);
    sendCallControlRequest(
      bridge_, topics::ecall::answer_ecall::req, "ecall.answer_ecall.rsp", phoneId_, callIndex_,
      callback
    );
    return telux::common::Status::SUCCESS;
}

telux::common::Status
SimulaCall::hold(std::shared_ptr<telux::common::ICommandResponseCallback> callback)
{
    LOG_DEBUG("[Call] hold not supported (out of eCall migration scope)");
    if (callback)
        callback->commandResponse(telux::common::ErrorCode::NOT_SUPPORTED);
    return telux::common::Status::NOTSUPPORTED;
}

telux::common::Status
SimulaCall::resume(std::shared_ptr<telux::common::ICommandResponseCallback> callback)
{
    LOG_DEBUG("[Call] resume not supported (out of eCall migration scope)");
    if (callback)
        callback->commandResponse(telux::common::ErrorCode::NOT_SUPPORTED);
    return telux::common::Status::NOTSUPPORTED;
}

telux::common::Status
SimulaCall::reject(std::shared_ptr<telux::common::ICommandResponseCallback> callback)
{
    // Mirrors taf_ecall's StopECall(): reject() and hangup() both terminate
    // the eCall server-side (end_ecall is the single "terminate" RPC) --
    // the choice of which ICall method to call is made by the (unchanged,
    // out-of-scope) tafECallImpl.cpp based on whether the call is INCOMING.
    LOG_DEBUG("[Call] reject phoneId=%d callIndex=%d", phoneId_, callIndex_);
    sendCallControlRequest(
      bridge_, topics::ecall::end_ecall::req, "ecall.end_ecall.rsp", phoneId_, callIndex_, callback
    );
    return telux::common::Status::SUCCESS;
}

telux::common::Status
SimulaCall::reject(
  const std::string& /*rejectSMS*/,
  std::shared_ptr<telux::common::ICommandResponseCallback> callback
)
{
    LOG_DEBUG("[Call] reject(rejectSMS) not supported (deprecated API, out of scope)");
    if (callback)
        callback->commandResponse(telux::common::ErrorCode::NOT_SUPPORTED);
    return telux::common::Status::NOTSUPPORTED;
}

telux::common::Status
SimulaCall::hangup(std::shared_ptr<telux::common::ICommandResponseCallback> callback)
{
    LOG_DEBUG("[Call] hangup phoneId=%d callIndex=%d", phoneId_, callIndex_);
    sendCallControlRequest(
      bridge_, topics::ecall::end_ecall::req, "ecall.end_ecall.rsp", phoneId_, callIndex_, callback
    );
    return telux::common::Status::SUCCESS;
}

telux::common::Status
SimulaCall::playDtmfTone(char, std::shared_ptr<telux::common::ICommandResponseCallback> callback)
{
    if (callback)
        callback->commandResponse(telux::common::ErrorCode::NOT_SUPPORTED);
    return telux::common::Status::NOTSUPPORTED;
}

telux::common::Status
SimulaCall::startDtmfTone(char, std::shared_ptr<telux::common::ICommandResponseCallback> callback)
{
    if (callback)
        callback->commandResponse(telux::common::ErrorCode::NOT_SUPPORTED);
    return telux::common::Status::NOTSUPPORTED;
}

telux::common::Status
SimulaCall::stopDtmfTone(std::shared_ptr<telux::common::ICommandResponseCallback> callback)
{
    if (callback)
        callback->commandResponse(telux::common::ErrorCode::NOT_SUPPORTED);
    return telux::common::Status::NOTSUPPORTED;
}

telux::tel::CallState
SimulaCall::getCallState()
{
    std::lock_guard<std::mutex> lk(mutex_);
    return state_;
}

int
SimulaCall::getCallIndex()
{
    return callIndex_;
}

telux::tel::CallDirection
SimulaCall::getCallDirection()
{
    return direction_;
}

std::string
SimulaCall::getRemotePartyNumber()
{
    return remotePartyNumber_;
}

telux::tel::CallEndCause
SimulaCall::getCallEndCause()
{
    std::lock_guard<std::mutex> lk(mutex_);
    return endCause_;
}

int
SimulaCall::getSipErrorCode()
{
    return 0;
}

int
SimulaCall::getPhoneId()
{
    return phoneId_;
}

bool
SimulaCall::isMultiPartyCall()
{
    return false;
}

telux::tel::RttMode
SimulaCall::getRttMode()
{
    return telux::tel::RttMode::DISABLED;
}

telux::tel::RttMode
SimulaCall::getLocalRttCapability()
{
    return telux::tel::RttMode::DISABLED;
}

telux::tel::RttMode
SimulaCall::getPeerRttCapability()
{
    return telux::tel::RttMode::DISABLED;
}

telux::common::Status
SimulaCall::modify(
  telux::tel::RttMode /*mode*/,
  std::shared_ptr<telux::common::ICommandResponseCallback> callback
)
{
    if (callback)
        callback->commandResponse(telux::common::ErrorCode::NOT_SUPPORTED);
    return telux::common::Status::NOTSUPPORTED;
}

telux::common::Status
SimulaCall::respondToModifyRequest(
  bool /*modifyResponseType*/,
  std::shared_ptr<telux::common::ICommandResponseCallback> callback
)
{
    if (callback)
        callback->commandResponse(telux::common::ErrorCode::NOT_SUPPORTED);
    return telux::common::Status::NOTSUPPORTED;
}

telux::tel::CallType
SimulaCall::getCallType()
{
    return telux::tel::CallType::VOICE_CALL;
}

}  // namespace telux::tel::simula
