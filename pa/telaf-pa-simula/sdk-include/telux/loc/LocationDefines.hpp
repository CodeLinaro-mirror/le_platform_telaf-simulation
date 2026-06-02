/*
 *  Copyright (c) 2017-2021, The Linux Foundation. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *    * Neither the name of The Linux Foundation nor the names of its
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 *  ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 *  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 *  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 *  OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 *  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Changes from Qualcomm Technologies, Inc. are provided under the following license:
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file       LocationDefines.hpp
 *
 * @brief      LocationDefines contains types related to location services.
 *
 */

#include <memory>
#include <vector>
#include <bitset>
#include <unordered_map>
#include <unordered_set>

#include "telux/common/CommonDefines.hpp"

#ifndef TELUX_LOC_LOCATIONDEFINES_HPP
#define TELUX_LOC_LOCATIONDEFINES_HPP

namespace telux {

namespace loc {
/** @addtogroup telematics_location
* @{ */

const float UNKNOWN_CARRIER_FREQ = -1;
const int UNKNOWN_SIGNAL_MASK = 0;
const double UNKNOWN_BASEBAND_CARRIER_NOISE = 0.0;
const uint64_t UNKNOWN_TIMESTAMP = 0;
const float DEFAULT_TUNC_THRESHOLD = 0.0; /**< Default value for threshold of time uncertainty.
                                               Units: milli-seconds. */
const int DEFAULT_TUNC_ENERGY_THRESHOLD = 0; /**< Default value for energy consumed of time
                                                  uncertainty. The default here means that the
                                                  engine is allowed to use infinite power.
                                                  Units: 100 micro watt second. */
const uint64_t INVALID_ENERGY_CONSUMED = 0xffffffffffffffff; /**< 0xffffffffffffffff indicates an
                                                                  invalid reading for energy
                                                                  consumed info. */
const float UNKNOWN_SV_TIME_SUB_NS = -1; /**< Unknown Sub nanoseconds portion of the received GNSS
                                              time. */
/**
 * Defines RTCM injection data format
 */
enum class DgnssDataFormat{
  /** Source data format is unknown */
  DATA_FORMAT_UNKNOWN                = 0,
  /** Source data format is RTCM_3 */
  DATA_FORMAT_RTCM_3                 = 1,
  /** Source data format is 3GPP RTK Rel-15 */
  DATA_FORMAT_3GPP_RTK_R15           = 2
};

/**
 * Defines status reported by cdfw for RTCM injection.
 */
enum class DgnssStatus{
  /** Dgnss subsystem doesn't support the data source */
  DATA_SOURCE_NOT_SUPPORTED          = 1,
  /** Dgnss subsystem doesn't support the data format */
  DATA_FORMAT_NOT_SUPPORTED          = 2,
  /** After the source injects the data, dgnss subsystem discovers there is
   *  another higher priority source injecting the data at the
   *  same time, and the current injected data is dropped */
  OTHER_SOURCE_IN_USE                = 3,
  /** There is a parsing error such as unrecognized format, CRC
   *  check failure, value range check failure, etc.; the injected
   *  data is dropped */
  MESSAGE_PARSE_ERROR                = 4,
  /** Data source is usable */
  DATA_SOURCE_USABLE                 = 5,
  /** Data source is not usable, for example,
   * the reference station is too far away to improve the potion accuracy */
  DATA_SOURCE_NOT_USABLE             = 6,
  /** The CDFW service askes the source client to stop
   *  injecting the correction data */
  CDFW_STOP_SOURCE_INJECT            = 7
};

/**
 * Defines the horizontal accuracy level of the fix.
 */
enum class HorizontalAccuracyLevel {
  LOW = 1,        /**< Client requires low horizontal accuracy */
  MEDIUM = 2, /**< Client requires medium horizontal accuracy */
  HIGH = 3    /**< Client requires high horizontal accuracy */
};

/**
 * Specifies the reliability of the position.
 */
enum class LocationReliability {
  UNKNOWN = -1, /**< Unknown location reliability*/
  NOT_SET = 0, /**<  Location reliability is not set. The reliability of this position report
                     could not be determined. It could be unreliable/reliable */
  VERY_LOW = 1, /**<  Location reliability is very low */
  LOW = 2, /**<  Location reliability is low, little or no cross-checking is possible */
  MEDIUM = 3, /**<  Location reliability is medium, limited cross-check passed */
  HIGH = 4 /**<  Location reliability is high, strong cross-check passed */
};

/**
 * Specify the set of navigation solutions that contribute to the Gnss Location.
 */
enum NavigationSolutionType {
    NAV_SBAS_SOLUTION_IONO,       /**< Bit mask to specify whether
                                        SBAS ionospheric solution is used */
    NAV_SBAS_SOLUTION_FAST,       /**< Bit mask to specify whether
                                        SBAS fast solution is used */
    NAV_SBAS_SOLUTION_LONG,       /**< Bit mask to specify whether
                                        SBAS long solution is used */
    NAV_SBAS_INTEGRITY,             /**< Bit mask to specify whether
                                        SBAS integrity information is used */
    NAV_DGNSS_SOLUTION,           /**< Bit mask to specify whether
                                        DGNSS solution is used */
    NAV_RTK_SOLUTION,             /**< Bit mask to specify whether
                                        RTK solution is used */
    NAV_PPP_SOLUTION,             /**< Bit mask to specify whether
                                        PPP solution is used */
    NAV_RTK_FIXED_SOLUTION,       /**< Bit mask to specify whether RTK fixed solution is used.
                                        If only solution RTK is set,
                                        fixes shall be treated as RTK_FLOAT solution.
                                        If both solutions RTK & RTK_FIXED are set,
                                        fixes shall be treated as RTK_FIXED solution. */
    NAV_ONLY_SBAS_CORRECTED_SV_USED,   /**< Bit mask to specify
                                           only SBAS corrected SV is used */
    NAV_COUNT  /**< Bitset */
};

/**
 * Bit mask to denote the corrections in NavigationSolutionType that are used
 * to improve the performance of GNSS output.
 */
using NavigationSolution = std::bitset<NAV_COUNT>;

/**
 * Indicates whether altitude is assumed or calculated.
 */
enum class AltitudeType {
  UNKNOWN = -1, /**< Unknown altitude type*/
  CALCULATED = 0, /**< Altitude is calculated  */
  ASSUMED = 1, /**< Altitude is assumed, there may not be enough
                    satellites to determine the precise altitude */
};

/**
 * Defines constellation type of GNSS.
 */
enum class GnssConstellationType {
  UNKNOWN = -1, /**< Unknown constellation type*/
  GPS = 1, /**< GPS satellite */
  GALILEO = 2, /**< GALILEO satellite */
  SBAS = 3, /**< SBAS satellite */
  COMPASS = 4, /**< COMPASS satellite.
               @deprecated constellation type is not supported.*/
  GLONASS = 5, /**< GLONASS satellite */
  BDS = 6, /**< BDS satellite */
  QZSS = 7, /**< QZSS satellite */
  NAVIC = 8 /**< NAVIC satellite*/
};

/**
 * Health status indicates whether satellite is operational or not.
 * This information comes from the most recent data transmitted in satellite
 * almanacs.
 */
enum class SVHealthStatus {
  UNKNOWN = -1, /**< Unknown sv health status*/
  UNHEALTHY = 0, /**< satellite is not operational and cannot be
                      used in position calculations */
  HEALTHY = 1 /**< satellite is fully operational */
};

/**
 * Satellite vehicle processing status.
 */
enum class SVStatus {
  UNKNOWN = -1, /**< Unknown sv status*/
  IDLE = 0, /**< SV is not being actively processed  */
  SEARCH = 1, /**< The system is searching for this SV */
  TRACK = 2 /**< SV is being tracked */
};

/**
 * Indicates whether Satellite Vehicle info like ephemeris and
 * almanac are present or not
 */
enum class SVInfoAvailability {
  UNKNOWN = -1, /**< Unknown sv info availability*/
  YES = 0, /**< Ephemeris or Almanac exits  */
  NO = 1 /**< Ephemeris or Almanac doesn't exist */
};

/**
 * Specifies which position technology was used to generate location
 * information in the @ref ILocationInfoEx.
 */
enum GnssPositionTechType {
  /** Technology used to generate location info
   *  is unknown.*/
  GNSS_DEFAULT = 0,
  /** Satellites-based technology was used to generate
   *  location info.*/
  GNSS_SATELLITE = (1 << 0),
  /** Cell towers were used to generate location info.*/
  GNSS_CELLID = (1 << 1),
  /** Wi-Fi access points were used to generate location info.*/
  GNSS_WIFI = (1 << 2),
  /** Sensors were used to generate location info.*/
  GNSS_SENSORS = (1 << 3),
  /**  Reference location was used to generate location info.*/
  GNSS_REFERENCE_LOCATION = (1 << 4),
  /** Coarse position injected into the location engine was used to
   *  generate location info.*/
  GNSS_INJECTED_COARSE_POSITION= (1 << 5),
  /** AFLT was used to generate location info.*/
  GNSS_AFLT = (1 << 6),
  /** GNSS and network-provided measurements were used to generate
   *  location info.*/
  GNSS_HYBRID = (1 << 7),
  /** Precise position engine was used to generate location info.*/
  GNSS_PPE = (1 << 8),
  /** Location was calculated using Vehicular data. */
  GNSS_VEHICLE = (1 << 9),
  /** Location was calculated using Visual data. */
  GNSS_VISUAL = (1 << 10),
  /** Location was calculated using Propagation logic, which uses cached measurements. */
  GNSS_PROPAGATED = (1 << 11),
};

/*Bit mask containing bits from GnssPositionTechType */
using GnssPositionTech = uint32_t;

/**
 * Specifies related kinematics mask
 */
enum KinematicDataValidityType {
  /** Navigation data has Forward Acceleration  */
  HAS_LONG_ACCEL = (1 << 0),
  /** Navigation data has Sideward Acceleration */
  HAS_LAT_ACCEL = (1 << 1),
  /** Navigation data has Vertical Acceleration */
  HAS_VERT_ACCEL = (1 << 2),
  /** Navigation data has Heading Rate */
  HAS_YAW_RATE = (1 << 3),
  /** Navigation data has Body pitch */
  HAS_PITCH = (1 << 4),
  /** Navigation data has Forward Acceleration  */
  HAS_LONG_ACCEL_UNC = (1 << 5),
  /** Navigation data has Sideward Acceleration */
  HAS_LAT_ACCEL_UNC = (1 << 6),
  /** Navigation data has Vertical Acceleration */
  HAS_VERT_ACCEL_UNC = (1 << 7),
  /** Navigation data has Heading Rate */
  HAS_YAW_RATE_UNC = (1 << 8),
  /** Navigation data has Body pitch */
  HAS_PITCH_UNC = (1 << 9),
  /** Navigation data has Body pitch rate */
  HAS_PITCH_RATE_BIT = (1<<10),
  /** Navigation data has Body pitch rate uncertainty */
  HAS_PITCH_RATE_UNC_BIT = (1<<11),
  /** Navigation data has roll */
  HAS_ROLL_BIT = (1<<12),
  /** Navigation data has roll uncertainty */
  HAS_ROLL_UNC_BIT = (1<<13),
  /** Navigation data has roll rate */
  HAS_ROLL_RATE_BIT = (1<<14),
  /** Navigation data has roll rate uncertainty */
  HAS_ROLL_RATE_UNC_BIT = (1<<15),
  /** Navigation data has yaw */
  HAS_YAW_BIT = (1<<16),
  /** Navigation data has yaw uncertainty */
  HAS_YAW_UNC_BIT = (1<<17)
};

/*Bit mask containing bits from KinematicDataValidityType */
using KinematicDataValidity = uint32_t;

/**
 * Specifies kinematics related information related to device
 * body frame parameters.
 */
struct GnssKinematicsData {
  /** Contains Body frame data valid bits. */
  KinematicDataValidity bodyFrameDataMask;
  /** Forward Acceleration in body frame (meters/second^2)*/
  float longAccel;
  /** Sideward Acceleration in body frame (meters/second^2)*/
  float latAccel;
  /** Vertical Acceleration in body frame (meters/second^2)*/
  float vertAccel;
  /** Heading Rate (Radians/second) */
  float yawRate;
  /** Body pitch (Radians) */
  float pitch;
  /** Uncertainty of Forward Acceleration in body
   *  frame (meters/second^2)
   *  Uncertainty is defined with 68% confidence level. */
  float longAccelUnc;
  /** Uncertainty of Side-ward Acceleration in body
   *  frame meters/second^2)
   *  Uncertainty is defined with 68% confidence level. */
  float latAccelUnc;
  /** Uncertainty of Vertical Acceleration in body
   *  frame (meters/second^2)
   *  Uncertainty is defined with 68% confidence level. */
  float vertAccelUnc;
  /** Uncertainty of Heading Rate (Radians/second)
   *  Uncertainty is defined with 68% confidence level. */
  float yawRateUnc;
  /** Uncertainty of Body pitch (Radians)
   *  Uncertainty is defined with 68% confidence level. */
  float pitchUnc;
  /** Body pitch rate, in unit of radians/second.*/
  float pitchRate;
  /** Uncertainty of pitch rate, in unit of radians/second.
   *  Uncertainty is defined with 68% confidence level. */
  float pitchRateUnc;
  /** Roll of body frame, clockwise is positive, in unit of radian. */
  float roll;
  /** Uncertainty of roll, in unit of radian.
   *  Uncertainty is defined with 68% confidence level. */
  float rollUnc;
  /** Roll rate of body frame, clockwise is positive, in unit of
   *  radian/second.*/
  float rollRate;
  /** Uncertainty of roll rate, in unit of radian/second.
   *  Uncertainty is defined with 68% confidence level. */
  float rollRateUnc;
  /** Yaw of body frame, clockwise is positive, in unit of radian. */
  float yaw;
  /** Uncertainty of yaw, in unit of radian.
   *  Uncertainty is defined with 68% confidence level. */
  float yawUnc;
};

/**
 * The location info is calculated according to the vehicle's GNSS antenna where as Vehicle
 * Reference Point(VRP) refers to a point on the vehicle where the display of the car sits.
 * The VRP based info is calculated by adding that extra difference between GNSS antenna and
 * the VRP on the top where the location info is recieved. The VRP parameters can be configured
 * through @ref ILocationConfigurator::configureLeverArm.
 * LLAInfo specifies latitude, longitude and altitude info of location for VRP-based.
 */
struct LLAInfo {
  /** Latitude, in unit of degrees, range [-90.0, 90.0]. */
  double latitude;
  /** Longitude, in unit of degrees, range [-180.0, 180.0]. */
  double longitude;
  /** Altitude above the WGS 84 reference ellipsoid, in unit of meters. */
  float altitude;
};

/**
 * Specify the different types of constellation supported.
 */
enum class GnssSystem {
  /** UNKNOWN satellite. */
  GNSS_LOC_SV_SYSTEM_UNKNOWN = -1,
  /** GPS satellite. */
  GNSS_LOC_SV_SYSTEM_GPS = 1,
  /** GALILEO satellite. */
  GNSS_LOC_SV_SYSTEM_GALILEO = 2,
  /** SBAS satellite. */
  GNSS_LOC_SV_SYSTEM_SBAS = 3,
  /** COMPASS satellite.
  @deprecated constellation type
  is not supported.*/
  GNSS_LOC_SV_SYSTEM_COMPASS = 4,
  /** GLONASS satellite. */
  GNSS_LOC_SV_SYSTEM_GLONASS = 5,
  /** BDS satellite. */
  GNSS_LOC_SV_SYSTEM_BDS = 6,
  /** QZSS satellite. */
  GNSS_LOC_SV_SYSTEM_QZSS = 7,
  /** NAVIC satellite. */
  GNSS_LOC_SV_SYSTEM_NAVIC = 8
};

/**
 * Validity field for different system time in struct TimeInfo.
 */
enum GnssTimeValidityType {
  /** valid systemWeek.*/
  GNSS_SYSTEM_TIME_WEEK_VALID = (1 << 0),
  /** valid systemMsec*/
  GNSS_SYSTEM_TIME_WEEK_MS_VALID = (1 << 1),
  /** valid systemClkTimeBias*/
  GNSS_SYSTEM_CLK_TIME_BIAS_VALID = (1 << 2),
  /** valid systemClkTimeUncMs*/
  GNSS_SYSTEM_CLK_TIME_BIAS_UNC_VALID = (1 << 3),
  /** valid refFCount*/
  GNSS_SYSTEM_REF_FCOUNT_VALID = (1 << 4),
  /** valid numClockResets*/
  GNSS_SYSTEM_NUM_CLOCK_RESETS_VALID = (1 << 5)
};

/*Bit mask containing bits from GnssTimeValidityType */
using GnssTimeValidity = uint32_t;

/** Specify non-Glonass Gnss system time info.*/
struct TimeInfo {
  /** Validity mask for below fields */
  GnssTimeValidity validityMask;
  /** Extended week number at reference tick.
   *  Unit: Week.
   *  Set to 65535 if week number is unknown.
   *  For GPS:
   *  Calculated from midnight, Jan. 6, 1980.
   *  OTA decoded 10 bit GPS week is extended to map between:
   *  [NV6264 to (NV6264 + 1023)].
   *  For BDS:
   *  Calculated from 00:00:00 on January 1, 2006 of Coordinated Universal Time
   *  (UTC).
   *  For GAL:
   *  Calculated from 00:00 UT on Sunday August 22, 1999
   *  (midnight between August 21 and August 22).*/
  uint16_t systemWeek;
  /** Time in to the current week at reference tick.
   *  Unit: Millisecond. Range: 0 to 604799999.*/
  uint32_t systemMsec;
  /** System clock time bias
   *  Units: Millisecond
   *  Note: System time (TOW Millisecond) = systemMsec - systemClkTimeBias.*/
  float systemClkTimeBias;
  /** Single sided maximum time bias uncertainty
   *  Units: Millisecond */
  float systemClkTimeUncMs;
  /** FCount (free running HW timer) value. Don't use for relative time purpose
   *  due to possible discontinuities.
   *  Unit: Millisecond */
  uint32_t refFCount;
  /** Number of clock resets/discontinuities detected,
   *  affecting the local hardware counter value. */
  uint32_t numClockResets;
};

/**
 * Validity field for GLONASS time in struct GlonassTimeInfo.
 */
enum GlonassTimeValidity {
  /** valid gloDays*/
  GNSS_CLO_DAYS_VALID = (1 << 0),
  /** valid gloMsec*/
  GNSS_GLOS_MSEC_VALID = (1 << 1),
  /** valid gloClkTimeBias*/
  GNSS_GLO_CLK_TIME_BIAS_VALID = (1 << 2),
  /** valid gloClkTimeUncMs*/
  GNSS_GLO_CLK_TIME_BIAS_UNC_VALID = (1 << 3),
  /** valid refFCount*/
  GNSS_GLO_REF_FCOUNT_VALID = (1 << 4),
  /** valid numClockResets*/
  GNSS_GLO_NUM_CLOCK_RESETS_VALID = (1 << 5),
  /** valid gloFourYear*/
  GNSS_GLO_FOUR_YEAR_VALID = (1 << 6)
};

/*Bit mask containing bits from GlonassTimeValidity */
using TimeValidity = uint32_t;

/** Specifies Glonass system time info.*/
struct GlonassTimeInfo {
  /** GLONASS day number in four years. Refer to GLONASS ICD.
   *  Applicable only for GLONASS and shall be ignored for other constellations.
   *  If unknown shall be set to 65535 */
  uint16_t gloDays;
  /** Validity mask for GlonassTimeInfo fields */
  TimeValidity validityMask;
  /** GLONASS time of day in Millisecond. Refer to GLONASS ICD.
   *  Units: Millisecond.*/
  uint32_t gloMsec;
  /** GLONASS clock time bias.
   *  Units: Millisecond
   *  Note: GLO time (TOD Millisecond) = gloMsec - gloClkTimeBias.
   *  Check for gloClkTimeUncMs before use. */
  float gloClkTimeBias;
  /** Single sided maximum time bias uncertainty
   *  Units: Millisecond */
  float gloClkTimeUncMs;
  /** FCount (free running HW timer) value. Don't use for relative time purpose
   *  due to possible discontinuities.
   *  Unit: Millisecond */
  uint32_t refFCount;
  /** Number of clock resets/discontinuities detected,
   *  affecting the local hardware counter value. */
  uint32_t numClockResets;
  /** GLONASS four year number from 1996. Refer to GLONASS ICD.
   *  Applicable only for GLONASS and shall be ignored for other constellations.*/
  uint8_t gloFourYear;
};

/** Union to hold GNSS system time from different constellations in
 *  SystemTime.*/
union SystemTimeInfo {
  /** System time info from GPS constellation.*/
  TimeInfo gps;
  /** System time info from GALILEO constellation.*/
  TimeInfo gal;
  /** System time info from BEIDOU constellation.*/
  TimeInfo bds;
  /** System time info from QZSS constellation.*/
  TimeInfo qzss;
  /** System time info from GLONASS constellation.*/
  GlonassTimeInfo glo;
  /** System time info from NAVIC constellation.*/
  TimeInfo navic;
};

/** GNSS system time in @ref ILocationInfoEx.*/
struct SystemTime {
  /** Specify the source constellation for GNSS system time. */
  GnssSystem gnssSystemTimeSrc;
  /** Specify the GNSS system time corresponding to the source.*/
  SystemTimeInfo time;
};

/** Specify GNSS Signal Type and RF Band used in struct GnssMeasurementInfo
 *  and ISVInfo class.*/
enum GnssSignalType {
  /** Gnss signal is of GPS L1CA RF Band. */
  GPS_L1CA = (1<<0),
  /** Gnss signal is of GPS L1C RF Band. */
  GPS_L1C = (1<<1),
  /** Gnss signal is of GPS L2 RF Band. */
  GPS_L2 = (1<<2),
  /** Gnss signal is of GPS L5 RF Band. */
  GPS_L5 = (1<<3),
  /** Gnss signal is of GLONASS G1 (L1OF) RF Band. */
  GLONASS_G1 = (1<<4),
  /** Gnss signal is of GLONASS G2 (L2OF) RF Band. */
  GLONASS_G2 = (1<<5),
  /** Gnss signal is of GALILEO E1 RF Band. */
  GALILEO_E1 = (1<<6),
  /** Gnss signal is of GALILEO E5A RF Band. */
  GALILEO_E5A = (1<<7),
  /** Gnss signal is of GALILEO E5B RF Band. */
  GALILIEO_E5B = (1<<8),
  /** Gnss signal is of BEIDOU B1 RF Band. */
  BEIDOU_B1 = (1<<9),
  /** Gnss signal is of BEIDOU B2 RF Band. */
  BEIDOU_B2 = (1<<10),
  /** Gnss signal is of QZSS L1CA RF Band. */
  QZSS_L1CA = (1<<11),
  /** Gnss signal is of QZSS L1S RF Band. */
  QZSS_L1S = (1<<12),
  /** Gnss signal is of QZSS L2 RF Band. */
  QZSS_L2 = (1<<13),
  /** Gnss signal is of QZSS L5 RF Band. */
  QZSS_L5 = (1<<14),
  /** Gnss signal is of SBAS L1 RF Band. */
  SBAS_L1 = (1<<15),
  /** Gnss signal is of BEIDOU B1I RF Band. */
  BEIDOU_B1I = (1<<16),
  /** Gnss signal is of BEIDOU B1C RF Band. */
  BEIDOU_B1C = (1<<17),
  /** Gnss signal is of BEIDOU B2I RF Band. */
  BEIDOU_B2I = (1<<18),
  /** Gnss signal is of BEIDOU B2AI RF Band. */
  BEIDOU_B2AI = (1<<19),
  /** Gnss signal is of NAVIC L5 RF Band. */
  NAVIC_L5 = (1<<20),
  /** Gnss signal is of BEIDOU B2A_Q RF Band. */
  BEIDOU_B2AQ = (1<<21),
  /** Gnss signal is of BEIDOU B2B_I RF Band. */
  BEIDOU_B2BI = (1<<22),
  /** Gnss signal is of BEIDOU B2B_Q RF Band. */
  BEIDOU_B2BQ = (1<<23),
  /** Gnss signal is of NAVIC L1 RF Band. */
  NAVIC_L1 = (1<<24)
};

/*Bit mask containing bits from GnssSignalType */
using GnssSignal = uint32_t;

/** Specify Location Capabilities Type.*/
enum LocCapabilityType {
  /** Support time based tracking session via @ref ILocationManager::startDetailedReports,
   *  @ref ILocationManager::startDetailedEngineReports and
   *  @ref ILocationManager::startBasicReports with distanceInMeters set to 0.
   */
  TIME_BASED_TRACKING = (1<<0),
  /** Support distance based tracking session via @ref ILocationManager::startBasicReports with
   *  distanceInMeters specified.
   */
  DISTANCE_BASED_TRACKING = (1<<1),
  /** Support Gnss Measurement data via @ref ILocationListener::onGnssMeasurementsInfo when a
   *  tracking session is enabled.
   */
  GNSS_MEASUREMENTS = (1<<2),
  /** Support configure constellations via @ref ILocationConfigurator::configureConstellations. */
  CONSTELLATION_ENABLEMENT = (1<<3),
  /** Support carrier phase for Precise Positioning Measurement Engine (PPME). */
  CARRIER_PHASE = (1<<4),
  /** Support GNSS Single Frequency feature. */
  QWES_GNSS_SINGLE_FREQUENCY = (1<<5),
  /** Supports GNSS Multi Frequency feature. */
  QWES_GNSS_MULTI_FREQUENCY = (1<<6),
  /** Support VEPP license bundle is enabled. VEPP bundle include Carrier Phase features. */
  QWES_VPE = (1<<7),
  /** Support for CV2X Location basic features. This includes features for
   *  GTS Time & Freq, @ref ILocationConfigurator::configureCTunc.
   */
  QWES_CV2X_LOCATION_BASIC = (1<<8),
  /** Support for CV2X Location premium features. This includes features for
   *  CV2X Location Basic features, QDR3 feature and @ref ILocationConfigurator::configurePACE.
   */
  QWES_CV2X_LOCATION_PREMIUM = (1<<9),
  /** Support PPE (Precise Positioning Engine) library is enabled or Precise Positioning Framework
   *  (PPF) is available. This includes features for Carrier Phase and SV Ephermeris.
   */
  QWES_PPE = (1<<10),
  /** Support QDR2_C license bundle is enabled. */
  QWES_QDR2 = (1<<11),
  /** Support QDR3_C license bundle is enabled. */
  QWES_QDR3 = (1<<12),
  /** support time-based batching session. */
  TIME_BASED_BATCHING = (1<<13),
  /** support distance-based batching session. */
  DISTANCE_BASED_BATCHING = (1<<14),
  /** Support geofencing. */
  GEOFENCE = (1<<15),
  /** Support outdoor trip batching session. */
  OUTDOOR_TRIP_BATCHING = (1<<16),
  /** Support SV Polynomial */
  SV_POLYNOMIAL = (1<<17),
  /** Indicates presence of ML Inference capability for Pseudo Range Measurements. */
  NLOS_ML20 = (1<<18)
};

/*Bit mask containing bits from LocCapabilityType */
using LocCapability = uint32_t;

/** Specify the satellite vehicle measurements that are used
 *  to calculate location in @ref ILocationInfoEx.*/
struct GnssMeasurementInfo {
  /** GnssSignalType mask */
  GnssSignal gnssSignalType;
  /** Specifies GNSS Constellation Type */
  GnssSystem gnssConstellation;
  /** GNSS SV ID.
   *  For GPS:      1 to 32.
   *  For GLONASS:  [65, 96] or [97, 110].
                    [65, 96] if orbital slot number(OSN) is known.
                    [97, 110] as frequency channel number(FCN) [-7, 6] plus 104.
                    i.e. encode FCN (-7) as 97, FCN (0) as 104, FCN (6) as 110.
   *  For SBAS:     120 to 158 and 183 to 191.
   *  For QZSS:     193 to 197.
   *  For BDS:      201 to 263.
   *  For GAL:      301 to 336.
   *  For NAVIC:    401 to 420.*/
  uint16_t gnssSvId;
};

/** Specify the set of SVs that are used to calculate
 *  location in @ref ILocationInfoEx.*/
struct SvUsedInPosition {
    /** Specify the set of SVs from GPS constellation that are used
     *  to compute the position. Bit 0 to Bit 31 corresponds
     *  to GPS SV id 1 to 32.*/
    uint64_t gps;
    /** Specify the set of SVs from GLONASS constellation that are
     *  used to compute the position.
     *  Bit 0 to Bit 31 corresponds to GLO SV id 65 to 96.*/
    uint64_t glo;
    /** Specify the set of SVs from GALILEO constellation that are
     *  used to compute the position.
     *  Bit 0 to Bit 35 corresponds to GAL SV id 301 to 336.*/
    uint64_t gal;
    /** Specify the set of SVs from BEIDOU constellation that are
     *  used to compute the position.
     *  Bit 0 to Bit 62 corresponds to BDS SV id 201 to 263.*/
    uint64_t bds;
    /** Specify the set of SVs from QZSS constellation that are used
     *  to compute the position.
     *  Bit 0 to Bit 4 corresponds to QZSS SV id 193 to 197.*/
    uint64_t qzss;
    /** Specify the set of SVs from NAVIC constellation that are used
     *  to compute the position.
     *  Bit 0 to Bit 19 corresponds to NAVIC SV id 401 to 420.*/
    uint64_t navic;
};

/** Specify the set of technologies that contribute to @ref
 *  ILocationInfoBase.
 */
enum LocationTechnologyType {
  /** Location was calculated using GNSS-based technology. */
  LOC_GNSS = (1 << 0),
  /** Location was calculated using Cell-based technology. */
  LOC_CELL = (1 << 1),
  /** Location was calculated using WiFi-based technology. */
  LOC_WIFI = (1 << 2),
  /** Location was calculated using Sensors-based technology. */
  LOC_SENSORS = (1 << 3),
  /** Location was calculated using Reference location. */
  LOC_REFERENCE_LOCATION = (1 << 4),
  /** Location was calculated using Coarse position injected into the location engine. */
  LOC_INJECTED_COARSE_POSITION = (1 << 5),
  /** Location was calculated using AFLT. */
  LOC_AFLT = (1 << 6),
  /** Location was calculated using GNSS and network-provided measurements. */
  LOC_HYBRID = (1 << 7),
  /** Location was calculated using Precise position engine. */
  LOC_PPE = (1 << 8),
  /** Location was calculated using Vehicular data. */
  LOC_VEH = (1 << 9),
  /** Location was calculated using Visual data. */
  LOC_VIS = (1 << 10),
  /** Location was calculated using Propagation logic, which uses cached measurements. */
  LOC_PROPAGATED = (1 << 11),
};

/*Bit mask containing bits from LocationTechnologyType */
using LocationTechnology = uint32_t;

/** Specify the valid fields in LocationInfoValidity
 *  User should determine whether a field in LocationInfoValidity
 *  is valid or not by checking the corresponding bit is set or not.
 */
enum LocationValidityType {
  /** Location has valid latitude and longitude.*/
    HAS_LAT_LONG_BIT          = (1<<0),
    /** Location has valid altitude.*/
    HAS_ALTITUDE_BIT          = (1<<1),
    /** Location has valid speed.*/
    HAS_SPEED_BIT             = (1<<2),
    /** Location has valid heading.*/
    HAS_HEADING_BIT           = (1<<3),
    /* Location has valid horizontal accuracy. */
    HAS_HORIZONTAL_ACCURACY_BIT = (1<<4),
    /** Location has valid vertical accuracy.*/
    HAS_VERTICAL_ACCURACY_BIT = (1<<5),
    /** Location has valid speed accuracy.*/
    HAS_SPEED_ACCURACY_BIT    = (1<<6),
    /** Location has valid heading accuracy.*/
    HAS_HEADING_ACCURACY_BIT  = (1<<7),
    /** Location has valid timestamp.*/
    HAS_TIMESTAMP_BIT         = (1<<8),
    /** Location has valid elapsed real time.*/
    HAS_ELAPSED_REAL_TIME_BIT = (1<<9),
    /** Location has valid elapsed real time uncertainty.*/
    HAS_ELAPSED_REAL_TIME_UNC_BIT = (1<<10),
    /** Location has valid time uncertainty.*/
    HAS_TIME_UNC_BIT = (1<<11),
    /** Location has valid elapsed gPTP time.*/
    HAS_GPTP_TIME_BIT         = (1<<12),
    /** Location has valid elapsed gPTP time uncertainty.*/
    HAS_GPTP_TIME_UNC_BIT     = (1<<13)
};

/*Bit mask containing bits from LocationValidityType */
using LocationInfoValidity = uint32_t;

/** Specify the valid fields in LocationInfoExValidityType.
 *  User should determine whether a field in LocationInfoExValidityType
 *  is valid or not by checking the corresponding bit is set or not.
 */
enum LocationInfoExValidityType {
  /** valid altitude mean sea level */
  HAS_ALTITUDE_MEAN_SEA_LEVEL = (1ULL << 0),
  /** valid pdop, hdop, and vdop */
  HAS_DOP = (1ULL << 1),
  /** valid magnetic deviation */
  HAS_MAGNETIC_DEVIATION = (1ULL << 2),
  /** valid horizontal reliability */
  HAS_HOR_RELIABILITY = (1ULL << 3),
  /** valid vertical reliability */
  HAS_VER_RELIABILITY = (1ULL << 4),
  /** valid elipsode semi major */
  HAS_HOR_ACCURACY_ELIP_SEMI_MAJOR = (1ULL << 5),
  /** valid elipsode semi minor */
  HAS_HOR_ACCURACY_ELIP_SEMI_MINOR = (1ULL << 6),
  /** valid accuracy elipsode azimuth */
  HAS_HOR_ACCURACY_ELIP_AZIMUTH = (1ULL << 7),
  /** valid gnss sv used in pos data */
  HAS_GNSS_SV_USED_DATA = (1ULL << 8),
  /** valid navSolutionMask */
  HAS_NAV_SOLUTION_MASK = (1ULL << 9),
  /** valid LocPosTechMask */
  HAS_POS_TECH_MASK = (1ULL << 10),
  /** valid LocSvInfoSource */
  HAS_SV_SOURCE_INFO = (1ULL << 11),
  /** valid position dynamics data */
  HAS_POS_DYNAMICS_DATA = (1ULL << 12),
  /** valid gdop, tdop */
  HAS_EXT_DOP = (1ULL << 13),
  /**valid North standard deviation */
  HAS_NORTH_STD_DEV = (1ULL << 14),
  /** valid East standard deviation*/
  HAS_EAST_STD_DEV = (1ULL << 15),
  /** valid North Velocity */
  HAS_NORTH_VEL = (1ULL << 16),
  /** valid East Velocity */
  HAS_EAST_VEL = (1ULL << 17),
  /** valid Up Velocity */
  HAS_UP_VEL = (1ULL << 18),
  /** valid North Velocity Uncertainty */
  HAS_NORTH_VEL_UNC = (1ULL << 19),
  /** valid East Velocity Uncertainty */
  HAS_EAST_VEL_UNC = (1ULL << 20),
  /** valid Up Velocity Uncertainty */
  HAS_UP_VEL_UNC = (1ULL << 21),
  /** valid leap_seconds */
  HAS_LEAP_SECONDS = (1ULL << 22),
  /** valid timeUncMs
   *
   * @deprecated
   * Use @ref LocationValidityType::HAS_TIME_UNC_BIT to get the required information about
   * validity of time uncertainty */
  HAS_TIME_UNC = (1ULL << 23),
  /** valid number of sv used */
  HAS_NUM_SV_USED_IN_POSITION = (1ULL << 24),
  /** valid sensor calibrationConfidencePercent */
  HAS_CALIBRATION_CONFIDENCE_PERCENT = (1ULL << 25),
  /** valid sensor calibrationConfidence */
  HAS_CALIBRATION_STATUS = (1ULL << 26),
  /** valid output engine type */
  HAS_OUTPUT_ENG_TYPE = (1ULL << 27),
  /** valid output engine mask */
  HAS_OUTPUT_ENG_MASK = (1ULL << 28),
  /** valid conformity index */
  HAS_CONFORMITY_INDEX_FIX = (1ULL << 29),
  /** valid lla vrp based*/
  HAS_LLA_VRP_BASED = (1ULL << 30),
  /** valid enu velocity vrp based*/
  HAS_ENU_VELOCITY_VRP_BASED = (1ULL << 31),
  /** valid altitude type*/
  HAS_ALTITUDE_TYPE = (1ULL << 32),
  /** valid report status*/
  HAS_REPORT_STATUS = (1ULL << 33),
  /** valid integrity risk*/
  HAS_INTEGRITY_RISK_USED = (1ULL << 34),
  /** valid protect level along track*/
  HAS_PROTECT_LEVEL_ALONG_TRACK = (1ULL << 35),
  /** valid protect level cross track*/
  HAS_PROTECT_LEVEL_CROSS_TRACK = (1ULL << 36),
  /** valid protect level vertical*/
  HAS_PROTECT_LEVEL_VERTICAL = (1ULL << 37),
  /** valid DR Solution status*/
  HAS_SOLUTION_STATUS = (1ULL << 38),
  /** valid dgnssStationId */
  HAS_DGNSS_STATION_ID = (1ULL<<39),
  /** valid baseline length */
  HAS_BASE_LINE_LENGTH = (1ULL<<40),
  /** valid age of correction */
  HAS_AGE_OF_CORRECTION = (1ULL<<41),
  /** valid leap second uncertainty */
  HAS_LEAP_SECONDS_UNC = (1ULL<<42)
};

/*Bit mask containing bits from LocationInfoExValidityType */
using LocationInfoExValidity = uint64_t;

/** Specify the GNSS signal type and RF band for jammer info and
 *  automatic gain control metric in GnssData.*/
enum GnssDataSignalTypes {
  /** Invalid signal type */
  GNSS_DATA_SIGNAL_TYPE_INVALID = -1,
  /** GPS L1CA RF Band.*/
  GNSS_DATA_SIGNAL_TYPE_GPS_L1CA = 0,
  /** GPS L1C RF Band.*/
  GNSS_DATA_SIGNAL_TYPE_GPS_L1C = 1,
  /** GPS L2C_L RF Band.*/
  GNSS_DATA_SIGNAL_TYPE_GPS_L2C_L = 2,
  /** GPS L5_Q RF Band.*/
  GNSS_DATA_SIGNAL_TYPE_GPS_L5_Q = 3,
  /** GLONASS G1 (L1OF) RF Band.*/
  GNSS_DATA_SIGNAL_TYPE_GLONASS_G1 = 4,
  /** GLONASS G2 (L2OF) RF Band.*/
  GNSS_DATA_SIGNAL_TYPE_GLONASS_G2 = 5,
  /** GALILEO E1_C RF Band.*/
  GNSS_DATA_SIGNAL_TYPE_GALILEO_E1_C = 6,
  /** GALILEO E5A_Q RF Band.*/
  GNSS_DATA_SIGNAL_TYPE_GALILEO_E5A_Q = 7,
  /** GALILEO E5B_Q RF Band.*/
  GNSS_DATA_SIGNAL_TYPE_GALILEO_E5B_Q = 8,
  /** BEIDOU B1_I RF Band.*/
  GNSS_DATA_SIGNAL_TYPE_BEIDOU_B1_I = 9,
  /** BEIDOU B1C RF Band.*/
  GNSS_DATA_SIGNAL_TYPE_BEIDOU_B1C = 10,
  /** BEIDOU B2_I RF Band.*/
  GNSS_DATA_SIGNAL_TYPE_BEIDOU_B2_I = 11,
  /** BEIDOU B2A_I RF Band.*/
  GNSS_DATA_SIGNAL_TYPE_BEIDOU_B2A_I = 12,
  /** QZSS L1CA RF Band.*/
  GNSS_DATA_SIGNAL_TYPE_QZSS_L1CA = 13,
  /** QZSS L1S RF Band.*/
  GNSS_DATA_SIGNAL_TYPE_QZSS_L1S = 14,
  /** QZSS L2C_L RF Band.*/
  GNSS_DATA_SIGNAL_TYPE_QZSS_L2C_L = 15,
  /** QZSS L5_Q RF Band.*/
  GNSS_DATA_SIGNAL_TYPE_QZSS_L5_Q = 16,
  /** SBAS L1_CA RF Band.*/
  GNSS_DATA_SIGNAL_TYPE_SBAS_L1_CA = 17,
  /** NAVIC L5 RF Band.*/
  GNSS_DATA_SIGNAL_TYPE_NAVIC_L5 = 18,
  /** BEIDOU B2A_Q RF Band.*/
  GNSS_DATA_SIGNAL_TYPE_BEIDOU_B2A_Q = 19,
  /** BEIDOU B2BI RF Band.*/
  GNSS_DATA_SIGNAL_TYPE_BEIDOU_B2BI = 20,
  /** BEIDOU B2BQ RF Band. */
  GNSS_DATA_SIGNAL_TYPE_BEIDOU_B2BQ = 21,
  /** NAVIC L1 RF Band. */
  GNSS_DATA_SIGNAL_TYPE_NAVIC_L1 = 22,
  /**< Maximum number of signal types.*/
  GNSS_DATA_MAX_NUMBER_OF_SIGNAL_TYPES = 23
};

/** Specify valid mask of data fields in GnssData.*/
enum GnssDataValidityType {
  /** Jammer Indicator is available */
  HAS_JAMMER = (1ULL << 0),
  /** AGC is available */
  HAS_AGC = (1ULL << 1)
};

/** Specifies GnssDataValidityType mask */
using GnssDataValidity = uint32_t;

/** Indicate RF Automatic Gain Control Status */
enum AgcStatus {
  /** AGC status is unknown */
  UNKNOWN = 0,
  /** Not saturated */
  NO_SATURATION = 1,
  /** Front end gain is at maximum saturation */
  FRONT_END_GAIN_MAXIMUM_SATURATION = 2,
  /** Front end gain is at minimum saturation */
  FRONT_END_GAIN_MINIMUM_SATURATION = 3
};

/** Specify the additional GNSS data that can be provided during a tracking
 *  session, currently jammer and automatic gain control data are available.*/
struct GnssData {
  /** bitwise OR of GnssDataValidityType */
  GnssDataValidity gnssDataMask[GnssDataSignalTypes::GNSS_DATA_MAX_NUMBER_OF_SIGNAL_TYPES];
  /** Jammer Indication for each signal type. Each index represents the signal type in
   *  GnssDataSignalTypes.
   */
  double jammerInd[GnssDataSignalTypes::GNSS_DATA_MAX_NUMBER_OF_SIGNAL_TYPES];
  /** Overall automatic gain control level as observed at the input to correlator,
   *  in units of dB.
   */
  double agc[GnssDataSignalTypes::GNSS_DATA_MAX_NUMBER_OF_SIGNAL_TYPES];
  /** RF Automatic gain control status for L1 band. */
  AgcStatus     agcStatusL1;
  /** RF Automatic gain control status for L2 band. */
  AgcStatus     agcStatusL2;
  /** RF Automatic gain control status for L5 band. */
  AgcStatus     agcStatusL5;
};

/** Specify the sensor calibration status in @ref ILocationInfoEx.*/
enum DrCalibrationStatusType {
  /** Indicate that roll calibration is needed. Need to take more
   *  turns on level ground.*/
  DR_ROLL_CALIBRATION_NEEDED  = (1<<0),
  /** Indicate that pitch calibration is needed. Need to take more
   *  turns on level ground.*/
  DR_PITCH_CALIBRATION_NEEDED = (1<<1),
  /** Indicate that yaw calibration is needed. Need to accelerate
   *  in a straight line.*/
  DR_YAW_CALIBRATION_NEEDED   = (1<<2),
  /** Indicate that odo calibration is needed. Need to accelerate
   *  in a straight line.*/
  DR_ODO_CALIBRATION_NEEDED   = (1<<3),
  /** Indicate that gyro calibration is needed. Need to take more
   *  turns on level ground.*/
  DR_GYRO_CALIBRATION_NEEDED  = (1<<4),
  /** Lot more turns on level ground needed */
  DR_TURN_CALIBRATION_LOW     = (1<<5),
  /** Some more turns on level ground needed */
  DR_TURN_CALIBRATION_MEDIUM  = (1<<6),
  /** Sufficient turns on level ground observed */
  DR_TURN_CALIBRATION_HIGH    = (1<<7),
  /** Lot more accelerations in straight line needed */
  DR_LINEAR_ACCEL_CALIBRATION_LOW      = (1<<8),
  /** Some more accelerations in straight line needed */
  DR_LINEAR_ACCEL_CALIBRATION_MEDIUM   = (1<<9),
  /** Sufficient acceleration events in straight line observed */
  DR_LINEAR_ACCEL_CALIBRATION_HIGH     = (1<<10),
  /** Lot more motion in straight line needed */
  DR_LINEAR_MOTION_CALIBRATION_LOW     = (1<<11),
  /** Some more motion in straight line needed */
  DR_LINEAR_MOTION_CALIBRATION_MEDIUM  = (1<<12),
  /** Sufficient motion events in straight line observed */
  DR_LINEAR_MOTION_CALIBRATION_HIGH    = (1<<13),
  /** Lot more stationary events on level ground needed */
  DR_STATIC_CALIBRATION_LOW            = (1<<14),
  /** Some more stationary events on level ground needed */
  DR_STATIC_CALIBRATION_MEDIUM         = (1<<15),
  /** Sufficient stationary events on level ground observed */
  DR_STATIC_CALIBRATION_HIGH           = (1<<16)
};

/** Specifies DrCalibrationStatusType mask */
using DrCalibrationStatus = uint32_t;

/** Specify various status that contributes to the DR position
 *  engine. */
enum DrSolutionStatusType {
    /** Vehicle sensor speed input was detected by the DR position engine. */
    VEHICLE_SENSOR_SPEED_INPUT_DETECTED = (1<<0),
    /** Vehicle sensor speed input was used by the DR position engine. */
    VEHICLE_SENSOR_SPEED_INPUT_USED     = (1<<1),
    /** DRE solution disengaged due to insufficient calibration */
    WARNING_UNCALIBRATED                = (1<<2),
    /** DRE solution disengaged due to bad GNSS quality */
    WARNING_GNSS_QUALITY_INSUFFICIENT   = (1<<3),
    /** DRE solution disengaged as ferry condition detected */
    WARNING_FERRY_DETECTED              = (1<<4),
    /** DRE solution disengaged as 6DOF sensor inputs not available */
    ERROR_6DOF_SENSOR_UNAVAILABLE       = (1<<5),
    /** DRE solution disengaged as vehicle speed inputs not available */
    ERROR_VEHICLE_SPEED_UNAVAILABLE     = (1<<6),
    /** DRE solution disengaged as Ephemeris info not available */
    ERROR_GNSS_EPH_UNAVAILABLE          = (1<<7),
    /** DRE solution disengaged as GNSS measurement info not available */
    ERROR_GNSS_MEAS_UNAVAILABLE         = (1<<8),
    /** DRE solution disengaged due non-availability of stored position from previous session */
    WARNING_INIT_POSITION_INVALID       = (1<<9),
    /** DRE solution dis-engaged due to vehicle motion detected at session start */
    WARNING_INIT_POSITION_UNRELIABLE    = (1<<10),
    /** DRE solution dis-engaged due to unreliable position */
    WARNING_POSITON_UNRELIABLE          = (1<<11),
    /** DRE solution dis-engaged due to a generic error */
    ERROR_GENERIC                       = (1<<12),
    /** DRE solution dis-engaged due to Sensor Temperature being out of range */
    WARNING_SENSOR_TEMP_OUT_OF_RANGE    = (1<<13),
    /** DRE solution dis-engaged due to insufficient user dynamics */
    WARNING_USER_DYNAMICS_INSUFFICIENT  = (1<<14),
    /** DRE solution dis-engaged due to inconsistent factory data */
    WARNING_FACTORY_DATA_INCONSISTENT   = (1<<15)
};

/** Specifies DrSolutionStatus mask */
using DrSolutionStatus = uint32_t;

/** Specifies the set of engines whose position reports are requested via
 *  startDetailedEngineReports.*/
enum LocReqEngineType{
    /** Indicate that the fused/default position is needed to be reported back
     *  for the tracking sessions. The default position is the propagated/aggregated
     *  reports from all engines running on the system (e.g.: DR/SPE/PPE) according to
     *  QTI algorithm.
     */
    LOC_REQ_ENGINE_FUSED_BIT = (1<<0),
    /** Indicate that the unmodified SPE position is needed to be reported back for the
     *  tracking sessions.
     */
    LOC_REQ_ENGINE_SPE_BIT   = (1<<1),
    /** Indicate that the unmodified PPE position is needed to be reported back for the
     *  tracking sessions.
     */
    LOC_REQ_ENGINE_PPE_BIT   = (1<<2),
     /**Indicate that the unmodified VPE position is needed to be reported back for the
      * tracking sessions.
      */
    LOC_REQ_ENGINE_VPE_BIT  = (1<<3),

};

/** Specifies LocReqEngineType mask*/
using LocReqEngine = uint16_t;

/** Specifies the type of engine for the reported fixes*/
enum LocationAggregationType {
  /** This is the propagated/aggregated report from the fixes of all engines
   *  running on the system (e.g.: DR/SPE/PPE).*/
    LOC_OUTPUT_ENGINE_FUSED = 0,
  /** This fix is the unmodified fix from modem GNSS engine */
    LOC_OUTPUT_ENGINE_SPE   = 1,
  /** This is the unmodified fix from PPP engine */
    LOC_OUTPUT_ENGINE_PPE   = 2,
  /** This is the unmodified fix from VPE engine. */
    LOC_OUTPUT_ENGINE_VPE  = 3,
};

/** Specifies the type of engine responsible for fixes when the engine type is fused*/
enum PositioningEngineType{
    /** For standard GNSS position engines.*/
    STANDARD_POSITIONING_ENGINE = (1 << 0),
    /** For dead reckoning position engines.*/
    DEAD_RECKONING_ENGINE       = (1 << 1),
    /** For precise position engines.*/
    PRECISE_POSITIONING_ENGINE  = (1 << 2),
    /** For VP position engine.*/
    VP_POSITIONING_ENGINE       = (1 << 3),
};

/** Specifies PositioningEngineType mask */
using PositioningEngine = uint32_t;

/**
 * Specify parameters related to enable/disable SVs */
struct SvBlackListInfo {
    /** constellation for the sv  */
    GnssConstellationType constellation;
    /** sv id for the constellation:
     * 0 means blacklist for all SVIds of a given constellation type
     * GPS SV id range: 1 to 32
     * GLONASS SV id range: 65 to 96
     * QZSS SV id range: 193 to 197
     * BDS SV id range: 201 to 263
     * GAL SV id range: 301 to 336
     * SBAS SV id range: 120 to 158 and 183 to 191
     * NAVIC SV id range: 401 to 420
     */
    uint32_t              svId;
};

typedef std::vector<SvBlackListInfo> SvBlackList;

/**
 *  Lever ARM type */
enum LeverArmType {
    /** Lever arm parameters regarding the VRP (Vehicle Reference
     *  Point) w.r.t the origin (at the GNSS Antenna) */
    LEVER_ARM_TYPE_GNSS_TO_VRP = 1,
    /** Lever arm regarding GNSS Antenna w.r.t the origin at the
     *  IMU (inertial measurement unit) for DR (dead reckoning
     *  engine) */
    LEVER_ARM_TYPE_DR_IMU_TO_GNSS = 2,
    /** Lever arm regarding GNSS Antenna w.r.t the origin at the
     *  IMU (inertial measurement unit) for VEPP (vision enhanced
     *  precise positioning engine)
     *  @deprecated enum type is not supported.*/
    LEVER_ARM_TYPE_VEPP_IMU_TO_GNSS = 3,
    /** Lever arm regarding GNSS Antenna w.r.t the origin at the
     *  IMU (inertial measurement unit) for VPE (vision positioning
     *  engine) */
    LEVER_ARM_TYPE_VPE_IMU_TO_GNSS = 3,
};

/**
 * Specify parameters related to lever arm */
struct LeverArmParams {
    /** Offset along the vehicle forward axis, in unit of meters */
    float forwardOffset;
    /** Offset along the vehicle starboard axis, in unit of
     *  meters */
    float sidewaysOffset;
    /** Offset along the vehicle up axis, in unit of meters  */
    float upOffset;
};

typedef std::unordered_map<LeverArmType, LeverArmParams> LeverArmConfigInfo;

/** Specify valid fields in GnssMeasurementsData.*/
enum GnssMeasurementsDataValidityType{
    /** Validity of svId.*/
    SV_ID_BIT                        = (1<<0),
    /** Validity of svType.*/
    SV_TYPE_BIT                      = (1<<1),
    /** Validity of stateMask.*/
    STATE_BIT                        = (1<<2),
    /** Validity of receivedSvTimeNs and receivedSvTimeSubNs.*/
    RECEIVED_SV_TIME_BIT             = (1<<3),
    /** Validity of receivedSvTimeUncertaintyNs.*/
    RECEIVED_SV_TIME_UNCERTAINTY_BIT = (1<<4),
    /** Validity of carrierToNoiseDbHz.*/
    CARRIER_TO_NOISE_BIT             = (1<<5),
    /** Validity of pseudorangeRateMps.*/
    PSEUDORANGE_RATE_BIT             = (1<<6),
    /** Validity of pseudorangeRateUncertaintyMps.*/
    PSEUDORANGE_RATE_UNCERTAINTY_BIT = (1<<7),
    /** Validity of adrStateMask.*/
    ADR_STATE_BIT                    = (1<<8),
    /** Validity of adrMeters.*/
    ADR_BIT                          = (1<<9),
    /** Validity of adrUncertaintyMeters.*/
    ADR_UNCERTAINTY_BIT              = (1<<10),
    /** Validity of carrierFrequencyHz.*/
    CARRIER_FREQUENCY_BIT            = (1<<11),
    /** Validity of carrierCycles.*/
    CARRIER_CYCLES_BIT               = (1<<12),
    /** Validity of carrierPhase.*/
    CARRIER_PHASE_BIT                = (1<<13),
    /** Validity of carrierPhaseUncertainty.*/
    CARRIER_PHASE_UNCERTAINTY_BIT    = (1<<14),
    /** Validity of multipathIndicator.*/
    MULTIPATH_INDICATOR_BIT          = (1<<15),
    /** Validity of signalToNoiseRatioDb.*/
    SIGNAL_TO_NOISE_RATIO_BIT        = (1<<16),
    /** Validity of agcLevelDb.*/
    AUTOMATIC_GAIN_CONTROL_BIT       = (1<<17),
    /** Validity of signal type.*/
    GNSS_SIGNAL_TYPE                 = (1<<18),
    /** Validity of basebandCarrierToNoise.*/
    BASEBAND_CARRIER_TO_NOISE        = (1<<19),
    /** Validity of fullInterSignalBias.*/
    FULL_ISB                         = (1<<20),
    /** Validity of fullInterSignalBiasUncertainty.*/
    FULL_ISB_UNCERTAINTY             = (1<<21)
};

/** Specifies GnssMeasurementsDataValidityType.*/
using GnssMeasurementsDataValidity = uint32_t;

/** Specify GNSS measurement state in
 *  GnssMeasurementsData::stateMask.*/
enum GnssMeasurementsStateValidityType {
    /** State is unknown.*/
    UNKNOWN_BIT                 = 0,
    /** State is "code lock".*/
    CODE_LOCK_BIT               = (1<<0),
    /** State is "bit sync".*/
    BIT_SYNC_BIT                = (1<<1),
    /** State is "subframe sync".*/
    SUBFRAME_SYNC_BIT           = (1<<2),
    /** State is "tow decoded".*/
    TOW_DECODED_BIT             = (1<<3),
    /** State is "msec ambiguous".*/
    MSEC_AMBIGUOUS_BIT          = (1<<4),
    /** State is "symbol sync".*/
    SYMBOL_SYNC_BIT             = (1<<5),
    /** State is "GLONASS string sync".*/
    GLO_STRING_SYNC_BIT         = (1<<6),
    /** State is "GLONASS TOD decoded".*/
    GLO_TOD_DECODED_BIT         = (1<<7),
    /** State is "BDS D2 bit sync".*/
    BDS_D2_BIT_SYNC_BIT         = (1<<8),
    /** State is "BDS D2 subframe sync".*/
    BDS_D2_SUBFRAME_SYNC_BIT    = (1<<9),
    /** State is "Galileo E1BC code lock".*/
    GAL_E1BC_CODE_LOCK_BIT      = (1<<10),
    /** State is "Galileo E1C second code lock".*/
    GAL_E1C_2ND_CODE_LOCK_BIT   = (1<<11),
    /** State is "Galileo E1B page sync".*/
    GAL_E1B_PAGE_SYNC_BIT       = (1<<12),
    /** State is "SBAS sync".*/
    SBAS_SYNC_BIT               = (1<<13)
};

/** Specifies GnssMeasurementsStateValidityType.*/
using GnssMeasurementsStateValidity = uint32_t;

/** Specify accumulated delta range state in
 *  GnssMeasurementsData::adrStateMask.*/
enum GnssMeasurementsAdrStateValidityType {
    /** State is unknown.*/
    UNKNOWN_STATE   = 0,
    /** State is valid.*/
    VALID_BIT       = (1<<0),
    /** State is "reset".*/
    RESET_BIT       = (1<<1),
    /** State is "cycle slip".*/
    CYCLE_SLIP_BIT  = (1<<2),
    /** State is "half cycle resolved".*/
    HALF_CYCLE_RESOLVED_BIT  = (1<<3)
};

/** Specifies GnssMeasurementsAdrStateValidityType.*/
using GnssMeasurementsAdrStateValidity = uint32_t;

/** Specify the GNSS multipath indicator state in
 *  GnssMeasurementsData::multipathIndicator.*/
enum GnssMeasurementsMultipathIndicator {
    /** Multipath indicator is unknown.*/
    UNKNOWN_INDICATOR     = 0,
    /** Multipath indicator is present.*/
    PRESENT               = 1,
    /** Multipath indicator is not present.*/
    NOT_PRESENT           = 2
};

/** Specify the valid fields in GnssMeasurementsClock.*/
enum GnssMeasurementsClockValidityType {
    /** Validity of leapSecond.*/
    LEAP_SECOND_BIT                   = (1<<0),
    /** Validity of timeNs.*/
    TIME_BIT                          = (1<<1),
    /** Validity of timeUncertaintyNs.*/
    TIME_UNCERTAINTY_BIT              = (1<<2),
    /** Validity of fullBiasNs.*/
    FULL_BIAS_BIT                     = (1<<3),
    /** Validity of biasNs.*/
    BIAS_BIT                          = (1<<4),
    /** Validity of biasUncertaintyNs.*/
    BIAS_UNCERTAINTY_BIT              = (1<<5),
    /** Validity of driftNsps.*/
    DRIFT_BIT                         = (1<<6),
    /** Validity of driftUncertaintyNsps.*/
    DRIFT_UNCERTAINTY_BIT             = (1<<7),
    /** Validity of hwClockDiscontinuityCount.*/
    HW_CLOCK_DISCONTINUITY_COUNT_BIT  = (1<<8),
    /** Validity of realTime.*/
    ELAPSED_REAL_TIME_BIT             = (1<<9),
    /** Validity of realTimeUncertainity.*/
    ELAPSED_REAL_TIME_UNC_BIT         = (1<<10),
    /** Validity of gPTPTime.*/
    ELAPSED_GPTP_TIME_BIT             = (1<<11),
    /** Validity of gPTPTimeUncertainity.*/
    ELAPSED_GPTP_TIME_UNC_BIT         = (1<<12)
};

/** Specifies GnssMeasurementsClockValidityType.*/
using GnssMeasurementsClockValidity = uint32_t;

/** Specify the signal measurement information such as satellite vehicle pseudo range,
 *  satellite vehicle time, carrier phase measurement etc. from GNSS positioning engine.
 */
struct GnssMeasurementsData {
    /** Bitwise OR of GnssMeasurementsDataValidityType to specify the
     *  valid fields in GnssMeasurementsData. */
    GnssMeasurementsDataValidity valid;
    /** Specify satellite vehicle ID number.*/
    int16_t svId;
    /** SV constellation type.*/
    GnssConstellationType svType;
    /** This field  provides an individual time-stamp for the measurement,
     *  and allows sub-nanosecond accuracy. It is always set to zero as all
     *  measurements are aligned to a common time.*/
    double timeOffsetNs;
    /** Bitwise OR of GnssMeasurementsStateValidityType to specify the
     *  GNSS measurement state.*/
    GnssMeasurementsStateValidity stateMask;
    /** The GNSS satellite (SV) time in the constellation time scale
     *  in nanoseconds at transmission.
     *  For GLONASS, this is the time of day, while for other systems,
     *  it is the time of week.
     *  The total SV time is calculated as:
     *  receivedSvTimeNs + receivedSvTimeSubNs.
     */
    int64_t receivedSvTimeNs;
    /** The sub nanosecond portion of the GNSS satellite (SV) time
     *  in the constellation time scale in nanoseconds at transmission.
     *  For GLONASS, this is the time of day, while for other systems,
     *  it is the time of week.
     *  The total SV time is calculated as:
     *  receivedSvTimeNs + receivedSvTimeSubNs.
     */
    float receivedSvTimeSubNs;
    /** Received GNSS satellite (SV) time 1-Sigma uncertainty in nanoseconds.*/
    int64_t receivedSvTimeUncertaintyNs;
    /** Carrier to noise ratio at antenna, in unit of dB-Hz.*/
    double carrierToNoiseDbHz;
    /** Uncorrected Pseudorange Rate in units of meter/second.*/
    double pseudorangeRateMps;
    /** Uncorrected Pseudorange Rate 1-Sigma Uncertainty in units of meter/second.*/
    double pseudorangeRateUncertaintyMps;
    /** Status bitmask of the ADR (Accumulated Delta Range) data.
     *  Please see description for GnssMeasurementsAdrStateValidity.*/
    GnssMeasurementsAdrStateValidity adrStateMask;
    /** Accumulated delta range in units of meters since last SV carrier phase lock.*/
    double adrMeters;
    /** 1-Sigma Accumulated delta range uncertainty in unit of meters.*/
    double adrUncertaintyMeters;
    /** Carrier frequency of the tracked signal in units of Hertz.*/
    float carrierFrequencyHz;
    /** This field is no longer available. Please refer to adrMeters.*/
    int64_t carrierCycles;
    /** This field is no longer available. Please refer to adrMeters.*/
    double carrierPhase;
    /** This field is no longer available. Please refer to adrUncertaintyMeters.*/
    double carrierPhaseUncertainty;
    /** This field is no longer available.*/
    GnssMeasurementsMultipathIndicator multipathIndicator;
    /** This field is no longer available.*/
    double signalToNoiseRatioDb;
    /** Overall automatic gain control level as observed at the input to correlator,
        in units of dB.*/
    double agcLevelDb;
    /** GnssSignalType mask */
    GnssSignal gnssSignalType;
    /** Carrier-to-noise ratio of the signal measured at baseband,
     *  in unit of dB-Hz. */
    double basebandCarrierToNoise;
    /** The full inter-signal bias (ISB) between the Signal specified
     *  in gnssSignalType and GPS L1 C/A in nanoseconds.
     *  This value is the sum of the estimated receiver-side and the
     *  space-segment-side inter-system bias, inter-frequency bias
     *  and inter-code bias. */
    double fullInterSignalBias;
    /** Uncertainty associated with the full inter-signal bias in
     *  nanoseconds. */
    double fullInterSignalBiasUncertainty;
};

/** Specify GNSS measurements clock.*/
struct GnssMeasurementsClock {
    /** Validity bitmask of the GnssMeasurementsClockValidityType.
     *  Please see description for GnssMeasurementsClockValidity.*/
    GnssMeasurementsClockValidity valid;
    /** Leap Second: Delta between the GNSS time and UTC time in units of seconds.*/
    int16_t leapSecond;
    /** The internal hardware clock value of the GNSS receiver increases monotonically
     *  in nanoseconds, as long as the GNSS session remains active.*/
    int64_t timeNs;
    /** System clock time uncertainty in units of nanoseconds.
     *  This field is always 0.*/
    double timeUncertaintyNs;
    /** Full bias, in uint of nanoseconds.
     *  The difference between hardware clock (timeNs) inside GPS receiver and the true
     *  GPS time since 0000Z, January 6, 1980, in nanoseconds.
     *  This value is available if the receiver has estimated GPS time.
     *  For clarification,  local estimate of GPS time = timeNs - (fullBiasNs + biasNs)
     */
    int64_t fullBiasNs;
    /** Sub-nanoseconds bias, in unit of nanoseconds.*/
    double biasNs;
    /** Bias uncertainty (one sigma), in unit of nanoseconds.*/
    double biasUncertaintyNs;
    /** GNSS Receiver Clock Drift in units of nanoseconds per seconds.*/
    double driftNsps;
    /** GNSS Receiver Clock Drift one sigma uncertainty in units of nanoseconds per seconds.*/
    double driftUncertaintyNsps;
    /** Number of clock resets/discontinuities detected, affecting the field timeNs.*/
    uint32_t hwClockDiscontinuityCount;
    /** Elapsed real-time of this clock since system boot, in unit of nanoseconds.*/
    uint64_t elapsedRealTime;
    /** uncertainty of elapsedRealTime, in unit of nanoseconds.*/
    uint64_t elapsedRealTimeUnc;
    /** GPTP time field corresponding to source time ticks. Used for time sync between different
     *  systems, in unit of nanoseconds.*/
    uint64_t elapsedgPTPTime;
    /** uncertainty of elapsedgPTPTime, in unit of nanoseconds.*/
    uint64_t elapsedgPTPTimeUnc;
};

/** Specify GNSS measurements clock and data.
 *  GnssMeasurementInfo is used to convey the satellite vehicle info whose measurements are
 *  actually used to generate the current position report. While GnssMeasurements contains the
 *  satellite measurements that device observed during tracking session, regardless the measurement
 *  is used or not used to compute the fix. Furthermore GnssMeasurements contains much richer set
 *  of information which can enable other third party engines to utilize the measurements and
 *  compute the position by itself.
 */
struct GnssMeasurements {
    /** GNSS measurements clock info.*/
    GnssMeasurementsClock clock;
    /** GNSS measurements data.*/
    std::vector<GnssMeasurementsData> measurements;
    /** Indicates the frequency for GNSS measurements generated at NHz or not.*/
    bool isNHz;
    /** RF Automatic gain control status for L1 band. */
    AgcStatus     agcStatusL1;
    /** RF Automatic gain control status for L2 band. */
    AgcStatus     agcStatusL2;
    /** RF Automatic gain control status for L5 band. */
    AgcStatus     agcStatusL5;
};

/**
 * Disaster and crisis report types that are currently supported by the GNSS Engine.
 */
enum GnssReportDCType {
    /**
     * Disaster Prevention information provided by Japan Meteorological Agency.
     */
    QZSS_JMA_DISASTER_PREVENTION_INFO = 43,
    /**
     * Disaster Prevention information provided by other organizations.
     */
    QZSS_NON_JMA_DISASTER_PREVENTION_INFO = 44
};

/**
 * Specify the Disaster-crisis type and data payload received from the GNSS engine.
 */
struct GnssDisasterCrisisReport {
    /**
     * Disaster and crisis report types supported by the GNSS Engine.
     */
    GnssReportDCType dcReportType;
    /**
     * The disaster crisis report data, packed into uint8_t.
     * The bits in the payload are packed w.r.t the MSB First ordering.
     */
    std::vector<uint8_t> dcReportData;
    /**
     * Number of valid bits that client should use in the payload as
     * per the dcReportData.
     */
    uint16_t numValidBits;
    /**
     * Pseudo-Random Number validity
     */
    bool prnValid;
    /**
     * Pseudo-Random Number.
     */
    uint8_t prn;
};

/** Specifies Source of Ephemeris data */
enum GnssEphSource {
    /** Source of ephemeris is unknown  */
    EPH_SRC_UNKNOWN = 0,
    /** Source of ephemeris is OTA  */
    EPH_SRC_OTA = 1,
    /** Max value for ephemeris Source. DO NOT USE  */
    EPH_SRC_MAX = 999
};

/** Specifies the action to be performed by the clients on the ephemeris info received. */
enum GnssEphAction {
    /** Epehmeris Action Unknown  */
    EPH_ACTION_UNKNOWN = 0,
    /** Update ephemeris data */
    EPH_ACTION_UPDATE = 1,
    /** delete ephemeris action. */
    EPH_ACTION_DELETE = 2,
    /** Max value for  ephemeris action. DO NOT USE  */
    EPH_ACTION_MAX = 999
};

/** Galileo Signal Source. */
enum GalEphSignalSource {
    /** GALILEO signal is unknown */
    GAL_SIG_SRC_UNKNOWN = 0,
    /** GALILEO signal is E1B  */
    GAL_SIG_SRC_E1B = 1,
    /** GALILEO signal is E5A  */
    GAL_SIG_SRC_E5A = 2,
    /** GALILEO signal is E5B  */
    GAL_SIG_SRC_E5B = 3
};

/** Common Ephemeris information for all constellations*/
struct GnssEphCommon {
    /** Specify satellite vehicle ID number.
     * For SV id range of each supported constellations, refer to
     * documentation of @ref telux::loc::GnssMeasurementInfo::gnssSvId.
    */
    uint16_t gnssSvId;

