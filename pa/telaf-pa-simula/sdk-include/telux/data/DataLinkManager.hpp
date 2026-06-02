/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file       DataLinkManager.hpp
 *
 * @brief      Data Link Manager class provides the interface to data communication links.
 */


#ifndef TELUX_DATA_DATALINKMANAGER_HPP
#define TELUX_DATA_DATALINKMANAGER_HPP


#include <memory>
#include <telux/data/DataDefines.hpp>

namespace telux {
namespace data {

/** @addtogroup telematics_data
 * @{ */

// Forward declarations
class IDataLinkListener;

/**
 * Specifies the media-independent interface (MII) variant and data rate.
 */
enum EthModeType {
   /** Unknown */
   ETHMODE_UNKNOWN = 0,
   /** USXGMII 10G data rate */
   ETHMODE_USXGMII_10G = (1 << 0),
   /** USXGMII 5G data rate */
   ETHMODE_USXGMII_5G = (1 << 1),
   /** USXGMII 2.5G data rate */
   ETHMODE_USXGMII_2_5G = (1 << 2),
   /** USXGMII 1G data rate */
   ETHMODE_USXGMII_1G = (1 << 3),
   /** USXGMII 100M data rate */
   ETHMODE_USXGMII_100M = (1 << 4),
   /** USXGMII 10M data rate */
   ETHMODE_USXGMII_10M = (1 << 5),
   /** SGMII 2.5G data rate */
   ETHMODE_SGMII_2_5G = (1 << 6),
   /** SGMII 1G data rate */
   ETHMODE_SGMII_1G = (1 << 7),
   /** SGMII 100M data rate */
   ETHMODE_SGMII_100M = (1 << 8),
};

/**
 * Bitmask containing EthModeType bits,
 * e.g., a value of 0x3 represents that USXGMII 10G and 5G are supported.
 */
using EthModes = uint32_t;

/**
 * Link mode update request status.
 */
enum class LinkModeChangeStatus {
    UNKNOWN         = 0,        /**< Unknown status */
    ACCEPTED        = 1,        /**< Request accepted */
    COMPLETED       = 2,        /**< Successfully completed */
    FAILED          = 3,        /**< Request failed */
    REJECTED        = 4,        /**< Request rejected */
    TIMEOUT         = 5         /**< Timed-out */
};

/** Provides Ethernet link capability */
struct EthCapability {
    EthModes ethModes;       /**< Bitmask containing EthModeType bits */
};


/**
 * @brief The Data Link Manager class provides APIs related to data communication links,
 *        for example, APIs to update the Ethernet link operating mode.
 *
 * Under certain scenarios, like thermal mitigation, the local ETH module needs to change the link
 * operating mode, e.g., downgrade it from EthModeType::USXGMII_10G to EthModeType::SGMII_1G. The
 * ETH module requires that the client communicate and coordinate with the remote end of the link
 * (called the peer) to transition to the new mode. The sequence in which the transition is
 * initiated and completed is illustrated below:
 * 1. On bootup, the client needs to set the capability of the peer using @ref setPeerEthCapability.
 *    This allows the local ETH module to transition to a mode that is supported by the peer.
 * 2. When a certain condition is met, like a thermal threshold being crossed, the local ETH module
 *    notifies clients about its request to transition to a new mode using
 *    @ref onEthModeChangeRequest.
 * 3. On receiving this request, the client is expected to:
 *    - Interact with the peer and request transition to the new mode
 *    - Use an interconnect other than Ethernet (ETH) to convey the new mode information to the
 *      peer, since the Ethernet link will be down after the @ref onEthModeChangeRequest
 *    - Indicate to the local ETH module the intent of the peer by calling
 *      @ref setPeerModeChangeRequestStatus.
 * 4. Client gets the status of the transaction to change the mode, via
 *    @ref onEthModeChangeTransactionStatus
 *
 * If this code is running as part of an ECU (other end of ETH link) which needs to react to new ETH
 * operating mode requests from NAD, then @ref setLocalEthOperatingMode needs to be called.
 *
 * For hypervisor-based platforms, IDataLinkManager is supported only in the primary/host VM.
 */
class IDataLinkManager {
public:
    /**
     * Checks the status of Data Link manager object and returns the result.
     *
     * @returns SERVICE_AVAILABLE    -  If Data Link manager object is ready for service.
     *          SERVICE_UNAVAILABLE  -  If Data Link manager object is temporarily unavailable.
     *          SERVICE_FAILED       -  If Data Link manager object encountered an irrecoverable
     *                                  failure.
     *
     */
    virtual telux::common::ServiceStatus getServiceStatus() = 0;

