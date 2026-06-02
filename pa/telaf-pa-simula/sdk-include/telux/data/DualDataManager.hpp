/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file       DualDataManager.hpp
 *
 * @brief      The DualDataManager class provides APIs to manage dual data
 *             connectivity. For example, you can use the DualDataManager class to:
 *             - Check the dual data capability of the device.
 *             - Check the dual data usage recommendation.
 *             - Perform DDS switch
 *             - Request current DDS SIM slot
 *             - Request recommended DDS SIM slot
 *             - Register for listener APIs to be notified about dual data changes
 *               (capability, usage recommendation, DDS recommendation).
 * Key points:
 * Dual SIM Dual Active (DSDA): In this case, both SIMs can operate independently. There are two
 *          radio resources available, allowing the SIMs to work independently. For example, a voice
 *          call can be made on SIM slot 1, and a data call can be made on SIM slot 2. Additionally,
 *          data calls can be started on both SIM slots.
 *
 * Dual SIM Dual Standby (DSDS): In this case, only one radio resource is available for use, which
 *          is time-shared between the SIMs. Both SIM slots use the same frequency band to transmit
 *          data, achieving maximum throughput. Voice calls can be received on either SIM, but
 *          long-running data calls cannot be started on both SIM slots simultaneously.
 *
 * Default Data Subscription (DDS): When the device is in DSDS mode or switches from DSDA to DSDS,
 *          long-running data calls are expected to run on the DDS SIM slot only. In scenarios like
 *          a voice call on the non-DDS SIM, the data call should temporarily switch to the non-DDS
 *          SIM to avoid loss of data service.
 */

#ifndef TELUX_DUAL_DATA_MANAGER_HPP
#define TELUX_DUAL_DATA_MANAGER_HPP

#include <memory>

#include <telux/common/CommonDefines.hpp>
#include <telux/data/DataDefines.hpp>

