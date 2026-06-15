// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

// test_hsm.cpp — exercises the LCA dispatcher independent of any AO thread.
// We feed events synchronously and capture the trace of writes / actions,
// then compare to the expected sequence.

#include <chart/event.hpp>
#include <chart/hsm.hpp>

#include <iostream>
#include <sstream>
#include <string>

using chart::Event;
using chart::Hsm;
using chart::Status;

namespace {

#define EXPECT_EQ(a, b)                                                     \
    do {                                                                    \
        auto _a = (a);                                                      \
        auto _b = (b);                                                      \
        if (!(_a == _b)) {                                                  \
            std::cerr << "EXPECT_EQ failed at " << __FILE__ << ":"          \
                      << __LINE__ << "\n  lhs=" << _a << "\n  rhs=" << _b   \
                      << "\n";                                              \
            std::exit(1);                                                   \
        }                                                                   \
    } while (0)

// Replicate the canonical HSM test chart, but record actions into a string
// buffer instead of stdout so we can assert on them.
class Probe : public Hsm {
public:
    int foo = 0;
    std::ostringstream log;
    void emit(const char* s) { log << s << ';'; }
    std::string take() {
        std::string out = log.str();
        log.str("");
        return out;
    }
};

constexpr chart::Signal A_Signal{chart::User_Signal_Begin, "A"};
constexpr chart::Signal B_Signal{chart::User_Signal_Begin + 1, "B"};
constexpr chart::Signal C_Signal{chart::User_Signal_Begin + 2, "C"};
constexpr chart::Signal D_Signal{chart::User_Signal_Begin + 3, "D"};
constexpr chart::Signal E_Signal{chart::User_Signal_Begin + 4, "E"};
constexpr chart::Signal F_Signal{chart::User_Signal_Begin + 5, "F"};
constexpr chart::Signal G_Signal{chart::User_Signal_Begin + 6, "G"};
constexpr chart::Signal H_Signal{chart::User_Signal_Begin + 7, "H"};
constexpr chart::Signal I_Signal{chart::User_Signal_Begin + 8, "I"};

static Status s   (Hsm*, Event const*);
static Status s1  (Hsm*, Event const*);
static Status s11 (Hsm*, Event const*);
static Status s2  (Hsm*, Event const*);
static Status s21 (Hsm*, Event const*);
static Status s211(Hsm*, Event const*);

static Status s(Hsm* h, Event const* e) {
    auto* self = static_cast<Probe*>(h);
    switch (e->sig) {
        case chart::Entry_Signal: self->emit("s-ENTRY"); return Status::HANDLED;
        case chart::Init_Signal:  self->emit("s-INIT");  return self->to(s11);
        case chart::Exit_Signal:  self->emit("s-EXIT");  return Status::HANDLED;
        case E_Signal: self->emit("s-E"); return self->to(s11);
        case I_Signal:
            self->emit("s-I");
            if (self->foo) { self->foo = 0; self->emit("foo=0"); }
            return Status::HANDLED;
        default: return self->super(&Hsm::top);
    }
}

static Status s1(Hsm* h, Event const* e) {
    auto* self = static_cast<Probe*>(h);
    switch (e->sig) {
        case chart::Entry_Signal: self->emit("s1-ENTRY"); return Status::HANDLED;
        case chart::Init_Signal:  self->emit("s1-INIT");  return self->to(s11);
        case chart::Exit_Signal:  self->emit("s1-EXIT");  return Status::HANDLED;
        case A_Signal: self->emit("s1-A"); return self->to(s1);
        case B_Signal: self->emit("s1-B"); return self->to(s11);
        case C_Signal: self->emit("s1-C"); return self->to(s2);
        case F_Signal: self->emit("s1-F"); return self->to(s211);
        case D_Signal:
            if (!self->foo) {
                self->emit("s1-D"); self->foo = 1; self->emit("foo=1");
                return self->to(s);
            }
            return Status::UNHANDLED;
        default: return self->super(s);
    }
}

static Status s11(Hsm* h, Event const* e) {
    auto* self = static_cast<Probe*>(h);
    switch (e->sig) {
        case chart::Entry_Signal: self->emit("s11-ENTRY"); return Status::HANDLED;
        case chart::Exit_Signal:  self->emit("s11-EXIT");  return Status::HANDLED;
        case G_Signal: self->emit("s11-G"); return self->to(s211);
        case H_Signal: self->emit("s11-H"); return self->to(s);
        case D_Signal:
            if (self->foo) {
                self->emit("s11-D"); self->foo = 0; self->emit("foo=0");
                return self->to(s1);
            }
            return Status::UNHANDLED;
        default: return self->super(s1);
    }
}

static Status s2(Hsm* h, Event const* e) {
    auto* self = static_cast<Probe*>(h);
    switch (e->sig) {
        case chart::Entry_Signal: self->emit("s2-ENTRY"); return Status::HANDLED;
        case chart::Init_Signal:  self->emit("s2-INIT");  return self->to(s211);
        case chart::Exit_Signal:  self->emit("s2-EXIT");  return Status::HANDLED;
        case I_Signal:
            self->emit("s2-I");
            if (!self->foo) { self->foo = 1; self->emit("foo=1"); }
            return Status::HANDLED;
        case C_Signal: self->emit("s2-C"); return self->to(s1);
        case F_Signal: self->emit("s2-F"); return self->to(s11);
        default: return self->super(s);
    }
}

static Status s21(Hsm* h, Event const* e) {
    auto* self = static_cast<Probe*>(h);
    switch (e->sig) {
        case chart::Entry_Signal: self->emit("s21-ENTRY"); return Status::HANDLED;
        case chart::Init_Signal:  self->emit("s21-INIT");  return self->to(s211);
        case chart::Exit_Signal:  self->emit("s21-EXIT");  return Status::HANDLED;
        case A_Signal: self->emit("s21-A"); return self->to(s21);
        case B_Signal: self->emit("s21-B"); return self->to(s211);
        case G_Signal: self->emit("s21-G"); return self->to(s11);
        default: return self->super(s2);
    }
}

static Status s211(Hsm* h, Event const* e) {
    auto* self = static_cast<Probe*>(h);
    switch (e->sig) {
        case chart::Entry_Signal: self->emit("s211-ENTRY"); return Status::HANDLED;
        case chart::Exit_Signal:  self->emit("s211-EXIT");  return Status::HANDLED;
        case D_Signal: self->emit("s211-D"); return self->to(s21);
        case H_Signal: self->emit("s211-H"); return self->to(s);
        default: return self->super(s21);
    }
}

void test_initial_transition() {
    Probe p;
    p.init(s2);
    EXPECT_EQ(p.take(),
              std::string("s-ENTRY;s2-ENTRY;s2-INIT;s21-ENTRY;s211-ENTRY;"));
}

void test_self_transition_runs_exit_init_entry() {
    Probe p;
    p.init(s2);
    p.take();
    p.dispatch(Event{A_Signal, nullptr});
    EXPECT_EQ(p.take(),
              std::string("s21-A;s211-EXIT;s21-EXIT;s21-ENTRY;s21-INIT;s211-ENTRY;"));
}

void test_lca_jump_across_branches() {
    Probe p;
    p.init(s2);
    p.take();
    p.dispatch(Event{C_Signal, nullptr});
    EXPECT_EQ(p.take(),
              std::string("s2-C;s211-EXIT;s21-EXIT;s2-EXIT;s1-ENTRY;s1-INIT;s11-ENTRY;"));
}

void test_unhandled_propagates_to_super() {
    Probe p;
    p.init(s2);
    p.take();
    p.dispatch(Event{D_Signal, nullptr});
    EXPECT_EQ(p.take(),
              std::string("s211-D;s211-EXIT;s21-INIT;s211-ENTRY;"));
    p.dispatch(Event{C_Signal, nullptr});
    p.take();
    p.dispatch(Event{D_Signal, nullptr});
    EXPECT_EQ(p.take(),
              std::string("s1-D;foo=1;s11-EXIT;s1-EXIT;s-INIT;s1-ENTRY;s11-ENTRY;"));
    EXPECT_EQ(p.foo, 1);
}

void test_internal_transition_no_exit_entry() {
    Probe p;
    p.init(s2);
    p.take();
    p.dispatch(Event{I_Signal, nullptr});
    EXPECT_EQ(p.take(), std::string("s2-I;foo=1;"));
    p.dispatch(Event{I_Signal, nullptr});
    EXPECT_EQ(p.take(), std::string("s2-I;"));
}

}  // namespace

int main() {
    test_initial_transition();
    test_self_transition_runs_exit_init_entry();
    test_lca_jump_across_branches();
    test_unhandled_propagates_to_super();
    test_internal_transition_no_exit_entry();
    std::cout << "test_hsm: OK" << std::endl;
    return 0;
}
