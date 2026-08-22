// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear
//
// Signals.hpp - chart user-signal definitions for the sml-pa data business domain.
//
// Starts at telux::common::simula::Common_Signal_End so data-domain signal
// ids never collide with component/common/Signals.hpp's range even though
// both live in the same process-wide chart::Event signal space.

#ifndef TELUX_DATA_SIMULA_SIGNALS_HPP
#define TELUX_DATA_SIMULA_SIGNALS_HPP

#include "../common/Signals.hpp"

namespace telux::data::simula {

namespace DataSignals {
    constexpr int Base_ = telux::common::simula::Common_Signal_End;

    // Manager readiness (2-state NotReady <-> Ready shell shared by
    // DataConnectionManager / DataProfileManager / ServingSystemManager)
    constexpr chart::Signal ReadinessEvt_Signal              {Base_ + 0,  "ReadinessEvt_Signal"};
    constexpr chart::Signal BridgeConnectivityChanged_Signal {Base_ + 1,  "BridgeConnectivityChanged_Signal"};

    // DataConnectionManager
    constexpr chart::Signal StartDataCall_Signal             {Base_ + 2,  "StartDataCall_Signal"};
    constexpr chart::Signal StopDataCall_Signal              {Base_ + 3,  "StopDataCall_Signal"};
    constexpr chart::Signal RequestDataCallList_Signal       {Base_ + 4,  "RequestDataCallList_Signal"};
    constexpr chart::Signal SessionFinished_Signal           {Base_ + 5,  "SessionFinished_Signal"};
    constexpr chart::Signal GetDefaultProfile_Signal         {Base_ + 6,  "GetDefaultProfile_Signal"};

    // DataCallSession
    constexpr chart::Signal Start_Signal                     {Base_ + 7,  "Data::Start_Signal"};
    constexpr chart::Signal Stop_Signal                      {Base_ + 8, "Data::Stop_Signal"};
    constexpr chart::Signal StartRsp_Signal                  {Base_ + 9, "StartRsp_Signal"};
    constexpr chart::Signal StopRsp_Signal                   {Base_ + 10, "StopRsp_Signal"};
    constexpr chart::Signal StateInd_Signal                  {Base_ + 11, "StateInd_Signal"};
    constexpr chart::Signal BringupTimeout_Signal            {Base_ + 12, "BringupTimeout_Signal"};
    constexpr chart::Signal TeardownTimeout_Signal           {Base_ + 13, "TeardownTimeout_Signal"};
    constexpr chart::Signal ListRsp_Signal                   {Base_ + 14, "ListRsp_Signal"};

    // DataProfileManager
    constexpr chart::Signal QueryProfile_Signal              {Base_ + 15, "QueryProfile_Signal"};
    constexpr chart::Signal RequestProfile_Signal            {Base_ + 16, "RequestProfile_Signal"};
    constexpr chart::Signal RequestProfileList_Signal        {Base_ + 17, "RequestProfileList_Signal"};
    constexpr chart::Signal CreateProfile_Signal             {Base_ + 18, "CreateProfile_Signal"};
    constexpr chart::Signal ModifyProfile_Signal             {Base_ + 19, "ModifyProfile_Signal"};
    constexpr chart::Signal DeleteProfile_Signal             {Base_ + 20, "DeleteProfile_Signal"};
    constexpr chart::Signal ProfileChangedEvt_Signal         {Base_ + 21, "ProfileChangedEvt_Signal"};

    // ServingSystemManager
    constexpr chart::Signal RequestServiceStatus_Signal      {Base_ + 22, "RequestServiceStatus_Signal"};
    constexpr chart::Signal RequestRoamingStatus_Signal      {Base_ + 23, "RequestRoamingStatus_Signal"};
    constexpr chart::Signal RequestNrIconType_Signal         {Base_ + 24, "RequestNrIconType_Signal"};
    constexpr chart::Signal MakeDormant_Signal               {Base_ + 25, "MakeDormant_Signal"};
    constexpr chart::Signal ServStateEvt_Signal              {Base_ + 26, "ServStateEvt_Signal"};
    constexpr chart::Signal ServRoamingEvt_Signal            {Base_ + 27, "ServRoamingEvt_Signal"};

    // DataConnectionManager -- full data-domain sim
    constexpr chart::Signal SetDefaultProfile_Signal         {Base_ + 28, "SetDefaultProfile_Signal"};
    constexpr chart::Signal RequestThrottledApnInfo_Signal   {Base_ + 29, "RequestThrottledApnInfo_Signal"};
    constexpr chart::Signal SetThroughputInterval_Signal     {Base_ + 30, "SetThroughputInterval_Signal"};
    constexpr chart::Signal GetLastThroughputInfo_Signal     {Base_ + 31, "GetLastThroughputInfo_Signal"};
    constexpr chart::Signal ThroughputInfoEvt_Signal         {Base_ + 32, "ThroughputInfoEvt_Signal"};
    constexpr chart::Signal QosStatusEvt_Signal              {Base_ + 33, "QosStatusEvt_Signal"};
    constexpr chart::Signal HwAccelEvt_Signal                {Base_ + 34, "HwAccelEvt_Signal"};
    constexpr chart::Signal ThrottleStatusEvt_Signal         {Base_ + 35, "ThrottleStatusEvt_Signal"};

    constexpr int Data_Signal_End = Base_ + 36;
}

}  // namespace telux::data::simula

#endif  // TELUX_DATA_SIMULA_SIGNALS_HPP
