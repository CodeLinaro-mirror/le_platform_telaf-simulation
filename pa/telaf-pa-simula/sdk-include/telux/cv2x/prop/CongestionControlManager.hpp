/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file       CongestionControlManager.hpp
 * @brief      CongestionControl Manager is a primary interface for CongestionControl related
 *             functionality.
 *             The Manager is used to provide config items for flow and calculations of
 *             Congestion Control. It provides inputs and outputs that are part of
 *             various parts of congestion control such as tracking error, inter-transmit time,
 *             packet error rate, density, and others. Outputs are provided to the user to notify
 *             when to send and change other related congestion control settings.
 */

#ifndef TELUX_CV2X_PROP_CONGESTIONCONTROLMANAGER_HPP
#define TELUX_CV2X_PROP_CONGESTIONCONTROLMANAGER_HPP
#include <telux/cv2x/prop/CongestionControlDefines.hpp>
#include <map>

namespace telux {
namespace cv2x {
namespace prop {

/** @addtogroup telematics_cv2x_cpp
 * @{ */

/**
 * uin64_t refers to the a vehicle's identity field provided in messages
 */
using CongestionControlMap = std::map<uint64_t, CongestionControlData>;

/**
 * @brief Utility class for congestion control logging and testing purposes.
 */
class CongestionControlUtility {
private:
    /**
     * uint8_t value for determining the verbosity level of congestion control manager
     */
    static uint8_t loggingLevel;
public:

    /**
     * Sets the logging level
     *
     * @param [in] loggingLevel - value to indicate level of console logging
     * @note -  Eval: This is a new API and is being evaluated. It is subject to change
     *          and could break backwards compatibility.
     */
    static void setLoggingLevel(uint8_t loggingLevelIn);

    /**
     * Gets the logging level
     *
     * @note -  Eval: This is a new API and is being evaluated. It is subject to change
     *          and could break backwards compatibility.
     * @returns uin8_t - the current logging level
     */
    static uint8_t getLoggingLevel();

    /**
     * Adds an artificial density over time
     * @param [in] density - artificial density that should be incorporated
     * @param [in] initDistance - the initial distance from host vehicle to artifical density
     * @note -  Eval: This is a new API and is being evaluated. It is subject to change
     *          and could break backwards compatibility.
     */
    static void addDensity(uint64_t density, uint64_t initDistance) {}
};

/**
 * @brief Congestion Control listeners implement this interface.
 */
class ICongestionControlListener {
public:
    /**
     * Called when the new congestion control data is available.
     *
     * @param [in] congestionControlUserData - pointer to output user data
     *      which manager will fill. Lets the user know they should immediately
     *      send a new message. If SPS enhancements are enabled, they may
     *      also need to perform SPS periodicity change.
     * @param [in] trackErrEvent - tells the listener that there is a tracking error event
     * @note -  Eval: This is a new API and is being evaluated. It is subject to change
     *          and could break backwards compatibility.
     */
    virtual void onCongestionControlDataReady(
        std::shared_ptr<CongestionControlUserData> congestionControlUserData,
        bool trackErrEvent) {}

    /** @param [in] newItt - updated inter-transmit time in milliseconds that the library computes
    * @note -  Eval: This is a new API and is being evaluated. It is subject to change
    *          and could break backwards compatibility.
    */
    virtual void onIttUpdate (uint64_t newItt) {}

    /**
     * Destructor for ICongestionControlListener
     */
    virtual ~ICongestionControlListener() {}
};


/**
 * @brief CongestionControl Manager is a primary interface for
 *        CongestionControl related functionality.
 */
class ICongestionControlManager {
public:

    /**
     * Called to update the internal config parameters with custom values
     *
     * @param [in] congestionControlConfigIn - a struct that holds various smaller config structs
     *                                         for each component of congestion control
     * @note -  Eval: This is a new API and is being evaluated. It is subject to change
     *          and could break backwards compatibility.
     * @returns CCErrorCode code meaning success or reason for error, if any
     */
    virtual CCErrorCode updateCongestionControlConfig(
        std::shared_ptr<CongestionControlConfig> congestionControlConfigIn) = 0;

