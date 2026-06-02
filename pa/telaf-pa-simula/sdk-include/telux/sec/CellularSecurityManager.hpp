/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file  CellularSecurityManager.hpp
 * @brief CellularSecurityManager provides support for detecting, monitoring and
 *        generating security threat scan report for cellular connections.
 */

#ifndef TELUX_SEC_CELLULARSECURITYMANAGER_HPP
#define TELUX_SEC_CELLULARSECURITYMANAGER_HPP

#include <cstdint>

#include <telux/common/CommonDefines.hpp>
#include <telux/tel/PhoneDefines.hpp>

namespace telux {
namespace sec {

/** @addtogroup telematics_sec_mgmt
 * @{ */

/**
 *  Describes the cellular threats detected.
 */
enum class CellularThreatType {

    /**
     *  No scoring (analysis) has been performed yet or it is in-progress. For example;
     *  during cell reselection, a device may be camped to a new cell and may remain idle
     *  (not exchanging data over cellular network). During this time scoring is not done.
     *  When device uses cellular network for actual use, scoring is done.
     */
    UNKNOWN = (1 << 1),

    /**
     *  Base station (BS) configuration is preventing the device from connecting
     *  to the neighboring base stations.
     */
    IMPRISON = (1 << 2),

    /**
     *  BS intercepts or jams signals to and from the device such that it results
     *  in a denial of cellular service.
     */
    DOS = (1 << 3),

    /**
     *  BS is forcing the device to downgrade to use less secure cellular service.
     *  For example; downgrade from LTE to second-generation cellular network (2G).
     */
    DOWNGRADE = (1 << 4),

    /**
     *  BS is continuously tracking location of the device.
     */
    LOCATION_TRACKED_USING_IMSI = (1 << 5),

    /**
     *  BS is continuously tracking location of the device using the authentication process.
     */
    LOCATION_TRACKED_USING_AUTH = (1 << 6),

    /**
     *  BS portrays itself as the best option for the UE to select.
     */
    PERSUADE = (1 << 7),

    /**
     *  No threat has been detected for this base station.
     */
    NO_THREAT_DETECTED = (1 << 8),

    /**
     *  GSM EDGE radio access network (GERAN) BS is not using encryption.
     */
    NO_ENCRYPTION = (1 << 9),

    /**
     *  GERAN BS is using weak encryption.
     */
    WEAK_ENCRYPTION = (1 << 10),

    /**
     *  When using long-term evolution (LTE), BS blacklisted itself on physical layer cell
     *  identity (PCI) and E-UTRA absolute radio frequency channel number (EARFCN).
     */
    SELF_BLACKLISTING_CELL = (1 << 11),

    /**
     *  On a unauthenticated GERAN, a short message service (SMS) was received.
     */
    UNAUTHENTICATED_SMS = (1 << 12),

    /**
     *  On an unauthenticated GERAN, an emergency message was received.
     */
    UNAUTHENTICATED_EMERGENCY_MESSAGE = (1 << 13),

    /**
     * The international mobile subscriber identity (IMSI) of the device has leaked
     * in an unencrypted state to an unauthenticated base station.
     */
    IMSI_LEAK = (1 << 14)
};

/**
 *  Describes the state of the cellular environment observed by the device.
 */
enum class EnvironmentState {

    /**
     *  No scoring (analysis) has been performed yet or it is in-progress.
     */
    UNKNOWN,

    /**
     *  Device and base station have authenticated each other and connected.
     */
    SAFE,

    /**
     *  Environment is potentially unsafe to operate. There may be potentially malicious
     *  BS. The threat score has crossed configured threshold at least once. More analysis
     *  needed to conclude, if the environment is safe.
     *
     */
    ALERT,

    /**
     *  Environment is hostile and threats have been detected. For example,
     *  compromised/malicious base stations are detected in the environment.
     */
    HOSTILE,
};

/**
 *  Describes an overall cellular environment's information.
 */
struct EnvironmentInfo {

