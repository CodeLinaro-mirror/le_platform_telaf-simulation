// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

// =============================================================================
// dining_philosophers.cpp — dining-philosophers problem, demonstrating
// multi-AO collaboration in chart.
//
// N = 5 philosophers (one ActiveObject each) + 1 Table arbiter AO, totaling
// 6 threads. The Table centralizes ownership of the 5 forks, avoiding the
// deadlock that direct fork grabbing would cause.
//
// Communication runs over Fabric pub/sub:
//   Philosopher → Table:  Hungry_Signal / Done_Signal  (payload carries philosopher id)
//   Table → Philosopher:  Eat_Signal                   (only the matching id reacts)
//
// State machines:
//   Philosopher: phil_top { thinking → hungry → eating → thinking }
//   Table:       table_serving (single state, handles all requests)
//
// Stops after 10 seconds; Spy records every state/event and dumps on exit.
// Trace records every HSM transition automatically (via ActiveObject::start_at)
// and dumps on exit.
//
// Build:
//   g++ -std=c++17 -I chart-cpp/include chart-cpp/examples/dining_philosophers.cpp -lpthread -o /tmp/dining
// =============================================================================

#include <chart/chart.hpp>

#include <array>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

using chart::ActiveObject;
using chart::Event;
using chart::Fabric;
using chart::Hsm;
using chart::Spy;
using chart::StateNames;
using chart::Status;
using chart::TimeEvent;

constexpr int N = 5;
// phil[i]'s left fork = forks[i], right fork = forks[(i+1)%N];
// adjacent philosophers therefore always share one fork
// (phil[i] and phil[i+1] share forks[i+1]).
static constexpr int LFORK(int i) { return i; }
static constexpr int RFORK(int i) { return (i + 1) % N; }
static constexpr int LPHIL(int i) { return (i + N - 1) % N; }
static constexpr int RPHIL(int i) { return (i + 1) % N; }

constexpr chart::Signal Hungry_Signal      {chart::User_Signal_Begin,     "Hungry"};
constexpr chart::Signal Done_Signal        {chart::User_Signal_Begin + 1, "Done"};
constexpr chart::Signal Eat_Signal         {chart::User_Signal_Begin + 2, "Eat"};
constexpr chart::Signal ThinkTimeout_Signal{chart::User_Signal_Begin + 3, "ThinkTimeout"};
constexpr chart::Signal EatTimeout_Signal  {chart::User_Signal_Begin + 4, "EatTimeout"};

// Cross-AO signal payload: philosopher id
struct PhilId { int id; };

// =============================================================================
// Philosopher
// =============================================================================

class Philosopher : public ActiveObject {
public:
    Philosopher(int id, std::string name)
        : ActiveObject(std::move(name)),
          id_(id),
          think_timer_(this, ThinkTimeout_Signal),
          eat_timer_(this, EatTimeout_Signal) {}

    int id() const { return id_; }
    TimeEvent& think_timer() { return think_timer_; }
    TimeEvent& eat_timer()   { return eat_timer_; }

private:
    int id_;
    TimeEvent think_timer_;
    TimeEvent eat_timer_;
};

static Status phil_top     (Hsm*, Event const*);
static Status phil_thinking(Hsm*, Event const*);
static Status phil_hungry  (Hsm*, Event const*);
static Status phil_eating  (Hsm*, Event const*);

// Parent state: owns lifetime-scoped subscription; INIT goes straight to thinking
static Status phil_top(Hsm* h, Event const* e) {
    auto* self = static_cast<Philosopher*>(h);
    switch (e->sig) {
        case chart::Entry_Signal:
            Fabric::instance().subscribe(self, Eat_Signal);
            Spy::record(self->name() + ": ENTRY top (subscribe Eat_Signal)");
            return Status::HANDLED;
        case chart::Exit_Signal:
            Fabric::instance().unsubscribe_all(self);
            Spy::record(self->name() + ": EXIT top");
            return Status::HANDLED;
        case chart::Init_Signal:
            return self->to(phil_thinking);
        default:
            return self->super(&Hsm::top);
    }
}

static Status phil_thinking(Hsm* h, Event const* e) {
    auto* self = static_cast<Philosopher*>(h);
    switch (e->sig) {
        case chart::Entry_Signal: {
            // Stagger think duration across philosophers so they don't all
            // get hungry at the same moment.
            auto delay = std::chrono::milliseconds{300 + 100 * self->id()};
            self->think_timer().arm_one_shot(delay);
            Spy::record(self->name() + ": thinking");
            return Status::HANDLED;
        }
        case ThinkTimeout_Signal:
            return self->to(phil_hungry);
        default:
            return self->super(phil_top);
    }
}

static Status phil_hungry(Hsm* h, Event const* e) {
    auto* self = static_cast<Philosopher*>(h);
    switch (e->sig) {
        case chart::Entry_Signal:
            Spy::record(self->name() + ": hungry → publish Hungry_Signal");
            Fabric::instance().publish(
                Event{Hungry_Signal, std::make_shared<PhilId>(PhilId{self->id()})});
            return Status::HANDLED;
        case Eat_Signal: {
            auto p = std::static_pointer_cast<PhilId>(e->payload);
            if (p && p->id == self->id()) return self->to(phil_eating);
            return Status::HANDLED;  // not for me, drop it
        }
        default:
            return self->super(phil_top);
    }
}

