// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear
//
// Call.hpp - SimulaCall, the MQTT-driven telux::tel::ICall implementation
// for eCall sessions.


#ifndef TELUX_TEL_SIMULA_CALL_HPP
#define TELUX_TEL_SIMULA_CALL_HPP

#include "../common/IModemBridge.hpp"

#include <memory>
#include <mutex>
#include <string>
#include <telux/tel/Call.hpp>

namespace telux::tel::simula {

class SimulaCall final : public telux::tel::ICall
{
public:
    SimulaCall(
      int phoneId,
      int callIndex,
      telux::tel::CallDirection direction,
      std::string remotePartyNumber,
      common::simula::IModemBridge& bridge
    );
    ~SimulaCall() override;

    SimulaCall(const SimulaCall&) = delete;
    SimulaCall& operator=(const SimulaCall&) = delete;

    // Mutators used only by SimulaCallManager -- not part of ICall.
    void setCallState(telux::tel::CallState state);
    void setCallEndCause(telux::tel::CallEndCause cause);

    // telux::tel::ICall
    telux::common::Status answer(
      std::shared_ptr<telux::common::ICommandResponseCallback> callback = nullptr,
      telux::tel::RttMode mode = telux::tel::RttMode::DISABLED
    ) override;
    telux::common::Status
      hold(std::shared_ptr<telux::common::ICommandResponseCallback> callback = nullptr) override;
    telux::common::Status
      resume(std::shared_ptr<telux::common::ICommandResponseCallback> callback = nullptr) override;
    telux::common::Status
      reject(std::shared_ptr<telux::common::ICommandResponseCallback> callback = nullptr) override;
    telux::common::Status reject(
      const std::string& rejectSMS,
      std::shared_ptr<telux::common::ICommandResponseCallback> callback = nullptr
    ) override;
    telux::common::Status
      hangup(std::shared_ptr<telux::common::ICommandResponseCallback> callback = nullptr) override;
    telux::common::Status playDtmfTone(
      char tone, std::shared_ptr<telux::common::ICommandResponseCallback> callback = nullptr
    ) override;
    telux::common::Status startDtmfTone(
      char tone, std::shared_ptr<telux::common::ICommandResponseCallback> callback = nullptr
    ) override;
    telux::common::Status stopDtmfTone(
      std::shared_ptr<telux::common::ICommandResponseCallback> callback = nullptr
    ) override;
    telux::tel::CallState getCallState() override;
    int getCallIndex() override;
    telux::tel::CallDirection getCallDirection() override;
    std::string getRemotePartyNumber() override;
    telux::tel::CallEndCause getCallEndCause() override;
    int getSipErrorCode() override;
    int getPhoneId() override;
    bool isMultiPartyCall() override;
    telux::tel::RttMode getRttMode() override;
    telux::tel::RttMode getLocalRttCapability() override;
    telux::tel::RttMode getPeerRttCapability() override;
    telux::common::Status modify(
      telux::tel::RttMode mode,
      std::shared_ptr<telux::common::ICommandResponseCallback> callback = nullptr
    ) override;
    telux::common::Status respondToModifyRequest(
      bool modifyResponseType,
      std::shared_ptr<telux::common::ICommandResponseCallback> callback = nullptr
    ) override;
    telux::tel::CallType getCallType() override;

private:
    common::simula::IModemBridge& bridge_;
    const int phoneId_;
    const int callIndex_;
    const telux::tel::CallDirection direction_;
    const std::string remotePartyNumber_;

    std::mutex mutex_;
    telux::tel::CallState state_{ telux::tel::CallState::CALL_IDLE };
    telux::tel::CallEndCause endCause_{ telux::tel::CallEndCause::NORMAL };
};

}  // namespace telux::tel::simula

#endif  // TELUX_TEL_SIMULA_CALL_HPP
