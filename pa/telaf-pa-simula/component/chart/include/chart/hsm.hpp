// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

// chart/hsm.hpp — hierarchical state machine core.
//
// Algorithm: LCA (least-common-ancestor) dispatch. State handlers are flat
// C-style function pointers — one function per state, a switch over signals
// inside. The default branch reports the state's super-state via
// self->super(parent). This keeps each state's logic in one place.
//
// Public API used by user code:
//   - chart::Hsm                 : base class
//   - chart::Status              : HANDLED / IGNORED / UNHANDLED /
//                                  NEED_TO_TRANSFER / TO_SUPER
//   - chart::StateFn             : Status (*)(Hsm*, Event const*)
//   - self->to(target_fn)        : declare an external transition
//   - self->super(parent_fn)     : declare super-state (used in default branch)
//   - chart::Hsm::top            : implicit root; user states ultimately
//                                  super() to it

#ifndef CHART_HSM_HPP
#define CHART_HSM_HPP

#include "assert.hpp"
#include "event.hpp"

namespace chart {

class Hsm;
enum class Status { HANDLED, IGNORED, UNHANDLED, NEED_TO_TRANSFER, TO_SUPER };
using StateFn = Status (*)(Hsm*, Event const*);

// Callback type invoked after every state transition (including start_at).
// sig  = signal that triggered the transition (None_Signal for start_at).
// from = state before transition.
// to   = state after transition (settled leaf).
// Registered via Hsm::set_trace(); nullptr disables (zero overhead).
using TraceFn = void (*)(void* ctx, Signal sig, StateFn from, StateFn to);

// Kind of dispatch event reported to a DispatchProbeFn. HandlerVisit fires
// once per state handler invoked during dispatch's walk-up; Hook and
// Ignored fire once at resolution when the RTC completes without a
// transition (see Hsm::dispatch below for exact semantics).
enum class DispatchEvent { HandlerVisit, Hook, Ignored };

// Callback fired from Hsm::dispatch to expose the walk to instrumentation
// (Spy) without coupling Hsm to it. Registered via set_dispatch_probe;
// nullptr disables (zero overhead beyond a pointer null check).
using DispatchProbeFn = void (*)(void* ctx, Signal sig, StateFn s,
                                 DispatchEvent kind);

class Hsm {
public:
    // Helpers used inside state handlers. They mutate temp_ and report
    // the symbolic intent via the return value.
    Status to(StateFn t)    { temp_ = t; return Status::NEED_TO_TRANSFER; }
    Status super(StateFn p) { temp_ = p; return Status::TO_SUPER;          }

    // Implicit root state. Returns IGNORED, never updates temp_, stops upward
    // walks. User state handlers should super(&Hsm::top) somewhere on their
    // hierarchy chain.
    static Status top(Hsm*, Event const*) { return Status::IGNORED; }

    // Drive the chart from "uninitialized" into `initial`, then recursively
    // apply Init_Signal until the chart settles on a leaf state. Calls all
    // ENTRY actions along the way.
    void init(StateFn initial);

    // Run-to-completion: feed `e` to current state, walk up via super on
    // UNHANDLED/TO_SUPER, perform a transition (with the matching EXIT/ENTRY
    // chain + INIT recursion) if the active handler returns NEED_TO_TRANSFER.
    void dispatch(Event const& e);

    StateFn current_state() const { return state_; }

    // Register a trace callback. Called after every transition with the
    // signal, source state, and settled destination state. Pass nullptr to
    // disable. ctx is forwarded verbatim to the callback.
    void set_trace(TraceFn fn, void* ctx = nullptr) {
        trace_fn_  = fn;
        trace_ctx_ = ctx;
    }

    // Register a dispatch probe. Called from dispatch() for each handler
    // visited (HandlerVisit) and once at resolution (Hook when a user signal
    // returned HANDLED without transitioning, Ignored when the event walked
    // all the way to top). Pass nullptr to disable.
    void set_dispatch_probe(DispatchProbeFn fn, void* ctx = nullptr) {
        probe_fn_  = fn;
        probe_ctx_ = ctx;
    }

    // True iff the most recent dispatch() resolved to Status::IGNORED
    // (the event walked past every registered ancestor to Hsm::top).
    // Mirrors miros' self.event.ignored — without it a silently dropped
    // event is undetectable from outside the chart.
    bool last_event_ignored() const { return last_ignored_; }

protected:
    StateFn state_ = nullptr;  // current active leaf state
    StateFn temp_  = nullptr;  // scratch slot mutated by to() / super()
    TraceFn trace_fn_  = nullptr;
    void*   trace_ctx_ = nullptr;
    DispatchProbeFn probe_fn_  = nullptr;
    void*           probe_ctx_ = nullptr;
    bool            last_ignored_ = false;

    // Max depth supported. UML charts rarely need more than 5–6.
    static constexpr int kMaxNest = 8;

