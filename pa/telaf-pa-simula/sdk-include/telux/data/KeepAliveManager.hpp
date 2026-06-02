/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */


/**
 * @file   KeepAliveManager.hpp
 * @brief  KeepAliveManager provides APIs to configure TCP keep-alive offloading.
 *
 *         TCP keep-alive offloading can be used by TCP clients on the AP/EAP to offload
 *         the sending of TCP keep-alive messages to the modem. This allows the client to
 *         keep the TCP connection valid while the AP/EAP is suspended.
 *
 *         TCP keep-alive offloading is supported in two modes: normal mode and monitor mode.
 *
 *         - Normal mode: Requires the client to specify the TCP session parameters
 *           (recvNext, recvWindow, sendNext, sendWindow)
 *         - Monitor mode: Allows the client to configure a packet monitor on the modem to allow
 *           the modem to learn the TCP session parameters used to set up TCP-keepAlive offloading.
 *
 *           The monitor allows the client to use TCP-keepAlive offloading without the need to
 *           retreive the TCP session parameters from the AP/EAP.
 *         @note The supported configuration is the TCP client running within the MDM and the TCP
 *         server operating outside of the MDM.
 */

#ifndef TELUX_KEEPALIVEMANAGER_HPP
#define TELUX_KEEPALIVEMANAGER_HPP

#include <memory>
#include <string>

#include <telux/common/CommonDefines.hpp>
#include <telux/common/SDKListener.hpp>

namespace telux {
namespace data {

/** @addtogroup telematics_data
 * @{ */

struct TCPKAParams {
    std::string srcIp;      /**< Source IPv4/IPv6 address */
    std::string dstIp;      /**< Destination IPv4/IPv6 address */
    int srcPort;            /* Source port */
    int dstPort;            /**< Destination port */
};

struct TCPSessionParams {
    uint32_t recvNext;     /**< Next sequence number expected on the incoming packet. */
    uint32_t recvWindow;   /**< Receive window */
    uint32_t sendNext;     /**< Next sequence number to be sent */
    uint32_t sendWindow;   /**< Send window */
};

using MonitorHandleType = uint32_t;
using TCPKAOffloadHandle = uint32_t;

class IKeepAliveListener;

/**
 * IKeepAliveManager is a primary interface to manage TCP keep-alive offloading.
 *
 * @note Eval: This is a new API and is being evaluated. It is subject to change and
 * could break backwards compatibility.
 */
class IKeepAliveManager {
public:
    /**
     * Checks the status of the TCPKAOffload manager and returns the result.
     *
     * @returns
     * - SERVICE_AVAILABLE: TCP keep-alive offload manager is ready for service.
     * - SERVICE_UNAVAILABLE: TCP keep-alive offload manager is temporarily unavailable.
     * - SERVICE_FAILED: TCP keep-alive offload manager encountered an irrecoverable failure.
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to change and
     * could break backwards compatibility.
     */
    virtual telux::common::ServiceStatus getServiceStatus() = 0;

    /**
     * Starts the TCP monitor for the specified TCP connection.
     *
     * After monitoring is enabled, the modem inspects packets sent/received over the TCP
     * connection to learn the TCP session parameters to use to send TCP keep-alive messages
     * if @ref startTCPKeepAliveOffload API is called for the TCP connection.
     *
     * At least one packet needs to be exchanged between the TCP client and the server after
     * calling this API for the modem to learn the TCP session parameters. The modem needs
     * to learn the TCP session parameters before @ref startTCPKeepAliveOffload is called with
     * MonitorHandle. For example, consider an application on a UE that uses MQTT over TCP to
     * communicate with the cloud. After entering MONITOR mode, the client can either send a
     * dummy PUBLISH message or resend a previous PUBLISH with the same topic. This ensures
     * that the modem receives a TCP packet after MONITOR mode is initiated.
     *
     * On platforms with access control enabled, the caller needs to have
     * the TELUX_DATA_KA_OFFLOAD_OPS permission to successfully invoke this API.
     *
     * @param[in] tcpKaParams   TCP connection information.
     * @param[out] handle       Handle to the TCP monitor used for this connection.
     *
     * @returns Error code indicating if the operation succeeded or not.
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to change and
     * could break backwards compatibility.
     *
     */
    virtual telux::common::ErrorCode enableTCPMonitor(
        const TCPKAParams &tcpKaParams, MonitorHandleType &monHandle) = 0;