    /** Specifies the source of ephemeris.*/
    GnssEphSource ephSource;

    /** Specifies the action to be performed on receipt of the ephemeris (Update/Delete)
     *  Action shall be performed on GnssEphSource specified. */
    GnssEphAction action;

    /** Issue of data ephemeris used (unit-less).
     *  GPS: IODE 8 bits.
     *  BDS: AODE 5 bits.
     *  GAL: SIS IOD 10 bits.
     *  Units: Unit-less */
    uint16_t IODE;

    /** Square root of semi-major axis.
     * Units: Square Root of Meters */
    double aSqrt;

    /** Mean motion difference from computed value.
     * Units: Radians/Second */
    double deltaN;

    /** Mean anomaly at reference time.
     * Units: Radians */
    double m0;

    /** Eccentricity.
     * Units: Unit-less */
    double eccentricity;

    /** Longitude of ascending node of orbital plane at the weekly epoch.
     * Units: Radians */
    double omega0;

    /** Inclination angle at reference time.
     * Units: Radians */
    double i0;

    /** Argument of Perigee.
     * Units: Radians */
    double omega;

    /** Rate of change of right ascension.
     * Units: Radians/Second */
    double omegaDot;

    /** Rate of change of inclination angle.
     * Units: Radians/Second */
    double iDot;