namespace telux {
namespace data {
/** @addtogroup telematics_data
 * @{ */

enum class DualDataUsageRecommendation
{
    ALLOWED = 0,            /** Long running data calls on both SIM slots is allowed. */
    NOT_ALLOWED = 1,        /** Long running data calls are not allowed on both SIM slots.
                                Data activities must be stopped on the nDDS slot. */
    NOT_RECOMMENDED = 2,    /** Recommendation is to have long running data calls only on the
                                DDS SIM slot. In this case, it is expected that data activities
                                on nDDS SIM slot are stopped. If data activities are continued
                                on both the slots for longer duration, the performance would be
                                degraded. */
};

/**
 * This function is called in response to requestCurrentDds API.
 *
 * The callback can be invoked from multiple different threads.
 * The implementation should be thread safe.
 *
 * @param [in] currentState  Provides the current DDS status @ref telux::data::DdsInfo.
 * @param [in] error         Return code for whether the operation succeeded or failed.
 *
 */
using RequestCurrentDdsRespCb = std::function<void(DdsInfo currentState,
    telux::common::ErrorCode error)>;

/**
 * Specifies which factor should be considered when the modem makes a recommendation of DDS. For
 * e.g., if the client sets THROUGHPUT as the recommendation factor, then the modem will favor the
 * SIM slot which is capable of higher throughput when making a recommendation for DDS.
 */
enum class DDSRecommendationBasis {
    THROUGHPUT = 1,     /**< DDS recommendation based on throughput */
    LATENCY             /**< DDS recommendation based on latency */
};

/**
 * Configuration for DDS recommendation
 */
struct DdsSwitchRecommendationConfig {
    DDSRecommendationBasis recommBasis;     /**< DDS recommendation based on throughput or latency */
    bool enableTemporaryRecommendations;    /**< Enable recommendations for temporary DDS switches */
    bool enablePermanentRecommendations;    /**< Enable recommendations for permanent DDS switches */
};

/**
 * Temporary recommendation type
 */
enum class TemporaryRecommendationType {
    UNKNOWN = 0,
    REVOKE,     /**< Revoke the previous temporary DDS recommendation sent.
                    - If not acted on previous recommendation then no need to switch.
                    - If the client already switched based on the previous temporary recommendation,
                      switch back. User recommended to switch to SIM slot mentioned in
                      @ref DdsInfo::slotId.
                     Actions to be performed upon revocation depend on the specific scenario. For more information about scenarios, refer to @ref TemporaryRecommendationCauseCode. */
    LOW,        /**< Recommends switching with low priority. Switching will enhance data service quality. */
    HIGH        /**< Highly recommends switching DDS immediately. Failure to switch will result in loss of data service. */
};

/**
 * Cause code for temporary recommendation.
 */
enum TemporaryRecommendationCauseCode {
    TEMP_CAUSE_CODE_UNKNOWN = 0,
    TEMP_CAUSE_CODE_DSDA_IMPOSSIBLE = (1 << 0), /**< Voice call started on nDDS SIM slot and device is in DSDS mode or moved from DSDA mode to DSDS mode. It is recommended to perform a temporary switch to nDDS SIM slot. */
    TEMP_CAUSE_CODE_DDS_INTERNET_UNAVAIL = (1 << 1), /**< Device is in DSDA mode, voice call started on nDDS SIM slot and then DDS internet throttled or DDS out of service. It is recommended to perform a temporary switch to nDDS SIM slot. */
    TEMP_CAUSE_CODE_TX_SHARING = (1 << 2), /**< In DSDA mode, voice call started on nDDS SIM slot and device moved to Tx sharing state. It is recommended to perform a temporary switch to nDDS SIM slot. */
    // 4th bit (1 << 3) is reserved for future use case
    TEMP_CAUSE_CODE_CALL_STATUS_CHANGED = (1 << 4), /**< Voice call/e-call ended. Temporary recommendation type will be revoke and it is recommended to do a permanent switch back to original DDS SIM slot i.e., SIM slot specified in the @ref DdsSwitchRecommendation */
    TEMP_CAUSE_CODE_ACTIVE_CALL_ON_DDS = (1 << 5), /**< There was a voice call on nDDS which caused a temporary recommendation. But now the current DDS voice call is on hold, and there is an active voice call on the original DDS SIM slot for more than 20 seconds. It will result in a temporary recommendation type revoke and the user is expected to perform a temporary switch back to the original DDS. */
    TEMP_CAUSE_CODE_TEMP_REC_DISABLED = (1 << 6), /**< A temporary recommendation was previously sent (temporary switch to nDDS). Now, temporary DDS switch disabled via @ref configureDdsSwitchRecommendation. This results in a temporary recommendation type revoke as there will be no more temporary recommendations forthcoming. */
    // 8th bit (1 << 7) is reserved for future use case
    TEMP_CAUSE_CODE_NON_DDS_INTERNET_UNAVAIL = (1 << 8), /**< There was a temporary recommendation to switch to nDDS in the past and the user has not acted on it (performed switch) yet. Now conditions have changed as a result of nDDS internet throttling. This results in nDDS no longer being recommended. This cause code comes along with a temporary recommendation type revoke to indicate the previous recommendation is no longer valid. */
    TEMP_CAUSE_CODE_DATA_OFF = (1 << 9), /**< There was a temporary recommendation to switch to nDDS in the past and the user has not acted on it (performed switch) yet. Now conditions have changed as a result of nDDS data being disabled, or roaming setting being disabled and the device is in a roaming area. This results in nDDS no longer being recommended. This cause code comes along with a temporary recommendation type revoke to indicate the previous recommendation is no longer valid. */
    TEMP_CAUSE_CODE_EMERGENCY_CALL_ON_GOING = (1 << 10), /**< Emergency call started on nDDS SIM slot. It is recommended to perform a temporary switch to nDDS SIM slot. */
    TEMP_CAUSE_CODE_DDS_SIM_REMOVED = (1 << 11) /**< As a result of a voice call, there was a temporary recommendation to switch to nDDS in the past. Subsequently, the original DDS SIM slot was removed. Now, after the voice call ends, it will result in this cause code with a revocation of the previous temporary recommendation. The expectation from the user in this case is to perform a permanent switch to nDDS as the DDS SIM slot has been removed. */
};

/**
 * Bitmask containing TemporaryRecommendationCauseCode bits, e.g., a value of 0x400 represents
 * an ongoing emergency call. Multiple cause codes are possible.
 */
using TemporaryRecommendationCauseCodes = uint64_t;

/**
 * Cause code for permanent recommendation.
 */
enum PermanentRecommendationCauseCode {
    PERM_CAUSE_CODE_UNKNOWN = 0,                 /**< Unknown cause. */
    PERM_CAUSE_CODE_TEMP_CLEAN_UP = (1 << 0),    /**< A temporary recommendation was previously sent (temporary switch to nDDS). Now, the modem is evaluating a permanent switch recommendation due to reasons such as the SIM slot being out of service, data being off, or roaming data being off while in a roaming area, etc. It is recommended to make a permanent switch to the SIM slot specified in the @ref DdsSwitchRecommendation. */
    PERM_CAUSE_CODE_DATA_SETTING_OFF = (1 << 1), /**< Data setting, for example, roaming, is not enabled, and the DDS SIM slot entered a roaming area. */
    PERM_CAUSE_CODE_PS_INVALID = (1 << 2),       /**< PS (Packet Switching) became invalid, resulting in the internet PDU session being released on the DDS SIM slot. */
    PERM_CAUSE_CODE_INTERNET_NOT_AVAIL = (1 << 3)/**< The DDS internet is disconnected, and the remaining throttle timer exceeds 1 minute. */
};

/**
 * Bitmask containing PermanentRecommendationCauseCode bits, e.g., a value of 0x2 represents that
 * the data setting is off. Multiple cause codes are possible.
 */
using PermanentRecommendationCauseCodes = uint64_t;

/**
 * Provides additional information about the recommendation.
 * Parameters for permanent and temporary recommendations are mutually exclusive.
 */
struct RecommendationDetails {
    TemporaryRecommendationType tempType;             /**< Temporary recommendation type */
    TemporaryRecommendationCauseCodes tempCause;      /**< Cause code for temporary recommendation */
    PermanentRecommendationCauseCodes permCause;      /**< Cause code for permanent recommendation */
};

/**
 * DDS recommendation information. @ref DdsInfo provides the recommended DDS SIM slot.
 * It is recommended to analyze the provided @ref RecommendationDetails to perform the appropriate
 * action.
 */
struct DdsSwitchRecommendation {
    DdsInfo recommendedDdsInfo;                     /**< Recommended DDS information */
    RecommendationDetails recommendationDetails;    /**< Details indicating the cause for the recommendation */
};

class IDualDataListener;

class IDualDataManager {
public:

