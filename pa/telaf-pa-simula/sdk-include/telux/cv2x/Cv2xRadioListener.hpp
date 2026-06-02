/*
 *  Copyright (c) 2018-2020, The Linux Foundation. All rights reserved.
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
* @file       Cv2xRadioListener.hpp
*
* @brief      Cv2xRadioListener is the listener interface for Cv2xRadio
*/

#ifndef TELUX_CV2X_CV2XRADIOLISTENER_HPP
#define TELUX_CV2X_CV2XRADIOLISTENER_HPP

#include <telux/common/SDKListener.hpp>
#include <telux/common/CommonDefines.hpp>
#include <telux/cv2x/Cv2xRadioTypes.hpp>

namespace telux {

namespace cv2x {

/** @addtogroup telematics_cv2x_cpp
 * @{ */

/**
 *@brief Listeners for Cv2xRadio must implement this interface.
 */
class ICv2xRadioListener : virtual public telux::common::ISDKListener {
public:

    /**
     * Called when the status of the CV2X radio has changed.
     *
     * @param [in] status - CV2X radio status.
     *
     * @deprecated use onStatusChanged in Cv2xListener
     */
    virtual void onStatusChanged(Cv2xStatus status) {}

    /**
     * Called when the status of the CV2X radio has changed.
     *
     * @param [in] status - CV2X radio status.
     *
     * @deprecated use onStatusChanged in Cv2xListener
     */
    virtual void onStatusChanged(Cv2xStatusEx status) {}

    /**
     * Called when the L2 Address has changed.
     *
     * @param [in] newL2Address - The new L2 address.
     */
    virtual void onL2AddrChanged(uint32_t newL2Address) {}

    /**
     * Called when SPS offset has changed.
     *
     * @param [in] spsId   - SPS Id of the SPS flow
     * @param [in] details - new SPS MAC PHY details.
     *
     * @deprecated use onSpsSchedulingChanged
     */
    virtual void onSpsOffsetChanged(int spsId,
                                    MacDetails details) {}

    /**
     * Called when SPS scheduling has changed.
     *
     * @param [in] schedulingInfo - SPS scheduling information .
     */
    virtual void onSpsSchedulingChanged(const SpsSchedulingInfo & schedulingInfo) {}

    /**
     * Called when Cv2x radio capabilities have changed.
     *
     * @param [in] capabilities - Capabilities of the CV2X radio .
     */
    virtual void onCapabilitiesChanged(const Cv2xRadioCapabilities & capabilities) {}


    /**
     * Called when a MAC address cloning attack is detected or cleared.
     * MAC address collisions should be extremely rare. If they are happening
     * frequently within a detetion period, it will be identified as a MAC address
     * cloning attack and users will be notified through this API. This API is also
     * invoked when the attack is cleared.
     * The collision count threshold and the detection period for MAC address cloning
     * attack detection are configurable by setting parameter cv2x.collision.threshold
     * and cv2x.collision.window.size in the file /etc/tel.conf on device.
     *
     * @param [out] detected - True when a MAC address cloning attack is detected.
     *                         False when a MAC address cloning attack is cleared.
     */
    virtual void onMacAddressCloneAttack(const bool detected) {}

    /**
     * Destructor for ICv2xRadioListener
     */
    virtual ~ICv2xRadioListener(){}
};

/** @} */ /* end_addtogroup telematics_cv2x_cpp */

} // namespace cv2x

} // namespace telux

#endif // TELUX_CV2X_CV2XRADIOLISTENER_HPP
