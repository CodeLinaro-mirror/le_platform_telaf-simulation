/*
 *  Copyright (c) 2019-2021, The Linux Foundation. All rights reserved.
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
 * @file       SimProfileManager.hpp
 *
 * @brief      SimProfileManager is a primary interface for remote eUICCs (eSIMs or embedded SIMs)
 *             provisioning.This interface provides APIs to add, delete, set profile,
 *             request profile list, update nickname, providing user consent, get Eid on the eUICC.
 *
 * @note This class is not intended for production software and is intended for testing and trial
 * purposes only.
 */

#ifndef TELUX_TEL_SIMPROFILEMANAGER_HPP
#define TELUX_TEL_SIMPROFILEMANAGER_HPP

#include <future>
#include <memory>

#include <telux/common/CommonDefines.hpp>

#include <telux/tel/SimProfileListener.hpp>
#include <telux/tel/SimProfile.hpp>

namespace telux {
namespace tel {

/** @addtogroup telematics_rsp
 * @{ */

/**
 * This function is called with the response to requestProfileList API.
 *
 * The callback can be invoked from multiple different threads.
 * The implementation should be thread safe.
 *
 * @param [in] info       Profiles information @ref telux::tel::SimProfile.
 * @param [in] error      Return code which indicates whether the operation
 *                        succeeded or not.  @ref telux::common::ErrorCode.
 */
using ProfileListResponseCb = std::function<void(
    const std::vector<std::shared_ptr<SimProfile>> &profiles, telux::common::ErrorCode error)>;

/**
 * This function is called with the response to requestEid API.
 *
 * The callback can be invoked from multiple different threads.
 * The implementation should be thread safe.
 *
 * @param [in] eid        eUICC identifier.
 * @param [in] error      Return code which indicates whether the operation
 *                        succeeded or not.  @ref telux::common::ErrorCode.
 */
using EidResponseCb = std::function<void(std::string eid, telux::common::ErrorCode error)>;

/**
 * This function is called with the response to requestServerAddress API.
 *
 * The callback can be invoked from multiple different threads.
 * The implementation should be thread safe.
 *
 * @param [in] smdpAddress        Configured SM-DP+ address on the eUICC.
 * @param [in] smdsAddress        Configured SMDS address on the eUICC.
 * @param [in] error              Return code which indicates whether the operation
 *                                succeeded or not.  @ref telux::common::ErrorCode.
 */
using ServerAddressResponseCb = std::function<void(std::string smdpAddress,
    std::string smdsAddress, telux::common::ErrorCode error)>;

/**
 *@brief ISimProfileManager is a primary interface for remote eUICCs (eSIMs or embedded SIMs)
 * provisioning.This interface provides APIs to add, delete, set profile, update nickname,
 * provide user consent, get Eid on the eUICC.
 */
class ISimProfileManager {
 public:
    /**
     * Checks if the eUICC subsystem is ready.
     *
     * @deprecated Use ISimProfileManager::getServiceStatus() API.
     *
     * @returns True if ISimProfileManager is ready for service, otherwise returns false.
     */
    virtual bool isSubsystemReady() = 0;

    /**
     * Wait for eUICC subsystem to be ready.
     *
     * @deprecated Use InitResponseCb in PhoneFactory::getSimProfileManager instead,
     *             to get notified about subsystem readiness.
     *
     * @returns A future that caller can wait on to be notified when card manager is ready.
     */
    virtual std::future<bool> onSubsystemReady() = 0;

    /**
    * This status indicates whether the ISimProfileManager object is in a usable state.
    *
    * @returns SERVICE_AVAILABLE    - If SimProfile Manager is ready for service.
    *          SERVICE_UNAVAILABLE  - If SimProfile Manager is temporarily unavailable.
    *          SERVICE_FAILED       - If SimProfile Manager encountered an irrecoverable
    *                                 failure.
    *
    *
    */
    virtual telux::common::ServiceStatus getServiceStatus() = 0;

    /**
     * Add new profile to eUICC card and download and install the profile on eUICC.
     *
     * On platforms with access control enabled, caller needs to have TELUX_TEL_SIM_PROFILE_OPS
     * permission to invoke this API successfully.
     *
     * @param [in] slotId                Slot identifier corresponding to the card.
     * @param [in] activationCode        Activation code.
     * @param [in] confirmationCode      Optional confirmation code required for downloading the
     *                                   profile.
     * @param [in] userConsentSupported  Optional User consent supported or not.
     * @param [in] callback              Optional callback function to get the result of add
     *                                   profile.
     *
     * @returns Status of add profile i.e. success or suitable error code.
     */
    virtual telux::common::Status addProfile(SlotId slotId,
        const std::string &activationCode, const std::string &confirmationCode = "",
        bool userConsentSupported = false, common::ResponseCallback callback = nullptr)
        = 0;

