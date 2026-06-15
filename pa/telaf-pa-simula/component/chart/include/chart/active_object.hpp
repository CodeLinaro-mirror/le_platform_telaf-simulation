// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

// chart/active_object.hpp — Active Object: HSM with its own thread + queue.
//
// Each ActiveObject runs run-to-completion semantics on its own thread, so
// state handlers never need locks for chart-internal data. The queue is
// FIFO by default (ordinary post_fifo) with an LIFO front-insert escape
// hatch (post_lifo) for high-priority self-posts.
//
// Lifecycle:
//   ActiveObject ao("name");      // constructed, no thread yet
//   ao.start_at(initial_state);   // runs init() inline, then starts worker
//   ao.post_fifo({sig, payload}); // dispatched on worker thread
//   ao.stop();                    // joins; further posts are dropped

#ifndef CHART_ACTIVE_OBJECT_HPP
#define CHART_ACTIVE_OBJECT_HPP

#include "assert.hpp"
#include "event.hpp"
#include "hsm.hpp"
#include "instrument.hpp"

#include <atomic>
#include <condition_variable>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace chart {

class ActiveObject;

namespace detail {

// Cleanup-hook registry. Library facilities (Fabric, future cross-AO
// services) register a callback once at startup; every ActiveObject
// destructor walks the registry so AOs leave whatever data structures
// they touched in a consistent state without coupling AO to Fabric.
struct CleanupRegistry {
    std::mutex m;
    std::vector<std::function<void(ActiveObject*)>> hooks;
    static CleanupRegistry& instance() {
        static CleanupRegistry r;
        return r;
    }
};

inline void register_cleanup_hook(std::function<void(ActiveObject*)> fn) {
    auto& r = CleanupRegistry::instance();
    std::lock_guard<std::mutex> lk(r.m);
    r.hooks.push_back(std::move(fn));
}

inline void run_cleanup_hooks(ActiveObject* ao) {
    auto& r = CleanupRegistry::instance();
    std::vector<std::function<void(ActiveObject*)>> snap;
    {
        std::lock_guard<std::mutex> lk(r.m);
        snap = r.hooks;
    }
    for (auto& fn : snap) fn(ao);
}

}  // namespace detail

class ActiveObject : public Hsm {
public:
    explicit ActiveObject(std::string name = {}) : name_(std::move(name)) {}

    // Non-copyable, non-movable: it owns a thread and condition variable.
    ActiveObject(ActiveObject const&)            = delete;
    ActiveObject& operator=(ActiveObject const&) = delete;
    ActiveObject(ActiveObject&&)                 = delete;
    ActiveObject& operator=(ActiveObject&&)      = delete;

    ~ActiveObject() {
        stop();
        detail::run_cleanup_hooks(this);
    }

    void set_instrument(Instrument* inst) { inst_ = inst; }
    void set_instrument(std::unique_ptr<Instrument> inst) {
        owned_inst_ = std::move(inst);
        inst_ = owned_inst_.get();
    }
    Instrument* instrument() const { return inst_; }

    // Inline init() runs on the *caller* thread so initial ENTRY/INIT
    // side effects are observable before any user post lands. Then the
    // worker thread is launched.
    void start_at(StateFn initial) {
        CHART_REQUIRE(!running_.load());
        if (inst_) {
            set_trace([](void* ctx, Signal sig, StateFn from, StateFn to) {
                auto* ao = static_cast<ActiveObject*>(ctx);
                ao->inst_->on_transition(sig, from, to);
            }, this);
            set_dispatch_probe([](void* ctx, Signal sig, StateFn s, DispatchEvent kind) {
                auto* ao = static_cast<ActiveObject*>(ctx);
                switch (kind) {
                    case DispatchEvent::HandlerVisit:
                        ao->inst_->on_visit(sig, s);
                        break;
                    case DispatchEvent::Hook:
                        ao->inst_->on_hook(sig, s);
                        break;
                    case DispatchEvent::Ignored:
                        ao->inst_->on_ignored(sig, s);
                        break;
                }
            }, this);
        }
        init(initial);
        running_.store(true);
        worker_ = std::thread([this] { run(); });
    }

    void post_fifo(Event e) {
        {
            std::lock_guard<std::mutex> lk(m_);
            if (!running_.load()) return;
            q_.push_back(std::move(e));
        }
        cv_.notify_one();
    }

    void post_lifo(Event e) {
        {
            std::lock_guard<std::mutex> lk(m_);
            if (!running_.load()) return;
            q_.push_front(std::move(e));
        }
        cv_.notify_one();
    }

    // Cooperative stop. Idempotent; safe to call from any thread except the
    // worker itself. Drops any pending events.
    //
    // The running_ flip MUST happen under m_ to close the race against a
    // worker sitting between "predicate evaluated false" and the internal
    // wait(lk) that atomically unlocks+blocks. Without the lock, exchange +
    // notify_all can land in that seam: worker has already read running_==true,
    // hasn't registered as a waiter yet, and the notify is lost -- worker then
    // blocks forever and stop() deadlocks in join(). Same discipline as
    // post_fifo / post_lifo above.
    void stop() {
        {
            std::lock_guard<std::mutex> lk(m_);
            if (!running_.exchange(false)) return;
        }
        cv_.notify_all();
        if (worker_.joinable() && worker_.get_id() != std::this_thread::get_id()) {
            worker_.join();
        }
        std::lock_guard<std::mutex> lk(m_);
        q_.clear();
    }

    std::string const& name() const { return name_; }
    bool running() const { return running_.load(); }

    // Identity of the worker thread, or a default-constructed id before
    // start_at()/after stop(). Lets a subclass reject a caller that would
    // deadlock by blocking the worker on work only the worker can do (e.g. a
    // synchronous queue fence -- see ModemBridge::drain()). Only meaningful
    // to compare against std::this_thread::get_id(); `worker_` is written
    // once by start_at() on the starting thread, so any read that is
    // ordered after start() returns sees the final value.
    std::thread::id worker_id() const { return worker_.get_id(); }

private:
    void run() {
        while (true) {
            Event e;
            {
                std::unique_lock<std::mutex> lk(m_);
                cv_.wait(lk, [this] { return !running_.load() || !q_.empty(); });
                if (!running_.load() && q_.empty()) return;
                e = std::move(q_.front());
                q_.pop_front();
            }
            dispatch(e);
        }
    }

    std::string name_;
    Instrument* inst_{nullptr};
    std::unique_ptr<Instrument> owned_inst_;
    std::thread worker_;
    std::mutex  m_;
    std::condition_variable cv_;
    std::deque<Event> q_;
    std::atomic<bool> running_{false};
};

}  // namespace chart

#endif  // CHART_ACTIVE_OBJECT_HPP
