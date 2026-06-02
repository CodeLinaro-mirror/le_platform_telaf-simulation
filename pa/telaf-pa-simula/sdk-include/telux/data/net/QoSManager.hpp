/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef TELUX_DATA_NET_QOSMANAGER_HPP
#define TELUX_DATA_NET_QOSMANAGER_HPP

#include <telux/common/CommonDefines.hpp>
#include <telux/common/SDKListener.hpp>

#include <telux/data/DataDefines.hpp>
#include <telux/data/TrafficFilter.hpp>

namespace telux {
namespace data {
namespace net {

using DataPath = telux::data::DataPath;
/**
 * Type of bandwidth configuration associated with traffic class
 */
enum class BandwidthConfigType {
    BW_RANGE = 1, /**< Bandwidth range */
};

struct BandwidthRange {
    uint32_t minBandwidth; /**< Minimum bandwidth in Mbps */
    uint32_t maxBandwidth; /**< Maximum bandwidth in Mbps */
};

union BandwidthValue {
    BandwidthRange bandwidthRange; /**< Bandwidth in range The sum of the minimum bandwidths across all traffic classes should not exceed the link capacity. */
};

/**
 * @brief Bandwidth configuration for traffic shaping
 */
struct BandwidthConfig {
    BandwidthConfigType dlBandwidthConfigType; /**< Type of DL bandwidth */
    BandwidthValue dlBandwidthValue;           /**< Value of DL bandwidth */

    void setDlBandwidthRange(uint32_t minBandwidth, uint32_t maxBandwidth) {
        dlBandwidthConfigType = BandwidthConfigType::BW_RANGE;
        dlBandwidthValue.bandwidthRange.minBandwidth = minBandwidth;
        dlBandwidthValue.bandwidthRange.maxBandwidth = maxBandwidth;
    }
};

/**
 * @brief Possible error codes while adding QoS filter config @ref addQoSFilter
 */
enum class QoSFilterErrorCode {
    SUCCESS = 0,
    MISSING_DIRECTION,                 /**< The mandatory 'data traffic direction' field is missing */
    INVALID_MULTIPLE_SOURCE_INFO,     /**< If traffic descriptor is set, only one of the following sources is expected: IPv4, IPv6, or VLAN */
    INVALID_MULTIPLE_DESTINATION_INFO /**< If traffic descriptor is set, only one of the following destinations is expected: IPv4, IPv6, or VLAN */
};

/**
 * @brief Possible error codes while creating traffic class ( @ref createTrafficClass ).
 */
enum class TcConfigErrorCode {
    SUCCESS = 0,
    MISSING_TRAFFIC_CLASS, /**< The mandatory 'traffic class' field is missing */
    MISSING_DATA_PATH,     /**< The mandatory 'data path' field is missing */
    MISSING_DIRECTION,     /**< The mandatory 'data traffic direction' field is missing */
};

/**
 * @brief Possible QoS filter 'installation status'.
 */
enum class FilterInstallationStatus {
    SUCCESS = 0,    /** QoS filter installed successfully. */
    FAILED,         /** QoS filter installation failed. */
    PENDING,        /** QoS filter is saved and will be installed when necessary conditions are met. For example, if no data calls are active and the QoS filter installation is requested on the modem, the status would be PENDING until a data call is brought up. */
    NOT_APPLICABLE, /** QoS filter is not applicable for the module. For example, in the case of @ref DataPath::TETHERED_TO_APPS_SW, filters will not be applicable for the modem. */
};

/**
 * @brief QoS filter status at different modules.
 */
struct QoSFilterStatus {
    FilterInstallationStatus ethStatus;   /** QoS filter installation status at the Eth. */
    FilterInstallationStatus modemStatus; /** QoS filter installation status at the modem. */
    FilterInstallationStatus ipaStatus;   /** QoS filter installation status at the IPA. */
};

/**
 * @brief Provide valid parameters in @ref TcConfig
 */
enum TcConfigValidField {
    TC_TRAFFIC_CLASS_VALID = (1 << 0),
    TC_DIRECTION_VALID = (1 << 1),
    TC_DATA_PATH_VALID = (1 << 2),
    TC_BANDWIDTH_CONFIG_VALID = (1 << 3),
};

/**
 * Bitmask containing TcConfigValidField bits,
 * e.g., a value of 0x5 represents that source IPv4 and ports are valid.
 */
using TcConfigValidFields = uint32_t;

/**
 * @brief Traffic class configuration.
 * The traffic class configuration contains the traffic class number, direction,
 * data path, and bandwidth configuration.
 *
 * @note Use getTcConfigValidFields to obtain a bitmask of @ref
 * TcConfigValidField, which indicates which fields are valid.
 */
class ITcConfig {
 public:
    /**
     * @brief Gets the valid fields in the traffic class configuration.
     * This function can be used to check whether a respective parameter is valid.
     *
     * @return TcConfigValidFields bit mask
     */
    virtual TcConfigValidFields getTcConfigValidFields() = 0;