    /**
     * Called to update the type of congestion control
     *
     * @param [in] congestionControlType - type of congestion control to be performed
     * @note -  Eval: This is a new API and is being evaluated. It is subject to change
     *          and could break backwards compatibility.
     * @returns CCErrorCode code meaning success or reason for error, if any
     */
    virtual CCErrorCode updateCongestionControlType(
        CongestionControlType congestionControlType) = 0;

    /**
     * The primary congestion control driver to be called after initialization.
     * Launches various threads for different components of congestion control.
     * Including channel quality, packet error rate, density, and inter-transmit
     * time calculations.
     *
     * @note -  Eval: This is a new API and is being evaluated. It is subject to change
     *          and could break backwards compatibility.
     * @returns CCErrorCode code meaning success or reason for error, if any
     */
    virtual CCErrorCode startCongestionControl() = 0;

    /**
     * Gracefully closes any lingering threads, semaphores, and cleans up any allocated data.
     *
     * @note -  Eval: This is a new API and is being evaluated. It is subject to change
     *          and could break backwards compatibility.
     * @returns CCErrorCode code meaning success or reason for error, if any
     */
    virtual CCErrorCode stopCongestionControl() = 0;

    /**
     * Called to register a ICongestionControlListener, which will be notified when new
     * congestion control data is ready.
     * @param [in] congCtrlListener - user-implemented class to listen for output user
     *      data which manager will fill.
     *      Lets the user know they should immediately send a new message.
     *      If SPS enhancements are enabled, they may perform SPS periodicity change.
     * @returns CCErrorCode code meaning success or reason for error, if any
     */

    virtual CCErrorCode registerListener(
        std::weak_ptr<ICongestionControlListener> congCtrlListener) = 0;

    /**
     * Called to deregister a ICongestionControlListener implementation
     * @param [in] congCtrlListener - user-implemented class to be deregistered
     * @returns CCErrorCode code meaning success or reason for error, if any
     */
    virtual CCErrorCode deregisterListener(
        std::weak_ptr<ICongestionControlListener> congCtrlListener) = 0;

    /**
    * Called to get a shared pointer to the results of the Congestion Control periodic calculations.
    * @note -  Eval: This is a new API and is being evaluated. It is subject to change
    *          and could break backwards compatibility.
    * @returns std::shared_ptr<CongestionControlUserData> - shared pointer reference to the
    *                          CongestionControlUserData that the manager will update.
    */
    virtual std::shared_ptr<CongestionControlUserData> getCongestionControlUserData() = 0;

    /**
     * Update the channel busy percentage related configs
     *
     * @param [in] cbpWeightFactor - weight factor for channel busy percentage calculation
     * @param [in] cbpInterval -  interval for channel busy percentage calculation
     * @note -  Eval: This is a new API and is being evaluated. It is subject to change
     *          and could break backwards compatibility.
     * @returns CCErrorCode code meaning success or reason for error, if any
     */
    virtual CCErrorCode updateCbpConfig(
        double cbpWeightFactor, uint64_t cbpInterval) = 0;

    /**
     * Update the packet error rate related configs
     *
     * @param [in] maxPacketErrorRate - maximum packet error rate
     * @param [in] packetErrorRateInterval - overall interval time for packet error calculation
     * @param [in] packetErrorRateSubInterval - sub interval time for packet error rate calculation
     * @note -  Eval: This is a new API and is being evaluated. It is subject to change
     *          and could break backwards compatibility.
     * @returns CCErrorCode code meaning success or reason for error, if any
     */
    virtual CCErrorCode updatePERConfig(
        double maxPacketErrorRate, int packetErrorRateInterval,
        int packetErrorRateSubInterval) = 0;

    /**
     * Update the density related configs
     *
     * @param [in] densCoeff - density coefficient for evaluating maximum inter-transmit time
     * @param [in] densWeightFactor - weight factor coefficient for smoothed density
     * @param [in] distThresh - threshold for minimal distance between host and remote vehicles
     * @note -  Eval: This is a new API and is being evaluated. It is subject to change
     *          and could break backwards compatibility.
     * @returns CCErrorCode code meaning success or reason for error, if any
     */
    virtual CCErrorCode updateDensConfig(double densCoeff,
        double densWeightFactor, uint64_t distThresh) = 0;

