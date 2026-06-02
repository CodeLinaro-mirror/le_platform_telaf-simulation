/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
/**
 * @file       NetworkSelectionManager.hpp
 *
 * @brief      Network Selection Manager class provides the interface to get and
 *             set network selection mode (Manual or Automatic), scan available
 *             networks and set and get preferred networks list.
 */

#ifndef TELUX_TEL_NETWORKSELECTIONMANAGER_HPP
#define TELUX_TEL_NETWORKSELECTIONMANAGER_HPP

#include <bitset>
#include <future>
#include <memory>
#include <vector>

#include <telux/common/CommonDefines.hpp>
#include <telux/tel/PhoneDefines.hpp>
#include <telux/tel/ServingSystemDefines.hpp>

namespace telux {
namespace tel {

// Forward declarations
class INetworkSelectionListener;
class OperatorInfo;

/** @addtogroup telematics_network_selection
* @{ */

/**
 * Defines network RAT type for preferred networks. Each value represents
 * corresponding bit for RatMask bitset.
 */
enum RatType {
   UMTS = 15, /**< UMTS */
   LTE = 14,  /**< LTE */
   GSM = 7,   /**< GSM */
   NR5G = 11, /**< NR5G */
};

/** @} */ /* end_addtogroup telematics_network_selection */

/**
 * 16 bit mask that denotes which of the radio access technologies defined in
 * RatType enum are used for preferred networks.
 */
using RatMask = std::bitset<16>;

/** @addtogroup telematics_network_selection
* @{ */

/**
 * Defines the preferred network information
 */
struct PreferredNetworkInfo {
   uint16_t mcc;    /**< mobile country code */
   uint16_t mnc;    /**< mobile network code */
   RatMask ratMask; /**< bit mask denotes which of the radio access technologies are set */
};

/**
 * Defines the status of the network scan results
 */
enum class NetworkScanStatus {
   COMPLETE = 0,         /**<  Network scan is successful and completed. No more indications are
                               expected for the scan request */
   PARTIAL = 1,          /**<  Network scan results are partial, further results are expected in
                               subsequent indication */
   FAILED = 2,           /**<  Network scan failed either due to radio link failure or it is
                               aborted or due to problem in performing incremental search. */
};

/**
 * Defines network selection mode
 */
enum class NetworkSelectionMode {
   UNKNOWN = -1,  /**< Unknown */
   AUTOMATIC = 0, /**< Device registers according to provisioned mcc and mnc */
   MANUAL = 1     /**< Device registers to specified network as per provided mcc and mnc */
};

/**
 * Defines in-use status of network operator
 */
enum class InUseStatus {
   UNKNOWN,         /**< Unknown */
   CURRENT_SERVING, /**< Current serving */
   AVAILABLE        /**< Available */
};

/**
 * Defines roaming status of network operator
 */
enum class RoamingStatus {
   UNKNOWN, /**< Unknown */
   HOME,    /**< Home */
   ROAM     /**< Roaming */
};

/**
 * Defines forbidden status of network operator
 */
enum class ForbiddenStatus {
   UNKNOWN,      /**< Unknown */
   FORBIDDEN,    /**< Forbidden */
   NOT_FORBIDDEN /**< Not forbidden */
};

/**
 * Defines preferred status of network operator
 */
enum class PreferredStatus {
   UNKNOWN,      /**< Unknown */
   PREFERRED,    /**< Preferred */
   NOT_PREFERRED /**< Not preferred */
};

/**
 * Defines status of network operator
 */
struct OperatorStatus {
   InUseStatus inUse = InUseStatus::UNKNOWN;       /**< In-use status of network operator */
   RoamingStatus roaming = RoamingStatus::UNKNOWN; /**< Roaming status of network operator */
   ForbiddenStatus forbidden
      = ForbiddenStatus::UNKNOWN; /**< Forbidden status of network operator */
   PreferredStatus preferred
      = PreferredStatus::UNKNOWN; /**< Preferred status of network operator */
};

/**
 * Defines Network scan type
 */
enum class NetworkScanType {
    CURRENT_RAT_PREFERENCE = 1,      /**< Network scan based on current RAT preference */
    USER_SPECIFIED_RAT,              /**< Network scan based on user specified RAT(s) */
    ALL_RATS                         /**< Network scan on GSM/WCDMA/LTE/NR5G */
};

/**
 * Defines Network scan information
 */
struct NetworkScanInfo {
    NetworkScanType scanType;  /**< Network scan type */
    RatMask ratMask;           /**< Bit mask denotes which of the radio access technologies are set. ratMask is valid/set only when scanType is provided as NetworkScanType::USER_SPECIFIED_RAT */
};

/**
 * Defines network selection mode information.
 */
struct NetworkModeInfo {
    NetworkSelectionMode mode;
    std::string mcc;  /**< Mobile Country Code (Applicable only for MANUAL selection mode). */
    std::string mnc;  /**< Mobile Network Code (Applicable only for MANUAL selection mode). */
};

/**
 * Defines dubious cell cause codes
 */
enum DubiousCellCauseCode {
    DUBIOUS_CELL_CAUSE_CEF = (1 << 0), /**< Connection Establishment Failure */
    DUBIOUS_CELL_CAUSE_RLF = (1 << 1), /**< Radio Link Failure (RLF) due to poor signal, handover
                                            failure, hardware issue etc. */
    DUBIOUS_CELL_CAUSE_PING_PONG = (1 << 2), /**< Device frequently switches between two or more
                                                  cells due to signal fluctuations, device movement
                                                  between overlapping cell coverage area etc. */
    DUBIOUS_CELL_CAUSE_LOW_DATA_RATE_PS = (1 << 3), /**< Cell is experiencing low data rates in the
                                                         packet-switched (PS) domain due to network
                                                         congestion, interference etc. */
    DUBIOUS_CELL_CAUSE_LOW_DATA_RATE_IMS = (1 << 4), /**< Cell is experiencing low data rates in
                                                          the IP Multimedia Subsystem (IMS) domain
                                                          due to network congestion, interference
                                                          etc. */
};

/**
 * Bitmask containing dubious cell cause code bits, e.g., a value of 0x10 represents low data rate
 * in IMS. Multiple cause codes are possible.
 */
using DbCellCauseCodeMask = std::bitset<32>;

/**
 * Defines NR subcarrier spacing type
 */
enum class NrSubcarrierSpacing {
    INVALID = -1,
    SCS_15  = 0, /**< Subcarrier spacing 15kHz */
    SCS_30  = 1, /**< Subcarrier spacing 30kHz */
    SCS_60  = 2, /**< Subcarrier spacing 60kHz */
    SCS_120 = 3, /**< Subcarrier spacing 120kHz */
    SCS_240 = 4, /**< Subcarrier spacing 240kHz */
};

/**
 * Defines dubious cell information
 */
struct DubiousCellInfo {
    std::string mcc;                    /**< Mobile country code */
    std::string mnc;                    /**< Mobile network code */
    unsigned int arfcn;                 /**< Absolute radio-frequency channel number */
    unsigned int pci;                   /**< Physical cell identity */
    RFBand activeBand;                  /**< RF band information for a dubious cell */
    DbCellCauseCodeMask causeCodeMask;  /**< Dubious cell cause code bit mask */
};

/**
 * Defines NR5G dubious cell information
 */
struct NrDubiousCell {
    DubiousCellInfo ci;                      /**< NR dubious cell */
    unsigned long long cgi = 0;              /**< Global cell ID */
    NrSubcarrierSpacing spacing;             /**< NR subcarrier spacing */
};

/**
 * Defines LTE dubious cell information
 */
struct LteDubiousCell {
    DubiousCellInfo ci;                     /**< LTE dubious cell */
    unsigned int cgi = 0;                   /**< Global cell ID */
};

/** @} */ /* end_addtogroup telematics_network_selection */

/**
 * This function is called with the response to requestNetworkSelectionMode API.
 *
 * The callback can be invoked from multiple different threads.
 * The implementation should be thread safe.
 *
 * @param [in] mode       @ref NetworkSelectionMode
 * @param [in] error      Return code which indicates whether the operation
 *                        succeeded or not
 *                        @ref telux::common::ErrorCode
 *
 * @deprecated Use SelectionModeInfoCb API instead.
 */
using SelectionModeResponseCallback
   = std::function<void(NetworkSelectionMode mode, telux::common::ErrorCode error)>;

/**
 * This function is called with the response to requestNetworkSelectionMode API.
 *
 * The callback can be invoked from multiple different threads.
 * The implementation should be thread safe.
 *
 * @param [in] info       Provides NetworkSelectionMode, MCC and MNC. @ref NetworkModeInfo
 * @param [in] error      Return code which indicates whether the operation
 *                        succeeded or not.
 *                        @ref telux::common::ErrorCode
 */
using SelectionModeInfoCb
   = std::function<void(NetworkModeInfo info, telux::common::ErrorCode error)>;

/**
 * This function is called with the response to requestPreferredNetworks API.
 *
 * The callback can be invoked from multiple different threads.
 * The implementation should be thread safe.
 *
 * @param [in] info         3GPP preferred networks list i.e PLMN list.
 * @param [in] staticInfo   Static 3GPP preferred networks list i.e OPLMN list.
 * @param [in] error        Return code which indicates whether the operation
 *                          succeeded or not.
 *                          @ref telux::common::ErrorCode
 */
using PreferredNetworksCallback = std::function<void(std::vector<PreferredNetworkInfo> info,
                                                     std::vector<PreferredNetworkInfo> staticInfo,
                                                     telux::common::ErrorCode error)>;

/**
 * This function is called with the response to performNetworkScan API.
 *
 * The callback can be invoked from multiple different threads.
 * The implementation should be thread safe.
 *
 * @param [in] operatorInfos   Operators info with details of network operator
 *                             name, MCC, MNC and status.
 * @param [in] error           Return code which indicates whether the operation
 *                             succeeded or not.
 *                             @ref telux::common::ErrorCode
 */
using NetworkScanCallback
   = std::function<void(std::vector<OperatorInfo> operatorInfos, telux::common::ErrorCode error)>;

/** @addtogroup telematics_network_selection
* @{ */

/**
 * @brief   Network Selection Manager class provides the interface to get and set
 *          network selection mode, preferred network list and scan available networks.
 */
class INetworkSelectionManager {
public:
   /**
    * Checks the status of network subsystem and returns the result.
    *
    * @returns True if network subsystem is ready for service otherwise false.
    *
    * @deprecated Use INetworkSelectionManager::getServiceStatus() API.
    */
   virtual bool isSubsystemReady() = 0;

