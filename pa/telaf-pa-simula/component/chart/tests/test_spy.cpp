// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear
//
// test_spy.cpp — T-02 regression for chart instrumentation additions:
//   * per-handler-visit records emitted from Hsm::dispatch (record_dispatch)
//   * HOOK detection: user signal returning HANDLED without transition
//   * Hsm::last_event_ignored() flag
//   * Spy::scribble() interleaving user log lines with auto trace
//   * per-RTC buffer (rtc_lines / clear_rtc)
//
// The test drives Hsm directly (no worker thread) so ordering is
// deterministic. It also wires a small probe by hand to keep the test
// independent of ActiveObject's registration.

#include <chart/event.hpp>
#include <chart/hsm.hpp>
#include <chart/spy.hpp>

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

using chart::DispatchEvent;
using chart::Event;
using chart::Hsm;
using chart::Signal;
using chart::Spy;
using chart::SpyMode;
using chart::StateFn;
using chart::StateNames;
using chart::Status;

namespace {

#define EXPECT_TRUE(cond)                                                     \
    do {                                                                      \
        if (!(cond)) {                                                        \
            std::cerr << "EXPECT_TRUE failed at " << __FILE__ << ":"          \
                      << __LINE__ << " -- " << #cond << "\n";                 \
            std::exit(1);                                                    \
        }                                                                    \
    } while (0)

bool contains(std::vector<std::string> const& lines, std::string const& needle) {
    for (auto const& l : lines) {
        if (l.find(needle) != std::string::npos) return true;
    }
    return false;
}

// Chart:  parent -> child   (child super = parent)
// PING handled in child (transitions to child again = self-transition -> uses TRAN path).
// HOOK signal handled in parent (HANDLED, no transition -> HOOK).
// IGN signal reaches top -> IGNORED.
constexpr Signal Ping_Signal{chart::User_Signal_Begin, "Ping"};
constexpr Signal Hook_Signal{chart::User_Signal_Begin + 1, "Hook"};
constexpr Signal Ign_Signal {chart::User_Signal_Begin + 2, "Ign"};

static Status parent(Hsm*, Event const*);
static Status child (Hsm*, Event const*);

static Status parent(Hsm* h, Event const* e) {
    switch (e->sig) {
        case chart::Entry_Signal: return Status::HANDLED;
        case chart::Exit_Signal:  return Status::HANDLED;
        case chart::Init_Signal:  return h->to(child);
        case Hook_Signal:         return Status::HANDLED;   // no trans -> HOOK
        default: return h->super(&Hsm::top);
    }
}
static Status child(Hsm* h, Event const* e) {
    switch (e->sig) {
        case chart::Entry_Signal: return Status::HANDLED;
        case chart::Exit_Signal:  return Status::HANDLED;
        case Ping_Signal:         return h->to(child);      // self-transition
        default: return h->super(parent);
    }
}

class Probe : public Hsm {};

}  // namespace