    /**
     * Gets local ethernet link capability and provides the supported ethernet data rates and
     * respective operating mode (MII variant) considered for thermal mitigation.
     *
     * On platforms with access control enabled, the caller needs to have TELUX_DATA_LINK_INFO
     * permission to successfully invoke this API.
     *
     * @param [out] ethCapability       Info with respect to ETH link capability
     *
     * @return Status of getEthCapability, i.e., success or applicable status code
     */
    virtual telux::common::Status getEthCapability(EthCapability &ethCapability) = 0;

    /**
     * Informs the NAD about the modes supported by the ECU on the other end of the Ethernet link
     * (peer). For instance, when the temperature of the NAD exceeds certain thresholds, the NAD
     * downgrades the mode of the ETH Link to a low mode supported by the other end.
     *
     * This info is not persistent over device reboot or sub-system restart (SSR) updated via
     * @ref IDataLinkListener::onServiceStatusChange()
     *
     * On platforms with access control enabled, the caller needs to have
     * TELUX_DATA_LINK_CONFIG permission to successfully invoke this API.
     *
     * @param [in] ethCapability       Bitmask containing EthModeType bits
     *
     * @return Status of setPeerEthCapability, i.e., success or applicable status code
     */
    virtual telux::common::Status setPeerEthCapability(EthCapability ethCapability) = 0;

    /**
     * Sets the local ethernet link operating mode.
     * This API can be used to change the local device ethernet data rate and operating mode,
     * for example, when the remote end of the Ethernet link requires a modified mode due to a
     * thermal threshold being crossed this API should be used.
     *
     * On platforms with access control enabled, the caller needs to have TELUX_DATA_LINK_CONFIG
     * permission to successfully invoke this API.
     *
     * @param [in] ethModeType          Provides the suggested ethernet speed and operating mode
     *                                  (MII variant)
     * @param [in] callback             Optional callback to get the response for
     *                                  setLocalEthOperatingMode.
     *
     * @return Status of setLocalEthOperatingMode, i.e., success or applicable status code
     */
    virtual telux::common::Status setLocalEthOperatingMode(
        EthModeType ethModeType, telux::common::ResponseCallback callback = nullptr) = 0;

    /**
     * Allows the client to provide the acknowledgement status from the remote end to the NAD that
     * made the link operating mode update request.
     *
     * The new ethernet operating mode will be suggested via
     * @ref IDataLinkListener::onEthModeChangeRequest in scenarios like when the temperature crosses
     * the expected limit. This suggestion needs to be accepted and processed by the other end of
     * ethernet connection. To complete the transition this API needs to be called with proper
     * @ref LinkModeChangeStatus.
     *
     * This API should be called when:
     * 1. A request is accepted by the remote end @ref LinkModeChangeStatus.
     * 2. The remote end successfully changes there ethernet data rate and operating mode.
     *
     * Any failure response interrupts updating the ETH mode, which was triggered via
     * @ref IDataLinkListener::onEthModeChangeRequest. In failure cases, the client needs to wait
     * for the new ETH mode update request @ref IDataLinkListener::onEthModeChangeRequest or, if
     * needed, the user can use @ref setLocalEthOperatingMode.
     *
     * On platforms with access control enabled, the caller needs to have TELUX_DATA_LINK_CONFIG
     * permission to successfully invoke this API.
     *
     * @param status             Current status of ETH mode change request
     * @return Status of setPeerModeChangeRequestStatus, i.e., success or applicable status code
     */
    virtual telux::common::Status setPeerModeChangeRequestStatus(LinkModeChangeStatus status) = 0;

    /**
     * Bring up or bring down the ethernet data link for MAC-to-MAC connections on the eth0
     * interface.
     *
     * The Ethernet data link can be brought up by the client once the peer entity is ready to
     * establish the ethernet data connection. To prevent packet loss, it's recommended to set the
     * ethernet data link state to UP after peer entity initialization.
     *
     * Clients are notified about ethernet link state changes by using
     * @ref IDataLinkListener::onEthDataLinkStateChange
     *
     * On platforms with access control enabled, the caller needs to have the TELUX_DATA_LINK_CONFIG
     * permission to successfully invoke this API.
     *
     * @param [in] ethLinkState               ethernet link state info.
     *
     * @return ErrorCode of setEthDataLinkState, i.e., OPERATION_NOT_ALLOWED/SUCCESS or applicable
     *                                              error code
     */
    virtual telux::common::ErrorCode setEthDataLinkState(LinkState ethLinkState) = 0;