    /** Amplitude of the cosine harmonic correction term to the argument of latitude.
     * Units: Radians */
    double cUc;

    /** Amplitude of the sine harmonic correction term to the argument of latitude.
     * Units: Radians */
    double cUs;

    /** Amplitude of the cosine harmonic correction term to the orbit radius.
     * Units: Meters */
    double cRc;

    /**  Amplitude of the sine harmonic correction term to the orbit radius.
     * Units: Meters */
    double cRs;

    /** Amplitude of the cosine harmonic correction term to the angle of inclination.
     * Units: Radians */
    double cIc;

    /** Amplitude of the sine harmonic correction term to the angle of inclination.
     * Units: Radians */
    double cIs;

    /** Reference time of ephemeris.
     * Units: Seconds */
    uint32_t toe;

    /**  Clock data reference time of week.
     * Units: Seconds */
    uint32_t toc;

    /** Clock bias correction coefficient.
     * Units: Seconds */
    double af0;

    /** Clock drift coefficient.
     * Units: Seconds/Second */
    double af1;

    /** Clock drift rate correction coefficient.
     * Units: Seconds/Seconds^2 */
    double af2;
};

/** Validity of GpsQzss Extended Ephemeris fields.*/
enum GpsQzssExtEphValidityType {
    /** Valid iscL1ca*/
    GPS_QZSS_EXT_EPH_ISC_L1CA_VALID = (1<<0),
     /** Valid iscL2c*/
    GPS_QZSS_EXT_EPH_ISC_L2C_VALID = (1<<1),
    /** Valid iscL5I5*/
    GPS_QZSS_EXT_EPH_ISC_L5I5_VALID = (1<<2),
    /** Valid iscL5Q5*/
    GPS_QZSS_EXT_EPH_ISC_L5Q5_VALID = (1<<3),
    /** Valid alert*/
    GPS_QZSS_EXT_EPH_ALERT_VALID = (1<<4),
    /** Valid uraned0*/
    GPS_QZSS_EXT_EPH_URANED0_VALID = (1<<5),
    /** Valid uraned1*/
    GPS_QZSS_EXT_EPH_URANED1_VALID = (1<<6),
    /** Valid uraned2*/
    GPS_QZSS_EXT_EPH_URANED2_VALID = (1<<7),
    /** Valid top*/
    GPS_QZSS_EXT_EPH_TOP_VALID = (1<<8),
    /** Valid topClock*/
    GPS_QZSS_EXT_EPH_TOP_CLOCK_VALID = (1<<9),
    /** Valid validtyPeriod*/
    GPS_QZSS_EXT_EPH_VALIDITY_PERIOD_VALID = (1<<10),
    /** Valid deltaNdot */
    GPS_QZSS_EXT_EPH_DELTA_NDOT_VALID = (1<11),
    /** Valid delaA*/
    GPS_QZSS_EXT_EPH_DELTAA_VALID = (1<<12),
    /** Valid adot */
    GPS_QZSS_EXT_EPH_ADOT_VALID = (1<<13)
};

/** Specifies GpsQzssExtEphValidityType.*/
using GpsQzssExtEphValidity = uint64_t;

/** Extended Ephemeris information for GPS and QZSS */
struct GpsQzssExtEphemeris {
    /** Specify satellite vehicle ID number.
     * For SV id range of each supported constellations, refer to
     * documentation of @ref telux::loc::GnssMeasurementInfo::gnssSvId.
     */
    uint16_t gnssSvId;

