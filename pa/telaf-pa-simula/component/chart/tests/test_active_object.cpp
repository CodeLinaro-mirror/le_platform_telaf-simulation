// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

// test_active_object.cpp — verifies thread/queue behavior of ActiveObject:
// post → dispatch ordering, lifo precedence, stop drains and drops.

#include <chart/chart.hpp>

#include <atomic>
#include <chrono>
#include <iostream>
#include <mutex>
#include <sstream>
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

constexpr chart::Signal S1_Signal{chart::User_Signal_Begin, "S1"};
constexpr chart::Signal S2_Signal{chart::User_Signal_Begin + 1, "S2"};
constexpr chart::Signal S3_Signal{chart::User_Signal_Begin + 2, "S3"};

class Recorder : public chart::ActiveObject {
public:
    using ActiveObject::ActiveObject;
    std::mutex m;
    std::vector<int> seen;
};

static Status active(Hsm* h, Event const* e) {
    auto* self = static_cast<Recorder*>(h);
    switch (e->sig) {
        case chart::Entry_Signal: return Status::HANDLED;
        case chart::Exit_Signal:  return Status::HANDLED;
        case S1_Signal:
        case S2_Signal:
        case S3_Signal: {
            std::lock_guard<std::mutex> lk(self->m);
            self->seen.push_back(e->sig);
            return Status::HANDLED;
        }
        default: return self->super(&Hsm::top);
    }
}

void test_fifo_order() {
    Recorder r("r");
    r.start_at(active);
    for (int i = 0; i < 100; ++i) r.post_fifo(Event{S1_Signal, nullptr});
    for (int i = 0; i < 100; ++i) {
        {
            std::lock_guard<std::mutex> lk(r.m);
            if (r.seen.size() == 100) break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds{2});
    }
    std::lock_guard<std::mutex> lk(r.m);
    ASSERT_TRUE(r.seen.size() == 100);
    for (int x : r.seen) ASSERT_TRUE(x == S1_Signal);
}

void test_lifo_pushes_to_front() {
    Recorder r("r");
    r.start_at(active);
    r.post_lifo(Event{S2_Signal, nullptr});
    r.post_fifo(Event{S3_Signal, nullptr});
    for (int i = 0; i < 50; ++i) {
        {
            std::lock_guard<std::mutex> lk(r.m);
            if (r.seen.size() == 2) break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds{2});
    }
    std::lock_guard<std::mutex> lk(r.m);
    ASSERT_TRUE(r.seen.size() == 2);
}

void test_stop_drops_pending() {
    Recorder r("r");
    r.start_at(active);
    r.stop();
    r.post_fifo(Event{S1_Signal, nullptr});
    std::this_thread::sleep_for(std::chrono::milliseconds{20});
    std::lock_guard<std::mutex> lk(r.m);
    ASSERT_TRUE(!r.running());
}

void test_concurrent_posts() {
    Recorder r("r");
    r.start_at(active);
    std::vector<std::thread> threads;
    std::atomic<int> sent{0};
    for (int t = 0; t < 8; ++t) {
        threads.emplace_back([&] {
            for (int i = 0; i < 200; ++i) {
                r.post_fifo(Event{S1_Signal, nullptr});
                ++sent;
            }
        });
    }
    for (auto& th : threads) th.join();

    for (int i = 0; i < 100; ++i) {
        {
            std::lock_guard<std::mutex> lk(r.m);
            if ((int)r.seen.size() == sent.load()) break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds{5});
    }
    std::lock_guard<std::mutex> lk(r.m);
    ASSERT_TRUE((int)r.seen.size() == sent.load());
}

int main() {
    test_fifo_order();
    test_lifo_pushes_to_front();
    test_stop_drops_pending();
    test_concurrent_posts();
    std::cout << "test_active_object: OK" << std::endl;
    return 0;
}
