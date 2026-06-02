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
 * @file       ConfigFactory.hpp
 *
 * @brief      ConfigFactory allows creation of config related classes.
 */

#ifndef TELUX_CONFIG_CONFIGFACTORY_HPP
#define TELUX_CONFIG_CONFIGFACTORY_HPP

#include <memory>
#include <mutex>

#include <telux/config/ModemConfigManager.hpp>
#include <telux/config/ConfigManager.hpp>

namespace telux {

namespace config {
/** @addtogroup telematics_config_manager
 * @{ */

/**
 * @brief   ConfigFactory allows creation of config related classes.
 */
class ConfigFactory {
public:
    /**
     * Get instance of Config Factory
     */
    static ConfigFactory &getInstance();

    /**
     * Get instance of ModemConfig manager
     *
     * On platforms with Access control enabled, Caller needs to have TELUX_CONFIG_MODEM_CONFIG
     * permission to invoke this API successfully.
     *
     * @param[in] callback     Optional callback to get the response of Modem Config Manager
     *                         initialization.
     *
     * @returns pointer of IModemConfigManager object.
     */
    virtual std::shared_ptr<IModemConfigManager> getModemConfigManager(
        telux::common::InitResponseCb callback = nullptr) = 0;

    /**
     * Get instance of the Config manager
     *
     * On platforms with Access control enabled, Caller needs to have TELUX_CONFIG_APPS_CONFIG
     * permission to invoke this API successfully.
     *
     * @param[in] callback     Optional callback to get the response of Config Manager
     *                         initialization.
     *
     * @returns pointer of IConfigManager object.
     */
    virtual std::shared_ptr<IConfigManager> getConfigManager(
        telux::common::InitResponseCb callback = nullptr) = 0;

#ifndef TELUX_DOXY_SKIP
protected:
    ConfigFactory();
    virtual ~ConfigFactory();
#endif

private:
    ConfigFactory(const ConfigFactory &) = delete;
    ConfigFactory &operator=(const ConfigFactory &) = delete;
};

/** @} */ /* end_addtogroup telematics_config_manager */
} // end of namespace config

} // end of namespace telux

#endif // TELUX_CONFIG_CONFIGFACTORY_HPP

