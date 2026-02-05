/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <thread>
#include <chrono>

#include "PhoneFactoryImplStub.hpp"
#include <mutex>
#include <syslog.h>

#include "SimProfileManagerBypass.hpp"
#include "MultiSimManagerImplBypass.hpp"

using namespace telux::tel;

std::recursive_mutex mutex_;

std::vector<telux::common::InitResponseCb> simProfileMgrCallbacks_;
telux::common::ServiceStatus simProfileMgrInitStatus_ = telux::common::ServiceStatus::SERVICE_UNAVAILABLE;
static std::shared_ptr<ISimProfileManager> simProfileManager_ = nullptr;

static void onSimProfileManagerResponse(telux::common::ServiceStatus status)
{
    std::vector<telux::common::InitResponseCb> simProfileMgrCallbacks;
    syslog(LOG_INFO, "%s %s", __FUNCTION__, " SimProfile Manager initialization status: ");

    {
       std::lock_guard<std::recursive_mutex> lock(mutex_);
       simProfileMgrInitStatus_ = status;
       bool reportServiceStatus = false;
       switch(status) {
          case telux::common::ServiceStatus::SERVICE_FAILED:
             simProfileManager_ = nullptr;
             reportServiceStatus = true;
             break;
          case telux::common::ServiceStatus::SERVICE_AVAILABLE:
             reportServiceStatus = true;
             break;
          default:
             break;
       }
       if (!reportServiceStatus) {
          return;
       }
       simProfileMgrCallbacks = simProfileMgrCallbacks_;
       simProfileMgrCallbacks_.clear();
    }
    for (auto &callback : simProfileMgrCallbacks) {
       if (callback) {
          callback(status);
       } else {
          syslog(LOG_INFO, "%s %s", __FUNCTION__, " Callback is NULL");
       }
    }
}

std::shared_ptr<ISimProfileManager>
PhoneFactoryImplStub::getSimProfileManager(telux::common::InitResponseCb  callback)
{
    syslog(LOG_INFO, "[bypass] %s", __FUNCTION__);

    std::lock_guard<std::recursive_mutex> lock(mutex_);

    if (simProfileManager_ == nullptr) {
        std::shared_ptr<SimProfileManagerBypass> simProfileManager = nullptr;
        try {
            simProfileManager = std::make_shared<SimProfileManagerBypass>();
        } catch (std::bad_alloc & e) {
            syslog(LOG_ERR, "%s %s", __FUNCTION__, e.what());
        }

        auto initCb = [this](telux::common::ServiceStatus status) {
            syslog(LOG_INFO, "%s", "SimProfileManager Initialization callback");
            onSimProfileManagerResponse(status);
        };
        auto status = simProfileManager->init(initCb);
        if (status != telux::common::Status::SUCCESS) {
            syslog(LOG_ERR, "%s", "Failed to initialize SimProfimeManager");
            simProfileManager = nullptr;
            return nullptr;
        }

        if (callback) {
            simProfileMgrCallbacks_.push_back(callback);
        } else {
            syslog(LOG_INFO, "%s %s", __FUNCTION__, "callback is NULL");
        }

        simProfileManager_ = simProfileManager;

    } else if (simProfileMgrInitStatus_ == telux::common::ServiceStatus::SERVICE_UNAVAILABLE){
        syslog(LOG_INFO,"%s %s", __FUNCTION__, " SimProfile Manager is not yet initialized");
        if (callback) {
            simProfileMgrCallbacks_.push_back(callback);
        } else {
            syslog(LOG_INFO, "%s %s", __FUNCTION__, "callback is NULL");
        }
    } else if (callback) {
        syslog(LOG_INFO, "%s %s", __FUNCTION__, " SimProfile Manager is initialized, invoking app callback");
        std::thread appCallback(callback, simProfileMgrInitStatus_);
        appCallback.detach();
    } else {
        syslog(LOG_ERR, "%s %s", __FUNCTION__, " SimProfile Manager is initialized, app Callback is NULL");
    }

    return simProfileManager_;
}

std::vector<telux::common::InitResponseCb> multiSimMgrCallbacks_;
telux::common::ServiceStatus multiSimMgrInitStatus_ = telux::common::ServiceStatus::SERVICE_UNAVAILABLE;
static std::shared_ptr<IMultiSimManager> multiSimManager_ = nullptr;


static void onMultiSimManagerResponse(telux::common::ServiceStatus status) {
    std::vector<telux::common::InitResponseCb> multiSimMgrCallbacks;
    LOG(INFO, __FUNCTION__, " MultiSim Manager initialization status: ", static_cast<int>(status));
    {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        multiSimMgrInitStatus_ = status;
        bool reportServiceStatus = false;
        switch(status) {
            case telux::common::ServiceStatus::SERVICE_FAILED:
                multiSimManager_ = NULL;
                reportServiceStatus = true;
                break;
            case telux::common::ServiceStatus::SERVICE_AVAILABLE:
                reportServiceStatus = true;
                break;
            default:
                break;
        }
        if (!reportServiceStatus) {
            return;
        }
        multiSimMgrCallbacks = multiSimMgrCallbacks_;
        multiSimMgrCallbacks_.clear();
    }

    for (auto &callback : multiSimMgrCallbacks) {
        if (callback) {
            callback(status);
        } else {
            LOG(INFO, __FUNCTION__, " Callback is NULL");
        }
    }
}

std::shared_ptr<IMultiSimManager> PhoneFactoryImplStub::getMultiSimManager(
    telux::common::InitResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (multiSimManager_ == nullptr) {
       std::shared_ptr<MultiSimManagerBypass> multiSimManagerBypassImpl = nullptr;
       try {
          multiSimManagerBypassImpl = std::make_shared<MultiSimManagerBypass>();
       } catch (std::bad_alloc & e) {
          LOG(ERROR, __FUNCTION__ , e.what());
          return nullptr;
       }
       auto initCb = [this](telux::common::ServiceStatus status) {
          LOG(DEBUG, __FUNCTION__, " MultiSim Manager initialization callback");
          onMultiSimManagerResponse(status);
       };
       auto status = multiSimManagerBypassImpl->init(initCb);
       if (status != telux::common::Status::SUCCESS) {
          LOG(ERROR, __FUNCTION__, " Failed to initialize the Muti Sim manager");
          multiSimManager_ = nullptr;
          return nullptr;
       }
       if (callback) {
          LOG(DEBUG, __FUNCTION__);
          multiSimMgrCallbacks_.push_back(callback);
       } else {
          LOG(DEBUG, __FUNCTION__, " Callback is NULL");
       }
       multiSimManager_ = multiSimManagerBypassImpl;
    } else if (multiSimMgrInitStatus_ == telux::common::ServiceStatus::SERVICE_UNAVAILABLE) {
       LOG(DEBUG, __FUNCTION__, " Muti Sim manager is not yet initialized");
       if (callback) {
          multiSimMgrCallbacks_.push_back(callback);
       } else {
          LOG(DEBUG, __FUNCTION__, " Callback is NULL");
       }
    } else if (callback) {
       LOG(DEBUG, __FUNCTION__, " Muti Sim manager is initialized, invoking app callback");
       std::thread appCallback(callback, multiSimMgrInitStatus_);
       appCallback.detach();
    } else {
       LOG(ERROR, __FUNCTION__, " Muti Sim manager is initialized, app Callback is NULL");
    }
    return multiSimManager_;
}

__attribute__((constructor)) void BypassPhoneFactoryConstructor()
{
    syslog(LOG_INFO, "[bypass] %s", __FUNCTION__);
}
