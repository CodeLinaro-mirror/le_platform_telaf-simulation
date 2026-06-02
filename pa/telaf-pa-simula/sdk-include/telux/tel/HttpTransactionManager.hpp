/*
 *  Copyright (c) 2021, The Linux Foundation. All rights reserved.
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
 * @file       HttpTransactionManager.hpp
 *
 * @brief      IHttpTransactionManager is the interface to service HTTP related requests
 *             from the modem for SIM profile update related operations.
 *
 */

#ifndef TELUX_TEL_HTTPTRANSACTIONMANAGER_HPP
#define TELUX_TEL_HTTPTRANSACTIONMANAGER_HPP

#include <memory>
#include <vector>

#include <telux/common/CommonDefines.hpp>

#include <telux/tel/HttpListener.hpp>
#include <telux/tel/SimProfileDefines.hpp>

namespace telux {
namespace tel {

/** @addtogroup telematics_rsp
 * @{ */

/**
 * Defines the HTTP request result.
 */
enum class HttpResult {
    TRANSACTION_SUCCESSFUL = 0, /**<  HTTP request successful \n  */
    UNKNOWN_ERROR = 1,          /**<  Unknown error \n  */
    HTTP_SERVER_ERROR = 2,      /**<  Server error \n  */
    HTTP_TLS_ERROR = 3,         /**<  TLS error    \n  */
    HTTP_NETWORK_ERROR = 4,     /**<  Network error \n  */
};

/**
 *@brief IHttpTransactionManager is the interface to service HTTP related requests
 *       from the modem for SIM profile update related operations.
 */
class IHttpTransactionManager {
 public:

    /**
     * This status indicates whether the IHttpTransactionManager object is in a usable state.
     *
     * @returns SERVICE_AVAILABLE    -  If HTTP transaction manager is ready for service.
     *          SERVICE_UNAVAILABLE  -  If HTTP transaction manager is temporarily unavailable.
     *          SERVICE_FAILED       -  If HTTP transaction manager encountered an irrecoverable
     *                                  failure.
     */
    virtual telux::common::ServiceStatus getServiceStatus() = 0;

    /**
     * Send the result of HTTP Post request transaction to modem.
     *
     * On platforms with access control enabled, caller needs to have
     * TELUX_TEL_SIM_PROFILE_HTTP_PROXY permission to invoke this API successfully.
     *
     * @param [in] token             Token identifier for request and response pair.
     * @param [in] result            HTTP transaction request result.
     * @param [in] headers           Custom Headers in HTTP Response.
     * @param [in] response          HTTP response payload.
     * @param [in] callback          Callback function to get the result of send HTTP transaction
     *                               request.
     *
     * @returns  Status of send HTTP transaction result i.e. success or suitable error code.
     */
    virtual telux::common::Status sendHttpTransactionResult(uint32_t token, HttpResult result,
        const std::vector<CustomHeader> &headers, const std::vector<uint8_t> &response,
        common::ResponseCallback callback = nullptr) = 0;

    /**
     * Register a listener for specific events like perform HTTP Post request.
     *
     * @param [in] listener    Pointer of IHttpTransactionListener object that processes the
     *                         notification.
     *
     * @returns Status of registerHttpListener success or suitable status code.
     */
    virtual telux::common::Status registerListener(std::weak_ptr<IHttpTransactionListener> listener)
        = 0;

    /**
     * De-register the listener.
     *
     * @param [in] listener    Pointer of IHttpTransactionListener object that needs to be
     *                         deregistered.
     *
     * @returns Status of deregisterHttpListener success or suitable status code.
     */
    virtual telux::common::Status deregisterListener(
        std::weak_ptr<IHttpTransactionListener> listener) = 0;

    /**
     * Destructor for IHttpTransactionManager
     */
    virtual ~IHttpTransactionManager() {
    }
};  // end of IHttpTransactionManager

/** @} */ /* end_addtogroup telematics_rsp */
}
}

#endif // TELUX_TEL_HTTPTRANSACTIONMANAGER_HPP