    /**
     * Update the tracking error related configs
     *
     * @param [in] txCtrlInterval -  interval for transmit rate control
     * @param [in] hvMinTimeDiff - min time difference between hv last known position and current
     * @param [in] hvMaxTimeDiff - max time difference between hv last known position and current
     * @param [in] rvMinTimeDiff - min time difference between rv last known position and current
     * @param [in] rvMaxTimeDiff - max time difference between rv last known position and current
     * @param [in] teLowerThresh - tracking error lower threshold
     * @param [in] teUpperThresh - tracking error upper threshold
     * @param [in] errSensitivity - tracking error sensitivity
     * @note -  Eval: This is a new API and is being evaluated. It is subject to change
     *          and could break backwards compatibility.
     * @returns CCErrorCode code meaning success or reason for error, if any
     */
    virtual CCErrorCode updateTeConfig(uint64_t txCtrlInterval,
        uint64_t hvMinTimeDiff, uint64_t hvMaxTimeDiff,
        uint64_t rvMinTimeDiff, uint64_t rvMaxTimeDiff,
        uint64_t teLowerThresh, uint64_t teUpperThresh,
        uint64_t errSensitivity) = 0;

    /**
     * Update the inter-transmit time related configs
     *
     * @param [in] reschedThresh - threshold for rescheduling frequency
     * @param [in] timeAccuracy - accuracy of time
     * @param [in] minIttThresh - minimum inter-transmit time
     * @param [in] maxIttThresh - maximum inter-transmit time
     * @param [in] txRand - transmission chance randomness
     * @note -  Eval: This is a new API and is being evaluated. It is subject to change
     *          and could break backwards compatibility.
     * @returns CCErrorCode code meaning success or reason for error, if any
     */
    virtual CCErrorCode updateIttConfig(uint64_t reschedThresh,
        uint64_t timeAccuracy, uint64_t minIttThresh,
        uint64_t maxIttThresh, uint64_t txRand) = 0;

    /**
     * Update the transmit rate control related configs
     *
     * @param [in] txCtrlInterval - time interval for periodic
     *      calculations of tracking error and inter-transmit time.
     * @note -  Eval: This is a new API and is being evaluated. It is subject to change
     *          and could break backwards compatibility.
     * @returns CCErrorCode code meaning success or reason for error, if any
     */
    virtual CCErrorCode updateTxRateCtrlInterval(uint64_t txCtrlInterval) = 0;

    /**
     * Update the sps enhancements related config
     *
     * @param [in] spsPeriodicity - New periodicity of sps flow to be set by user
     * @param [in] changeFrequency - Random chance to not change sps flow periodicity and
     *          inter-transmit time
     * @param [in] hysterPercent - Hysteresis percentage for choosing the inter-transmit time
     *             thresholds. Used to prevent frequent changes to sps flow periodicity.
     * @note -  Eval: This is a new API and is being evaluated. It is subject to change
     *          and could break backwards compatibility.
     * @returns CCErrorCode code meaning success or reason for error, if any
     */
    virtual CCErrorCode updateSpsEnhanceConfig(uint64_t spsPeriodicity,
        uint64_t changeFrequency, double hysterPercent) = 0;

    /**
     * Enables sps ennhancements
     *
     * @param [in] enable - Boolean to enable or disable sps enhancements
     * @note -  Eval: This is a new API and is being evaluated. It is subject to change
     *          and could break backwards compatibility.
     */
    virtual void enableSpsEnhancements(bool enable) = 0;

    /**
     * Used to check if sps enhancements are enabled
     *
     * @note -  Eval: This is a new API and is being evaluated. It is subject to change
     *          and could break backwards compatibility.
     * @returns CCErrorCode code meaning success or reason for error, if any
     */
    virtual bool isSpsEnhanceEnabled() = 0;

    /**
     * Used whenever the user needs to update latest host vehicle information to manager
     *
     * @param [in] pos - latest position (lat, long, elevation) of host vehicle
     * @param [in] speed - latest speed of host vehicle
     * @note -  Eval: This is a new API and is being evaluated. It is subject to change
     *          and could break backwards compatibility.
     * @returns CCErrorCode code meaning success or reason for error, if any
     */
    virtual CCErrorCode updateHostVehicleData(Position& pos, double speed) = 0;

