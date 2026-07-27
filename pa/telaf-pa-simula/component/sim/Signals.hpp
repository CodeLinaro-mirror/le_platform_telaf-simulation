// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear
// SIM-domain chart signals; names support chart::SpyInstrument output.

#ifndef TELUX_TEL_SIMULA_SIGNALS_HPP
#define TELUX_TEL_SIMULA_SIGNALS_HPP

#include "../common/Signals.hpp"

namespace telux::tel::simula {

namespace SimSignals {
    constexpr int Base_ = telux::common::simula::Common_Signal_End;
    // Shared manager signals.
    constexpr chart::Signal ReadinessEvt_Signal              {Base_ + 0,  "ReadinessEvt_Signal"};
    constexpr chart::Signal BridgeConnectivityChanged_Signal {Base_ + 1,  "BridgeConnectivityChanged_Signal"};
    // SimulaCardManager
    constexpr chart::Signal GetState_Signal                  {Base_ + 2,  "GetState_Signal"};
    constexpr chart::Signal CardPowerUp_Signal               {Base_ + 3,  "CardPowerUp_Signal"};
    constexpr chart::Signal CardPowerDown_Signal             {Base_ + 4,  "CardPowerDown_Signal"};
    constexpr chart::Signal CardStateEvt_Signal              {Base_ + 5,  "CardStateEvt_Signal"};
    constexpr chart::Signal RegisterCardListener_Signal      {Base_ + 6,  "RegisterCardListener_Signal"};
    constexpr chart::Signal RemoveCardListener_Signal        {Base_ + 7,  "RemoveCardListener_Signal"};
    // SimulaSubscriptionManager
    constexpr chart::Signal GetIccid_Signal                  {Base_ + 8,  "GetIccid_Signal"};
    constexpr chart::Signal GetImsi_Signal                   {Base_ + 9,  "GetImsi_Signal"};
    constexpr chart::Signal SubInfoChangedEvt_Signal         {Base_ + 10, "SubInfoChangedEvt_Signal"};
    constexpr chart::Signal RegisterSubListener_Signal       {Base_ + 11, "RegisterSubListener_Signal"};
    constexpr chart::Signal RemoveSubListener_Signal         {Base_ + 12, "RemoveSubListener_Signal"};
    // SimulaMultiSimManager
    constexpr chart::Signal RequestSlotStatus_Signal         {Base_ + 13, "RequestSlotStatus_Signal"};
    constexpr chart::Signal RequestHighCapability_Signal     {Base_ + 14, "RequestHighCapability_Signal"};
    constexpr chart::Signal SetHighCapability_Signal         {Base_ + 15, "SetHighCapability_Signal"};
    constexpr chart::Signal SwitchActiveSlot_Signal          {Base_ + 16, "SwitchActiveSlot_Signal"};

    constexpr int Sim_Signal_End = Base_ + 17;
}
}  // namespace telux::tel::simula

#endif  // TELUX_TEL_SIMULA_SIGNALS_HPP

