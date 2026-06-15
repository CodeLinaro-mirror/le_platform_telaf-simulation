// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

// test_time_event.cpp — verifies one-shot, periodic, and disarm semantics.

#include <chart/chart.hpp>

#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>

using chart::Event;
using chart::Hsm;
using chart::Status;

#define ASSERT_TRUE(x)                                                      \
    do {                                                                    \
        if (!(x)) {                                                         \
            std::cerr << "ASSERT_TRUE failed: " #x " @ " << __FILE__ << ":" \
                      << __LINE__ << "\n";                                  \
            std::exit(1);                                                   \
        }                                                                   \
    } while (0)

constexpr chart::Signal Tick_Signal{chart::User_Signal_Begin, "Tick"};

class Counter : public chart::ActiveObject {
public:
    Counter() : ActiveObject("counter"), tick(this, Tick_Signal) {}
    std::atomic<int> hits{0};
    chart::TimeEvent tick;
};

static Status active(Hsm* h, Event const* e) {
    auto* self = static_cast<Counter*>(h);
    switch (e->sig) {
        case chart::Entry_Signal: return Status::HANDLED;
        case chart::Exit_Signal:  return Status::HANDLED;
        case Tick_Signal: ++self->hits; return Status::HANDLED;
        default: return self->super(&Hsm::top);
    }
}

void test_one_shot_fires_once() {
    Counter c;
    c.start_at(active);
    c.tick.arm_one_shot(std::chrono::milliseconds{30});
    std::this_thread::sleep_for(std::chrono::milliseconds{120});
    ASSERT_TRUE(c.hits.load() == 1);
}

void test_periodic_fires_repeatedly() {
    Counter c;
    c.start_at(active);
    c.tick.arm_periodic(std::chrono::milliseconds{20},
                        std::chrono::milliseconds{20});
    std::this_thread::sleep_for(std::chrono::milliseconds{120});
    c.tick.disarm();
    int hits = c.hits.load();
    ASSERT_TRUE(hits >= 3 && hits <= 8);
}

void test_disarm_cancels_pending() {
    Counter c;
    c.start_at(active);
    c.tick.arm_one_shot(std::chrono::milliseconds{50});
    std::this_thread::sleep_for(std::chrono::milliseconds{10});
    c.tick.disarm();
    std::this_thread::sleep_for(std::chrono::milliseconds{80});
    ASSERT_TRUE(c.hits.load() == 0);
}

void test_destructor_disarms() {
    Counter c;
    c.start_at(active);
    {
        chart::TimeEvent local(&c, Tick_Signal);
        local.arm_one_shot(std::chrono::milliseconds{50});
    }  // local dtor → disarm
    std::this_thread::sleep_for(std::chrono::milliseconds{80});
    ASSERT_TRUE(c.hits.load() == 0);
}

int main() {
    test_one_shot_fires_once();
    test_periodic_fires_repeatedly();
    test_disarm_cancels_pending();
    test_destructor_disarms();
    std::cout << "test_time_event: OK" << std::endl;
    return 0;
}
