// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

// chart/defer.hpp — defer / recall pattern for events that an AO can't yet
// handle in its current state. Drop them in a DeferQueue, recall them later
// (typically when the AO transitions into a state that will accept them).
//
// Usage inside a state handler:
//   case BusyReq_Signal:
//     chart::defer(*self->buffer, *e);  // not ready yet
//     return chart::Status::HANDLED;
//
//   case chart::Entry_Signal:
//     chart::recall(*self->buffer, *self);  // pop one, dispatch reprocesses
//     return chart::Status::HANDLED;

#ifndef CHART_DEFER_HPP
#define CHART_DEFER_HPP

#include "active_object.hpp"
#include "event.hpp"

#include <cstddef>
#include <deque>
#include <mutex>
#include <utility>

namespace chart {

class DeferQueue {
public:
    void push(Event e) {
        std::lock_guard<std::mutex> lk(m_);
        q_.push_back(std::move(e));
    }

    bool pop(Event& out) {
        std::lock_guard<std::mutex> lk(m_);
        if (q_.empty()) return false;
        out = std::move(q_.front());
        q_.pop_front();
        return true;
    }

    bool empty() const {
        std::lock_guard<std::mutex> lk(m_);
        return q_.empty();
    }

    std::size_t size() const {
        std::lock_guard<std::mutex> lk(m_);
        return q_.size();
    }

    void clear() {
        std::lock_guard<std::mutex> lk(m_);
        q_.clear();
    }

private:
    mutable std::mutex m_;
    std::deque<Event> q_;
};

inline void defer(DeferQueue& q, Event e) { q.push(std::move(e)); }

// Pop one deferred event and re-post it on the AO via LIFO so it's the
// next event consumed (classic defer/recall semantics).
inline bool recall(DeferQueue& q, ActiveObject& ao) {
    Event e;
    if (!q.pop(e)) return false;
    ao.post_lifo(std::move(e));
    return true;
}

// Recall everything; events come out in original order (head first).
inline std::size_t recall_all(DeferQueue& q, ActiveObject& ao) {
    std::size_t n = 0;
    Event e;
    while (q.pop(e)) {
        ao.post_fifo(std::move(e));
        ++n;
    }
    return n;
}

}  // namespace chart

#endif  // CHART_DEFER_HPP