    /** Validity mask for the GpsQzss Extended Ephemeris fields.*/
    GpsQzssExtEphValidity validityMask;

    /** InterSignal Correction between L1ca Data and Pilot channels in milliseconds.
     *  Always zero for QZSS. Units: milliseconds */
    float iscL1ca;

    /** InterSignal Correction between L2c Data and Pilot channels in milliseconds.
     *  Units: milliseconds */
    float iscL2c;

    /** InterSignal Correction between L5I5 Data and Pilot channels in milliseconds.
     *  Units: milliseconds */
    float iscL5I5;

    /** InterSignal Correction between L5Q5 Data and Pilot channels in milliseconds.
     *  Units: milliseconds */
    float iscL5Q5;

    /** Alert Bit Info (unitless). */
    uint8_t alert;

    /** NED accuracy index (5 bits, unitless). */
    uint8_t uraNed0;

    /** NED accuracy change index(3 bits), UraNed1 = 1/2^N (m/s), N=14 + UraNed1 index (unitless).*/
    uint8_t uraNed1;

    /** NED accuracy change rate index (3 bits),
     *  UraNed2 = 1/2^N (m/s^2), N=28 + UraNed2 index (unitless). */
    uint8_t uraNed2;

    /** Data predict time of week, 0-604500 sec.
     *  Units: seconds */
    double top;

