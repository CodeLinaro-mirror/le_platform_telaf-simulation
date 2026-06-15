// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

// chart/spy.hpp — opt-in, thread-safe trace recorder.
//
// Spy is intentionally decoupled from Hsm/ActiveObject: it doesn't get
// auto-called from dispatch (that would force a name lookup on every signal
// even when disabled). Instead, user code (or a small wrapper in their state
// handler) calls Spy::record("...") at moments worth tracing.
//
// A small StateNames helper is provided for projects that want symbolic
// state names in their trace lines without hand-maintaining a map.
// CHART_NAMED_STATE(fn, display_name) registers a state-handler function
// pointer with StateNames at translation-unit static-init time, so spy
// output prints "DataCallSession::Idle" instead of a raw address — without a
// hand-maintained, drift-prone central registration list.
//
// Place it at file (namespace) scope, immediately after the handler's
// definition, in the SAME namespace the handler lives in:
//
//   chart::Status Idle_St(chart::Hsm* h, chart::Event const* e) { ... }
//   CHART_NAMED_STATE(Idle_St, "DataCallSession::Idle");
//
// Safe against the static-init-order fiasco: StateNames::instance() is a
// Meyers singleton, constructed on first use by the registrar's ctor.
//
// Graceful degradation: a handler with no CHART_NAMED_STATE simply shows up
// as "<unknown>" in traces — it never fails to compile or crashes.
//
// Three verbosity tiers (SpyMode), independent of the two ring buffers:
//   Off     — nothing recorded.
//   On      — transition trace only (record_trace). This is the "what changed"
//             view: [  123.456] [ao_name] e->Signal() FromState->ToState
//   Verbose — On plus every per-handler visit, HOOK, IGNORED and scribble
//             (record_dispatch / record / scribble):
//             [  123.456] [ao_name] SIGNAL:state_name[:HOOK]
//
// The trace and spy lines each still have an independent ring buffer (default
// 4096 lines); SpyMode only decides which record calls fire, not where they
// land. enable(bool)/enabled() remain as backward-compat shims over set_mode.

#ifndef CHART_SPY_HPP
#define CHART_SPY_HPP

#include "hsm.hpp"
#include "instrument.hpp"

#include <atomic>
#include <cstddef>
#include <cstdio>
#include <ctime>
#include <deque>
#include <mutex>
#include <ostream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace chart {

// Verbosity tier for Spy. Off < On < Verbose; higher tiers are supersets.
enum class SpyMode { Off = 0, On = 1, Verbose = 2 };

class StateNames {
public:
    static void reg(StateFn fn, std::string name) {
        auto& s = instance();
        std::lock_guard<std::mutex> lk(s.m_);
        s.map_[fn] = std::move(name);
    }
    static std::string lookup(StateFn fn) {
        if (fn == &Hsm::top) return "top";
        auto& s = instance();
        std::lock_guard<std::mutex> lk(s.m_);
        auto it = s.map_.find(fn);
        return it != s.map_.end() ? it->second : std::string{"<unknown>"};
    }

private:
    StateNames() = default;
    static StateNames& instance() { static StateNames s; return s; }
    std::mutex m_;
    std::unordered_map<StateFn, std::string> map_;
};

#define CHART_NAMED_STATE(fn, display_name)                                   \
    namespace {                                                               \
    struct ChartNameReg_##fn {                                                \
        ChartNameReg_##fn() { ::chart::StateNames::reg(&fn, (display_name)); } \
    } chart_name_reg_##fn##_inst;                                             \
    }

class Spy {
public:
    // Set the verbosity tier. Entering Verbose bumps the spy ring buffer to
    // 16k lines (comfortable headroom for a long soak) so callers don't have
    // to remember; leaving Verbose does not shrink it back.
    static void set_mode(SpyMode m) {
        instance().mode_.store(static_cast<int>(m));
        if (m == SpyMode::Verbose) max_size(16384);
    }
    static SpyMode mode() {
        return static_cast<SpyMode>(instance().mode_.load());
    }

    // Backward-compat shims over set_mode: true -> On, false -> Off.
    static void enable(bool on) { set_mode(on ? SpyMode::On : SpyMode::Off); }
    static bool enabled() { return mode() != SpyMode::Off; }