    /**
     *  Please refer @ref EnvironmentState for details.
     */
    EnvironmentState environmentState;
};

/**
 *  Based on the policy configured on the device, certain actions are taken automatically.
 *  For example; when a compromised/malicious BS is detected, it is blacklisted (cell
 *  barring) for a certain period of time (hence device will not be able to connect to it).
 *
 *  When configured action has been taken, a security report is generated. In that report,
 *  ActionType represents exact action taken.
 */
enum class ActionType {

    /**
     *  No specific action taken.
     */
    NONE,

    /**
     *  Priority of this cell for selection is reduced so that other cells get more priority
     *  for cell selection/reselection during device attempting to camp to a cell.
     */
    DEPRIORITIZED,

    /**
     *  Priority of this cell (previously deprioritized) for selection is resumed to regular
     *  status.
     */
    REMOVED_DEPRIORITIZATION,

    /**
     *  This cell has been barred (device will not camp to this cell).
     */
    CELL_BARRED,

    /**
     *  Cell barring has been removed from this previously barred cell. This cell can be
     *  considered for connection, during cell selection/reselection process.
     */
    REMOVED_CELL_BARRING,

    /**
     *  The configured action was outside the allowed range of actions.
     */
    INVALID
};

/**
 *  Defines all the cell info types.
 */
enum class RATType {
    /* Unknown */
    UNKNOWN = 1,

    /* Global system for mobile communications */
    GSM,

    /* Wideband code division multiple access */
    WCDMA,

    /* Long-term evolution */
    LTE,

    /* New radio fifth generation */
    NR5G
};

/**
 *  Represents security scan report for a cellular connection per base station.
 */
struct CellularSecurityReport {

    /**
     *  The higher the score higher the possibility of a compromised/malicious
     *  base station. The range of valid values for the score is configurable
     *  in the platform. The default range is 0 to 500.
     */
    uint32_t threatScore;

    /**
     *  Unique identifier of a cell operated by a mobile network operator.
     */
    uint32_t cellId;

    /**
     *  Physical cell id; identifier of a cell in the physical layer of the
     *  cellular technology.
     */
    uint32_t pid;

    /**
     * Mobile country code to uniquely identify a mobile network operator (carrier).
     */
    std::string mcc;

    /**
     * Mobile network code to uniquely identify a mobile network operator (carrier).
     */
    std::string mnc;

    /**
     * Types of the threat identified. Please refer @ref CellularThreatType for
     * more details.
     */
    std::vector<CellularThreatType> threats;

    /**
     *  Action taken based on the policy configured and threat score.
     */
    ActionType actionType;

    /**
     *  Radio access technology being used for communication between the device and
     *  the base station (2G/GERAN, 3G/WCDMA, 4G/LTE and 5G/NR).
     */
    RATType rat;
};

/**
 *  For the current session, it represents a high-level summary of the security stats
 *  gathered till now. This gives an overall idea about the operational cellular
 *  environment.
 *
 *  This can be useful in cases for example, to decide whether a security sensitive
 *  operation should be deferred to a later time or place with less hostile environment
 *  or extra preventive measures should be activated.
 */
struct SessionStats {

    /**
     *  Number of the reports received.
     */
    uint32_t reportsCount;

    /**
     *  Number of times hostile score threshold was crossed. This count depends on
     *  the value of the threshold configured in the platform. This count increments
     *  each time the threat score increases beyond this threshold.
     */
    uint32_t thresholdCrossedCount;

    /**
     *  Different types of threats detected.
     */
    std::vector<CellularThreatType> threats;

    /**
     *  An average score (average of @ref CellularSecurityReport::threatScore).
     */
    float averageThreatScore;

    /**
     *  Last action that was taken based on the policy configured, when a malicious
     *  activity was detected.
     */
    ActionType lastAction;

