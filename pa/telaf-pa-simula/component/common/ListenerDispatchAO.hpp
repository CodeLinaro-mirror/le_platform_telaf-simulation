// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear
//
// ListenerDispatchAO.hpp - singleton AO that delivers listener callbacks on
// a thread distinct from any manager AO's worker, so a listener
// implementation calling back into a manager API cannot deadlock with the
// manager that fired the event. Shared across every domain (data, sim,
// network, phone, sms, ...) — not domain-specific.

#ifndef TELUX_COMMON_SIMULA_LISTENER_DISPATCH_AO_HPP
#define TELUX_COMMON_SIMULA_LISTENER_DISPATCH_AO_HPP

#include <chart/active_object.hpp>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace telux::common::simula {

// A single dispatch task: invoke `invoker` on every still-alive listener.
// `debug_tag` is logged on any exception thrown by `invoker`.
struct DispatchTask
{
    std::vector<std::weak_ptr<void>> listeners;
    std::function<void(std::shared_ptr<void>)> invoker;
    std::string debug_tag;
};

class ListenerDispatchAO final : private chart::ActiveObject
{
public:
    static ListenerDispatchAO& instance();

    // Idempotent start/stop. start() boots the worker thread; stop() joins.
    void start();
    void stop() { chart::ActiveObject::stop(); }

    // Enqueue a dispatch task. Thread-safe; may be called from any manager AO.
    void enqueue(std::shared_ptr<DispatchTask> task);

private:
    ListenerDispatchAO();
    ~ListenerDispatchAO();

    ListenerDispatchAO(const ListenerDispatchAO&) = delete;
    ListenerDispatchAO& operator=(const ListenerDispatchAO&) = delete;

    friend chart::Status Dispatching_St(chart::Hsm*, chart::Event const*);
};

}  // namespace telux::common::simula

#endif  // TELUX_COMMON_SIMULA_LISTENER_DISPATCH_AO_HPP