    /**
     * Retrieves the current state of the ethernet data link for MAC-to-MAC connections on the eth0
     * interface.
     *
     * This API allows the clients to query the current state of the ethernet data link,
     * which can be either UP, DOWN or UNKNOWN. The state of the ethernet data link is
     * determined by the platform and is influenced by various factors such as
     * network connectivity, peer availability, and system configuration.
     *
     * On platforms with access control enabled, the caller needs to have the
     * TELUX_DATA_LINK_INFO permission to successfully invoke this API.
     *
     * @param [out] ethLinkState     current state of the ethernet data link.
     *
     * @return ErrorCode of getEthDataLinkState, i.e., OPERATION_NOT_ALLOWED/SUCCESS or applicable
     *                                              error code
     */
    virtual telux::common::ErrorCode getEthDataLinkState(LinkState &ethLinkState) = 0;

    /**
     * Registers with the Data Link Manager as a listener for service statuses and other events.
     *
     * @param [in] listener    Pointer to the IDataLinkListener object that processes the
     *                         notification
     *
     * @returns Status of registerListener success or suitable status code
     *
     */
    virtual telux::common::Status registerListener(std::weak_ptr<IDataLinkListener> listener) = 0;

    /**
     * Removes a previously added listener.
     *
     * @param [in] listener    Pointer to the IDataLinkListener object that needs to be removed
     *
     * @returns Status of deregisterListener success or suitable status code
     *
     */
    virtual telux::common::Status deregisterListener(std::weak_ptr<IDataLinkListener> listener) = 0;
};

/**
 * Interface for the Data Link listener object. Client needs to implement this interface to be
 * notified of data link service notifications like onServiceStatusChange.
 *
 * The listener methods can be invoked from multiple threads. The implementation should be thread
 * safe.
 */
class IDataLinkListener : public telux::common::ISDKListener {
 public:
    /**
     * Called when the service status changes.
     *
     * @param [in] status - @ref ServiceStatus
     */
    virtual void onServiceStatusChange(telux::common::ServiceStatus status) {}

    /**
     * Requests a change in ethernet speed and operating mode (MII variant).
     *
     * This is invoked by the platform, possibly due to thermal mitigation. This API is invoked to
     * request that the client helps coordinate a change in the ethernet speed and operating mode.
     * When this API is called, the client is expected to inform the peer about this request and get
     * an acknowledgement whether the peer is moving to the requested mode
     * @ref setPeerModeChangeRequestStatus.
     *
     * Since the ETH link will be down after the @ref onEthModeChangeRequest is invoked, the client
     * should use an interconnect other than ETH to convey the new mode information to the peer and
     * obtain acknowledgment. Once the peer has successfully transitioned to the desired mode and
     * updated the local ETH module, successful acknowledgment via
     * @ref setPeerModeChangeRequestStatus will make the ETH link usable.
     *
     * On platforms with access control enabled, the caller needs to have TELUX_DATA_LINK_CONFIG
     * permission to successfully invoke this API.
     *
     * @param [in] ethModeType          Provides the suggested ethernet speed and operating mode
     *                                  (MII variant)
     */
    virtual void onEthModeChangeRequest(EthModeType ethModeType) {}

    /**
     * Informs about ethernet speed and operating mode (MII variant) status changes.
     *
     * In case of a timeout or failure reported via this API, communication over ETH link would not
     * be possible. A new ETH mode change request will be initiated via @ref onEthModeChangeRequest
     * when the temperature crosses the expected limit. However, to complete the transition to the
     * new ETH mode, a successful acknowledgment within a certain time via
     * @ref setPeerModeChangeRequestStatus is required.
     *
     * On platforms with access control enabled, the caller needs to have TELUX_DATA_LINK_INFO
     * permission to successfully invoke this API.
     *
     * @param [in] EthModeType          Provides the updated ethernet speed and operating mode
     *                                  (MII variant)
     * @param [in] status               Current status of ETH mode change request
     */
    virtual void onEthModeChangeTransactionStatus(EthModeType ethModeType,
        LinkModeChangeStatus status) {}

    /**
     * Notifies clients about ethernet data link state changes.
     *
     * On platforms with access control enabled, the caller needs to have the TELUX_DATA_LINK_INFO
     * permission to receive this event.
     *
     * @param [in] ethLinkState          current ethernet link state
     */
    virtual void onEthDataLinkStateChange(LinkState ethLinkState) {}

    /**
     * Destructor for IDataLinkListener
     */
    virtual ~IDataLinkListener(){};
};

/** @} */ /* end_addtogroup telematics_data */
}
}

#endif  //TELUX_DATA_DATALINKMANAGER_HPP