    // Walk `from` upward by repeatedly issuing None_Signal; returns the
    // chain length (states stored into `out`, top-most user state last,
    // never includes &Hsm::top). Mutates temp_ along the way and restores it.
    int collect_lineage(StateFn from, StateFn out[kMaxNest]);
};

// ---------------------------------------------------------------------------
// inline implementations
// ---------------------------------------------------------------------------

inline int Hsm::collect_lineage(StateFn from, StateFn out[kMaxNest]) {
    StateFn save = temp_;
    Event empty{None_Signal, {}};
    int n = 0;
    out[n++] = from;
    temp_ = from;
    StateFn cur = from;
    cur(this, &empty);  // -> sets temp_ to its super (or leaves it for top)
    while (temp_ != &Hsm::top && temp_ != cur) {
        cur = temp_;
        if (n >= kMaxNest) { CHART_ASSERT(false); break; }
        out[n++] = cur;
        cur(this, &empty);
    }
    temp_ = save;
    return n;
}

inline void Hsm::init(StateFn initial) {
    CHART_REQUIRE(initial != nullptr);
    state_ = &Hsm::top;
    temp_  = initial;
    StateFn t = state_;  // = &Hsm::top initially; later = the just-entered leaf.

    Event empty{None_Signal,  {}};
    Event entry{Entry_Signal, {}};
    Event init_evt{Init_Signal, {}};

    do {
        // Drill from temp_ up to t, recording the path on the way.
        StateFn path[kMaxNest];
        int ip = 0;
        path[0] = temp_;
        StateFn cur = path[0];
        cur(this, &empty);  // updates temp_ to super(cur)
        while (temp_ != t) {
            if (ip + 1 >= kMaxNest) { CHART_ASSERT(false); break; }
            ++ip;
            path[ip] = temp_;
            cur = temp_;
            cur(this, &empty);
        }

        // ENTRY in reverse: from t-side down to the deepest target.
        do {
            path[ip](this, &entry);
        } while (--ip >= 0);

        t = path[0];  // leaf just entered
        temp_ = t;    // restore in case INIT inspects it
    } while (t(this, &init_evt) == Status::NEED_TO_TRANSFER);

    state_ = t;
    temp_  = t;
    if (trace_fn_)
        trace_fn_(trace_ctx_, None_Signal, &Hsm::top, state_);
}

inline void Hsm::dispatch(Event const& e) {
    Event empty{None_Signal,   {}};
    Event exit_evt{Exit_Signal, {}};
    Event entry_evt{Entry_Signal, {}};
    Event init_evt{Init_Signal, {}};

    StateFn original_state = state_;
    StateFn s = nullptr;  // handler that processes the event
    Status r;
    last_ignored_ = false;

    // 1) Walk current → super → super … until a handler doesn't return TO_SUPER.
    temp_ = state_;
    do {
        s = temp_;
        // Instrumentation: record the visit before invoking the handler so
        // the trace order matches the walk. Cheap null check when disabled.
        if (probe_fn_)
            probe_fn_(probe_ctx_, e.sig, s, DispatchEvent::HandlerVisit);
        r = s(this, &e);
        if (r == Status::UNHANDLED) {
            // Guard rejected the transition; treat as if the state delegated to
            // its super-state. We must walk to the super manually (UNHANDLED
            // does not update temp_).
            s(this, &empty);
            r = Status::TO_SUPER;
        }
    } while (r == Status::TO_SUPER);

    if (r != Status::NEED_TO_TRANSFER) {
        // HANDLED or IGNORED — chart state unchanged.
        if (probe_fn_) {
            if (r == Status::IGNORED) {
                probe_fn_(probe_ctx_, e.sig, s, DispatchEvent::Ignored);
            } else if (e.sig.id >= User_Signal_Begin) {
                // User signal HANDLED without transition = hook.
                probe_fn_(probe_ctx_, e.sig, s, DispatchEvent::Hook);
            }
        }
        if (r == Status::IGNORED) last_ignored_ = true;
        state_ = original_state;
        temp_  = original_state;
        return;
    }

    // 2) Transition: source = s, target = temp_ (set by to()).
    StateFn target = temp_;

    // 2a) EXIT actions from original_state up to (but excluding) s.
    StateFn cur = original_state;
    while (cur != s) {
        if (cur(this, &exit_evt) == Status::HANDLED) {
            // EXIT was handled — temp_ wasn't touched, walk to super manually.
            cur(this, &empty);
        }
        cur = temp_;
    }

    // 2b) Self-transition is special: LCA is parent of s, exit + re-enter s.
    if (s == target) {
        s(this, &exit_evt);
        target(this, &entry_evt);
    } else {
        // Build target's ancestor list (target itself first, top-most user state last).
        StateFn tpath[kMaxNest];
        int tn = collect_lineage(target, tpath);

        // 2c) Walk from s upward; LCA is the first state present in tpath.
        int lca_idx = -1;
        cur = s;
        while (cur != &Hsm::top) {
            for (int i = 0; i < tn; ++i) {
                if (tpath[i] == cur) { lca_idx = i; break; }
            }
            if (lca_idx >= 0) break;
            if (cur(this, &exit_evt) == Status::HANDLED) {
                cur(this, &empty);
            }
            cur = temp_;
        }
        if (lca_idx < 0) lca_idx = tn;  // LCA = &Hsm::top → enter all of tpath.

        // 2d) ENTRY from just-below-LCA down to target (tpath[0]).
        int start = (lca_idx == tn) ? tn - 1 : lca_idx - 1;
        for (int i = start; i >= 0; --i) {
            tpath[i](this, &entry_evt);
        }
    }

    // 3) INIT recursion on the (possibly composite) target.
    cur = target;
    while (true) {
        temp_ = cur;
        if (cur(this, &init_evt) != Status::NEED_TO_TRANSFER) break;

        StateFn new_target = temp_;
        // ENTRY chain from cur (exclusive) down to new_target.
        StateFn npath[kMaxNest];
        int np = 0;
        npath[np++] = new_target;
        StateFn x = new_target;
        x(this, &empty);
        while (temp_ != cur && temp_ != &Hsm::top) {
            if (np >= kMaxNest) { CHART_ASSERT(false); break; }
            npath[np++] = temp_;
            x = temp_;
            x(this, &empty);
        }
        for (int i = np - 1; i >= 0; --i) {
            npath[i](this, &entry_evt);
        }
        cur = new_target;
    }

    state_ = cur;
    temp_  = cur;
    if (trace_fn_)
        trace_fn_(trace_ctx_, e.sig, original_state, state_);
}

}  // namespace chart

#endif  // CHART_HSM_HPP