int main() {
    StateNames::reg(parent, "parent");
    StateNames::reg(child,  "child");

    // Verbose: per-handler-visit / HOOK / IGNORED / scribble all fire.
    Spy::set_mode(SpyMode::Verbose);
    Spy::clear();
    Spy::clear_rtc();

    Probe p;
    // Register the same probe ActiveObject would install, but by hand.
    p.set_dispatch_probe(
        [](void* ctx, Signal sig, StateFn s, DispatchEvent kind) {
            (void)ctx;
            switch (kind) {
                case DispatchEvent::HandlerVisit:
                    Spy::record_dispatch("probe",
                                         sig.name,
                                         StateNames::lookup(s));
                    break;
                case DispatchEvent::Hook:
                    Spy::record("probe", sig.name,
                                StateNames::lookup(s), /*hook=*/true);
                    break;
                case DispatchEvent::Ignored:
                    Spy::record("probe",
                                std::string(sig.name) + ":IGNORED",
                                StateNames::lookup(s));
                    break;
            }
        },
        &p);

    // Wire the transition trace probe too (ActiveObject installs this in
    // real code); needed to exercise the On tier, which records traces only.
    p.set_trace(
        [](void* ctx, Signal sig, StateFn from, StateFn to) {
            (void)ctx;
            Spy::record_trace("probe", sig.name,
                              StateNames::lookup(from), StateNames::lookup(to));
        },
        &p);

    p.init(parent);
    EXPECT_TRUE(p.current_state() == child);

    // --- Case 1: PING -> self-transition; per-handler visits should be
    // recorded in the RTC buffer. Not a HOOK, not IGNORED.
    Spy::clear_rtc();
    Event ping{Ping_Signal, {}};
    p.dispatch(ping);
    EXPECT_TRUE(!p.last_event_ignored());
    auto rtc1 = Spy::rtc_lines();
    EXPECT_TRUE(!rtc1.empty());
    EXPECT_TRUE(contains(rtc1, "Ping:child"));
    EXPECT_TRUE(!contains(rtc1, ":HOOK"));

    // --- Case 2: HOOK -> parent HANDLED w/o transition, must record HOOK.
    Spy::clear_rtc();
    Event hookE{Hook_Signal, {}};
    p.dispatch(hookE);
    EXPECT_TRUE(!p.last_event_ignored());
    auto rtc2 = Spy::rtc_lines();
    // Walk visits: child (super to parent), then parent (HANDLED).
    EXPECT_TRUE(contains(rtc2, "Hook:child"));
    EXPECT_TRUE(contains(rtc2, "Hook:parent"));
    EXPECT_TRUE(contains(rtc2, ":HOOK"));

    // --- Case 3: IGN -> unhandled all the way to top. last_event_ignored.
    Spy::clear_rtc();
    Event igE{Ign_Signal, {}};
    p.dispatch(igE);
    EXPECT_TRUE(p.last_event_ignored());
    auto rtc3 = Spy::rtc_lines();
    EXPECT_TRUE(contains(rtc3, ":IGNORED"));

    // --- Case 4: scribble adds a user line into the current RTC buffer.
    Spy::clear_rtc();
    Spy::scribble("hello from user");
    Event ping2{Ping_Signal, {}};
    p.dispatch(ping2);
    auto rtc4 = Spy::rtc_lines();
    EXPECT_TRUE(contains(rtc4, "hello from user"));
    EXPECT_TRUE(contains(rtc4, "Ping:child"));

    // --- Case 5: clear_rtc empties the current-RTC buffer without
    // touching the full ring.
    std::size_t total_before = Spy::size();
    Spy::clear_rtc();
    EXPECT_TRUE(Spy::rtc_size() == 0);
    EXPECT_TRUE(Spy::size() == total_before);

    // --- Case 6: Off flips record_dispatch/scribble into no-ops.
    Spy::set_mode(SpyMode::Off);
    EXPECT_TRUE(!Spy::enabled());
    Spy::clear_rtc();
    p.dispatch(ping);
    Spy::scribble("silent");
    EXPECT_TRUE(Spy::rtc_size() == 0);

    // --- Case 7: On records transition traces but NOT per-handler-visit /
    // scribble (those are Verbose-only). This is the core on != verbose fix.
    Spy::set_mode(SpyMode::On);
    EXPECT_TRUE(Spy::enabled());
    EXPECT_TRUE(Spy::mode() == SpyMode::On);
    Spy::clear();
    Spy::clear_rtc();
    Spy::clear_trace();
    p.dispatch(ping);         // self-transition -> emits a trace line
    Spy::scribble("still silent under On");
    EXPECT_TRUE(Spy::rtc_size() == 0);  // no per-handler/scribble records
    EXPECT_TRUE(Spy::size() == 0);      // spy ring untouched under On
    EXPECT_TRUE(Spy::trace_size() > 0); // but transition trace fired

    // --- Case 8: enable/enabled shims still map onto the tri-state mode.
    Spy::enable(false);
    EXPECT_TRUE(Spy::mode() == SpyMode::Off);
    EXPECT_TRUE(!Spy::enabled());
    Spy::enable(true);
    EXPECT_TRUE(Spy::mode() == SpyMode::On);
    EXPECT_TRUE(Spy::enabled());

    std::cout << "test_spy PASSED\n";
    return 0;
}