    /**
     * Checks the status of the DualDataManager object and returns the result.
     *
     * @returns SERVICE_AVAILABLE    -  If DualDataManager is ready for service.
     *          SERVICE_UNAVAILABLE  -  If DualDataManager is temporarily unavailable.
     *          SERVICE_FAILED       -  If DualDataManager encountered an irrecoverable
     *                                  failure.
     *
     */
    virtual telux::common::ServiceStatus getServiceStatus() = 0;

    /**
     * Allows the client to perform the DDS switch. Client has the option
     * to either select permanent or temporary switch.
     *
     * On platforms with access control enabled, the caller needs to have the TELUX_DUAL_DATA_CONFIG
     * permission to successfully invoke this API.
     *
     * @param [in] request          Client has to provide the request
     *                              @ref telux::data::DdsInfo.
     *
     * @param [in] callback         Callback to get response for requestDdsSwitch.
     *                              Possible ErrorCode in @ref telux::common::ResponseCallback:
     *                              - If the DDS switch is performed successfully
     *                                @ref telux::common::ErrorCode::SUCCESS
     *                              - If the DDS switch request is rejected
     *                                @ref telux::common::ErrorCode::OPERATION_NOT_ALLOWED
     *                                The following scenarios are examples of when a switch
     *                                request will be rejected:
     *                                    1. Slot1 is permanent DDS and the client attempts to
     *                                       trigger a permanent DDS switch on slot 1.
     *                                    2. During an MT/MO voice call and the client attempts
     *                                       to trigger a permanent DDS switch.
     *                              - If the DDS switch is allowed but due to some reason DDS
     *                                switch failed @ref telux::common::ErrorCode::GENERIC_FAILURE
     *
     * @returns Status of requestDdsSwitch, i.e., success or suitable status code.
     *
     */
    virtual telux::common::Status requestDdsSwitch(DdsInfo request,
        telux::common::ResponseCallback callback = nullptr) = 0;