   /**
    * Wait for network subsystem to be ready.
    *
    * @returns  A future that caller can wait on to be notified when network
    *           subsystem is ready.
    *
    * @deprecated Use InitResponseCb in PhoneFactory::getNetworkSelectionManager instead, to
    *             get notified about subsystem readiness.
    */
   virtual std::future<bool> onSubsystemReady() = 0;

   /**
    * This status indicates whether the INetworkSelectionManager object is in a usable state.
    *
    * @returns SERVICE_AVAILABLE    -  If Serving System manager is ready for service.
    *          SERVICE_UNAVAILABLE  -  If Serving System manager is temporarily unavailable.
    *          SERVICE_FAILED       -  If Serving System manager encountered an irrecoverable
    *                                  failure.
    *
    */
   virtual telux::common::ServiceStatus getServiceStatus() = 0;

   /**
    * Get current network selection mode (i.e Manual or Automatic) asynchronously.
    *
    * On platforms with Access control enabled, Caller needs to have
    * TELUX_TEL_NETWORK_SELECTION_READ permission to invoke this API successfully.
    *
    * @param [in] callback    Callback function to get the response of get
    *                         network selection mode request.
    *
    * @returns Status of requestNetworkSelectionMode i.e. success or suitable error code.
    *
    */
   virtual telux::common::Status
      requestNetworkSelectionMode(SelectionModeInfoCb callback)
      = 0;

