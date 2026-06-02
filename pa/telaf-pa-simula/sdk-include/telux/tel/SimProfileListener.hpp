/*
 *  Copyright (c) 2019-2020, The Linux Foundation. All rights reserved.
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
 * @file       SimProfileListener.hpp
 *
 * @brief      The interface listens for profile download indication and keep track of
 *             download and install progress of profile.
 */

#ifndef TELUX_TEL_SIMPROFILELISTENER_HPP
#define TELUX_TEL_SIMPROFILELISTENER_HPP

#include <telux/tel/SimProfileDefines.hpp>

namespace telux {
namespace tel {

/** @addtogroup telematics_rsp
 * @{ */

/**
 * @brief The interface listens for profile download indication and keep track of
 *         download and install progress of profile.
 *
 *        The methods in the listener can be invoked from multiple threads.
 *        It is client's responsibility to make sure the implementation is thread safe.
 */
class ISimProfileListener : public telux::common::IServiceStatusListener {
 public:

    /**
     * This function is called when indication about status of profile download and installation
     * comes.
     *
     * On platforms with access control enabled, the client needs to have TELUX_TEL_SIM_PROFILE_OPS
     * permission to invoke this API successfully.
     *
     * @param [in] slotId                   Slot on which profile get downloaded and installed.
     * @param [in] status                   @ref telux::tel::DownloadStatus.
     * @param [in] cause                    @ref telux::tel::DownloadErrorCause.
     *
     */
    virtual void onDownloadStatus(SlotId slotId, DownloadStatus status,
        DownloadErrorCause cause) {
    }

    /**
     * This function is invoked when information about user consent and profile policy rules is
     * received. The client application is expected to provide user consent for download and
     * install profile by calling @ref telux::tel::ISimProfileManager::provideUserConsent if
     * user consent is expected.
     *
     * On platforms with access control enabled, the client needs to have TELUX_TEL_SIM_PROFILE_OPS
     * permission to invoke this API successfully.
     *
     * @param [in] slotId                   Slot on which profile get downloaded and installed.
     * @param [in] userConsentRequired      User consent required or not. If true it means user is
     *                                      expected to provide consent for download and install.
     * @param [in] mask                     @ref telux::tel::PolicyRuleMask (Profile policy rules Mask)
     *
     */
    virtual void onUserDisplayInfo(SlotId slotId, bool userConsentRequired,
        PolicyRuleMask mask) {
    }

    /**
     * This function is invoked when confirmation code is required. The client application
     * is expected to provide confirmation code for download and install profile by calling
     * @ref telux::tel::ISimProfileManager::provideConfirmationCode.
     *
     * On platforms with access control enabled, the client needs to have TELUX_TEL_SIM_PROFILE_OPS
     * permission to invoke this API successfully.
     *
     * @param [in] slotId                   Slot on which profile get downloaded and installed.
     * @param [in] profileName              Profile name corresponding to which confirmation code
     *                                      is required.
     *
     */
    virtual void onConfirmationCodeRequired(SlotId slotId, std::string profileName) {
    }

    /**
     * Destructor of ISimProfileListener
     */
    virtual ~ISimProfileListener() {
    }
};
/** @} */ /* end_addtogroup telematics_rsp */
}  // end of namespace tel

}  // end of namespace telux

#endif // TELUX_TEL_SIMPROFILELISTENER_HPP
