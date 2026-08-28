// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear
//
// InitCallbackGate.hpp - stores initialization callbacks until the owning
// manager first becomes ready. Callbacks added after readiness are invoked
// immediately. Registration and the readiness transition are synchronized so
// callbacks cannot be lost when they occur concurrently.

#ifndef TELUX_COMMON_SIMULA_INIT_CALLBACK_GATE_HPP
#define TELUX_COMMON_SIMULA_INIT_CALLBACK_GATE_HPP

#include <mutex>
#include <telux/common/CommonDefines.hpp>
#include <utility>
#include <vector>

namespace telux::common::simula {

class InitCallbackGate
{
public:
    // An optional callback may be registered during construction.
    explicit InitCallbackGate(telux::common::InitResponseCb cb = nullptr)
    {
        if (cb)
            cbs_.push_back(std::move(cb));
    }

    InitCallbackGate(const InitCallbackGate&) = delete;
    InitCallbackGate& operator=(const InitCallbackGate&) = delete;

    // Register a callback. If readiness has already been reported, invoke it
    // immediately with the recorded status.
    void add(telux::common::InitResponseCb cb)
    {
        if (!cb)
            return;
        telux::common::ServiceStatus fired_status;
        {
            std::lock_guard<std::mutex> lk(mutex_);
            if (!fired_)
            {
                cbs_.push_back(std::move(cb));
                return;
            }
            fired_status = status_;
        }
        cb(fired_status);
    }

    // Record the first readiness transition and invoke callbacks registered
    // before it. Returns true only for that first transition; callers can use
    // false to notify regular service-status listeners instead.
    bool markReadyAndFire(telux::common::ServiceStatus status)
    {
        std::vector<telux::common::InitResponseCb> to_fire;
        {
            std::lock_guard<std::mutex> lk(mutex_);
            if (fired_)
                return false;
            fired_ = true;
            status_ = status;
            to_fire.swap(cbs_);
        }
        for (auto& cb : to_fire)
            cb(status);
        return true;
    }

private:
    std::mutex mutex_;
    std::vector<telux::common::InitResponseCb> cbs_;
    bool fired_{ false };
    telux::common::ServiceStatus status_{
        telux::common::ServiceStatus::SERVICE_UNAVAILABLE
    };
};

}

#endif  // TELUX_COMMON_SIMULA_INIT_CALLBACK_GATE_HPP
