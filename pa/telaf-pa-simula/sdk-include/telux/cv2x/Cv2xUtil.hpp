/*
 *  Copyright (c) 2021 The Linux Foundation. All rights reserved.
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
* @file       Cv2xUtil.hpp
*
* @brief      Cv2xUtil class encapsulates CV2X utilities.
*/

#ifndef TELUX_CV2X_CV2XUTIL_HPP
#define TELUX_CV2X_CV2XUTIL_HPP

#include <telux/cv2x/Cv2xRadioTypes.hpp>

namespace telux {

namespace cv2x {

/** @addtogroup telematics_cv2x_cpp
 * @{ */

/**
 * Cv2x utility class
*/
class Cv2xUtil{
public:

    /**
     * This function is called to convert cv2x flow priority to traffic class.
     * The Traffic Class indicates class or priority of IPv6 packet. If congestion occurs
     * then packets with least priority will be discarded(See RFC2460 section-7).
     * The result of this method is to fill the IPv6 header traffic class field,
     * it is usually called just before sending IPv6 packet.
     *
     * @param [in] priority      - cv2x flow priority
     *
     * @returns uint8_t to indicate the result of traffic class
     */
    static uint8_t priorityToTrafficClass(Priority priority);

    /**
     * This function is called to convert IPv6 packet traffic class to cv2x flow priority.
     * The Traffic Class indicates class or priority of IPv6 packet. If congestion occurs
     * then packets with least priority will be discarded(See RFC2460 section-7).
     * It is to get the corresponding cv2x flow priority of the received packets,
     * which usually being called when a new IPv6 packet received.
     *
     * @param [in] trafficClass  -  class or priority of IPv6 packet(See RFC2460 section-7)
     *
     * @returns cv2x flow priority
     */
    static Priority TrafficClassToPriority(uint8_t trafficClass);
};

/** @} */ /* end_addtogroup telematics_cv2x_cpp */

} // namespace cv2x

} // namespace telux

#endif // TELUX_CV2X_CV2XUTIL_HPP

