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

    constexpr chart::Signal EcallOperatingModeEvt_Signal     {Base_ + 19, "EcallOperatingModeEvt_Signal"};

    // SimulaTelServingSystemManager (tel::IServingSystemManager) -- RF band
    constexpr chart::Signal RequestRFBandCapability_Signal   {Base_ + 20, "RequestRFBandCapability_Signal"};
    constexpr chart::Signal SetRFBandPreferences_Signal      {Base_ + 21, "SetRFBandPreferences_Signal"};
    constexpr chart::Signal RequestRFBandPreferences_Signal  {Base_ + 22, "RequestRFBandPreferences_Signal"};

    // SimulaNetworkSelectionManager (INetworkSelectionManager) -- PLMN scan
    constexpr chart::Signal PerformNetworkScan_Signal        {Base_ + 23, "PerformNetworkScan_Signal"};
    constexpr chart::Signal SetPreferredNetworks_Signal      {Base_ + 24, "SetPreferredNetworks_Signal"};
    constexpr chart::Signal RequestPreferredNetworks_Signal  {Base_ + 25, "RequestPreferredNetworks_Signal"};

    // SimulaTelServingSystemManager (tel::IServingSystemManager) -- service
    constexpr chart::Signal SetServiceDomainPreference_Signal     {Base_ + 26, "SetServiceDomainPreference_Signal"};
    constexpr chart::Signal RequestServiceDomainPreference_Signal {Base_ + 27, "RequestServiceDomainPreference_Signal"};

    constexpr chart::Signal OperatorInfoEvt_Signal       {Base_ + 28, "OperatorInfoEvt_Signal"};
    constexpr chart::Signal VoiceServiceStateEvt_Signal  {Base_ + 29, "VoiceServiceStateEvt_Signal"};
    constexpr chart::Signal NetworkTimeEvt_Signal        {Base_ + 30, "NetworkTimeEvt_Signal"};
    constexpr chart::Signal RFBandInfoEvt_Signal         {Base_ + 31, "RFBandInfoEvt_Signal"};
    constexpr chart::Signal NetworkRejectionEvt_Signal   {Base_ + 32, "NetworkRejectionEvt_Signal"};
    constexpr chart::Signal SelectionModeEvt_Signal      {Base_ + 33, "SelectionModeEvt_Signal"};

    constexpr int Tel_Signal_End = Base_ + 34;
}

namespace EcallSignals {
    constexpr int Base_ = TelSignals::Tel_Signal_End;

    constexpr chart::Signal ReadinessEvt_Signal          {Base_ + 0, "Ecall::ReadinessEvt_Signal"};
    constexpr chart::Signal BridgeConnectivityChanged_Signal {Base_ + 1, "Ecall::BridgeConnectivityChanged_Signal"};

    constexpr chart::Signal RunEcallOp_Signal            {Base_ + 2, "Ecall::RunEcallOp_Signal"};

    // Indications SimulaCallManager subscribes to.
    constexpr chart::Signal CallStateEvt_Signal          {Base_ + 3, "Ecall::CallStateEvt_Signal"};
    constexpr chart::Signal MsdStatusEvt_Signal          {Base_ + 4, "Ecall::MsdStatusEvt_Signal"};
    constexpr chart::Signal HlapTimerEventEvt_Signal     {Base_ + 5, "Ecall::HlapTimerEventEvt_Signal"};
    constexpr chart::Signal SetInitCb_Signal             {Base_ + 6, "Ecall::SetInitCb_Signal"};
    constexpr chart::Signal RedialEvt_Signal             {Base_ + 7, "Ecall::RedialEvt_Signal"};

    constexpr int Ecall_Signal_End = Base_ + 8;
}

namespace ImsSignals {
    constexpr int Base_ = EcallSignals::Ecall_Signal_End;

    constexpr chart::Signal ReadinessEvt_Signal              {Base_ + 0, "Ims::ReadinessEvt_Signal"};
    constexpr chart::Signal BridgeConnectivityChanged_Signal {Base_ + 1, "Ims::BridgeConnectivityChanged_Signal"};
    constexpr chart::Signal SetInitCb_Signal                 {Base_ + 2, "Ims::SetInitCb_Signal"};

    // SimulaImsServingSystemManager (IImsServingSystemManager)
    constexpr chart::Signal RequestRegistrationInfo_Signal   {Base_ + 3, "Ims::RequestRegistrationInfo_Signal"};
    constexpr chart::Signal RequestServiceInfo_Signal        {Base_ + 4, "Ims::RequestServiceInfo_Signal"};
    constexpr chart::Signal RequestPdpStatus_Signal          {Base_ + 5, "Ims::RequestPdpStatus_Signal"};

    constexpr chart::Signal RunImsSettingsOp_Signal          {Base_ + 6, "Ims::RunImsSettingsOp_Signal"};

    constexpr chart::Signal RegStatusEvt_Signal               {Base_ + 7,  "Ims::RegStatusEvt_Signal"};
    constexpr chart::Signal ServiceInfoEvt_Signal             {Base_ + 8,  "Ims::ServiceInfoEvt_Signal"};
    constexpr chart::Signal PdpStatusEvt_Signal               {Base_ + 9,  "Ims::PdpStatusEvt_Signal"};
    constexpr chart::Signal ServiceConfigEvt_Signal           {Base_ + 10, "Ims::ServiceConfigEvt_Signal"};
    constexpr chart::Signal SipUserAgentEvt_Signal            {Base_ + 11, "Ims::SipUserAgentEvt_Signal"};

    constexpr int Ims_Signal_End = Base_ + 12;
}

}  // namespace telux::tel::simula

#endif  // TELUX_TEL_SIMULA_SIGNALS_HPP
