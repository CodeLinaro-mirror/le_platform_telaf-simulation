/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <algorithm>

#include "SimProfileManagerBypass.hpp"

#define DEFAULT_NUM_SLOTS 1
namespace telux {
namespace tel {

SimProfileManagerBypass::SimProfileManagerBypass() {
    LOG(DEBUG, __FUNCTION__);
}

SimProfileManagerBypass::~SimProfileManagerBypass() {
    LOG(DEBUG, __FUNCTION__);
}

telux::common::Status SimProfileManagerBypass::init(telux::common::InitResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);

    listenerMgr_ = std::make_shared<telux::common::ListenerManager<ISimProfileListener>>();
    if (!listenerMgr_) {
       LOG(ERROR, __FUNCTION__, " Unable to instantiate ListenerManager");
       cleanup();
       return telux::common::Status::FAILED;
    }

    initCb_ = callback;

    auto f = std::async(std::launch::async, [this]() { this->initSync(); }).share();
    taskQ_.add(f);

    return telux::common::Status::SUCCESS;
}

void SimProfileManagerBypass::cleanup() {
    LOG(DEBUG, __FUNCTION__);

    if(listenerMgr_) {
        listenerMgr_ = nullptr;
    }

    cachedSetProfileCmdIdMap_.clear();
    cachedDeleteProfileCmdIdMap_.clear();
    profileEnabledMap_.clear();
}

void SimProfileManagerBypass::setServiceStatus(telux::common::ServiceStatus status) {
    LOG(DEBUG, __FUNCTION__, " Service Status: ", static_cast<int>(status));
    {
       std::lock_guard<std::mutex> lk(mtx_);
       subSystemStatus_ = status;
    }
    if (initCb_) {
       initCb_(status);
    } else {
       LOG(ERROR, __FUNCTION__, " Callback is NULL");
    }
}

void SimProfileManagerBypass::fakeRefrashRegResponse() {
    setServiceStatus(telux::common::ServiceStatus::SERVICE_AVAILABLE);
    cv_.notify_all();
}

void SimProfileManagerBypass::initSync() {
    LOG(DEBUG, __FUNCTION__);

    noOfSlots_= DEFAULT_NUM_SLOTS; //Defaulting to 1 for Single SIM.

    auto f = std::async(std::launch::async, [this]() { this->fakeRefrashRegResponse(); }).share();
    taskQ_.add(f);
}

bool SimProfileManagerBypass::isSubsystemReady() {
    std::lock_guard<std::mutex> lk(mtx_);
    return (subSystemStatus_ == telux::common::ServiceStatus::SERVICE_AVAILABLE);
}

std::future<bool> SimProfileManagerBypass::onSubsystemReady() {
    LOG(DEBUG, __FUNCTION__);
    auto future = std::async(std::launch::async, [&] { return waitForInitialization(); });
    return future;
}

bool SimProfileManagerBypass::waitForInitialization() {
    LOG(DEBUG, __FUNCTION__);
    std::unique_lock<std::mutex> lock(mtx_);
     while (subSystemStatus_ != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        cv_.wait(lock);
    }
    return true;
}

telux::common::ServiceStatus SimProfileManagerBypass::getServiceStatus() {
    std::lock_guard<std::mutex> lk(mtx_);
    LOG(DEBUG, __FUNCTION__, " Service Status: ", static_cast<int>(subSystemStatus_));
    return subSystemStatus_;
}

telux::common::Status SimProfileManagerBypass::requestProfileList(SlotId slotId,
    ProfileListResponseCb callback) {
    LOG(DEBUG, __FUNCTION__, " Slot Id: ", static_cast<int>(slotId));

    static auto profile01 = std::make_shared<telux::tel::SimProfile>(
        1, ProfileType::REGULAR, std::string("89019990018000006241"), false,
        std::string("testIDEMIA"), std::string("QUALCOMM Wireless One"), std::string("QUALCOMM"),
        IconType::NONE, std::vector<uint8_t>({1,2,3,4}), ProfileClass::OPERATIONAL, 0x55AA);

    static auto profile02 = std::make_shared<telux::tel::SimProfile>(
        2, ProfileType::EMERGENCY, std::string("89339981214014636084"), true,
        std::string("SecondProfile"),std::string("Idemia Test 6"), std::string("MNO1"),
        IconType::JPEG, std::vector<uint8_t>({5,6,7,8}), ProfileClass::OPERATIONAL, 0xFFAA);

    static std::vector<std::shared_ptr<telux::tel::SimProfile>> profiles;

    if (profiles.empty()) {
        profiles.emplace_back(profile01);
        profiles.emplace_back(profile02);
    }

    if (callback != nullptr) {
        auto f = std::async(std::launch::async, [callback]() { callback(profiles,  telux::common::ErrorCode::SUCCESS); }).share();
        taskQ_.add(f);
    }

    return telux::common::Status::SUCCESS;
}

telux::common::Status SimProfileManagerBypass::setProfile(SlotId slotId, int profileId, bool enable,
    common::ResponseCallback callback) {

    if (callback != nullptr) {
        auto f = std::async(std::launch::async, [callback]() { callback(telux::common::ErrorCode::SUCCESS); }).share();
        taskQ_.add(f);
    }

    return telux::common::Status::SUCCESS;
}

telux::common::Status SimProfileManagerBypass::registerListener(
    std::weak_ptr<ISimProfileListener> listener) {
    if (listenerMgr_) {
        return listenerMgr_->registerListener(listener);
    } else {
        LOG(ERROR, __FUNCTION__, " listenerMgr is null");
    }
    return telux::common::Status::FAILED;
}

telux::common::Status SimProfileManagerBypass::deregisterListener(
    std::weak_ptr<ISimProfileListener> listener) {
    if (listenerMgr_) {
        return listenerMgr_->deRegisterListener(listener);
    } else {
        LOG(ERROR, __FUNCTION__, " listenerMgr is null");
    }
    return telux::common::Status::FAILED;
}

telux::common::Status SimProfileManagerBypass::addProfile(SlotId slotId,
    const std::string &activationCode, const std::string &confirmationCode,
    bool userConsentSupported, common::ResponseCallback callback){

    return telux::common::Status::SUCCESS;
}

telux::common::Status SimProfileManagerBypass::deleteProfile(SlotId slotId, int profileId,
    common::ResponseCallback callback)
{
    return telux::common::Status::SUCCESS;
}

telux::common::Status SimProfileManagerBypass::updateNickName(SlotId slotId, int profileId,
    const std::string &nickName, common::ResponseCallback callback)
{
    return telux::common::Status::SUCCESS;
}

telux::common::Status SimProfileManagerBypass::requestEid(SlotId slotId,
    EidResponseCb callback)
{
    static std::string eid = "89049032000001000000044091520988";
    if (callback != nullptr) {
        auto f = std::async(std::launch::async, [callback]() { callback(eid, telux::common::ErrorCode::SUCCESS); }).share();
        taskQ_.add(f);
    }

    return telux::common::Status::SUCCESS;
}

telux::common::Status SimProfileManagerBypass::provideUserConsent(SlotId slotId,
    bool userConsent, UserConsentReasonType reason,
    common::ResponseCallback callback)
{
    return telux::common::Status::SUCCESS;
}

telux::common::Status SimProfileManagerBypass::provideConfirmationCode(SlotId slotId, std::string code,
    common::ResponseCallback callback)
{
    return telux::common::Status::SUCCESS;
}

telux::common::Status SimProfileManagerBypass::setServerAddress(SlotId slotId, const std::string &smdpAddress,
    common::ResponseCallback callback)
{
    return telux::common::Status::SUCCESS;
}

telux::common::Status SimProfileManagerBypass::requestServerAddress(SlotId slotId,
    ServerAddressResponseCb callback)
{
    return telux::common::Status::SUCCESS;
}

telux::common::Status SimProfileManagerBypass::memoryReset(SlotId slotId, ResetOptionMask mask,
    common::ResponseCallback callback)
{
    return telux::common::Status::SUCCESS;
}

}
}
