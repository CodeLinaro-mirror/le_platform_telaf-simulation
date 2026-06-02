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
* @file       Cv2xRadio.hpp
*
* @brief      Cv2xRadio class encapsulates a CV2X radio (channel) resource.
*/

#ifndef TELUX_CV2X_CV2XRADIO_HPP
#define TELUX_CV2X_CV2XRADIO_HPP


#include <telux/cv2x/Cv2xFactory.hpp>
#include <telux/cv2x/Cv2xRadioManager.hpp>
#include <telux/cv2x/Cv2xRadioListener.hpp>
#include <telux/cv2x/Cv2xRxSubscription.hpp>
#include <telux/cv2x/Cv2xTxFlow.hpp>
#include <telux/cv2x/Cv2xTxRxSocket.hpp>
#include <telux/cv2x/Cv2xTxStatusReportListener.hpp>

#include <future>
#include <memory>

#include <telux/common/CommonDefines.hpp>

namespace telux {
namespace cv2x {

class ICv2xRadioListener;
class ICv2xRxSubscription;
class ICv2xTxFlow;
class ICv2xTxRxSocket;


/**
 * This function is called as a response to @ref ICv2xRadio::createRxSubscription.
 *
 * @param [in] rxSub       - Rx Subscription
 * @param [in] error       - Indicates whether socket creation succeeded
 *                         - SUCCESS
 *                         - GENERIC_FAILURE
 */
using CreateRxSubscriptionCallback =
    std::function<void (std::shared_ptr<ICv2xRxSubscription> rxSub,
                        telux::common::ErrorCode error)>;

/**
 * This function is called as a  response to @ref ICv2xRadio::createTxSpsFlow
 *
 * @param [in] txSpsFlow      - Sps flow
 * @param [in] txEventFlow    - Optional event flow. Will be nullptr if event flow was
 *                              not specified in the request
 * @param [in] spsError       - Indicates whether Tx SPS flow creation succeeded
 *                            - SUCCESS
 *                            - GENERIC_FAILURE
 * @param [in] eventError     - Indicates whether optional Tx Event flow creation succeeded
 *                            - SUCCESS
 *                            - GENERIC_FAILURE
 *
 */
using CreateTxSpsFlowCallback = std::function<void (std::shared_ptr<ICv2xTxFlow> txSpsFlow,
                                                    std::shared_ptr<ICv2xTxFlow> txEventFlow,
                                                    telux::common::ErrorCode spsError,
                                                    telux::common::ErrorCode eventError)>;

/**
 * This function is called with the response to @ref ICv2xRadio::createTxEventFlow
 *
 * @param [in] txEventFlow    - Event flow
 * @param [in] error          - Indicates whether Tx event flow creation succeeded
 *                            - SUCCESS
 *                            - GENERIC_FAILURE
 */
using CreateTxEventFlowCallback = std::function<void (std::shared_ptr<ICv2xTxFlow> txEventFlow,
                                                      telux::common::ErrorCode error)>;

/**
 * This function is called with the response to @ref ICv2xRadio::closeTxFlow.
 *
 * @param [in] txFlow    - Closed tx flow
 * @param [in] error     - Indicates whether close operation succeeded
 *                       - SUCCESS
 *                       - GENERIC_FAILURE
 */
using CloseTxFlowCallback = std::function<void (std::shared_ptr<ICv2xTxFlow> txFlow,
                                                telux::common::ErrorCode error)>;

/**
 * This function is called with the response to @ref ICv2xRadio::closeRxSubscription.
 *
 * @param [in] rxSub     - Closed rx subscription
 * @param [in] error     - Indicates whether Rx subscription close succeeded
 *                       - SUCCESS
 *                       - GENERIC_FAILURE
 */
using CloseRxSubscriptionCallback =
    std::function<void (std::shared_ptr<ICv2xRxSubscription> rxSub,
                        telux::common::ErrorCode error)>;


/**
 * This function is called with the response to @ref ICv2xRadio::changeSpsFlowInfo.
 *
 * @param [in] txFlow    - Sps flow that requested reservation change
 * @param [in] error     - SUCCESS if Tx reservation change succeeded
 *                       - SUCCESS
 *                       - GENERIC_FAILURE
 */
using ChangeSpsFlowInfoCallback =
    std::function<void (std::shared_ptr<ICv2xTxFlow> txFlow, telux::common::ErrorCode error)>;

/**
 * This function is called with the response to @ref ICv2xRadio::requestSpsFlowInfo.
 *
 * @param [in] txFlow      - SPS flow that requested info
 * @param [in] spsInfo     - SPS flow reservation info
 * @param [in] error       - SUCCESS if Tx reservation change succeeded
 *                         - SUCCESS
 *                         - GENERIC_FAILURE
 */
using RequestSpsFlowInfoCallback =
    std::function<void (std::shared_ptr<ICv2xTxFlow> txFlow,
                        const SpsFlowInfo & spsInfo,
                        telux::common::ErrorCode error)>;
/**
 * This function is called with the response to @ref ICv2xRadio::changeEventFlowInfo.
 *
 * @param [in] txFlow    - Event flow that requested reservation change
 * @param [in] error     - SUCCESS if Tx parameter change succeeded
 *                       - SUCCESS
 *                       - GENERIC_FAILURE
 */
using ChangeEventFlowInfoCallback =
    std::function<void (std::shared_ptr<ICv2xTxFlow> txFlow, telux::common::ErrorCode error)>;

/**
 * This function is called with the response to @ref ICv2xRadio::requestCapabilities.
 *
 * @param [in] capabilities     - Capability info
 * @param [in] error       - SUCCESS if capabilities request succeeded
 *                         - SUCCESS
 *                         - GENERIC_FAILURE
 */
using RequestCapabilitiesCallback =
    std::function<void(const Cv2xRadioCapabilities & capabilities,
                       telux::common::ErrorCode error)>;

/**
 * This function is called with the response to @ref ICv2xRadio::requestDataSessionSettings.
 *
 * @param [in] settings     - Data session settings
 * @param [in] error       - SUCCESS if data session settings request succeeded
 *                         - SUCCESS
 *                         - GENERIC_FAILURE
 */
using RequestDataSessionSettingsCallback =
    std::function<void (const DataSessionSettings & settings,
                        telux::common::ErrorCode error)>;
/**
 * This function is called with the response to @ref ICv2xRadio::updateTrustedUEList.
 *
 * @param [in] error       - SUCCESS if update succeeded
 *                         - INVALID_ARGUMENTS if trustedUEs or maliciousIds
 *                           length greater than maximum value
 *                         - SUCCESS
 *                         - GENERIC_FAILURE
 *                         - INVALID_ARGUMENTS
 */
using UpdateTrustedUEListCallback =
    std::function<void(telux::common::ErrorCode error)>;

/**
 * This function is called with the response to ICv2xRadio::updateSrcL2Info.
 *
 * @param [in] error       - SUCCESS if Tx reservation change succeeded
 *                         - SUCCESS
 *                         - GENERIC_FAILURE
 */
using UpdateSrcL2InfoCallback =
    std::function<void (telux::common::ErrorCode error)>;

/**
 * This function is called with the response to @ref ICv2xRadio::createCv2xTcpSocket.
 *
 * @param [in] sock      - TCP socket
 * @param [in] error     - Indicates whether TCP socket creation succeeded
 *                       - SUCCESS
 *                       - GENERIC_FAILUREs
 */
using CreateTcpSocketCallback =
    std::function<void (std::shared_ptr<ICv2xTxRxSocket> sock,
                        telux::common::ErrorCode error)>;

/**
 * This function is called with the response to @ref ICv2xRadio::closeCv2xTcpSocket.
 *
 * @param [in] sock      - Closed TCP socket
 * @param [in] error     - Indicates whether close operation succeeded
 *                       - SUCCESS
 *                       - GENERIC_FAILURE
 */
using CloseTcpSocketCallback = std::function<void (std::shared_ptr<ICv2xTxRxSocket> sock,
                                             telux::common::ErrorCode error)>;

/** @addtogroup telematics_cv2x_cpp
 * @{ */

/**
 * This is class encapsulates a Cv2xRadio interface.
 *
 * Returned from @ref ICv2xRadioManager::getCv2xRadio
 */
class ICv2xRadio {
public:

    /**
     * Returns true if the radio interface has completed initialization.
     *
     * @returns True if initialized. False otherwise.
     */
    virtual bool isInitialized() const = 0;

    /**
     * Registers a listener for this Cv2xRadio.
     *
     * @param [in] listener - Listener that implements Cv2xRadioListener
     *                        interface.
     */
    virtual telux::common::Status registerListener(
        std::weak_ptr<ICv2xRadioListener> listener) = 0;

    /**
     * Deregisters a listener from this Cv2xRadio.
     *
     * @param [in] listener - Previously registered Cv2xRadioListener that is
     *                        to be deregistered.
     */
    virtual telux::common::Status deregisterListener(
        std::weak_ptr<ICv2xRadioListener> listener) = 0;

    /**
     * Creates and initializes a new Rx subscription which will be returned
     * in the user-supplied callback.
     *
     * @param [in] ipType  - IP traffic type (IP or NON-IP)
     * @param [in] port    - Rx port number
     * @param [in] cb      - Callback function that is invoked when socket
     *                       creation is complete.
     * @param [in] idList  - Service ID list to subscribe, optional parameter
     *                       using nullptr by default. Subscribe wildcard if
     *                       this parameter is set to nullptr.
     *
     * On platforms with access control enabled, the caller needs to have TELUX_CV2X_FLOW_OPS
     * permission to successfully invoke this API.
     *
     * @returns SUCCESS on success. Error status otherwise.
     *
     * @dependencies The interface must be pre-initialized with init().
     *
     */
    virtual telux::common::Status createRxSubscription(
        TrafficIpType ipType,
        uint16_t port,
        CreateRxSubscriptionCallback cb,
        std::shared_ptr<std::vector<uint32_t>> idList = nullptr) = 0;

