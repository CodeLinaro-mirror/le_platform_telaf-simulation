/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef TELUX_CV2X_PROP_CONGESTIONCONTROLDEFINES_HPP
#define TELUX_CV2X_PROP_CONGESTIONCONTROLDEFINES_HPP


#include <vector>
#include <string>
#include <memory>
#include <semaphore.h>

namespace telux {
namespace cv2x {
namespace prop {

/** @addtogroup telematics_cv2x_cpp
 * @{ */

enum class CCErrorCode {
   SUCCESS = 0,                          /**< No error */
   GENERIC_FAILURE = 1,                  /**< Generic Failure */
   NO_PERMISSION = 2,                    /**< No permission */
};

/**
 * Type of congestion control
 */
enum class CongestionControlType {
    /** Default type of congestion control.
     * Based on J3161/1 and J2945/1. */
    SAE
};

/**
 * Position in terms of latitude, longitude, and elevation along with its heading.
 */
struct Position  {
    /** Latitude, in unit of degrees, range [-90.0, 90.0].*/
    double posLat;
    /** Longitude, in unit of degrees, range [-180.0, 180.0]. */
    double posLong;
    /** Heading, in unit of degrees, range [0 to 359.999]. */
    double heading;
    /**  Altitude above the WGS 84 reference ellipsoid, in unit of meters.*/
    double elev; // unused
};

/**
 * sub per interval data (5 times per RV)
 */
struct SubPERInterData  {
    /** First message count of a vehicle in a PacketErrorRate sub interval */
    uint64_t msgCntFirst;
    /** Last message count of a vehicle in a PacketErrorRate sub interval */
    uint64_t msgCntLast;
    /** Total number of recieved messages of a vehicle in a PacketErrorRate sub interval */
    uint64_t rxCnt;
    /** Specifies if the remote vehicle's data is still valid for PER calculation */
    bool isValid;
};

/**
 * per interval data (1 time per RV)
 */
struct PERInterData  {
    /** First message count of a vehicle in a PacketErrorRate interval */
    uint64_t msgCntFirst;
    /** Last message count of a vehicle in a PacketErrorRate interval */
    uint64_t msgCntLast;
    /** Current message count of a vehicle in a PacketErrorRate interval */
    uint64_t msgCntCurr;
    /** Total number of expected messages from a Remote Vehicle
     *  this PacketErrorRate interval */
    uint64_t totalExpectMsgs;
    /** Total actually received messages from a Remote Vehicle in a
     * PacketErrorRate interval */
    uint64_t totalRxMsgs;
    /** Total calculated missed messages from a Remote Vehicle in a
      * PacketErrorRate interval */
    uint64_t totalMissMsgs;
    /** Calculated Packet Error Rate for a Remote Vehicle based on
      * totalExpectMsgs, totalRxMsgs, totalMissMsgs in this PacketErrorRate interval */
    double packetErrRate;
      /** The most recent PacketErrorRate */
    double lastPacketErrRate;
};

/**
 * data for each vehicle
 * general information provided from the sender's message contents
 * and also contains information updated while congestion control is running
 * such as packet error rate (per) data
 */
struct CongestionControlData {
    /** Latest position of this vehicle */
    Position pos;
    /** Latest speed of this vehicle */
    double speed;
    /** Latest received message time stamp of this vehicle */
    uint64_t rxTimeStamp; //ms
    /** Latest msg cnt of this per interval (filled by the client) */
    uint64_t currMsgCnt;
    /** Packet error rate data for this vehicle updated every PER sub interval */
    std::vector<struct SubPERInterData> subPERInterData;
    /** Packet error rate data for this vehicle updated every PER interval */
    PERInterData packetErrInterData;
    /** Flag indicating a new packet error rate (PER) sub interval is occurring */
    bool newPERSubInterval;
    /** Flag indicating this vehicle is in range within a specified threshold */
    bool inRange;
    /** Timestamp of last sent message */
    uint64_t lastTxMsgTime; //ms
    /** Latest GNSS fix time */
    uint64_t lastGnssFixTime;
    /** Latest calculated tracking error. The difference between the last assumed known
     *  position and the assumed estimated position. */
    double trackingErr;
};

/**
 * channel quality related data
 */
struct ChannelData {
    /** Unfiltered channel busy percentage */
    double rawCbp;
    /** Filtered and calculated channel busy ratio */
    double channBusyRatio;
    /** Last channel busy ratio */
    double lastChannBusyRatio;
    /** Latest interval's packet error rate */
    double packetErrorRate;
    /**  Latest channel quality indication value */
    double channQualInd;
};


/**
 * Output for sps enhancements
 */
struct SpsEnhanceData {
    /** Upper hysteresis-based SPS periodicity threshold */
    uint64_t upperHystThresh;
    /** Lower hysteresis-based SPS periodicity threshold */
    uint64_t lowerHystThresh;
    /** SPS periodicity rounded to nearest valid periodicity */
    uint64_t roundedSpsInterval;
    /** Percentage of hysteresis for threshold calculation */
    double hysteresis;
};

/**
 * Data for tracking error calculation
 */
struct TrackingErrorData {
    /** Last position sent out via message */
    Position lastPosSent;
    /** Last speed sent out via message */
    double lastSpeedSent;
    /** Last heading sent out via message */
    double lastHeadingSent;
    /** Curent position */
    Position currPos;
    /** Current speed */
    double currSpeed;
    /** Current heading */
    double currHeading;
};

/**
 * Output of congestion control
 */
struct CongestionControlCalculations {
    /** Alert the user to update their max ITT value if it needs to be */
    bool updateMaxITT;
    /** Current max Inter-Transmit Time  */
    uint64_t maxITT;
    /** Alert the user to send a critical BSM via event flow */
    bool sendCriticalMsg;
    /** Alert the user to send at this moment */
    bool sendNow;
    /**  New priority for the next packet sent OTA  */
    uint64_t priority;
    /** New transmit power */
    uint64_t txPower;
    /** Latest calculated tracking error */
    double trackingError;
    /** Latest calculated smoothed average density */
    double smoothDens;
    /** Latest unsmoothed density in range */
    int totalRvsInRange;
    /** Latest calculated channel quality indicator and packet error rates */
    std::shared_ptr<ChannelData> channData;
    /** SPS flow changes that may need to be made for congestion control */
    std::shared_ptr<SpsEnhanceData> spsEnhanceData;
};

/**
 * User-provided struct for congestion control outputs
 * Will contain the relevant information to let user know when to TX
 * and also other settings/data that it should change/use for congestion control.
 */
struct CongestionControlUserData {
    /** Pointer to be set to a TransmitFlow based class or data struct */
    void* spsTransmit;
    /** Flag to let the manager know that sps enhancements are enabled */
    bool spsEnhancementsEnabled;
    /** Output for the congestion control algorithm for user */
    std::shared_ptr<CongestionControlCalculations> congestionControlCalculations;
    /** Semaphore to prevent any race conditions when using output */
    sem_t* congestionControlSem;
};

/**
 * Config for all SPS Enhancements
 */
struct SPSEnhanceConfig {
    /** The current SPS periodicity
     *  Supported values are 20, 50, and multiples of 100 */
    uint64_t spsPeriodicity;
    /** The chance for actually updating maximum inter-transmit time
     *  and also the SPS periodicity of the current SPS flow. */
    uint64_t changeFrequency; // default is 1/5 or 20%
    /** A percentage which expands the range of hysteresis thresholds
     *  to prevent volatile changes in maximum inter-transmit time and
     *  SPS periodicity. */
    double hysterPercent;
};

/**
 * Config for density calculation
 */
struct DensityConfig {
    /** Density weight factor for lambda parameterized smoothing function */
    double densWeightFactor;
    /** Minimum distance threshold to consider a vehicle relevant for PER
     *  calculations. */
    uint64_t distThresh;
    /** Density coefficient constant which is part of maximum Inter-Transmit Time
     *  calculations. The smaller the value, the more sensitive the calculation. */
    double densCoeff;
};

/**
 * Config for packet error rate calculation
 */
struct PERConfig {
    /** Time interval between each periodic packet error rate calculation */
    int packetErrorInterval;
    /** Each subinterval time period for packet error rate calculation */
    int packetErrorSubInterval;
    /** Numer of subintervals per Packet Error rATE interval.
     * Equivalent to the interval time divided by subinterval time */
    int maxPERSubinters;
    /** Maximum packet error rate threshold. Anything above is capped to this. */
    double maxPacketErrorRate;
};

/**
 * Config for channel quality calculation
 */
struct CQIConfig {
    /** Channel quality indication threshold */
    uint64_t threshold; // for capping channel quality indication
};

/**
 * Config for channel busy percentage
 */
struct CBPConfig {
    /** Weight factor in calculating the CBP from raw CBP */
    double cbpWeightFactor;
    /** Time interval between each periodic CBP calculation */
    uint64_t cbpInterval;
};

/**
 * Config for tracking error (TE).

 */
struct TEConfig {
    /** Interval for calculating the tracking error and
     * determining new Inter-Transmit Time (ITT) */
    uint64_t txCtrlInterval;
    /** Minimum HV position estimate delay used to calculate the HV local estimate */
    uint64_t hvMinTimeDiff;
    /** Maximum HV position estimate delay used to calculate the HV local estimate */
    uint64_t hvMaxTimeDiff;
    /** Minimum delay used to calculate the where RV estimates the HV to be */
    uint64_t rvMinTimeDiff;
    /** Minimum delay used to calculate the where RV estimates the HV to be */
    uint64_t rvMaxTimeDiff;
    /** Minimum communications-based error threshold */
    double teLowerThresh;
    /** Maximum tracking error upper threshold. Used to determine whether to
     * send a BSM or not*/
    double teUpperThresh;
    /** For calculating the probability of transmission based on tracking error */
    uint64_t errSensitivity;
};

/**
 * Config for inter-transmit time
 */
struct ITTConfig {
    /** Threshold for making decision to update ITT or not */
    uint64_t reschedThresh;
    /** Time resolution in time */
    uint64_t timeAccuracy;
    /** Minimum Inter-transmit time threshold */
    uint64_t minIttThresh;
    /** Maximum Inter-transmit time threshold */
    uint64_t maxIttThresh;
    /** Random chance for not sending */
    uint64_t txRand;
};

/**
 * Config for power
 */
struct PowerConfig {
    /** Minimum permitted radiated power */
   uint64_t minRadiPwr;
    /** Maximum permitted radiated power */
    uint64_t maxRadiPwr;
};

/**
 * Config for congestion control which contains sub-config items
 */
struct CongestionControlConfig {
    /** Power calculation Configuration parameters */
    PowerConfig pwrConfig;
    /** Channel Busy Percentage calculation Configuration parameters */
    CBPConfig cbpConfig;
    /** Channel Quality Indication calculation Configuration parameters */
    CQIConfig cqiConfig;
    /** Packet Error Rate calculation Configuration parameters */
    PERConfig perConfig;
    /** Smoothed In-Range Density calculation Configuration parameters */
    DensityConfig densConfig;
    /** Tracking Error calculation Configuration parameters */
    TEConfig teConfig;
    /** Inter-Transmit Time calculation Configuration parameters */
    ITTConfig ittConfig;
    /** Flag to enable SPS enhancements */
    bool enableSpsEnhance;
    /** SPS Enhancements Configuration parameters */
    SPSEnhanceConfig spsEnhanceConfig;
    /** Type of congestion control to be used. SAE only supported today */
    CongestionControlType congestionControlType;
};

/**
 * Print Position items
 * @param [in] position - position struct reference
 * @todo use the TelSDK Logger mechanism which has options for console
 */
void printPosition(Position& position);

/**
 * Print ChannelData items
 * @param [in] channelData - channel data struct reference
 * @todo use the TelSDK Logger mechanism which has options for console
 */
void printChannelData(ChannelData& channelData);

/**
 * Print TrackingErrorData items
 * @param [in] teData - tracking error input struct reference
 * @todo use the TelSDK Logger mechanism which has options for console
 */
void printTrackingErrorData(TrackingErrorData& teData);

/**
 * Print SPSEnhanceConfig items
 * @param [in] spsEnhanceConfig - sps enhancements config struct reference
 * @todo use the TelSDK Logger mechanism which has options for console
 */
void printSPSEnhanceConfig(SPSEnhanceConfig& spsEnhanceConfig);

/**
 * Print DensityConfig items
 * @param [in] densConfig - density config struct reference
 * @todo use the TelSDK Logger mechanism which has options for console
 */
void printDensityConfig(DensityConfig& densConfig);

/**
 * Print PERConfig items
 * @param [in] perConfig - packet error rate config struct reference
 * @todo use the TelSDK Logger mechanism which has options for console
 */
void printPERConfig(PERConfig& perConfig);

/**
 * Print CQIConfig
 * @param [in] cqiConfig - channel quality indication config struct reference
 * @todo use the TelSDK Logger mechanism which has options for console
 */
void printCQIConfig(CQIConfig& cqiConfig);

/**
 * Print CBPConfig items
 * @param [in] cbpConfig - channel busy percentage config struct reference
 * @todo use the TelSDK Logger mechanism which has options for console
 */
void printCBPConfig(CBPConfig& cbpConfig);

/**
 * Print TEConfig items
 * @param [in] teConfig - tracking error config struct reference
 * @todo use the TelSDK Logger mechanism which has options for console
 */
void printTEConfig(TEConfig& teConfig);

/**
 * Print ITTConfig items
 * @param [in] ittConfig - inter transmit time config struct reference
 * @todo use the TelSDK Logger mechanism which has options for console
 */
void printITTConfig(ITTConfig& ittConfig);

/**
 * Print PowerConfig items
 * @param [in] powerConfig - power config struct reference
 * @todo use the TelSDK Logger mechanism which has options for console
 */
void printPowerConfig(PowerConfig& powerConfig);

/** @} */ /* end_addtogroup telematics_cv2x_cpp */
} // namespace prop
} // namespace cv2x
} // namespace telux

#endif // TELUX_CV2X_PROP_CONGESTIONCONTROLDEFINES_HPP