   /**
    * Set current network selection mode and receive the response asynchronously.
    *
    * On platforms with Access control enabled, Caller needs to have TELUX_TEL_NETWORK_SELECTION_OPS
    * permission to invoke this API successfully.
    *
    * @note This API is not supported for the NTN network.
    *
    * @param [in] selectMode    Selection mode for a network i.e. automatic or manual.
    *                           If selection mode is automatic then MCC and MNC are ignored.
    *                           If it is manual, client has to explicitly pass MCC and MNC
    *                           as arguments.
    * @param [in] callback      Optional callback function to get the response of
    *                           set network selection mode request.
    * @param [in] mcc           Mobile Country Code (Applicable only for MANUAL selection mode).
    * @param [in] mnc           Mobile Network Code (Applicable only for MANUAL selection mode).
    *
    * @returns  Status of setNetworkSelectionMode i.e. success or suitable error code.
    */
   virtual telux::common::Status setNetworkSelectionMode(NetworkSelectionMode selectMode,
                                                         std::string mcc, std::string mnc,
                                                         common::ResponseCallback callback
                                                         = nullptr)
      = 0;

   /**
    * Get 3GPP preferred network list and static 3GPP preferred network list
    * asynchronously. Higher priority networks appear first in the list. The networks
    * that appear in the 3GPP Preferred Networks list get higher priority than the
    * networks in the static 3GPP preferred networks list.
    *
    * On platforms with Access control enabled, Caller needs to have
    * TELUX_TEL_NETWORK_SELECTION_READ permission to invoke this API successfully.
    *
    * @param [in] callback    Callback function to get the response of get preferred
    *                         networks request.
    *
    * @returns Status of requestPreferredNetworks i.e. success or suitable error code.
    */
   virtual telux::common::Status requestPreferredNetworks(PreferredNetworksCallback callback) = 0;

