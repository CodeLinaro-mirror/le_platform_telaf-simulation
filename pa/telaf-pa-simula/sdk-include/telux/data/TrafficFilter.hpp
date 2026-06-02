/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef TELUX_DATA_TRAFFICFILTER_HPP
#define TELUX_DATA_TRAFFICFILTER_HPP

#include <memory>
#include <telux/common/CommonDefines.hpp>
#include <telux/data/DataDefines.hpp>

namespace telux {
namespace data {

/**
 * @brief Specifies the data path through the various internal components.
 */
enum class DataPath {
    TETHERED_TO_WAN_HW = 0, /**< Data flow between clients tethered to the NAD over ethernet and the WAN interface using hardware acceleration. Data path: Eth <=> IPA <=> Modem <=> WAN */
    TETHERED_TO_APPS_SW,    /**< Data flows between clients tethered to the NAD over ethernet and software running on the application processor using a software path. Data path: Eth <=> Apps Processor */
    APPS_TO_WAN             /**< Data flow between the application processor and WAN. Data path: Apps Processor <=> WAN */
};

/**
 * @brief Provide valid parameters in @ref ITrafficFilter
 */
enum TrafficFilterValidField {
    TF_DIRECTION_VALID = (1 << 0),
    TF_PCP_VALID = (1 << 1),
    TF_IP_PROTOCOL_VALID = (1 << 2),
    TF_SOURCE_IPV4_ADDRESS_VALID = (1 << 3),
    TF_SOURCE_IPV6_ADDRESS_VALID = (1 << 4),
    TF_SOURCE_PORT_VALID = (1 << 5),
    TF_SOURCE_VLAN_LIST_VALID = (1 << 6),
    TF_DESTINATION_IPV4_ADDRESS_VALID = (1 << 7),
    TF_DESTINATION_IPV6_ADDRESS_VALID = (1 << 8),
    TF_DESTINATION_PORT_VALID = (1 << 9),
    TF_DESTINATION_VLAN_LIST_VALID = (1 << 10),
    TF_DATA_PATH_VALID = (1 << 11),
    TF_SOURCE_PORT_RANGE_VALID = (1 << 12),
    TF_DESTINATION_PORT_RANGE_VALID = (1 << 13),
    TF_SOURCE_PORT_CONFIG_VALID = (1 << 14),
    TF_DESTINATION_PORT_CONFIG_VALID = (1 << 15),
};

/**
 * Bitmask containing TrafficFilterValidField bits,
 * e.g., a value of 0x5 represents that source IPv4 and ports are valid.
 */
using TrafficFilterValidFields = uint32_t;

/**
 * @brief Indicates whether the parameter is for the source or destination.
 */
enum class FieldType {
    SOURCE,
    DESTINATION,
};
/**
 * Port Configuration
 *
 * @ref PortConfig::port -  Port number. If range is non-zero, this is the starting port number;
 * otherwise, it is a single port number.
 *
 * @ref PortConfig::range - Optional field. Port range.
 *
 * @ref PortConfig::maxActiveConnections - Optional field, applicable for QoS filter. This
 * indicates the maximum number of active connections possible at the same time in the given source
 * port range, considering all traffic sources - in apps processor or tethered client. This is
 * applicable when multiple traffic sources:
 *  1. send data in the UL direction over the cellular connection
 *  2. use the same data call (use the same public IP)
 *  3. have the same source port range
 *
 *  If the maximum active connections are less than the range provided, then by default, the
 *  range will be considered the maximum active connections.
 *  Some examples below consider multiple clients' traffic sources satisfying the above conditions.
 *
 *  Case-1:
 *       - Client-1: Location: Apps processor, start port 1, range = 10
 *       - Client-2: Location: Tethered client, start port 1, range = 10
 *       - PortConfig = {port = 1, range = 10, maxActiveConnections = 20}
 *       - Explanation: If both clients use all the ports 1-10 at the same time.
 *
 *  Case-2:
 *       - Client-1: Location: Tethered client-1, port range = 1-10
 *       - Client-2: Location: Tethered client-2, port range = 1-10
 *       - PortConfig = {port = 1, range = 10, maxActiveConnections = 15}
 *       - Explanation: If both clients together use 15 ports at the same time.
 *
 *  Case-3:
 *       - Client-1: Location: Apps processor, port range = 1-10
 *       - Client-2: Location: Tethered client-1, port range = 1-10
 *       - Client-3: Location: Tethered client-2, port range = 1-10
 *       - PortConfig = {port = 1, range = 10, maxActiveConnections = 30}
 *       - Explanation: If all three clients use all the ports 1-10 at the same time.
 *
 *  Case-3:
 *       - Client-1: Location: Apps processor, port range = 1-10
 *       - Client-2: Location: Tethered client-1, port range = 1-10
 *       - Client-3: Location: Tethered client-2, port range = 1-10
 *       - PortConfig = {port = 1, range = 10, maxActiveConnections = 30}
 *       - Explanation: If all three clients use all the ports 1-10 at the same time.
 *
 *  Case-4:
 *       - Client-1: Location: Apps processor, port range = 1-10
 *       - Client-2: Location: Tethered client-1, port range = 1-10
 *       - Client-3: Location: Tethered client-2, port range = 1-10
 *       - PortConfig = {port = 1, range = 10, maxActiveConnections = 7}
 *       - Explanation: If all clients together use 7 ports at the same time, the maximum number of
 *                    active connections is less than the range provided above, which is considered
 *                    the maximum active connections.
 */
struct PortConfig {
    uint16_t port;
    uint16_t range = 0;  /**< Optional field. */
    uint16_t maxActiveConnections = 0;  /**< Optional field. */
};

/**
 * @brief Traffic filter is group of generic data flow identification via source
 * info, destination info and protocol which is built via @ref
 * TrafficFilterBuilder.
 */
class ITrafficFilter {
 public:
    /**
     * @brief Get the Traffic Filter Valid Fields
     * This function can be used to check whether the respective parameter is
     * valid.
     *
     * @return TrafficFilterValidFields bit mask
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to
     * change and could break backwards compatibility.
     */
    virtual TrafficFilterValidFields getTrafficFilterValidFields() = 0;

