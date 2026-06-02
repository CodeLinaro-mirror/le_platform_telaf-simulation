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
  @file v2x_radio_api.h

  @addtogroup telematics_cv2x_c_radio
  Abstraction of the radio driver parameters for a V2X broadcast socket
  interface, including 3GPP CV2X QoS bandwidth contracts.
 */

#ifndef TELUX_CV2X_LEGACY_V2X_RADIO_API_H
#define TELUX_CV2X_LEGACY_V2X_RADIO_API_H

#include <net/ethernet.h> /* the L2 protocols */
#include <netinet/in.h>
#include <netpacket/packet.h>
#include <stdint.h>
#include <sys/socket.h>
#include <stdbool.h>
#include <unistd.h>

#include "v2x_common.pb.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @addtogroup telematics_cv2x_c_radio
@{ */

/** Radio data handle to the interface. */
typedef int v2x_radio_handle_t;

/** Invalid handle returned by v2x_radio_init() and v2x_radio_init_v2() upon an error. */
#define V2X_RADIO_HANDLE_BAD (-1)

/** Limit on the number of simultaneous RmNet Radio interfaces this library can
    have open at once.

    Typically, there are only a few actual radios. On the same radio however,
    one interface can be for IP traffic, and another interface can be for
    non-IP traffic.
 */
#define V2X_MAX_RADIO_SESSIONS (10)

/** Wildcard value for a port number. When the wildcard is used, all V2X
    received traffic is routed.
 */
#define V2X_RX_WILDCARD_PORTNUM (9000)

/** Maximum length of the pool ID list that is returned in
    #v2x_iface_capabilities_t.
 */
#define MAX_POOL_IDS_LIST_LEN (20)

/** Maximum length of the malicious ID list that can be passed in
    v2x_radio_update_trusted_ue_list().
 */
#define MAX_MALICIOUS_IDS_LIST_LEN (50)

/** Maximum length of the trusted ID list that can be passed in
    v2x_radio_update_trusted_ue_list().
 */
#define MAX_TRUSTED_IDS_LIST_LEN   (50)

/** Maximum length for the subscribed service ID list that can
     be passed in v2x_radio_rx_sock_create_and_bind_v2().
  */
#define MAX_SUBSCRIBE_SIDS_LIST_LEN (10)

/** Maximum length for the L2 ID list that can
     be passed in v2x_set_l2_filters() and v2x_cancel_l2_filters.
  */
#define MAX_FILTER_IDS_LIST_LEN (50)

/** Maximum number of antennas that is supported.
    Used in @ref v2x_tx_status_report_t */
#define V2X_MAX_ANTENNAS_SUPPORTED (2)

/** Maximum number of V2X Tx pools that is supported.
    Used in @ref v2x_radio_status_ex_t */
#define V2X_MAX_TX_POOL_NUM (2)

/** Maximum number of V2X Rx pools that is supported.
    Used in @ref v2x_radio_status_ex_t */
#define V2X_MAX_RX_POOL_NUM (4)

/** Maximum number of detected SLSS sync reference UEs.
    Used in @ref v2x_slss_rx_info_t */
#define V2X_MAX_SLSS_SYNC_REF_UE_NUM (16)

/**
    Describes whether the radio chip modem should attempt or support concurrent
    3GPP CV2X operation with a WWAN 4G/5G data call.

    Some chips are only capable of operating on CV2X. In this case:
    - The callers can only request V2X_WWAN_NONCONCURRENT.
    - The interface parameters that are returned list v2x_concurrency_sel_t as
      the value for V2X_WAN_NONCONCURRENT.
 */
typedef enum {
    V2X_WWAN_NONCONCURRENT = 0,  /**< No simultaneous WWAN + CV2X on this
                                      interface. */
    V2X_WWAN_CONCURRENT = 1      /**< Interface allows requests for concurrent
                                      support of WWAN + CV2X connections. */
} v2x_concurrency_sel_t;

/**
    Event indications sent asynchronously from the radio via callbacks that
    indicate the state of the radio. The state can change in response to the
    loss of timing precision or a geofencing change.
    @deprecated This enum type is deprecated, please consider use
    %v2x_radio_status_ex_t instead.
 */
typedef enum {
    V2X_INACTIVE = 0,    /**< V2X communication is disabled. */
    V2X_ACTIVE,          /**< V2X communication is enabled. Transmit and
                              receive are possible. */
    V2X_TX_SUSPENDED,    /**< Small loss of timing precision occurred.
                              Transmit is no longer supported. */
    V2X_RX_SUSPENDED,    /**< Radio can no longer receive any messages. */
    V2X_TXRX_SUSPENDED,  /**< Radio can no longer transmit or receive for
                              some reason. @newpage */
} v2x_event_t;

/**
    Range of supported priority levels, where a lower number means a higher
    priority. For example, 8 is the current 3GPP standard.
 */
typedef enum {
    V2X_PRIO_MOST_URGENT = 0,  /**< Highest priority. */
    V2X_PRIO_1 = 1,
    V2X_PRIO_2 = 2,
    V2X_PRIO_3 = 3,
    V2X_PRIO_4 = 4,
    V2X_PRIO_5 = 5,
    V2X_PRIO_6 = 6,
    V2X_PRIO_BACKGROUND = 7    /**< Lowest priority. */
} v2x_priority_et;

/**
    Valid service availability states.
 */
typedef enum  {
    SERVICE_UNAVAILABLE = 0,
    SERVICE_AVAILABLE = 1,
    SERVICE_FAILED = 2,
} v2x_service_status_t;

/**
    Defines possible values for CV2X radio RX/TX status.
    1. If Rx is in inactive state, Tx should also be in inactive state.
    2. If Rx is in active state, Tx should be in active(normal case)
       or suspended state(sensing or tunnel mode).
    3. If Rx is in suspended state, Tx should be in suspended state.
    Used in @ref v2x_status_info_t
 */
typedef enum {
    V2X_RADIO_STATUS_INACTIVE = 0,    /**< RX/TX is inactive */
    V2X_RADIO_STATUS_ACTIVE = 1,      /**< RX/TX is active */
    V2X_RADIO_STATUS_SUSPENDED = 2,   /**< RX/TX is suspended */
    V2X_RADIO_STATUS_UNKNOWN = 3,     /**< RX/TX status unknown */
} v2x_radio_status_type_t;

/**
    Defines possible values for cause of CV2X radio failure.
    The cause code is only associated with cv2x suspend/inactive status,
    if cv2x is active, the cause code has no meaning.
    Used in @ref v2x_status_info_t
 */
typedef enum {
    V2X_RADIO_CAUSE_TIMING,           /**< CV2X is suspended due to the outage of timing
                                           reference. */
    V2X_RADIO_CAUSE_CONFIG,           /**< CV2X is inactive due to v2x.xml is missing, invalid,
                                           or expired. */
    V2X_RADIO_CAUSE_UE_MODE,          /**< CV2X is inactive due to CV2X mode is not started. */
    V2X_RADIO_CAUSE_GEOPOLYGON,       /**< CV2X is inactive due to UE enters a geo-polygon that
                                           does not support cv2x. */
    V2X_RADIO_CAUSE_THERMAL,          /**< CV2X is suspended when the device's temperature
                                           is high. */
    V2X_RADIO_CAUSE_THERMAL_ECALL,    /**< CV2X is suspended when the device's temperature
                                           is high and emergency call is ongoing. */
    V2X_RADIO_CAUSE_GEOPOLYGON_SWITCH,/**< CV2X is suspended when UE switches to a new geopolygon
                                           that also supports CV2X and UE is already in CV2X
                                           active status, CV2X status will change to active after
                                           the update is done. */
    V2X_RADIO_CAUSE_SENSING,          /**< CV2X Tx is suspended when GNSS signal recovers or CV2X
                                           mode just starts. UE needs sensing for 1 second before
                                           Tx can begin, Tx status will change to active after
                                           sensing is done. */
    V2X_RADIO_CAUSE_LPM,              /**< CV2X is inactive due to unexpected operating mode. */
    V2X_RADIO_CAUSE_DISABLED,         /**< CV2X is inactive due to CV2X is disabled in the EFS. */
    V2X_RADIO_CAUSE_NO_GNSS,          /**< CV2X is inactive due to GNSS signal is not available
                                           when starting CV2X. */
    V2X_RADIO_CAUSE_INVALID_LICENSE,  /**< CV2X is inactive due to invalid license. */
    V2X_RADIO_CAUSE_NOT_READY,        /**< CV2X is inactive due to low layer is not ready to
                                           start CV2X. */
    V2X_RADIO_CAUSE_NTN,              /**< CV2X is suspended due to NTN is in progress. */

    V2X_RADIO_CAUSE_NO_DATA_CALL,     /**< CV2X is inactive due to no cv2x data call. */
    V2X_RADIO_CAUSE_UNKNOWN,          /**< Invalid cause type only used internally. */
} v2x_radio_cause_type_t;

/**
    Encapsulates CV2X Tx/Rx status and cause of failure.
 */
typedef struct {
    v2x_radio_status_type_t status;  /**< Tx/Rx status */
    v2x_radio_cause_type_t cause;    /**< Cause of failure */
} v2x_status_info_t;

/**
    Encapsulates status of CV2X radio.
 */
typedef struct {
    v2x_status_info_t tx_status;  /**< TX status */
    v2x_status_info_t rx_status;  /**< RX status */
} v2x_radio_status_t;

/**
    Encapsulates status for single TX/RX pool.
 */
typedef struct {
    uint8_t pool_id;             /**< pool ID */
    v2x_status_info_t status;    /**< Tx/Rx pool status */
} v2x_pool_status_t;

/**
    V2X overall radio status and per pool status.
 */
typedef struct {
    v2x_radio_status_t status;       /**< CV2X overall TX/RX status */
    uint8_t tx_pool_size;            /**< Number of Tx pools in array of pool_status. */
    v2x_pool_status_t tx_pool_status[V2X_MAX_TX_POOL_NUM]; /**< CV2X Tx pool status. */
    uint8_t rx_pool_size;            /**< Number of Rx pools in array of pool_status. */
    v2x_pool_status_t rx_pool_status[V2X_MAX_RX_POOL_NUM]; /**< CV2X Rx pool status. */
} v2x_radio_status_ex_t;

/**
    Contains time confidence, position confidence, and propagation delay for a
    trusted UE.
*/
typedef struct {
    uint32_t source_l2_id;
    /**< L2 ID of the trusted source */

    float time_uncertainty;
    /**< Time uncertainty in milliseconds. */

    uint16_t time_confidence_level;
    /**< Deprecated. Use time_uncertainty instead.

         Confidence level of the time period.

         @values 0 through 127, where 0 is invalid or unavailable and 127 is
                 the most confident */

    uint16_t position_confidence_level;
    /**< Confidence level of the position.

         @values 0 through 127, where 0 is invalid or unavailable and 127 is
                 the most confident */

    uint32_t propagation_delay;
    /**< Propagation delay in microseconds. */
} trusted_ue_info_t;

/**
    Contains minimum and maximum EARFCNs for a Tx pool ID. Multiple Tx
    Pools allow the same radio and overall frequency range to be shared for
    multiple types of traffic like V2V and V2X. Each pool ID and frequency
    range corresponds to a certain type of traffic.
    Both edge guard bands are not included in the EARFCN range reported.
    The calculation for the full bandwidth includes both edge guard bands is:
    bandwidth(MHz) = (max_freq-min_freq)/9.
    This struct is used in #v2x_iface_capabilities_t.
 */
typedef struct {
    uint8_t pool_id;      /**< ID of the Tx pool. */
    uint16_t min_freq;    /**< Minimum EARFCN of this pool. */
    uint16_t max_freq;    /**< Maximum EARFCN of this pool. @newpagetable */
} tx_pool_id_info_t;

/**
    Contains information on the capabilities of a Radio interface.
 */
typedef struct {
    int link_ip_MTU_bytes;
    /**< Maximum data payload length (in bytes) of a packet supported by the
         IP Radio interface. */

    int link_non_ip_MTU_bytes;
    /**< Maximum data payload length (in bytes) of a packet supported by the
         non-IP Radio interface. */

    v2x_concurrency_sel_t max_supported_concurrency;
    /**< Indicates whether this interface supports concurrent WWAN with
         V2X (PC5). */

    uint16_t non_ip_tx_payload_offset_bytes;
    /**< Byte offset in a non-IP Tx packet before the actual payload begins.
         In 3GPP CV2X, the first byte after the offset is the 1-byte V2X
         Family ID.

         This offset is to the left for a per-packet Tx header that includes
         Tx information that might be inserted in front of the packet
         payload (in subsequent releases).

         An example of Tx information is MAC/Phy parameters (power, rate,
         retransmissions policy, and so on).

         Currently, this value is expected to be 0. But it is reserved to
         support possible per-packet Tx/Rx headers that might be added in
         future releases of this API. */

    uint16_t non_ip_rx_payload_offset_bytes;
    /**< Byte offset in a non-IP Rx packet before the actual payload begins.

         Initially, this value is zero. But it allows for later insertion of
         per-packet Rx information (sometimes called metadata) to be
         added to the front of the data payload. An example of Rx
         information is MAC/Phy measurements (receive signal strength,
         timestamps, and so on).

         The V2X Family ID is considered as part of the payload in the 3GPP
         CV2X. Higher layers (applications that are clients to this API) must
         remove or advance past that 1 byte to get to the more familiar
         actual WSMP/Geonetworking payload. */

    uint16_t int_min_periodicity_multiplier_ms;
    /**< Lowest number of milliseconds requested for a bandwidth.

         This value is also the basis for all possible bandwidth reservation
         periods. For example, if this multiplier=100 ms, applications can only
         reserve bandwidths of 100 ms, 200 ms, up to 1000 ms. */

    uint16_t int_maximum_periodicity_ms;
    /**< Least frequent bandwidth periodicity that is supported. Above this
         value, use event-driven periodic messages of a period larger than
         this value. */

    unsigned supports_10ms_periodicity : 1;
    /**< Indicates whether n*10 ms periodicities are supported.

         @values
         - 0 - Not supported
         - 1 - Supported @tablebulletend */

    unsigned supports_20ms_periodicity : 1;
    /**< Indicates whether an n*20 ms bandwidth reservation is supported.

         @values
         - 0 - Not supported
         - 1 - Supported @tablebulletend */

    unsigned supports_50ms_periodicity : 1;
    /**< Indicates whether 50 ms periodicity is supported.

         @values
         - 0 - Not supported
         - 1 - Supported @tablebulletend */

    unsigned supports_100ms_periodicity : 1;
    /**< Indicates whether the basic minimum periodicity of 100 ms is
         supported.

         @values
         - 0 - Not supported
         - 1 - Supported @tablebulletend */

    unsigned max_quantity_of_auto_retrans : 4;
    /**< Maximum number automatic retransmissions.

         @values 0 through 15 */

    unsigned size_of_layer2_mac_address : 4;
    /**< Size of the L2 MAC address.

         Different Radio Access Technologies have different-sized L2 MAC
         addresses: 802.11 has 6 bytes, whereas 3GPP PC5 has only 3 bytes.

         Because a randomized MAC address comes from an HSM with good pseudo
         random entropy, higher layers must know how many bytes of the MAC
         address to generate. */

    uint16_t v2x_number_of_priority_levels;
    /**< Number of different priority levels supported. For example, 8 is
         the current 3GPP standard (where a lower number means a higher
         priority). */

    uint16_t highest_priority_value;
    /**< Least urgent priority number supported by this radio.

         Higher numbers are lower priority, so if the full range is supported,
         this value is #V2X_PRIO_BACKGROUND. */

    uint16_t lowest_priority_value;
    /**< Highest priority value (most urgent traffic).

         Lower numbers are higher priority, so if the highest level supported
         this value is #V2X_PRIO_MOST_URGENT. */

    uint16_t max_qty_SPS_flows;
    /**< Maximum number of supported SPS reservations. */

    uint16_t max_qty_non_SPS_flows;
    /**< Maximum number of supported event flows (non-SPS ports). */

    int32_t max_tx_pwr;
    /**< Maximum supported transmission power in dBm. */

    int32_t min_tx_pwr;
    /**< Minimum supported transmission power in dBm. */

    uint32_t tx_pool_ids_supported_len;
    /**< Length of the tx_pool_ids_supported array. */

    tx_pool_id_info_t tx_pool_ids_supported[MAX_POOL_IDS_LIST_LEN];
    /**< Array of Tx pool IDs and their associated minimum and maximum
         frequencies. @newpagetable */

} v2x_iface_capabilities_t;

/**
    Converts a traffic priority to one of the 255 IPv6 traffic class bytes that
    are used in the data plane to indicate per-packet priority on non-SPS
    (event driven) data ports.

    @datatypes
    #v2x_priority_et

    @param[in] priority  Packet priority that is to be converted to an IPv6
                         traffic class. \n
                         @vertspace{3}
                         This priority is between the lowest and highest
                         priority values returned in #v2x_iface_capabilities_t.

    @detdesc
    This function is symmetric and is a reverse operation.
    @par
    The traffic priority is one of the values between min_priority_value and
    max_priority_value returned in the #v2x_iface_capabilities_t query.

    @return
    IPv6 traffic class for achieving the calling input parameter priority
    level. @newpage
 */
uint16_t v2x_convert_priority_to_traffic_class(v2x_priority_et priority);

/**
    Maps an IPv6 traffic class to a V2X priority value.

    @param[in] traffic_class  IPv6 traffic classification that came in a packet
                              from the radio.

    @detdesc
    This function is the inverse of the v2x_convert_priority_to_traffic_class()
    function. It is symmetric and is a reverse operation.

    @return
    Priority level (between highest and lowest priority values) equivalent to
    the input IPv6 traffic class parameter.
 */
v2x_priority_et v2x_convert_traffic_class_to_priority(uint16_t traffic_class);
/** @} *//* end_addtogroup telematics_cv2x_c_radio */

/** @ingroup v2x_deprecated_radio
    Deprecated. Use #v2x_tx_flow_info_t.

    Contains parameters used to convey MAC/Phy settings (such as transmit power
    limits) channel/bandwidth from the application.

    Applications might need to set these parameters in response to a WSA/WRA or
    other application-level reconfiguration (such as power reduction).
    Currently, these parameters are all transmission-profile types of
    parameters.
 */
typedef struct {
    int channel_center_khz;
    /**< Channel center frequency in kHz. */

    int channel_bandwidth_mhz;
    /**< Channel bandwidth in MHz, such as 5 MHz, 10 MHz, and 20 MHz. */

    int tx_power_limit_decidbm;
    /**< Limit on the transmit power in tenths of a dBm.

         Examples of how this field is used:
         - To reduce a range that is possible as the output of an
           application-layer congestion management scheme.
         - In cases when a small communication range is needed, such as
           indoors and electronic fare collection.
         - In ETSI use cases where the power might need to be temporarily
           restricted to accommodate a nearby mobile 5.8 CEN enforcement toll
           (EFC) reader. @tablebulletend */

    int qty_auto_retrans;
    /**< Used to request the number of automatic-retransmissions. The maximum
         supported number is defined in v2x_iface_capabilities_t. */

    uint8_t l2_source_addr_length_bytes;
    /**< Length of the L2 MAC address that is supplied to the radio. */

    uint8_t *l2_source_addr_p;
    /**< Pointer to l2_source_addr_length_bytes, which contains the L2 SRC
         address that the application layer selected for the radio. */

} v2x_radio_macphy_params_t;

/** @addtogroup telematics_cv2x_c_radio
@{ */
/**
    Used when requesting a QoS bandwidth contract, which is implemented in
    PC5 3GPP V2-X radio as a <i>Semi Persistent Flow</i> (SPS).

    The underlying radio providing the interface might support periodicities of
    various granularity in 100 ms integer multiples (such as 200 ms, 300 ms,
    and 400 ms).

    The reservation is also used internally as a handle.
 */
typedef struct {
    int v2xid;
    /**< Variable length 4-byte PSID or ITS_AID, or another application ID. */

    v2x_priority_et priority;
    /**< Specifies one of the 3GPP levels of priority for the traffic that is
         pre-reserved on the SPS flow.

         Use v2x_radio_query_capabilities() to get the exact number of
         supported priority levels. */

    int period_interval_ms;
    /**< Bandwidth-reserved periodicity interval in milliseconds.

         There are limits on which intervals the underlying radio supports.
         Use the capabilities query method to discover the
         int_min_periodicity_multiplier_ms and int_maximum_periodicity_ms
         supported intervals. */

    int tx_reservation_size_bytes;
    /**< Number of Tx bandwidth bytes that are sent every periodicity
         interval. @newpagetable */

} v2x_tx_bandwidth_reservation_t;

/**
    Contains the measurement parameters for configuring the MAC/Phy radio
    channel measurements (such as CBR utilization).

    The radio chip contains requests on radio measurement parameters that API
    clients can use to specify the following:
    - How their higher-level application requires the CBR/CBP to be measured
    - Over which time window
    - When to send a report
 */
typedef struct {
    int channel_measurement_interval_us;
    /**< Duration in microseconds of the sliding window size. */

    int rs_threshold_decidbm;
    /**< Parameter to the radio CBR measurement that is used for determining
         how busy the channel is.

         Signals weaker than the specified receive strength (RSRP, or RSSI) are
         not considered to be in use (busy). */
} v2x_chan_meas_params_t;

/**
    Periodically returned by the radio with all measurements about the radio
    channel, such as the amount of noise and bandwidth saturation
    (channel_busy_percentage, or CBR).
 */
typedef struct {
    float channel_busy_percentage;
    /**< No measurement parameters are supplied. */

    float noise_floor;
    /**< Measurement of the background noise for a quiet channel. */

    float time_uncertainty;
    /**< V2X time uncertainty in milliseconds. @newpagetable */
} v2x_chan_measurements_t;

/**
    Contains callback functions used in a v2x_radio_init() and v2x_radio_init_v2 call.

    The radio interface uses these callback functions for events such as
    completion of initialization, a Layer-02 MAC address change, or a status
    event (loss of sufficient GPS time precision to transmit).

    These callbacks are related to a specific radio interface, and its
    MAC/Phy parameters, such as transmit power, bandwidth utilization, and
    changes in radio status.
 */
typedef struct {
    /**
    Callback that indicates initialization is complete.

    @datatypes
    #v2x_status_enum_type

    @param[in] status   Updated current radio status that indicates whether
                        transmit and receive are ready.
    @param[in] context  Pointer to the context that was supplied during initial
                        registration.

    @newpage
    */
    void (*v2x_radio_init_complete)(v2x_status_enum_type status,
                                    void *context);

    /**
    Callback made when the status in the radio changes. For example, in
    response to a fault when there is a loss of GPS timing accuracy.
    @deprecated This callback is deprecated, please consider use
    %v2x_ext_radio_status_listener instead.

    @datatypes
    #v2x_event_t

    @param[in] event    Delivery of the event that just occurred, such losing
                        the ability to transmit.
    @param[in] context  Pointer to the context of the caller who originally
                        registered for this callback.
    */
    void (*v2x_radio_status_listener)(v2x_event_t event,
                                      void *context);

    /**
    Callback made from lower layers when periodic radio measurements are
    prepared.

    @datatypes
    #v2x_chan_measurements_t

    @param[in] measurements  Pointer to the periodic measurements.
    @param[in] context       Pointer to the context of the caller who
                             originally registered for this callback.
    */
    void (*v2x_radio_chan_meas_listener)(v2x_chan_measurements_t *measurements, void *context);

    /**
    Callback made by the platform SDK when the MAC address (L2 SRC address)
    changes.

    @param[in] new_l2_address  New L2 source address as an integer
                               (because the L2 address is 3 bytes).
    @param[in] context         Pointer to the context of the caller who
                               originally registered for this callback.

    @newpage
    */
    void (*v2x_radio_l2_addr_changed_listener)(int new_l2_address, void *context);

    /**
    Callback made to indicate that the requested radio MAC/Phy change (such
    as channel/frequency and power) has completed.

    @param[in] context  Pointer to the context of the caller who originally
                        registered for this callback.
    */
    void (*v2x_radio_macphy_change_complete_cb)(void *context);

    /**
    Callback made when V2X capabilities change.

    @datatypes
    #v2x_iface_capabilities_t

    @param[in] caps     Pointer to the capabilities of this interface.
    @param[in] context  Pointer to the context of the caller who originally
                        registered for this callback.
    */
    void (*v2x_radio_capabilities_listener)(v2x_iface_capabilities_t *caps,
                                            void *context);

    /**
    Callback made when the service status changes.

    @datatypes
    #v2x_service_status_t

    @param[in] status   Service status.
    @param[in] context  Pointer to the context of the caller who originally
                        registered for this callback.

    @newpage
    */
    void (*v2x_service_status_listener)(v2x_service_status_t status,
                                        void *context);

} v2x_radio_calls_t;

/**
    Contains MAC information that is reported from the actual MAC SPS in the
    radio. The offsets can periodically change on any given transmission
    report.
 */
typedef struct {
    uint32_t periodicity_in_use_ns;
    /**< Actual transmission interval period (in nanoseconds) scheduled
         relative to 1PP 0:00.00 time. */

    uint16_t currently_reserved_periodic_bytes;
    /**< Actual number of bytes currently reserved at the MAC layer. This
         number can be slightly larger than original request. */

    uint32_t tx_reservation_offset_ns;
    /**< Actual offset, from a 1PPS pulse and Tx flow periodicity, that the MAC
         selected and is using for the transmit reservation.

         If data goes to the radio with enough time, it can be transmitted on
         the medium in the next immediately scheduled slot. @newpagetable */

    uint64_t utc_time_ns;
    /**< Absolute UTC start time of next selected grant, in nanoseconds. */

} v2x_sps_mac_details_t;

/**
    Callback functions used in v2x_radio_tx_sps_sock_create_and_bind() calls.
 */
typedef struct {
    /**
    Callback made upon completion of a reservation change that a
    v2x_radio_tx_reservation_change() call initiated for a MAC/Phy contention.

    The current SPS offset and reservation parameter are passed in the details
    structure returned by the pointer details.

    @datatypes
    #v2x_sps_mac_details_t

    @param[in] context  Pointer to the application context.
    @param[in] details  Pointer to the MAC information.

    @newpage
    */
    void (*v2x_radio_l2_reservation_change_complete_cb)(void *context,
                                                        v2x_sps_mac_details_t *details);

    /**
    Callback periodically made when the MAC SPS timeslot changes. The new
    reservation offset is in the details structure returned by pointer details.

    @datatypes
    #v2x_sps_mac_details_t

    @param[in] measurements  Pointer to the channel measurements.
    @param[in] context       Pointer to the context.

    @detdesc
    This callback can occur when a MAC contention triggers a new reservation
    time slot to be selected. It is relevant only to connections opened with
    v2x_radio_tx_sps_sock_create_and_bind().

    @newpage
    */
    void (*v2x_radio_sps_offset_changed)(void *context,
                                         v2x_sps_mac_details_t *details);

} v2x_per_sps_reservation_calls_t;

/**
    V2X Tx retransmission policies supported by the modem.
 */
typedef enum {
    V2X_AUTO_RETRANSMIT_DISABLED = 0,  /**< Retransmit mode is disabled. */
    V2X_AUTO_RETRANSMIT_ENABLED = 1,   /**< Retransmit mode is enabled. */
    V2X_AUTO_RETRANSMIT_DONT_CARE = 2, /**< Modem falls back to its default
                                            behavior. */
} v2x_auto_retransmit_policy_t;

/**
    Defines possible values for SLSS sync pattern.
    Used in @ref v2x_slss_sync_ref_ue_info_t
 */
typedef enum {
    V2X_SLSS_SYNC_PATTERN_OFFSET_IND_1,   /**< UE transmits SLSS in subframes indicated by
                                               the syncOffsetIndicator1 specified in V2X
                                               configuration. */
    V2X_SLSS_SYNC_PATTERN_OFFSET_IND_2,   /**< UE transmits SLSS in subframes indicated by
                                               the syncOffsetIndicator2 specified in V2X
                                               configuration. */
    V2X_SLSS_SYNC_PATTERN_OFFSET_IND_3,   /**< UE transmits SLSS in subframes indicated by
                                               the syncOffsetIndicator3 specified in V2X
                                               configuration. */
    V2X_SLSS_SYNC_PATTERN_ODD_RESERVED,   /**< UE transmits SLSS in odd-numbered reserved
                                               subframes. */
    V2X_SLSS_SYNC_PATTERN_EVEN_RESERVED,  /**< UE transmits SLSS in even-numbered reserved
                                               subframes. */
    V2X_SLSS_SYNC_PATTERN_UNKNOWN,        /**< Unkown SLSS sync pattern. */
} v2x_slss_sync_pattern_t;

/**
 * Encapsulates parameters of an SLSS sync reference UE.
 * Used in @ref v2x_slss_rx_info_t.
 */
typedef struct {
    uint16_t slss_id;
    /**< The SLSS ID of the sync reference UE that is defined in 3GPP TS 36.331
         chapter 6.3.8. */
    bool in_coverage;
    /**< Indicates whether or not the UE is in coverage of GNSS that is defined in
         3GPP TS 36.331 chapter 6.5.2. */
    v2x_slss_sync_pattern_t pattern;
    /**< Indicates the SLSS sync pattern of the UE that is defined in 3GPP TS 36.331
         chapter 6.3.8. */
    uint8_t rsrp;
    /**< SLSS RSRP value of the UE in dBm is ((float)rsrp - 256)/2. */
    bool selected;
    /**< Indicates whether or not the sync reference UE has been selected
         as the timing source. */
} v2x_slss_sync_ref_ue_info_t;

/**
 * Encapsulates parameters of CV2X SLSS Rx Information.
 *
 * Used in @ref v2x_get_slss_rx_info and @ref v2x_slss_rx_info_listener.
 */
typedef struct {
    uint32_t num_ue;
    /**< The number of SLSS sync reference UEs in array ueInfo. */
    v2x_slss_sync_ref_ue_info_t ue_info[V2X_MAX_SLSS_SYNC_REF_UE_NUM];
    /**< Array of detected SLSS sync reference UEs. */
} v2x_slss_rx_info_t;

/**
    Advanced parameters that can be specified for Tx SPS and event-driven
    flows.
 */
typedef struct {
    v2x_auto_retransmit_policy_t retransmit_policy;
    /**< V2X retransmit policy. */

    uint8_t default_tx_power_valid;
    /**< Indicates whether the default Tx power is specified.

         @values
         - 0 - Default power is not specified
         - 1 - Default power is specified and is valid @tablebulletend */

    int32_t default_tx_power;
    /**< Default power used for transmission. */

    uint8_t mcs_index_valid;
    /**< Indicates whether the MCS index is specified.

         @values
         - 0 - Index is not specified
         - 1 - Index is specified and is valid @tablebulletend */

    uint8_t mcs_index;
    /**< MCS index number */

    uint8_t tx_pool_id_valid;
    /**< Indicates whether the Tx pool ID is valid.

         @values
         - 0 - ID is not specified
         - 1 - ID is specified and is valid @tablebulletend */

    uint8_t tx_pool_id;
    /**< ID of the Tx pool. */

    uint8_t is_unicast_valid;
    /**< Indicates whether is_unicast is specified.

         @values
         - 0 - Is unicast is not specified
         - 1 - Is unicast is specified and is valid @tablebulletend */

    uint8_t is_unicast;
    /**< Non zero if requested flow is unicast.
         Note: Unicast flows ignore subscribed Service Ids */

} v2x_tx_flow_info_t;

/**
    Parameters to identify a Tx or Rx socket.
 */
typedef struct {
    int sock; /**< Pointer to the file descriptor for the socket. */

    struct sockaddr_in6 sockaddr; /**< IPv6 socket address. The sockaddr_in6 buffer is initialized with the IPv6 source address and source port that are used for the bind() function. */

} v2x_sock_info_t;

/**
    Parameters to identify a service ID list.
 */
typedef struct {
    int length;
    /**< number of services IDs included in the array of sid. */

    uint32_t sid[MAX_SUBSCRIBE_SIDS_LIST_LEN];
    /**< array of service IDs. */
} v2x_sid_list_t;

/**
    Advanced parameters that can be specified for Tx SPS flows.
 */
typedef struct {
    v2x_tx_bandwidth_reservation_t reservation;
    /**< Transmit reservation information. */

    v2x_tx_flow_info_t flow_info;
    /**< Transmit resource information about the SPS Tx flow. */

} v2x_tx_sps_flow_info_t;

/**
    Parameters that can be specified for the creation of CV2X socket.
 */
typedef struct {
    uint32_t service_id;
    /**< V2X service ID bound to the CV2X socket. */

    uint16_t local_port;
    /**< Local port number of the CV2X socket used for binding. */
} socket_info_t;

/**
    V2X Ip Types
 */
typedef enum {
    TRAFFIC_IP = 0,     /**< Use Ip type traffic. */
    TRAFFIC_NON_IP = 1  /**< Use Non-Ip type traffic. @newpage */
} traffic_ip_type_t;

typedef traffic_ip_type_t traffic_ip_type;

/**
 * Contains remote UE source L2 ID that expecting to filter.
 */
typedef struct src_l2_filter_info_t {
    /** remote UE L2 addr to filter. */
    uint32_t src_l2_id;

    /** Duration, in millisec (resolution 100 msec). */
    uint32_t duration_ms;

    /** Proximity service per packet priority (PPPP), packets with priority above this value
         will be dropped. Range 0-7, 0 mean all of the pkts will be dropped. */
    uint8_t pppp;
} src_l2_filter_info;

/**
    Fault detection for Tx chain that including PA and front end.
 */
typedef enum {
    INACTIVE,      /**< The Tx chain is not working. */
    OPERATIONAL,   /**< The Tx chain is operational. */
    FAULT,         /**< Fault detected on the Tx chain. */
} rf_status_t;

/**
    Tx status per Tx chain and Tx power per Tx antenna for a specific transport block.
 */
typedef struct {
    rf_status_t status;
    /**< Fault detection status for a specific Tx chain. */
    int32_t power;
    /**< The target Tx power after MPR/AMPR reduction for a specific Tx antenna
         in dBm*10 format. Invalid value is -700, it means the corresponding
         antenna is not being used for the transmission of this transport block. */
} v2x_rf_tx_info_t;

/**
    Defines possible values for the segment type of a transport block.
 */
typedef enum {
    FIRST,      /**< V2X packet is segmented, it's the first transport block. */
    LAST,       /**< V2X packet is segmented, it's the last transport block. */
    MIDDLE,     /**< V2X packet is segmented, it's a transport block between first and last. */
    ONLY_ONE,   /**< V2X packet is not segmented, it's the only one transport block. */
} v2x_segment_type_t;


/**
    Defines new Tx or re-Tx type relevant to a transport block.
 */
typedef enum {
    V2X_NEW_TX,        /**< New Tx of the V2X transport block. */
    V2X_RE_TX,         /**< Re-Tx of the V2X transport block. */
    V2X_SLSS_TX,       /**< Tx of SLSS. */
} v2x_tx_type_t;

/**
    Information on Tx status of a V2X transport block that is reported
    from low layer.
    1. A V2X Tx packet might trigger multiple reports because of the segmentaion
    and re-Tx in low layer.
    2. If a transport block is dropped in low layer, no report will be triggered
    for that transport block.
    3. The power in the array of rfInfo is the target Tx power value in dBm*10 after
    MPR/AMPR reduction for a specific Tx antenna. The status in the array of rfInfo
    is the fault detection status for a specific Tx chain.
     - In CDD mode, two antennas have transmission for a specific transport block,
    both rfInfo[0].power and rfInfo[1].power are valid (not -700), rfInfo[i].status
    is reflecting the status of Tx chain/Tx antenna i.
     - In TXD mode, data transmission swtiches between two antennas/chains and only
    one antenna/chain has transmission for a specific transport block, the Tx antenna
    being used has valid power (not -700) in the array of rfInfo, rfInfo[i].status
    is reflecting the status of Tx chain i or the status of the Tx antenna i whose
    power is valid (not -700) in the array of rfInfo.
    Used in @ref v2x_tx_status_report_listener
 */
typedef struct {
    v2x_rf_tx_info_t rf_info[V2X_MAX_ANTENNAS_SUPPORTED];
    /**< Tx status per Tx chain and Tx power per Tx antenna. */
    uint8_t num_rb;
    /**< Number of resource blocks used for the transport block. */
    uint8_t start_rb;
    /**< Start resource block index used for the transport block. */
    uint8_t mcs;
    /**< Modulation and coding scheme used for the transport block
         that is defined in 3GPP TS 36.213. */
    uint8_t seg_num;
    /**< Total number of segments of a V2X packet. */
    v2x_segment_type_t seg_type;
    /**< Segment type of the transport block. */
    v2x_tx_type_t tx_type;
    /**< Indication of new Tx or re-Tx of the transport block. */
    uint16_t ota_timing;
    /**< OTA timing in format of system frame number*10 + subframe number. */
    uint16_t port;
    /**< Port number that can be used to link the report to a specific Tx
         flow which has the same source port number. */
} v2x_tx_status_report_t;

/**
    Callback made when a CV2X transport block is transmitted in low layer if
    a listener has been registered by calling @ref v2x_register_tx_status_report_listener.

    @datatypes
    #v2x_tx_status_report_t

    @param[in] info     V2X Tx status report information.
    @newpage
*/
typedef void (*v2x_tx_status_report_listener)(const v2x_tx_status_report_t info);

/**
     Callback made when CV2X Tx/Rx status is changed and a listener has been registered
     by calling @ref v2x_register_ext_radio_status_listener.

    @datatypes
    #v2x_radio_status_ex_t

    @param[out] status     Pointer to V2X overall Tx/Rx status and per pool status.
    @newpage
*/
typedef void (*v2x_ext_radio_status_listener)(const v2x_radio_status_ex_t* status);

/**
    Called when CV2X SLSS Rx is enabled and any of below events has occurred:
    - A new SLSS synce reference UE is detected, lost, or selected as the timing source,
      report the present sync reference UEs.
    - UE timing source switches from SLSS to GNSS, report 0 sync reference UE.
    - SLSS Rx is disabled, report 0 sync reference UE.
    - Cv2x is stopped, report 0 sync reference UE.

    @datatypes
    #v2x_slss_rx_info_t

    @param[out] info     Pointer to V2X SLSS Rx information.
    @newpage
*/
typedef void (*v2x_slss_rx_listener)(const v2x_slss_rx_info_t* info);

/**
    Method used to query the platform SDK for its version number, build
    information, and build date.

    @return
    v2x_api_ver_t - Contains the build date and API version number. @newpage
 */
extern v2x_api_ver_t v2x_radio_api_version();

/**
    Gets the capabilities of CV2X radio.

    @datatypes
    #v2x_iface_capabilities_t

    @param[out] caps       Pointer to the v2x_iface_capabilities_t structure,
                           which contains the capabilities of this specific
                           interface.

    @return
    #V2X_STATUS_SUCCESS - The radio is ready for data-plane sockets to be
    created and bound.
    @par
    Error code - If there is a problem (see #v2x_status_enum_type).
 */
extern v2x_status_enum_type v2x_radio_query_capabilities(v2x_iface_capabilities_t *caps);
/** @} *//* end_addtogroup telematics_cv2x_c_radio */

/** @ingroup v2x_deprecated_radio
    Deprecated. Gets the capabilities of a specific Radio interface attached to the system.

    @datatypes
    #v2x_iface_capabilities_t

    @param[in] iface_name  Pointer to the Radio interface name. \n
                           @vertspace{3}
                           The Radio interface is one of the following:
                           - An RmNet interface (HLOS)
                           - The interface supplied for IP communication
                           - The interface for non-IP communication (such as
                             WSMP and Geonetworking). @vertspace{-13}
    @param[out] caps       Pointer to the v2x_iface_capabilities_t structure,
                           which contains the capabilities of this specific
                           interface.

    @return
    #V2X_STATUS_SUCCESS - The radio is ready for data-plane sockets to be
    created and bound.
    @par
    Error code - If there is a problem (see #v2x_status_enum_type).
 */
extern v2x_status_enum_type v2x_radio_query_parameters(const char *iface_name, v2x_iface_capabilities_t *caps);

/** @ingroup v2x_deprecated_radio
    Deprecated. Pass #traffic_ip_type_t on radio init.

    Initializes the Radio interface and sets the callback that will be used
    when events in the radio change (including when radio initialization is
    complete).

    @datatypes
    #v2x_concurrency_sel_t \n
    #v2x_radio_calls_t

    @param[in] interface_name  Pointer to the NULL-terminated parameter that
                               specifies which Radio interface name caller is
                               to be initialized (the IP or non-IP interface of
                               a specific name). \n
                               @vertspace{3}
                               The Radio interface is one of the following:
                               - An RmNet interface (HLOS)
                               - The interface supplied for IP communication
                               - The interface for non-IP communication (such
                                 as WSMP and Geonetworking). @vertspace{-13}
    @param[in] mode            WAN concurrency mode, although the radio might
                               not support concurrency. Errors can be generated.
    @param[in] callbacks       Pointer to the v2x_radio_calls_t structure that
                               is prepopulated with function pointers used
                               during radio events (such as loss of time
                               synchronization or accuracy) for subscribers. \n
                               @vertspace{3}
                               This parameter also points to a callback for
                               this initialization function.
    @param[in] context         Voluntary pointer to the first parameter on the
                               callback.

    @detdesc
    This function call is a blocking, and it is a control plane action.
    @par
    Use v2x_radio_deinit() when radio operations are complete.
    @par
    Callback @ref v2x_radio_calls_t::v2x_radio_init_complete()is made when
    initialization is complete.

    @return
    Handle to the specified initialized radio. The handle is used for
    reconfiguring, opening or changing, and closing reservations.
    @par
    #V2X_RADIO_HANDLE_BAD - If there is an error. No initialization callback
    is made. @newpage
 */
v2x_radio_handle_t v2x_radio_init(char *interface_name,
                                  v2x_concurrency_sel_t mode,
                                  v2x_radio_calls_t *callbacks,
                                  void *context);

/** @ingroup v2x_deprecated_radio
    Deprecated. Pass #v2x_tx_flow_info_t on flow creation.

    Configures the MAC/Phy parameters on an initialized radio handle to
    either an IP or non-IP radio. Parameters include the source L2 address,
    channel, bandwidth, and transmit power.

    After the radio has been configured or changed, a callback to
    #v2x_radio_calls_t::v2x_radio_macphy_change_complete_cb() is made with the
    supplied context.

    @datatypes
    #v2x_radio_handle_t \n
    #v2x_radio_macphy_params_t

    @param[in] handle   Identifies the initialized Radio interface.
    @param[in] macphy   Pointer to the MAC/Phy parameters to be configured.
    @param[in] context  Voluntary pointer to the context that will be supplied
                        as the first parameter in the callback.

    @return
    #V2X_STATUS_SUCCESS - The radio is now ready for data-plane sockets to be
    open and bound.
    @par
    Error code - If there is a problem (see #v2x_status_enum_type).

    @dependencies
    The interface must be pre-initialized with v2x_radio_init() or v2x_radio_init_v2(). The handle
    from that function must be used as the parameter in this function. @newpage
 */
extern v2x_status_enum_type v2x_radio_set_macphy(v2x_radio_handle_t handle, v2x_radio_macphy_params_t *macphy,
        void *context);

/** @addtogroup telematics_cv2x_c_radio
@{ */
/**
    De-initializes a specific Radio interface.

    @datatypes
    #v2x_radio_handle_t

    @param[in] handle  Handle to the Radio that was initialized.

    @return
    Indication of success or failure (see #v2x_status_enum_type).

    @dependencies
    The interface must be pre-initialized with v2x_radio_init() or v2x_radio_init_v2(). The handle
    from that function must be used as the parameter in this function. @newpage
 */
extern v2x_status_enum_type v2x_radio_deinit(v2x_radio_handle_t handle);

/**
    Opens a new V2X radio receive socket, and initializes the given sockaddr
    buffer. The socket is also bound as an AF_INET6 UDP type socket.

    On platforms with access control enabled, the caller needs to have TELUX_CV2X_FLOW_OPS
    permission to successfully invoke this API.

    @datatypes
    #v2x_radio_handle_t

    @param[in] handle        Identifies the initialized Radio interface.
    @param[out] sock         Pointer to the socket that, on success, returns
                             the socket descriptor. The caller must release 
                             this socket with v2x_radio_sock_close().
    @param[out] rx_sockaddr  Pointer to the IPv6 UDP socket. The sockaddr_in6
                             buffer is initialized with the IPv6 source address
                             and source port that are used for the bind.

    @detdesc
    You can execute any sockopts that are appropriate for this type of socket
    (AF_INET6).
    @par
    The port number for the receive path is not exposed, but it is in the
    sockaddr_in6 structure (if the caller is interested).

    @return
    0 -- On success.
    @par
    Otherwise:
     - EPERM - Socket creation failed; for more details, check errno.h.
     - EAFNOSUPPORT - On failure to find the interface.
     - EACCES - On failure to get the MAC address of the device.

    @dependencies
    The interface must be pre-initialized with v2x_radio_init() or v2x_radio_init_v2(). The handle
    from that function must be used as the parameter in this function. @newpage
 */
extern int v2x_radio_rx_sock_create_and_bind(v2x_radio_handle_t handle, int *sock, struct sockaddr_in6 *rx_sockaddr);

/**
    Opens a new V2X radio receive socket with specific service IDs for subscription,
    and initializes the given sockaddr buffer. The socket is also bound as an
    AF_INET6 UDP type socket.

    On platforms with access control enabled, the caller needs to have TELUX_CV2X_FLOW_OPS
    permission to successfully invoke this API.

    @datatypes
    #v2x_radio_handle_t

    @param[in] handle        Identifies the initialized Radio interface.
    @param[in] id_ist_len    Identifies the length of service ID list.
    @param[in] id_list       Pointer to the service ID list for subscription,
                             subscribe wildcard if input nullptr.
    @param[out] sock         Pointer to the socket that, on success, returns
                             the socket descriptor. The caller must release
                             this socket with v2x_radio_sock_close().
    @param[out] rx_sockaddr  Pointer to the IPv6 UDP socket. The sockaddr_in6
                             buffer is initialized with the IPv6 source address
                             and source port that are used for the bind.

    @detdesc
    You can execute any sockopts that are appropriate for this type of socket
    (AF_INET6).
    @par
    This API can be used to subscribe wildcard, catchall port, or specifc service IDs.
    The Rx port should be set with v2x_set_rx_port() before any subscription via this API,
    otherwise a default port number 9000 will be used.
    @par
    Wildcard is used to receive all traffic. Only one port can be registered as
    wildcard port. Once wildcard is registered successfully, all received packets
    will be directed to wildcard port, and any subscription for specific service
    IDs or catchall port at other ports will be invalid. The parameter id_list
    of this API should be set to a null list for wildcard subscription.
    @par
    Catchall port is used to receive packets with non-registered service IDs
    (via specific service IDs subscription). Only one port can be registered
    as catchall port. If catchall port is registered successfully, received
    packets with non-registered service ID will be directed to catchall port.
    All specific service IDs subscription (if any) should be performed before
    catchall port subscription. The parameter id_list of this API should include
    all non-registered service IDs for catchall port subscription.
    @par
    Any port different from catchall port can be used to receive packets with specific
    service IDs. Only one port can be registered for a single service ID, a list of
    service IDs can be registered at a single port. To subscribe specific service IDs
    at a given Rx port, a Tx flow must be pre-setup with the Tx service ID set to any
    service ID included in the list of specific service IDs and the Tx source port set
    to the same port number as Rx port. The parameter id_list of this API should include
    all interested service IDs for the given Rx port.

    @return
    0 -- On success.
    @par
    Otherwise:
     - EPERM - Socket creation failed; for more details, check errno.h.
     - EAFNOSUPPORT - On failure to find the interface.
     - EACCES - On failure to get the MAC address of the device.

    @dependencies
    The interface must be pre-initialized with v2x_radio_init(). The handle from
    that function must be used as the parameter in this function. The Rx port
    must be pre-set with v2x_set_rx_port(), otherwise a default port number will
    be used. For any specific service ID subscription, a Tx flow must be pre-setup
    using one of the following methods:
    - v2x_radio_tx_sps_sock_create_and_bind()
    - v2x_radio_tx_sps_sock_create_and_bind_v2()
    - v2x_radio_tx_sps_only_create()
    - v2x_radio_tx_sps_only_create_v2()
    - v2x_radio_tx_event_sock_create_and_bind()
    - v2x_radio_tx_event_sock_create_and_bind_v2()
    - v2x_radio_tx_event_sock_create_and_bind_v3() @newpage
 */
extern int v2x_radio_rx_sock_create_and_bind_v2(v2x_radio_handle_t handle,
    int id_ist_len,
    uint32_t *id_list,
    int *sock,
    struct sockaddr_in6 *rx_sockaddr);

/**
    Opens a new V2X radio receive socket with specific service IDs for subscription
    and specifc port number for the receive path, and initializes the given sockaddr
    buffer. The socket is also bound as an AF_INET6 UDP type socket.

    This %v2x_radio_rx_sock_create_and_bind_v3() method differs from
    v2x_radio_rx_sock_create_and_bind_v2() in that you can use the
    port_num parameter to specify the port number for the receive path.

    On platforms with access control enabled, the caller needs to have TELUX_CV2X_FLOW_OPS
    permission to successfully invoke this API.

    @datatypes
    #v2x_radio_handle_t

    @param[in] handle        Identifies the initialized Radio interface.
    @param[in] port_num      Identifies the port number for the receive path.
    @param[in] id_ist_len    Identifies the length of service ID list.
    @param[in] id_list       Pointer to the service ID list for subscription,
                             subscribe wildcard if input nullptr.
    @param[out] sock         Pointer to the socket that, on success, returns
                             the socket descriptor. The caller must release
                             this socket with v2x_radio_sock_close().
    @param[out] rx_sockaddr  Pointer to the IPv6 UDP socket. The sockaddr_in6
                             buffer is initialized with the IPv6 source address
                             and source port that are used for the bind.

    @detdesc
    You can execute any sockopts that are appropriate for this type of socket
    (AF_INET6).
    @par
    This API can be used to subscribe wildcard, catchall port, or specifc service IDs.
    @par
    Wildcard is used to receive all traffic. Only one port can be registered as
    wildcard port. Once wildcard is registered successfully, all received packets
    will be directed to wildcard port, and any subscription for specific service
    IDs or catchall port at other ports will be invalid. The parameter id_list
    of this API should be set to a null list for wildcard subscription.
    @par
    Catchall port is used to receive packets with non-registered service IDs
    (via specific service IDs subscription). Only one port can be registered
    as catchall port. If catchall port is registered successfully, received
    packets with non-registered service ID will be directed to catchall port.
    All specific service IDs subscription (if any) should be performed before
    catchall port subscription. The parameter id_list of this API should include
    all non-registered service IDs for catchall port subscription.
    @par
    Any port different from catchall port can be used to receive packets with specific
    service IDs. Only one port can be registered for a single service ID, a list of
    service IDs can be registered at a single port. To subscribe specific service IDs
    at a given Rx port, a Tx flow must be pre-setup with the Tx service ID set to any
    service ID included in the list of specific service IDs and the Tx source port set
    to the same port number as Rx port. The parameter id_list of this API should include
    all interested service IDs for the given Rx port.

    @return
    0 -- On success.
    @par
    Otherwise:
     - EPERM - Socket creation failed; for more details, check errno.h.
     - EAFNOSUPPORT - On failure to find the interface.
     - EACCES - On failure to get the MAC address of the device.

    @dependencies
    The interface must be pre-initialized with v2x_radio_init(). The handle from
    that function must be used as the parameter in this function. For any specific
    service ID subscription, a Tx flow must be pre-setup using one of the following
    methods:
    - v2x_radio_tx_sps_sock_create_and_bind()
    - v2x_radio_tx_sps_sock_create_and_bind_v2()
    - v2x_radio_tx_sps_only_create()
    - v2x_radio_tx_sps_only_create_v2()
    - v2x_radio_tx_event_sock_create_and_bind()
    - v2x_radio_tx_event_sock_create_and_bind_v2()
    - v2x_radio_tx_event_sock_create_and_bind_v3() @newpage
 */
extern int v2x_radio_rx_sock_create_and_bind_v3(v2x_radio_handle_t handle,
    uint16_t port_num,
    int id_ist_len,
    uint32_t *id_list,
    int *sock,
    struct sockaddr_in6 *rx_sockaddr);

/**
     Enable or disable the meta data report for the packets corresponding to the service IDs.

     If enabled, the meta data report would be generated in addition to the actual OTA payload
     packet, and it comes from the same data interface as the OTA packet itself, it consist of
     RF RSSI (received signal strength indicator) status, 32-bit SCI Format 1 (3GPP TS 36.213,
     section 14.1), packet delay estimation, L2 destination ID, and the resource blocks used for
     the packet's transmission: subframe, subchannel index.

    On platforms with access control enabled, the caller needs to have TELUX_CV2X_INFO
    permission to successfully invoke this API.

     @datatypes
     #v2x_radio_handle_t

     @param[in]  handle           Identifies the initialized Radio interface.
     @param[in]  enable           enable or disable the meta data
     @param[in]  id_list_len      number of the service IDs provided in the id_list
     @param[in]  id_list          Pointer to the Rx service ID list

     @detdesc
     This function extracts the received packet's meta data from the payload, currently
     only NON-IP packets can have the meta data reported, it is not supported yet for
     IP packets.
     @par
     If the meta data report is enabled for certain services, call #v2x_parse_rx_meta_data to
     extract the meta data by providing a pointer to a object of type #rx_packet_meta_data_t, and
     the real payload.

     @return
     0 -- On success.
 */
extern int v2x_radio_enable_rx_meta_data(v2x_radio_handle_t handle,
                                         bool enable,
                                         int id_list_len,
                                         uint32_t *id_list);
/**
    Creates Tx SPS socket, Tx Event socket and Rx socket with specified parameters.
    The socket is also bound as an AF_INET6 UDP type socket.

    This %v2x_radio_sock_create_and_bind() method is the combination of function
    v2x_radio_tx_sps_sock_create_and_bind_v2()/v2x_radio_tx_event_sock_create_and_bind_v2
    in the transmit direction and function v2x_radio_rx_sock_create_and_bind_v3()
    in the receiving direction.

    On platforms with access control enabled, the caller needs to have TELUX_CV2X_FLOW_OPS
    permission to successfully invoke this API.

    @datatypes
    #v2x_radio_handle_t

    @param[in]  handle           Identifies the initialized Radio interface.
    @param[in]  tx_flow_info     Pointer to the Tx SPS or event flow information.
                                 To create event flow, set reservation.v2xid
                                 and flow_info in this structure.
    @param[in]  calls            Pointer to reservation callbacks or listeners.
                                 \n @vertspace{3}
                                 This parameter is called when underlying radio
                                 MAC parameters change related to the SPS
                                 bandwidth contract.
                                 For example, the callback after a
                                 reservation change, or if the timing offset of
                                 the SPS adjusts itself in response to
                                 traffic. \n @vertspace{3}
                                 This parameter passes NULL if no callbacks are
                                 required.
    @param[in]  tx_sps_portnum   Requested Tx source port number for SPS transmissions,
                                 or -1 for no Tx sps flow.
    @param[in]  tx_event_portnum Requested Tx source port number for event transmissions,
                                 or -1 for no Tx event flow.
    @param[in]  rx_portnum       Requested Rx destination port number, or -1 for no Rx
                                 subscription.
    @param[in]  rx_id_list       Pointer to the Rx service ID list for subscription,
                                 subscribe wildcard if input nullptr.
    @param[out] tx_sps_sock      Pointer to the Tx sps socket that, on success, returns
                                 the socket descriptor and the IPv6 socket address. The
                                 caller must release this socket with v2x_radio_sock_close().
    @param[out] tx_event_sock    Pointer to the Tx event socket that, on success, returns
                                 the socket descriptor and the IPv6 socket address. The
                                 caller must release this socket with v2x_radio_sock_close().
    @param[out] rx_sock          Pointer to the Rx socket that, on success, returns
                                 the socket descriptor and the IPv6 socket address. The
                                 caller must release this socket with v2x_radio_sock_close().

    @detdesc
    You can execute any sockopts that are appropriate for this type of socket
    (AF_INET6).
    @par
    This API can be used for the registeration of both Tx and Rx. It sets up sockets
    on the requested port numbers. A negative port number corresponds to no actions
    for Tx or Rx.

    @par
    Wildcard is used to receive all traffic. Only one port can be registered as
    wildcard port. Once wildcard is registered successfully, all received packets
    will be directed to wildcard port, and any subscription for specific service
    IDs or catchall port at other ports will be invalid. The parameter rx_id_list
    of this API should be set to a null list for wildcard subscription.
    @par
    Catchall port is used to receive packets with non-registered service IDs
    (via specific service IDs subscription). Only one port can be registered
    as catchall port. If catchall port is registered successfully, received
    packets with non-registered service ID will be directed to catchall port.
    All specific service IDs subscription (if any) should be performed before
    catchall port subscription. The parameter rx_id_list of this API should
    include all non-registered service IDs for catchall port subscription.
    @par
    Any port different from catchall port can be used to receive packets with specific
    service IDs. Only one port can be registered for a single service ID, a list of
    service IDs can be registered at a single port. To subscribe specific service IDs
    at a given Rx port, a Tx flow should also be set up using this API. The parameter
    rx_id_list should include all interested service IDs for the given Rx port, the
    parameter tx_flow_info.reservation.v2xid should be set to one of the service ID
    included in rx_id_list, the parmenter tx_sps_portnum or tx_event_portnum should
    be set to the same port number as rx_portnum.

    @return
    0 -- On success.
    @par
    Otherwise:
     - EPERM - Socket creation failed; for more details, check errno.h.
     - EAFNOSUPPORT - On failure to find the interface.
     - EACCES - On failure to get the MAC address of the device.

    @dependencies
    The interface must be pre-initialized with v2x_radio_init(). The handle from
    that function must be used as the parameter in this function. @newpage
 */
extern int v2x_radio_sock_create_and_bind(
    v2x_radio_handle_t handle,
    v2x_tx_sps_flow_info_t *tx_flow_info,
    v2x_per_sps_reservation_calls_t *calls,
    int tx_sps_portnum,
    int tx_event_portnum,
    int rx_portnum,
    v2x_sid_list_t *rx_id_list,
    v2x_sock_info_t *tx_sps_sock,
    v2x_sock_info_t *tx_event_sock,
    v2x_sock_info_t *rx_sock);

/**
    Creates and binds a socket with a bandwidth-reserved (SPS) Tx flow with the
    requested ID, priority, periodicity, and size on a specified source port
    number. The socket is created as an IPv6 UDP socket.

    On platforms with access control enabled, the caller needs to have TELUX_CV2X_FLOW_OPS
    permission to successfully invoke this API.

    @datatypes
    #v2x_radio_handle_t \n
    v2x_tx_bandwidth_reservation_t \n
    v2x_per_sps_reservation_calls_t

    @param[in]  handle          Identifies the initialized Radio interface on
                                which this data connection is made.
    @param[in]  res             Pointer to the parameter structure (how often
                                the structure is sent, how many bytes are
                                reserved, and so on).
    @param[in]  calls           Pointer to reservation callbacks or listeners.
                                \n @vertspace{3}
                                This parameter is called when underlying radio
                                MAC parameters change related to the SPS
                                bandwidth contract.
                                For example, the callback after a
                                reservation change, or if the timing offset of
                                the SPS adjusts itself in response to
                                traffic. \n
                                @vertspace{3}
                                This parameter passes NULL if no callbacks are
                                required.
    @param[in]  sps_portnum     Requested source port number for the bandwidth
                                reserved SPS transmissions.
    @param[in]  event_portnum   Requested source port number for the bandwidth
                                reserved event transmissions, or  -1 for no
                                event port.
    @param[out] sps_sock        Pointer to the socket that is bound to the
                                requested port for Tx with reserved bandwidth.
    @param[out] sps_sockaddr    Pointer to the IPv6 UDP socket. \n
                                @vertspace{3}
                                The sockaddr_in6 buffer is initialized with the
                                IPv6 source address and source port that are
                                used for the bind() function.
                                The caller can then use the buffer for
                                subsequent sendto() function calls.
    @param[out] event_sock      Pointer to the socket that is bound to the
                                event-driven transmission port.
    @param[out] event_sockaddr  Pointer to the IPV6 UDP socket. \n
                                @vertspace{3}
                                The sockaddr_in6 buffer is initialized with the
                                IPv6 source address and source port that are
                                used for the bind() function.
                                The caller can then use the buffer for
                                subsequent sendto() function calls.

    @newpage
    @detdesc
    The radio attempts to reserve the flow with the specified size and rate
    passed in the request parameters.
    @par
    This function is used only for Tx. It sets up two UDP sockets on the
    requested two HLOS port numbers.
    @par
    For only a single SPS flow, indicate the event port number by using a
    negative number or NULL for the event_sockaddr. For a single event-driven
    port, use v2x_radio_tx_event_sock_create_and_bind() instead.
    @par
    Because the modem endpoint requires a specific global address, all data
    sent on these sockets must have a configurable IPv6 destination address for
    the non-IP traffic.
    @par
    The Priority parameter of the SPS reservation is used only for the reserved
    Tx bandwidth (SPS) flow. The non-SPS/event-driven data sent to the event_portnum
    parameter is prioritized on the air, based on the IPv67 Traffic Class of the packet.
    @par
    The caller is expected to identify two unused local port numbers to use for
    binding: one for the event-driven flow and one for the SPS flow.
    @par
    This call is a blocking call. When it returns, the sockets are ready to
    use, assuming there is no error.

    @return
    0 - On success.
    @par
    Otherwise:
     - EPERM - Socket creation failed; for more details, check errno.h.
     - EAFNOSUPPORT - On failure to find the interface.
     - EACCES - On failure to get the MAC address of the device.

    @dependencies
    The interface must be pre-initialized with v2x_radio_init() or v2x_radio_init_v2(). The handle
    from that function must be used as the parameter in this function. @newpage
*/
extern int v2x_radio_tx_sps_sock_create_and_bind(v2x_radio_handle_t handle,
        v2x_tx_bandwidth_reservation_t *res,
        v2x_per_sps_reservation_calls_t *calls,
        int sps_portnum,
        int event_portnum,
        int *sps_sock,
        struct sockaddr_in6 *sps_sockaddr,
        int *event_sock,
        struct sockaddr_in6 *event_sockaddr);

/**
    Creates a socket with a bandwidth-reserved (SPS) Tx flow.

    Only SPS transmissions are to be implemented for the socket, which is created
    as an IPv6 UDP socket.

    On platforms with access control enabled, the caller needs to have TELUX_CV2X_FLOW_OPS
    permission to successfully invoke this API.

    @datatypes
    #v2x_radio_handle_t \n
    #v2x_tx_bandwidth_reservation_t \n
    #v2x_per_sps_reservation_calls_t

    @param[in]  handle          Identifies the initialized Radio interface on
                                which this data connection is made.
    @param[in]  res             Pointer to the parameter structure (how often
                                the structure is sent, how many bytes are
                                reserved, and so on).
    @param[in]  calls           Pointer to reservation callbacks or listeners.
                                \n @vertspace{3}
                                This parameter is called when underlying radio
                                MAC parameters change related to the SPS
                                bandwidth contract.
                                For example, the callback after a
                                reservation change, or if the timing offset of
                                the SPS adjusts itself in response to
                                traffic. \n
                                @vertspace{3}
                                This parameter passes NULL if no callbacks are
                                required.
    @param[in]  sps_portnum     Requested source port number for the bandwidth
                                reserved SPS transmissions.
    @param[out] sps_sock        Pointer to the socket that is bound to the
                                requested port for Tx with reserved bandwidth.
    @param[out] sps_sockaddr    Pointer to the IPv6 UDP socket. \n
                                @vertspace{3}
                                The sockaddr_in6 buffer is initialized with the
                                IPv6 source address and source port that are
                                used for the bind() function.
                                The caller can then use the buffer for
                                subsequent sendto() function calls.

    @detdesc
    The radio attempts to reserve the flow with the specified size and rate
    passed in the request parameters.
    @par
    This function is used only for Tx. It sets up a UDP socket on the
    requested HLOS port number.
    Because the modem endpoint requires a specific global address, all data
    sent on the socket must have a configurable IPv6 destination address for
    the non-IP traffic.
    @par
    The Priority parameter of the SPS reservation is used only for the reserved
    Tx bandwidth (SPS) flow.
    @par
    The caller is expected to identify an unused local port number for the SPS
    flow.
    @par
    This call is a blocking call. When it returns, the socket is ready to
    use, assuming there is no error.

    @return
    0 -- On success.
    @par
    Otherwise:
     - EPERM - Socket creation failed; for more details, check errno.h.
     - EINVAL - On failure to find the interface or get bad parameters.

    @dependencies
    The interface must be pre-initialized with v2x_radio_init() or v2x_radio_init_v2(). The handle
    from that function must be used as the parameter in this function. @newpage
*/
extern int v2x_radio_tx_sps_only_create(v2x_radio_handle_t handle,
    v2x_tx_bandwidth_reservation_t *res,
    v2x_per_sps_reservation_calls_t *calls,
    int sps_portnum,
    int *sps_sock,
    struct sockaddr_in6 *sps_sockaddr);

/**
    Adjusts the reservation for transmit bandwidth.

    @datatypes
    v2x_tx_bandwidth_reservation_t

    @param[out] sps_sock             Pointer to the socket bound to the
                                     requested port.
    @param[in]  updated_reservation  Pointer to a bandwidth reservation with
                                     new reservation information.

    On platforms with access control enabled, the caller needs to have TELUX_CV2X_FLOW_OPS
    permission to successfully invoke this API.

    @detdesc
    This function can be used as follows:
    - When the bandwidth requirement changes in periodicity (for example, due
      to an application layer DCC algorithm)
    - Because the packet size is increasing (for example, due to a growing path
        history size in a BSM).
    - When user wants to update reservation priority.
    @par
    When the reservation change is complete, a callback to the structure is passed
    in a v2x_radio_init() or v2x_radio_init_v2() call.

    @return
    #V2X_STATUS_SUCCESS.
    @par
    Error code - If there is a problem (see #v2x_status_enum_type).

    @dependencies
    An SPS flow must have been successfully initialized with the
    v2x_radio_tx_sps_sock_create_and_bind(). @newpage
 */
extern v2x_status_enum_type v2x_radio_tx_reservation_change(int *sps_sock,
        v2x_tx_bandwidth_reservation_t *updated_reservation);
/** @} *//* end_addtogroup telematics_cv2x_c_radio */

/** @ingroup v2x_deprecated_radio
    Obsolete. This API is not supported and will be removed in the future.

    Flushes the radio transmitter queue.

    This function is used for all packets on the specified interface that have
    not been sent yet. This action is necessary when a radio MAC address change
    is coordinated for anonymity.

    @param[in] interface  Name of the Radio interface operating system.

    @return
    None.
 */
extern void v2x_radio_tx_flush(char *interface);

/** @addtogroup telematics_cv2x_c_radio
@{ */
/**
    Opens and binds an event-driven socket (one with no bandwidth reservation).
    The socket is bound as an AF_INET6 UDP type socket.

    On platforms with access control enabled, the caller needs to have TELUX_CV2X_FLOW_OPS
    permission to successfully invoke this API.

    @param[in]  interface        Pointer to the operating system name to use.
                                 This interface is an RmNet interface (HLOS).
    @param[in]  v2x_id           Used for transmissions that are ultimately
                                 mapped to an L2 destination address.
    @param[in]  event_portnum    Local port number to which the socket is
                                 bound. Used for transmissions of this ID.
    @param[out] event_sock_addr  Pointer to the sockaddr_ll structure buffer
                                 to be initialized.
    @param[out] sock             Pointer to the file descriptor. Loaded when
                                 the function is successful.

    @detdesc
    This function is used only for Tx when no periodicity is available for the
    application type. If you know your transmit data periodicity, use
    v2x_radio_tx_sps_sock_create_and_bind() instead.
    @par
    These event-driven sockets pay attention to QoS parameters in the IP
    socket.

    @return
    0 - On success.
    @par
    Otherwise:
     - EPERM - Socket creation failed; for more details, check errno.h.
     - EAFNOSUPPORT - On failure to find the interface.
     - EACCES - On failure to get the MAC address of the device. @newpage
 */
extern int v2x_radio_tx_event_sock_create_and_bind(const char *interface,
        int v2x_id,
        int event_portnum,
        struct sockaddr_in6 *event_sock_addr,
        int *sock);

/**
    Requests a channel utilization (CBP/CBR) measurement result on a
    channel.

    @datatypes
    #v2x_radio_handle_t \n
    #v2x_chan_meas_params_t

    @param[in] handle            Handle to the port.
    @param[in] measure_this_way  Indicates how and what to measure, and how
                                 often to send results. \n
                                 @vertspace{3}
                                 Some higher-level
                                 standards (like J2945/1 and ETSI TS102687 DCC)
                                 have specific time windows and items to
                                 measure.

    @detdesc
    This function uses the callbacks passed in during initialization to
    deliver the measurements. Measurement callbacks continue until the Radio
    interface is closed.

    @return
    #V2X_STATUS_SUCCESS - The radio is now ready for data-plane sockets to be
    created and bound.
    @par
    #V2X_STATUS_FAIL - CBR measurement is not supported yet.

    @dependencies
    The interface must be pre-initialized with v2x_radio_init() or v2x_radio_init_v2(). The handle
    from that function must be used as the parameter in this function. @newpage
 */
extern v2x_status_enum_type v2x_radio_start_measurements(v2x_radio_handle_t handle,
        v2x_chan_meas_params_t *measure_this_way);

/**
    Discontinues any periodic MAC/Phy channel measurements and the reporting of
    them via listener calls.

    @datatypes
    #v2x_radio_handle_t

    @param[in] handle  Handle to the radio measurements to be stopped.

    @return
    #V2X_STATUS_SUCCESS.

    @dependencies
    The measurements must have been started with
    v2x_radio_start_measurements(). @newpage
 */
extern v2x_status_enum_type v2x_radio_stop_measurements(v2x_radio_handle_t handle);
/** @} *//* end_addtogroup telematics_cv2x_c_radio */

/** @ingroup v2x_deprecated_radio
    Obsolete. This API is not supported and will be removed in the future.

    Queries for the current V2X Rx or Tx status.

    @return
    Indication of success or failure (see #v2x_status_enum_type).
 */
v2x_event_t v2x_radio_get_status(void);

/** @addtogroup telematics_cv2x_c_radio
@{ */
/**
    Closes a specified socket file descriptor and deregisters any modem
    resources associated with it (such as reserved SPS bandwidth contracts).

    On platforms with access control enabled, the caller needs to have TELUX_CV2X_FLOW_OPS
    permission to successfully invoke this API.

    @param[in] sock_fd  Socket file descriptor.

    @detdesc
    This function works on receive, SPS, or event-driven sockets.
    @par
    The socket file descriptor must be closed when the client exits. We
    recommend using a trap to catch controlled shutdowns.

    @return
    Integer value of the close(sock) operation.

    @dependencies
    The socket must have been opened with one of the following methods:
    - v2x_radio_rx_sock_create_and_bind()
    - v2x_radio_tx_sps_sock_create_and_bind()
    - v2x_radio_tx_sps_sock_create_and_bind_v2()
    - v2x_radio_tx_sps_only_create()
    - v2x_radio_tx_sps_only_create_v2()
    - v2x_radio_tx_event_sock_create_and_bind()
    - v2x_radio_tx_event_sock_create_and_bind_v2()
    - v2x_radio_tx_event_sock_create_and_bind_v3() @newpage
 */
extern int v2x_radio_sock_close(int *sock_fd);

/**
    Configures the V2X log level and destination for SDK and lower layers.

    @param[in] new_level   Log level to set to one of the standard syslog
                           levels (LOG_ERR, LOG_INFO, and so on).
    @param[in] use_syslog  Destination: send to stdout (0) or syslog
                           (otherwise).

    @return
    None.
 */
extern void v2x_radio_set_log_level(int new_level, int use_syslog);

/**
    Polls for the recent V2X status.

    @param[out] status_age_useconds  Pointer to the age in microseconds of the
                                     last event (radio status) that is being
                                     reported.

    @detdesc
    This function does not generate any modem control traffic. For efficiency,
    it simply returns the most recently cached value that was reported from the
    modem (often reported at a high rate or frequent rate from the modem).

    @return
    Indication of success or failure (see #v2x_status_enum_type). @newpage
 */
extern v2x_event_t cv2x_status_poll(uint64_t *status_age_useconds);

/**
    Triggers the modem to change its source L2 address by randomly generating
    a new address.

    On platforms with access control enabled, the caller needs to have TELUX_CV2X_CONFIG
    permission to successfully invoke this API.

    @datatypes
    #v2x_radio_handle_t

    @param[in] handle  Initialized Radio interface on which this data
                       connection is made.

    @detdesc
    When the change is complete, clients are notified of the new L2 address
    via the #v2x_radio_calls_t::v2x_radio_l2_addr_changed_listener() callback
    function.

    @return
    0 - On success.
    @par
    Otherwise:
     - EPERM - Socket creation failed; for more details, check errno.h.
     - EAFNOSUPPORT - On failure to find the interface.
     - EACCES - On failure to get the MAC address of the device. @newpage
 */
extern int v2x_radio_trigger_l2_update(
    v2x_radio_handle_t handle);

/**
    Updates the list of malicious and trusted IDs tracked by the modem.

    On platforms with access control enabled, the caller needs to have TELUX_CV2X_CONFIG
    permission to successfully invoke this API.

    @datatypes
    #trusted_ue_info_t

    @param[in] malicious_list_len  Number of malicious IDs in malicious_list.
    @param[in] malicious_list      List of malicious IDs.
    @param[in] trusted_list_len    Number of trusted IDs in trusted_list.
    @param[in] trusted_list        List of trusted IDs.

    @return
    0 - On success.
    @par
    Otherwise:
     - EPERM - Socket creation failed; for more details, check errno.h.
     - EAFNOSUPPORT - On failure to find the interface.
     - EACCES - On failure to get the MAC address of the device. @newpage
 */
int v2x_radio_update_trusted_ue_list(unsigned int malicious_list_len,
                                     unsigned int malicious_list[MAX_MALICIOUS_IDS_LIST_LEN],
                                     unsigned int trusted_list_len,
                                     trusted_ue_info_t trusted_list[MAX_TRUSTED_IDS_LIST_LEN]);

/**
    Creates and binds a socket with a bandwidth-reserved (SPS) Tx flow with the
    requested ID, priority, periodicity, and size on a specified source port
    number. The socket is created as an IPv6 UDP socket.

    This %v2x_radio_tx_sps_sock_create_and_bind_v2() method differs from
    v2x_radio_tx_sps_sock_create_and_bind() in that you can use the
    sps_flow_info parameter to specify transmission resource information about
    the Tx flow.

    On platforms with access control enabled, the caller needs to have TELUX_CV2X_FLOW_OPS
    permission to successfully invoke this API.

    @datatypes
    #v2x_radio_handle_t \n
    #v2x_tx_sps_flow_info_t \n
    #v2x_per_sps_reservation_calls_t

    @param[in]  handle          Identifies the initialized Radio interface on
                                which this data connection is made.
    @param[in]  sps_flow_info   Pointer to the flow information in the
                                v2x_tx_sps_flow_info_t structure.
    @param[in]  calls           Pointer to reservation callbacks or listeners.
                                \n @vertspace{3}
                                This parameter is called when underlying radio
                                MAC parameters change related to the SPS
                                bandwidth contract.
                                For example, the callback after a
                                reservation change, or if the timing offset of
                                the SPS adjusts itself in response to
                                traffic. \n @vertspace{3}
                                This parameter passes NULL if no callbacks are
                                required.
    @param[in]  sps_portnum     Requested source port number for the bandwidth
                                reserved SPS transmissions.
    @param[in]  event_portnum   Requested source port number for the bandwidth
                                reserved event transmissions, or  -1 for no
                                event port.
    @param[out] sps_sock        Pointer to the socket that is bound to the
                                requested port for Tx with reserved bandwidth.
    @param[out] sps_sockaddr    Pointer to the IPv6 UDP socket. \n
                                @vertspace{3}
                                The sockaddr_in6 buffer is initialized with the
                                IPv6 source address and source port that are
                                used for the bind() function.
                                The caller can then use the buffer for
                                subsequent sendto() function calls.
    @param[out] event_sock      Pointer to the socket that is bound to the
                                event-driven transmission port.
    @param[out] event_sockaddr  Pointer to the IPV6 UDP socket. \n
                                @vertspace{3}
                                The sockaddr_in6 buffer is initialized with the
                                IPv6 source address and source port that are
                                used for the bind() function.
                                The caller can then use the buffer for
                                subsequent sendto() function calls.

    @newpage
    @detdesc
    The radio attempts to reserve the flow with the specified size and rate
    passed in the request parameters.
    @par
    This function is used only for Tx. It sets up two UDP sockets on the
    requested two HLOS port numbers.
    @par
    For only a single SPS flow, indicate the event port number by using a
    negative number or NULL for the event_sockaddr. For a single event-driven
    port, use v2x_radio_tx_event_sock_create_and_bind() or
    v2x_radio_tx_event_sock_create_and_bind_v2() or
    v2x_radio_tx_event_sock_create_and_bind_v3() instead.
    @par
    Because the modem endpoint requires a specific global address, all data
    sent on these sockets must have a configurable IPv6 destination address for
    the non-IP traffic.
    @par
    The Priority parameter of the SPS reservation is used only for the reserved
    Tx bandwidth (SPS) flow. The non-SPS/event-driven data sent to the event_portnum
    parameter is prioritized on the air, based on the IPv67 Traffic Class of the packet.
    @par
    The caller is expected to identify two unused local port numbers to use for
    binding: one for the event-driven flow and one for the SPS flow.
    @par
    This call is a blocking call. When it returns, the sockets are ready to
    use, assuming there is no error.

    @return
    0 - On success.
    @par
    Otherwise:
     - EPERM - Socket creation failed; for more details, check errno.h.
     - EAFNOSUPPORT - On failure to find the interface.
     - EACCES - On failure to get the MAC address of the device.

    @dependencies
    The interface must be pre-initialized with v2x_radio_init() or v2x_radio_init_v2(). The handle
    from that function must be used as the parameter in this function. @newpage
*/
extern int v2x_radio_tx_sps_sock_create_and_bind_v2(
    v2x_radio_handle_t handle,
    v2x_tx_sps_flow_info_t *sps_flow_info,
    v2x_per_sps_reservation_calls_t *calls,
    int sps_portnum,
    int event_portnum,
    int *sps_sock,
    struct sockaddr_in6 *sps_sockaddr,
    int *event_sock,
    struct sockaddr_in6 *event_sockaddr);

/**
    Creates a socket with a bandwidth-reserved (SPS) Tx flow.

    Only SPS transmissions are to be implemented for the socket, which is
    created as an IPv6 UDP socket.

    This %v2x_radio_tx_sps_only_create_v2() method differs from
    v2x_radio_tx_sps_only_create() in that you can use the sps_flow_info
    parameter to specify transmission resource information about the Tx flow.

    On platforms with access control enabled, the caller needs to have TELUX_CV2X_FLOW_OPS
    permission to successfully invoke this API.

    @datatypes
    #v2x_radio_handle_t \n
    #v2x_tx_sps_flow_info_t \n
    #v2x_per_sps_reservation_calls_t

    @param[in]  handle          Identifies the initialized Radio interface on
                                which this data connection is made.
    @param[in]  sps_flow_info   Pointer to the flow information in the
                                v2x_tx_sps_flow_info_t structure.
    @param[in]  calls           Pointer to reservation callbacks or listeners.
                                \n @vertspace{3}
                                This parameter is called when underlying radio
                                MAC parameters change related to the SPS
                                bandwidth contract.
                                For example, the callback after a
                                reservation change, or if the timing offset of
                                the SPS adjusts itself in response to
                                traffic. \n @vertspace{3}
                                This parameter passes NULL if no callbacks are
                                required.
    @param[in]  sps_portnum     Requested source port number for the bandwidth
                                reserved SPS transmissions.
    @param[out] sps_sock        Pointer to the socket that is bound to the
                                requested port for Tx with reserved bandwidth.
    @param[out] sps_sockaddr    Pointer to the IPv6 UDP socket. \n
                                @vertspace{3}
                                The sockaddr_in6 buffer is initialized with the
                                IPv6 source address and source port that are
                                used for the bind() function.
                                The caller can then use the buffer for
                                subsequent sendto() function calls.

    @detdesc
    The radio attempts to reserve the flow with the specified size and rate
    passed in the request parameters.
    @par
    This function is used only for Tx. It sets up a UDP socket on the
    requested HLOS port number.
    Because the modem endpoint requires a specific global address, all data
    sent on the socket must have a configurable IPv6 destination address for
    the non-IP traffic.
    @par
    The caller is expected to identify an unused local port number to use for
    binding the SPS flow.
    @par
    This call is a blocking call. When it returns, the socket is ready to
    use, assuming there is no error. @newpage

    @return
    0 - On success.
    @par
    Otherwise:
     - EPERM - Socket creation failed; for more details, check errno.h.
     - EINVAL - On failure to find the interface or get bad parameters.

    @dependencies
    The interface must be pre-initialized with v2x_radio_init() or v2x_radio_init_v2(). The handle
    from that function must be used as the parameter in this function. @newpage
*/
int v2x_radio_tx_sps_only_create_v2(v2x_radio_handle_t handle,
    v2x_tx_sps_flow_info_t *sps_flow_info,
    v2x_per_sps_reservation_calls_t *calls,
    int sps_portnum,
    int *sps_sock,
    struct sockaddr_in6 *sps_sockaddr);

/**
    Adjusts the reservation for transmit bandwidth.

    This %v2x_radio_tx_reservation_change_v2() method differs from
    v2x_radio_tx_reservation_change() in that you can use the updated_flow_info
    parameter to specify transmission resource information about the Tx flow.

    On platforms with access control enabled, the caller needs to have TELUX_CV2X_FLOW_OPS
    permission to successfully invoke this API.

    @datatypes
    v2x_tx_sps_flow_info_t

    @param[out] sps_sock           Pointer to the socket bound to the requested
                                   port.
    @param[in]  updated_flow_info  Pointer to the new reservation information.

    @detdesc
    This function can be used as follows:
    - When the bandwidth requirement changes in periodicity (for example, due
      to an application layer DCC algorithm)
    - Because the packet size is increasing (for example, due to a growing path
      history size in a BSM).
    - When user wants to update reservation priority.
    @par
    When the reservation change is complete, a callback to the structure is
    passed in a v2x_radio_init() or v2x_radio_init_v2() call.

    @return
    #V2X_STATUS_SUCCESS.
    @par
    Error code - On failure (see #v2x_status_enum_type).

    @dependencies
    An SPS flow must have been successfully initialized with
    v2x_radio_tx_sps_sock_create_and_bind() or
    v2x_radio_tx_sps_sock_create_and_bind_v2(). @newpage
 */
extern v2x_status_enum_type v2x_radio_tx_reservation_change_v2(
    int *sps_sock,
    v2x_tx_sps_flow_info_t *updated_flow_info);
/** @} *//* end_addtogroup telematics_cv2x_c_radio */


/** @ingroup v2x_deprecated_radio
    Deprecated. Pass #traffic_ip_type_t on flow creation.

    Opens and binds an event-driven socket (one with no bandwidth reservation).
    The socket is bound as an AF_INET6 UDP type socket.

    This %v2x_radio_tx_event_sock_create_and_bind_v2() method differs from
    v2x_radio_tx_event_sock_create_and_bind() in that you can use the
    event_flow_info parameter to specify transmission resource information
    about the Tx flow.

    On platforms with access control enabled, the caller needs to have TELUX_CV2X_FLOW_OPS
    permission to successfully invoke this API.

    @datatypes
    v2x_tx_flow_info_t

    @param[in]  interface        Pointer to the operating system name to use.
                                 This interface is an RmNet interface (HLOS).
    @param[in]  v2x_id           Used for transmissions that are ultimately
                                 mapped to an L2 destination address.
    @param[in]  event_portnum    Local port number to which the socket is
                                 bound. Used for transmissions of this ID.
    @param[in]  event_flow_info  Pointer to the event flow parameters.
    @param[out] event_sock_addr  Pointer to the sockaddr_ll structure buffer
                                 to be initialized.
    @param[out] sock             Pointer to the file descriptor. Loaded when
                                 the function is successful.

    @detdesc
    This function is used only for Tx when no periodicity is available for the
    application type. If you know your transmit data periodicity, use
    v2x_radio_tx_sps_sock_create_and_bind() or
    v2x_radio_tx_sps_sock_create_and_bind_v2() instead.
    @par
    These event-driven sockets pay attention to QoS parameters in the IP
    socket.

    @return
    0 - On success.
    @par
    Otherwise:
     - EPERM - Socket creation failed; for more details, check errno.h.
     - EAFNOSUPPORT - On failure to find the interface.
     - EACCES - On failure to get the MAC address of the device. @newpage
 */
extern int v2x_radio_tx_event_sock_create_and_bind_v2(
    const char *interface,
    int v2x_id,
    int event_portnum,
    v2x_tx_flow_info_t *event_flow_info,
    struct sockaddr_in6 *event_sock_addr,
    int *sock);

/** @addtogroup telematics_cv2x_c_radio
@{ */
/**
    Adjusts the flow parameters for an existing Tx event socket.

    On platforms with access control enabled, the caller needs to have TELUX_CV2X_CONFIG
    permission to successfully invoke this API.

    @datatypes
    #v2x_tx_flow_info_t

    @param[out] sock               Pointer to the socket bound to the
                                   requested port.
    @param[in]  updated_flow_info  Pointer to the new flow parameters.

    @detdesc
    When the reservation change is complete, a callback to the structure is
    passed in a v2x_radio_init() or v2x_radio_init_v2() call.
    @par
    This call is a blocking call. When it returns, the socket is ready to be
    use, assuming there is no error.

    @return
    #V2X_STATUS_SUCCESS.
    @par
    Error code - On failure (see #v2x_status_enum_type).

    @dependencies
    An event flow must have been successfully initialized with
    v2x_radio_tx_event_sock_create_and_bind() or
    v2x_radio_tx_event_sock_create_and_bind_v2()
    v2x_radio_tx_event_sock_create_and_bind_v3(). @newpage
 */
extern v2x_status_enum_type v2x_radio_tx_event_flow_info_change(
    int *sock,
    v2x_tx_flow_info_t *updated_flow_info);

/**
    Starts V2X mode.

    The V2X radio status must be INACTIVE. If the status is ACTIVE or
    SUSPENDED (see #v2x_event_t), call stop_v2x_mode() first.

    This call is a blocking call. When it returns, V2X mode has been started,
    assuming there is no error.

    On platforms with access control enabled, the caller needs to have TELUX_CV2X_OPS
    permission to successfully invoke this API.

    @return
    #V2X_STATUS_SUCCESS.
    @par
    Otherwise:
    - #V2X_STATUS_EALREADY - Failure because V2X mode is already started.
    - #V2X_STATUS_FAIL - Other failure.

    @dependencies
    V2X radio status must be #V2X_INACTIVE (#v2x_event_t).
 */
extern v2x_status_enum_type start_v2x_mode();

/**
    Stops V2X mode.

    The V2X radio status must be ACTIVE or SUSPENDED (see #v2x_event_t).
    If the status is INACTIVE, call start_v2x_mode() first.

    This call is a blocking call. When it returns, V2X mode has been stopped,
    assuming there is no error.

    On platforms with access control enabled, the caller needs to have TELUX_CV2X_OPS
    permission to successfully invoke this API.

    @return
    #V2X_STATUS_SUCCESS.
    @par
    Otherwise:
    - #V2X_STATUS_EALREADY - Failure because V2X mode is already stopped.
    - #V2X_STATUS_FAIL - Other failure.

    @dependencies
    V2X radio status must be #V2X_ACTIVE, #V2X_TX_SUSPENDED, #V2X_RX_SUSPENDED,
    or #V2X_TXRX_SUSPENDED. @newpage
 */
extern v2x_status_enum_type stop_v2x_mode();

/** @ingroup v2x_deprecated_radio
    @deprecated This API has been deprecated. Please use @ref v2x_radio_init_v3()
    instead.

    Initializes the Radio interface and sets the callback that will be used
    when events in the radio change (including when radio initialization is
    complete).

    @datatypes
    #traffic_ip_type_t \n
    #v2x_concurrency_sel_t \n
    #v2x_radio_calls_t

    @param[in] ip_type         The Ip or non-Ip interface.
    @param[in] mode            WAN concurrency mode, although the radio might
                               not support concurrency. Errors can be generated.
    @param[in] callbacks_p     Pointer to the v2x_radio_calls_t structure that
                               is prepopulated with function pointers used
                               during radio events (such as loss of time
                               synchronization or accuracy) for subscribers. \n
                               @vertspace{3}
                               This parameter also points to a callback for
                               this initialization function.
    @param[in] ctx_p           Voluntary pointer to the first parameter on the
                               callback.

    @detdesc
    This function call is a blocking, and it is a control plane action.
    @par
    Use v2x_radio_deinit() when radio operations are complete.
    @par
    Callback @ref v2x_radio_calls_t::v2x_radio_init_complete()is made when
    initialization is complete.

    @return
    Handle to the specified initialized radio. The handle is used for
    reconfiguring, opening or changing, and closing reservations.
    @par
    #V2X_RADIO_HANDLE_BAD -- If there is an error. No initialization callback
    is made. @newpage
 */
v2x_radio_handle_t v2x_radio_init_v2(traffic_ip_type_t ip_type,
                                     v2x_concurrency_sel_t mode,
                                     v2x_radio_calls_t *callbacks_p,
                                     void *ctx_p);

/**
    Initializes Cv2x radio and sets the callback that will be used when events
    in the radio change (including when radio initialization is complete).
    The callers can get the handles of Cv2x IP and non-IP interface on success.
    The handle of interface is used for reconfiguring, opening or changing,
    and closing reservations.

    @datatypes
    #v2x_concurrency_sel_t \n
    #v2x_radio_calls_t

    @param[in] mode              WAN concurrency mode, although the radio might
                                 not support concurrency. Errors can be generated.
    @param[in] callbacks_p       Pointer to the v2x_radio_calls_t structure that
                                 is prepopulated with function pointers used
                                 during radio events (such as loss of time
                                 synchronization or accuracy) for subscribers. \n
                                 @vertspace{3}
                                 This parameter also points to a callback for
                                 this initialization function.
    @param[in] ctx_p             Voluntary pointer to the first parameter on the
                                 callback.
    @param[out] ip_handle_p      Pointer to the handle of IP interface. Pass nullptr
                                 if IP interface is not used.
    @param[out] non_ip_handle_p  Pointer to the handle of non-IP interface. Pass nullptr
                                 if non-IP interface is not used.

    @detdesc
    This function call is a blocking, and it is a control plane action.
    @par
    Use v2x_radio_deinit() with either IP or non-IP handle when radio operations are complete.
    @par
    Callback @ref v2x_radio_calls_t::v2x_radio_init_complete()is made when
    initialization is complete.

    @return
    0 - On success.
    @par
    Otherwise:
     - EINVAL - Invalid input parmaters.
     - EPERM - Radio initialization failed.

    @dependencies
    This API might fail if the underlying Cv2x status is currently in an inactive state.
    Use @ref v2x_register_ext_radio_status_listener to register a listener for CV2X overall
    Tx/Rx status, then use @ref v2x_get_ext_radio_status to get current V2X overall radio status.
 */
int v2x_radio_init_v3(v2x_concurrency_sel_t mode,
                      v2x_radio_calls_t *callbacks_p,
                      void *ctx_p,
                      v2x_radio_handle_t *ip_handle_p,
                      v2x_radio_handle_t *non_ip_handle_p);

/**
    Opens and binds an event-driven socket (one with no bandwidth reservation).
    The socket is bound as an AF_INET6 UDP type socket.

    On platforms with access control enabled, the caller needs to have TELUX_CV2X_FLOW_OPS
    permission to successfully invoke this API.

    This %v2x_radio_tx_event_sock_create_and_bind_v3() method differs from
    v2x_radio_tx_event_sock_create_and_bind_v2() in that you can use the traffic_ip_type_t
    parameter to specify traffic ip type instead of requiring the interface name.

    @datatypes
    v2x_tx_flow_info_t
    traffic_ip_type_t

    @param[in]  ip_type          traffice_ip_type.
    @param[in]  v2x_id           Used for transmissions that are ultimately
                                 mapped to an L2 destination address.
    @param[in]  event_portnum    Local port number to which the socket is
                                 bound. Used for transmissions of this ID.
    @param[in]  event_flow_info  Pointer to the event flow parameters.
    @param[out] event_sockaddr   Pointer to the sockaddr_ll structure buffer
                                 to be initialized.
    @param[out] sock             Pointer to the file descriptor. Loaded when
                                 the function is successful.

    @detdesc
    This function is used only for Tx when no periodicity is available for the
    application type. If you know your transmit data periodicity, use
    v2x_radio_tx_sps_sock_create_and_bind() or
    v2x_radio_tx_sps_sock_create_and_bind_v2() instead.
    @par
    These event-driven sockets pay attention to QoS parameters in the IP
    socket.

    @return
    0 - On success.
    @par
    Otherwise:
     - EPERM - Socket creation failed; for more details, check errno.h.
     - EAFNOSUPPORT - On failure to find the interface.
     - EACCES - On failure to get the MAC address of the device.
 */
int v2x_radio_tx_event_sock_create_and_bind_v3(
        traffic_ip_type_t ip_type,
        int v2x_id,
        int event_portnum,
        v2x_tx_flow_info_t *event_flow_info,
        struct sockaddr_in6 *event_sockaddr,
        int *sock);

/**
    Returns interface name set during radio initialization.

    @datatypes
    traffic_ip_type_t

    @param[in]  ip_type     traffic_ip_type_t
    @param[out] iface_name  pointer to buffer for interface name
    @param[in]  buffer_len  length of the buffer passed for interface name.
 *                          Must be at least the max buffer size for an interface name (IFNAMSIZE).
    @detdesc
    This function should only be called after successfully initializing a radio.

    @return
    #V2X_STATUS_SUCCESS.
    @par
    #V2X_STATUS_FAIL - If there is an error. Interface name will be an
    empty string. @newpage
 */
v2x_status_enum_type get_iface_name(traffic_ip_type_t ip_type, char * iface_name, size_t buffer_len);

/**
    Creates a TCP socket for event Tx and Rx. The socket is bound as an AF_INET6 TCP
    type socket.

    This %v2x_radio_tcp_sock_create_and_bind() API creates a new TCP socket and binds
    the socket to the IPv6 address of local IP interface with specified source port.
    Additionally, this API also registers a Tx event flow and subscribes Rx with specified
    service ID to enable TCP control and data packets in both transmitting and receiving
    directions.

    On platforms with access control enabled, the caller needs to have TELUX_CV2X_FLOW_OPS
    permission to successfully invoke this API.

    @par
    If the created socket is expected to work as TCP client mode, the caller must establish
    a connection to the address specifed using function connect(), and then use the socket
    for send() and recv() on successful connection. The caller must release the created
    socket and associated resources with v2x_radio_sock_close().
    @par
    If the created socket is expected to work as TCP server mode, the caller must mark the
    created socket as a listening socket with function listen(), that is, as a socket that
    will be used to accept incoming connection requests using accept(). The caller can then
    use the connected socket returned by accept() for send() and recv(). The caller must close
    all connected sockets returned by accept() with function close() first, and then release
    the listening socket and associated resources with v2x_radio_sock_close().
    @par
    This call is a blocking call. When it returns, the created TCP socket is ready to
    use, assuming there is no error.

    @datatypes
    #v2x_radio_handle_t

    @param[in]  handle           Identifies the initialized Radio interface. The caller
                                 must specify IP interface for raido initilization.
    @param[in]  event_info       Pointer to the Tx event flow information.
    @param[in]  sock_info        Pointer to the TCP socket information. \n @vertspace{3}
    @param[out] sock_fd          Pointer to the socket that, on success, returns the TCP socket
                                 descriptor. \n @vertspace{3}
                                 The caller must release this socket with v2x_radio_sock_close().
    @param[out] sockaddr         Pointer to the address of TCP socket. The sockaddr_in6 buffer is
                                 initialized with the IPv6 source address and source port that
                                 are used for the bind.

    @detdesc
    You can execute any sockopts that are appropriate for this type of socket
    (AF_INET6).
    @par
    @return
    0 - On success.
    @par
    Otherwise:
     - EINVAL - On failure to find the interface or get bad parameters.
     - EPERM - Socket operation failed; for more details, check errno.h.

    @dependencies
    The interface used for IP communication must be pre-initialized with
    v2x_radio_init(). The handle from that function must be used as the
    parameter in this function. @newpage
 */
extern int v2x_radio_tcp_sock_create_and_bind(
    v2x_radio_handle_t handle,
    const v2x_tx_flow_info_t *event_info,
    const socket_info_t *sock_info,
    int *sock_fd,
    struct sockaddr_in6 *sockaddr);

/**
    Set RF peak cv2x transmit power.
    This affects the power for all existing flows and for any flow created int the future.

    On platforms with access control enabled, the caller needs to have TELUX_CV2X_CONFIG
    permission to successfully invoke this API.

    Precondition -- v2x mode enabled.

    @param [in] txPower   Desired global Cv2x peak tx power in dBm,
                          The value should be in the range from -40 to 31.
                          CV2X modem will set the TX power to -40dBm or 31dBm respectively
                          if the value provided is less than -40 or bigger than 31.

    @returns V2X_STATUS_SUCCESS on success. Error status otherwise.
 */
v2x_status_enum_type v2x_set_peak_tx_power(int8_t txPower);

/**
    Set src L2 ID list for filtering.
    This affects/disables receiving packets from the src L2 IDs in the list.

    On platforms with access control enabled, the caller needs to have TELUX_CV2X_CONFIG
    permission to successfully invoke this API.

    @param [in] list_len   number of rc L2 IDs, max value 50
    @param [in] list_array array that stores the src L2 IDs, durations and pppp values for filter

    @returns V2X_STATUS_SUCCESS on success. Error status otherwise.
 */
v2x_status_enum_type v2x_set_l2_filters(uint32_t list_len, src_l2_filter_info* list_array);

/**
    Remove specific src L2 ID list for filtering.
    This affects/enables receiving packets from the src L2 IDs in the list.

    On platforms with access control enabled, the caller needs to have TELUX_CV2X_CONFIG
    permission to successfully invoke this API.

    @param [in] list_len   number of rc L2 IDs, max value 50
    @param [in] l2_id_list array that stores the src L2 IDs

    @returns V2X_STATUS_SUCCESS on success. Error status otherwise.
 */
v2x_status_enum_type v2x_remove_l2_filters(uint32_t list_len, uint32_t* l2_id_list);

/**
    Registers a listener for CV2X Tx status report.

    @datatypes
    v2x_tx_status_report_listener

    @param[in] port            Set this value to the port number of registered Tx Flow
                               if user wants to receive Tx status report associated with
                               its own Tx flow. If user wants to receive Tx status report
                               associated with all Tx flows in system, set this value to 0.
                               @vertspace{3}

    @param[in] callback        Callback function of @ref v2x_tx_status_report_listener
                               structure that is called on Tx status reports. \n
                               @vertspace{3}

    @return
    #V2X_STATUS_SUCCESS.
    @par
    #V2X_STATUS_FAIL -- If there is an error.

    @dependencies
    CV2X radio must be pre-initialized with @ref v2x_radio_init_v2() or v2x_radio_init_v3().
 */
v2x_status_enum_type v2x_register_tx_status_report_listener(
    uint16_t port,
    v2x_tx_status_report_listener callback);

/**
    Deregisters a listener for CV2X Tx status report.

    @datatypes
    v2x_tx_status_report_listener

    @param[in] port            Port number of previously registered
                               @ref v2x_tx_status_report_listener that is to be deregistered.
                               If the listener is registered with port number 0,
                               set this value to 0 to deregister the listener.\n@vertspace{3}

    @detdesc
    User will not receive Tx status reports after the deregistration.

    @return
    #V2X_STATUS_SUCCESS.
    @par
    #V2X_STATUS_FAIL -- If there is an error.

    @dependencies
    CV2X radio must be pre-initialized with @ref v2x_radio_init_v2() or v2x_radio_init_v3().
    @newpage
 */
v2x_status_enum_type v2x_deregister_tx_status_report_listener(uint16_t port);

/**
    Set CV2X global IP address for the IP interface.

    On platforms with access control enabled, the caller needs to have TELUX_CV2X_CONFIG
    permission to successfully invoke this API.

    @param [in] prefix_len CV2X global IP address prefix length in bits, range [64, 128]
    @param [in] ipv6_addr  CV2X global IP address.

    @returns V2X_STATUS_SUCCESS on success. Error status otherwise.
 */
v2x_status_enum_type v2x_set_global_IPaddr(uint8_t prefix_len, uint8_t* ipv6_addr);

/**
    Set CV2X IP interface global IP unicast routing information.

    @param [in] dest_mac_addr CV2X destination L2 address for unicast routing purpose.
                              expecting a 6 bytes array address, in which the L2 addr stored in
                              the last 3 entries in big endian order.

    @returns V2X_STATUS_SUCCESS on success. Error status otherwise.
 */
v2x_status_enum_type v2x_set_ip_routing_info(uint8_t* dest_mac_addr);

/**
    Get current V2X overall radio status and per pool status.

    @param [out] status        Pointer to structure v2x_radio_status_ex_t, which
                               contains V2X overall radio status and per pool status
                               on success.

    @returns V2X_STATUS_SUCCESS on success. Error status otherwise.
 */
v2x_status_enum_type v2x_get_ext_radio_status(v2x_radio_status_ex_t* status);

/**
    Registers a listener for CV2X overall Tx/Rx status and per pool status.

    @datatypes
    v2x_ext_radio_status_listener

    @param[in] callback        Callback function of @ref v2x_ext_radio_status_listener
                               structure that is called on CV2X Tx/Rx status change. \n
                               @vertspace{3}

    @return
    #V2X_STATUS_SUCCESS.
    @par
    #V2X_STATUS_FAIL -- If there is an error.
 */
v2x_status_enum_type v2x_register_ext_radio_status_listener(
    v2x_ext_radio_status_listener callback);

/**
    Get the current V2X SLSS Rx information.

    @param [out] slss_info     Pointer to structure v2x_slss_rx_info_t, which
                               contains V2X SLSS Rx information on success.

    @returns V2X_STATUS_SUCCESS on success. Error status otherwise.
 */
v2x_status_enum_type v2x_get_slss_rx_info(v2x_slss_rx_info_t* slss_info);

/**
    Registers a listener for CV2X SLSS Rx information.

    @datatypes
    v2x_slss_rx_info_listener

    @param[in] callback        Callback function of @ref v2x_slss_rx_listener
                               structure that is called on SLSS Rx information change.\n

    @return
    #V2X_STATUS_SUCCESS.
    @par
    #V2X_STATUS_FAIL -- If there is an error.
 */
v2x_status_enum_type v2x_register_slss_rx_listener(v2x_slss_rx_listener callback);

/**
    Deregisters a listener for CV2X SLSS Rx information.

    @datatypes
    v2x_slss_rx_info_listener

    @param[in] callback        Previously registered @ref v2x_slss_rx_listener that
                               is to be deregistered.\n

    @return
    #V2X_STATUS_SUCCESS.
    @par
    #V2X_STATUS_FAIL -- If there is an error.
 */
v2x_status_enum_type v2x_deregister_slss_rx_listener(v2x_slss_rx_listener callback);

/**
    Inject coarse UTC time when UE is synchronized to SLSS.

    GNSS fix is not available when UE is synchronized to SLSS. To get
    accurate UTC time in this case, user can register a listener by
    invoking @ref v2x_register_utc_from_slss and then inject coarse UTC
    time derrived from received application messages using this API.
    The age of injected UTC time could be nearly 10 seconds at most.
    After that, accurate UTC time will be notified to user periodically
    through the registered listener.

    On platforms with access control enabled, the caller needs to have
    TELUX_CV2X_CONFIG permission to successfully invoke this API.

    @param [in] utc     UTC time since Jan. 1, 1970. Units: Milliseconds.

    @returns V2X_STATUS_SUCCESS on success. Error status otherwise.
 */
v2x_status_enum_type v2x_inject_coarse_utc_time(uint64_t utc);

/**
    Inject vehicle speed, which will be used to select radio resources for C-V2X
    transmission when GNSS is not available.

    User could inject speed at any time, the last injected value will be used when
    GNSS is not available.

    The strategy for speed injection could be
      - inject periodically, recommended interval bigger than 1 second
      - inject when speed is crossing 60/80/100/120/140/160/180/200 kmph

    On platforms with access control enabled, the caller needs to have TELUX_CV2X_CONFIG
    permission to successfully invoke this API.

    @param [in] speed  - Vehicle speed in kmph.
    @param [in] cb     - Callback that is invoked when speed injection is done.
    @returns V2X_STATUS_SUCCESS if no error occurred.
 */
v2x_status_enum_type v2x_inject_vehicle_speed(uint32_t speed);

/** @} *//* end_addtogroup telematics_cv2x_c_radio */

/*
 * Testing functions mainly for sim environment
 * but also useful for IPV6 testing
 */
extern void v2x_set_dest_ipv6_addr(char *new_addr);
extern void v2x_set_dest_port(uint16_t portnum);
extern void v2x_set_rx_port(uint16_t portnum);
void v2x_disable_socket_connect();

#ifdef __cplusplus
}
#endif

#endif // TELUX_CV2X_LEGACY_V2X_RADIO_API_H
