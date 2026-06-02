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
* @file       ModemConfigDefines.hpp
*
* @brief      ModemConfigDefines contains enumerations and variables used for
*             modem config subsystem.
*
*/

#include <telux/common/CommonDefines.hpp>

#ifndef TELUX_CONFIG_MODEMCONFIGDEFINES_HPP
#define TELUX_CONFIG_MODEMCONFIGDEFINES_HPP

#include <string>

namespace telux {

namespace config {
/** @addtogroup telematics_config_manager
 * @{ */

using ConfigId = std::string;

enum class ConfigType {
    HARDWARE, /**< For hardware or platform related configuration files */
    SOFTWARE, /**< For software or carrier related configuration files */
};

struct ConfigInfo {
    ConfigId id;  /**< stores the id of the configuration */
    ConfigType type; /**< stores config type */
    uint32_t size;   /**< stores the size of the configuration */
    std::string desc; /**< stores the configuration description */
    uint32_t version; /**< stores version of the config file */
};

/**
 * Selection Mode defines status of auto selection mode for configs.
 */
enum class AutoSelectionMode {
    DISABLED, /**<  Auto selection disabled  */
    ENABLED, /**<  Auto selection enabled*/
};

/**
 * ConfigUpdateStatus represent status of config update, a update of config happens when a
 * software config is activated and all segments using the config are updated with new config.
 */
enum class ConfigUpdateStatus {
    START, /**< start of updation process */
    COMPLETE, /**< end of updation process */
};


/** @} */ /* end_addtogroup telematics_config_manager */
}  // end of namespace config

}  // end of namespace telux

#endif // TELUX_CONFIG_MODEMCONFIGDEFINES_HPP