    /**
     * Used whenever the user needs to update latest host vehicle information to manager
     *
     * @param [in] lastTxTime - latest tx time of host vehicle (if any) used for scheduling
     * @note -  Eval: This is a new API and is being evaluated. It is subject to change
     *          and could break backwards compatibility.
     * @returns CCErrorCode code meaning success or reason for error, if any
     */
    virtual CCErrorCode updateLastTxTime(uint64_t lastTxTime) = 0;

    /**
     * Update the host vehicle gnss fix time
     *
     * @param [in] gnssFixTimestamp - the new host vehicle gnss fix timestamp
     * @note -  Eval: This is a new API and is being evaluated. It is subject to change
     *          and could break backwards compatibility.
     * @returns CCErrorCode code meaning success or reason for error, if any
     */
    virtual CCErrorCode updateHvGnssFixTime(uint64_t gnssFixTimestamp) = 0;

    /**
     * Update the channel busy ratio
     *
     * @param [in] channBusyRatio - current channel busy ratio of host vehicle
     * @note -  Eval: This is a new API and is being evaluated. It is subject to change
     *          and could break backwards compatibility.
     * @returns CCErrorCode code meaning success or reason for error, if any
     */
    virtual CCErrorCode updateChannelBusyRate(double channBusyRatio) = 0;

    /**
     * Should be called when user detects a critical event.
     * This function notifies the congestion control manager about the critical event.
     * This is important so that the manager can update the internal transmit schedule
     * for a specified time.
     * @note -  Eval: This is a new API and is being evaluated. It is subject to change
     *          and could break backwards compatibility.
     * @returns CCErrorCode code meaning success or reason for error, if any
     */
    virtual CCErrorCode notifyCriticalEvent() = 0;

    /**
     * Called when user needs to notify congestion control to disable critical event
     *
     * @note -  Eval: This is a new API and is being evaluated. It is subject to change
     *          and could break backwards compatibility.
     * @returns CCErrorCode code meaning success or reason for error, if any
     */
    virtual CCErrorCode disableCriticalEvent() = 0;

    /**
     * Called whenever there is a packet received from new vehicle nearby
     *
     * @param [in] id - A remote vehicle identity
     * @param [in]  - latitude - latitude of the vehicle
     * @param [in]  - longitude - longitude of the vehicle
     * @param [in]  - heading - heading of the vehicle
     * @param [in]  - speed - speed of the vehicle
     * @param [in]  - timestamp - timestamp at which the message was received
     * @param [in]  - msgCount - Current message count taken from the decoded message
     * @note -  Eval: This is a new API and is being evaluated. It is subject to change
     *          and could break backwards compatibility.
     * @returns CCErrorCode code meaning success or reason for error, if any
     */
    virtual CCErrorCode addCongestionControlData(uint64_t id,
        double latitude, double longitude,
        double heading, double speed,
        uint64_t timestamp, uint64_t msgCount) = 0;

    /**
     * Called when we need to remove data related to a vehicle
     *
     * @param [in] id - A remote vehicle identity whose CongestionControlData will be removed
     *                  from the manager. This API is called when a remote vehicle has left
     *                  a specified range or is no longer important for congestion control.
     * @note -  Eval: This is a new API and is being evaluated. It is subject to change
     *          and could break backwards compatibility.
     * @returns CCErrorCode code meaning success or reason for error, if any
     */
    virtual CCErrorCode removeCongestionControlData(uint64_t id) = 0;

    /**
     * Called when user needs to access a nearby vehicle's latest congestion control data
     *
     * @param [in] id - A remote vehicle identity
     * @note -  Eval: This is a new API and is being evaluated. It is subject to change
     *          and could break backwards compatibility.
     * @returns CongestionControlData copy of the data the manager has and uses
     *          for a given vehicle id, if any.
     */
    virtual std::shared_ptr<CongestionControlData> getCongestionControlData(
        uint64_t id) = 0;

    ICongestionControlManager();
    ~ICongestionControlManager();

};
/** @} */ /* end_addtogroup telematics_cv2x_cpp */

}  // End of namespace prop
}  // End of namespace cv2x
}  // End of namespace telux
#endif // TELUX_CV2X_PROP_CONGESTIONCONTROLMANAGER_HPP