    /**
     * This status indicates whether the Cv2xRadio is in a usable state.
     *
     * @returns SERVICE_AVAILABLE    -  If cv2x radio is ready for service.
     *          SERVICE_UNAVAILABLE  -  If cv2x radio is temporarily unavailable.
     *          SERVICE_FAILED       -  If cv2x radio encountered an irrecoverable failure.
     *
     */
    virtual telux::common::ServiceStatus getServiceStatus() = 0;

    /**
     * Enable or disable (depends on the parameter "bool enable") the received
     * packets' meta data report for the service IDs provided.
     *
     * The meta data consist of RF RSSI (received signal strength indicator) status, 32-bit SCI
     * Format 1 (3GPP TS 36.213, section 14.1), packet delay estimation, L2 destination ID,
     * and the resource blocks used for the packet's transmission: subframe, subchannel index.
     *
     * On platforms with access control enabled, the caller needs to have TELUX_CV2X_INFO
     * permission to successfully invoke this API.
     *
     *
     * @param [in] ipType  - IP traffic type (IP or NON-IP)
     * @param [in] enable  - enable the rx meta data if set to true, otherwise disable
     * @param [in] idList  - Service ID list of which the received packets' report are desired
     * @param [in] cb      - Callback that is invoked when meta data is enabled or disabled.
     *
     * @returns SUCCESS if no error occurred
     *
     * @par Meta data report for IP packets is not supported yet, it will return NOSUPPORTED.
     *
     */
    virtual telux::common::Status enableRxMetaDataReport(
        TrafficIpType ipType,
        bool enable,
        std::shared_ptr<std::vector<std::uint32_t>> idList,
        telux::common::ResponseCallback cb) = 0;

