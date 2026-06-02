/*
 *  Copyright (c) 2019,2021 The Linux Foundation. All rights reserved.
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
 *
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file       IpFilter.hpp
 * @brief      A IP filter class to add specific filters like what data will be allowed from
 *             the modem to the application processor. Only data packets that match the filter
 *             will be sent to the apps processor. Also used to configure Firewall rules.
 */

#ifndef TELUX_DATA_IPFILTER_HPP
#define TELUX_DATA_IPFILTER_HPP

#include <cstdint>

#include <telux/common/CommonDefines.hpp>

#include <telux/data/DataDefines.hpp>

namespace telux {
namespace data {

/** @addtogroup telematics_data
 * @{ */

using TypeOfService = uint8_t;
using FlowLabel = uint32_t;

/**
 * IPv4 header info
 */
struct IPv4Info {
    std::string srcAddr; /**< address of the device that sends the packet. */
    std::string srcSubnetMask;
    std::string destAddr; /**< address of receiving end */
    std::string destSubnetMask;
    TypeOfService value = 0; /**< level of throughput, reliability, and delay */
    TypeOfService mask = 0;
    IpProtocol nextProtoId = 0; /**< Protocol ID (i.e TCP, UDP or ICMP ) */
};

/**
 * IPv6 header info
 */
struct IPv6Info {
    std::string srcAddr;        /**< address of the device that sends the packet. */
    uint8_t srcPrefixLen;       /**< source prefix length used to create subnet */
    std::string destAddr;       /**< address of receiving end */
    uint8_t dstPrefixLen;       /**< destination prefix length used to create subnet */
    IpProtocol nextProtoId = 0; /**< Protocol ID (i.e TCP, UDP or ICMP ) */
    TrafficClass val = 0;       /**< indicates the class or priority of the IPv6 packet, enables the ability to track specific traffic flows at the network layer. */

    TrafficClass mask = 0;
    FlowLabel flowLabel = 0; /**< Indicates that this packet belongs to a specific sequence of packets between a source and destination, requiring special handling by intermediate IPv6 router. Note: FlowLabel is currently ignored in the firewall configuration */
    uint8_t natEnabled = 0;
};

/**
 * UDP header info
 */
struct UdpInfo {
    PortInfo src;  /**< Source port and range */
    PortInfo dest; /**< Destination port and range */
};

/**
 * TCP header info
 */
struct TcpInfo {
    PortInfo src;  /**< Source port and range */
    PortInfo dest; /**< Destination port and range */
};

/**
 * Internet Control Message Protocol (ICMP)
 */
struct IcmpInfo {
    uint8_t type; /**< ICMP message type - RFC2780 */
    uint8_t code; /**< ICMP message code - RFC2780 */
};

/**
 * Encapsulating Security Payload
 */
struct EspInfo {
    uint32_t spi; /**< Security Parameters Index */
};

/**
 * @brief       A IP filter class to add specific filters like what data will be allowed from
 *              the modem to the application processor. Only data packets that match the filter
 *              will be sent to the apps processor. Also used to configure Firewall rules.
 */
class IIpFilter {
 public:
    /**
     * Get the IPv4 header info
     *
     * @returns @ref telux::data::IPv4Info
     *
     */
    virtual IPv4Info getIPv4Info() = 0;

    /**
     * sets the IPv4 header info
     *
     * @param [in] ipv4Info     IPv4 structure @ref telux::data::IPv4Info
     *
     * @returns Immediate status of setIPv4Info() request sent
     *                   i.e. success or suitable status code.
     *
     */
    virtual telux::common::Status setIPv4Info(const IPv4Info &ipv4Info) = 0;

    /**
     * Get the IPv6 header info
     *
     * @returns @ref telux::data::IPv6Info
     *
     */
    virtual IPv6Info getIPv6Info() = 0;

    /**
     * sets the IPv6 header info
     *
     * @param [in] ipv6Info     IPv6 structure @ref telux::data::IPv6Info
     *
     * @returns Immediate status of setIPv6Info() request sent
     *                   i.e. success or suitable status code.
     *
     */
    virtual telux::common::Status setIPv6Info(const IPv6Info &ipv6Info) = 0;