    /**
     * Request the current DDS slot information.
     *
     * On platforms with access control enabled, the caller needs to have the TELUX_DUAL_DATA_INFO
     * permission to successfully invoke this API.
     *
     * @param [in] callback      Callback to get response for requestCurrentDds.
     *
     * @returns Status of requestCurrentDds, i.e., success or suitable status code.
     *
     */
    virtual telux::common::Status requestCurrentDds(RequestCurrentDdsRespCb callback) = 0;

    /**
     * The response for this API allows the user determine if the device supports
     * dual data feature or not.
     *
     * @param [out] isCapable      Provides the dual data capability of the device
     *
     *                             - True:  Device supports dual data.
     *                             - False: Device does not support dual data.
     *
     *                             If the device supports dual data, use
     *                             @ref getDualDataUsageRecommendation to check, whether long
     *                             running data calls on both slots is allowed, not-allowed
     *                             or not-recommended.
     *
     * @returns Error code which indicates whether the operation succeeded or not.
     *
     * On platforms with access control enabled, the caller needs to have the TELUX_DUAL_DATA_INFO
     * permission to successfully invoke this API.
     *
     * @note    Eval: This is a new API and is being evaluated. It is subject to change
     *          and could break backwards compatibility.
     */
    virtual telux::common::ErrorCode getDualDataCapability(bool &isCapable) = 0;

    /**
     * Queries the dual data usage recommendation.
     *
     * @param [out] recommendation   Recommendation about dual data usage.
     *
     * @returns Error code which indicates whether the operation succeeded or not
     *
     * On platforms with access control enabled, the caller needs to have the TELUX_DUAL_DATA_INFO
     * permission to successfully invoke this API.
     *
     * @note    Eval: This is a new API and is being evaluated. It is subject to change
     *          and could break backwards compatibility.
     */
    virtual telux::common::ErrorCode getDualDataUsageRecommendation(
        DualDataUsageRecommendation &recommendation) = 0;

    /**
     * Configure DDS recommendation.
     * It is used to control temporary and permanent recommendations, along with recommendation
     * types like throughput-based or latency-based. This configuration needs to be set to enable
     * indication @ref onDdsSwitchRecommendation and get the expected result in
     * @ref getDdsSwitchRecommendation.
     *
     * This configuration is not persistent across reboots or SSR (subsystem restart).
     *
     * On platforms with access control enabled, the caller needs to have TELUX_DUAL_DATA_CONFIG
     * permission to invoke this API successfully.
     *
     * @param [in] ddsSwitchRecommendationConfig           DDS  switch recommendation configuration
     * @return Error code which indicates whether the operation succeeded or not
     *         @ref telux::common::ErrorCode
     *
     */
    virtual telux::common::ErrorCode configureDdsSwitchRecommendation(
        const DdsSwitchRecommendationConfig ddsSwitchRecommendationConfig) = 0;

