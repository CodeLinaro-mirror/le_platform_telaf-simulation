/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file       SimProfile.hpp
 * @brief      This is a container class represents single eUICC profile on the card.
 *
 */

#ifndef TELUX_TEL_SIMPROFILE_HPP
#define TELUX_TEL_SIMPROFILE_HPP

#include <vector>
#include <string>

#include <telux/tel/SimProfileDefines.hpp>
#include <cstdint>

namespace telux {
namespace tel {

/** @addtogroup telematics_rsp
 * @{ */

/**
 * @brief  SimProfile class represents single eUICC profile on the card.
 */
class SimProfile {
 public:

    SimProfile(int profileId, ProfileType profileType, const std::string &iccid, bool isActive,
        const std::string &nickName, const std::string &spn, const std::string &name,
        IconType iconType, std::vector<uint8_t> icon, ProfileClass profileClass,
        PolicyRuleMask policyRuleMask, int slotId = DEFAULT_SLOT_ID);

    /**
     * Get slot id associated for this profile
     *
     * @returns SlotId
     */
    int getSlotId();

    /**
     * Get profile identifier. The profile identifier is not persistently unique. It is
     * unique for given snapshot of SIM profiles state. The profile identifier could
     * change when any profile is deleted and added.
     *
     * @returns unique identifier for the profile
     */
    int getProfileId();

    /**
     * Get profile Type.
     *
     * @returns profile type
     */
    ProfileType getType();

    /**
     * Get profile ICCID.
     *
     * @returns profile ICCID coded as in EF-ICCID
     */
    const std::string &getIccid();

    /**
     * Indicates the profile state whether active or not.
     *
     * @returns true if profile is Active
     */
    bool isActive();

    /**
     * Get profile nick name.
     *
     * @returns profile nick name
     */
    const std::string &getNickName();

    /**
     * Get profile service provider name.
     *
     * @returns profile service provider name.
     */
    const std::string &getSPN();

    /**
     * Get profile name.
     *
     * @returns profile name
     */
    const std::string &getName();

    /**
     * Get profile icon type.
     *
     * @returns profile icon type
     */
    IconType getIconType();

    /**
     * Get profile icon content.
     *
     * @returns profile icon content
     */
    std::vector<uint8_t> getIcon();

    /**
     * Get profile class.
     *
     * @returns profile class
     */
    ProfileClass getClass();

    /**
     * Get profile policy rules.
     *
     * @returns mask of profile policy rules
     */
    PolicyRuleMask getPolicyRule();

    /**
     * Get the text related informative representation of this object.
     *
     * @returns String containing informative string.
     *
     */
    std::string toString();

 private:
    int profileId_;
    ProfileType profileType_;
    std::string iccid_;
    bool isActive_;
    std::string nickName_;
    std::string spn_;
    std::string name_;
    IconType iconType_;
    std::vector<uint8_t> icon_;
    ProfileClass profileClass_;
    PolicyRuleMask policyRuleMask_;
    int slotId_;
};

/** @} */ /* end_addtogroup telematics_rsp */
}
}

#endif // TELUX_TEL_SIMPROFILE_HPP
