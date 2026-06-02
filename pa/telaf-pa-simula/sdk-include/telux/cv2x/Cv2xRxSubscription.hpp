/*
 *  Copyright (c) 2018-2020 The Linux Foundation. All rights reserved.
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

/**
* @file       Cv2xRxSubscription.hpp
*
* @brief      Represents a Cv2x Rx subscription. Encapsulates the Rx socket
*             and associated Socket address.
*/

/*
 * Changes from Qualcomm Technologies, Inc. are provided under the following license:
 *
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef TELUX_CV2X_CV2XRXSUBSCRIPTION_HPP
#define TELUX_CV2X_CV2XRXSUBSCRIPTION_HPP

#include <vector>
#include <memory>
#include <netinet/in.h>

#include <telux/cv2x/Cv2xRadioTypes.hpp>


namespace telux {

namespace cv2x {

/** @addtogroup telematics_cv2x_cpp
 * @{ */

/**
 * This class encapsulates a Cv2xRadio Rx Subscription. It contains the Rx socket
 * associated with the subscription from which client applications can read data. This
 * class is referenced in @ref ICv2xRadio::createRxSubscription and
 * @ref ICv2xRadio::closeRxSubscription.
 */
class ICv2xRxSubscription {
public:

    /**
     * Accessor for Rx subscription ID
     *
     * @returns subscription ID
     */
    virtual uint32_t getSubscriptionId() const = 0;

    /**
     * Accessor for IP traffic type
     *
     * @returns The Rx subscriptions's IP traffic type (IP or NON-IP)
     */
    virtual TrafficIpType getIpType() const = 0;

    /**
     * Accessor for the socket file descriptor
     *
     * @returns The Rx subscriptions's socket fd.
     */
    virtual int getSock() const = 0;

    /**
     * Accessor for the socket address description
     *
     * @returns The Rx subscriptions's socket address
     */
    virtual struct sockaddr_in6 getSockAddr() const = 0;

    /**
     * Accessor for the subscriptions's port number
     *
     * @returns The Rx subscriptions's port num
     */
    virtual uint16_t getPortNum() const = 0;

    /**
     * Get subscriptions's service ID list
     *
     * @returns The Rx subscriptions's service ID list
     */
    virtual std::shared_ptr<std::vector<uint32_t>> getServiceIDList() const = 0;

    virtual ~ICv2xRxSubscription() {}

    /**
     * Set subscriptions's service ID list
     *
     * @param [in] idList        - the subscriptions's service ID list
     *
     * @deprecated Service IDs are set via ICv2xRadio::createRxSubscription and cannot be
     * modified after that.
     */
    virtual void setServiceIDList(const std::shared_ptr<std::vector<uint32_t>> idList) = 0;
};

/** @} */ /* end_addtogroup telematics_cv2x_cpp */

} // namespace cv2x

} // namespace telux

#endif // TELUX_CV2X_CV2XRXSUBSCRIPTION_HPP
