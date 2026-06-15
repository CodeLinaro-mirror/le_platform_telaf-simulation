// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

// =============================================================================
// blinky.cpp — demonstrates chart::TimeEvent.
//
// One ActiveObject with two states OFF / ON; Timeout_Signal toggles between
// them. The TimeEvent is armed on ENTRY and disarmed on EXIT, firing once
// per second.
//
// Build:
//   g++ -std=c++17 -I chart-cpp/include chart-cpp/examples/blinky.cpp -lpthread -o /tmp/blinky
//
// Run: stops after about 5 seconds.
// =============================================================================

#include <chart/chart.hpp>

#include <chrono>
#include <iostream>
#include <thread>

using chart::Event;
using chart::Hsm;
using chart::Status;

constexpr chart::Signal Timeout_Signal{chart::User_Signal_Begin, "Timeout"};

class Blinky : public chart::ActiveObject {
public:
    Blinky() : ActiveObject("blinky"), tick_(this, Timeout_Signal) {}
    chart::TimeEvent tick_;
};

static Status off_state(Hsm*, Event const*);
static Status on_state(Hsm*, Event const*);

static Status off_state(Hsm* h, Event const* e) {
    auto* self = static_cast<Blinky*>(h);
    switch (e->sig) {
        case chart::Entry_Signal:
            std::cout << "[" << self->name() << "] OFF" << std::endl;
            self->tick_.arm_one_shot(std::chrono::milliseconds{1000});
            return Status::HANDLED;
        case chart::Exit_Signal:
            self->tick_.disarm();
            return Status::HANDLED;
        case Timeout_Signal:
            return self->to(on_state);
        default: return self->super(&Hsm::top);
    }
}

static Status on_state(Hsm* h, Event const* e) {
    auto* self = static_cast<Blinky*>(h);
    switch (e->sig) {
        case chart::Entry_Signal:
            std::cout << "[" << self->name() << "] ON" << std::endl;
            self->tick_.arm_one_shot(std::chrono::milliseconds{1000});
            return Status::HANDLED;
        case chart::Exit_Signal:
            self->tick_.disarm();
            return Status::HANDLED;
        case Timeout_Signal:
            return self->to(off_state);
        default: return self->super(&Hsm::top);
    }
}

int main() {
    Blinky b;
    b.start_at(off_state);
    std::this_thread::sleep_for(std::chrono::seconds{5});
    b.stop();
    return 0;
}