   /**
    * Set 3GPP preferred network list and receive the response asynchronously. It
    * overrides the existing preferred network list. The preferred network list
    * affects network selection selection when automatic registration is performed
    * by the device. Higher priority networks should appear first in the list.
    *
    * On platforms with Access control enabled, Caller needs to have TELUX_TEL_NETWORK_SELECTION_OPS
    * permission to invoke this API successfully.
    *
    * @note This API is not supported for the NTN network.
    *
    * @param [in] preferredNetworksInfo   List of 3GPP preferred networks.
    * @param [in] clearPrevious           If flag is false then new 3GPP preferred
    *                                     network list is appended to existing
    *                                     preferred network list. If flag
    *                                     is true then old list is flushed and
    *                                     new 3GPP preferred network list is added.
    * @param [in] callback                Callback function to get the response
    *                                     of set preferred network list request.
    *
    * @returns Status of setPreferredNetworks i.e. success or suitable error code.
    */

   virtual telux::common::Status
      setPreferredNetworks(std::vector<PreferredNetworkInfo> preferredNetworksInfo,
                           bool clearPrevious, common::ResponseCallback callback = nullptr)
      = 0;

   /**
    * Perform the network scan and returns a list of available networks.
    *
    * On platforms with Access control enabled, Caller needs to have TELUX_TEL_NETWORK_SELECTION_OPS
    * permission to invoke this API successfully.
    *
    * @param [in] callback    Callback function to get the response of perform
    *                         network scan request
    *
    * @returns Status of performNetworkScan i.e. success or suitable error code.
    *
    * @deprecated Use INetworkSelectionManager::performNetworkScan(
    *     common::ResponseCallback callback) API instead
    */
   virtual telux::common::Status performNetworkScan(NetworkScanCallback callback) = 0;

   /**
    * Perform the network scan. The available networks list is returned incrementally as they
    * become available, without waiting for the entire scan to complete through the
    * indication API (INetworkSelectionListener::onNetworkScanResults).
    * The scan status in indication will indicate if its a partial result or complete result.
    *
    * On platforms with Access control enabled, Caller needs to have TELUX_TEL_NETWORK_SELECTION_OPS
    * permission to invoke this API successfully.
    *
    * @note This API is not supported for the NTN network. To perform network scan on the NTN
    * network, use @ref telux::satcom::INtnManager::enableCellularScan
    *
    * @param [in] info        Provides network scan type and if the network scan type is user
    *                         prefered RAT, includes RAT(s) information. @ref NetworkScanInfo
    * @param [in] callback    Callback function to get the response of network scan request
    *
    * @returns Status of performNetworkScan i.e. success or suitable error code.
    *
    *
    */
   virtual telux::common::Status performNetworkScan(NetworkScanInfo info,
      common::ResponseCallback callback = nullptr) = 0;

