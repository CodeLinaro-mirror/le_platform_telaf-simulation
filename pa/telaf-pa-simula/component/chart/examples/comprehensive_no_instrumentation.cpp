// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

// =============================================================================
// Equivalent of examples/comprehensive_no_instrumentation.py.
//
// Uses chart (this repo's chart-cpp/) in the "one state = one function = one
// switch" style: state hierarchy, extended variable foo, guards, self
// transitions, and INIT recursion correspond 1:1 with the Python original.
//
// State hierarchy (canonical HSM test chart):
//   s (top, default INIT → s11)
//     ├── s1   (default INIT → s11)
//     │     └── s11
//     └── s2   (default INIT → s211, initial active state)
//           └── s21  (default INIT → s211)
//                 └── s211
//
// Build:
//   g++ -std=c++17 -I chart-cpp/include examples/comprehensive_no_instrumentation.cpp -lpthread -o /tmp/comp
// =============================================================================

#include <chart/chart.hpp>

#include <cctype>
#include <iostream>
#include <string>
#include <thread>
#include <unordered_map>

using chart::Event;
using chart::Hsm;
using chart::Status;

// ---- Application state machine: foo and write() hung off ActiveObject -----
class ExampleStatechart : public chart::ActiveObject {
public:
    using ActiveObject::ActiveObject;
    int foo = 0;
    static void write(const char* s) { std::cout << s << ';' << std::flush; }
};

// ---- User signals: constexpr Signal for switch and Event construction
constexpr chart::Signal A_Signal{chart::User_Signal_Begin, "A"};
constexpr chart::Signal B_Signal{chart::User_Signal_Begin + 1, "B"};
constexpr chart::Signal C_Signal{chart::User_Signal_Begin + 2, "C"};
constexpr chart::Signal D_Signal{chart::User_Signal_Begin + 3, "D"};
constexpr chart::Signal E_Signal{chart::User_Signal_Begin + 4, "E"};
constexpr chart::Signal F_Signal{chart::User_Signal_Begin + 5, "F"};
constexpr chart::Signal G_Signal{chart::User_Signal_Begin + 6, "G"};
constexpr chart::Signal H_Signal{chart::User_Signal_Begin + 7, "H"};
constexpr chart::Signal I_Signal{chart::User_Signal_Begin + 8, "I"};

// ---- Forward declarations: states transition between each other ----------
static Status s   (Hsm*, Event const*);
static Status s1  (Hsm*, Event const*);
static Status s11 (Hsm*, Event const*);
static Status s2  (Hsm*, Event const*);
static Status s21 (Hsm*, Event const*);
static Status s211(Hsm*, Event const*);

// ---- State implementations: each function maps 1:1 to its Python sibling -
static Status s(Hsm* h, Event const* e) {
    auto* self = static_cast<ExampleStatechart*>(h);
    switch (e->sig) {
        case chart::Entry_Signal: self->write("s-ENTRY"); return Status::HANDLED;
        case chart::Init_Signal:  self->write("s-INIT");  return self->to(s11);
        case chart::Exit_Signal:  self->write("s-EXIT");  return Status::HANDLED;
        case E_Signal:            self->write("s-E");     return self->to(s11);
        case I_Signal:
            self->write("s-I");
            if (self->foo) { self->foo = 0; self->write("foo = 0"); }
            return Status::HANDLED;
        default: return self->super(&Hsm::top);
    }
}

static Status s1(Hsm* h, Event const* e) {
    auto* self = static_cast<ExampleStatechart*>(h);
    switch (e->sig) {
        case chart::Entry_Signal: self->write("s1-ENTRY"); return Status::HANDLED;
        case chart::Init_Signal:  self->write("s1-INIT");  return self->to(s11);
        case chart::Exit_Signal:  self->write("s1-EXIT"); return Status::HANDLED;
        case A_Signal: self->write("s1-A"); return self->to(s1);  // self
        case B_Signal: self->write("s1-B"); return self->to(s11);
        case C_Signal: self->write("s1-C"); return self->to(s2);
        case F_Signal: self->write("s1-F"); return self->to(s211);
        case D_Signal:
            // Returning UNHANDLED when the guard fails; the dispatcher
            // forwards the event to the parent state s.
            if (!self->foo) {
                self->write("s1-D");
                self->foo = 1; self->write("foo = 1");
                return self->to(s);
            }
            return Status::UNHANDLED;
        default: return self->super(s);
    }
}

