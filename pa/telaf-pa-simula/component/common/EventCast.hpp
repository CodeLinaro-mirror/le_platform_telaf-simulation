// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef TELUX_COMMON_SIMULA_EVENT_CAST_HPP
#define TELUX_COMMON_SIMULA_EVENT_CAST_HPP

#include <chart/event.hpp>
#include <memory>

namespace telux::common::simula {

template<typename T>
std::shared_ptr<T>
event_cast(const chart::Event& e)
{
    return std::static_pointer_cast<T>(e.payload);
}

}  // namespace telux::common::simula

#endif  // TELUX_COMMON_SIMULA_EVENT_CAST_HPP
