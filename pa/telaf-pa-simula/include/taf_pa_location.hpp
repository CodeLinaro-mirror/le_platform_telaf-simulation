/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef TAF_PA_LOCATION_H
#define TAF_PA_LOCATION_H

#include <string>
#include <vector>
#include <set>
#include <unordered_set>
#include <functional> // For std::function
#include <memory>     // For std::shared_ptr
#include <any>        // For std::any context

// Assuming taf_pa_common.hpp defines pa_result_t and PA_SHARED / PA_WEAK
// If not, you might need to define pa_result_t or similar here,
// or include a specific header that does.
#include "taf_pa_common.h"

namespace tafpa::location {

#define PA_MAX_LEN_BYTE 50
#define TAF_PA_LOCATION_UNKNOWN -1
#define TAF_PA_GNSS_DATA_ARRAY_SIZE 23

// Define the Client ID type
using taf_pa_location_LocationId = uint64_t;

typedef enum{
TAF_PA_LOCATION_CALCULATED = 0,
TAF_PA_LOCATION_ASSUMED = 1,
}taf_pa_location_AltitudeType_t;

typedef enum{
TAF_PA_LOCATION_SUSPENDED = 1,
TAF_PA_LOCATION_RUNNING = 2,
TAF_PA_LOCATION_SUSPEND_RETAIN = 3
}taf_pa_location_LocationEngineRunState_t;

typedef enum{
TAF_PA_LOCATION_SPE = 1,
TAF_PA_LOCATION_PPE = 2,
TAF_PA_LOCATION_DRE = 3,
TAF_PA_LOCATION_VPE = 4
}taf_pa_location_EngineType_t;

typedef enum{
TAF_PA_LOCATION_HAS_LONG_ACCEL = (1 << 0),
TAF_PA_LOCATION_HAS_LAT_ACCEL = (1 << 1),
TAF_PA_LOCATION_HAS_VERT_ACCEL = (1 << 2),
TAF_PA_LOCATION_HAS_YAW_RATE = (1 << 3),
TAF_PA_LOCATION_HAS_PITCH = (1 << 4),
TAF_PA_LOCATION_HAS_LONG_ACCEL_UNC = (1 << 5),
TAF_PA_LOCATION_HAS_LAT_ACCEL_UNC = (1 << 6),
TAF_PA_LOCATION_HAS_VERT_ACCEL_UNC = (1 << 7),
TAF_PA_LOCATION_HAS_YAW_RATE_UNC = (1 << 8),
TAF_PA_LOCATION_HAS_PITCH_UNC = (1 << 9),
TAF_PA_LOCATION_HAS_PITCH_RATE_BIT = (1<<10),
TAF_PA_LOCATION_HAS_PITCH_RATE_UNC_BIT = (1<<11),
TAF_PA_LOCATION_HAS_ROLL_BIT = (1<<12),
TAF_PA_LOCATION_HAS_ROLL_UNC_BIT = (1<<13),
TAF_PA_LOCATION_HAS_ROLL_RATE_BIT = (1<<14),
TAF_PA_LOCATION_HAS_ROLL_RATE_UNC_BIT = (1<<15),
TAF_PA_LOCATION_HAS_YAW_BIT = (1<<16),
TAF_PA_LOCATION_HAS_YAW_UNC_BIT = (1<<17)
}taf_pa_location_KinematicDataValidityType_t;

typedef enum{
TAF_PA_LOCATION_LOC_GNSS = (1 << 0),
TAF_PA_LOCATION_LOC_CELL = (1 << 1),
TAF_PA_LOCATION_LOC_WIFI = (1 << 2),
TAF_PA_LOCATION_LOC_SENSORS = (1 << 3),
TAF_PA_LOCATION_LOC_REFERENCE_LOCATION = (1 << 4),
TAF_PA_LOCATION_LOC_INJECTED_COARSE_POSITION = (1 << 5),
TAF_PA_LOCATION_LOC_AFLT = (1 << 6),
TAF_PA_LOCATION_LOC_HYBRID = (1 << 7),
TAF_PA_LOCATION_LOC_PPE = (1 << 8),
TAF_PA_LOCATION_LOC_VEH = (1 << 9),
TAF_PA_LOCATION_LOC_VIS = (1 << 10),
TAF_PA_LOCATION_LOC_PROPAGATED = (1 << 11),
}taf_pa_location_LocationTechnologyType_t;

typedef enum{
TAF_PA_LOCATION_HAS_LAT_LONG_BIT           = (1<<0),
TAF_PA_LOCATION_HAS_ALTITUDE_BIT           = (1<<1),
TAF_PA_LOCATION_HAS_SPEED_BIT              = (1<<2),
TAF_PA_LOCATION_HAS_HEADING_BIT            = (1<<3),
TAF_PA_LOCATION_HAS_HORIZONTAL_ACCURACY_BIT = (1<<4),
TAF_PA_LOCATION_HAS_VERTICAL_ACCURACY_BIT = (1<<5),
TAF_PA_LOCATION_HAS_SPEED_ACCURACY_BIT     = (1<<6),
TAF_PA_LOCATION_HAS_HEADING_ACCURACY_BIT   = (1<<7),
TAF_PA_LOCATION_HAS_TIMESTAMP_BIT          = (1<<8),
TAF_PA_LOCATION_HAS_ELAPSED_REAL_TIME_BIT = (1<<9),
TAF_PA_LOCATION_HAS_ELAPSED_REAL_TIME_UNC_BIT = (1<<10),
TAF_PA_LOCATION_HAS_TIME_UNC_BIT = (1<<11),
TAF_PA_LOCATION_HAS_GPTP_TIME_BIT          = (1<<12),
TAF_PA_LOCATION_HAS_GPTP_TIME_UNC_BIT      = (1<<13)
}taf_pa_location_LocationValidityType_t;

typedef enum{
TAF_PA_LOCATION_HAS_ALTITUDE_MEAN_SEA_LEVEL = (1ULL << 0),
TAF_PA_LOCATION_HAS_DOP = (1ULL << 1),
TAF_PA_LOCATION_HAS_MAGNETIC_DEVIATION = (1ULL << 2),
TAF_PA_LOCATION_HAS_HOR_RELIABILITY = (1ULL << 3),
TAF_PA_LOCATION_HAS_VER_RELIABILITY = (1ULL << 4),
TAF_PA_LOCATION_HAS_HOR_ACCURACY_ELIP_SEMI_MAJOR = (1ULL << 5),
TAF_PA_LOCATION_HAS_HOR_ACCURACY_ELIP_SEMI_MINOR = (1ULL << 6),
TAF_PA_LOCATION_HAS_HOR_ACCURACY_ELIP_AZIMUTH = (1ULL << 7),
TAF_PA_LOCATION_HAS_GNSS_SV_USED_DATA = (1ULL << 8),
TAF_PA_LOCATION_HAS_NAV_SOLUTION_MASK = (1ULL << 9),
TAF_PA_LOCATION_HAS_POS_TECH_MASK = (1ULL << 10),
TAF_PA_LOCATION_HAS_SV_SOURCE_INFO = (1ULL << 11),
TAF_PA_LOCATION_HAS_POS_DYNAMICS_DATA = (1ULL << 12),
TAF_PA_LOCATION_HAS_EXT_DOP = (1ULL << 13),
TAF_PA_LOCATION_HAS_NORTH_STD_DEV = (1ULL << 14),
TAF_PA_LOCATION_HAS_EAST_STD_DEV = (1ULL << 15),
TAF_PA_LOCATION_HAS_NORTH_VEL = (1ULL << 16),
TAF_PA_LOCATION_HAS_EAST_VEL = (1ULL << 17),
TAF_PA_LOCATION_HAS_UP_VEL = (1ULL << 18),
TAF_PA_LOCATION_HAS_NORTH_VEL_UNC = (1ULL << 19),
TAF_PA_LOCATION_HAS_EAST_VEL_UNC = (1ULL << 20),
TAF_PA_LOCATION_HAS_UP_VEL_UNC = (1ULL << 21),
TAF_PA_LOCATION_HAS_LEAP_SECONDS = (1ULL << 22),
TAF_PA_LOCATION_HAS_TIME_UNC = (1ULL << 23),
TAF_PA_LOCATION_HAS_NUM_SV_USED_IN_POSITION = (1ULL << 24),
TAF_PA_LOCATION_HAS_CALIBRATION_CONFIDENCE_PERCENT = (1ULL << 25),
TAF_PA_LOCATION_HAS_CALIBRATION_STATUS = (1ULL << 26),
TAF_PA_LOCATION_HAS_OUTPUT_ENG_TYPE = (1ULL << 27),
TAF_PA_LOCATION_HAS_OUTPUT_ENG_MASK = (1ULL << 28),
TAF_PA_LOCATION_HAS_CONFORMITY_INDEX_FIX = (1ULL << 29),
TAF_PA_LOCATION_HAS_LLA_VRP_BASED = (1ULL << 30),
TAF_PA_LOCATION_HAS_ENU_VELOCITY_VRP_BASED = (1ULL << 31),
TAF_PA_LOCATION_HAS_ALTITUDE_TYPE = (1ULL << 32),
TAF_PA_LOCATION_HAS_REPORT_STATUS = (1ULL << 33),
TAF_PA_LOCATION_HAS_INTEGRITY_RISK_USED = (1ULL << 34),
TAF_PA_LOCATION_HAS_PROTECT_LEVEL_ALONG_TRACK = (1ULL << 35),
TAF_PA_LOCATION_HAS_PROTECT_LEVEL_CROSS_TRACK = (1ULL << 36),
TAF_PA_LOCATION_HAS_PROTECT_LEVEL_VERTICAL = (1ULL << 37),
TAF_PA_LOCATION_HAS_SOLUTION_STATUS = (1ULL << 38),
TAF_PA_LOCATION_HAS_DGNSS_STATION_ID = (1ULL<<39),
TAF_PA_LOCATION_HAS_BASE_LINE_LENGTH = (1ULL<<40),
TAF_PA_LOCATION_HAS_AGE_OF_CORRECTION = (1ULL<<41),
TAF_PA_LOCATION_HAS_LEAP_SECONDS_UNC = (1ULL<<42)
}taf_pa_location_LocationInfoExValidityType_t;

typedef enum{
TAF_PA_LOCATION_DR_ROLL_CALIBRATION_NEEDED          = (1<<0),
TAF_PA_LOCATION_DR_PITCH_CALIBRATION_NEEDED = (1<<1),
TAF_PA_LOCATION_DR_YAW_CALIBRATION_NEEDED           = (1<<2),
TAF_PA_LOCATION_DR_ODO_CALIBRATION_NEEDED           = (1<<3),
TAF_PA_LOCATION_DR_GYRO_CALIBRATION_NEEDED          = (1<<4)
}taf_pa_location_DrCalibrationStatusType_t;

typedef enum{
TAF_PA_LOCATION_VEHICLE_SENSOR_SPEED_INPUT_DETECTED = (1<<0),
TAF_PA_LOCATION_VEHICLE_SENSOR_SPEED_INPUT_USED     = (1<<1),
TAF_PA_LOCATION_WARNING_UNCALIBRATED                = (1<<2),
TAF_PA_LOCATION_WARNING_GNSS_QUALITY_INSUFFICIENT   = (1<<3),
TAF_PA_LOCATION_WARNING_FERRY_DETECTED              = (1<<4),
TAF_PA_LOCATION_ERROR_6DOF_SENSOR_UNAVAILABLE       = (1<<5),
TAF_PA_LOCATION_ERROR_VEHICLE_SPEED_UNAVAILABLE     = (1<<6),
TAF_PA_LOCATION_ERROR_GNSS_EPH_UNAVAILABLE          = (1<<7),
TAF_PA_LOCATION_ERROR_GNSS_MEAS_UNAVAILABLE         = (1<<8),
TAF_PA_LOCATION_WARNING_INIT_POSITION_INVALID       = (1<<9),
TAF_PA_LOCATION_WARNING_INIT_POSITION_UNRELIABLE    = (1<<10),
TAF_PA_LOCATION_WARNING_POSITON_UNRELIABLE          = (1<<11),
TAF_PA_LOCATION_ERROR_GENERIC                       = (1<<12),
TAF_PA_LOCATION_WARNING_SENSOR_TEMP_OUT_OF_RANGE    = (1<<13),
TAF_PA_LOCATION_WARNING_USER_DYNAMICS_INSUFFICIENT  = (1<<14),
TAF_PA_LOCATION_WARNING_FACTORY_DATA_INCONSISTENT   = (1<<15)
}taf_pa_location_DrSolutionStatusType_t;

typedef enum{
TAF_PA_LOCATION_LOC_OUTPUT_ENGINE_FUSED = 0,
TAF_PA_LOCATION_LOC_OUTPUT_ENGINE_SPE   = 1,
TAF_PA_LOCATION_LOC_OUTPUT_ENGINE_PPE   = 2,
TAF_PA_LOCATION_LOC_OUTPUT_ENGINE_VPE   = 3,
}taf_pa_location_LocationAggregationType_t;

typedef enum{
TAF_PA_LOCATION_STANDARD_POSITIONING_ENGINE = (1 << 0),
TAF_PA_LOCATION_DEAD_RECKONING_ENGINE       = (1 << 1),
TAF_PA_LOCATION_PRECISE_POSITIONING_ENGINE  = (1 << 2),
TAF_PA_LOCATION_VP_POSITIONING_ENGINE       = (1 << 3),
}taf_pa_location_PositioningEngineType_t;

typedef enum{
TAF_PA_LOCATION_BODY_TO_SENSOR_MOUNT_PARAMS_VALID          = (1<<0),
TAF_PA_LOCATION_VEHICLE_SPEED_SCALE_FACTOR_VALID           = (1<<1),
TAF_PA_LOCATION_VEHICLE_SPEED_SCALE_FACTOR_UNC_VALID = (1<<2),
TAF_PA_LOCATION_GYRO_SCALE_FACTOR_VALID                    = (1<<3),
TAF_PA_LOCATION_GYRO_SCALE_FACTOR_UNC_VALID                = (1<<4),
}taf_pa_location_DRConfigValidityType_t;

typedef enum{
TAF_PA_LOCATION_NOT_SET = 0,
TAF_PA_LOCATION_VERY_LOW = 1,
TAF_PA_LOCATION_LOW = 2,
TAF_PA_LOCATION_MEDIUM = 3,
TAF_PA_LOCATION_HIGH = 4,
}taf_pa_location_LocationReliability_t;

typedef enum{
TAF_PA_LOCATION_VALID_ENABLED          = (1<<0),
TAF_PA_LOCATION_VALID_ENABLED_FOR_E911 = (1<<1),
TAF_PA_LOCATION_VALID_VERSION          = (1<<2)
}taf_pa_location_RobustLocationConfigType_t;

typedef enum{
TAF_PA_LOCATION_GPS = 1,
TAF_PA_LOCATION_GALILEO = 2,
TAF_PA_LOCATION_SBAS = 3,
TAF_PA_LOCATION_COMPASS = 4,
TAF_PA_LOCATION_GLONASS = 5,
TAF_PA_LOCATION_BDS = 6,
TAF_PA_LOCATION_QZSS = 7,
TAF_PA_LOCATION_NAVIC = 8,
}taf_pa_location_GnssConstellationType_t;

struct taf_pa_location_SvBlackListInfo_t{
taf_pa_location_GnssConstellationType_t constellation;
uint32_t svId;
};

typedef enum{
TAF_PA_LOCATION_HAS_JAMMER = (1ULL << 0),
TAF_PA_LOCATION_HAS_AGC = (1ULL << 1)
}taf_pa_location_GnssDataValidityType_t;

typedef enum{
TAF_PA_LOCATION_LEAP_SECOND_SYS_INFO_CURRENT_LEAP_SECONDS_BIT = (1ULL << 0),
TAF_PA_LOCATION_LEAP_SECOND_SYS_INFO_LEAP_SECOND_CHANGE_BIT = (1ULL << 1)
}taf_pa_location_LeapSecondInfoValidityType_t;

typedef enum{
TAF_PA_LOCATION_LOCATION_SYS_INFO_LEAP_SECOND = (1ULL << 0),
}taf_pa_location_LocationSystemInfoValidityType_t;

typedef enum{
TAF_PA_LOCATION_SBAS_CORRECTION_IONO = 0,
TAF_PA_LOCATION_SBAS_CORRECTION_FAST = 1,
TAF_PA_LOCATION_SBAS_CORRECTION_LONG = 2,
TAF_PA_LOCATION_SBAS_INTEGRITY = 3,
TAF_PA_LOCATION_SBAS_CORRECTION_DGNSS = 4,
TAF_PA_LOCATION_SBAS_CORRECTION_RTK = 5,
TAF_PA_LOCATION_SBAS_CORRECTION_PPP = 6,
TAF_PA_LOCATION_SBAS_CORRECTION_RTK_FIXED = 7,
TAF_PA_LOCATION_SBAS_CORRECTED_SV_USED = 8,
TAF_PA_LOCATION_SBAS_COUNT = 9
}taf_pa_location_SbasCorrectionType_t;

typedef enum{
TAF_PA_LOCATION_STATUS_UNKNOWN = 0,
TAF_PA_LOCATION_STATUS_NOT_AVAIL = 1,
TAF_PA_LOCATION_STATUS_NOT_VALID = 2,
TAF_PA_LOCATION_STATUS_VALID = 3,
}taf_pa_location_XtraDataStatus_t;

typedef enum{
TAF_PA_LOCATION_GNSS_LOC_SV_SYSTEM_UNKNOWN = -1,
TAF_PA_LOCATION_GNSS_LOC_SV_SYSTEM_GPS = 1,
TAF_PA_LOCATION_GNSS_LOC_SV_SYSTEM_GALILEO = 2,
TAF_PA_LOCATION_GNSS_LOC_SV_SYSTEM_SBAS = 3,
TAF_PA_LOCATION_GNSS_LOC_SV_SYSTEM_COMPASS = 4,
TAF_PA_LOCATION_GNSS_LOC_SV_SYSTEM_GLONASS = 5,
TAF_PA_LOCATION_GNSS_LOC_SV_SYSTEM_BDS = 6,
TAF_PA_LOCATION_GNSS_LOC_SV_SYSTEM_QZSS = 7,
TAF_PA_LOCATION_GNSS_LOC_SV_SYSTEM_NAVIC = 8
}taf_pa_location_GnssSystem_t;

typedef enum{
TAF_PA_LOCATION_GPS_L1CA = (1<<0),
TAF_PA_LOCATION_GPS_L1C = (1<<1),
TAF_PA_LOCATION_GPS_L2 = (1<<2),
TAF_PA_LOCATION_GPS_L5 = (1<<3),
TAF_PA_LOCATION_GLONASS_G1 = (1<<4),
TAF_PA_LOCATION_GLONASS_G2 = (1<<5),
TAF_PA_LOCATION_GALILEO_E1 = (1<<6),
TAF_PA_LOCATION_GALILEO_E5A = (1<<7),
TAF_PA_LOCATION_GALILEO_E5B = (1<<8),
TAF_PA_LOCATION_BEIDOU_B1 = (1<<9),
TAF_PA_LOCATION_BEIDOU_B2 = (1<<10),
TAF_PA_LOCATION_QZSS_L1CA = (1<<11),
TAF_PA_LOCATION_QZSS_L1S = (1<<12),
TAF_PA_LOCATION_QZSS_L2 = (1<<13),
TAF_PA_LOCATION_QZSS_L5 = (1<<14),
TAF_PA_LOCATION_SBAS_L1 = (1<<15),
TAF_PA_LOCATION_BEIDOU_B1I = (1<<16),
TAF_PA_LOCATION_BEIDOU_B1C = (1<<17),
TAF_PA_LOCATION_BEIDOU_B2I = (1<<18),
TAF_PA_LOCATION_BEIDOU_B2AI = (1<<19),
TAF_PA_LOCATION_NAVIC_L5 = (1<<20),
TAF_PA_LOCATION_BEIDOU_B2AQ = (1<<21),
TAF_PA_LOCATION_BEIDOU_B2BI = (1<<22),
TAF_PA_LOCATION_BEIDOU_B2BQ = (1<<23),
TAF_PA_LOCATION_NAVIC_L1 = (1<<24)
}taf_pa_location_GnssSignalType_t;

typedef enum{
TAF_PA_LOCATION_SUCCESS = 0,
TAF_PA_LOCATION_INTERMEDIATE = 1,
TAF_PA_LOCATION_FAILURE = 2
}taf_pa_location_ReportStatus_t;

typedef enum{
TAF_PA_LOCATION_GNSS_DEFAULT = 0,
TAF_PA_LOCATION_GNSS_SATELLITE = (1 << 0),
TAF_PA_LOCATION_GNSS_CELLID = (1 << 1),
TAF_PA_LOCATION_GNSS_WIFI = (1 << 2),
TAF_PA_LOCATION_GNSS_SENSORS = (1 << 3),
TAF_PA_LOCATION_GNSS_REFERENCE_LOCATION = (1 << 4),
TAF_PA_LOCATION_GNSS_INJECTED_COARSE_POSITION= (1 << 5),
TAF_PA_LOCATION_GNSS_AFLT = (1 << 6),
TAF_PA_LOCATION_GNSS_HYBRID = (1 << 7),
TAF_PA_LOCATION_GNSS_PPE = (1 << 8),
TAF_PA_LOCATION_GNSS_VEHICLE = (1 << 9),
TAF_PA_LOCATION_GNSS_VISUAL = (1 << 10),
TAF_PA_LOCATION_GNSS_PROPAGATED = (1 << 11),
}taf_pa_location_GnssPositionTechType_t;

typedef enum{
TAF_PA_LOCATION_TIME_BASED_TRACKING = (1<<0),
TAF_PA_LOCATION_DISTANCE_BASED_TRACKING = (1<<1),
TAF_PA_LOCATION_GNSS_MEASUREMENTS = (1<<2),
TAF_PA_LOCATION_CONSTELLATION_ENABLEMENT = (1<<3),
TAF_PA_LOCATION_CARRIER_PHASE = (1<<4),
TAF_PA_LOCATION_QWES_GNSS_SINGLE_FREQUENCY = (1<<5),
TAF_PA_LOCATION_QWES_GNSS_MULTI_FREQUENCY = (1<<6),
TAF_PA_LOCATION_QWES_VPE = (1<<7),
TAF_PA_LOCATION_QWES_CV2X_LOCATION_BASIC = (1<<8),
TAF_PA_LOCATION_QWES_CV2X_LOCATION_PREMIUM = (1<<9),
TAF_PA_LOCATION_QWES_PPE = (1<<10),
TAF_PA_LOCATION_QWES_QDR2 = (1<<11),
TAF_PA_LOCATION_QWES_QDR3 = (1<<12),
TAF_PA_LOCATION_TIME_BASED_BATCHING = (1<<13),
TAF_PA_LOCATION_DISTANCE_BASED_BATCHING = (1<<14),
TAF_PA_LOCATION_GEOFENCE = (1<<15),
TAF_PA_LOCATION_OUTDOOR_TRIP_BATCHING = (1<<16),
TAF_PA_LOCATION_SV_POLYNOMIAL = (1<<17),
TAF_PA_LOCATION_NLOS_ML20 = (1<<18)
}taf_pa_location_LocCapabilityType_t;

typedef enum{
TAF_PA_LOCATION_GEODETIC_TYPE_NONE = -1,
TAF_PA_LOCATION_GEODETIC_TYPE_WGS_84 = 0,
TAF_PA_LOCATION_GEODETIC_TYPE_PZ_90 = 1,
}taf_pa_location_GeodeticDatumType_t;

typedef enum {
TAF_PA_LOCATION_GGA = (1 << 0),
TAF_PA_LOCATION_RMC = (1 << 1),
TAF_PA_LOCATION_GSA = (1 << 2),
TAF_PA_LOCATION_VTG = (1 << 3),
TAF_PA_LOCATION_GNS = (1 << 4),
TAF_PA_LOCATION_DTM = (1 << 5),
TAF_PA_LOCATION_GPGSV = (1 << 6),
TAF_PA_LOCATION_GLGSV = (1 << 7),
TAF_PA_LOCATION_GAGSV = (1 << 8),
TAF_PA_LOCATION_GQGSV = (1 << 9),
TAF_PA_LOCATION_GBGSV = (1 << 10),
TAF_PA_LOCATION_GIGSV = (1 << 11),
TAF_PA_LOCATION_ALL = 0xffffffff,
}taf_pa_location_NmeaSentenceType_t;

typedef enum{
TAF_PA_LOCATION_YES = 0,
TAF_PA_LOCATION_NO = 1
}taf_pa_location_SVInfoAvailability_t;

struct taf_pa_location_XtraStatus_t {
bool featureEnabled;
taf_pa_location_XtraDataStatus_t xtraDataStatus;
uint32_t xtraValidForHours;
};

struct taf_pa_location_NmeaConfig_t {
uint32_t sentenceConfig;
taf_pa_location_GeodeticDatumType_t datumType;
uint16_t engineType;
};

struct taf_pa_location_TimeInfo_t{
uint32_t validityMask;
uint16_t systemWeek;
uint32_t systemMsec;
float systemClkTimeBias;
float systemClkTimeUncMs;
uint32_t refFCount;
uint32_t numClockResets;
};

struct taf_pa_location_GlonassTimeInfo_t{
uint16_t gloDays;
uint32_t validityMask;
uint32_t gloMsec;
float gloClkTimeBias;
float gloClkTimeUncMs;
uint32_t refFCount;
uint32_t numClockResets;
uint8_t gloFourYear;
};

union taf_pa_location_SystemTimeInfo_t {
taf_pa_location_TimeInfo_t gps;
taf_pa_location_TimeInfo_t gal;
taf_pa_location_TimeInfo_t bds;
taf_pa_location_TimeInfo_t qzss;
taf_pa_location_GlonassTimeInfo_t glo;
taf_pa_location_TimeInfo_t navic;
};

struct taf_pa_location_SystemTime_t {
taf_pa_location_GnssSystem_t gnssSystemTimeSrc;
taf_pa_location_SystemTimeInfo_t time;
};

struct taf_pa_location_GnssMeasurementInfo_t{
taf_pa_location_GnssSignalType_t gnssSignalType;
taf_pa_location_GnssSystem_t gnssConstellation;
uint16_t gnssSvId;
};

struct taf_pa_location_BodyToSensorMountParams_t{
float rollOffset;
float yawOffset;
float pitchOffset;
float offsetUnc;
};

struct taf_pa_location_DREngineConfiguration_t{
uint16_t validMask;
taf_pa_location_BodyToSensorMountParams_t mountParam;
float speedFactor;
float speedFactorUnc;
float gyroFactor;
float gyroFactorUnc;
};

struct taf_pa_location_GnssKinematicsData_t{
taf_pa_location_KinematicDataValidityType_t bodyFrameDataMask;
float longAccel;
float latAccel;
float vertAccel;
float yawRate;
float pitch;
float longAccelUnc;
float latAccelUnc;
float vertAccelUnc;
float yawRateUnc;
float pitchUnc;
float pitchRate;
float pitchRateUnc;
float roll;
float rollUnc;
float rollRate;
float rollRateUnc;
float yaw;
float yawUnc;
};

struct taf_pa_location_SvInfo_t
{
taf_pa_location_GnssConstellationType_t satConst;
bool satUsed;
bool satTracked;
uint8_t satSnr;
uint8_t satElev;
uint16_t satId;
uint16_t satAzim;
uint32_t signalType;
uint16_t glonassFcn;
double baseBandCnr;
};

struct taf_pa_location_LocationSystemInfo_t {
uint32_t validLocationSystemInfoMask;
uint32_t validLeapSecondSysInfoMask;
uint8_t current;
uint8_t leapSecondsBeforeChange;
uint8_t leapSecondsAfterChange;
taf_pa_location_TimeInfo_t timeinfo;
};

struct taf_pa_location_SvUsedInPosition_t
{
uint64_t gps;
uint64_t glo;
uint64_t gal;
uint64_t bds;
uint64_t qzss;
uint64_t navic;
};

struct taf_pa_location_NmeaInfoEvent_t
{
uint64_t timestamp;
std::string nmeaMask;
};

struct taf_pa_location_CapabilityChangeEvent_t
{
taf_pa_location_LocCapabilityType_t locCapability;
};

struct taf_pa_location_RobustLocationVersion_t {
uint8_t major;
uint16_t minor;
};

struct taf_pa_location_RobustLocationConfiguration_t {
uint16_t validMask;
bool enabled;
bool enabledForE911;
taf_pa_location_RobustLocationVersion_t version;
};

typedef enum{
TAF_PA_LOCATION_LEVER_ARM_TYPE_GNSS_TO_VRP = 1,
TAF_PA_LOCATION_LEVER_ARM_TYPE_DR_IMU_TO_GNSS = 2,
TAF_PA_LOCATION_LEVER_ARM_TYPE_VEPP_IMU_TO_GNSS = 3,
TAF_PA_LOCATION_LEVER_ARM_TYPE_VPE_IMU_TO_GNSS = 4,
}taf_pa_location_LeverArmType_t;

struct taf_pa_location_LeverArmParams_t {
float forwardOffset;
float sidewaysOffset;
float upOffset;
uint8_t levArmType;
};

struct taf_pa_location_LocEngineInfo_t {
double   latitude;
double   longitude;
float    altMeanSeaLevel;
uint64_t timeStamp;
float    hUncertainity;
double   altitude;
float    vUncertainity;
float    hSpeed;
float    hSpeedUncertainity;
float    magneticDeviation;
uint64_t epochTime;
float    horUncEllipseSemiMajor;
float    horUncEllipseSemiMinor;
float    direction;
float    directionAccuracy;
uint32_t gpsWeek;
uint32_t gpsTimeOfWeek;
float    timeAccuracy;
float    hdop;
float    vdop;
float    pdop;
float    gdop;
float    tdop;
uint8_t  leapSeconds;
uint8_t leapSecondsUnc;
uint16_t satsUsedCount;
float    robustConformity;
uint8_t  confidencePercent;
double   vrpLatitude;
double   vrpLongitude;
double   vrpAltitude;
double   eastVel;
double   northVel;
double   upVel;
uint64_t gPtpTime;
uint64_t gPtpTimeUnc;
float    azimuth;
float    eastDev;
float    northDev;
uint64_t realTime;
uint64_t realTimeUnc;
std::vector<uint16_t> SVIdData;
std::vector<float> verticalSpeed;
std::vector<float> verticalSpeedAccuracy;
std::vector<taf_pa_location_GnssMeasurementInfo_t> measInfoData;
taf_pa_location_SystemTime_t sysTime;
taf_pa_location_GnssPositionTechType_t posTechnology;
taf_pa_location_AltitudeType_t mAltType;
taf_pa_location_ReportStatus_t reportStatus;
taf_pa_location_DrCalibrationStatusType_t  calibrationStatus;
taf_pa_location_DrSolutionStatusType_t  drSolutionStatus;
taf_pa_location_GnssKinematicsData_t GnssKinematicsData;
taf_pa_location_SvUsedInPosition_t svData;
std::string sbasMask; // If this is a bitmask, consider using uint32_t or uint64_t
taf_pa_location_LocationValidityType_t validityMask;
taf_pa_location_LocationInfoExValidityType_t validityExMask;
taf_pa_location_PositioningEngineType_t engMask;
taf_pa_location_LocationAggregationType_t locationEngType;
taf_pa_location_LocationReliability_t horiReliablity;
taf_pa_location_LocationReliability_t vertReliablity;
taf_pa_location_LocationTechnologyType_t techMask;
};

struct taf_pa_location_GnssSVInfo_t
{
taf_pa_location_GnssConstellationType_t constType;
uint16_t satId;
taf_pa_location_SVInfoAvailability_t hasFix;
float Snr;
float elevation;
float azimuth;
uint32_t signalType;
uint16_t GlonassFcn;
double BasebandCnr;
};

struct taf_pa_location_GnssData_t {
uint32_t gnssDataMask[TAF_PA_GNSS_DATA_ARRAY_SIZE];
double jammerInd[TAF_PA_GNSS_DATA_ARRAY_SIZE];
double agc[TAF_PA_GNSS_DATA_ARRAY_SIZE];
};

typedef enum{
    TAF_PA_LOCATION_SV_ID_BIT                        = (1<<0),
    TAF_PA_LOCATION_SV_TYPE_BIT                      = (1<<1),
    TAF_PA_LOCATION_STATE_BIT                        = (1<<2),
    TAF_PA_LOCATION_RECEIVED_SV_TIME_BIT             = (1<<3),
    TAF_PA_LOCATION_RECEIVED_SV_TIME_UNCERTAINTY_BIT = (1<<4),
    TAF_PA_LOCATION_CARRIER_TO_NOISE_BIT             = (1<<5),
    TAF_PA_LOCATION_PSEUDORANGE_RATE_BIT             = (1<<6),
    TAF_PA_LOCATION_PSEUDORANGE_RATE_UNCERTAINTY_BIT = (1<<7),
    TAF_PA_LOCATION_ADR_STATE_BIT                    = (1<<8),
    TAF_PA_LOCATION_ADR_BIT                          = (1<<9),
    TAF_PA_LOCATION_ADR_UNCERTAINTY_BIT              = (1<<10),
    TAF_PA_LOCATION_CARRIER_FREQUENCY_BIT            = (1<<11),
    TAF_PA_LOCATION_CARRIER_CYCLES_BIT               = (1<<12),
    TAF_PA_LOCATION_CARRIER_PHASE_BIT                = (1<<13),
    TAF_PA_LOCATION_CARRIER_PHASE_UNCERTAINTY_BIT    = (1<<14),
    TAF_PA_LOCATION_MULTIPATH_INDICATOR_BIT          = (1<<15),
    TAF_PA_LOCATION_SIGNAL_TO_NOISE_RATIO_BIT        = (1<<16),
    TAF_PA_LOCATION_AUTOMATIC_GAIN_CONTROL_BIT       = (1<<17),
    TAF_PA_LOCATION_GNSS_SIGNAL_TYPE                 = (1<<18),
    TAF_PA_LOCATION_BASEBAND_CARRIER_TO_NOISE        = (1<<19),
    TAF_PA_LOCATION_FULL_ISB                         = (1<<20),
    TAF_PA_LOCATION_FULL_ISB_UNCERTAINTY             = (1<<21)
}taf_pa_location_GnssMeasurementsDataValidityType_t;

typedef enum{
    TAF_PA_LOCATION_UNKNOWN_BIT                 = 0,
    TAF_PA_LOCATION_CODE_LOCK_BIT               = (1<<0),
    TAF_PA_LOCATION_BIT_SYNC_BIT                = (1<<1),
    TAF_PA_LOCATION_SUBFRAME_SYNC_BIT           = (1<<2),
    TAF_PA_LOCATION_TOW_DECODED_BIT             = (1<<3),
    TAF_PA_LOCATION_MSEC_AMBIGUOUS_BIT          = (1<<4),
    TAF_PA_LOCATION_SYMBOL_SYNC_BIT             = (1<<5),
    TAF_PA_LOCATION_GLO_STRING_SYNC_BIT         = (1<<6),
    TAF_PA_LOCATION_GLO_TOD_DECODED_BIT         = (1<<7),
    TAF_PA_LOCATION_BDS_D2_BIT_SYNC_BIT         = (1<<8),
    TAF_PA_LOCATION_BDS_D2_SUBFRAME_SYNC_BIT    = (1<<9),
    TAF_PA_LOCATION_GAL_E1BC_CODE_LOCK_BIT      = (1<<10),
    TAF_PA_LOCATION_GAL_E1C_2ND_CODE_LOCK_BIT   = (1<<11),
    TAF_PA_LOCATION_GAL_E1B_PAGE_SYNC_BIT       = (1<<12),
    TAF_PA_LOCATION_SBAS_SYNC_BIT               = (1<<13)
}taf_pa_location_GnssMeasurementsStateValidityType_t;

typedef enum {
    TAF_PA_LOCATION_UNKNOWN_STATE   = 0,
    TAF_PA_LOCATION_VALID_BIT       = (1<<0),
    TAF_PA_LOCATION_RESET_BIT       = (1<<1),
    TAF_PA_LOCATION_CYCLE_SLIP_BIT  = (1<<2)
}taf_pa_location_GnssMeasurementsAdrStateValidityType_t;

typedef enum{
    TAF_PA_LOCATION_UNKNOWN_INDICATOR     = 0,
    TAF_PA_LOCATION_PRESENT               = 1,
    TAF_PA_LOCATION_NOT_PRESENT           = 2
}taf_pa_location_GnssMeasurementsMultipathIndicator_t;

typedef enum{
    TAF_PA_LOCATION_LEAP_SECOND_BIT                   = (1<<0),
    TAF_PA_LOCATION_TIME_BIT                          = (1<<1),
    TAF_PA_LOCATION_TIME_UNCERTAINTY_BIT              = (1<<2),
    TAF_PA_LOCATION_FULL_BIAS_BIT                     = (1<<3),
    TAF_PA_LOCATION_BIAS_BIT                          = (1<<4),
    TAF_PA_LOCATION_BIAS_UNCERTAINTY_BIT              = (1<<5),
    TAF_PA_LOCATION_DRIFT_BIT                         = (1<<6),
    TAF_PA_LOCATION_DRIFT_UNCERTAINTY_BIT             = (1<<7),
    TAF_PA_LOCATION_HW_CLOCK_DISCONTINUITY_COUNT_BIT  = (1<<8),
    TAF_PA_LOCATION_ELAPSED_REAL_TIME_BIT             = (1<<9),
    TAF_PA_LOCATION_ELAPSED_REAL_TIME_UNC_BIT         = (1<<10),
    TAF_PA_LOCATION_ELAPSED_GPTP_TIME_BIT             = (1<<11),
    TAF_PA_LOCATION_ELAPSED_GPTP_TIME_UNC_BIT         = (1<<12)
}taf_pa_location_GnssMeasurementsClockValidityType_t;

typedef enum{
  TAF_PA_LOCATION_AGC_UNKNOWN = 0,
  TAF_PA_LOCATION_NO_SATURATION = 1,
  TAF_PA_LOCATION_FRONT_END_GAIN_MAXIMUM_SATURATION = 2,
  TAF_PA_LOCATION_FRONT_END_GAIN_MINIMUM_SATURATION = 3
}taf_pa_location_AgcStatus_t;

typedef enum{
    TAF_PA_LOCATION_AIDING_DATA_EPHEMERIS  = (1 << 0),
    TAF_PA_LOCATION_AIDING_DATA_DR_SENSOR_CALIBRATION = (1 << 1),
}taf_pa_location_AidingDataType_t;

struct taf_pa_location_GnssMeasurementsData_t{
    taf_pa_location_GnssMeasurementsDataValidityType_t valid;
    int16_t svId;
    taf_pa_location_GnssConstellationType_t svType;
    double timeOffsetNs;
    taf_pa_location_GnssMeasurementsStateValidityType_t stateMask;
    int64_t receivedSvTimeNs;
    float receivedSvTimeSubNs;
    int64_t receivedSvTimeUncertaintyNs;
    double carrierToNoiseDbHz;
    double pseudorangeRateMps;
    double pseudorangeRateUncertaintyMps;
    taf_pa_location_GnssMeasurementsAdrStateValidityType_t adrStateMask;
    double adrMeters;
    double adrUncertaintyMeters;
    float carrierFrequencyHz;
    int64_t carrierCycles;
    double carrierPhase;
    double carrierPhaseUncertainty;
    taf_pa_location_GnssMeasurementsMultipathIndicator_t multipathIndicator;
    double signalToNoiseRatioDb;
    double agcLevelDb;
    uint32_t gnssSignalType;
    double basebandCarrierToNoise;
    double fullInterSignalBias;
    double fullInterSignalBiasUncertainty;
};

struct taf_pa_location_GnssMeasurementsClock_t{
    taf_pa_location_GnssMeasurementsClockValidityType_t valid;
    int16_t leapSecond;
    int64_t timeNs;
    double timeUncertaintyNs;
    int64_t fullBiasNs;
    double biasNs;
    double biasUncertaintyNs;
    double driftNsps;
    double driftUncertaintyNsps;
    uint32_t hwClockDiscontinuityCount;
    uint64_t elapsedRealTime;
    uint64_t elapsedRealTimeUnc;
    uint64_t elapsedgPTPTime;
    uint64_t elapsedgPTPTimeUnc;
};

struct taf_pa_location_GnssMeasurements_t{
    taf_pa_location_GnssMeasurementsClock_t clock;
    std::vector<taf_pa_location_GnssMeasurementsData_t> measurements;
    bool isNHz;
    taf_pa_location_AgcStatus_t     agcStatusL1;
    taf_pa_location_AgcStatus_t     agcStatusL2;
    taf_pa_location_AgcStatus_t     agcStatusL5;
};

using taf_pa_location_onLocationSystemInfo = std::function<void(taf_pa_location_LocationId clientId, const std::shared_ptr<taf_pa_location_LocationSystemInfo_t>& LocSysInfo, std::any context)>;
using taf_pa_location_onGnssNmeaInfo = std::function<void(taf_pa_location_LocationId clientId, const std::shared_ptr<taf_pa_location_NmeaInfoEvent_t>& nmeaEventInfo, std::any context)>;
using taf_pa_location_onCapabilitiesInfo = std::function<void(taf_pa_location_LocationId clientId, const std::shared_ptr<taf_pa_location_CapabilityChangeEvent_t>& capEventInfo, std::any context)>;
using taf_pa_location_onGnssSVInfo = std::function<void(taf_pa_location_LocationId clientId, const std::vector<std::shared_ptr<taf_pa_location_GnssSVInfo_t>>& GnssSVInfo, std::any context)>;
using taf_pa_location_onGnssSignalInfo = std::function<void(taf_pa_location_LocationId clientId, const std::shared_ptr<taf_pa_location_GnssData_t>& gnssDatainfo, std::any context)>;
using taf_pa_location_onXtraStatusUpdate = std::function<void(taf_pa_location_LocationId clientId, const std::shared_ptr<taf_pa_location_XtraStatus_t>& xtraStatus, std::any context)>;
using taf_pa_location_onGnssMeasurementsInfo = std::function<void(taf_pa_location_LocationId clientId, const std::shared_ptr<taf_pa_location_GnssMeasurements_t>& measurementInfo, std::any context)>;
using taf_pa_location_onDetailedEngineLocationUpdate = std::function<void(taf_pa_location_LocationId clientId, const std::vector<std::shared_ptr<taf_pa_location_LocEngineInfo_t>>& locationEngineInfo, std::any context)>;

struct taf_pa_location_EventListener{
    taf_pa_location_onCapabilitiesInfo onCapabilitiesInfo;
    taf_pa_location_onGnssNmeaInfo onGnssNmeaInfo;
    taf_pa_location_onLocationSystemInfo onLocationSystemInfo;
    taf_pa_location_onGnssSVInfo onGnssSVInfo;
    taf_pa_location_onGnssSignalInfo onGnssSignalInfo;
    taf_pa_location_onXtraStatusUpdate onXtraStatusUpdate;
    taf_pa_location_onGnssMeasurementsInfo onGnssMeasurementsInfo;
    taf_pa_location_onDetailedEngineLocationUpdate onDetailedEngineLocationUpdate;
};

using taf_pa_location_GeneralCb = std::function<void(pa_result_t result, std::any context)>;

using taf_pa_location_RequestMinSVElevationCb = std::function<void(pa_result_t result, uint8_t* minElevation, std::any context)>;
using taf_pa_location_RequestMinGpsWeekCb = std::function<void(pa_result_t result, uint16_t* minGpsWeek, std::any context)>;
using taf_pa_location_RequestRobustLocationCb = std::function<void(pa_result_t result, const taf_pa_location_RobustLocationConfiguration_t& rLConfig, std::any context)>;
using taf_pa_location_RequestSecondaryBandConfigCb = std::function<void(pa_result_t result, const std::set<taf_pa_location_GnssConstellationType_t>& constellationSet, std::any context)>;
using taf_pa_location_RequestXtraStatusCb = std::function<void(pa_result_t result, const taf_pa_location_XtraStatus_t& xtraStatus, std::any context)>;


PA_SHARED PA_WEAK  pa_result_t taf_pa_location_Init();

PA_SHARED PA_WEAK  taf_pa_location_LocationId taf_pa_location_CreateClient();
PA_SHARED PA_WEAK  pa_result_t taf_pa_location_DeleteClient(taf_pa_location_LocationId clientId);

PA_SHARED PA_WEAK  pa_result_t taf_pa_location_RegisterListener(taf_pa_location_LocationId clientId, taf_pa_location_EventListener* eventListener, std::any context);
PA_SHARED PA_WEAK  pa_result_t taf_pa_location_startDetailedEngineReports(taf_pa_location_LocationId clientId, uint32_t optInterval, uint16_t engineType, taf_pa_location_GeneralCb callback, uint32_t reportMask, std::any context);
PA_SHARED PA_WEAK  pa_result_t taf_pa_location_stopReports(taf_pa_location_LocationId clientId, taf_pa_location_GeneralCb callback, std::any context);
PA_SHARED PA_WEAK  uint32_t taf_pa_location_getCapabilities(taf_pa_location_LocationId clientId, std::any context);


PA_SHARED PA_WEAK  pa_result_t taf_pa_location_configureConstellations(const std::vector<taf_pa_location_SvBlackListInfo_t>& svBlackListData, taf_pa_location_GeneralCb callback, bool deviceReset, std::any context);
PA_SHARED PA_WEAK  pa_result_t taf_pa_location_deleteAidingData(taf_pa_location_AidingDataType_t aidingData, taf_pa_location_GeneralCb callback, std::any context);
PA_SHARED PA_WEAK  pa_result_t taf_pa_location_deleteAllAidingData(taf_pa_location_GeneralCb callback, std::any context);
PA_SHARED PA_WEAK  pa_result_t taf_pa_location_configureMinSVElevation(uint8_t minElevation, taf_pa_location_GeneralCb callback, std::any context);
PA_SHARED PA_WEAK  pa_result_t taf_pa_location_requestMinSVElevation(taf_pa_location_RequestMinSVElevationCb callback, std::any context);
PA_SHARED PA_WEAK  pa_result_t taf_pa_location_configureNmeaTypes(taf_pa_location_NmeaSentenceType_t nmeaType, taf_pa_location_GeneralCb callback, std::any context);
PA_SHARED PA_WEAK  pa_result_t taf_pa_location_configureDR(const taf_pa_location_DREngineConfiguration_t& drConfig, taf_pa_location_GeneralCb callback, std::any context);
PA_SHARED PA_WEAK  pa_result_t taf_pa_location_configureEngineState(taf_pa_location_EngineType_t engineType, taf_pa_location_LocationEngineRunState_t engineState, taf_pa_location_GeneralCb callback, std::any context);
PA_SHARED PA_WEAK  pa_result_t taf_pa_location_configureRobustLocation(bool enableRobustloc, bool enableE911loc, taf_pa_location_GeneralCb callback, std::any context);
PA_SHARED PA_WEAK  pa_result_t taf_pa_location_requestRobustLocation(taf_pa_location_RequestRobustLocationCb callback, std::any context);
PA_SHARED PA_WEAK  pa_result_t taf_pa_location_configureSecondaryBand(const std::unordered_set<taf_pa_location_GnssConstellationType_t>& constSet, taf_pa_location_GeneralCb callback, std::any context);
PA_SHARED PA_WEAK  pa_result_t taf_pa_location_requestSecondaryBandConfig(taf_pa_location_RequestSecondaryBandConfigCb callback, std::any context);
PA_SHARED PA_WEAK  pa_result_t taf_pa_location_configureLeverArm(const taf_pa_location_LeverArmParams_t* leverArmConfigInfoPtr, taf_pa_location_GeneralCb callback, std::any context);
PA_SHARED PA_WEAK  pa_result_t taf_pa_location_configureMinGpsWeek(uint16_t minGpsWeek, taf_pa_location_GeneralCb callback, std::any context);
PA_SHARED PA_WEAK  pa_result_t taf_pa_location_configureNmea(const taf_pa_location_NmeaConfig_t& nmeaConfigData, taf_pa_location_GeneralCb callback, std::any context);
PA_SHARED PA_WEAK  pa_result_t taf_pa_location_requestMinGpsWeek(taf_pa_location_RequestMinGpsWeekCb callback, std::any context);
PA_SHARED PA_WEAK  pa_result_t taf_pa_location_requestXtraStatus(taf_pa_location_RequestXtraStatusCb callback, std::any context);
} // namespace tafpa::location

#endif /* TAF_PA_LOCATION_H */
