// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

// chart/time_event.hpp — one-shot and periodic self-posting timers.
//
// A TimeEvent posts a fixed signal to its target ActiveObject after a delay
// (one-shot) or repeatedly with a period (periodic). All TimeEvents share
// a single background thread (TimerService) that wakes on the next deadline.
//
// Cancellation is safe even if it races with firing — the worker checks a
// validity flag under the lock before each post and before re-insertion.
//
// Note: TimeEvent stores a raw ActiveObject*; the user must keep the target
// alive at least until disarm() returns. The destructor disarms automatically.

#ifndef CHART_TIME_EVENT_HPP
#define CHART_TIME_EVENT_HPP

#include "active_object.hpp"
#include "event.hpp"

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <mutex>
#include <optional>
#include <set>
#include <thread>
#include <unordered_map>
#include <utility>

namespace chart {

class TimerService {
public:
    using Token = std::uint64_t;
    using Clock = std::chrono::steady_clock;

    static TimerService& instance() {
        static TimerService s;
        return s;
    }

    Token schedule(std::function<void()> fn, Clock::time_point first,
                   std::chrono::milliseconds period) {
        std::unique_lock<std::mutex> lk(m_);
        Token tok = ++next_;
        valid_[tok] = true;
        items_.insert(Item{first, std::move(fn), period, tok});
        cv_.notify_all();
        return tok;
    }

    void cancel(Token tok) {
        std::lock_guard<std::mutex> lk(m_);
        auto it = valid_.find(tok);
        if (it != valid_.end()) it->second = false;
    }

private:
    // Worker is started in the ctor and joined in the dtor. This means a
    // process that includes <chart/time_event.hpp> always pays for one
    // background thread; for the projects we care about that's a fine
    // tradeoff vs. the lazy-start race surface.
    TimerService() : worker_([this] { run(); }) {}
    TimerService(TimerService const&) = delete;
    TimerService& operator=(TimerService const&) = delete;

    // On program exit (function-local static destruction), tear the worker
    // down deterministically. Without this, a worker would still be sleeping
    // on cv_/m_ when those members are destroyed, causing UB.
    ~TimerService() {
        {
            std::lock_guard<std::mutex> lk(m_);
            stopping_ = true;
        }
        cv_.notify_all();
        if (worker_.joinable()) worker_.join();
    }

    struct Item {
        Clock::time_point due;
        std::function<void()> fn;
        std::chrono::milliseconds period;  // 0 -> one-shot
        Token tok;
        bool operator<(Item const& o) const {
            return due != o.due ? due < o.due : tok < o.tok;
        }
    };

    void run() {
        std::unique_lock<std::mutex> lk(m_);
        while (true) {
            if (stopping_) return;
            if (items_.empty()) {
                cv_.wait(lk, [this] { return stopping_ || !items_.empty(); });
                continue;
            }
            auto next = items_.begin()->due;
            cv_.wait_until(lk, next);
            if (stopping_) return;
            auto now = Clock::now();
            while (!items_.empty() && items_.begin()->due <= now) {
                Item it = *items_.begin();
                items_.erase(items_.begin());
                auto vit = valid_.find(it.tok);
                bool alive = (vit != valid_.end()) && vit->second;
                if (!alive) {
                    if (vit != valid_.end()) valid_.erase(vit);
                    continue;
                }
                lk.unlock();
                try { it.fn(); } catch (...) { /* swallow user exceptions */ }
                lk.lock();
                if (stopping_) return;
                vit = valid_.find(it.tok);
                bool still_alive = (vit != valid_.end()) && vit->second;
                if (it.period.count() > 0 && still_alive) {
                    it.due = Clock::now() + it.period;
                    items_.insert(std::move(it));
                } else {
                    if (vit != valid_.end()) valid_.erase(vit);
                }
            }
        }
    }

    std::mutex m_;
    std::condition_variable cv_;
    std::multiset<Item> items_;
    std::unordered_map<Token, bool> valid_;
    Token next_{0};
    bool stopping_{false};
    std::thread worker_;
};

class TimeEvent {
public:
    TimeEvent(ActiveObject* target, Signal sig) : target_(target), sig_(sig) {
        CHART_REQUIRE(target_ != nullptr);
    }
    ~TimeEvent() { disarm(); }

    TimeEvent(TimeEvent const&)            = delete;
    TimeEvent& operator=(TimeEvent const&) = delete;

    void arm_one_shot(std::chrono::milliseconds delay) {
        disarm();
        auto* tgt = target_;
        Signal s = sig_;
        tok_ = TimerService::instance().schedule(
            [tgt, s] { tgt->post_fifo({s, nullptr}); },
            TimerService::Clock::now() + delay, std::chrono::milliseconds{0});
    }

    void arm_periodic(std::chrono::milliseconds first,
                      std::chrono::milliseconds period) {
        CHART_REQUIRE(period.count() > 0);
        disarm();
        auto* tgt = target_;
        Signal s = sig_;
        tok_ = TimerService::instance().schedule(
            [tgt, s] { tgt->post_fifo({s, nullptr}); },
            TimerService::Clock::now() + first, period);
    }

    void disarm() {
        if (tok_.has_value()) {
            TimerService::instance().cancel(*tok_);
            tok_.reset();
        }
    }

    bool armed() const { return tok_.has_value(); }

private:
    ActiveObject* target_;
    Signal sig_;
    std::optional<TimerService::Token> tok_;
};

}  // namespace chart

#endif  // CHART_TIME_EVENT_HPP