    // Optional real-time tee for spy lines. Pass nullptr to disable.
    static void live_sink(std::ostream* os) {
        auto& s = instance();
        std::lock_guard<std::mutex> lk(s.m_);
        s.live_ = os;
    }

    // Optional real-time tee for trace lines. Pass nullptr to disable.
    static void live_trace_sink(std::ostream* os) {
        auto& s = instance();
        std::lock_guard<std::mutex> lk(s.m_);
        s.live_trace_ = os;
    }

    // Free-form spy line, auto-prefixed with monotonic timestamp.
    // Typical format: [  123.456] [ao_name] SIGNAL:state_name
    static void record(std::string line) {
        auto& s = instance();
        if (s.mode_.load() < static_cast<int>(SpyMode::Verbose)) return;
        char prefix[32];
        std::snprintf(prefix, sizeof(prefix), "[%10.3f] ", mono_secs());
        std::string full = prefix + line;
        std::lock_guard<std::mutex> lk(s.m_);
        if (s.live_) { (*s.live_) << full << '\n'; s.live_->flush(); }
        s.rtc_.push_back(full);
        s.lines_.push_back(std::move(full));
        while (s.lines_.size() > s.max_) s.lines_.pop_front();
    }

    // Structured spy record — matches miros spy format.
    // Produces: [  123.456] [ao_name] SIGNAL:state_name[:HOOK]
    static void record(std::string const& ao_name, std::string const& signal,
                       std::string const& state, bool hook = false) {
        std::string line = "[" + ao_name + "] " + signal + ":" + state;
        if (hook) line += ":HOOK";
        record(std::move(line));
    }

    // Per-handler-visit record fired from Hsm::dispatch during walk-up
    // (see DispatchEvent::HandlerVisit). Appended to both the full ring
    // buffer and the current-RTC buffer so callers can emit one block per
    // run-to-completion step (mirrors miros' post_action wrappers).
    static void record_dispatch(std::string const& ao_name,
                                 std::string const& signal,
                                 std::string const& state) {
        auto& s = instance();
        if (s.mode_.load() < static_cast<int>(SpyMode::Verbose)) return;
        char prefix[32];
        std::snprintf(prefix, sizeof(prefix), "[%10.3f] ", mono_secs());
        std::string full =
            std::string(prefix) + "[" + ao_name + "] " + signal + ":" + state;
        std::lock_guard<std::mutex> lk(s.m_);
        if (s.live_) { (*s.live_) << full << '\n'; s.live_->flush(); }
        s.lines_.push_back(full);
        while (s.lines_.size() > s.max_) s.lines_.pop_front();
        s.rtc_.push_back(std::move(full));
    }

    // Free-form user log line interleaved into the current-RTC buffer.
    // Also appended to the full ring so `dump()` still sees it in order.
    // Miros equivalent: chart.scribble("...").
    static void scribble(std::string line) {
        auto& s = instance();
        if (s.mode_.load() < static_cast<int>(SpyMode::Verbose)) return;
        char prefix[32];
        std::snprintf(prefix, sizeof(prefix), "[%10.3f] ", mono_secs());
        std::string full = prefix + std::move(line);
        std::lock_guard<std::mutex> lk(s.m_);
        if (s.live_) { (*s.live_) << full << '\n'; s.live_->flush(); }
        s.lines_.push_back(full);
        while (s.lines_.size() > s.max_) s.lines_.pop_front();
        s.rtc_.push_back(std::move(full));
    }

    // Snapshot of lines recorded during the current run-to-completion step.
    // The caller typically clear_rtc()s at the start of each dispatch, lets
    // record_dispatch / scribble accumulate, then reads back here.
    static std::vector<std::string> rtc_lines() {
        auto& s = instance();
        std::lock_guard<std::mutex> lk(s.m_);
        return {s.rtc_.begin(), s.rtc_.end()};
    }

    // Reset the current-RTC buffer. Full ring buffers are unaffected.
    static void clear_rtc() {
        auto& s = instance();
        std::lock_guard<std::mutex> lk(s.m_);
        s.rtc_.clear();
    }

    static std::size_t rtc_size() {
        auto& s = instance();
        std::lock_guard<std::mutex> lk(s.m_);
        return s.rtc_.size();
    }