    /**
     * @brief Returns the direction (e.g., UPLINK, DOWNLINK).
     *
     * @return Direction enum representing the traffic direction.
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to
     * change and could break backwards compatibility.
     */
    virtual Direction getDirection() = 0;

    /**
     * @brief Returns the data path ( @ref DataPath ) of the traffic filter.
     *
     * @return Data path of the traffic filter.
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to
     * change and could break backwards compatibility.
     */
    virtual DataPath getDataPath() = 0;

    /**
     * @brief Returns the Priority Code Point (PCP) value.
     *
     * @return PCP value as an int8_t.
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to
     * change and could break backwards compatibility.
     */
    virtual int8_t getPCP() = 0;

    /**
     * @brief Retrieves the IP protocol.
     * @return The IP protocol value @ref IpProtocol.
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to
     * change and could break backwards compatibility.
     */
    virtual IpProtocol getIPProtocol() = 0;

    /**
     * @brief Retrieves the IPv4 Address
     *
     * @param [in]  fieldType   Indicates whether the get is for the source or destination.
     * @return IPv4 address
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to
     * change and could break backwards compatibility.
     */
    virtual std::string getIPv4Address(FieldType fieldType) = 0;

    /**
     * @brief Retrieves the IPv6 address.
     *
     * @param [in]  fieldType   Indicates whether the get is for the source or destination.
     * @return IPv6 address.
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to
     * change and could break backwards compatibility.
     */
    virtual std::string getIPv6Address(FieldType fieldType) = 0;

    /**
     * @brief Retrieves the port configuration.
     *
     * @param [in] fieldType     Indicates whether the get is for the source or destination.
     * @return The port configuration.
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to
     * change and could break backwards compatibility.
     */
    virtual PortConfig& getPortConfig(FieldType fieldType) = 0;

    /**
     * @brief Retrieves the port.
     *
     * @param [in] fieldType     Indicates whether the get is for the source or destination.
     * @return The port.
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to
     * change and could break backwards compatibility.
     */
    virtual uint16_t getPort(FieldType fieldType) = 0;

    /**
     * @brief Retrieves the port range.
     *
     * @param [in] fieldType     Indicates whether the get is for the source or destination.
     * @param [out] startPort    Start port number
     * @param [out] range        Port range
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to change and could
     * break backwards compatibility.
     */
    virtual void getPortRange(FieldType fieldType, uint16_t& startPort, uint16_t& range) = 0;

    /**
     * @brief Retrieves the list of VLANs.
     *
     * @param [in] fieldType     Indicates whether the get is for the source or destination.
     * @return A vector of integers representing the source VLANs.
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to
     * change and could break backwards compatibility.
     */
    virtual std::vector<int> getVlanList(FieldType fieldType) = 0;

    /**
     * @brief Converts the API object to a human-readable string.
     * @return A string representation of the API state.
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to
     * change and could break backwards compatibility.
     */
    virtual std::string toString() = 0;