    /** Data predict time of week (clock) , scale 300 seconds.
     *  Units: seconds */
    uint16_t topClock;

    /** Validity Period in seconds.
     *  Units: seconds */
    uint32_t validityPeriod;

    /** Rate of Mean motion difference from computed value [semi-circle/sec^2] (unitless). */
    double deltaNdot;

    /** Semi-Major Axis Difference At Reference Time [m].
     *  Units: Meters */
    double deltaA;

    /** Change Rate In Semi-Major Axis [m/sec].
     *  Units: Meters/seconds */
    double adot;
};

/** Common Ephemeris information for GPS and QZSS*/
struct GpsQzssEphemeris {
    /**   Common ephemeris data.   */
    GnssEphCommon commonData;

    /**   Signal health, where set bit indicates unhealthy signal.
     *   Bit 0 : L5 Signal Health.
     *   Bit 1 : L2 Signal Health.
     *   Bit 2 : L1 Signal Health.*/
    uint8_t signalHealth;

    /**  User Range Accuracy Index.
     *   Units: Unit-less */
    uint8_t URAI;

    /**   Indicates which codes are commanded ON for the L2 channel (2-bits).
     *   Valid Values:
     *   00 : Reserved
     *   01 : P code ON
     *   10 : C/A code ON */
    uint8_t codeL2;

    /** L2 P-code indication flag.
     *  Value 1 indicates that the Nav data stream was commanded OFF
     *  on the P-code of the L2 channel. */
    uint8_t dataFlagL2P;

    /** Time of group delay.
     *  Units: Seconds */
    double tgd;

    /** Indicates the curve-fit interval used by the CS.
     *  Valid Values:
     *  0 : Four hours
     *  1 : Greater than four hours */
    uint8_t fitInterval;

    /**  Issue of Data, Clock.
     *   Units: Unit-less */
    uint16_t IODC;

    /** Indicates the validity of GpsQzssExtEphemeris data. */
    bool extendedEphDataValidity;

    /** Extended Ephemeris data for GPS/QZSS */
    GpsQzssExtEphemeris gpsQzssExtEphData;
};

/** Ephemeris information for GLONASS*/
struct GlonassEphemeris {
    /** Specify satellite vehicle ID number.
     *  For SV id range of each supported constellations, refer to
     *  documentation of @ref telux::loc::GnssMeasurementInfo::gnssSvId.
     */
    uint16_t gnssSvId;

    /** Specifies the source of ephemeris.*/
    GnssEphSource ephSource;

     /** Specifies the action to be performed on receipt of the ephemeris (Update/Delete)
      *  Action shall be performed on GnssEphSource specified. */
    GnssEphAction action;

    /**  SV health flags.
     * Valid Values:
     * 0 : Healthy
     * 1 : Unhealthy */
    uint8_t bnHealth;

    /** Ln SV health flags.
     *  Valid Values:
     *  0 : Healthy
     *  1 : Unhealthy */
    uint8_t lnHealth;

    /** Index of a time interval within current day according to UTC(SU) + 03 hours 00 min.
     * Units: Unit-less */
    uint8_t tb;

    /** SV accuracy index.
     * Units: Unit-less */
    uint8_t ft;

    /** GLONASS-M flag.
     * Valid Values:
     * 0 : GLONASS
     * 1 : GLONASS-M */
    uint8_t gloM;

    /** Characterizes "Age" of current information.
     * Units: Days */
    uint8_t enAge;

    /** GLONASS frequency number + 8.
     * Range: 1 to 14
     */
    uint8_t gloFrequency;

    /** Time interval between two adjacent values of tb parameter.
     * Units: Minutes */
    uint8_t p1;

    /** Flag of oddness ("1") or evenness ("0") of the value of tb
     *  for intervals 30 or 60 minutes. */
    uint8_t p2;

    /** Time difference between navigation RF signal transmitted in L2 sub-band
     *  and aviation RF signal transmitted in L1 sub-band.
     *  Units: Seconds */
    float deltaTau;

    /** Satellite XYZ position.
     *  Units: Meters */
    double position[3];

    /** Satellite XYZ velocity.
     *  Units: Meters/Second */
    double velocity[3];

    /** Satellite XYZ sola-luni acceleration.
     *  Units: Meters/Second^2 */
    double acceleration[3];

    /** Satellite clock correction relative to GLONASS time.
     *  Units: Seconds */
    float tauN;

    /** Relative deviation of predicted carrier frequency value
     * from nominal value at the instant tb.
     * Units: Unit-less */
    float gamma;

    /** Complete ephemeris time, including N4, NT and Tb.
     * [(N4-1)*1461 + (NT-1)]*86400 + tb*900
     * Units: Seconds */
    double toe;

    /** Current date, calendar number of day within four-year interval.
     *  Starting from the 1-st of January in a leap year.
     *  Units: Days */
    uint16_t nt;
};

/** BDS SV type */
enum BdsSvType {
    BDS_SV_TYPE_UNKNOWN = 0,
    BDS_SV_TYPE_GEO     = 1,
    BDS_SV_TYPE_IGSO    = 2,
    BDS_SV_TYPE_MEO     = 3
};

/** Validity of Bds Extended Ephemeris fields.*/
enum BdsExtEphValidityType {
    /** Valid iscB2a*/
    BDS_EXT_EPH_ISC_B2A_VALID = (1<<0),
    /** Valid iscB1c */
    BDS_EXT_EPH_ISC_B1C_VALID = (1<<1),
    /** Valid tgdB2a */
    BDS_EXT_EPH_TGD_B2A_VALID = (1<<2),
    /** Valid tgdB1c */
    BDS_EXT_EPH_TGD_B1C_VALID = (1<<3),
    /** Valid svType */
    BDS_EXT_EPH_SV_TYPE_VALID = (1<<4),
    /** Valid validtyPeriod */
    BDS_EXT_EPH_VALIDITY_PERIOD = (1<<5),
    /** Valid integrityFlags */
    BDS_EXT_EPH_INTEGRITY_FLAGS = (1<<6),
    /** Valid deltaNdot */
    BDS_EXT_EPH_DELTA_NDOT_VALID = (1<<7),
    /** Valid deltaA */
    BDS_EXT_EPH_DELTAA_VALID = (1<<8),
    /** Valid adot */
    BDS_EXT_EPH_ADOT_VALID = (1<<9)
};

/** Specifies BdsExtEphValidityType.*/
using BdsExtEphValidity = uint64_t;

/** Extended Ephemeris information for BDS */
struct BdsExtEphemeris {
    /** Specify satellite vehicle ID number.
     *  For SV id range of each supported constellations, refer to
     *  documentation of @ref telux::loc::GnssMeasurementInfo::gnssSvId.
     */
    uint16_t gnssSvId;

    /** Validity mask for the Bds Extended Ephemeris fields.*/
    BdsExtEphValidity validityMask;

    /** InterSignal Correction between B2a Data and Pilot channels in milliseconds.
     *  Units: milliseconds */
    float iscB2a;

    /** InterSignal Correction between B1c Data and Pilot channels in milliseconds.
     *  Units: milliseconds */
    float iscB1c;

    /** Time of Group Delay For B2a in milliseconds.
     *  Units: milliseconds */
    float tgdB2a;

    /** Time of Group Delay For B1C in milliseconds.
     *  Units: milliseconds */
    float tgdB1c;

    /** Bds Sv Type  GEO / MEO / IGSO (Unitless). */
    BdsSvType svType;

    /** Validity Period in seconds.
     *  Units: seconds */
    uint32_t validityPeriod;

    /** Satellite Integrity Flags consists data integrity Flag(DIF),
     *  Signal Integrity Flag(SIF), Accuracy Integrity Flag (AIF).
     *  Values:
     *  b0 - AIF, The signal is Valid(0) or Invalid (1).
     *  b1 - SIF, The signal is Normal(0) or Abnormal (1).
     *  b2 - DIF, The error of message parameters in this signal does not
     *  exceeds the prediction accuracy (0)/ Exceeds the prediction accuracy (1).
     *  b3 - B1I, ephemeris health (unitless).
     */
    uint8_t integrityFlags;

    /** Rate of Mean motion difference from computed value [semi-circle/sec^2] (unitless). */
    double deltaNdot;