    /**
     * @brief Returns the traffic class.
     *
     * @return TrafficClass representing the traffic class.
     */
    virtual TrafficClass getTrafficClass() = 0;

    /**
     * @brief Returns the direction (e.g., UPLINK, DOWNLINK).
     *
     * @return Direction enum representing the traffic direction.
     */
    virtual Direction getDirection() = 0;

    /**
     * @brief Gets the QoS filter data path.
     *
     * @return DataPath enum representing the data path.
     */
    virtual DataPath getDataPath() = 0;

    /**
     * @brief Gets the bandwidth configuration.
     *
     * @return BandwidthConfig representing the bandwidth configuration.
     */
    virtual BandwidthConfig getBandwidthConfig() = 0;

    /**
     * @brief Converts the API object to a human-readable string.
     * @return A string representation of the API state.
     */
    virtual std::string toString() = 0;
};

/**
 * @brief Traffic class config builder is used to build @ref ITcConfig.
 * Set the expected parameters, and then call the @ref TcConfigBuilder::build
 * method.
 */
class TcConfigBuilder {
 public:
    /**
     * @brief Sets the traffic class for the filter configuration.
     *
     * @param [in] trafficClass     The desired traffic class.
     * @return Reference to this builder for method chaining.
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to
     * change and could break backwards compatibility.
     */
    TcConfigBuilder &setTrafficClass(TrafficClass trafficClass);

    /**
     * @brief Sets the direction for the filter configuration.
     *
     * @param [in] direction    The desired direction.
     * @return Reference to this builder for method chaining.
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to
     * change and could break backwards compatibility.
     */
    TcConfigBuilder &setDirection(Direction direction);

    /**
     * @brief Sets the expected data path ( @ref DataPath ) for the QoS filter. It
     * indicates how data transfers are expected to happen within internal
     * components.
     * No traffic shaping supported in uplink direction in any module.
     * - @ref DataPath::TETHERED_TO_WAN_HW (ETH <=> IPA <=> Modem) :
     *      - Uplink   : Prioritization happens in modem.
     *      - Downlink : Traffic shaping supported only in ETH.
     *                   Prioritization happens only in ETH.
     *      - Traffic classes with data path TETHERED_TO_WAN_HW can be associated with traffic
     *        filters with data path TETHERED_TO_WAN_HW and APPS_TO_WAN.
     *
     * - @ref DataPath::TETHERED_TO_APPS_SW (ETH <=> Apps):
     *      - Modem and IPA are not in the data path.
     *      - Uplink   : Prioritization happens in ETH.
     *      - Downlink : Traffic shaping supported in ETH.
     *                   Prioritization happens in ETH.
     *      - Traffic classes with data path TETHERED_TO_APPS_SW can be associated with traffic
     *        filters with data path TETHERED_TO_APPS_SW.
     *
     * - @ref DataPath::APPS_TO_WAN  (Apps <=> Modem) :
     *      - IPA and ETH are not in the data path.
     *      - Uplink    : Prioritization happens in modem.
     *      - Downlink  : Traffic shaping not supported.
     *                    Prioritization is not supported.
     *      - Traffic classes with data path APPS_TO_WAN can be associated with traffic filters with
     *        data path APPS_TO_WAN.
     *      - Traffic classes created with APPS_TO_WAN can only be associated with UPLINK data path.
     *
     * @param [in] dataPath     Expected data path
     * @return Reference to this builder for method chaining.
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to
     * change and could break backwards compatibility.
     */
    TcConfigBuilder &setDataPath(DataPath dataPath);