   /**
    * Set a list of LTE dubious cells to expedite the detection of data stalls. It overrides the
    * existing dubious cell list.
    *
    * Dubious cell parameters are not persistent over device reboot or subsystem restart (SSR)
    * updated via @ref INetworkSelectionListener::onServiceStatusChange.
    *
    * On platforms with Access control enabled, Caller needs to have TELUX_TEL_SNS_CONFIG
    * permission to invoke this API successfully.
    *
    * @param [in] lteDbCellList         List of LTE dubious cells.
    *                                   @ref telux::tel::LteDubiousCell
    *
    * @returns Error code which indicates whether the operation succeeded or not.
    *
    * @note    Eval: This is a new API and is being evaluated. It is subject to change
    *          and could break backwards compatibility.
    */
   virtual telux::common::ErrorCode setLteDubiousCell(const std::vector<LteDubiousCell>
           &lteDbCellList) = 0;

   /**
    * Set a list of NR dubious cells to expedite the detection of data stalls. It overrides the
    * existing dubious cell list.
    *
    * Dubious cell parameters are not persistent over device reboot or subsystem restart (SSR)
    * updated via @ref INetworkSelectionListener::onServiceStatusChange.
    *
    * On platforms with Access control enabled, Caller needs to have TELUX_TEL_SNS_CONFIG
    * permission to invoke this API successfully.
    *
    * @param [in] nrDbCellList         List of NR dubious cells.
    *                                  @ref telux::tel::NrDubiousCell
    *
    * @returns Error code which indicates whether the operation succeeded or not.
    *
    * @note    Eval: This is a new API and is being evaluated. It is subject to change
    *          and could break backwards compatibility.
    */
   virtual telux::common::ErrorCode setNrDubiousCell(const std::vector<NrDubiousCell>
           &nrDbCellList) = 0;

    /**
     * Abort an ongoing network scan operation that was previously initiated using
     * telux::tel::INetworkSelectionManager::performNetworkScan(NetworkScanInfo info,
     * common::ResponseCallback callback).
     * If no network scan has been initiated prior to this call, the API returns
     * telux::common::ErrorCode::INVALID_OPERATION, indicating that there is no active
     * scan to abort.
     *
     * If this API is invoked after calling the deprecated
     * telux::tel::INetworkSelectionManager::performNetworkScan(NetworkScanCallback callback),
     * the deprecated API will receive a callback with telux::common::ErrorCode::ABORTED,
     * indicating that the ongoing operation has been successfully aborted.
     *
     * On platforms with access control enabled, the caller needs to have the
     * TELUX_TEL_NETWORK_SELECTION_OPS permission to successfully invoke this API.
     *
     * @returns Status of abortNetworkScan i.e. success or suitable error code.
     *
     * @note   Eval: This is a new API and is being evaluated. It is subject to
     *         change and could break backwards compatibility.
     */
   virtual telux::common::ErrorCode abortNetworkScan() = 0;

   /**
    * Register a listener for specific updates from network access service.
    *
    * @param [in] listener    Pointer of INetworkSelectionListener object that
    *                         processes the notification
    *
    * @returns Status of registerListener i.e success or suitable status code.
    */
   virtual telux::common::Status registerListener(std::weak_ptr<INetworkSelectionListener> listener)
      = 0;

   /**
    * Deregister the previously added listener.
    *
    * @param [in] listener    Previously registered INetworkSelectionListener
    *                         that needs to be removed
    *
    * @returns Status of removeListener success or suitable status code
    */
   virtual telux::common::Status
      deregisterListener(std::weak_ptr<INetworkSelectionListener> listener)
      = 0;