    /** Semi-Major Axis Difference At Reference Time [m].
     *  Units: meters */
    double deltaA;

    /** Change Rate In Semi-Major Axis [m/sec].
     *  Units: Meters/seconds */
    double adot;
};

/** Ephemeris information for BDS*/
struct BdsEphemeris {

    /**  Common ephemeris data.   */
    GnssEphCommon commonData;

    /**  Satellite health information applied to both B1 and B2 (SatH1).
     * Valid Values:
     * 0 : Healthy
     * 1 : Unhealthy */
    uint8_t svHealth;

    /**  Age of data clock.
     *   Units: Hours */
    uint8_t AODC;

    /** Equipment group delay differential on B1 signal.
     *  Units: Nano-Seconds */
    double tgd1;

    /** Equipment group delay differential on B2 signal.
     *  Units: Nano-Seconds */
    double tgd2;

    /** User range accuracy index (4-bits).
     *  Units: Unit-less */
    uint8_t URAI;

    /** Indicates the validity of BdsExtEphemeris data. */
    bool extendedEphDataValidity;

    /** Extended Ephemeris data for BDS */
    BdsExtEphemeris bdsExtEphData;
};

/** Ephemeris information for GALILEO*/
struct GalileoEphemeris{

    /**  Common ephemeris data. */
    GnssEphCommon commonData;

    /** Galileo Signal Source.*/
    GalEphSignalSource dataSourceSignal;

    /**  Signal-in-space index for dual frequency E1-E5b/E5a depending on GalEphSignalSource.
     *   Units: Unit-less */
    uint8_t sisIndex;

    /** E1-E5a Broadcast group delay from F/Nav (E5A).
     *  Units: Seconds */
    double bgdE1E5a;

    /**  E1-E5b Broadcast group delay from I/Nav (E1B or E5B).
     * For E1B or E5B signal, both bgdE1E5a and bgdE1E5b are valid.
     * For E5A signal, only bgdE1E5a is valid.
     * Signal source identified using GalEphSignalSource.
     * Units: Seconds */
    double bgdE1E5b;

    /** SV health status of signal identified by GalEphSignalSource.
     * Valid Values:
     * 0 : Healthy
     * 1 : Unhealthy */
    uint8_t svHealth;
};

/** Ephemeris information for QZSS*/
struct QzssEphemeris{
    /** Common GPS-QZSS Ephemeris structure */
    GpsQzssEphemeris qzssEphData;
};

/** Ephemeris information for NAVIC*/
struct NavicEphemeris {
    /** Common ephemeris data. */
    GnssEphCommon commonData;
    /** Week number since the NavIC system time start epoch (August 22, 1999) */
    uint32_t weekNum;
    /** Issue of Data, Clock */
    uint32_t iodec;
    /** Health status of navigation data on L5 SPS signal.
     *  0=OK,
     *  1=bad */
    uint8_t l5Health;
    /** Health status of navigation data on S SPS signal.
     *  0=OK,
     *  1=bad */
    uint8_t sHealth;
    /** Inclination angle at reference time
     *  Unit: radian */
    double inclinationAngleRad;
    /** User Range Accuracy Index(4bit) */
    uint8_t urai;
    /** Time of Group delay
     *  Unit: second */
    double  tgd;
};

/**
 * Specify the Ephemeris information for a constellation received from the GNSS engine.
 */
struct GnssEphemeris {
    /** SV constellation type.*/
    GnssSystem constellationType;

    /** Validity of GNSS System Time of the ephemeris report */
    bool isSystemTimeValid;

    /** GNSS System Time of the ephemeris report */
    TimeInfo timeInfo;

    /** Based on Constellation type, only the vector for the specified constellation
     * shall be populated while the other vectors will be empty. */

    /** Ephemeris Data for each GPS SV */
    std::vector<GpsQzssEphemeris> gpsEphemerisData;

    /** Ephemeris Data for each GLONASS SV */
    std::vector<GlonassEphemeris> gloEphemerisData;

    /** Ephemeris Data for each BDS SV */
    std::vector<BdsEphemeris> bdsEphemerisData;

    /** Ephemeris Data for each GAL SV */
    std::vector<GalileoEphemeris> galEphemerisData;

    /** Ephemeris Data for each QZSS SV */
    std::vector<QzssEphemeris> qzssEphemerisData;

    /** Ephemeris Data for each NAVIC SV */
    std::vector<NavicEphemeris> navicEphemerisData;

    /** Validity of Ephemeris Signal Source Type (Unitless).
     *  Valid only for GPS/QZSS/BDS constellations */
    bool validDataSourceSignal;

    /**  Ephemeris Signal Source Type */
    GnssDataSignalTypes dataSourceSignal;
};

/** Specify leap second change event info.*/
struct LeapSecondChangeInfo {
    /** GPS timestamp that corrresponds to the last known leap
     *  second change event.
     *  The info can be available on two scenario:
     *  1: This leap second change event has been scheduled and yet
     *     to happen
     *  2: This leap second change event has already happened and
     *     next leap second change event has not yet been
     *     scheduled.*/
    TimeInfo timeInfo;
    /** Number of leap seconds prior to the leap second change event
     *  that corresponds to the timestamp at timeInfo.*/
    uint8_t leapSecondsBeforeChange;
    /** Number of leap seconds after the leap second change event
     *  that corresponds to the timestamp at timeInfo.*/
    uint8_t leapSecondsAfterChange;
};

/** Specify the valid fields in LeapSecondInfo.*/
enum LeapSecondInfoValidityType{
    /** Validity of LeapSecondInfo::current.*/
    LEAP_SECOND_SYS_INFO_CURRENT_LEAP_SECONDS_BIT = (1ULL << 0),
    /** Validity of LeapSecondInfo::info.*/
    LEAP_SECOND_SYS_INFO_LEAP_SECOND_CHANGE_BIT = (1ULL << 1)
};

/** Specifies LeapSecondInfoValidityType mask */
using LeapSecondInfoValidity = uint32_t;

/** Specify leap second info, including current leap second and
 *  leap second change event info if available.*/
struct LeapSecondInfo {
    /** Validity of LeapSecondInfo fields. */
    LeapSecondInfoValidity valid;
    /** Current leap seconds, in unit of seconds.
     *  This info will only be available only if the leap second change info
     *  is not available.*/
    uint8_t               current;
    /** Leap second change event info. The info can be available on
     *  two scenario:
     *  1: this leap second change event has been scheduled and yet
     *     to happen
     *  2: this leap second change event has already happened and
     *     next leap second change event has not yet been scheduled.
     *  If leap second change info is available, to figure out the
     *  current leap second info, compare current gps time with
     *  LeapSecondChangeInfo::timeInfo to know whether
     *  to choose leapSecondBefore or leapSecondAfter as current
     *  leap second.*/
    LeapSecondChangeInfo  info;
};

/** Specify the set of valid fields in LocationSystemInfo*/
enum LocationSystemInfoValidityType{
    /** contains current leap second or leap second change info */
    LOCATION_SYS_INFO_LEAP_SECOND = (1ULL << 0),
};

/** Specifies LocationSystemInfoValidityType mask */
using LocationSystemInfoValidity = uint32_t;

/** Specify location system information.*/
struct LocationSystemInfo {
    /** validity of LocationSystemInfo::info*/
    LocationSystemInfoValidity valid;
    /** Current leap second and leap second info.*/
    LeapSecondInfo   info;
};

/**
 *  Specify the valid fields in GnssEnergyConsumedInfo. */
enum GnssEnergyConsumedInfoValidityType {
    /** validity of GnssEnergyConsumedInfo*/
    ENERGY_CONSUMED_SINCE_FIRST_BOOT_BIT = (1<<0)
};

/** Specifies GnssEnergyConsumedInfoValidityType */
using GnssEnergyConsumedInfoValidity = uint16_t;

/** Specify the info regarding energy consumed by GNSS
 *  engine.*/
struct GnssEnergyConsumedInfo {
    /** Bitwise OR of GnssEnergyConsumedInfoValidityType to
     *  specify the valid fields in GnssEnergyConsumedInfo.*/
    GnssEnergyConsumedInfoValidity valid;