    /**
     *  Set to true, if an action was taken, when the score crossed hostile threshold.
     */
    bool anyActionTaken;
};

/**
 * Receives security scan reports when a change in cellular environment is detected.
 * For example;
 * 1. Device connects to a given cell tower.
 * 2. Device moves between different cell towers.
 * 3. A new cellular base station is detected.
 * 4. There is a change in the threat score beyond defined threshold.
 */
class ICellularScanReportListener : public telux::common::IServiceStatusListener {

 public:
    /**
     * Invoked to provide a security scan report for cellular connection environment.
     *
     * @param[in] report @ref CellularSecurityReport result of the cellular security scanning
     *
     * @param[in] environmentInfo @ref EnvironmentInfo overall environment information
     *
     */
    virtual void onScanReportAvailable(CellularSecurityReport report,
        EnvironmentInfo environmentInfo) { }

    /**
     * Destructor for ICellularScanReportListener.
     */
    virtual ~ICellularScanReportListener() { }
};

/**
 * Provides support for detecting, monitoring and generating security threat scan
 * report for cellular connections.
 *
 * When a change in the cellular operating environment is detected, information
 * about the environment is gathered and analyzed for targeted, general purpose
 * attacks and anomalies. This information is then provided as a security scan report.
 *
 * The report includes information such as, IMSI leak, tracking location of the device,
 * denial of service, man-in-the-middle attack, spam or phishing SMS, fake emergency
 * messages and rogue base stations.
 */
class ICellularSecurityManager {

 public:

   /**
    * Registers given listener to receive cellular security scan report.
    *
    * On platforms with access control enabled, caller needs to have TELUX_SEC_CCS_REPORT
    * permission to invoke this API successfully.
    *
    * @param [in] reportListener Receives security scan reports via
    *             @ref ICellularScanReportListener::onScanReportAvailable()
    *
    * @returns @ref telux::common::ErrorCode::SUCCESS, if the listener is registered,
    *          otherwise, an appropriate error code
    *
    * @note Eval: This is a new API and is being evaluated. It is subject
    *             to change and could break backwards compatibility.
    */
   virtual telux::common::ErrorCode registerListener(
        std::weak_ptr<ICellularScanReportListener> reportListener) = 0;

   /**
    * Unregisters the given listener registered previously with @ref registerListener().
    *
    * On platforms with access control enabled, caller needs to have TELUX_SEC_CCS_REPORT
    * permission to invoke this API successfully.
    *
    * @param [in] reportListener Listener to unregister
    *
    * @returns @ref telux::common::ErrorCode::SUCCESS, if the listener is deregistered,
    *          otherwise, an appropriate error code
    *
    * @note Eval: This is a new API and is being evaluated. It is subject
    *             to change and could break backwards compatibility.
    */
   virtual telux::common::ErrorCode deRegisterListener(
        std::weak_ptr<ICellularScanReportListener> reportListener) = 0;

   /**
    * Gets current session statistics such as average score, number of reports generated,
    * and threat types detected etc.
    *
    * A session starts when a listener is registered using
    * @ref ICellularSecurityManager::registerListener and ends when it is
    * deregistered using @ref ICellularSecurityManager::deRegisterListener.
    *
    * On platforms with access control enabled, caller needs to have TELUX_SEC_CCS_REPORT
    * permission to invoke this API successfully.
    *
    * @param [out] sessionStats @ref SessionStats will contain current session's stats upon
    *                           method return
    *
    * @returns Status @ref telux::common::ErrorCode::SUCCESS, if the stats are fetched
    *                 successfully, otherwise, an appropriate error code
    *
    * @note Eval: This is a new API and is being evaluated. It is subject
    *             to change and could break backwards compatibility.
    */
   virtual telux::common::ErrorCode getCurrentSessionStats(SessionStats& sessionStats) = 0;

   /**
    * Destructor of ICellularSecurityManager. Cleans up as applicable.
    */
   virtual ~ICellularSecurityManager() {};
};

/** @} */  // end_addtogroup telematics_sec_mgmt

}  // End of namespace sec
}  // End of namespace telux

#endif // TELUX_SEC_CELLULARSECURITYMANAGER_HPP