   /**
    * Get current network selection mode (i.e Manual or Automatic) asynchronously.
    *
    * On platforms with Access control enabled, Caller needs to have
    * TELUX_TEL_NETWORK_SELECTION_READ permission to invoke this API successfully.
    *
    * @param [in] callback    Callback function to get the response of get
    *                         network selection mode request.
    *
    * @returns Status of requestNetworkSelectionMode i.e. success or suitable error code.
    *
    * @deprecated Use INetworkSelectionManager::requestNetworkSelectionMode(
    *     SelectionModeInfoCb callback) API instead.
    */
   virtual telux::common::Status requestNetworkSelectionMode(SelectionModeResponseCallback callback)
      = 0;

   virtual ~INetworkSelectionManager(){};
};

/**
 * Operator Info class provides operator name, MCC, MNC and network status.
 */
class OperatorInfo {
public:
   OperatorInfo(std::string networkName, std::string mcc, std::string mnc,
                OperatorStatus operatorStatus);

   OperatorInfo(std::string networkName, std::string mcc, std::string mnc, RadioTechnology rat,
      OperatorStatus operatorStatus);

   /**
    * Get Operator name or description
    *
    * @returns Operator name.
    */
   std::string getName();

   /**
    * Get mcc from the operator numeric.
    *
    * @returns MCC.
    */
   std::string getMcc();

   /**
    * Get mnc from operator numeric.
    *
    * @returns MNC.
    */
   std::string getMnc();

   /**
    * Get radio access technology.
    *
    * @returns Radio access technology(RAT) @ref RadioTechnology.
    */
   RadioTechnology getRat();

   /**
    * Get status of operator.
    *
    * @returns status of the operator @ref OperatorStatus.
    */
   OperatorStatus getStatus();

private:
   std::string networkName_;
   std::string mcc_;
   std::string mnc_;
   RadioTechnology rat_;
   OperatorStatus operatorStatus_;
};

/**
 * @brief Listener class for getting network selection mode change notification
 *
 *        The methods in listener can be invoked from multiple different threads.
 *        Client needs to make sure that implementation is thread-safe.
 */
class INetworkSelectionListener : public common::IServiceStatusListener {
public:
   /**
    * This function is called whenever network selection mode is changed.
    *
    * On platforms with Access control enabled, Caller needs to have
    * TELUX_TEL_NETWORK_SELECTION_READ permission to receive this notification.
    *
    * @param [in] info   Provides NetworkSelectionMode, MCC and MNC. @ref NetworkModeInfo
    *
    */
   virtual void onSelectionModeChanged(NetworkModeInfo info) {
   }

   /**
    * This function is called in response to performNetworkScan API.
    * This API will be invoked multiple times in case of partial network scan results.
    * In case of network scan failure and network scan completed this API will not be invoked
    * further.
    *
    * @note This API is not supported for the NTN network.
    *
    * @param [in] scanStatus      Status of the network scan results @ref NetworkScanStatus
    * @param [in] operatorInfos   Operators info with details of network operator name, MCC,
    *                             MNC, etc. In case of partial network scan results, the
    *                             operator info will have the information of the new set of
    *                             operator info along with previous partial network scan results.
    *
    *
    */
   virtual void onNetworkScanResults(NetworkScanStatus scanStatus,
      std::vector<telux::tel::OperatorInfo> operatorInfos) {
   }

   /**
    * This function is called whenever network selection mode is changed.
    *
    * On platforms with Access control enabled, Caller needs to have
    * TELUX_TEL_NETWORK_SELECTION_READ permission to receive this notification.
    *
    * @param [in] mode    Network selection mode. @ref NetworkSelectionMode
    *
    * @deprecated Use INetworkSelectionListener::onSelectionModeChanged(
    *    NetworkModeInfo info) API instead.
    */
   virtual void onSelectionModeChanged(NetworkSelectionMode mode) {
   }

   /**
    * Destructor of INetworkSelectionListener
    */
   virtual ~INetworkSelectionListener() {
   }
};

/** @} */ /* end_addtogroup telematics_network_selection */

}  // end of namespace tel
}  // end of namespace telux

#endif // TELUX_TEL_NETWORKSELECTIONMANAGER_HPP