    /**
     * Request the current default data subscription (DDS) SIM slot recommendation.
     * The modem provides a recommendation for the DDS sub. The recommendation is provided
     * based on multiple factors like internet availability, throttling, roaming, voice call
     * status, etc. For more information about scenarios, please check
     * @ref TemporaryRecommendationCauseCode and @ref PermanentRecommendationCauseCode.
     *
     * Use @ref onDdsSwitchRecommendation to get updates about changes in the recommendation.
     * User can check the previous or cached recommendation from the modem via this API.
     *
     * @note Ensure @ref configureDdsSwitchRecommendation is called beforehand to get the expected
     * result.
     *
     * On platforms with access control enabled, the caller needs to have the TELUX_DUAL_DATA_INFO
     * permission to successfully invoke this API.
     *
     * @param [out] ddsSwitchRecommendation           DDS switch recommendation
     * @return Error code which indicates whether the operation succeeded or not
     *         @ref telux::common::ErrorCode
     *
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to change and could break
     * backwards compatibility.
     */
    virtual telux::common::ErrorCode getDdsSwitchRecommendation(
        DdsSwitchRecommendation &ddsSwitchRecommendation) = 0;

    /**
     * Registers with the DualDataManager as a listener for service status and other events.
     *
     * @param [in] listener    Pointer to the IDualDataListener object that processes the
     *                         notification
     *
     * @returns Status of registerListener.
     *
     */
    virtual telux::common::Status registerListener(
        std::weak_ptr<IDualDataListener> listener) = 0;

    /**
     * Removes a previously added listener.
     *
     * @param [in] listener    Pointer to the IDualDataListener object that needs to be removed
     *
     * @returns Status of deregisterListener.
     *
     */
    virtual telux::common::Status deregisterListener(
        std::weak_ptr<IDualDataListener> listener) = 0;

    /**
     * Destructor for IDualDataManager
     */
    virtual ~IDualDataManager(){};
};

class IDualDataListener : public telux::common::ISDKListener {
public:
    /**
     * This function is called when the service status changes.
     *
     * @param [in] status - @ref ServiceStatus
     */
    virtual void onServiceStatusChange(telux::common::ServiceStatus status) {}

    /**
     * Provides the current DDS state and is called whenever a DDS switch occurs.
     *
     * @param [in] currentState      Provides the current DDS status.
     *                               - Slot ID on which the DDS switch occurred.
     *                               - DDS switch type @ref telux::data::DdsType.
     */
    virtual void onDdsChange(DdsInfo currentState) {}

    /**
     * This function is called when the dual data capability changes.
     *
     * @param [in] isDualDataCapable - Dual data capability.
     *
     */
    virtual void onDualDataCapabilityChange(bool isDualDataCapable) {}

    /**
     * This function is called when the dual data usage recommendation changes.
     *
     * @param [in] recommendation - Provides dual data usage recommendation.
     *
     */
    virtual void onDualDataUsageRecommendationChange(
        DualDataUsageRecommendation recommendation) {}

    /**
     * This function is called when the DDS (Default Data Subscription) recommendation
     * changes.
     * The recommendation may be triggered due to internet unavailability, throttling, roaming,
     * voice call status change, etc. For more information about scenarios, please check
     * @ref TemporaryRecommendationCauseCode and @ref PermanentRecommendationCauseCode.
     *
     * @note @ref configureDdsSwitchRecommendation should be called beforehand to enable these
     * indications.
     *
     * @param [in] ddsSwitchRecommendation      Provides the recommended DDS switch.
     *
     * @note Eval: This is a new indication and is being evaluated. It is subject to change and
     * could break backwards compatibility.
     */
    virtual void onDdsSwitchRecommendation(const DdsSwitchRecommendation ddsSwitchRecommendation) {}

    virtual ~IDualDataListener() {}
};

/** @} */ /* end_addtogroup telematics_data */
}  // namespace data
}  // namespace telux

#endif  //TELUX_DUAL_DATA_MANAGER_HPP
