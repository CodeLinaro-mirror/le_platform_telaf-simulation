// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

// chart/publish_subscribe.hpp — process-local pub/sub fabric.
//
// Multiple ActiveObjects can `Fabric::instance().subscribe(this, sig)` for
// signals they want; any caller that does `Fabric::instance().publish(evt)`
// fans the event out to every current subscriber via post_fifo.
//
// Uses shared_mutex for the subscribers table: many publishes / few subscribe
// changes is the common pattern. Duplicate subscriptions (same AO, same sig)
// are deduplicated.

#ifndef CHART_PUBLISH_SUBSCRIBE_HPP
#define CHART_PUBLISH_SUBSCRIBE_HPP

#include "active_object.hpp"
#include "event.hpp"

#include <algorithm>
#include <iterator>
#include <shared_mutex>
#include <unordered_map>
#include <vector>

namespace chart {

class Fabric {
public:
    static Fabric& instance() {
        static Fabric f;
        return f;
    }

    void subscribe(ActiveObject* who, int sig) {
        CHART_REQUIRE(who != nullptr);
        std::unique_lock<std::shared_mutex> lk(m_);
        auto& list = subs_[sig];
        if (std::find(list.begin(), list.end(), who) == list.end())
            list.push_back(who);
    }

    void unsubscribe(ActiveObject* who, int sig) {
        std::unique_lock<std::shared_mutex> lk(m_);
        auto it = subs_.find(sig);
        if (it == subs_.end()) return;
        auto& list = it->second;
        list.erase(std::remove(list.begin(), list.end(), who), list.end());
        if (list.empty()) subs_.erase(it);
    }

    // Removes `who` from every subscription list. Useful in AO dtors.
    void unsubscribe_all(ActiveObject* who) {
        std::unique_lock<std::shared_mutex> lk(m_);
        for (auto it = subs_.begin(); it != subs_.end();) {
            auto& list = it->second;
            list.erase(std::remove(list.begin(), list.end(), who), list.end());
            it = list.empty() ? subs_.erase(it) : std::next(it);
        }
    }

    void publish(Event e) {
        // Snapshot subscribers under shared lock, then post outside the lock —
        // posting can take per-AO locks, and we don't want to nest them.
        std::vector<ActiveObject*> targets;
        {
            std::shared_lock<std::shared_mutex> lk(m_);
            auto it = subs_.find(e.sig);
            if (it == subs_.end()) return;
            targets = it->second;
        }
        for (auto* ao : targets) ao->post_fifo(e);
    }

private:
    Fabric() = default;
    Fabric(Fabric const&)            = delete;
    Fabric& operator=(Fabric const&) = delete;

    std::shared_mutex m_;
    std::unordered_map<int, std::vector<ActiveObject*>> subs_;
};

namespace detail {

// On every ActiveObject destruction, drop it from every subscription list.
// This is registered exactly once thanks to inline-variable ODR merging.
inline const bool fabric_cleanup_registered_ = []() {
    register_cleanup_hook([](ActiveObject* ao) {
        Fabric::instance().unsubscribe_all(ao);
    });
    return true;
}();

}  // namespace detail

}  // namespace chart

#endif  // CHART_PUBLISH_SUBSCRIBE_HPP