    /**
     * Delete profile from eUICC card.
     * 1. Deletion of enabled profile
     *    a) This API will disable the profile first and then delete it.
     *    b) The profile is associated with profile policy rules(PPRs) so before
     *       disabling the profile, this API checks if the PPRs
     *       @ref telux::tel::PolicyRuleType allow the operation.
     *    c) If the policy rules are not set, then first disabling of profile happens
     *       followed by deletion of profile.
     *    d) If disable succeeds but deletion fails, then the API attempts
     *       to roll back the profile back to the original (enabled) state.
     *    e) If rollback fails due to any reason such as eUICC being in incompatabile
     *       state then the profile will be in disabled state and the API will return
     *       telux::common::ErrorCode::ROLLBACK_FAILED
     * 2. Deletion of disabled profile
     *     a) This API checks the PPR @ref telux::tel::PolicyRuleType::PROFILE_DELETE_NOT_ALLOWED
     *        before deletion of profile.
     *     b) If the PPR is not set, then deletion of profile is performed.
     *        If the PPR is set, then the API returns
     *        telux::common::ErrorCode::OPERATION_NOT_ALLOWED.
     *
     * On platforms with access control enabled, caller needs to have TELUX_TEL_SIM_PROFILE_OPS
     * permission to invoke this API successfully.
     *
     * @param [in] slotId            Slot identifier corresponding to the card.
     * @param [in] profileId         Profile identifier
     * @param [in] callback          Optional callback function to get the result of delete
     *                               profile.
     *
     * @returns Status of delete profile i.e. success or suitable error code.
     */
    virtual telux::common::Status deleteProfile(SlotId slotId, int profileId,
        common::ResponseCallback callback = nullptr)
        = 0;

    /**
     * Enable or disable profile which allows to switch to other profile on eUICC card.
     *
     * On platforms with access control enabled, caller needs to have TELUX_TEL_SIM_PROFILE_OPS
     * permission to invoke this API successfully.
     *
     * @param [in] slotId            Slot identifier corresponding to the card.
     * @param [in] profileId         Profile identifier.
     * @param [in] enable            Indicates whether a profile must be enabled or disabled.
     *                               true - Enable and false - Disable.
     * @param [in] callback          Optional callback function to get the result of set profile.
     *
     * @returns Status of set profile i.e. success or suitable error code.
     */
    virtual telux::common::Status setProfile(SlotId slotId, int profileId, bool enable = false,
        common::ResponseCallback callback = nullptr)
        = 0;

    /**
     * Update nick name of the profile.
     *
     * On platforms with access control enabled, caller needs to have TELUX_TEL_SIM_PROFILE_OPS
     * permission to invoke this API successfully.
     *
     * @param [in] slotId          Slot identifier corresponding to the card.
     * @param [in] profileId       Profile identifier
     * @param [in] nickName        New nick name for profile.
     * @param [in] callback        Optional callback function to get the result of update nickname.
     *
     * @returns Status of update nick name i.e. success or suitable error code.
     */
    virtual telux::common::Status updateNickName(SlotId slotId, int profileId,
        const std::string &nickName, common::ResponseCallback callback = nullptr)
        = 0;

    /**
     * Request list of profiles supported by the eUICC card.
     *
     * On platforms with access control enabled, caller needs to have TELUX_TEL_SIM_PROFILE_READ
     * permission to invoke this API successfully.
     *
     * @param [in] slotId            Slot identifier corresponding to the card.
     * @param [in] callback          Callback function to get the result of request profile list.
     *
     * @returns  Status of request profile list i.e. success or suitable error code.
     */
    virtual telux::common::Status requestProfileList(SlotId slotId, ProfileListResponseCb callback)
        = 0;

    /**
     * Request eUICC identifier(EID) for the slot.
     *
     * On platforms with access control enabled, caller needs to have TELUX_TEL_SIM_PROFILE_READ
     * permission to invoke this API successfully.
     *
     * @param [in] slotId            Slot identifier corresponding to the card.
     * @param [in] callback          Callback function to get the result of request EID.
     *
     * @returns  Status of request EID i.e. success or suitable error code.
     *
     * @deprecated Use telux::tel::ICard::requestEid API instead
     */
    virtual telux::common::Status requestEid(SlotId slotId, EidResponseCb callback)
        = 0;