    /**
     * Creates a Tx SPS flow with the specified IP type, serviceId, and other
     * parameters specified in reservation. Additionally, an option event flow
     * will be created with the same IP type and serviceId. A Tx socket will
     * be created and initialized for the SPS flow. A Tx socket will be created
     * and initialized for the event flow if the optional event flow is specified.
     *
     * On platforms with access control enabled, the caller needs to have TELUX_CV2X_FLOW_OPS
     * permission to successfully invoke this API.
     *
     * @param [in] ipType            - IP traffic type (IP or NON-IP)
     * @param [in] serviceId         - ID used for transmissions that will be
     *                                 mapped to an L2 destination address.
     *                                 Variable length 4-byte PSID or ITS_AID, or
     *                                 another service ID.
     * @param [in] spsInfo           - SPS reservation parameters.
     * @param [in] spsSrcPort        - Requested source port number for the
     *                                 bandwidth reserved SPS transmissions.
     * @param [in] eventSrcPortValid - True if an optional event flow is desired. If
     *                                 this field is left false, the event flow will
     *                                 not be created.
     * @param [in] eventSrcPort      - Requested source port number for the optional
     *                                 event flow.
     * @param [in] cb                - Callback function that is invoked when socket
     *                                 creation is complete. This must not be null.
     *
     * @par This caller is expected to identify two unused local port numbers
     * to use for binding: one for the event-driven flow and one for the SPS
     * flow.
     *
     * @returns SUCCESS upon success. Error status otherwise.
     */
    virtual telux::common::Status createTxSpsFlow(
        TrafficIpType ipType,
        uint32_t serviceId,
        const SpsFlowInfo & spsInfo,
        uint16_t spsSrcPort,
        bool eventSrcPortValid,
        uint16_t eventSrcPort,
        CreateTxSpsFlowCallback cb) = 0;

    /**
     * Creates an event flow. An associated Tx socket will be created and
     * initialized.
     *
     * On platforms with access control enabled, the caller needs to have TELUX_CV2X_FLOW_OPS
     * permission to successfully invoke this API.
     *
     * @param [in] ipType         - IP traffic type (IP or NON-IP)
     * @param [in] serviceId      - ID used for transmissions that will be
     *                              mapped to an L2 destination address.
     *                              Variable length 4-byte PSID or ITS_AID, or
     *                              another service ID.
     * @param [in] eventSrcPort   - Local port number to which the socket is
     *                              bound. Used for transmissions of this ID.
     * @param [in] cb             - Callback function that is invoked when socket
     *                              creation is complete. This must not be null.
     *
     * @detdesc This function is used only for TX when no periodicity is
     *          available for the application type. If your transmit data
     *          periodicity is known, use createTxSpsFlow() instead.
     *
     * @par These even-driven sockets pay attention to the QoS parameters in
     *      the IP socket.
     *
     * @returns SUCCESS upon success. Error status otherwise.
     */
    virtual telux::common::Status createTxEventFlow(
        TrafficIpType ipType,
        uint32_t serviceId,
        uint16_t eventSrcPort,
        CreateTxEventFlowCallback cb) = 0;

    /**
     * Creates an event flow. An associated Tx socket will be created and
     * initialized.
     *
     * On platforms with access control enabled, the caller needs to have TELUX_CV2X_FLOW_OPS
     * permission to successfully invoke this API.
     *
     * @param [in] ipType         - IP traffic type (IP or NON-IP)
     * @param [in] serviceId      - ID used for transmissions that will be
     *                              mapped to an L2 destination address.
     *                              Variable length 4-byte PSID or ITS_AID, or
     *                              another service ID.
     * @param [in] flowInfo       - Flow configuration parameters
     * @param [in] eventSrcPort   - Local port number to which the socket is
     *                              bound. Used for transmissions of this ID.
     * @param [in] cb             - Callback function that is invoked when socket
     *                              creation is complete. This must not be null.
     *
     * @detdesc This function is used only for TX when no periodicity is
     *          available for the application type. If your transmit data
     *          periodicity is known, use createTxSpsFlow() instead.
     *
     * @par These even-driven sockets pay attention to the QoS parameters in
     *      the IP socket.
     *
     * @returns SUCCESS upon success. Error status otherwise.
     */
    virtual telux::common::Status createTxEventFlow(
        TrafficIpType ipType,
        uint32_t serviceId,
        const EventFlowInfo & flowInfo,
        uint16_t eventSrcPort,
        CreateTxEventFlowCallback cb) = 0;

    /**
     * Closes the RxSubscription and frees resources (such as the Rx socket) associated
     * with it.
     *
     * @param [in] rxSub - RxSubscription to close
     * @param [in] cb    - Callback that is invoked when socket close is complete.
     *                     This may be null.
     *
     * @returns SUCCESS if no error occurred.
     */
    virtual telux::common::Status closeRxSubscription(std::shared_ptr<ICv2xRxSubscription> rxSub,
                                                      CloseRxSubscriptionCallback cb) = 0;

