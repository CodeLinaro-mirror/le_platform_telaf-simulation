// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

// chart/event.hpp — Signal struct, Event, and system signal definitions.
//
// Signal carries both an integer id (for switch dispatch) and a compile-time
// name (for instrumentation/diagnostics). User code defines signals as
// constexpr Signal values; no runtime registry needed.

#ifndef CHART_EVENT_HPP
#define CHART_EVENT_HPP

#include <memory>

namespace chart {

struct Signal {
    int id{0};
    const char* name{""};

    constexpr operator int() const { return id; }

    constexpr bool operator==(Signal o) const { return id == o.id; }
    constexpr bool operator!=(Signal o) const { return id != o.id; }
    constexpr bool operator< (Signal o) const { return id <  o.id; }
    constexpr bool operator<=(Signal o) const { return id <= o.id; }
    constexpr bool operator> (Signal o) const { return id >  o.id; }
    constexpr bool operator>=(Signal o) const { return id >= o.id; }
};

// System signals. Every state handler must respond to Entry/Exit/Init;
// None_Signal is internal — used by the HSM dispatcher to make a state
// handler report its super-state via `self->super(parent)` from its
// default branch.
constexpr Signal None_Signal       {0, "None"};
constexpr Signal Entry_Signal      {1, "Entry"};
constexpr Signal Exit_Signal       {2, "Exit"};
constexpr Signal Init_Signal       {3, "Init"};
constexpr int    User_Signal_Begin = 4;  // first id available to user code

struct Event {
    Signal sig{None_Signal};
    // shared_ptr<void> lets payloads be any type; consumer static_pointer_casts
    // back to the concrete type they expect. shared_ptr semantics also keep
    // payloads alive across thread boundaries (post → consume in another AO).
    std::shared_ptr<void> payload{};
};

}  // namespace chart

#endif  // CHART_EVENT_HPP
