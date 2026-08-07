// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear
//
// Phone.hpp - SimulaPhone, the MQTT-driven telux::tel::IPhone implementation.
//


#ifndef TELUX_TEL_SIMULA_PHONE_HPP
#define TELUX_TEL_SIMULA_PHONE_HPP

#include "../common/IModemBridge.hpp"

#include <nlohmann/json.hpp>
#include <telux/tel/CellInfo.hpp>
#include <telux/tel/Phone.hpp>

namespace telux::tel::simula {

// Parses one wire-format cell entry (cell_info_ind.json / *cell_info_rsp.json
// shape -- both carry the identical per-cell object) into the matching
// telux::tel::CellInfo subclass. Shared between SimulaPhone::requestCellInfo()
// and PhoneManager.cpp's cell_info.ind handler so both convert identically;
// see Phone.cpp for the definition and its per-cellType fallback behavior
// (returns nullptr for CDMA/TDSCDMA or a missing RAT sub-object).
std::shared_ptr<telux::tel::CellInfo> wireToCellInfo(const nlohmann::json& j);

class SimulaPhone final : public telux::tel::IPhone
{
public:
    SimulaPhone(int phoneId, common::simula::IModemBridge& bridge);
    ~SimulaPhone() override;

    SimulaPhone(const SimulaPhone&) = delete;
    SimulaPhone& operator=(const SimulaPhone&) = delete;

    // telux::tel::IPhone
    telux::common::Status getPhoneId(int& phoneId) override;
    telux::tel::RadioState getRadioState() override;
    telux::common::Status
    requestVoiceRadioTechnology(telux::tel::VoiceRadioTechResponseCb callback) override;
    telux::tel::ServiceState getServiceState() override;
    telux::common::Status requestVoiceServiceState(
      std::weak_ptr<telux::tel::IVoiceServiceStateCallback> callback
    ) override;
    telux::common::Status setRadioPower(
      bool enable,
      std::shared_ptr<telux::common::ICommandResponseCallback> callback = nullptr
    ) override;
    telux::common::Status requestCellInfo(telux::tel::CellInfoCallback callback) override;
    telux::common::Status
    setCellInfoListRate(uint32_t timeInterval, telux::common::ResponseCallback callback) override;
    telux::common::Status requestSignalStrength(
      std::shared_ptr<telux::tel::ISignalStrengthCallback> callback = nullptr
    ) override;
    telux::common::Status setECallOperatingMode(
      telux::tel::ECallMode eCallMode,
      telux::common::ResponseCallback callback
    ) override;
    telux::common::Status
    requestECallOperatingMode(telux::tel::ECallGetOperatingModeCallback callback) override;
    telux::common::Status requestOperatorName(telux::tel::OperatorNameCallback callback) override;
    telux::common::Status requestOperatorInfo(telux::tel::OperatorInfoCallback callback) override;
    telux::common::Status configureSignalStrength(
      std::vector<telux::tel::SignalStrengthConfig> signalStrengthConfig,
      telux::common::ResponseCallback callback = nullptr
    ) override;
    telux::common::Status configureSignalStrength(
      std::vector<telux::tel::SignalStrengthConfigEx> signalStrengthConfigEx,
      uint16_t hysteresisMs = 0,
      telux::common::ResponseCallback callback = nullptr
    ) override;

private:
    common::simula::IModemBridge& bridge_;
    int phoneId_;
};

}  // namespace telux::tel::simula

#endif  // TELUX_TEL_SIMULA_PHONE_HPP