    /**
     * Closes the TxFlow and frees resources associated with it (such as reserved
     * SPS bandwidth contracts and sockets). This function works on both SPS and event
     * flows.
     *
     * On platforms with access control enabled, the caller needs to have TELUX_CV2X_FLOW_OPS
     * permission to successfully invoke this API.
     *
     * @param [in] txFlow  - Tx (SPS or event) flow to close.
     * @param [in] cb      - Callback that is invoked when Tx flow close is complete.
     *                       This may be null.
     *
     * @returns SUCCESS if no error occurred.
     */
    virtual telux::common::Status closeTxFlow(std::shared_ptr<ICv2xTxFlow> txFlow,
                                              CloseTxFlowCallback cb) = 0;

    /**
     * Request to change TX SPS Flow reservation parameters.
     *
     * @param [in] txFlow       - Tx SPS flow
     * @param [in] spsInfo      - Desired SPS reservation parameters
     * @param [in] cb           - Callback that is invoked upon reservation change. This
     *                            may be null.
     *
     * @returns SUCCESS if no error occurred.
     */
    virtual telux::common::Status changeSpsFlowInfo(
        std::shared_ptr<ICv2xTxFlow> txFlow,
        const SpsFlowInfo & spsInfo,
        ChangeSpsFlowInfoCallback cb) = 0;

    /**
     * Request SPS flow info.
     *
     * @param [in] txFlow       - Tx SPS flow
     * @param [in] cb           - Callback that will be invoked and returns the SPS info.
     *                            Must not be null.
     *
     * @returns SUCCESS if no error occurred.
     */
    virtual telux::common::Status requestSpsFlowInfo(
        std::shared_ptr<ICv2xTxFlow> txFlow,
        RequestSpsFlowInfoCallback cb) = 0;

    /**
     * Request to change TX Event Flow reservation parameters.
     *
     * On platforms with access control enabled, the caller needs to have TELUX_CV2X_FLOW_OPS
     * permission to successfully invoke this API.
     *
     * @param [in] txFlow       - Tx Event flow
     * @param [in] flowInfo     - Desired Event flow parameters
     * @param [in] cb           - Callback that is invoked upon parameter
     *                            change. This may be null.
     *
     * @returns SUCCESS if no error occurred.
     */
    virtual telux::common::Status changeEventFlowInfo(
        std::shared_ptr<ICv2xTxFlow> txFlow,
        const EventFlowInfo & flowInfo,
        ChangeEventFlowInfoCallback cb) = 0;

    /**
     * Request modem Cv2x capability information.
     *
     * @param [in] cb           - Callback that will be invoked and returns the
     *                            capability info. Must not be null.
     *
     * @returns SUCCESS if no error occurred.
     */
    virtual telux::common::Status requestCapabilities(
        RequestCapabilitiesCallback cb) = 0;

    /**
     * Request data session settings currently in use.
     *
     * @param [in] cb           - Callback that will be invoked and returns the
     *                            data session settings. Must not be null.
     *
     * @returns SUCCESS if no error occurred.
     */
    virtual telux::common::Status requestDataSessionSettings(
        RequestDataSessionSettingsCallback cb) = 0;

    /**
     * Requests modem to change L2 info.
     *
     * On platforms with access control enabled, the caller needs to have TELUX_CV2X_CONFIG
     * permission to successfully invoke this API.
     *
     * @param  [in] cb          - Callback that will be invoked and returns status.
     *                            Must not be null.
     *
     * @returns SUCCESS if no error occurred.
     */
    virtual telux::common::Status updateSrcL2Info(UpdateSrcL2InfoCallback cb) = 0;

    /**
     * Send request to modem to update the list of malicious UE source IDs and
     * trusted UE source IDs with corresponding confidence information.
     *
     * On platforms with access control enabled, the caller needs to have TELUX_CV2X_CONFIG
     * permission to invoke this API successf
     *
     * @param  [in] infoList    - Trusted and malicious UE information list
     * @param  [in] cb          - Callback that will be invoked and returns status.
     *                            Must not be null.
     *
     * @returns SUCCESS if no error occurred.
     */
    virtual telux::common::Status updateTrustedUEList(
        const TrustedUEInfoList & infoList,
        UpdateTrustedUEListCallback cb) = 0;