    /**
     * @brief Sets the bandwidth configuration.
     *
     * Bandwidth configuration/traffic shaping is supported only in ETH in the downlink direction.
     * Paths involving ETH include, for example, @ref TETHERED_TO_WAN_HW or @ref TETHERED_TO_APPS_SW
     * Bandwidth configuration/traffic shaping is not supported in Modem and IPA modules.
     *
     * @param [in] bandwidthConfig     Expected bandwidth configuration
     * @return Reference to this builder for method chaining.
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to
     * change and could break backwards compatibility.
     */
    TcConfigBuilder &setBandwidthConfig(BandwidthConfig bandwidthConfig);

    /**
     * @brief Builds the traffic class configuration.
     *
     * @return Shared pointer to the constructed traffic class configuration.
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to
     * change and could break backwards compatibility.
     */
    std::shared_ptr<ITcConfig> build();

 private:
    std::shared_ptr<ITcConfig> tcConfig_ = nullptr;
};

/**
 * @brief QoS filter ( @ref IQoSFilter ) handle.
 */
using QoSFilterHandle = uint32_t;

/**
 * @brief QoS filter configuration.
 */
struct QoSFilterConfig {
    TrafficClass trafficClass;
    std::shared_ptr<ITrafficFilter> trafficFilter;
};

/**
 * @brief QoS filter information.
 */
class IQoSFilter {
 public:
    static const QoSFilterHandle INVALID_HANDLE = 0;

    /**
     * @brief Returns the Quality of Service (QoS) filter handle.
     *
     * @return QoS filter handle as a @ref QoSFilterHandle.
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to
     * change and could break backwards compatibility.
     */
    virtual QoSFilterHandle getHandle() = 0;

    /**
     * @brief Returns the installation status of a QoS filter.
     *
     * @return QoS filter status as a @ref QoSFilterStatus.
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to
     * change and could break backwards compatibility.
     */
    virtual QoSFilterStatus getStatus() = 0;

    /**
     * @brief Returns the traffic class.
     *
     * @return TrafficClass representing the traffic class.
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to
     * change and could break backwards compatibility.
     */
    virtual TrafficClass getTrafficClass() = 0;

    /**
     * @brief Returns a shared pointer to the traffic descriptor.
     *
     * @return Shared pointer to ITrafficFilter.
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to
     * change and could break backwards compatibility.
     */
    virtual std::shared_ptr<ITrafficFilter> getTrafficFilter() = 0;

    /**
     * @brief Converts the API object to a human-readable string.
     * @return A string representation of the API state.
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to
     * change and could break backwards compatibility.
     */
    virtual std::string toString() = 0;
};

// Forward declarations
class IQoSListener;

/**
 * @brief The QoS Manager class provides a set of APIs related to Quality of Service (QoS) for the
 * various data flows that flow via the NAD. Its purpose is to manage aspects like assigning
 * priority to the data flow, limiting the bandwidth of each flow relative to other flows, etc.
 * Prioritization of traffic using QoS filters is expected to be done on a limited amount of the
 * total bandwidth.
 *
 *    - Data Flow Identification @ref ITrafficFilter :
 *          - Data flows can be identified using various parameters from network layers 2, 3, and 4.
 *          - These parameters include: Five-tuple (source and destination IP addresses, source and
 *            destination port numbers, and IP protocol), VLAN ID, and PCP number (assigned to VLAN
 *            using @ref IVlanManager::createVlan) etc.
 *          - A data flow is described using a Traffic Filter @ref ITrafficFilter. A Traffic Filter
 *            is created using @ref TrafficFilterBuilder.
 *
 *    - Traffic Classes:
 *          - A traffic class is similar to a class in Linux traffic control (tc).
 *          - Each traffic class can have multiple associated data flows.
 *          - Each traffic class is identified by a unique ID. Traffic class IDs start from 0
 *            (highest priority) and go up to the maximum allowed traffic class.
 *          - Lower values correspond to higher priorities.
 *
 *    - Traffic shaping/bandwidth configuration:
 *          - One can specify constraints/limits on the bandwidth that each Traffic class is allowed
 *            using @ref createTrafficClass.
 *          - Currently this is used to configure the bandwidth on the traffic egressing the NAD via
 *            the Eth link, to other devices/ECUs.
 *
 *    - Creating QoS filter:
 *          - Users assign relative priorities between data flows and QoS filter by associating a
 *            data flow with a traffic class.
 *          - This association is done by @ref QoSFilterConfig.
 *          - Once a QoS filter config is created, it needs to be added to the system using
 *            @ref addQoSFilter. Adding a filter returns a handle. This handle can then be used to
 *            perform operations like deleting a QoS filter @ref deleteQosFilter.
 *
 *    - QoS filters are added to different modules based on the data path assigned to the traffic
 *      class. These can be Ethernet (Eth), IP Accelerator (IPA), modem.
 *
 * QoS support matrix for individual @ref DataPath :
 *  - Data path: TETHERED_TO_APPS_SW
 *      - Direction: UPLINK
 *          - Traffic shaping: No
 *          - Traffic Prioritization: Yes
 *      - Direction: DOWNLINK
 *          - Traffic shaping: Yes
 *          - Traffic Prioritization: Yes
 *
 *  - Data path: APPS_TO_WAN
 *      - Direction: UPLINK
 *          - Traffic shaping: No
 *          - Traffic Prioritization: Yes
 *      - Direction: DOWNLINK
 *          - Traffic shaping: No
 *          - Traffic Prioritization: No
 *
 * - Data path: TETHERED_TO_WAN_HW
 *      - Direction: UPLINK
 *          - Traffic shaping: No
 *          - Traffic Prioritization: Yes
 *      - Direction: DOWNLINK
 *          - Traffic shaping: Yes
 *          - Traffic Prioritization: Yes
 */
class IQoSManager {
 public:
    /**
     * Checks the status of QoS manager and returns the result.
     *
     * @returns SERVICE_AVAILABLE     QoS manager object is ready for service.
     *          SERVICE_UNAVAILABLE   QoS manager object is temporarily unavailable.
     *          SERVICE_FAILED        QoS manager object encountered an irrecoverable failure
     */
    virtual telux::common::ServiceStatus getServiceStatus() = 0;