    /** Energy consumed by the modem GNSS engine since device first
     *  ever bootup, in unit of 0.1 milli watt seconds.
     *  For an invalid reading, INVALID_ENERGY_CONSUMED is returned.*/
    uint64_t energySinceFirstBoot;
};

/**
 *  Specifies the set of aiding data. This is referenced in the
 *  deleteAidingData for deleting any aiding data. */
enum AidingDataType {
    /** Mask to delete ephemeris aiding data */
    AIDING_DATA_EPHEMERIS  = (1 << 0),
    /** Mask to delete calibration data from dead reckoning position engine */
    AIDING_DATA_DR_SENSOR_CALIBRATION = (1 << 1),
};

/** Specifies AidingDataType mask */
using AidingData = uint32_t;

/**
 *  Specifies the set of terrestrial technologies. */
enum TerrestrialTechnologyType {
    /** Cell-based technology */
    GTP_WWAN = (1 << 0),
};

/** Specifies TerrestrialTechnologyType mask */
using TerrestrialTechnology = uint32_t;

/**
 *  Specifies the HLOS generated NMEA sentence types. */
enum NmeaSentenceType {
    /** GGA NMEA sentence */
    GGA = (1 << 0),
    /** RMC NMEA sentence */
    RMC = (1 << 1),
    /** GSA NMEA sentence */
    GSA = (1 << 2),
    /** VTG NMEA sentence */
    VTG = (1 << 3),
    /** GNS NMEA sentence */
    GNS = (1 << 4),
    /** DTM NMEA sentence */
    DTM = (1 << 5),
    /** GPGSV NMEA sentence for SVs from GPS constellation */
    GPGSV = (1 << 6),
    /** GLGSV NMEA sentence for SVs from GLONASS constellation */
    GLGSV = (1 << 7),
    /** GAGSV NMEA sentence for SVs from GALILEO constellation */
    GAGSV = (1 << 8),
    /** GQGSV NMEA sentence for SVs from QZSS constellation */
    GQGSV = (1 << 9),
    /** GBGSV NMEA sentence for SVs from BEIDOU constellation */
    GBGSV = (1 << 10),
    /** GIGSV NMEA sentence for SVs from NAVIC constellation */
    GIGSV = (1 << 11),
    /** All NMEA sentences */
    ALL = 0xffffffff,
};

/** Specifies NmeaSentenceType mask */
using NmeaSentenceConfig = uint32_t;

/** Specify the Geodetic datum for NMEA sentence types that are generated. */
enum class GeodeticDatumType {
    /** No type*/
    GEODETIC_TYPE_NONE = -1,
    /** Geodetic datum type to indicate the use of World Geodetic System 1984 (WGS84) system */
    GEODETIC_TYPE_WGS_84 = 0,
    /** Geodetic datum type to indicate the use of PZ90/GLONASS system */
    GEODETIC_TYPE_PZ_90 = 1,
};

/** Specify the Nmea Config Parameters */
struct NmeaConfig {
    /** Specify the sentences to be configured. */
    NmeaSentenceConfig sentenceConfig = NmeaSentenceType::ALL;
    /** Specify the datum type to be configured. */
    GeodeticDatumType datumType = GeodeticDatumType::GEODETIC_TYPE_WGS_84;
    /**
     * Specify the Engine type for which Nmea sentences should be generated.
     * Also refer to  @ref ILocationConfigurator::configureNmea and
     * @ref ILocationManager::startDetailedEngineReports to understand the usage further.
    */
    LocReqEngine engineType = LocReqEngineType::LOC_REQ_ENGINE_FUSED_BIT;
};

/** Specify the valid mask for robust location configuration
 *  used by the GNSS standard position engine (SPE). */
enum RobustLocationConfigType {
    /** Validity of enabled */
    VALID_ENABLED          = (1<<0),
    /** Validity of enabledForE911. */
    VALID_ENABLED_FOR_E911 = (1<<1),
    /** Validity of version. */
    VALID_VERSION          = (1<<2)
};

/** Specifies RobustLocationConfigType mask */
using RobustLocationConfig = uint16_t;

/** Specify the versioning info of robust location module for
 *  the GNSS standard position engine (SPE). */
struct RobustLocationVersion {
    /** Major version number. */
    uint8_t major;
    /** Minor version number. */
    uint16_t minor;
};

/** Specify the robust location configuration used by the GNSS
 *  standard position engine (SPE) */
struct RobustLocationConfiguration {
    /** Validity mask */
    RobustLocationConfig validMask;
    /** Specify whether robust location feature is enabled or
     *  not. */
    bool enabled;
    /** Specify whether robust location feature is enabled or not
     *  when device is on E911 call. */
    bool enabledForE911;
    /** Specify the version info of robust location module used
     *  by the GNSS standard position engine (SPE). */
    RobustLocationVersion version;
};

/** Specify the valid mask for the configuration parameters of
 *  dead reckoning position engine */
enum DRConfigValidityType {
    /** Validity of body to sensor mount parameters. */
    BODY_TO_SENSOR_MOUNT_PARAMS_VALID    = (1<<0),
    /** Validity of vehicle speed scale factor. */
    VEHICLE_SPEED_SCALE_FACTOR_VALID     = (1<<1),
    /** Validity of vehicle speed scale factor uncertainty. */
    VEHICLE_SPEED_SCALE_FACTOR_UNC_VALID = (1<<2),
    /** Validity of gyro scale factor. */
    GYRO_SCALE_FACTOR_VALID              = (1<<3),
    /** Validity of gyro scale factor uncertainty. */
    GYRO_SCALE_FACTOR_UNC_VALID          = (1<<4),
};

/** Specifies DRConfigValidityType */
using DRConfigValidity = uint16_t;

/**
 * Specify vehicle body-to-Sensor mount parameters for use
 * by dead reckoning positioning engine. */
struct BodyToSensorMountParams {
    /** The misalignment of the sensor board along the
     *  horizontal plane of the vehicle chassis measured looking
     *  from the vehicle to forward direction.
     *  In unit of degrees.
     *  Range: [-180.0, 180.0].*/
    float rollOffset;
    /** The misalignment along the horizontal plane of the vehicle
     *  chassis measured looking from the vehicle to the right
     *  side. Positive pitch indicates vehicle is inclined such
     *  that forward wheels are at higher elevation than rear
     *  wheels.
     *  In unit of degrees.
     *  Range: [-180.0, 180.0].*/
    float yawOffset;
    /** The angle between the vehicle forward direction and the
     *  sensor axis as seen from the top of the vehicle, and
     *  measured in counterclockwise direction.
     *  In unit of degrees.
     *  Range: [-180.0, 180.0].*/
    float pitchOffset;
    /** Single uncertainty number that may be the largest of the
     *  uncertainties for roll offset, pitch offset and yaw
     *  offset.
     *  In unit of degrees.
     *  Range: [-180.0, 180.0].*/
    float offsetUnc;
};

/**
 *  Specifies the set of gnss reports. */
enum GnssReportType {
    /** Location reports */
    LOCATION          = (1 << 0),
    /** Satellite reports */
    SATELLITE_VEHICLE = (1 << 1),
    /**
     * To receive updates via @ref ILocationListener::onGnssNmeaInfo,
     * clients need to set this bit in the reportMask parameter passed to
     * @ref ILocationManager::startDetailedReports and
     * @ref ILocationManager::startDetailedEngineReports.
     *
     * Clients should set NMEA if they only need sentences from FUSED engine
     * Or set ENGINE NMEA if they need sentences from specific engine types.
     * Clients should never set both.
     *
     * Also refer to @ref ILocationManager::startDetailedEngineReports
     * to understand the usage further.
     */
    NMEA              = (1 << 2),
    /** Data reports */
    DATA              = (1 << 3),
    /** Low rate measurement reports. Currently the rate is defined to be 1 Hz. */
    MEASUREMENT       = (1 << 4),
    /** High rate measurement reports. Currently the rate is defined to be 10 Hz.
     *  Client cannot specify rates. The data in high rate would be different that from low rate.
     *  Also there might be difference in accuracy of fields for the both the rates. */
    HIGH_RATE_MEASUREMENT    = (1 << 5),
    /*Disaster Crisis Reports*/
    DISASTER_CRISIS   = (1 << 6),
    /**
     * To receive updates via @ref ILocationListener::onEngineNmeaInfo,
     * clients need to set this bit in the reportMask parameter passed to
     * @ref ILocationManager::startDetailedEngineReports.
     *
     * Clients should set NMEA if they only need sentences from FUSED engine
     * Or set ENGINE NMEA if they need sentences from specific engine types.
     * Clients should never set both.
     *
     * Also refer to @ref ILocationManager::startDetailedEngineReports
     * to understand the usage further.
     */
    ENGINE_NMEA       = (1 << 7),
    /**
     * To receive updates via @ref ILocationListener::onGnssEphemerisInfo,
     * clients need to set this bit in the reportMask parameter passed to
     * @ref ILocationManager::startDetailedReports and
     * @ref ILocationManager::startDetailedEngineReports.
     *
     * These reports are obtained only from the GNSS(SPE) engine
     * whenever there is an update in the ephemeris information for a constellation.
     */
    EPHEMERIS         = (1 << 8),
    /** GNSS extended data */
    EXTENDED_DATA     = (1 << 9)
};

/** Specifies the applicable reports using the bits represented in GnssReportType */
using GnssReportTypeMask = uint32_t;

/**< 0xffffffff indicates all the reports. All the reports but ENGINE_NMEA
     will be enabled by default if no specific report masks are specified.
     ENGINE_NMEA and NMEA are mutually exclusive. */
const uint32_t DEFAULT_GNSS_REPORT = (0xffffffff ^ ENGINE_NMEA);

/** Specify the dead reckoning engine configuration parameters.
 */
struct DREngineConfiguration {
    /** Specify the valid fields. */
    DRConfigValidity validMask;
    /** Body to sensor mount parameters used by dead reckoning
     *  positioning engine. */
    BodyToSensorMountParams mountParam;
    /** Vehicle Speed Scale Factor configuration input for the dead reckoning positioning engine.
     *  The multiplicative scale factor is applied to the received Vehicle Speed value
     *  (in meter/second) to obtain the true Vehicle Speed. Range is [0.9 to 1.1].
     *  Note: The scale factor is specific to a given vehicle make & model. */
    float speedFactor;
    /** Vehicle Speed Scale Factor Uncertainty (68% confidence) configuration input for the dead
     *  reckoning positioning engine. Range is [0.0 to 0.1].
     *  Note: The scale factor uncertainty is specific to a given vehicle make & model. */
    float speedFactorUnc;
    /** Gyroscope Scale Factor configuration input for the dead reckoning positioning engine. The
     *  multiplicative scale factor is applied to received gyroscope value to obtain the true
     *  value. Range is [0.9 to 1.1].
     *  Note: The scale factor is specific to the Gyroscope sensor and typically derived from
     *  either sensor data-sheet or from actual calibration. */
    float gyroFactor;
    /** Gyroscope Scale Factor uncertainty (68% confidence) configuration input for the dead
     *  reckoning positioning engine. Range is [0.0 to 0.1].
     *  Note: The scale factor uncertainty is specific to the Gyroscope sensor and typically
     *  derived from either sensor data-sheet or from actual calibration. */
    float gyroFactorUnc;
};

/**
 * Define the set of constellations for secondary band.
 */
typedef std::unordered_set<GnssConstellationType> ConstellationSet;

/**
 * Specify the position engine types */
enum class EngineType {
    /** Unknown engine type. */
    UNKNOWN = -1,
    /** Standard GNSS position engine. */
    SPE = 1,
    /** Precise position engine. */
    PPE = 2,
    /** Dead reckoning position engine. */
    DRE = 3,
    /** Vision positioning engine. */
    VPE = 4
};

/**
 * Specify the position engine run state */
enum class LocationEngineRunState {
    /** Unknown engine run state. */
    UNKNOWN = -1,
    /**
     * Request the position engine to be put into suspended state.
     * When put in this state the QDR engine will discard calibration data.
     *
    */
    SUSPENDED = 1,
    /** Request the position engine to be put into running state. */
    RUNNING = 2,
    /**
     * Request the position engine to be put into suspend state while
     * retaining any calibration data.
     * While configuring this engine state via @ref ILocationConfigurator::configureEngineState,
     * the vehicle is expected to be stationary and should be set to RUNNING
     * before the vehicle is expected to move(for example,on Ignition On).
     * This state is applicable when the client expects QDR to retain necessary data for
     * subsequent resume/reboot while being suspended.
     *
     */
    SUSPEND_RETAIN = 3
};

/**
 * Specify the status of the report */
enum class ReportStatus {
    /** Report status is unknown. */
    UNKNOWN = -1,
    /** Report status is successful. The engine is able to calculate the desired fix. Most
     *  of the fields in ILocationInfoEx will be valid. */
    SUCCESS = 0,
    /** Report is still in progress. The engine has not completed its calculations when this
     *  report was generated. Accuracy of various fields is non-optimal. Only some of the fields
     *  in ILocationInfoEx will be valid. */
    INTERMEDIATE = 1,
    /** Report status has failed. The engine is not able to calculate the fix. Most of the fields
     *  in ILocationInfoEx will be invalid. */
    FAILURE = 2
};

/**
 * Specify the logcat debug level during XTRA's param configuration. Currently, only XTRA
 * daemon will support the runtime configuration of the debug log level.
 */
enum class DebugLogLevel {
    /** No message is logged. */
    DEBUG_LOG_LEVEL_NONE = 0,
    /** Only error level debug messages will get logged. */
    DEBUG_LOG_LEVEL_ERROR = 1,
    /** Only warning and error level debug messages will get logged. */
    DEBUG_LOG_LEVEL_WARNING = 2,
    /** Only info, warning and error level debug messages will get logged. */
    DEBUG_LOG_LEVEL_INFO = 3,
    /** Only debug, info, warning and error level debug messages will get logged. */
    DEBUG_LOG_LEVEL_DEBUG = 4,
    /** Verbose, debug, info, warning and error level debug messages will get logged. */
    DEBUG_LOG_LEVEL_VERBOSE = 5,
};

/** Xtra feature configuration parameters */
struct XtraConfig {
    /**
     * Number of minutes between periodic, consecutive successful
     * XTRA assistance data downloads.
     *
     * If 0 is specified, modem default download for XTRA
     * assistance data will be performed.
     */
    uint32_t downloadIntervalMinute;
    /**
     * Connection timeout when connecting backend for both xtra
     * assistance data download and NTP time download.
     *
     * If 0 is specified, the download timeout value will use
     * device default values.
     */
    uint32_t downloadTimeoutSec;
    /**
     * Interval to wait before retrying for xtra assistance data's
     * download in case of failure.
     *
     * If 0 is specified, XTRA download retry will follow device
     * default behavior and downloadRetryAttempts will also use device
     * default value.
     */
    uint32_t downloadRetryIntervalMinute;
    /**
     * Total number of allowed retry attempts for assistance data's
     * download in case of failure.
     *
     * If 0 is specified, XTRA download retry will follow device
     * default behavior and downloadRetryIntervalMinute will also use
     * device default value.
     */
    uint32_t downloadRetryAttempts;
     /**
     * Path to the certificate authority (CA) repository that needs
     * to be used for XTRA assistance data download.
     * If empty string is specified, device default CA repositaory
     * will be used.
     */
    std::string caPath;
    /**
     * URLs from which XTRA assistance data will be fetched.
     * At least one and up to three URLs need to be configured when
     * this API is used.
     *
     * The URLs, if provided, shall include the port number to be
     * used for download.
     *
     * Valid xtra server URLs should start with "https://".
     *
     * Example of a valid URL : https://path.exampleserver.net:443
     *
     */
    std::vector<std::string> serverURLs;
    /**
     * URLs for NTP server to fetch current time.
     *
     * If no NTP server URL is provided, then device will use the
     * default NTP server.
     *
     * The URLs, if provided, shall include the port number to be
     * used for download.
     *
     * Example of a valid ntp server URL is:
     * ntp.exampleserver.com:123.
     */
    std::vector<std::string> ntpServerURLs;
    /**
     * Enable or disable XTRA integrity download.
     *
     * true: enable XTRA integrity download.
     * false: disable XTRA integrity download.
     */
    bool isIntegrityDownloadEnabled;
    /**
     *  Download interval for xtra integrity, only applicable
     *  if XTRA integrity download is enabled.
     *
     *  If 0 is specified, the download timeout value will use
     *  device default value.
     */
    uint32_t integrityDownloadIntervalMinute;
    /** Level of debug log messages that will be logged. */
    DebugLogLevel daemonDebugLogLevel;
    /** URL of NTS KE Server.
     *
     *  The URL, if provided, shall be complete and shall include
     *  the port number.
     *
     *  Max of 128 bytes, including null-terminating byte will be
     *  supported.
     *
     *  Valid NTS KE server URL should start with "https://".
     *
     *  If NTS KE server URL is not specified, then device will use
     *  the default URL of https://nts.xtracloud.net:4460.
     */
    std::string ntsServerURL;
    /**
     * Enable or disable diag logging for Xtra.
     *
     * false : disable the diag logging for Xtra.
     * true  : enable the diag logging for Xtra.
     *
     */
    bool isDiagLoggingEnabled;
};

/** Provides the status of the previously downloaded Xtra data. */
enum class XtraDataStatus {
    /**
     * If XTRA feature is disabled or if XTRA feature is enabled,
     * but XTRA daemon has not yet retrieved the assistance data
     * status from modem on early stage of device bootup, xtra data
     * status will be unknown.
     */
    STATUS_UNKNOWN = 0,
    /** If XTRA feature is enabled, but XTRA data is not present on the device. */
    STATUS_NOT_AVAIL = 1,
    /** If XTRA feature is enabled, XTRA data has been downloaded ever but no longer valid. */
    STATUS_NOT_VALID = 2,
    /** If XTRA feature is enabled, XTRA data has been downloaded and is currently valid. */
    STATUS_VALID = 3,
};

/** Specify Xtra assistant data's current status, validity and whether it is enabled. */
struct XtraStatus {
    /** XTRA assistance data and NTP time download is enabled or disabled. */
    bool featureEnabled;
    /**
     * XTRA assistance data status. If XTRA assistance data
     * download is not enabled, this field will be set to
     * XTRA_DATA_STATUS_UNKNOWN.
     */
    XtraDataStatus xtraDataStatus;
    /**
     * Number of hours that xtra assistance data will remain valid.
     *
     * This field will be valid when xtraDataStatus is set to
     * XTRA_DATA_STATUS_VALID.
     * For all other XtraDataStatus, this field will be set to 0.
     */
    uint32_t xtraValidForHours;
    /** User consent to avail the Xtra assistance service. */
    bool userConsent;
};

/** Enum of all the possible indications invoked by a Location Configurator listener.  */
enum LocConfigIndicationsType {
    /** Register to receive Xtra status updates. */
    LOC_CONF_IND_XTRA_STATUS = 0,
    /** Register to receive Gnss signal updates. */
    LOC_CONF_IND_SIGNAL_UPDATE
};

/** This bitset represents the list of the Location Config Indications selected by the Client. */
using LocConfigIndications = std::bitset<32>;

/**
 * @deprecated This Enum is no longer supported.
 * Use @ref ILocationInfoEx::NavigationSolutionType to get the required information about
 * solutions (corrections) used in the fix.
 */
/**
 * Specify set of navigation solutions that contribute to Gnss Location.
 * Defines Satellite Based Augmentation System(SBAS) corrections.
 * SBAS contributes to improve the performance of GNSS system.
 */
enum SbasCorrectionType {
  SBAS_CORRECTION_IONO, /**< Bit mask to specify whether
                             SBAS ionospheric correction is used */
  SBAS_CORRECTION_FAST, /**< Bit mask to specify whether
                             SBAS fast correction is used */
  SBAS_CORRECTION_LONG, /**< Bit mask to specify whether
                             SBAS long correction is used */
  SBAS_INTEGRITY, /**< Bit mask to specify whether
                      SBAS integrity information is used */
  SBAS_CORRECTION_DGNSS, /**< Bit mask to specify whether
                              SBAS DGNSS correction is used */
  SBAS_CORRECTION_RTK, /**< Bit mask to specify whether
                            SBAS RTK correction is used */
  SBAS_CORRECTION_PPP, /**< Bit mask to specify whether
                            SBAS PPP correction is used */
  SBAS_CORRECTION_RTK_FIXED, /**< Bit mask to specify whether
                            SBAS RTK fixed correction is used */
  SBAS_CORRECTION_ONLY_SBAS_CORRECTED_SV_USED_, /**< Bit mask to specify
                            only SBAS corrected SV is used */
  SBAS_COUNT  /**< Bitset */
};
/**
 * @deprecated This bitmask is no longer supported.
 * Use @ref ILocationInfoEx::NavigationSolution to get the required information about
 * solutions (corrections) used in the fix.
 */
/**
 * Bit mask that denotes which of the SBAS corrections in SbasCorrection used
 * to improve the performance of GNSS output.
 */
using SbasCorrection = std::bitset<SBAS_COUNT>;

/**
 * @brief ILocationInfoBase provides interface to get basic position related
 * information like latitude, longitude, altitude, timestamp.
 *
 */
class ILocationInfoBase {
public:
/**
 * Retrieves the validity of the Location basic Info.
 *
 * @returns Location basic validity mask.
 *
 */
  virtual LocationInfoValidity getLocationInfoValidity() = 0;

/**
 * Retrieves technology used in computing this fix.
 *
 * @returns Location technology mask.
 *
 */
  virtual LocationTechnology getTechMask() = 0;

/**
 * Retrieves Speed.
 *
 * @returns speed in meters per second.
 *
 */
  virtual float getSpeed() = 0;
/**
 * Retrieves latitude.
 * Positive and negative values indicate northern and southern latitude
 * respectively
 *    - Units: Degrees
 *    - Range: -90.0 to 90.0
 *
 * @returns Latitude if available else returns NaN.
 *
 */
  virtual double getLatitude() = 0;

/**
 * Retrieves longitude.
 * Positive and negative values indicate eastern and western longitude
 * respectively
 *    - Units: Degrees
 *    - Range: -180.0 to 180.0
 *
 * @returns Longitude if available else returns NaN.
 *
 */
  virtual double getLongitude() = 0;

/**
 * Retrieves altitude above the WGS 84 reference ellipsoid.
 *    - Units: Meters
 *
 * @returns Altitude if available else returns NaN.
 *
 */
  virtual double getAltitude() = 0;

/**
 * Retrieves heading/bearing.
 *    - Units: Degrees
 *    - Range: 0 to 360
 *
 * @returns Heading if available else returns NaN.
 *
 */
  virtual float getHeading() = 0;

/**
 * Retrieves the horizontal uncertainty.
 *    - Units: Meters
 * Uncertainty is defined with 68% confidence level.
 *
 * @returns Horizontal uncertainty.
 *
 */
  virtual float getHorizontalUncertainty() = 0;
/**
 * Retrieves the vertical uncertainty.
 *    - Units: Meters
 * Uncertainty is defined with 68% confidence level.
 *
 * @returns Vertical uncertainty if available else returns NaN.
 *
 */
  virtual float getVerticalUncertainty() = 0;

/**
 * Retrieves UTC timeInfo for the location fix.
 *    - Units: Milliseconds since Jan 1, 1970
 *
 * @returns TimeStamp in milliseconds if available else returns UNKNOWN_TIMESTAMP
 * which is zero(as UTC timeStamp has elapsed since January 1, 1970, it cannot be 0)
 *
 */
  virtual uint64_t getTimeStamp() = 0;

/**
 * Retrieves 3-D speed uncertainty/accuracy.
 *    - Units: Meters per Second
 * Uncertainty is defined with 68% confidence level.
 *
 * @returns Speed uncertainty if available else returns NaN.
 *
 */
  virtual float getSpeedUncertainty() = 0;

/**
 * Retrieves heading uncertainty.
 *    - Units: Degrees
 *    - Range: 0 to 360
 * Uncertainty is defined with 68% confidence level.
 *
 * @returns Heading uncertainty if available else returns NaN.
 *
 */
  virtual float getHeadingUncertainty() = 0;

/**
 * Boot timestamp corresponding to the UTC timestamp for Location fix.
 *    - Units: Nano-second
 *
 * @returns elapsed real time.
 *
 */
  virtual uint64_t getElapsedRealTime() = 0;

/**
 * Retrieves elapsed real time uncertainty.
 *    - Units: Nano-second
 *
 * @returns elapsed real time uncertainty.
 *
 */
  virtual uint64_t getElapsedRealTimeUncertainty() = 0;

/**
 * Retrieves time uncertainty.
 * For PVT report from SPE engine, confidence level is at 99%.
 * For PVT reports from other engines, confidence level is undefined.
 *
 * @return - Time uncertainty in milliseconds.
 *
 */
  virtual float getTimeUncMs() = 0;

/**
 * Retrieves elapsed gPTP time. GPTP time field corresponding to source time ticks.
 * Used for time sync between different systems. Validity of this field is given by value of
 * @ref LocationValidityType::HAS_GPTP_TIME_BIT
 *    - Units: Nanoseconds
 *
 * @returns elapsed gPTP time
 *
 */
  virtual uint64_t getElapsedGptpTime() = 0;

/**
 * Retrieves elapsed gPTP time uncertainty. Validity of this field is given by value of
 * @ref LocationValidityType::HAS_GPTP_TIME_UNC_BIT
 *    - Units: Nanoseconds
 *
 * @returns elapsed gPTP time uncertainty
 *
 */
  virtual uint64_t getElapsedGptpTimeUnc() = 0;

};

/**
 * @brief ILocationInfoEx provides interface to get richer position related
 * information like latitude, longitude, altitude and other information like time stamp,
 * session status, dop, reliabilities, uncertainties etc.
 *
 */
class ILocationInfoEx : public ILocationInfoBase {
public:

/**
 * Retrives the validity of the location info ex. It provides the validity of various information
 * like dop, reliabilities, uncertainties etc.
 *
 * @returns Location ex validity mask
 */
  virtual LocationInfoExValidity getLocationInfoExValidity() = 0;

/**
 * Retrieves the altitude with respect to mean sea level.
 *    - Units: Meters
 *
 * @returns Altitude with respect to mean sea level if available else returns
 * NaN.
 *
 */
  virtual float getAltitudeMeanSeaLevel() = 0;

/**
 * Retrieves position dilution of precision.
 *
 * @returns Position dilution of precision if available else returns NaN.
 * Range: 0 (highest accuracy) to 50 (lowest accuracy)
 *
 */
  virtual float getPositionDop() = 0;

/**
 * Retrieves horizontal dilution of precision.
 *
 * @returns Horizontal dilution of precision if available else returns NaN.
 * Range: 0 (highest accuracy) to 50 (lowest accuracy)
 *
 */
  virtual float getHorizontalDop() = 0;

/**
 * Retrieves vertical dilution of precision.
 *
 * @returns Vertical dilution of precision if available else returns NaN
 * Range: 0 (highest accuracy) to 50 (lowest accuracy)
 *
 */
  virtual float getVerticalDop() = 0;
/**
 * Retrieves geometric dilution of precision.
 *
 * @returns geometric dilution of precision.
 * Range: 0 (highest accuracy) to 50 (lowest accuracy)
 *
 */
  virtual float getGeometricDop() = 0;
/**
 * Retrieves time dilution of precision.
 *
 * @returns Time dilution of precision.
 * Range: 0 (highest accuracy) to 50 (lowest accuracy)
 *
 */
  virtual float getTimeDop() = 0;

/**
 * Retrieves the difference between the bearing to true north and the bearing
 * shown on magnetic compass. The deviation is positive when the magnetic
 * north is east of true north.
 *    - Units: Degrees
 *
 * @returns Magnetic Deviation if available else returns NaN
 *
 */
  virtual float getMagneticDeviation() = 0;

/**
 * Specifies the reliability of the horizontal position.
 *
 * @returns @ref LocationReliability of the horizontal position if available
 * else returns
 * UNKNOWN.
 *
 */
  virtual LocationReliability getHorizontalReliability() = 0;

/**
 * Specifies the reliability of the vertical position.
 *
 * @returns @ref LocationReliability of the vertical position if available
 * else returns UNKNOWN.
 *
 */
  virtual LocationReliability getVerticalReliability() = 0;

/**
 * Retrieves semi-major axis of horizontal elliptical uncertainty.
 *    - Units: Meters
 * Uncertainty is defined with 39% confidence level.
 *
 * @returns Semi-major horizontal elliptical uncertainty if available else
 * returns NaN.
 *
 */
  virtual float getHorizontalUncertaintySemiMajor() = 0;

/**
 * Retrieves semi-minor axis of horizontal elliptical uncertainty.
 *    - Units: Meters
 * Uncertainty is defined with 39% confidence level.
 *
 * @returns Semi-minor horizontal elliptical uncertainty
 * if available else returns NaN.
 *
 */
  virtual float getHorizontalUncertaintySemiMinor() = 0;

/**
 * Retrieves elliptical horizontal uncertainty azimuth of orientation.
 *    - Units: Decimal degrees
 *    - Range: 0 to 180
 * Confidence for uncertainty is not specified.
 *
 * @returns Elliptical horizontal uncertainty azimuth of orientation
 * if available else returns NaN.
 *
 */
  virtual float getHorizontalUncertaintyAzimuth() = 0;
/**
 * Retrieves east standard deviation.
 *    - Units: Meters
 * Uncertainty is defined with 68% confidence level.
 *
 * @returns East Standard Deviation.
 *
 */
  virtual float getEastStandardDeviation() = 0;

/**
 * Retrieves north standard deviation.
 *    - Units: Meters
 * Uncertainty is defined with 68% confidence level.
 *
 * @returns North Standard Deviation.
 *
 */
  virtual float getNorthStandardDeviation() = 0;

/**
 * Retrieves number of satellite vehicles used in position report.
 *
 * @returns number of Sv used.
 *
 */
  virtual uint16_t getNumSvUsed() = 0;

/**
 * Retrives the set of satellite vehicles that are used to calculate position.
 *
 * @returns set of satellite vehicles for different constellations.
 */
  virtual SvUsedInPosition getSvUsedInPosition() = 0;

/**
 * Retrieves GNSS Satellite Vehicles used in position data.
 *
 * @param [out] idsOfUsedSVs Vector of Satellite Vehicle identifiers.
 *
 */
  virtual void getSVIds(std::vector<uint16_t> &idsOfUsedSVs) = 0;

/**
 * Retrieves navigation solution mask used to indicate solutions used in the fix.
 *
 * @return - Navigation solution mask used.
 *
 */
  virtual NavigationSolution getNavigationSolution() = 0;

/**
 * Retrieves position technology mask used to indicate which technology is used.
 *
 * @return - Position technology used in computing this fix.
 *
 */
  virtual GnssPositionTech getPositionTechnology() = 0;

/**
 * Retrieves position related information.
 *
 */
  virtual GnssKinematicsData getBodyFrameData() = 0;

/**
 * Retrieves gnss measurement usage info.
 *
 */
  virtual std::vector<GnssMeasurementInfo> getmeasUsageInfo() = 0;

/**
 * Retrieves type of gnss system.
 *
 * @return - Type of Gnss System.
 *
 */
  virtual SystemTime getGnssSystemTime() = 0;

/**
 * Retrieves leap seconds if available.
 *
 * @param [out] leapSeconds - leap seconds
 *       - Units: Seconds
 *
 * @returns Status of leap seconds.
 *
 */
  virtual telux::common::Status getLeapSeconds(uint8_t &leapSeconds) = 0;

/**
 * Retrieves east, North, Up velocity if available.
 *
 * @param [out] velocityEastNorthUp - east, North, Up velocity
 *       - Units: Meters/second
 *
 * @returns Status of availability of east, North, Up velocity.
 *
 */
  virtual telux::common::Status
      getVelocityEastNorthUp(std::vector<float> &velocityEastNorthUp) = 0;

/**
 * Retrieves east, North, Up velocity uncertainty if available.
 * Uncertainty is defined with 68% confidence level.
 *
 * @param [out] velocityUncertaintyEastNorthUp - east, North, Up velocity
 * uncertainty
 *       - Units: Meters/second
 *
 * @returns Status of availability of east, North, Up velocity uncertainty.
 *
 */
  virtual telux::common::Status getVelocityUncertaintyEastNorthUp(
      std::vector<float> &velocityUncertaintyEastNorthUp) = 0;

/**
 * Sensor calibration confidence percent, range [0, 100].
 *
 * @returns the percentage of calibration taking all the parameters into account.
 *
 */
  virtual uint8_t getCalibrationConfidencePercent() = 0;

/**
 * Sensor calibration status.
 *
 * @returns mask indicating the calibration status with respect to different parameters.
 *
 */
  virtual DrCalibrationStatus getCalibrationStatus() = 0;

/**
 * DR solution status.
 *
 * @returns mask indicating the solution status with respect to the DR position engine.
 *
 */
  virtual DrSolutionStatus getSolutionStatus() = 0;

/**
 * Location engine type. When the type is set to LOC_ENGINE_SRC_FUSED, the fix is
 * the propagated/aggregated reports from all engines running on the system (e.g.:
 * DR/SPE/PPE) based QTI algorithm. To check which location engine contributes
 * to the fused output, check for locOutputEngMask.
 *
 * @returns the type of engine that was used for calculating the position fix.
 *
 */
  virtual LocationAggregationType getLocOutputEngType() = 0;

/**
 * When loc output eng type is set to fused, this field indicates the set of engines
 * contribute to the fix.
 *
 * @returns the combination of position engines used in calculating the position report
 * when the loc output end type is set to fused.
 *
 */
  virtual PositioningEngine getLocOutputEngMask() = 0;

/**
 * When robust location is enabled, this field will indicate how well the various input
 * data considered for navigation solution conforms to expectations.
 *
 * @returns values in the range [0.0, 1.0], with 0.0 for least conforming and 1.0 for
 * most conforming.
 *
 */
  virtual float getConformityIndex() = 0;

/**
 * Vehicle Reference Point(VRP) based latitude, longitude and altitude information.
 *
 */
  virtual LLAInfo getVRPBasedLLA() = 0;

/**
 * VRP-based east, north and up velocity information.
 * @returns - vector of directional velocities in this order {east velocity, north velocity,
 *            up velocity}
 */
  virtual std::vector<float> getVRPBasedENUVelocity() = 0;

/**
 * Determination of altitude is assumed or calculated. ASSUMED means there may not be
 * enough satellites to determine the precise altitude.
 * @returns altitude type ASSUMED/CALCULATED or if not avalilable then UNKNOWN.
 */
  virtual AltitudeType getAltitudeType() = 0;

/**
 * Indicates the status of this report in terms of how optimally the report was calculated
 * by the engine.
 *
 * @returns Status of the report. Returns ReportStatus::UNKNOWN if status is unavailable.
 */
  virtual ReportStatus getReportStatus() = 0;

/**
 * Integrity risk used for protection level parameters. Unit of 2.5e-10.
 * Valid range is [1 to (4e9-1)]. Values other than valid range means integrity risk is disabled
 * and @ref ILocationInfoEx::getProtectionLevelAlongTrack,
 * @ref ILocationInfoEx::getProtectionLevelCrossTrack and
 * @ref ILocationInfoEx::getProtectionLevelVertical will not be available.
 *
 */
  virtual uint32_t getIntegrityRiskUsed() = 0;

/**
 * Along-track protection level at specified integrity risk, in unit of meter.
 *
 */
  virtual float getProtectionLevelAlongTrack() = 0;

/**
 * Cross-track protection level at specified integrity risk, in unit of meter.
 *
 */
  virtual float getProtectionLevelCrossTrack() = 0;

/**
 * Vertical component protection level at specified integrity risk, in unit of meter.
 *
 */
  virtual float getProtectionLevelVertical() = 0;

/**
 * Retrieves navigation solution mask used to indicate SBAS corrections.
 *
 * @return - SBAS (Satellite Based Augmentation System) Correction mask used.
 *
 * @deprecated This API is no longer supported.
 * Use @ref ILocationInfoEx::getNavigationSolution to get the required information about
 * solutions (corrections) used in the fix.
 *
 */
  virtual SbasCorrection getSbasCorrection() = 0;

/** List of DGNSS station IDs providing corrections.
 *  Range:
 *  - SBAS --  120 to 158 and 183 to 191
 *  - Monitoring station -- 1000-2023 (Station ID biased by 1000)
 *  - Other values reserved.
 */
  virtual std::vector<uint16_t> getDgnssStationIds() = 0;

