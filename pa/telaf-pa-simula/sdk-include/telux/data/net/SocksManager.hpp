/*
 *  Copyright (c) 2019, The Linux Foundation. All rights reserved.
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
 * @file       SocksManager.hpp
 *
 * @brief      SocksManager is a primary interface for configuring Socks Proxy Server
 *
 */

#ifndef TELUX_DATA_NET_SOCKSMANAGER_HPP
#define TELUX_DATA_NET_SOCKSMANAGER_HPP

#include <future>
#include <vector>
#include <list>
#include <memory>

#include <telux/common/SDKListener.hpp>
#include <telux/common/CommonDefines.hpp>
#include <telux/data/DataDefines.hpp>

namespace telux {
namespace data {
namespace net {

// Forward declarations
class ISocksListener;

/** @addtogroup telematics_data_net
 * @{ */
/**
 *@brief    SocksManager is a primary interface for configuring legacy Socks proxy server.
 *          It also provides interface to Subsystem Restart events by registering as listener.
 *          Notifications will be received when modem is ready/not ready.
 */
class ISocksManager {
 public:
    /**
     * Checks the status of SocksManager and returns the result.
     *
     * @returns SERVICE_AVAILABLE      If Socks manager object is ready for service.
     *          SERVICE_UNAVAILABLE    If Socks manager object is temporarily unavailable.
     *          SERVICE_FAILED       - If Socks manager object encountered an irrecoverable failure.
     *
     */
    virtual telux::common::ServiceStatus getServiceStatus() = 0;

    /**
     * Checks if the SocksManager subsystem is ready.
     *
     * @returns True if SocksManager is ready for service, otherwise
     * returns false.
     *
     * @deprecated Use getServiceStatus API..
     */
    virtual bool isSubsystemReady() = 0;

    /**
     * Wait for SocksManager subsystem to be ready.
     *
     * @returns A future that caller can wait on to be notified
     * when Socksanager is ready.
     *
     *  @deprecated Use InitResponseCb callback in factory API getSocksManager.
     */
    virtual std::future<bool> onSubsystemReady() = 0;

    /**
     * Enable or Disable Socks proxy service.
     *
     * On platforms with Access control enabled, Caller needs to have TELUX_DATA_NETWORK_CONFIG
     * permission to invoke this API successfully.
     *
     * @param [in] enable            true: enable proxy, false: disable proxy
     * @param [in] callback          optional callback to get the operation error code if any
     *
     * @returns Status of proxy enablement i.e. success or suitable status code.
     *
     */

    virtual  telux::common::Status enableSocks(bool enable,
        telux::common::ResponseCallback callback = nullptr) = 0;

    /**
     * Register Socks Manager as listener for Data Service heath events like data service available
     * or data service not available.
     *
     * @param [in] listener    pointer of ISocksListener object that processes the
     * notification
     *
     * @returns Status of registerListener success or suitable status code
     *
     */
    virtual telux::common::Status registerListener(std::weak_ptr<ISocksListener> listener) = 0;

    /**
     * Removes a previously added listener.
     *
     * @param [in] listener    pointer of ISocksListener object that needs to be removed
     *
     * @returns Status of deregisterListener success or suitable status code
     *
     */
    virtual telux::common::Status deregisterListener(std::weak_ptr<ISocksListener> listener) = 0;

    /**
     * Get the associated operation type for this instance.
     *
     * @returns OperationType of getOperationType i.e. LOCAL or REMOTE.
     *
     */
    virtual telux::data::OperationType getOperationType() = 0;

    /**
     * Destructor for Socks Manager
     */
    virtual ~ISocksManager(){};
};  // end of ISocksManager

/**
 * Interface for Socks listener object. Client needs to implement this interface to get
 * access to Socks services notifications like onServiceStatusChange.
 *
 * The methods in listener can be invoked from multiple different threads. The implementation
 * should be thread safe.
 *
 */
class ISocksListener : public telux::common::ISDKListener{
 public:
    /**
     * This function is called when service status changes.
     *
     * @param [in] status - @ref ServiceStatus
     */
    virtual void onServiceStatusChange(telux::common::ServiceStatus status) {}

    /**
     * Destructor for ISocksListener
     */
    virtual ~ISocksListener(){};
};

/** @} */ /* end_addtogroup telematics_data_net */
}
}
}

#endif // TELUX_DATA_NET_SOCKSMANAGER_HPP