static Status s11(Hsm* h, Event const* e) {
    auto* self = static_cast<ExampleStatechart*>(h);
    switch (e->sig) {
        case chart::Entry_Signal: self->write("s11-ENTRY"); return Status::HANDLED;
        case chart::Exit_Signal:  self->write("s11-EXIT");  return Status::HANDLED;
        case G_Signal: self->write("s11-G"); return self->to(s211);
        case H_Signal: self->write("s11-H"); return self->to(s);
        case D_Signal:
            if (self->foo) {
                self->write("s11-D");
                self->foo = 0; self->write("foo = 0");
                return self->to(s1);
            }
            return Status::UNHANDLED;
        default: return self->super(s1);
    }
}

static Status s2(Hsm* h, Event const* e) {
    auto* self = static_cast<ExampleStatechart*>(h);
    switch (e->sig) {
        case chart::Entry_Signal: self->write("s2-ENTRY"); return Status::HANDLED;
        case chart::Init_Signal:  self->write("s2-INIT");  return self->to(s211);
        case chart::Exit_Signal:  self->write("s2-EXIT");  return Status::HANDLED;
        case I_Signal:
            self->write("s2-I");
            if (!self->foo) { self->foo = 1; self->write("foo = 1"); }
            return Status::HANDLED;
        case C_Signal: self->write("s2-C"); return self->to(s1);
        case F_Signal: self->write("s2-F"); return self->to(s11);
        default: return self->super(s);
    }
}

static Status s21(Hsm* h, Event const* e) {
    auto* self = static_cast<ExampleStatechart*>(h);
    switch (e->sig) {
        case chart::Entry_Signal: self->write("s21-ENTRY"); return Status::HANDLED;
        case chart::Init_Signal:  self->write("s21-INIT");  return self->to(s211);
        case chart::Exit_Signal:  self->write("s21-EXIT");  return Status::HANDLED;
        case A_Signal: self->write("s21-A"); return self->to(s21);  // self
        case B_Signal: self->write("s21-B"); return self->to(s211);
        case G_Signal: self->write("s21-G"); return self->to(s11);
        default: return self->super(s2);
    }
}

static Status s211(Hsm* h, Event const* e) {
    auto* self = static_cast<ExampleStatechart*>(h);
    switch (e->sig) {
        case chart::Entry_Signal: self->write("s211-ENTRY"); return Status::HANDLED;
        case chart::Exit_Signal:  self->write("s211-EXIT");  return Status::HANDLED;
        case D_Signal: self->write("s211-D"); return self->to(s21);
        case H_Signal: self->write("s211-H"); return self->to(s);
        default: return self->super(s21);
    }
}

// ---- Main ----------------------------------------------------------------
// The input loop runs in a dedicated std::thread (per request); the main
// thread waits on its join.

int main() {
    ExampleStatechart app("app");
    ExampleStatechart::write("foo = 0");
    app.start_at(s2);  // triggers the ENTRY chain and INIT recursion into s211

    std::thread input([&] {
        static const std::unordered_map<char, chart::Signal> map = {
            {'A', A_Signal}, {'B', B_Signal}, {'C', C_Signal},
            {'D', D_Signal}, {'E', E_Signal}, {'F', F_Signal},
            {'G', G_Signal}, {'H', H_Signal}, {'I', I_Signal},
        };
        std::string line;
        while (true) {
            std::cout << "\n:" << std::flush;
            if (!std::getline(std::cin, line)) break;
            if (line.empty()) continue;
            char ch = static_cast<char>(
                std::toupper(static_cast<unsigned char>(line[0])));
            if (ch == 'T') break;
            auto it = map.find(ch);
            if (it == map.end()) {
                std::cout << "\nEvent not defined." << std::endl;
                continue;
            }
            app.post_fifo(Event{it->second, nullptr});
        }
        app.stop();
    });

    input.join();
    std::cout << "\nTerminating" << std::endl;
    return 0;
}