    /**
     * Stops the TCP monitor for the specified MonitorHandle.
     *
     * On platforms with access control enabled, the caller needs to have the
     * TELUX_DATA_KA_OFFLOAD_OPS permission to successfully invoke this API.
     *
     * @param[in] handle    Obtained from @ref enableTCPMonitor.
     *
     * @returns Error code indicating if the operation succeeded or not.
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to change and
     * could break backwards compatibility.
     *
     */
    virtual telux::common::ErrorCode disableTCPMonitor(const MonitorHandleType monHandle) = 0;

    /**
     * Starts TCP keep-alive offloading based on the TCP keep-alive offloading parameter`s.
     *
     * This variant of the startTCPKeepAliveOffload API requires the user to specify the
     * TCP connection parameters (source IP, destination IP, source port, destination port)
     * and the TCP session parameters (recvNext, recvWindow, sendNext, sendWindow)
     * associated with the TCP connection.
     *
     * For TCP keep-alive offloading to work, the TCP client or the server shall not send/ack
     * any packets after the TCP session parameters are collected from the TCP/IP stack.
     * When the TCP keep-alive is offloaded to the modem, it does not support any TCP options in
     * the header, i.e., the keep-alive is sent without any options set.
     * Before calling the @ref startTCPKeepAliveOffload API, there should be no in-flight packets
     * i.e., all TCP packets between the server and client must be acknowledged.
     * The TCP client or server shall not send or acknowledge any packets after
     * the @ref startTCPKeepAliveOffload API is called, or else the keep-alive offload to the modem
     * will stop.
     *
     * On platforms with access control enabled, the caller needs to have the
     * TELUX_DATA_KA_OFFLOAD_OPS permission to successfully invoke this API.
     *
     * @param[in] tcpKaParams       TCP connection information required to offload
     *                              sending the keep-alive messages.
     * @param[in] tcpSessionParams  The TCP sliding window parameters.
     * @param[in] interval          Interval between two consecutive keep-alive messages to
     *                              be sent.
     * @param[out] handle           TCP offload handle.
     *
     * @returns Error code indicating if the operation succeeded or not.
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to change and
     * could break backwards compatibility.
     *
     */
    virtual telux::common::ErrorCode startTCPKeepAliveOffload(
        const TCPKAParams &tcpKaParams, const TCPSessionParams &tcpSessionParams,
        const uint32_t interval, TCPKAOffloadHandle &handle) = 0;

    /**
     * Starts TCP keep-alive offloading based on the active TCP monitor.
     *
     * This variant of startTCPKeepAliveOffload API does not require TCP session parameters.
     *
     * This API is to be used with the @ref enableTCPMonitor API. The modem learns
     * the TCP session parameters by monitoring the TCP connection. At least one TCP packet needs
     * to be exchanged between the TCP server and the client after @ref enableTCPMonitor is
     * called and before this variant of the @ref startTCPKeepAliveOffload API is called.
     * Before calling the @ref startTCPKeepAliveOffload API, there should be no in-flight packets
     * i.e., all TCP packets between the server and client must be acknowledged.
     * The TCP client or server shall not send or acknowledge any packets after
     * the @ref startTCPKeepAliveOffload API is called, or else the keep-alive offload to the modem
     * will stop.
     *
     * When the TCP keep-alive is offloaded to the modem, it does not support any TCP options in
     * the header, i.e., the keep-alive is sent without any options set.
     *
     * On platforms with access control enabled, the caller needs to have the
     * TELUX_DATA_KA_OFFLOAD_OPS permission to successfully invoke this API.
     *
     * @param[in] handle    Monitor handle returned by @ref enableTCPMonitor
     *                      API.
     * @param[in] interval  Interval between two consecutive KeepAlive messages to
     *                      be sent.
     * @param[out] handle   TCP offload handle.
     *
     * @returns Error code indicating if the operation succeeded or not.
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to change and
     * could break backwards compatibility.
     *
     */
    virtual telux::common::ErrorCode startTCPKeepAliveOffload(
        const MonitorHandleType monHandle, const uint32_t interval, TCPKAOffloadHandle &handle) = 0;

