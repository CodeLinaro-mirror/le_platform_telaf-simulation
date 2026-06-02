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
 * @file       HttpListener.hpp
 *
 * @brief      The interface listens for indication to perform HTTP POST request and send back the
 *             response for HTTP request to modem.
 */

#ifndef TELUX_TEL_HTTPLISTENER_HPP
#define TELUX_TEL_HTTPLISTENER_HPP

#include <string>
#include <vector>

#include <telux/common/CommonDefines.hpp>
#include <telux/tel/SimProfileDefines.hpp>

namespace telux {
namespace tel {

/** @addtogroup telematics_rsp
 * @{ */

/**
 *Header information to be sent along with HTTP post request.
 */
struct CustomHeader {
    std::string name;  /**< Header name */
    std::string value; /**< Header value */
};

/**
 * @brief The interface listens for indication to perform HTTP request and send back the
 *        response for HTTP request to modem.
 *
 *        The methods in the listener can be invoked from multiple threads.
 *        It is client's responsibility to make sure the implementation is thread safe.
 */
class IHttpTransactionListener : public telux::common::IServiceStatusListener {
 public:
    /**
     * An application handling this indication should perform the HTTP request and call the
     * IHttpTransactionManager::sendHttpTransactionReq to provide the result of the HTTP
     * transaction.
     *
     * On platforms with access control enabled, the client needs to have
     * TELUX_TEL_SIM_PROFILE_HTTP_PROXY permission to invoke this API successfully.
     *
     * @param [in] url                   URL to sent HTTP post request.
     * @param [in] tokenId               Token identifier.
     * @param [in] headers               Header information to be sent along with HTTP post request.
     * @param [in] reqPayload            Request payload.
     */
    virtual void onNewHttpRequest(const std::string &url, uint32_t tokenId,
        const std::vector<CustomHeader> &headers, const std::vector<uint8_t> &reqPayload) {
    }

    /**
     * Destructor of IHttpTransactionListener
     */
    virtual ~IHttpTransactionListener() {
    }
};
/** @} */ /* end_addtogroup telematics_rsp */

}  // end of namespace tel
}  // end of namespace telux

#endif // TELUX_TEL_HTTPLISTENER_HPP