    /**
     * Destructor for ICv2xRadio
     */
    virtual ~ICv2xRadio(){}

    /**
     * Get interface name based on ipType.
     *
     * @param ipType          - IP traffic type (IP or NON-IP)
     *
     * @returns Interface name as a string
     *
     */
    virtual std::string getIfaceNameFromIpType(TrafficIpType ipType) = 0;

    /**
     * Creates a CV2X TCP socket with specified event flow information and TCP socket
     * information. The TCP socket will be created and bound to the IPv6 address of local
     * IP interface with specifed source port. Additionally, this API also registers a Tx
     * event flow and subscribes Rx with specified service ID. If the created socket is
     * expected to work as TCP client mode, the caller must connect the created socket to
     * a destination using connect() and then use the socket for send() and recv() on
     * successful connection. If the created socket is expected to work as TCP server mode,
     * the caller must mark this socket as a listening socket using listen() and accept
     * connections received from this listening socket using accept(), and then use the
     * accepted sockets returned from accept() for send() or recv().
     *
     * On platforms with access control enabled, the caller needs to have TELUX_CV2X_FLOW_OPS
     * permission to successfully invoke this API.
     *
     * @param [in] eventInfo    - Information for the Event flow.
     * @param [in] sockInfo     - Information for the TCP socket.
     * @param [in] cb           - Callback function that is invoked when socket
     *                            creation is complete. This must not be null.
     *
     * @par The caller is expected to identify an unused local port number as the source
     * port number in structure @ref SocketInfo to use for binding.
     * @par The caller must release the created socket and associated resources with
     * @ref closeCv2xTcpSocket. Additionally, if the created socket is marked as a listening
     * socket, the caller must close all the accepted sockets returned by accept() using
     * close() first, and then release the listening socket and associated resources by
     * calling @ref closeCv2xTcpSocket.
     * @returns SUCCESS upon success. Error status otherwise.
     */
    virtual telux::common::Status createCv2xTcpSocket(
        const EventFlowInfo &eventInfo,
        const SocketInfo &sockInfo,
        CreateTcpSocketCallback cb) = 0;


    /**
     * Closes the CV2X TCP socket and frees resources associated with it (such as
     * registered event Tx flow and subscribed Rx service ID and created TCP socket).
     *
     * On platforms with access control enabled, the caller needs to have TELUX_CV2X_FLOW_OPS
     * permission to successfully invoke this API.
     *
     * @param [in] sock   - CV2X TCP socket to close.
     * @param [in] cb     - Callback that is invoked when CV2X TCP socket close is complete.
     *                      This may be null.
     *
     * @returns SUCCESS if no error occurred.
     */
    virtual telux::common::Status closeCv2xTcpSocket(std::shared_ptr<ICv2xTxRxSocket> sock,
                                                     CloseTcpSocketCallback cb) = 0;

    /**
     * Registers a listener for Tx status report.
     *
     * @param [in] port     - Set this value to the port number of registered Tx Flow
     *                        if user wants to receive Tx status report associated with
     *                        its own Tx flow. If user wants to receive Tx status report
     *                        associated with all Tx flows in system, set this value to 0.
     * @param [in] listener - Listener that implements ICv2xTxStatusReportListener
     *                        interface.
     * @param [in] cb       - Callback that is invoked when the registration of CV2X Tx
     *                        status report is complete.
     */
    virtual telux::common::Status registerTxStatusReportListener(
        uint16_t port,
        std::shared_ptr<ICv2xTxStatusReportListener> listener,
        telux::common::ResponseCallback cb) = 0;

    /**
     * Deregisters a listener for Tx status report.
     *
     * @param [in] port     - Port number of previously registered ICv2xTxStatusReportListener
     *                        that is to be deregistered. If the listener is registered with
     *                        port number 0, set this value to 0 to deregister the listener.
     * @param [in] cb       - Callback that is invoked when the deregistration of CV2X Tx
     *                        status report is complete.
     */
    virtual telux::common::Status deregisterTxStatusReportListener(
        uint16_t port,
        telux::common::ResponseCallback cb) = 0;