    /**
     * @brief Creates a traffic class.
     *
     * To create a traffic class, provide the traffic class configuration using
     * @ref ITcConfig, which is constructed using @ref TcConfigBuilder. Traffic
     * classes are uniquely identified by their traffic class number and
     * direction. Additionally, the data path (hardware accelerated or software
     * path) is a mandatory parameter. Also, an optional parameter bandwidth
     * configuration can be provided for the downlink direction (traffic egressing
     * the NAD via the Ethernet link).
     *
     * If any attribute of the traffic class needs to be updated (e.g. bandwidth),
     * - Delete the existing traffic class using @ref deleteTrafficClass. This
     * action will also delete all QoS filters associated with that traffic class.
     * - Create the traffic class with the updated configuration.
     * - Create and add required QoS filters using @ref addQoSFilter.
     *
     * Traffic class creation is persistent across reboots.
     *
     * On platforms with access control enabled, the caller needs to have TELUX_DATA_QOS_OPS
     * permission to successfully invoke this API.
     *
     * @param [in] tcConfig             Traffic class configuration.
     * @param [out] tcConfigErrorCode   Error code specific to @ref ITcConfig
     * @return Error code which indicates whether the operation succeeded or not
     *         @ref telux::common::ErrorCode
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to
     * change and could break backwards compatibility.
     */
    virtual telux::common::ErrorCode createTrafficClass(
        std::shared_ptr<ITcConfig> tcConfig, TcConfigErrorCode &tcConfigErrorCode)
        = 0;

    /**
     * @brief Retrieves all traffic class configurations.
     *
     * @param [out] tcConfigs     Vector of traffic class configurations.
     * @return Error code which indicates whether the operation succeeded or not
     *         @ref telux::common::ErrorCode
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to
     * change and could break backwards compatibility.
     */
    virtual telux::common::ErrorCode getAllTrafficClasses(
        std::vector<std::shared_ptr<ITcConfig>> &tcConfigs)
        = 0;

    /**
     * @brief Deletes a traffic class.
     *
     * To delete a traffic class, provide the traffic class configuration using
     * @ref ITcConfig. The traffic class configuration is built via @ref
     * TcConfigBuilder. The traffic class number and direction are mandatory
     * parameters that need to be set via the builder.
     *
     * On platforms with access control enabled, the caller needs to have TELUX_DATA_QOS_OPS
     * permission to successfully invoke this API.
     *
     * @param [in] tcConfig   Traffic class config
     * @return Error code which indicates whether the operation succeeded or not
     *         @ref telux::common::ErrorCode
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to
     * change and could break backwards compatibility.
     */
    virtual telux::common::ErrorCode deleteTrafficClass(std::shared_ptr<ITcConfig> tcConfig) = 0;

