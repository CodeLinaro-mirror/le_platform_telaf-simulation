// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

// chart/instrument.hpp — abstract instrumentation interface for ActiveObject.
//
// An Instrument observes HSM dispatch activity without coupling ActiveObject
// to any specific tracing backend (Spy, syslog, test mock, etc.).
// Each ActiveObject holds an optional Instrument* — nullptr means zero-cost
// bypass (no virtual dispatch on the hot path).

#ifndef CHART_INSTRUMENT_HPP
#define CHART_INSTRUMENT_HPP

#include "hsm.hpp"

namespace chart {

class Instrument {
public:
    virtual ~Instrument() = default;

    // A state transition completed.
    virtual void on_transition(Signal sig, StateFn from, StateFn to) = 0;

    // A state handler was visited during the dispatch walk-up.
    virtual void on_visit(Signal sig, StateFn state) = 0;

    // A user signal was HANDLED without causing a transition (hook).
    virtual void on_hook(Signal sig, StateFn state) = 0;

    // The event walked to top without being handled.
    virtual void on_ignored(Signal sig, StateFn state) = 0;
};

}  // namespace chart

#endif  // CHART_INSTRUMENT_HPP
