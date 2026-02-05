/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <string>
#include <algorithm>
#include <telux/common/DeviceConfig.hpp>

#include "MultiSimManagerImplBypass.hpp"
// #include "MultiSimHelper.hpp"
#include "common/Logger.hpp"
namespace telux {
namespace tel {

static SlotId activeSlotId = SlotId::SLOT_ID_1;

static std::map<SlotId, SlotStatus> slotStatusMap =
{
    {SlotId::SLOT_ID_1, {SlotState::ACTIVE, CardState::CARDSTATE_PRESENT,CardError::UNKNOWN}},
};

MultiSimManagerBypass::MultiSimManagerBypass() {
    LOG(DEBUG, __FUNCTION__, " constructor called ");
}

MultiSimManagerBypass::~MultiSimManagerBypass() {
    LOG(DEBUG, __FUNCTION__, " destructor called ");
}

bool MultiSimManagerBypass::isSubsystemReady() {
    std::lock_guard<std::mutex> lock(mtx_);
    return (subSystemStatus_ == telux::common::ServiceStatus::SERVICE_AVAILABLE);
}

bool MultiSimManagerBypass::waitForInitialization() {
    LOG(DEBUG, __FUNCTION__);
    std::unique_lock<std::mutex> cvLock(mtx_);
    while(subSystemStatus_ != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        cv_.wait(cvLock);
    }
    return true;
}

std::future<bool> MultiSimManagerBypass::onSubsystemReady() {
    LOG(INFO, __FUNCTION__);
    auto f = std::async(std::launch::async, [&] { return waitForInitialization(); });
    return f;
}

telux::common::Status MultiSimManagerBypass::init(telux::common::InitResponseCb callback) {
    LOG(INFO, __FUNCTION__);

    listenerMgr_ = std::make_shared<telux::common::ListenerManager<IMultiSimListener>>();
    if (!listenerMgr_) {
        return telux::common::Status::FAILED;
    }
    initCb_ = callback;
    auto f = std::async(std::launch::async, [this]() { this->initSync(); }).share();
    taskQ_.add(f);

    return telux::common::Status::SUCCESS;
}

void MultiSimManagerBypass::cleanup() {
    LOG(INFO, __FUNCTION__);

    if(listenerMgr_) {
        listenerMgr_ = nullptr;
    }
}

void MultiSimManagerBypass::initSync() {
    LOG(DEBUG, __FUNCTION__);

    slotCount_ = slotStatusMap.size();

    telux::common::Status status = telux::common::Status::SUCCESS;

    if(status != telux::common::Status::SUCCESS) {
        setServiceStatus(telux::common::ServiceStatus::SERVICE_FAILED);
        LOG(ERROR, __FUNCTION__, " MultiSimManager initialization failed");
    } else {
        setServiceStatus(telux::common::ServiceStatus::SERVICE_AVAILABLE);
        LOG(INFO, __FUNCTION__, " MultiSimManager is ready");
    }

    cv_.notify_all();
}

telux::common::ServiceStatus MultiSimManagerBypass::getServiceStatus() {
    std::lock_guard<std::mutex> lock(mtx_);
    LOG(DEBUG, __FUNCTION__, " Service Status: ", static_cast<int>(subSystemStatus_));
    return subSystemStatus_;
}

void MultiSimManagerBypass::setServiceStatus(telux::common::ServiceStatus status) {
    LOG(DEBUG, __FUNCTION__, " Service Status: ", static_cast<int>(status));
    {
       std::lock_guard<std::mutex> lock(mtx_);
       subSystemStatus_ = status;
    }
    if (initCb_) {
       initCb_(status);
    } else {
       LOG(ERROR, __FUNCTION__, " Callback is NULL");
    }
}

telux::common::Status MultiSimManagerBypass::getSlotCount(int &slotCount) {
    LOG(INFO, __FUNCTION__);
    if(getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " Multi Sim subsystem not ready");
        return telux::common::Status::NOTREADY;
    }
    slotCount = slotCount_;
    return telux::common::Status::SUCCESS;
}

telux::common::Status
MultiSimManagerBypass::requestHighCapability(HighCapabilityCallback callback) {
    LOG(INFO, __FUNCTION__);
    return telux::common::Status::SUCCESS;
}

telux::common::Status
MultiSimManagerBypass::setHighCapability(int slotId, common::ResponseCallback callback) {
    LOG(INFO, __FUNCTION__, " Set High capability on Slot Id: ", slotId);
    return telux::common::Status::SUCCESS;
}

telux::common::Status
MultiSimManagerBypass::switchActiveSlot(SlotId slotId,common::ResponseCallback callback) {
    LOG(INFO, __FUNCTION__, " Switching active slot to Slot Id: ", static_cast<int>(slotId));

    if(getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " Multi Sim subsystem not ready");
        return telux::common::Status::NOTREADY;
    }

    if (slotId > slotCount_) {
        LOG(ERROR, __FUNCTION__, " Bad slot id: ", slotId);
    } else {
        activeSlotId = slotId;
    }

    if (callback) {
        if (slotId > slotCount_)
        {
            auto f = std::async(std::launch::async,
                                [callback]() { callback(telux::common::ErrorCode::SIM_ERR); }).share();
            taskQ_.add(f);
        } else {
            auto f = std::async(std::launch::async,
                                [callback]() { callback(telux::common::ErrorCode::SUCCESS); }).share();
            taskQ_.add(f);
        }
    }

    return telux::common::Status::SUCCESS;
}

telux::common::Status
MultiSimManagerBypass::requestSlotStatus(SlotStatusCallback callback) {
    LOG(DEBUG, __FUNCTION__);

    if(getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " Multi Sim subsystem not ready");
        return telux::common::Status::NOTREADY;
    }

    for(auto it = slotStatusMap.begin(); it != slotStatusMap.end(); ++it) {
        auto slotId = it->first;
        auto slotStatus = it->second;
        LOG(INFO, __FUNCTION__, "SlotId: ", static_cast<int>(slotId),
                                ", SlotState: ", static_cast<int>(slotStatus.slotState),
                                ", CardState: ", static_cast<int>(slotStatus.cardState),
                                ", CardError: ", static_cast<int>(slotStatus.cardError));
    }

    if (callback) {
        auto f = std::async(std::launch::async,
                            [callback]() { callback(slotStatusMap, telux::common::ErrorCode::SUCCESS); }).share();
        taskQ_.add(f);
    }

    return telux::common::Status::SUCCESS;
}

telux::common::Status
MultiSimManagerBypass::registerListener(std::weak_ptr<IMultiSimListener> listener) {
    LOG(INFO, __FUNCTION__);
    telux::common::Status status = telux::common::Status::FAILED;
    if (listenerMgr_) {
        status = listenerMgr_->registerListener(listener);
    }
    return status;
}

telux::common::Status
MultiSimManagerBypass::deregisterListener(std::weak_ptr<IMultiSimListener> listener) {
    LOG(INFO, __FUNCTION__);
    telux::common::Status status = telux::common::Status::FAILED;
    if (listenerMgr_) {
        status = listenerMgr_->deRegisterListener(listener);
    }
    return status;
}

}
}
