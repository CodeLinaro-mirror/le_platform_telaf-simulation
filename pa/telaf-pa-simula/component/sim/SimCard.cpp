// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "SimCard.hpp"

namespace telux::tel::simula {

// ============================================================================
// SimulaCardApp
// ============================================================================

SimulaCardApp::SimulaCardApp(telux::tel::AppState appState) : appState_(appState) {}

telux::tel::AppType
SimulaCardApp::getAppType()
{
    return telux::tel::APPTYPE_USIM;
}

telux::tel::AppState
SimulaCardApp::getAppState()
{
    std::lock_guard<std::mutex> lk(m_);
    return appState_;
}

std::string
SimulaCardApp::getAppId()
{
    return "";
}

telux::common::Status
SimulaCardApp::changeCardPassword(telux::tel::CardLockType, std::string, std::string, telux::tel::PinOperationResponseCb)
{
    return telux::common::Status::NOTSUPPORTED;
}

telux::common::Status
SimulaCardApp::unlockCardByPuk(telux::tel::CardLockType, std::string, std::string, telux::tel::PinOperationResponseCb)
{
    return telux::common::Status::NOTSUPPORTED;
}

telux::common::Status
SimulaCardApp::unlockCardByPin(telux::tel::CardLockType, std::string, telux::tel::PinOperationResponseCb)
{
    return telux::common::Status::NOTSUPPORTED;
}

telux::common::Status
SimulaCardApp::queryPin1LockState(telux::tel::QueryPin1LockResponseCb)
{
    return telux::common::Status::NOTSUPPORTED;
}

telux::common::Status
SimulaCardApp::queryFdnLockState(telux::tel::QueryFdnLockResponseCb)
{
    return telux::common::Status::NOTSUPPORTED;
}

telux::common::Status
SimulaCardApp::setCardLock(telux::tel::CardLockType, std::string, bool, telux::tel::PinOperationResponseCb)
{
    return telux::common::Status::NOTSUPPORTED;
}

void
SimulaCardApp::setAppState(telux::tel::AppState state)
{
    std::lock_guard<std::mutex> lk(m_);
    appState_ = state;
}

// ============================================================================
// SimulaCard
// ============================================================================

SimulaCard::SimulaCard(int slotId, telux::tel::CardState cardState, telux::tel::AppState appState)
    : slotId_(slotId)
    , cardState_(cardState)
    , usimApp_(std::make_shared<SimulaCardApp>(appState))
{}

telux::common::Status
SimulaCard::getState(telux::tel::CardState& cardState)
{
    std::lock_guard<std::mutex> lk(m_);
    cardState = cardState_;
    return telux::common::Status::SUCCESS;
}

std::vector<std::shared_ptr<telux::tel::ICardApp>>
SimulaCard::getApplications(telux::common::Status* status)
{
    std::lock_guard<std::mutex> lk(m_);
    if (status)
        *status = telux::common::Status::SUCCESS;
    if (cardState_ != telux::tel::CardState::CARDSTATE_PRESENT)
        return {};
    return { usimApp_ };
}

telux::common::Status
SimulaCard::openLogicalChannel(std::string, std::shared_ptr<telux::tel::ICardChannelCallback>)
{
    return telux::common::Status::NOTSUPPORTED;
}

telux::common::Status
SimulaCard::closeLogicalChannel(int, std::shared_ptr<telux::common::ICommandResponseCallback>)
{
    return telux::common::Status::NOTSUPPORTED;
}

telux::common::Status
SimulaCard::transmitApduLogicalChannel(
  int, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, std::vector<uint8_t>,
  std::shared_ptr<telux::tel::ICardCommandCallback>
)
{
    return telux::common::Status::NOTSUPPORTED;
}

telux::common::Status
SimulaCard::transmitApduBasicChannel(
  uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, std::vector<uint8_t>,
  std::shared_ptr<telux::tel::ICardCommandCallback>
)
{
    return telux::common::Status::NOTSUPPORTED;
}

telux::common::Status
SimulaCard::exchangeSimIO(
  uint16_t, uint8_t, uint8_t, uint8_t, uint8_t, std::string, std::vector<uint8_t>,
  std::string, std::string, std::shared_ptr<telux::tel::ICardCommandCallback>
)
{
    return telux::common::Status::NOTSUPPORTED;
}

int
SimulaCard::getSlotId()
{
    return slotId_;
}

telux::common::Status
SimulaCard::requestEid(telux::tel::EidResponseCallback)
{
    return telux::common::Status::NOTSUPPORTED;
}

std::shared_ptr<telux::tel::ICardFileHandler>
SimulaCard::getFileHandler()
{
    return nullptr;
}

bool
SimulaCard::isNtnProfileActive()
{
    return false;
}

void
SimulaCard::setState(telux::tel::CardState state)
{
    std::lock_guard<std::mutex> lk(m_);
    cardState_ = state;
}

telux::tel::CardState
SimulaCard::getStateValue() const
{
    std::lock_guard<std::mutex> lk(m_);
    return cardState_;
}

}  // namespace telux::tel::simula
