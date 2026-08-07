// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear
//
// Signals.hpp - chart user-signal definitions for the sml-pa tel (radio)
// business domain.


#ifndef TELUX_TEL_SIMULA_SIGNALS_HPP
#define TELUX_TEL_SIMULA_SIGNALS_HPP

#include "../data/Signals.hpp"

namespace telux::tel::simula {

namespace TelSignals {
    constexpr int Base_ = telux::data::simula::DataSignals::Data_Signal_End;

    // Manager readiness (2-state NotReady <-> Ready shell shared by
    // SimulaPhoneManager / SimulaTelServingSystemManager / SimulaNetworkSelectionManager)
    constexpr chart::Signal ReadinessEvt_Signal              {Base_ + 0,  "Tel::ReadinessEvt_Signal"};
    constexpr chart::Signal BridgeConnectivityChanged_Signal {Base_ + 1,  "Tel::BridgeConnectivityChanged_Signal"};
  
    constexpr chart::Signal SetInitCb_Signal                 {Base_ + 16, "Tel::SetInitCb_Signal"};

    // SimulaPhoneManager (IPhoneManager) 
    constexpr chart::Signal RequestOperatingMode_Signal      {Base_ + 2,  "RequestOperatingMode_Signal"};
    constexpr chart::Signal SetOperatingMode_Signal          {Base_ + 3,  "SetOperatingMode_Signal"};
    constexpr chart::Signal RequestCellularCapability_Signal {Base_ + 4,  "RequestCellularCapability_Signal"};
    constexpr chart::Signal OpModeEvt_Signal                 {Base_ + 5,  "OpModeEvt_Signal"};
    constexpr chart::Signal SignalStrengthEvt_Signal         {Base_ + 6,  "SignalStrengthEvt_Signal"};
    constexpr chart::Signal CellInfoEvt_Signal               {Base_ + 7,  "CellInfoEvt_Signal"};

    // SimulaTelServingSystemManager (tel::IServingSystemManager)
    constexpr chart::Signal SetRatPreference_Signal          {Base_ + 8,  "SetRatPreference_Signal"};
    constexpr chart::Signal RequestRatPreference_Signal      {Base_ + 9,  "RequestRatPreference_Signal"};
    constexpr chart::Signal SysInfoEvt_Signal                {Base_ + 10, "SysInfoEvt_Signal"};
    constexpr chart::Signal RatPrefEvt_Signal                {Base_ + 11, "RatPrefEvt_Signal"};
    constexpr chart::Signal DcStatusEvt_Signal               {Base_ + 12, "DcStatusEvt_Signal"};
    constexpr chart::Signal LteCsCapabilityEvt_Signal        {Base_ + 17, "LteCsCapabilityEvt_Signal"};
    constexpr chart::Signal RequestRFBandInfo_Signal         {Base_ + 18, "RequestRFBandInfo_Signal"};

    // SimulaNetworkSelectionManager (INetworkSelectionManager)
    constexpr chart::Signal SetNetworkSelectionMode_Signal        {Base_ + 13, "SetNetworkSelectionMode_Signal"};
    constexpr chart::Signal RequestNetworkSelectionModeInfo_Signal {Base_ + 14, "RequestNetworkSelectionModeInfo_Signal"};
    constexpr chart::Signal RequestNetworkSelectionMode_Signal    {Base_ + 15, "RequestNetworkSelectionMode_Signal"};

    constexpr int Tel_Signal_End = Base_ + 19;
}

}  // namespace telux::tel::simula

#endif  // TELUX_TEL_SIMULA_SIGNALS_HPP
