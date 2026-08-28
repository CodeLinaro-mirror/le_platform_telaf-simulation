// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear
//
// Signals.hpp - chart user-signal definitions for the sml-pa net business
// domain (telux::data::net managers backing telaf/interfaces/taf_net.api).
//
// Starts at telux::data::simula::DataSignals::Data_Signal_End so net-domain
// signal ids never collide with the data domain's range -- both live in the
// same process-wide chart::Event signal space, and both are linked into
// libtelux_data.so (see component/net/README rationale in DataFactory.cpp).

#ifndef TELUX_DATA_NET_SIMULA_SIGNALS_HPP
#define TELUX_DATA_NET_SIMULA_SIGNALS_HPP

#include "../data/Signals.hpp"

namespace telux::data::net::simula {

namespace NetSignals {
    constexpr int Base_ = telux::data::simula::DataSignals::Data_Signal_End;

    // Manager readiness (same 2-state NotReady <-> Ready shell the data
    // domain's managers use).
    constexpr chart::Signal ReadinessEvt_Signal              {Base_ + 0, "Net::ReadinessEvt_Signal"};
    constexpr chart::Signal BridgeConnectivityChanged_Signal  {Base_ + 1, "Net::BridgeConnectivityChanged_Signal"};

    // VlanManager
    constexpr chart::Signal CreateVlan_Signal                {Base_ + 2, "CreateVlan_Signal"};
    constexpr chart::Signal RemoveVlan_Signal                {Base_ + 3, "RemoveVlan_Signal"};
    constexpr chart::Signal QueryVlanInfo_Signal             {Base_ + 4, "QueryVlanInfo_Signal"};
    constexpr chart::Signal BindVlan_Signal                  {Base_ + 5, "BindVlan_Signal"};
    constexpr chart::Signal UnbindVlan_Signal                {Base_ + 6, "UnbindVlan_Signal"};
    constexpr chart::Signal QueryVlanBindings_Signal         {Base_ + 7, "QueryVlanBindings_Signal"};
    constexpr chart::Signal VlanHwAccelEvt_Signal            {Base_ + 8, "VlanHwAccelEvt_Signal"};

    // L2tpManager
    constexpr chart::Signal SetL2tpConfig_Signal             {Base_ + 9,  "SetL2tpConfig_Signal"};
    constexpr chart::Signal RequestL2tpConfig_Signal         {Base_ + 10, "RequestL2tpConfig_Signal"};
    constexpr chart::Signal AddTunnel_Signal                 {Base_ + 11, "AddTunnel_Signal"};
    constexpr chart::Signal RemoveTunnel_Signal              {Base_ + 12, "RemoveTunnel_Signal"};
    constexpr chart::Signal AddSession_Signal                {Base_ + 13, "AddSession_Signal"};
    constexpr chart::Signal RemoveSession_Signal             {Base_ + 14, "RemoveSession_Signal"};
    constexpr chart::Signal BindSession_Signal               {Base_ + 15, "BindSession_Signal"};
    constexpr chart::Signal UnbindSession_Signal             {Base_ + 16, "UnbindSession_Signal"};
    constexpr chart::Signal QuerySessionBindings_Signal      {Base_ + 17, "QuerySessionBindings_Signal"};

    // NatManager
    constexpr chart::Signal AddNatEntry_Signal               {Base_ + 18, "AddNatEntry_Signal"};
    constexpr chart::Signal RemoveNatEntry_Signal            {Base_ + 19, "RemoveNatEntry_Signal"};
    constexpr chart::Signal RequestNatEntries_Signal         {Base_ + 20, "RequestNatEntries_Signal"};

    // SocksManager
    constexpr chart::Signal EnableSocks_Signal               {Base_ + 21, "EnableSocks_Signal"};

    // DataSettingsManager
    constexpr chart::Signal RestoreFactorySettings_Signal    {Base_ + 22, "RestoreFactorySettings_Signal"};
    constexpr chart::Signal SetBackhaulPref_Signal           {Base_ + 23, "SetBackhaulPref_Signal"};
    constexpr chart::Signal RequestBackhaulPref_Signal       {Base_ + 24, "RequestBackhaulPref_Signal"};
    constexpr chart::Signal SetBandInterference_Signal       {Base_ + 25, "SetBandInterference_Signal"};
    constexpr chart::Signal RequestBandInterference_Signal   {Base_ + 26, "RequestBandInterference_Signal"};
    constexpr chart::Signal SetWwanConnectivity_Signal       {Base_ + 27, "SetWwanConnectivity_Signal"};
    constexpr chart::Signal RequestWwanConnectivity_Signal   {Base_ + 28, "RequestWwanConnectivity_Signal"};
    constexpr chart::Signal SetMacSecState_Signal            {Base_ + 29, "SetMacSecState_Signal"};
    constexpr chart::Signal RequestMacSecState_Signal        {Base_ + 30, "RequestMacSecState_Signal"};
    constexpr chart::Signal SwitchBackhaul_Signal            {Base_ + 31, "SwitchBackhaul_Signal"};
    constexpr chart::Signal RequestDdsSwitch_Signal          {Base_ + 32, "RequestDdsSwitch_Signal"};
    constexpr chart::Signal RequestCurrentDds_Signal         {Base_ + 33, "RequestCurrentDds_Signal"};
    constexpr chart::Signal WwanConnectivityEvt_Signal       {Base_ + 34, "WwanConnectivityEvt_Signal"};
    constexpr chart::Signal DdsChangedEvt_Signal             {Base_ + 35, "DdsChangedEvt_Signal"};
    constexpr chart::Signal IpptConfigSyncEvt_Signal         {Base_ + 36, "IpptConfigSyncEvt_Signal"};
    constexpr chart::Signal IpConfigSyncEvt_Signal           {Base_ + 37, "IpConfigSyncEvt_Signal"};

    constexpr int Net_Signal_End = Base_ + 38;
}

}  // namespace telux::data::net::simula

#endif  // TELUX_DATA_NET_SIMULA_SIGNALS_HPP