    /**
     * Destructor for ITrafficFilter
     */
    virtual ~ITrafficFilter() {
    }
};

/**
 * @brief Traffic Filter Builder is used to build @ref ITrafficFilter.
 *
 * Set the expected parameters, and then call the @ref
 * TrafficFilterBuilder::build method. It will return an instance of @ref ITrafficFilter.
 *
 * @note Eval: This is a new API and is being evaluated. It is subject to
 * change and could break backwards compatibility.
 */
class TrafficFilterBuilder {
 public:
    /**
     * @brief Constructs a TrafficFilterBuilder.
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to
     * change and could break backwards compatibility.
     */
    TrafficFilterBuilder();

    /**
     * @brief Builds the traffic filter.
     * @return Shared pointer to the constructed traffic filter.
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to
     * change and could break backwards compatibility.
     */
    std::shared_ptr<ITrafficFilter> build();

    /**
     * @brief Sets the direction for the filter configuration.
     *
     * @param [in] direction    The desired direction.
     * @return Reference to this builder for method chaining.
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to
     * change and could break backwards compatibility.
     */
    TrafficFilterBuilder &setDirection(Direction direction);

    /**
     * @brief Sets the expected data path ( @ref DataPath ) for the traffic filter.
     * If the data path is not set, @ref DataPath::TETHERED_TO_WAN_HW will be selected as the
     * default data path.
     *
     * @param [in] dataPath     Expected data path
     * @return Reference to this builder for method chaining.
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to
     * change and could break backwards compatibility.
     */
    TrafficFilterBuilder &setDataPath(DataPath dataPath = DataPath::TETHERED_TO_WAN_HW);

    /**
     * @brief Sets the priority code point (PCP) for the filter configuration.
     *
     * @param [in] pcp      The PCP value.
     * @return Reference to this builder for method chaining.
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to
     * change and could break backwards compatibility.
     */
    TrafficFilterBuilder &setPCP(int8_t pcp);

    /**
     * @brief Sets the IP protocol. The protocol numbers are defined by Internet
     * Assigned Numbers Authority (IANA)
     *
     * @param [in] ipProtocol   IP protocol (e.g., TCP, UDP).
     * @return Reference to this builder for method chaining.
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to
     * change and could break backwards compatibility.
     */
    TrafficFilterBuilder &setIPProtocol(IpProtocol ipProtocol);

    /**
     * @brief Sets the IPv4 address and subnet.
     *
     * @param [in] ipv4Addr     IPv4 address.
     * @param [in] fieldType    Indicates whether the set is for the source or destination.
     * @return Reference to this builder for method chaining.
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to
     * change and could break backwards compatibility.
     */
    TrafficFilterBuilder &setIPv4Address(std::string ipv4Addr, FieldType fieldType);

    /**
     * @brief Sets the IPv6 address and prefix length.
     *
     * @param [in] ipv6Addr     IPv6 address.
     * @param [in] fieldType    Indicates whether the set is for the source or destination.
     * @return Reference to this builder for method chaining.
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to
     * change and could break backwards compatibility.
     */
    TrafficFilterBuilder &setIPv6Address(std::string ipv6Addr, FieldType fieldType);

    /**
     * @brief Sets the port configuration.
     *
     * @param [in] portConfig       Port configuration
     * @param [in] fieldType        Indicates whether the set is for the source or destination.
     * @return Reference to this builder for method chaining.
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to
     * change and could break backwards compatibility.
     */
    TrafficFilterBuilder &setPortConfig(PortConfig portConfig, FieldType fieldType);

    /**
     * @brief Sets the port range.
     *
     * @param [in] startPort    Start port number
     * @param [in] range        Port range
     * @param [in] fieldType    Indicates whether the set is for the source or destination.
     * @return Reference to this builder for method chaining.
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to
     * change and could break backwards compatibility.
     */
    TrafficFilterBuilder &setPortRange(uint16_t startPort, uint16_t range, FieldType fieldType);

    /**
     * @brief Sets the port.
     *
     * @param [in] port         port number
     * @param [in] fieldType    Indicates whether the set is for the source or destination.
     * @return Reference to this builder for method chaining.
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to
     * change and could break backwards compatibility.
     */
    TrafficFilterBuilder &setPort(uint16_t port, FieldType fieldType);

    /**
     * @brief Sets the source VLAN list.
     *
     * @param [in] vlanList     Vector of VLAN IDs.
     * @param [in] fieldType    Indicates whether the set is for the source or destination.
     * @return Reference to this builder for method chaining.
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to
     * change and could break backwards compatibility.
     */
    TrafficFilterBuilder &setVlanList(std::vector<int> vlanList, FieldType fieldType);

 private:
    std::shared_ptr<ITrafficFilter> trafficFilter_;
};

}  // namespace data
}  // namespace telux
#endif  // TELUX_DATA_TRAFFICFILTER_HPP
