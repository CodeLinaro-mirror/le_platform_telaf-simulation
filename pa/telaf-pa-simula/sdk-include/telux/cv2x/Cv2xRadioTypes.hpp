/*
 *  Copyright (c) 2018-2021, The Linux Foundation. All rights reserved.
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
* @file       Cv2xRadioTypes.hpp
*
* @brief      Contains common data types used in Cv2x Radio API
*/

#ifndef TELUX_CV2X_CV2XRADIOTYPES_HPP
#define TELUX_CV2X_CV2XRADIOTYPES_HPP

#include <bitset>
#include <vector>

namespace telux {

namespace cv2x {

#define CV2X_IPV6_ADDR_ARRAY_LEN 16
#define CV2X_MAC_ADDR_LEN        6
#define IPV6_MIN_PREFIX_LENGTH   64
#define IPV6_MAX_PREFIX_LEN      128

/** @addtogroup telematics_cv2x_cpp
 * @{ */

/**
 * Defines Maximum number of antennas that is supported.
 *
 * Used in @ref TxStatusReport
 */
constexpr uint8_t MAX_ANTENNAS_SUPPORTED = 2u;

/**
 * Defines CV2X Traffic Types.
 */
enum class TrafficCategory {
    SAFETY_TYPE,      /**< Safety message traffic category */
    NON_SAFETY_TYPE,  /**< Non-safety message traffic category */
};

/**
 * Defines possible values for CV2X radio RX/TX status.
 * 1. If Rx is in inactive state, Tx should also be in inactive state.
 * 2. If Rx is in active state, Tx should be in active(normal case)
 *    or suspended state(sensing or tunnel mode).
 * 3. If Rx is in suspended state, Tx should be in suspended state.
 * Used in @ref Cv2xStatus
 */
enum class Cv2xStatusType {
    INACTIVE,    /**< RX/TX is inactive */
    ACTIVE,      /**< RX/TX is active */
    SUSPENDED,   /**< RX/TX is suspended */
    UNKNOWN,     /**< RX/TX status unknown */
};

/**
 * Defines possible values for cause of CV2X radio failure.
 * The cause code is only associated with cv2x suspend/inactive status,
 * if cv2x is active, the cause code has no meaning.
 * Used in @ref Cv2xStatus
 */
enum class Cv2xCauseType {
    TIMING,             /**< CV2X is suspended due to the outage of timing reference. */
    CONFIG,             /**< CV2X is inactive due to v2x.xml is missing, invalid, or expired. */
    UE_MODE,            /**< CV2X is inactive due to CV2X mode is not started. */
    GEOPOLYGON,         /**< CV2X is inactive due to UE enters a geo-polygon that does not support cv2x. */
    THERMAL,            /**< CV2X is suspended when the device's temperature is high. */
    THERMAL_ECALL,      /**< CV2X is suspended when the device's temperature is high and emergency call is ongoing. */
    GEOPOLYGON_SWITCH,  /**< CV2X is suspended when UE switches to a new geopolygon that also supports CV2X and UE is already in CV2X active status, CV2X status will change to active after the update is done. */
    SENSING,            /**< CV2X Tx is suspended when GNSS signal recovers or CV2X mode just starts. UE needs sensing for 1 second before Tx can begin, Tx status will change to active after sensing is done. */
    LPM,                /**< CV2X is inactive due to unexpected operating mode. */
    DISABLED,           /**< CV2X is inactive due to CV2X is disabled in the EFS. */
    NO_GNSS,            /**< CV2X is inactive due to GNSS signal is not available when starting CV2X. */
    INVALID_LICENSE,    /**< CV2X is inactive due to invalid license. */
    NOT_READY,          /**< CV2X is inactive due to low layer is not ready to start CV2X. */
    NTN,                /**< CV2X is suspended due to NTN is in progress. */
    NO_DATA_CALL,       /**< CV2X is inactive due to no cv2x data call. */
    UNKNOWN             /**< Invalid cause type only used internally. */
};

/**
 * Defines possible values for SLSS sync pattern.
 * Used in @ref SyncRefUeInfo
 */
enum class SlssSyncPattern {
    OFFSET_IND_1,      /**< UE transmits SLSS in subframes indicated by the
                            syncOffsetIndicator1 specified in V2X configuration. */
    OFFSET_IND_2,      /**< UE transmits SLSS in subframes indicated by the
                            syncOffsetIndicator2 specified in V2X configuration. */
    OFFSET_IND_3,      /**< UE transmits SLSS in subframes indicated by the
                            syncOffsetIndicator3 specified in V2X configuration. */
    ODD_RESERVED,      /**< UE transmits SLSS in odd-numbered reserved subframes. */
    EVEN_RESERVED,     /**< UE transmits SLSS in even-numbered reserved subframes. */
    UNKNOWN,           /**< Invalid cause type only used internally. */
};

/**
 * Encapsulates parameters of an SLSS sync reference UE.
 * Used in @ref SlssRxInfo.
 */
struct SyncRefUeInfo {
    uint16_t slssId;
    /**< The SLSS ID of the sync reference UE that is defined in 3GPP TS 36.331
         chapter 6.3.8. */
    bool inCoverage;
    /**< Indicates whether or not the UE is in coverage of GNSS that is defined in
         3GPP TS 36.331 chapter 6.5.2. */
    SlssSyncPattern pattern = SlssSyncPattern::UNKNOWN;
    /**< Indicates the SLSS sync pattern of the UE that is defined in 3GPP TS 36.331
         chapter 6.3.8. */
    uint8_t rsrp;
    /**< SLSS RSRP value of the UE in dBm is ((float)rsrp - 256)/2. */
    bool selected;
    /**< Indicates whether or not the sync reference UE has been selected
         as the timing source. */
};

/**
 * Encapsulates parameters of CV2X SLSS Rx Information.
 *
 * Used in @ref telux::cv2x::ICv2xListener::onSlssRxInfoChanged.
 */
struct SlssRxInfo {
    std::vector<SyncRefUeInfo> ueInfo;
    /**< Vector of detected SLSS sync reference UEs. */
};

/**
 * Encapsulates parameters of a CV2X socket.
 *
 * Used in @ref ICv2xRadio::createCv2xTcpSocket.
 */
struct SocketInfo {
    uint32_t serviceId;
    /**< V2X service ID bound to the socket. */
    uint16_t localPort;
    /**< Local port number of the socket used for binding. */
};

/**
 * Encapsulates status of CV2X radio.
 *
 * Used in @ref ICv2xRadioManager::requestCv2xStatus and ICv2xRadioListener.
 */
struct Cv2xStatus {
    Cv2xStatusType rxStatus = Cv2xStatusType::UNKNOWN;  /**< RX status */
    Cv2xStatusType txStatus = Cv2xStatusType::UNKNOWN;  /**< TX status */
    Cv2xCauseType rxCause = Cv2xCauseType::UNKNOWN;     /**< RX cause of failure */
    Cv2xCauseType txCause = Cv2xCauseType::UNKNOWN;     /**< TX cause of failure */
    uint8_t cbrValue = 255;                             /**< Channel Busy Ratio */
    bool cbrValueValid = false;                         /**< CBR value is valid */
};

/**
 * Encapsulates status for single pool.
 *
 * Used in @ref Cv2xStatusEx.
 */
struct Cv2xPoolStatus {
    uint8_t poolId = 0u; /**< pool ID */
    Cv2xStatus status;   /**< status */
};

/**
 * Encapsulates status of CV2X radio and per pool status.
 *
 * Used in @ref ICv2xRadioManager::requestCv2xStatus and
 * Cv2xRadioListener.
 */
struct Cv2xStatusEx {
    Cv2xStatus status;                      /**< Overall Cv2x status */
    std::vector<Cv2xPoolStatus> poolStatus; /**< Multi pool status vector */
    bool timeUncertaintyValid = false;      /**< Time uncertainty value is valid */
    float timeUncertainty;                  /**< Time uncertainty value in milleseconds */
};

/**
 * Defines CV2X traffic type in terms of IP or NON-IP.
 */
enum class TrafficIpType {
   TRAFFIC_IP,     /**< IP message traffic */
   TRAFFIC_NON_IP, /**< NON-IP message traffic */
};

/**
 * Defines CV2X modes of concurrency with cellular WWAN.
 *
 * Used in @ref Cv2xRadioCapabilities
 */
enum class RadioConcurrencyMode {
    WWAN_NONCONCURRENT, /**< No simultaneous WWAN + CV2X on this interface  */
    WWAN_CONCURRENT,    /**< Interface supports requests for concurrent WWAN +
                             CV2X connections. */
};

/**
 * Defines CV2X status change events. The state can change in response to the
 * loss of timing precision or a geofencing change.
 *
 * Used in @ref ICv2xRadioListener::onStatusChanged
 */
enum class Cv2xEvent {
    CV2X_INACTIVE,  /**<  */
    CV2X_ACTIVE,    /**<  */
    TX_SUSPENDED,   /**<  */
    TXRX_SUSPENDED, /**<  */
};

/**
 * Range of supported priority levels, where a lower number means a higher
 * priority. For example, 8 is the current 3GPP standard.
 *
 * Used in @ref Cv2xRadioCapabilities and @ref SpsFlowInfo
 */
enum class Priority {
    MOST_URGENT,
    PRIORITY_1,
    PRIORITY_2,
    PRIORITY_3,
    PRIORITY_4,
    PRIORITY_5,
    PRIORITY_6,
    PRIORITY_BACKGROUND,
    PRIORITY_UNKNOWN,
};

/**
 * Range of supported periodicities in milliseconds.
 *
 * Used in @ref Cv2xRadioCapabilities and @ref SpsFlowInfo
 *
 * @deprecated: enum class not going to be supported in future releases. Clients should stop using
 * this. Once a class has been marked as Deprecated, the class could be removed in future releases.
 *
 */
enum class Periodicity {
    PERIODICITY_10MS,
    PERIODICITY_20MS,
    PERIODICITY_50MS,
    PERIODICITY_100MS,
    PERIODICITY_UNKNOWN,
};

/**
 * Contains minimum and maximum EARFCNs for a given Tx pool ID. Multiple Tx
 * Pools allow the same radio and overall frequency range to be shared for
 * multiple types of traffic like V2V and V2X. Each pool ID and frequency range
 * corresponds to a certain type of traffic.
 * Both edge guard bands are not included in the EARFCN range reported.
 * The calculation for the full bandwidth includes both edge guard bands is:
 * bandwidth(MHz) = (maxFreq-minFreq)/9.
 *
 * Used in @ref Cv2xRadioCapabilities
 */
struct TxPoolIdInfo {
    uint8_t poolId;
    /**< TX pool ID. */
    uint16_t minFreq;
    /**< Minimum EARFCN of this pool. */
    uint16_t maxFreq;
    /**< Maximum EARFCN of this pool. */
};

/**
 * Contains event flow configuration parameters.
 *
 * Used in @ref ICv2xRadio::createTxEventFlow
 */
struct EventFlowInfo {
    bool autoRetransEnabledValid = true;
    /**< Set to true if autoRetransEnabled field is specified. If false, the
         system will use the default setting. */
    bool autoRetransEnabled = true;
    /**< Used to enable automatic-retransmissions. */
    bool peakTxPowerValid = false;
    /**< Set to true if peakTxPower is used. If false, the system will
         use the default setting. */
    int32_t peakTxPower;
    /**< Max Tx power setting in dBm. */
    bool mcsIndexValid = false;
    /**< Set to true if mcsIndex is used. If false, the system will use its
         default setting. */
    uint8_t mcsIndex;
    /**< Modulation and Coding Scheme Index to use. */
    bool txPoolIdValid = false;
    /**< Set to true if txPoolId is used.  If false, the system will use its
         default setting. */
    uint8_t txPoolId = 0u;
    /**< Transmission Pool ID. */
    bool isUnicast = false;
    /**< Set to true if isUnicast flow.  If false, Non-Unicast flow will be created.
         Note: Unicast flows ignore subscribed Service Ids */
};

/**
 * Used to request the QoS bandwidth contract, implemented in PC5 3GPP V2X
 * radio as a <i>Semi Persistent Flow</i> (SPS).
 *
 * The underlying radio providing the interface might support periodicities of
 * various granularity in 100ms integer multiples (e.g. 200ms, 300ms).
 *
 * Used in @ref ICv2xRadio::createTxSpsFlow and @ref ICv2xRadio::changeSpsFlowInfo
 */
struct SpsFlowInfo {
    Priority priority = Priority::PRIORITY_2;
    /**< Specifies one of the 3GPP levels of Priority for the traffic that is
         pre-reserved on the SPS flow. Default is PRIORITY_2.

         Use getCapabilities() to discover the supported priority levels.
         @deprecated: periodicity, Use new periodicityMs instead */
    Periodicity periodicity = Periodicity::PERIODICITY_100MS;
    /**This is the new interface to specify periodicity in milliseconds for
       SpsFlowInfo. Enum Periodicity is deprecated and will be removed in future
       release.
    */
    uint64_t periodicityMs = 100;
    /**< Bandwidth-reserved periodicity interval in interval in milliseconds.

         There are limits on which intervals the underlying radio supports.
         Use getCapabilities() to discover minPeriodicityMultiplierMs and
         maximumPeriodicityMs. */
    uint32_t nbytesReserved = 0u;
    /**< Number of bytes of TX bandwidth that are sent every periodicity
         interval. */
    bool autoRetransEnabledValid = true;
    /**< Set to true if autoRetransEnabled field is specified. If false, the
         system will use the default setting. */
    bool autoRetransEnabled = true;
    /**< Used to enable automatic-retransmissions. */
    bool peakTxPowerValid = false;
    /**< Set to true if peakTxPower is used. If false, the system will
         use the default setting. */
    int32_t peakTxPower;
    /**< Max Tx power setting in dBm. */
    bool mcsIndexValid = false;
    /**< Set to true if mcsIndex is used. If false, the system will use its
         default setting. */
    uint8_t mcsIndex;
    /**< Modulation and Coding Scheme Index to use. */
    bool txPoolIdValid = false;
    /**< Set to true if txPoolId is used.  If false, the system will use its
         default setting. */
    uint8_t txPoolId = 0u;
    /**< Transmission Pool ID. */
};

/**
 * Contains capabilities of the Cv2xRadio.
 *
 * Used in @ref ICv2xRadio::requestCapabilities and @ref
 * ICv2xRadioListener::onCapabilitiesChanged
 */
struct Cv2xRadioCapabilities {
    uint32_t linkIpMtuBytes;
    /**< Maximum data payload length (in bytes) of a packet supported by the
         IP Radio interface. */
    uint32_t linkNonIpMtuBytes;
    /**< Maximum data payload length (in bytes) of a packet supported by the
         non-IP Radio interface. */
    RadioConcurrencyMode maxSupportedConcurrency;
    /**< Indicates whether this interface supports concurrent WWAN with
         V2X (PC5). */
    uint16_t nonIpTxPayloadOffsetBytes;
    /**< Byte offset in a non-IP Tx packet before the actual payload begins. */
    uint16_t nonIpRxPayloadOffsetBytes;
    /**< Byte offset in a non-IP Rx packet before the actual payload begins.
         @deprecated: periodicitiesSupported, Use new periodicities instead */
    std::bitset<8> periodicitiesSupported;
    std::vector<uint64_t> periodicities;
    /**< Specifies the periodicities supported */
    uint8_t maxNumAutoRetransmissions;
    /**< Least frequent bandwidth periodicity that is supported. Above this
         value, use event-driven periodic messages of a period larger than
         this value. */
    uint8_t layer2MacAddressSize;
    /**< Size of the L2 MAC address.

         Different Radio Access Technologies have different-sized L2 MAC
         addresses: 802.11 has 6 bytes, whereas 3GPP PC5 has only 3 bytes.

         Because a randomized MAC address comes from an HSM with good pseudo
         random entropy, higher layers must know how many bytes of the MAC
         address to generate. */
    std::bitset<8> prioritiesSupported;
    /**< Bit set of different priority levels supported by this Cv2xRadio.
         Refer to @ref Priority */
    uint16_t maxNumSpsFlows;
    /**< Maximum number of supported SPS reservations. */
    uint16_t maxNumNonSpsFlows;
    /**< Maximum number of supported event flows (non-SPS ports). */
    int32_t maxTxPower;
    /**< Maximum supported transmission power. */
    int32_t minTxPower;
    /**< Minimum supported transmission power. */
    std::vector<TxPoolIdInfo> txPoolIdsSupported;
    /**< Vector of supported transmission pool IDs. */
    uint8_t isUnicastSupported;
    /**< Non zero value if UDP event unicast is supported. */
};

/**
 * Contains MAC information that is reported from the actual MAC SPS in the
 * radio. The offsets can periodically change on any given transmission report.
 */
struct MacDetails {
    uint32_t periodicityInUseNs;
    /**< Actual transmission interval period (in nanoseconds) scheduled
         relative to 1PP 0:00.00 time */
    uint16_t currentlyReservedPeriodicBytes;
    /**< Actual number of bytes currently reserved at the MAC layer. This number
         can be slightly larger than original request. */
    uint32_t txReservationOffsetNs;
    /**< Actual offset, from a 1PPS pulse and TX flow periodicity, that the
         MAC selected and is using for the transmit reservation. If the data
         goes to the radio with enough time, it can be transmitted on the
         medium in the next immediately scheduled slot. */
};

/**
 * Contains SPS packet scheduling information that is reported from the radio.
 *
 * Used in @ref ICv2xRadioListener::onSpsSchedulingChanged
 */
struct SpsSchedulingInfo {
    uint8_t spsId;
    /**< SPS ID */
    uint64_t utcTime;
    /**< Absolute UTC start time of next selected grant in nanoseconds. */
    uint32_t periodicity;
    /**< Periodicity of the grant in milliseconds. */
};

/**
 * Contains time confidence, position confidence, and propogation delay for a
 * trusted UE.
 *
 * Used in @ref TrustedUEInfo
 */
struct TrustedUEInfo {
    uint32_t sourceL2Id;
    /**< Trusted Source L2 ID */
    float timeUncertainty;
    /**< Time uncertainty value in milliseconds. */
    uint16_t timeConfidenceLevel;
    /**< @deprecated Use timeUncertainty
         Time confidence level.
         Range from 0 to 127 with 0 being invalid/unavailable
         and 127 being the most confident. */
    uint16_t positionConfidenceLevel;
    /**< Position confidence level.
         Range from 0 to 127 with 0 being invalid/unavailable
         and 127 being the most confident. */
    uint32_t propagationDelay;
    /**< Propagation delay in microseconds. */
};

/**
 * Contains list of malicious UE source L2 IDs.
 * Contains list of trusted UE source L2 IDs and associated confidence values.
 *
 * Used in @ref ICv2xRadio::updateTrustedUEList
 */
struct TrustedUEInfoList {
    bool maliciousIdsValid = false;
    /**< Malicious remote UE sources are valid. */
    std::vector<uint32_t> maliciousIds;
    /**< Malicious remote UE source L2 IDs. */
    bool trustedUEsValid = false;
    /**< Trusted remote UE sources are valid. */
    std::vector<TrustedUEInfo> trustedUEs;
    /**< Trusted remote UE sources. */
};

/**
 * Contains IPv6 address.
 *
 * Used in @ref DataSessionSettings
 */
struct IPv6Address {
    uint8_t addr[16];  /**< Contains IPv6 address */
};

/**
 * Contains packet data session settings.
 *
 * Used in @ref ICv2xRadio::requestDataSessionSettings
 */
struct DataSessionSettings {
    bool mtuValid = false;
    /**< Set to true if mtu is valid. */
    uint32_t mtu;
    /**< MTU size. */
    bool ipv6AddrValid = false;
    /**< Set to true if ipv6 address is valid. */
    IPv6Address ipv6Addr;
    /**< IPv6 address. */
};

/**
 * V2X configuration source types listed in ascending order of priority.
 * The system always uses the V2X configuration with the highest priority
 * if multiple V2X configuration sources exist.
 *
 * Used in @ref ConfigEventInfo
 */
enum class ConfigSourceType {
    UNKNOWN = 0u,   /**< V2X config file source is unknown */
    PRECONFIG = 1u, /**< V2X config file source is preconfig */
    SIM_CARD = 2u,  /**< V2X config file source is SIM card */
    OMA_DM = 4u,    /**< V2X config file source is OMA-DM */
};

/**
 * Defines possible values for the events relevant to CV2X config file.
 *
 * Used in @ref ConfigEventInfo
 */
enum class ConfigEvent {
    CHANGED = 0u,  /**< V2X config file is changed */
    EXPIRED = 1u,  /**< V2X config file is expired */
};

/**
 * Information about any update to a CV2X config file.
 *
 * Used in @ref ICv2xConfigListener::onConfigChanged
 */
struct ConfigEventInfo {
    ConfigSourceType source;
    /**< The type of the V2X config file. */
    ConfigEvent event;
    /**< V2X config event. */
};

/**
 * Contains remote UE source L2 ID that modem will drop on Rx.
 *
 * Used in @ref ICv2xRadioManager::setL2Filters
 */
struct L2FilterInfo {
    /** remote UE L2 MAC addr to filter. */
    uint32_t srcL2Id;

