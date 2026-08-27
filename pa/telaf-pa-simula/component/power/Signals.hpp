// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear
//
// Signals.hpp - chart user-signal definitions for the sml-pa power business domain.
//
// Starts at telux::common::simula::Common_Signal_End so power-domain signal
// ids never collide with component/common/Signals.hpp's range even though
// both live in the same process-wide chart::Event signal space.

#ifndef TELUX_POWER_SIMULA_SIGNALS_HPP
#define TELUX_POWER_SIMULA_SIGNALS_HPP

#include "../common/Signals.hpp"

namespace telux::power::simula {

namespace PowerSignals {
    constexpr int Base_ = telux::common::simula::Common_Signal_End;

    // Manager readiness (2-state NotReady <-> Ready shell)
    constexpr chart::Signal ReadinessEvt_Signal              {Base_ + 0,  "ReadinessEvt_Signal"};
    constexpr chart::Signal BridgeConnectivityChanged_Signal {Base_ + 1,  "BridgeConnectivityChanged_Signal"};

    // SimulaTcuActivityManager
    constexpr chart::Signal SetActivityState_Signal          {Base_ + 2,  "SetActivityState_Signal"};
    constexpr chart::Signal GetAllMachineNames_Signal        {Base_ + 3,  "GetAllMachineNames_Signal"};
    constexpr chart::Signal SendActivityStateAck_Signal      {Base_ + 4,  "SendActivityStateAck_Signal"};
    constexpr chart::Signal StateUpdateInd_Signal            {Base_ + 5,  "StateUpdateInd_Signal"};
    constexpr chart::Signal MachineUpdateInd_Signal          {Base_ + 6,  "MachineUpdateInd_Signal"};
    constexpr chart::Signal SlaveAckStatusInd_Signal         {Base_ + 7,  "SlaveAckStatusInd_Signal"};

    // SimulaWakeupManager
    constexpr chart::Signal WakeupInd_Signal                 {Base_ + 8,  "WakeupInd_Signal"};

    constexpr int Power_Signal_End = Base_ + 9;
}

}  // namespace telux::power::simula

#endif  // TELUX_POWER_SIMULA_SIGNALS_HPP
