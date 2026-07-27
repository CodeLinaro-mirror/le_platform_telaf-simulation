// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear
// Thread-safe simulated card and USIM application value objects.

#ifndef TELUX_TEL_SIMULA_SIM_CARD_HPP
#define TELUX_TEL_SIMULA_SIM_CARD_HPP

#include <memory>
#include <mutex>
#include <string>
#include <telux/tel/CardApp.hpp>
#include <telux/tel/CardManager.hpp>
#include <vector>

namespace telux::tel::simula {

class SimulaCardApp final : public telux::tel::ICardApp
{
public:
    explicit SimulaCardApp(telux::tel::AppState appState);

    // telux::tel::ICardApp
    telux::tel::AppType getAppType() override;
    telux::tel::AppState getAppState() override;
    std::string getAppId() override;
    telux::common::Status changeCardPassword(
      telux::tel::CardLockType lockType,
      std::string oldPwd,
      std::string newPwd,
      telux::tel::PinOperationResponseCb callback
    ) override;
    telux::common::Status unlockCardByPuk(
      telux::tel::CardLockType lockType,
      std::string puk,
      std::string newPin,
      telux::tel::PinOperationResponseCb callback
    ) override;
    telux::common::Status unlockCardByPin(
      telux::tel::CardLockType lockType,
      std::string pin,
      telux::tel::PinOperationResponseCb callback
    ) override;
    telux::common::Status queryPin1LockState(telux::tel::QueryPin1LockResponseCb callback) override;
    telux::common::Status queryFdnLockState(telux::tel::QueryFdnLockResponseCb callback) override;
    telux::common::Status setCardLock(
      telux::tel::CardLockType lockType,
      std::string password,
      bool isEnabled,
      telux::tel::PinOperationResponseCb callback
    ) override;

    // Called only from the owning manager's AO thread.
    void setAppState(telux::tel::AppState state);

private:
    mutable std::mutex m_;
    telux::tel::AppState appState_;
};

class SimulaCard final : public telux::tel::ICard
{
public:
    SimulaCard(int slotId, telux::tel::CardState cardState, telux::tel::AppState appState);

    // telux::tel::ICard
    telux::common::Status getState(telux::tel::CardState& cardState) override;
    std::vector<std::shared_ptr<telux::tel::ICardApp>> getApplications(
      telux::common::Status* status = nullptr
    ) override;
    telux::common::Status openLogicalChannel(
      std::string applicationId,
      std::shared_ptr<telux::tel::ICardChannelCallback> callback = nullptr
    ) override;
    telux::common::Status closeLogicalChannel(
      int channelId,
      std::shared_ptr<telux::common::ICommandResponseCallback> callback = nullptr
    ) override;
    telux::common::Status transmitApduLogicalChannel(
      int channel,
      uint8_t cla,
      uint8_t instruction,
      uint8_t p1,
      uint8_t p2,
      uint8_t p3,
      std::vector<uint8_t> data,
      std::shared_ptr<telux::tel::ICardCommandCallback> callback = nullptr
    ) override;
    telux::common::Status transmitApduBasicChannel(
      uint8_t cla,
      uint8_t instruction,
      uint8_t p1,
      uint8_t p2,
      uint8_t p3,
      std::vector<uint8_t> data,
      std::shared_ptr<telux::tel::ICardCommandCallback> callback = nullptr
    ) override;
    telux::common::Status exchangeSimIO(
      uint16_t fileId,
      uint8_t command,
      uint8_t p1,
      uint8_t p2,
      uint8_t p3,
      std::string filePath,
      std::vector<uint8_t> data,
      std::string pin2,
      std::string aid,
      std::shared_ptr<telux::tel::ICardCommandCallback> callback = nullptr
    ) override;
    int getSlotId() override;
    telux::common::Status requestEid(telux::tel::EidResponseCallback callback) override;
    std::shared_ptr<telux::tel::ICardFileHandler> getFileHandler() override;
    bool isNtnProfileActive() override;

    // Mutators. Called only from the owning Manager's AO thread.
    void setState(telux::tel::CardState state);
    telux::tel::CardState getStateValue() const;
    std::shared_ptr<SimulaCardApp> usimApp() const { return usimApp_; }

private:
    mutable std::mutex m_;
    int slotId_;
    telux::tel::CardState cardState_;
    std::shared_ptr<SimulaCardApp> usimApp_;
};

}  // namespace telux::tel::simula

#endif  // TELUX_TEL_SIMULA_SIM_CARD_HPP