static Status phil_eating(Hsm* h, Event const* e) {
    auto* self = static_cast<Philosopher*>(h);
    switch (e->sig) {
        case chart::Entry_Signal:
            Spy::record(self->name() + ": eating");
            self->eat_timer().arm_one_shot(std::chrono::milliseconds{200});
            return Status::HANDLED;
        case chart::Exit_Signal:
            // Return forks immediately on leaving eating (Done_Signal is
            // interpreted by the Table as: both adjacent forks freed).
            Spy::record(self->name() + ": done → publish Done_Signal");
            Fabric::instance().publish(
                Event{Done_Signal, std::make_shared<PhilId>(PhilId{self->id()})});
            return Status::HANDLED;
        case EatTimeout_Signal:
            return self->to(phil_thinking);
        default:
            return self->super(phil_top);
    }
}

// =============================================================================
// Table — arbiter: the sole owner of fork state
// =============================================================================

class Table : public ActiveObject {
public:
    using ActiveObject::ActiveObject;

    enum class Fork : std::uint8_t { FREE, USED };

    // forks[i] is phil[i]'s left fork and phil[(i-1+N)%N]'s right fork
    std::array<Fork, N> forks{};
    std::array<bool, N> hungry{};

    // Called only from table_serving on the Table's own worker thread, no
    // locking needed.
    void try_feed(int i) {
        if (!hungry[i]) return;
        if (forks[LFORK(i)] != Fork::FREE) return;
        if (forks[RFORK(i)] != Fork::FREE) return;
        forks[LFORK(i)] = Fork::USED;
        forks[RFORK(i)] = Fork::USED;
        hungry[i] = false;
        Spy::record("Table: grant Eat_Signal → phil[" + std::to_string(i) + "]");
        Fabric::instance().publish(
            Event{Eat_Signal, std::make_shared<PhilId>(PhilId{i})});
    }
};

static Status table_serving(Hsm* h, Event const* e) {
    auto* self = static_cast<Table*>(h);
    switch (e->sig) {
        case chart::Entry_Signal:
            for (int i = 0; i < N; ++i) {
                self->forks[i]  = Table::Fork::FREE;
                self->hungry[i] = false;
            }
            Fabric::instance().subscribe(self, Hungry_Signal);
            Fabric::instance().subscribe(self, Done_Signal);
            Spy::record("Table: serving");
            return Status::HANDLED;
        case chart::Exit_Signal:
            Fabric::instance().unsubscribe_all(self);
            return Status::HANDLED;
        case Hungry_Signal: {
            auto p = std::static_pointer_cast<PhilId>(e->payload);
            Spy::record("Table: phil[" + std::to_string(p->id) + "] is hungry");
            self->hungry[p->id] = true;
            self->try_feed(p->id);
            return Status::HANDLED;
        }
        case Done_Signal: {
            auto p = std::static_pointer_cast<PhilId>(e->payload);
            int i = p->id;
            Spy::record("Table: phil[" + std::to_string(i) + "] returned forks");
            self->forks[LFORK(i)] = Table::Fork::FREE;
            self->forks[RFORK(i)] = Table::Fork::FREE;
            // Both neighbors are likely blocked waiting on one of these forks.
            self->try_feed(LPHIL(i));
            self->try_feed(RPHIL(i));
            return Status::HANDLED;
        }
        default:
            return self->super(&Hsm::top);
    }
}

// =============================================================================
// main
// =============================================================================

int main() {
    // State names — make spy output readable
    StateNames::reg(phil_top,      "phil_top");
    StateNames::reg(phil_thinking, "phil_thinking");
    StateNames::reg(phil_hungry,   "phil_hungry");
    StateNames::reg(phil_eating,   "phil_eating");
    StateNames::reg(table_serving, "table_serving");

    Spy::enable(true);
    Spy::live_sink(&std::cerr);        // stream spy lines to stderr in real time
    Spy::live_trace_sink(&std::cerr);  // stream trace lines to stderr in real time
    Spy::max_size(8192);
    Spy::max_trace_size(8192);

    Table table("table");
    std::array<std::unique_ptr<Philosopher>, N> phils;
    for (int i = 0; i < N; ++i) {
        phils[i] = std::make_unique<Philosopher>(
            i, "phil_" + std::to_string(i));
    }

    // Start the Table first: ensures it has subscribed to Hungry/Done before
    // the philosophers begin publishing.
    table.start_at(table_serving);
    for (auto& p : phils) p->start_at(phil_top);

    std::this_thread::sleep_for(std::chrono::seconds{10});

    // Stop philosophers first: avoids the Table publishing Eat_Signal to
    // an already-stopped AO.
    for (auto& p : phils) p->stop();
    table.stop();

    std::cout << "\n=== Spy trace (" << Spy::size() << " entries) ===\n";
    Spy::dump(std::cout);
    std::cout << "\n=== HSM trace (" << Spy::trace_size() << " transitions) ===\n";
    Spy::dump_trace(std::cout);
    return 0;
}