 /** Distance between the basestation and the receiver.
  *  Units: meter.
 */
  virtual double getBaselineLength() = 0;

 /** Difference in time between the fix timestamp using the correction
  * and the time of the correction data.
  * Units: milliseconds.
 */
  virtual uint64_t getAgeOfCorrections() = 0;

/**
 * Returns the leap seconds uncertainty associated with the PVT report.
 * Units- seconds
 */
  virtual uint8_t getLeapSecondsUncertainty() = 0;
};

/**
 * @brief ISVInfo provides interface to retrieve information
 *        about Satellite Vehicles, their position and health status
 */
class ISVInfo {
public:
/**
 * Indicates to which constellation this satellite vehicle belongs.
 *
 * @returns  @ref GnssConstellationType if available else returns UNKNOWN.
 *
 */
  virtual GnssConstellationType getConstellation() = 0;

/**
 * GNSS satellite vehicle ID.
 * SV id range of each supported constellations mentioned in @ref GnssMeasurementInfo.
 *
 * @returns Identifier of the satellite vehicle otherwise 0(as 0 is not an ID
 * for any of the SVs)
 *
 */
  virtual uint16_t getId() = 0;

/**
 * Health status of satellite vehicle.
 *
 * @returns  HealthStatus of Satellite Vehicle if available else returns
 * UNKNOWN.
 *          - @ref SVHealthStatus
 * @deprecated This API is not supported.
 *
 */
  virtual SVHealthStatus getSVHealthStatus() = 0;

/**
 * Status of satellite vehicle.
 *
 * @note    This API is work-in-progress and is subject to change.
 * @returns Satellite Vehicle Status if available else returns UNKNOWN.
 *          - @ref SVStatus
 * @deprecated This API is not supported.
 *
 */
  virtual SVStatus getStatus() = 0;

/**
 * Indicates whether ephemeris information(which allows the receiver
 * to calculate the satellite's position) is available.
 *
 * @returns @ref SVInfoAvailability if Ephemeris exists or not else returns
 * UNKNOWN.
 *
 */
  virtual SVInfoAvailability getHasEphemeris() = 0;

/**
 * Indicates whether almanac information(which allows receivers to know
 * which satellites are available for tracking) is available.
 *
 * @returns @ref SVInfoAvailability if almanac exists or not else returns
 * UNKNOWN.
 *
 */
  virtual SVInfoAvailability getHasAlmanac() = 0;

/**
 * Indicates whether the satellite is used in computing the fix.
 *
 * @returns @ref SVInfoAvailability, if satellite used or not else returns
 * UNKNOWN.
 *
 */
  virtual SVInfoAvailability getHasFix() = 0;

/**
 * Retrieves satellite vehicle elevation angle.
 *    - Units: Degrees
 *    - Range: 0 to 90
 *
 * @returns Elevation if available else returns NaN.
 *
 */
  virtual float getElevation() = 0;

/**
 * Retrieves satellite vehicle azimuth angle.
 *    - Units: Degrees
 *    - Range: 0 to 360
 *
 * @returns Azimuth if available else returns NaN.
 */
  virtual float getAzimuth() = 0;

/**
 * Retrieves signal-to-noise ratio of the signal measured at antenna of the satellite vehicle.
 *    - Units: dB-Hz
 *
 * @returns SNR if available else returns 0.0 value.
 *
 */
  virtual float getSnr() = 0;

/**
 * Indicates the carrier frequency of the signal tracked.
 *
 * @returns carrier frequency in Hz else returns UNKNOWN_CARRIER_FREQ frequency
 * when not supported.
 */
  virtual float getCarrierFrequency() = 0;

/**
 * Indicates the validity for different types of signal
 * for gps, galileo, beidou etc.
 *
 * @returns signalType mask else return UNKNOWN_SIGNAL_MASK when not supported.
 */
  virtual GnssSignal getSignalType() = 0;

 /**
  * Retrieves GLONASS frequency channel number in the range [1, 14] which is calculated as
  * FCN [-7, 6] + 8.
  *
  * @returns GLONASS frequency channel number.
  */
   virtual uint16_t getGlonassFcn() = 0;

/**
 * Carrier-to-noise ratio of the signal measured at baseband.
 *    - Units: dB-Hz
 *
 * @returns carrier-to-noise ratio at baseband else returns UNKNOWN_BASEBAND_CARRIER_NOISE ratio
 * when not supported.
 */
  virtual double getBasebandCnr() = 0;
};

/**
 * @brief IGnssSVInfo provides interface to retrieve the list of SV info
 * available and whether altitude is assumed or calculated.
 */
class IGnssSVInfo {
public:
/**
 * Indicates whether altitude is assumed or calculated.
 *
 * @returns @ref AltitudeType if available else returns UNKNOWN.
 * @deprecated This API is not supported.
 *
 */
  virtual AltitudeType getAltitudeType() = 0;

/**
 * Pointer to satellite vehicles information for all GNSS
 * constellations.
 *
 * @returns Vector of pointer of ISVInfo object if available else returns
 * empty vector.
 *
 */
  virtual std::vector<std::shared_ptr<ISVInfo> > getSVInfoList() = 0;
};

/**
 * @brief IGnssSignalInfo provides interface to retrieve GNSS data information
 * like jammer metrics and automatic gain control for satellite signal type.
 */
class IGnssSignalInfo {
public:
/**
 * Retrieves jammer metric and Automatic Gain Control(AGC) corresponding to signal types.Jammer metric is
 * linearly proportional to the sum of jammer and noise power at the GNSS
 * antenna port.
 *
 * @returns List of jammer metric and a list of automatic gain control for signal type.
 *
 */

  virtual GnssData getGnssData() = 0;
};

/** @} */ /* end_addtogroup telematics_location */

} // end of namespace loc
} // end of namespace telux

#endif // TELUX_LOC_LOCATIONDEFINES_HPP