    /**
     * @brief Adds a QoS filter.
     *
     * A QoS filter configuration ( @ref QoSFilterConfig) associates data flow identifiers
     * ( @ref ITrafficFilter) with a traffic class. Associating a data flow with a traffic class
     * allows users to assign relative priorities between data flows and build QoS filters. The
     * traffic filter is constructed using @ref TrafficFilterBuilder.
     *
     * - While building a TrafficFilter, direction is a mandatory parameter that must be set via the
     *   @ref TrafficFilterBuilder. Other parameters are optional.
     *
     * - The IPv4 ( @ref TrafficFilterBuilder::setIPv4Address ),
     *   IPv6 ( @ref TrafficFilterBuilder::setIPv6Address ), and
     *   VLAN ( @ref TrafficFilterBuilder::setVlanList ) parameters of the traffic filter are
     *   mutually exclusive and only one type of attribute can be set out of these for a given
     *   filter. For example, if a filter is needed for both IPv4 and IPv6, then two filters will
     *   have to be added one for IPv4 and the other for IPv6.
     *
     * - Prioritization is possible in the uplink and downlink direction. However, prioritization in
     *   the modem is possible only in the uplink direction.Modem uses 5 tuple information to do
     *   prioritization. To do prioritization in the modem the following parameters are mandatory
     *   when creating traffic filters.
     *   - Source IP @ref TrafficFilterBuilder::setIPv4Address(ipv4Addr, FieldType::SOURCE) or
     *     @ref TrafficFilterBuilder::setIPv6Address(ipv6Addr, FieldType::SOURCE)
     *   - Protocol @ref TrafficFilterBuilder::setIPProtocol
     *   - Destination address or destination port, one of:
     *     @ref TrafficFilterBuilder::setIPv4Address(ipv4Addr, FieldType::DESTINATION) or
     *     @ref TrafficFilterBuilder::setIPv6Address(ipv6Addr, FieldType::DESTINATION) or
     *     @ref TrafficFilterBuilder::setPort(port, FieldType::DESTINATION)
     *
     * - PCP associated with a QoS filter will be used for prioritization with the Eth module.
     *   - The traffic class has a one-to-one mapping with PCP, i.e., only one PCP-based traffic
     *     filter can be associated with a traffic class.
     *   - A traffic class with a higher priority PCP should be associated with a traffic class of
     *     high priority. Note: A higher value PCP (for example 7) is considered the highest
     *     priority whereas a lower value traffic class (for example 0) is considered as highest
     *     priority.
     *      - For example, traffic class 0 (highest priority) can be associated with PCP 7 (highest
     *        priority).
     *   - PCP 0 is reserved and should not be used by clients.
     *   - To ensure prioritization of data flows originating from clients running on the NAD
     *     application processor destined towards the Ethernet module, VLAN needs to be used and the
     *     VLAN needs to be associated with a PCP value with the corresponding priority. To create
     *     the VLAN refer to @ref IVlanManager::createVlan and to associate the PCP value refer to
     *     @ref VlanConfig::priority parameter.
     *
     * Adding a filter returns a handle. This handle can then be used to perform operations like
     * deleting a QoS filter @ref deleteQosFilter or getting a QoS filter info @ref getQosFilter.
     *
     * If any attribute of the QoS filter needs to be updated,
     * - Delete the existing QoS filter using @ref deleteQosFilter.
     * - Create and add the QoS filter with the updated configuration.
     *
     * Once a QoS filter is added, it remains persistent across reboots.
     *
     * - To verify the status of the QoS filter after addition, use @ref getQosFilter to retrieve
     *   the QoS filter information via its handle.
     * - Check the QoS filter status at individual modules using @ref IQoSFilter::getStatus.
     * - If the filter is not applicable in all modules, it is considered unnecessary and can be
     *   deleted using @ref deleteQosFilter to free up resources.
     *
     * On platforms with access control enabled, the caller needs to have TELUX_DATA_QOS_OPS
     * permission to successfully invoke this API.
     *
     * @param [in]  qosFilterConfig     QoS filter configuration
     * @param [out] filterHandle        On successful addition QoS filter handle will be provided
     * @param [out] qosFilterErrorCode  Error code specific to @ref QoSFilterConfig
     * @return Error code which indicates whether the operation succeeded or not.
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to
     * change and could break backwards compatibility.
     */
    virtual telux::common::ErrorCode addQoSFilter(QoSFilterConfig qosFilterConfig,
        QoSFilterHandle &filterHandle, QoSFilterErrorCode &qosFilterErrorCode)
        = 0;