    /**
     * Provide user consent required for downloading and installing profile.
     * This API should be called in response to
     * @ref telux::tel::ISimProfileListener::onUserDisplayInfo.
     *
     * On platforms with access control enabled, caller needs to have
     * TELUX_TEL_SIM_PROFILE_USER_CONSENT permission to invoke this API successfully.
     *
     * @param [in] slotId            Slot identifier corresponding to the card.
     * @param [in] userConsent       Consent for proﬁle download and install.
                                     True means user consent given to download and install.
     * @param [in] reason            Reason for not providing user consent to download and install.
     *                               @ref telux::tel::UserConsentReasonType
     * @param [in] callback          Optional callback function to get the result of user consent
     *                               request.
     *
     * @returns  Status of user consent request i.e. success or suitable error code.
     */
    virtual telux::common::Status provideUserConsent(SlotId slotId, bool userConsent,
        UserConsentReasonType reason, common::ResponseCallback callback = nullptr)
        = 0;

    /**
     * Provide confirmation code required for downloading and installing profile. This API should be
     * called in response to @ref telux::tel::ISimProfileListener::onConfirmationCodeRequired.
     *
     * On platforms with access control enabled, caller needs to have TELUX_TEL_SIM_PROFILE_OPS
     * permission to invoke this API successfully.
     *
     * @param [in] slotId            Slot identifier corresponding to the card.
     * @param [in] code              Confirmation code for profile download and install.
     * @param [in] callback          Optional callback function to get the result of confirmation
     *                               request.
     *
     * @returns  Status of provide confirmation code i.e. success or suitable error code.
     */
    virtual telux::common::Status provideConfirmationCode(SlotId slotId, std::string code,
        common::ResponseCallback callback = nullptr)
        = 0;

    /**
     * Get Subscription Manager Data Preparation (SM-DP+) address and the Subscription Manager
     * Discovery Server (SMDS) address configured on the eUICC.
     *
     * On platforms with access control enabled, caller needs to have TELUX_TEL_SIM_PROFILE_READ
     * permission to invoke this API successfully.
     *
     * @param [in] slotId            Slot identifier corresponding to the card.
     * @param [in] callback          Callback function to get the result of server address request.
     *
     * @returns  Status of server address request i.e. success or suitable error code.
     */
    virtual telux::common::Status requestServerAddress(SlotId slotId,
        ServerAddressResponseCb callback) = 0;

    /**
     * Set Subscription Manager Data Preparation (SM-DP+) address on the eUICC. If SMDP+
     * address length is zero then the existing SM-DP+ address on the eUICC is removed.
     *
     * On platforms with access control enabled, caller needs to have TELUX_TEL_SIM_PROFILE_CONFIG
     * permission to invoke this API successfully.
     *
     * @param [in] slotId            Slot identifier corresponding to the card.
     * @param [in] smdpAddress       SM-DP+ address to be configured on the eUICC.
     * @param [in] callback          Optional Callback function to get the result of set
     *                               SM-DP+ request.
     * @returns  Status of set server address request i.e. success or suitable error code.
     */
    virtual telux::common::Status setServerAddress(SlotId slotId, const std::string &smdpAddress,
        common::ResponseCallback callback = nullptr) = 0;

    /**
     * Resets the memory of the eUICC card based on @ref telux::tel::ResetOptionMask.
     *
     * On platforms with access control enabled, caller needs to have TELUX_TEL_SIM_PROFILE_OPS
     * permission to invoke this API successfully.
     *
     * @param [in] slotId            Slot identifier corresponding to the card.
     * @param [in] mask              Memory reset options mask @ref telux::tel::ResetOptionMask
     * @param [in] callback          Optional Callback function to get the result of memory
     *                               reset request.
     * @returns  Status of memory reset request i.e. success or suitable error code.
     *
     */
    virtual telux::common::Status memoryReset(SlotId slotId, ResetOptionMask mask,
        common::ResponseCallback callback = nullptr) = 0;

    /**
     * Register a listener to listen for status of specific events like download and installation
     * of profile on eUICC.
     *
     * @param [in] listener    Pointer of ISimProfileListener object that processes the
     * notification.
     *
     * @returns Status of registerListener success or suitable status code
     */
    virtual telux::common::Status registerListener(std::weak_ptr<ISimProfileListener> listener) = 0;

    /**
     * De-register the listener.
     *
     * @param [in] listener    Pointer of ISimProfileListener object that needs to be removed
     *
     * @returns Status of deregisterListener success or suitable status code
     */
    virtual telux::common::Status deregisterListener(std::weak_ptr<ISimProfileListener> listener)
        = 0;

    /**
     * Destructor for ISimProfileManager
     */
    virtual ~ISimProfileManager() {
    }
};  // end of ISimProfileManager

/** @} */ /* end_addtogroup telematics_rsp */
}
}

#endif // TELUX_TEL_SIMPROFILEMANAGER_HPP