    /**
     * Set CV2X global IP address for the IP interface.
     *
     * Use case and Precondition:
     * OBU:
     * Registers a TX/RX *NON IP* flow for receiving the signed WSA/WRA for IP session initiation;
     * Once receives the IP prefix in the WDS/WRA from RSU, call this method.
     *
     * RSU:
     * Specifies its own global prefix via this method, and creates/composes WSA/WRA
     * advertising the IP configs.
     *
     * On platforms with access control enabled, the caller needs to have TELUX_CV2X_CONFIG
     * permission to successfully invoke this API.
     *
     * @param [in] ipv6Addr   - CV2X global IP address.
     * @param [in] cb     - Callback that is invoked when set the global IP address complete.
     *                      This may be null.
     *
     * @returns SUCCESS if no error occurred.
     */
    virtual telux::common::Status setGlobalIPInfo(const IPv6AddrType &ipv6Addr,
        common::ResponseCallback cb) = 0;

    /**
     * Set CV2X IP interface global IP unicast routing information.
     *
     * Use case and Precondition:
     * OBU:
     * Registers a TX/RX *NON IP* flow for receiving the signed WSA/WRA for IP session initiation;
     * Once receives the IP prefix in the WSA/WRA from RSU, call the @ref setGlobalIPInfo method
     * to update the ip interface with global IP;
     * Now call this method to set the routing information with dest L2 addr negotiated in WSA/WRA.
     *
     * RSU:
     * Specifies its own global prefix via @ref setGlobalIPInfo, and creates/composes WSA/WRA
     * advertising the IP configs;
     * Now set routing information of its own via this method.
     *
     * On platforms with access control enabled, the caller needs to have TELUX_CV2X_CONFIG
     * permission to successfully invoke this API.
     *
     * @param [in] destL2Addr   - CV2X destination L2 address for unicast routing purpose.
     * @param [in] cb     - Callback that is invoked when set global IP unicast routing
     * information complete. This may be null.
     *
     * @returns SUCCESS if no error occurred.
     */
    virtual telux::common::Status setGlobalIPUnicastRoutingInfo(
        const GlobalIPUnicastRoutingInfo &destL2Addr, common::ResponseCallback cb) = 0;

    /**
     * Get the capabilities of this Cv2xRadio.
     *
     * @returns Cv2xRadioCapabilities - Contains capabilities of this Cv2xRadio.
     *
     * @deprecated Use requestCapabilities() API
     */
    virtual Cv2xRadioCapabilities getCapabilities() const = 0;

    /**
     * Inject vehicle speed, which will be used to select radio resources for C-V2X
     * transmission when GNSS is not available.
     *
     * User could inject speed at any time, the last injected value will be used when
     * GNSS is not available.
     *
     * The strategy for speed injection could be
     *  - inject periodically, recommended interval bigger than 1 second
     *  - inject when speed is crossing 60/80/100/120/140/160/180/200 kmph
     *
     * On platforms with access control enabled, the caller needs to have TELUX_CV2X_CONFIG
     * permission to successfully invoke this API.
     *
     * @param [in] speed  - Vehicle speed in kmph.
     * @param [in] cb     - Callback that is invoked when speed injection is done.
     * @returns SUCCESS if no error occurred.
     *
     */
    virtual telux::common::Status injectVehicleSpeed(
        uint32_t speed, telux::common::ResponseCallback cb) = 0;

    /**
     * Returns true if the radio interface was successfully initialized.
     *
     * @returns True if ready. False otherwise.
     *
     * @deprecated use getServiceStatus instead
     */
    virtual bool isReady() const = 0;

    /**
     * Returns a future that indicated if the radio interface is ready or if
     * radio failed to initialize.
     *
     * @returns SUCCESS if Cv2xRadio initialization was successful. Otherwise it
     *          returns an Error Code.
     *
     * @deprecated the readiness can be notified via the callback passed to
     *          ICv2xRadioManager::getCv2xRadio.
     */
    virtual std::future<telux::common::Status> onReady() = 0;
};

/** @} */ /* end_addtogroup telematics_cv2x_cpp */

} // namespace cv2x

} // namespace telux

#endif // TELUX_CV2X_CV2XRADIO_HPP
