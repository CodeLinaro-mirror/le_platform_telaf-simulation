// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

// =============================================================================
// ping_pong.cpp — demonstrates chart::Fabric pub/sub.
//
// Two ActiveObjects: Ping and Pong.
//   Ping: publishes Ping_Signal on start; subscribes to Pong_Signal and
//         re-publishes Ping_Signal each time it receives one.
//   Pong: subscribes to Ping_Signal; publishes Pong_Signal on each receipt.
// The main thread sleeps 200ms before stopping; several PING/PONG rounds
// should be observed during that window.
//
// Build:
//   g++ -std=c++17 -I chart-cpp/include chart-cpp/examples/ping_pong.cpp -lpthread -o /tmp/ping_pong
// =============================================================================

#include <chart/chart.hpp>

#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>

using chart::Event;
using chart::Hsm;
using chart::Status;

constexpr chart::Signal Ping_Signal{chart::User_Signal_Begin, "Ping"};
constexpr chart::Signal Pong_Signal{chart::User_Signal_Begin + 1, "Pong"};

static std::atomic<int> ping_count{0};
static std::atomic<int> pong_count{0};

class Ping : public chart::ActiveObject {
public:
    using ActiveObject::ActiveObject;
};

class Pong : public chart::ActiveObject {
public:
    using ActiveObject::ActiveObject;
};

static Status ping_active(Hsm* h, Event const* e) {
    auto* self = static_cast<Ping*>(h);
    switch (e->sig) {
        case chart::Entry_Signal:
            chart::Fabric::instance().subscribe(self, Pong_Signal);
            chart::Fabric::instance().publish(Event{Ping_Signal, nullptr});
            ++ping_count;
            return Status::HANDLED;
        case Pong_Signal:
            ++pong_count;
            chart::Fabric::instance().publish(Event{Ping_Signal, nullptr});
            ++ping_count;
            return Status::HANDLED;
        case chart::Exit_Signal:
            chart::Fabric::instance().unsubscribe_all(self);
            return Status::HANDLED;
        default: return self->super(&Hsm::top);
    }
}

static Status pong_active(Hsm* h, Event const* e) {
    auto* self = static_cast<Pong*>(h);
    switch (e->sig) {
        case chart::Entry_Signal:
            chart::Fabric::instance().subscribe(self, Ping_Signal);
            return Status::HANDLED;
        case Ping_Signal:
            chart::Fabric::instance().publish(Event{Pong_Signal, nullptr});
            return Status::HANDLED;
        case chart::Exit_Signal:
            chart::Fabric::instance().unsubscribe_all(self);
            return Status::HANDLED;
        default: return self->super(&Hsm::top);
    }
}

int main() {
    Pong pong("pong");
    Ping ping("ping");

    pong.start_at(pong_active);
    ping.start_at(ping_active);

    std::this_thread::sleep_for(std::chrono::milliseconds{200});

    ping.stop();
    pong.stop();

    std::cout << "ping count: " << ping_count.load()
              << ", pong count: " << pong_count.load() << std::endl;
    return 0;
}
