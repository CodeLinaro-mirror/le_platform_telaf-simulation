// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

// test_pub_sub.cpp — verifies Fabric subscribe/publish/unsubscribe semantics.

#include <chart/chart.hpp>

#include <atomic>
#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

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

constexpr chart::Signal Topic_Signal{chart::User_Signal_Begin, "Topic"};

class Sub : public chart::ActiveObject {
public:
    using ActiveObject::ActiveObject;
    std::atomic<int> hits{0};
};

static Status active(Hsm* h, Event const* e) {
    auto* self = static_cast<Sub*>(h);
    switch (e->sig) {
        case chart::Entry_Signal:
            chart::Fabric::instance().subscribe(self, Topic_Signal);
            return Status::HANDLED;
        case chart::Exit_Signal:
            chart::Fabric::instance().unsubscribe_all(self);
            return Status::HANDLED;
        case Topic_Signal:
            ++self->hits;
            return Status::HANDLED;
        default: return self->super(&Hsm::top);
    }
}

void wait_for(std::vector<Sub*> const& subs, int target) {
    for (int i = 0; i < 200; ++i) {
        bool all = true;
        for (auto* s : subs) if (s->hits.load() < target) { all = false; break; }
        if (all) return;
        std::this_thread::sleep_for(std::chrono::milliseconds{2});
    }
}

void test_fanout_to_all_subscribers() {
    std::vector<std::unique_ptr<Sub>> subs;
    std::vector<Sub*> raw;
    for (int i = 0; i < 5; ++i) {
        subs.emplace_back(std::make_unique<Sub>("s" + std::to_string(i)));
        subs.back()->start_at(active);
        raw.push_back(subs.back().get());
    }
    std::this_thread::sleep_for(std::chrono::milliseconds{20});
    for (int i = 0; i < 10; ++i) {
        chart::Fabric::instance().publish(Event{Topic_Signal, nullptr});
    }
    wait_for(raw, 10);
    for (auto* s : raw) ASSERT_TRUE(s->hits.load() == 10);
}

void test_unsubscribe_stops_delivery() {
    Sub a("a"), b("b");
    a.start_at(active);
    b.start_at(active);
    std::this_thread::sleep_for(std::chrono::milliseconds{10});

    chart::Fabric::instance().publish(Event{Topic_Signal, nullptr});
    wait_for({&a, &b}, 1);
    ASSERT_TRUE(a.hits.load() == 1 && b.hits.load() == 1);

    chart::Fabric::instance().unsubscribe(&b, Topic_Signal);
    chart::Fabric::instance().publish(Event{Topic_Signal, nullptr});
    std::this_thread::sleep_for(std::chrono::milliseconds{30});
    ASSERT_TRUE(a.hits.load() == 2);
    ASSERT_TRUE(b.hits.load() == 1);
}

int main() {
    test_fanout_to_all_subscribers();
    test_unsubscribe_stops_delivery();
    std::cout << "test_pub_sub: OK" << std::endl;
    return 0;
}
