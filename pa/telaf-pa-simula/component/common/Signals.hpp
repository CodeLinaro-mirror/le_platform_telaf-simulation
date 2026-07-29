// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear
//
// Signals.hpp - chart user-signal definitions for component/common AOs
// (ModemBridge, ListenerDispatchAO).
//
// Starts at chart::User_Signal_Begin. `Common_Signal_End` is the id one
// past the last value declared here; domain-layer Signals.hpp files
// (e.g. component/data/Signals.hpp) start their own constants at this value
// so common and domain signal ids never collide even though both live in
// the same process-wide chart::Event signal space.

#ifndef TELUX_COMMON_SIMULA_SIGNALS_HPP
#define TELUX_COMMON_SIMULA_SIGNALS_HPP

#include <chart/event.hpp>

namespace telux::common::simula {

namespace CommonSignals {
    // ModemBridge lifecycle
    constexpr chart::Signal Start_Signal                 {chart::User_Signal_Begin + 0,  "Start_Signal"};
    constexpr chart::Signal Stop_Signal                  {chart::User_Signal_Begin + 1,  "Stop_Signal"};
    constexpr chart::Signal LinkUp_Signal                {chart::User_Signal_Begin + 2,  "LinkUp_Signal"};
    constexpr chart::Signal LinkDown_Signal              {chart::User_Signal_Begin + 3,  "LinkDown_Signal"};
    constexpr chart::Signal ConnectTimeout_Signal        {chart::User_Signal_Begin + 4,  "ConnectTimeout_Signal"};
    constexpr chart::Signal Backoff_Signal               {chart::User_Signal_Begin + 5,  "Backoff_Signal"};

    // ModemBridge: Connected/Subscribing sub-machine
    constexpr chart::Signal AllSubAcked_Signal           {chart::User_Signal_Begin + 6,  "AllSubAcked_Signal"};

    // ModemBridge RPC plumbing
    constexpr chart::Signal SendReq_Signal               {chart::User_Signal_Begin + 7,  "SendReq_Signal"};
    constexpr chart::Signal RspTimeout_Signal            {chart::User_Signal_Begin + 8,  "RspTimeout_Signal"};
    constexpr chart::Signal EvtRecv_Signal               {chart::User_Signal_Begin + 9,  "EvtRecv_Signal"};
    constexpr chart::Signal SubscribeEvent_Signal        {chart::User_Signal_Begin + 10, "SubscribeEvent_Signal"};
    constexpr chart::Signal UnsubscribeEvent_Signal      {chart::User_Signal_Begin + 11, "UnsubscribeEvent_Signal"};
    constexpr chart::Signal SubscribeConnectivity_Signal {chart::User_Signal_Begin + 12, "SubscribeConnectivity_Signal"};
    constexpr chart::Signal UnsubscribeConnectivity_Signal {chart::User_Signal_Begin + 13, "UnsubscribeConnectivity_Signal"};
    // Synchronisation fence: handled identically in every state (including
    // ShuttingDown) -- its only job is to be *reached*, at which point the
    // poster's drain() unblocks. See IModemBridge::drain().
    constexpr chart::Signal Drain_Signal                 {chart::User_Signal_Begin + 14, "Drain_Signal"};

    // ListenerDispatchAO
    constexpr chart::Signal DispatchTask_Signal          {chart::User_Signal_Begin + 15, "DispatchTask_Signal"};
}

constexpr int Common_Signal_End = chart::User_Signal_Begin + 16;

}  // namespace telux::common::simula

#endif  // TELUX_COMMON_SIMULA_SIGNALS_HPP
