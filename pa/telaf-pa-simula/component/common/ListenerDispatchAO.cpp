// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "ListenerDispatchAO.hpp"

#include "Log.hpp"
#include "Signals.hpp"

#include <chart/event.hpp>
#include <chart/hsm.hpp>
#include <chart/spy.hpp>

namespace telux::common::simula {

using namespace CommonSignals;

chart::Status
Dispatching_St(chart::Hsm* h, chart::Event const* e);

ListenerDispatchAO&
ListenerDispatchAO::instance()
{
    static ListenerDispatchAO inst;
    return inst;
}

ListenerDispatchAO::ListenerDispatchAO()
    : chart::ActiveObject("ListenerDispatchAO")
{}

ListenerDispatchAO::~ListenerDispatchAO()
{
    stop();
}

void
ListenerDispatchAO::start()
{
    if (running())
        return;
    set_instrument(std::make_unique<chart::SpyInstrument>(name()));
    start_at(Dispatching_St);
}

void
ListenerDispatchAO::enqueue(std::shared_ptr<DispatchTask> task)
{
    post_fifo({ DispatchTask_Signal, std::move(task) });
}

chart::Status
Dispatching_St(chart::Hsm* h, chart::Event const* e)
{
    auto* self = h;
    switch (e->sig)
    {
        case chart::Entry_Signal:
        case chart::Exit_Signal:
            return chart::Status::HANDLED;
        case DispatchTask_Signal:
        {
            auto t = std::static_pointer_cast<DispatchTask>(e->payload);
            if (!t || !t->invoker)
                return chart::Status::HANDLED;
            LOG_DEBUG("[ListenerDispatch] invoking task listeners=%zu", t->listeners.size());
            for (auto& w : t->listeners)
            {
                auto s = w.lock();
                if (!s)
                {
                    LOG_DEBUG("[ListenerDispatch] listener expired, skip");
                    continue;
                }
                try
                {
                    t->invoker(s);
                }
                catch (const std::exception& ex)
                {
                    LOG_ERROR("[ListenerDispatch] invoker threw: %s", ex.what());
                }
                catch (...)
                {
                    LOG_ERROR("[ListenerDispatch] invoker threw unknown exception");
                }
            }
            return chart::Status::HANDLED;
        }
        default:
            return self->super(&chart::Hsm::top);
    }
}

CHART_NAMED_STATE(Dispatching_St, "ListenerDispatchAO::Dispatching");

}  // namespace telux::common::simula