    /**
     * Stops TCP keep-alive offloading for the specified TCPKAOffloadHandle.
     *
     * When the client sends a stop TCP keep-alive offload request, a positive response
     * indicates that sending keep-alive has stopped. The @ref onKeepAliveStatusChange indication
     * for keep-alive stopped is only called in case of an error.
     *
     * On platforms with access control enabled, the caller needs to have the
     * TELUX_DATA_KA_OFFLOAD_OPS permission to successfully invoke this API.
     *
     * @param[in] handle    TCP offload handle obtained from @ref startTCPKeepAliveOffload.
     *
     * @returns Error code indicating if the operation succeeded or not.
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to change and
     * could break backwards compatibility.
     *
     */
    virtual telux::common::ErrorCode stopTCPKeepAliveOffload(
        const TCPKAOffloadHandle handle) = 0;

    /**
     * Registers with the DataTCPKAOffloadManager as a listener to receive TCP connection
     * offload management related notifications.
     *
     * @param[in] listener  Listener to receive notifications.
     *
     * @returns @ref telux::common::Status::SUCCESS if the listener is registered;
     * otherwise, an appropriate status code.
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to change and
     * could break backwards compatibility.
     */
    virtual telux::common::Status registerListener(
        std::weak_ptr<telux::data::IKeepAliveListener> listener) = 0;

    /**
     * Deregisters the given listener previously registered with @ref registerListener.
     *
     * @param[in] listener  Listener to deregister.
     *
     * @returns @ref telux::common::Status::SUCCESS if the listener is deregistered,
     * otherwise, an appropriate status code.
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to change and
     * could break backwards compatibility.
     */
    virtual telux::common::Status deregisterListener(
        std::weak_ptr<telux::data::IKeepAliveListener> listener) = 0;
};

/**
 * @brief Listener class to get notifications when the modem stops sending
 * TCP keep-alive messages
 *
 * The client shall implement this interface to receive notifications.
 *
 * The listener methods can be invoked from multiple threads and it is
 * the client's responsibility to ensure that the implementation is
 * thread safe.
 *
 */
class IKeepAliveListener : public telux::common::ISDKListener {
public:

    /**
     * Constructor for the IKeepAliveListener class.
     *
     */
    IKeepAliveListener() {}
    /**
     * This function is called in case of the start of a keep-alive message or an error. The modem
     * can stop sending keep-alive messages as a result of network failure.
     *
     * @param[in] error     Possible values are NETWORK_ERR and SUCCESS.
     *
     * @param[in] handle    TCPKA offload handle.
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to change and
     * could break backwards compatibility.
     */
    virtual void onKeepAliveStatusChange(
        telux::common::ErrorCode error, TCPKAOffloadHandle handle) {}

    /**
     * This function is called when service status changes.
     *
     * @param [in] status @ref ServiceStatus
     */
    virtual void onServiceStatusChange(telux::common::ServiceStatus status) {}

    /**
     * Destructor of IKeepAliveListener.
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to change and
     * could break backwards compatibility.
     */
    virtual ~IKeepAliveListener(){};
};

/** @} */ /* end_addtogroup telematics_data */
} // namespace data
} // namespace telux

#endif // TELUX_KEEPALIVEMANAGER_HPP