    // Transition record — called automatically by ActiveObject on every
    // state transition (start_at and dispatch). Matches miros trace format.
    // Produces: [  123.456] [ao_name] e->Signal() FromState->ToState
    static void record_trace(std::string const& ao_name, std::string const& sig,
                              std::string const& from, std::string const& to) {
        auto& s = instance();
        if (s.mode_.load() < static_cast<int>(SpyMode::On)) return;
        char prefix[32];
        std::snprintf(prefix, sizeof(prefix), "[%10.3f] ", mono_secs());
        std::string line = std::string(prefix)
                         + "[" + ao_name + "] e->" + sig + "() "
                         + from + "->" + to;
        std::lock_guard<std::mutex> lk(s.m_);
        if (s.live_trace_) { (*s.live_trace_) << line << '\n'; s.live_trace_->flush(); }
        s.traces_.push_back(std::move(line));
        while (s.traces_.size() > s.max_trace_) s.traces_.pop_front();
    }

    static void dump(std::ostream& os) {
        auto& s = instance();
        std::lock_guard<std::mutex> lk(s.m_);
        for (auto const& l : s.lines_) os << l << '\n';
    }

    static void dump_trace(std::ostream& os) {
        auto& s = instance();
        std::lock_guard<std::mutex> lk(s.m_);
        for (auto const& l : s.traces_) os << l << '\n';
    }

    static void clear() {
        auto& s = instance();
        std::lock_guard<std::mutex> lk(s.m_);
        s.lines_.clear();
    }

    static void clear_trace() {
        auto& s = instance();
        std::lock_guard<std::mutex> lk(s.m_);
        s.traces_.clear();
    }

    static void max_size(std::size_t n) {
        auto& s = instance();
        std::lock_guard<std::mutex> lk(s.m_);
        s.max_ = n;
        while (s.lines_.size() > s.max_) s.lines_.pop_front();
    }

    static void max_trace_size(std::size_t n) {
        auto& s = instance();
        std::lock_guard<std::mutex> lk(s.m_);
        s.max_trace_ = n;
        while (s.traces_.size() > s.max_trace_) s.traces_.pop_front();
    }

    static std::size_t size() {
        auto& s = instance();
        std::lock_guard<std::mutex> lk(s.m_);
        return s.lines_.size();
    }

    static std::size_t trace_size() {
        auto& s = instance();
        std::lock_guard<std::mutex> lk(s.m_);
        return s.traces_.size();
    }

private:
    Spy() = default;
    static Spy& instance() { static Spy s; return s; }

    static double mono_secs() {
        struct timespec ts{};
        clock_gettime(CLOCK_MONOTONIC, &ts);
        return static_cast<double>(ts.tv_sec)
             + static_cast<double>(ts.tv_nsec) * 1e-9;
    }

    std::atomic<int> mode_{static_cast<int>(SpyMode::Off)};
    std::mutex m_;
    // Spy ring buffer.
    std::deque<std::string> lines_;
    std::size_t max_{4096};
    std::ostream* live_{nullptr};
    // Trace ring buffer (independent from spy lines).
    std::deque<std::string> traces_;
    std::size_t max_trace_{4096};
    std::ostream* live_trace_{nullptr};
    // Current run-to-completion buffer. Cleared by clear_rtc() at the
    // caller's discretion (typically at the start of each dispatch).
    std::deque<std::string> rtc_;
};

// Instrument adapter that forwards to the global Spy singleton.
// Name is captured at construction — zero per-call string work for it.
class SpyInstrument : public Instrument {
public:
    explicit SpyInstrument(std::string ao_name) : name_(std::move(ao_name)) {}

    void on_transition(Signal sig, StateFn from, StateFn to) override {
        Spy::record_trace(name_, sig.name,
                          StateNames::lookup(from), StateNames::lookup(to));
    }

    void on_visit(Signal sig, StateFn state) override {
        Spy::record_dispatch(name_, sig.name, StateNames::lookup(state));
    }

    void on_hook(Signal sig, StateFn state) override {
        Spy::record(name_, sig.name, StateNames::lookup(state), true);
    }

    void on_ignored(Signal sig, StateFn state) override {
        Spy::record(name_, std::string(sig.name) + ":IGNORED", StateNames::lookup(state));
    }

private:
    std::string name_;
};

}  // namespace chart

#endif  // CHART_SPY_HPP
