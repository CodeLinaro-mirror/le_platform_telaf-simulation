/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "taf_pa_common.h"
#include "taf_pa_location.hpp"

using namespace tafpa::location;

pa_result_t tafpa::location::taf_pa_location_Init()
{
    PA_INFO("[simulation-location]: initialized");
    // return PA_NOT_IMPLEMENTED;
    return PA_OK;
}

taf_pa_location_LocationId tafpa::location::taf_pa_location_CreateClient()
{
    PA_INFO("Location PA: Default platform adapter CreateClient() called");
    return 0;
}

pa_result_t tafpa::location::taf_pa_location_DeleteClient(taf_pa_location_LocationId clientId)
{
    PA_INFO("Location PA: Default platform adapter DeleteClient()");
    (void)clientId;
    return PA_NOT_IMPLEMENTED;
}

pa_result_t tafpa::location::taf_pa_location_RegisterListener(taf_pa_location_LocationId clientId, taf_pa_location_EventListener* eventListener, std::any context)
{
    PA_INFO("Location PA: Default platform adapter RegisterListener() called ");
    (void)clientId;
    (void)eventListener;
    (void)context;
    return PA_NOT_IMPLEMENTED;
}

pa_result_t tafpa::location::taf_pa_location_startDetailedEngineReports(taf_pa_location_LocationId clientId, uint32_t optInterval, uint16_t engineType, taf_pa_location_GeneralCb callback, uint32_t reportMask, std::any context)
{
    PA_INFO("Location PA: Default platform adapter startDetailedEngineReports() called ");
    (void)clientId;
    (void)optInterval;
    (void)engineType;
    (void)reportMask;
    (void)callback;
    (void)context;
    return PA_NOT_IMPLEMENTED;
}

pa_result_t tafpa::location::taf_pa_location_stopReports(taf_pa_location_LocationId clientId, taf_pa_location_GeneralCb callback, std::any context)
{
    PA_INFO("Location PA: Default platform adapter stopReports() called");
    (void)clientId;
    (void)callback;
    (void)context;
    return PA_NOT_IMPLEMENTED;
}

uint32_t tafpa::location::taf_pa_location_getCapabilities(taf_pa_location_LocationId clientId, std::any context)
{
    PA_INFO("Location PA: Default platform adapter getCapabilities() called");
    (void)clientId;
    (void)context;
    return 0;
}

pa_result_t tafpa::location::taf_pa_location_configureConstellations(const std::vector<taf_pa_location_SvBlackListInfo_t>& svBlackListData, taf_pa_location_GeneralCb callback, bool deviceReset, std::any context)
{
    PA_INFO("Location PA: Default platform adapter configureConstellations() called");
    (void)svBlackListData;
    (void)deviceReset;
    (void)callback;
    (void)context;
    return PA_NOT_IMPLEMENTED;
}

pa_result_t tafpa::location::taf_pa_location_deleteAidingData(taf_pa_location_AidingDataType_t aidingData, taf_pa_location_GeneralCb callback, std::any context)
{
    PA_INFO("Location PA: Default platform adapter deleteAidingData() called");
    (void)aidingData;
    (void)callback;
    (void)context;
    return PA_NOT_IMPLEMENTED;
}

pa_result_t tafpa::location::taf_pa_location_deleteAllAidingData(taf_pa_location_GeneralCb callback, std::any context)
{
    PA_INFO("Location PA: Default platform adapter deleteAllAidingData() called");
    (void)callback;
    (void)context;
    return PA_NOT_IMPLEMENTED;
}

pa_result_t tafpa::location::taf_pa_location_configureMinSVElevation(uint8_t minElevation, taf_pa_location_GeneralCb callback, std::any context)
{
    PA_INFO("Location PA: Default platform adapter configureMinSVElevation() called");
    (void)minElevation;
    (void)callback;
    (void)context;
    return PA_NOT_IMPLEMENTED;
}

pa_result_t tafpa::location::taf_pa_location_requestMinSVElevation(taf_pa_location_RequestMinSVElevationCb callback, std::any context)
{
    PA_INFO("Location PA: Default platform adapter requestMinSVElevation() called");
    (void)callback;
    (void)context;
    return PA_NOT_IMPLEMENTED;
}

pa_result_t tafpa::location::taf_pa_location_configureNmeaTypes(taf_pa_location_NmeaSentenceType_t nmeaType, taf_pa_location_GeneralCb callback, std::any context)
{
    PA_INFO("Location PA: Default platform adapter configureNmeaTypes() called ");
    (void)nmeaType;
    (void)callback;
    (void)context;
    return PA_NOT_IMPLEMENTED;
}