    /**
     * @brief Retrieves QoS filter information for a given handle.
     *
     * QoS filter status at each module can be retrieved from @ref IQoSFilter::getStatus().
     *
     * @param [in] qosFilterHandle      QoS filter handle.
     * @param [out] qosFilter           Shared pointer to QoSFilter corresponding to the handle.
     * @return Error code which indicates whether the operation succeeded or not
     *         @ref telux::common::ErrorCode
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to
     * change and could break backwards compatibility.
     */
    virtual telux::common::ErrorCode getQosFilter(QoSFilterHandle filterHandle,
        std::shared_ptr<IQoSFilter> &qosFilter) = 0;

    /**
     * @brief Retrieves information about existing QoS filters.
     *
     * @param [out] qosFilters     Vector of shared pointers to IQoSFilter.
     * @return Error code which indicates whether the operation succeeded or not
     *         @ref telux::common::ErrorCode
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to
     * change and could break backwards compatibility.
     */
    virtual telux::common::ErrorCode getQosFilters(
        std::vector<std::shared_ptr<IQoSFilter>> &qosFilters)
        = 0;

    /**
     * @brief Deletes a QoS filter.
     *
     * The QoS filter handle is used to delete a QoS filter. The QoS filter handle can be obtained:
     * 1. During QoS filter ( @ref addQoSFilter ) addition.
     * 2. Getting QoS filters ( @ref getQosFilters ) which has @ref IQoSFilter::getHandle to get
     * the filter handle.
     *
     * On platforms with access control enabled, the caller needs to have TELUX_DATA_QOS_OPS
     * permission to successfully invoke this API.
     *
     * @param [in] qosFilterHandle      QoS filter handle to be deleted.
     * @return Error code which indicates whether the operation succeeded or not
     *         @ref telux::common::ErrorCode
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to
     * change and could break backwards compatibility.
     */
    virtual telux::common::ErrorCode deleteQosFilter(QoSFilterHandle qosFilterHandle) = 0;

    /**
     * @brief Deletes all traffic classes and QoS filters.
     *
     * This API deletes all configurations added via @ref addQoSFilter and @ref createTrafficClass.
     *
     * On platforms with access control enabled, the caller needs to have TELUX_DATA_QOS_OPS
     * permission to successfully invoke this API.
     *
     * @return Error code which indicates whether the operation succeeded or not
     *         @ref telux::common::ErrorCode
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to
     * change and could break backwards compatibility.
     */
    virtual telux::common::ErrorCode deleteAllQosConfigs() = 0;

    /**
     * Registers a listener with the QoS Manager.
     *
     * @param [in] listener     Pointer to theIQoSListener object that processes the notification
     *
     * @returns Status of registerListener success or suitable status code
     *
     */
    virtual telux::common::Status registerListener(std::weak_ptr<IQoSListener> listener) = 0;

    /**
     * Removes a previously added listener.
     *
     * @param [in] listener     Pointer to the IQoSListener object that needs to be removed
     *
     * @returns Status of deregisterListener success or suitable status code
     *
     */
    virtual telux::common::Status deregisterListener(std::weak_ptr<IQoSListener> listener) = 0;

    /**
     * Destructor for IQoSManager
     */
    virtual ~IQoSManager(){};
};  // end of IQoSManager

class IQoSListener : public telux::common::ISDKListener {
 public:
    /**
     * This function is called when service status changes.
     *
     * @param [in] status - @ref ServiceStatus
     */
    virtual void onServiceStatusChange(telux::common::ServiceStatus status) {
    }

    /**
     * This function is called when the QoS filter status changes.
     * QoS filter status at each module can be retrieved from @ref IQoSFilter::getStatus().
     *
     * @param [in] qosFilter           Shared pointer to QoSFilter.
     */
    virtual void onQoSFilterStatusChange(std::shared_ptr<IQoSFilter> qosFilter) {
    }

    /**
     * Destructor for IQoSListener
     */
    virtual ~IQoSListener(){};
};

/** @} */ /* end_addtogroup telematics_data_net */
}  // namespace net
}  // namespace data
}  // namespace telux
#endif  // TELUX_DATA_NET_QOSMANAGER_HPP