    /**
     * Get the IpProtocol Number
     *
     * @returns @ref telux::data::IpProtocol
     *
     */
    virtual IpProtocol getIpProtocol() = 0;

    /**
     * Get the IP family type
     *
     * @returns @ref telux::data::IpFamilyType
     *
     */
    virtual IpFamilyType getIpFamily() = 0;

    /**
     * Destructor for IIpFilter
     */
    virtual ~IIpFilter() {
    }
};

/**
 * @brief  This class represents a IP Filter for the UDP, get
 * the new instance from telux::data::DataFactory
 */
class IUdpFilter : virtual public IIpFilter {
 public:
    /**
     * Get the UDP header info
     *
     * @returns @ref telux::data::UdpInfo
     *
     */
    virtual UdpInfo getUdpInfo() = 0;

    /**
     * sets the UDP header info
     *
     * @param [in] udpInfo     UdpInfo structure @ref telux::data::UdpInfo
     *
     * @returns Immediate status of setUdpInfo() request sent
     *                   i.e. success or suitable status code.
     *
     */
    virtual telux::common::Status setUdpInfo(const UdpInfo &udpInfo) = 0;

    /**
     * Destructor for IUdpFilter
     */
    virtual ~IUdpFilter() {
    }
};

/**
 * @brief  This class represents a IP Filter for the TCP, get
 * the new instance from telux::data::DataFactory
 */
class ITcpFilter : virtual public IIpFilter {
 public:
    /**
     * Get the TCP header info
     *
     * @returns @ref telux::data::TcpInfo
     *
     */
    virtual TcpInfo getTcpInfo() = 0;

    /**
     * sets the TCP header info
     *
     * @param [in] tcpInfo     TcpInfo structure @ref telux::data::TcpInfo
     *
     * @returns Immediate status of setTcpInfo() request sent
     *                   i.e. success or suitable status code.
     *
     */
    virtual telux::common::Status setTcpInfo(const TcpInfo &tcpInfo) = 0;

    /**
     * Destructor for ITcpFilter
     */
    virtual ~ITcpFilter() {
    }
};

/**
 * @brief  This class represents a IP Filter for the ICMP, get
 * the new instance from telux::data::DataFactory
 */
class IIcmpFilter : virtual public IIpFilter {
 public:
    /**
     * Get the ICMP header info
     *
     * @returns @ref telux::data::IcmpInfo
     *
     */
    virtual IcmpInfo getIcmpInfo() = 0;

    /**
     * sets the ICMP header info
     *
     * @param [in] icmpInfo     TcpInfo structure @ref telux::data::IcmpInfo
     *
     * @returns Immediate status of setIcmpInfo() request sent
     *                   i.e. success or suitable status code.
     *
     */
    virtual telux::common::Status setIcmpInfo(const IcmpInfo &icmpInfo) = 0;

    /**
     * Destructor for IIcmpFilter
     */
    virtual ~IIcmpFilter() {
    }
};

/**
 * @brief  This class represents a IP Filter for the ESP, get
 * the new instance from telux::data::DataFactory
 */
class IEspFilter : virtual public IIpFilter {
 public:
    /**
     * Get the ESP header info
     *
     * @returns @ref telux::data::EspInfo
     *
     */
    virtual EspInfo getEspInfo() = 0;

    /**
     * sets the ICMP header info
     *
     * @param [in] espInfo     EspInfo structure @ref telux::data::EspInfo
     *
     * @returns Immediate status of setEspInfo() request sent
     *                   i.e. success or suitable status code.
     *
     */
    virtual telux::common::Status setEspInfo(const EspInfo &espInfo) = 0;

    /**
     * Destructor for IEspFilter
     */
    virtual ~IEspFilter() {
    }
};
/** @} */ /* end_addtogroup telematics_data */
}  // namespace data
}  // namespace telux
#endif // TELUX_DATA_IPFILTER_HPP