pa_result_t tafpa::location::taf_pa_location_configureDR(const taf_pa_location_DREngineConfiguration_t& drConfig, taf_pa_location_GeneralCb callback, std::any context)
{
    PA_INFO("Location PA: Default platform adapter configureDR() called (unimplemented)");
    (void)drConfig;
    (void)callback;
    (void)context;
    return PA_NOT_IMPLEMENTED;
}

pa_result_t tafpa::location::taf_pa_location_configureEngineState(taf_pa_location_EngineType_t engineType, taf_pa_location_LocationEngineRunState_t engineState,taf_pa_location_GeneralCb callback, std::any context)
{
    PA_INFO("Location PA: Default platform adapter configureEngineState() called");
    (void)engineType;
    (void)engineState;
    (void)callback;
    (void)context;
    return PA_NOT_IMPLEMENTED;
}

pa_result_t tafpa::location::taf_pa_location_configureRobustLocation(bool enableRobustloc, bool enableE911loc, taf_pa_location_GeneralCb callback, std::any context)
{
    PA_INFO("Location PA: Default platform adapter configureRobustLocation() called (unimplemented)");
    (void)enableRobustloc;
    (void)enableE911loc;
    (void)callback;
    (void)context;
    return PA_NOT_IMPLEMENTED;
}

pa_result_t tafpa::location::taf_pa_location_requestRobustLocation(taf_pa_location_RequestRobustLocationCb callback, std::any context)
{
    PA_INFO("Location PA: Default platform adapter requestRobustLocation() called (unimplemented)");
    (void)callback;
    (void)context;
    return PA_NOT_IMPLEMENTED;
}

pa_result_t tafpa::location::taf_pa_location_configureSecondaryBand(const std::unordered_set<taf_pa_location_GnssConstellationType_t>& constSet, taf_pa_location_GeneralCb callback, std::any context)
{
    PA_INFO("Location PA: Default platform adapter configureSecondaryBand() called (unimplemented)");
    (void)constSet;
    (void)callback;
    (void)context;
    return PA_NOT_IMPLEMENTED;
}

pa_result_t tafpa::location::taf_pa_location_requestSecondaryBandConfig(taf_pa_location_RequestSecondaryBandConfigCb callback, std::any context)
{
    PA_INFO("Location PA: Default platform adapter requestSecondaryBandConfig() called (unimplemented)");
    (void)callback;
    (void)context;
    return PA_NOT_IMPLEMENTED;
}

pa_result_t tafpa::location::taf_pa_location_configureLeverArm(const taf_pa_location_LeverArmParams_t* leverArmConfigInfoPtr, taf_pa_location_GeneralCb callback, std::any context)
{
    PA_INFO("Location PA: Default platform adapter configureLeverArm() called (unimplemented)");
    (void)leverArmConfigInfoPtr;
    (void)callback;
    (void)context;
    return PA_NOT_IMPLEMENTED;
}

pa_result_t tafpa::location::taf_pa_location_configureMinGpsWeek(uint16_t minGpsWeek, taf_pa_location_GeneralCb callback, std::any context)
{
    PA_INFO("Location PA: Default platform adapter configureMinGpsWeek() called (unimplemented)");
    (void)minGpsWeek;
    (void)callback;
    (void)context;
    return PA_NOT_IMPLEMENTED;
}

pa_result_t tafpa::location::taf_pa_location_configureNmea(const taf_pa_location_NmeaConfig_t& nmeaConfigData, taf_pa_location_GeneralCb callback, std::any context)
{
    PA_INFO("Location PA: Default platform adapter configureNmea() called (unimplemented)");
    (void)nmeaConfigData;
    (void)callback;
    (void)context;
    return PA_NOT_IMPLEMENTED;
}

pa_result_t tafpa::location::taf_pa_location_requestMinGpsWeek(taf_pa_location_RequestMinGpsWeekCb callback, std::any context)
{
    PA_INFO("Location PA: Default platform adapter requestMinGpsWeek() called (unimplemented)");
    (void)callback;
    (void)context;
    return PA_NOT_IMPLEMENTED;
}

pa_result_t tafpa::location::taf_pa_location_requestXtraStatus(taf_pa_location_RequestXtraStatusCb callback, std::any context)
{
    PA_INFO("Location PA: Default platform adapter requestXtraStatus() called (unimplemented)");
    (void)callback;
    (void)context;
    return PA_NOT_IMPLEMENTED;
}