    /** Duration, in millisec (resolution 100 msec). 0 means delete the filter. */
    uint32_t durationMs;

    /** Proximity service per packet priority (PPPP), packets with priority above this value
         will be dropped. Range 0-7, 0 mean all priority pkts from that UE would be dropped. */
    uint8_t pppp;
};

/**
 * Fault detection for Tx chain that including PA and front end.
 *
 * Used in @ref RFTxInfo
 */
enum class RFTxStatus {
    INACTIVE,        /**< The Tx chain is not working. */
    OPERATIONAL,     /**< The Tx chain is operational. */
    FAULT,           /**< Fault detected on the Tx chain. */
};

/**
 * Tx status per Tx chain and Tx power per Tx antenna for a specific transport block.
 *
 * Used in @ref TxStatusReport
 */
struct RFTxInfo {
    RFTxStatus status;
    /**< Fault detection status for a specific Tx chain. */
    int32_t power;
    /**< The target Tx power after MPR/AMPR reduction for a specific Tx antenna
         in dBm*10 format. Invalid value is -700, it means the corresponding
         antenna is not being used for the transmission of this transport block. */
};

/**
 * Defines possible values for the segment type of a transport block.
 *
 * Used in @ref TxStatusReport
 */
enum class SegmentType {
    FIRST,        /**< V2X packet is segmented, it's the first transport block. */
    LAST,         /**< V2X packet is segmented, it's the last transport block. */
    MIDDLE,       /**< V2X packet is segmented, it's a transport block between first and last. */
    ONLY_ONE,     /**< V2X packet is not segmented, it's the only one transport block. */
};

/**
 * Defines new Tx or re-Tx type relevant to a transport block.
 *
 * Used in @ref TxStatusReport
 */
enum class TxType {
    NEW_TX,      /**< New Tx of the V2X transport block. */
    RE_TX,       /**< Re-Tx of the V2X transport block. */
    SLSS_TX,     /**< Tx of SLSS. */
};

/**
 * Information on Tx status of a V2X transport block that is reported
 * from low layer.
 * 1. A V2X packet might trigger multiple reports because of the segmentaion
 * and re-Tx in low layer.
 * 2. If a transport block is dropped in low layer, no report will be triggered
 * for that transport block.
 * 3. The power in the array of rfInfo is the target Tx power value in dBm*10 after
 * MPR/AMPR reduction for a specific Tx antenna. The status in the array of rfInfo
 * is the fault detection status for a specific Tx chain.
 *  - In CDD mode, two antennas are being used for a specific transport block,
 * both rfInfo[0].power and rfInfo[1].power are valid (not -700), rfInfo[i].status
 * is reflecting the status of Tx chain/Tx antenna i.
 *  - In TXD mode, data transmission swtiches between two antennas/chains and only
 * one antenna/chain is being used for a specific transport block, the Tx antenna
 * being used has valid power (not -700) in the array of rfInfo, rfInfo[i].status
 * is reflecting the status of Tx chain i or the status of the Tx antenna i whose
 * power is valid (not -700) in the array of rfInfo.
 * Used in @ref ICv2xTxStatusReportListener::onTxStatusReport
 */
struct TxStatusReport {
    RFTxInfo rfInfo[MAX_ANTENNAS_SUPPORTED];
    /**< Tx status per Tx chain and Tx power per Tx antenna. */
    uint8_t numRb;
    /**< Number of resource blocks used for the transport block. */
    uint8_t startRb;
    /**< Start resource block index used for the transport block. */
    uint8_t mcs;
    /**< Modulation and coding scheme used for the transport block
         that is defined in 3GPP TS 36.213. */
    uint8_t segNum;
    /**< Total number of segments of a V2X packet. */
    SegmentType segType;
    /**< Segment type of the transport block. */
    TxType txType;
    /**< Indication of new Tx or re-Tx of the transport block. */
    uint16_t otaTiming;
    /**< OTA timing in format of system frame number*10 + subframe number. */
    uint16_t port;
    /**< Port number that can be used to link the report to a specific Tx
         flow which has the same source port number. */
};

/**
 * Encapsulates ipv6 prefix length in bits and ipv6 prefix.
 *
 * Used in @ref ICv2xRadio::setGlobalIPInfo.
 */
struct IPv6AddrType
{
    /** ipv6 address prefix length in bits, range [64, 128] */
    uint8_t prefixLen;
    uint8_t ipv6Addr[CV2X_IPV6_ADDR_ARRAY_LEN];
};

/**
 * Encapsulates destination L2 address.
 *
 * Used in @ref ICv2xRadio::setGlobalIPUnicastRoutingInfo.
 */
struct GlobalIPUnicastRoutingInfo
{
    /** Array that stores CV2X L2 MAC address at the last 3 bytes in big endian order. */
    uint8_t destMacAddr[CV2X_MAC_ADDR_LEN];
};

/** @} */ /* end_addtogroup telematics_cv2x_cpp */

} // namespace cv2x

} // namespace telux

#endif // TELUX_CV2X_CV2XRADIOTYPES_HPP
