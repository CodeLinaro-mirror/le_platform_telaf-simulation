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
 * @file       SimProfileDefines.hpp
 * @brief      SimProfileDefines provides the enumerations required for eUICC operations.
 */

#ifndef TELUX_TEL_SIMPROFILEDEFINES_HPP
#define TELUX_TEL_SIMPROFILEDEFINES_HPP

#include <bitset>
#include <vector>

#include <telux/common/CommonDefines.hpp>

namespace telux {
namespace tel {

/** @addtogroup telematics_rsp
 * @{ */

/**
 * Indicates profile type of card
 */
enum class ProfileType {
    UNKNOWN = -1,
    REGULAR = 0, /**< Regular profile */
    EMERGENCY,   /**< Emergency profile */
};

/**
 * Indicates profile icon type.
 */
enum class IconType {
    NONE = 0, /**< No icon information */
    JPEG,     /**< JPEG icon */
    PNG,      /**< PNG icon */
};

/**
 * Indicates profile class.
 */
enum class ProfileClass {
    UNKNOWN = -1, /**< No info about profile class */
    TEST = 0,     /**< Test profile */
    PROVISIONING, /**< Provisioning profile */
    OPERATIONAL   /**< Operational  profile */
};

/**
 * Indicates profile download status.
 */
enum class DownloadStatus {
    DOWNLOAD_ERROR = 0,                  /**< Profile download error */
    DOWNLOAD_INSTALLATION_COMPLETE,      /**< Profile download and installation is complete */
};

/**
 * Indicates profile download error cause.
 */
enum class DownloadErrorCause {
    GENERIC = 1,                      /**< Generic error */
    SIM,                              /**< Error from the SIM card */
    NETWORK,                          /**< Error from the network */
    MEMORY,                           /**< Error due to no memory */
    UNSUPPORTED_PROFILE_CLASS,        /**< Unsupported profile class */
    PPR_NOT_ALLOWED,                  /**< Profile policy rules not allowed */
    END_USER_REJECTION,               /**< End user rejection */
    END_USER_POSTPONED,               /**< End user postponed */
};

/**
 * Indicates the reason for user consent not provided.
 */
enum class UserConsentReasonType {
    END_USER_REJECTION = 0,            /**< End user rejection */
    END_USER_POSTPONED,                /**< End user postponed */
};

/**
 * Defines profile policy rules(PPR). Each value represents
 * corresponding bit for PprMask bitset.
 */
enum PolicyRuleType {
    PROFILE_DISABLE_NOT_ALLOWED, /**< Disabling of the profile is not allowed */
    PROFILE_DELETE_NOT_ALLOWED,  /**< Deletion of the profile is not allowed */
    PROFILE_DELETE_ON_DISABLE,   /**< Deletion of the profile is required on successful
                                    disabling */
};

/**
 * 16 bit mask that denotes which of the profile policy rules(PPR) defined in
 * Policy Rule Type enum are used.
 */
using PolicyRuleMask = std::bitset<16>;

/**
 * Defines memory reset options. Each value represents
 * corresponding bit for ResetOptionMask bitset.
 */
enum ResetOption {
    TEST_PROFILES = (1 << 0),           /**< Delete all the test profiles */
    OPERATIONAL_PROFILE = (1 << 1),     /**< Delete all operational profiles */
    DEFAULT_SMDP_ADDRESS = (1 << 2),    /**< Reset the default SM-DP+ address */
};

/**
 * 64 bit mask that denotes which of the reset options defined in
 * @ ref ResetOption enum are used.
 */
using ResetOptionMask = std::bitset<64>;

/** @} */ /* end_addtogroup telematics_rsp */
}
}

#endif // TELUX_TEL_SIMPROFILEDEFINES_HPP
