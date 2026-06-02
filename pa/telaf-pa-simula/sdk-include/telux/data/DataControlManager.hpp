/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file       DataControlManager.hpp
 *
 *
 * @brief      The DataControlManager class provides APIs related to data control.
 *             For example - ability to set data stall parameters.
 */

#ifndef TELUX_DATA_DATACONTROLMANAGER_HPP
#define TELUX_DATA_DATACONTROLMANAGER_HPP

#include <memory>

#include <telux/common/SDKListener.hpp>
#include <telux/data/DataDefines.hpp>

namespace telux {
namespace data {

/** @addtogroup telematics_data
 * @{ */

// Forward declarations
class IDataControlListener;

/**
 * Specifies an application type
 */
enum class ApplicationType {
    UNSPECIFIED = 0,     /** Unspecified application*/
    CONV_AUDIO = 1,      /** Conversation audio application */
    CONV_VIDEO = 2,      /** Conversation video application */
    STREAMING_AUDIO = 3, /** Streaming audio application */
    STREAMING_VIDEO = 4, /** Streaming video application */
    TYPE_GAMING = 5,     /** Gaming application */
    WEB_BROWSING = 6,    /** Web browsing application */
    FILE_TRANSFER = 7    /** File transfer application */
};

/**
 * Specifies the data stall parameters
 */
struct DataStallParams {
    Direction trafficDir;    /** Traffic direction */
    ApplicationType appType; /** Application type */
    bool dataStall = false;  /** Data stall status */
};

/**
 * @brief The DataControlManager class provides APIs related to data control.
 *        For example - ability to set data stall parameters.
 */

class IDataControlManager {
public:
    /**
     * Checks the status of Data Control manager object and returns the result.
     *
     * @returns SERVICE_AVAILABLE    -  If DataControlManager object is ready for service.
     *          SERVICE_UNAVAILABLE  -  If DataControlManager object is temporarily unavailable.
     *          SERVICE_FAILED       -  If DataControlManager object encountered an irrecoverable
     *                                  failure.
     *
     * @note   Eval: This is a new API and is being evaluated. It is subject to change and could
     *         break backwards compatibility.
     */
    virtual telux::common::ServiceStatus getServiceStatus() = 0;

    /**
     * Allows a client to indicate to the modem that the client has detected a data stall. When a
     * client invokes this API on detecting a data stall on the current serving cell, it expedites
     * the modem's mitigation for data stalls.
     *
     * Data stall parameters are not persistent over device reboot or subsystem restart (SSR)
     * updated via @ref IDataControlListener::onServiceStatusChange
     *
     * On platforms with access control enabled, caller needs to have TELUX_DATA_SNS_CONFIG
     * permission to invoke this API successfully.
     *
     * @param [in]  slotId     Slot ID on which the operation is being performed.
     * @param [in]  params     Data stall parameters. @ref telux::data::DataStallParams
     *
     * @returns                @ref telux::common::ErrorCode as appropriate.
     *
     * @note   Eval: This is a new API and is being evaluated. It is subject to change and could
     *         break backwards compatibility.
     */
    virtual telux::common::ErrorCode setDataStallParams(const SlotId &slotId,
            const DataStallParams &params) = 0;

    /**
     * Register with the DataControlManager as a listener for service status and other events.
     *
     * @param [in] listener    Pointer of IDataControlListener object that processes the
     * notification
     *
     * @returns Status of registerListener success or suitable status code
     *
     * @note   Eval: This is a new API and is being evaluated. It is subject to change and could
     *         break backwards compatibility.
     */
    virtual telux::common::Status registerListener(
        std::weak_ptr<IDataControlListener> listener) = 0;

    /**
     * Removes a previously added listener.
     *
     * @param [in] listener    Pointer of IDataControlListener object that needs to be removed
     *
     * @returns Status of deregisterListener success or suitable status code
     *
     * @note   Eval: This is a new API and is being evaluated. It is subject to change and could
     *         break backwards compatibility.
     */
    virtual telux::common::Status deregisterListener(
        std::weak_ptr<IDataControlListener> listener) = 0;
};

/**
 * Interface for DataControl listener object. Client needs to implement this interface to get
 * access to DataControl services notifications like onServiceStatusChange.
 *
 * The methods in listener can be invoked from multiple different threads. The implementation
 * should be thread safe.
 *
 */
class IDataControlListener : public telux::common::ISDKListener {
 public:
    /**
     * This function is called when service status changes.
     *
     * @param [in] status    @ref ServiceStatus
     */
    virtual void onServiceStatusChange(telux::common::ServiceStatus status) {}

    /**
     * Destructor for IDataControlListener
     */
    virtual ~IDataControlListener(){};
};

/** @} */ /* end_addtogroup telematics_data */
}
}

#endif // TELUX_DATA_DATACONTROLMANAGER_HPP
