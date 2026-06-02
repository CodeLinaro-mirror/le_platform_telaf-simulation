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
 * @file       ModemConfigManager.hpp
 * @brief      ModemConfigManager provides APIs to get list of config files present in
 *             modem's storage, load a new config file in modem's storage, activate a
 *             config file, get active config file information, deactivate a config file,
 *             remove config file from the modem's storage, get and set mode of config
 *             auto selection, register and deregister listener for config update in modem.
 */

#include <vector>
#include <future>

#ifndef TELUX_CONFIG_MODEMCONFIGMANAGER_HPP
#define TELUX_CONFIG_MODEMCONFIGMANAGER_HPP

#include <telux/common/CommonDefines.hpp>
#include <telux/config/ModemConfigDefines.hpp>
#include <telux/config/ModemConfigListener.hpp>

namespace telux {

namespace config {
/** @addtogroup telematics_config_manager
 * @{ */

/**
 * This function is called as a response to @ref requestConfigList().
 *
 * @param [in] configList  -  Contains the list of config files in modem's storage.
 * @param [in] error       -  Return code which indicates whether the operation
 *                            succeeded or not.
 *                            @ref ErrorCode
 */
using ConfigListCallback = std::function<void (std::vector<ConfigInfo> configList,
                                    telux::common::ErrorCode error)>;

/**
 * This function is called as a response to @ref getAutoSelectionMode().
 *
 * @param [in] mode           -  contains status of auto selection for configs.
 * @param [in] error          -  Return code which indicates whether the operation
 *                               succeeded or not.
 *                               @ref ErrorCode
 */
using GetAutoSelectionModeCallback = std::function<void (AutoSelectionMode mode,
                                    telux::common::ErrorCode error)>;

/**
 * This function is called as a response to @ref getActiveConfig().
 *
 * @param [in] configInfo     -  Information of active config file for specified slot id.
 * @param [in] error          -  Return code which indicates whether the operation
 *                               succeeded or not.
 *                               @ref ErrorCode
 */
using GetActiveConfigCallback = std::function<void (ConfigInfo configInfo,
                                    telux::common::ErrorCode error)>;

/**
 * @brief   IModemConfigManager provides interface to list config files present in modem's storage.
 *          load a new config file in modem, activate a config file, get active config file
 *          information, deactivate a config file, delete config file from the modem's storage,
 *          get and set mode of config auto selection, register and deregister listener
 *          for config update in modem. The config files are also referred to as MBNs.
 */
class IModemConfigManager {
public:

   /**
    * Checks the status of modem config subsystem and returns the result.
    *
    * @returns    If true that means ModemConfigManager is ready for performing config operations.
    *
    * @deprecated Use @ref getServiceStatus API
    */
    virtual bool isSubsystemReady() = 0;

   /**
    * This status indicates whether the manager object is in a usable state or not.
    *
    * @returns SERVICE_AVAILABLE    -  if modem config manager is ready to use.
    *          SERVICE_UNAVAILABLE  -  if modem config manager is temporarily unavailable to use.
    *          SERVICE_FAILED       -  if modem config manager encountered an irrecoverable failure
    *                                  and can not be used.
    */
    virtual telux::common::ServiceStatus getServiceStatus() = 0;

   /**
    * Wait for modem config subsystem to be ready.
    *
    * @returns    A future that caller can wait on to be notified when modem config
    *             subsystem is ready.
    *
    * @deprecated Use InitResponseCb callback in factory API @ref ConfigFactory::getModemConfigManager.
    */
    virtual std::future<bool> onSubsystemReady() = 0;

   /**
    * Fetching the list of config files present in modem's storage.
    *
    * @param [in] cb - callback to the Response function.
    *
    * returns SUCCESS if the request to get config list is sent successfully.
    */
    virtual telux::common::Status requestConfigList(ConfigListCallback cb) = 0;

  /**
    * Loads a new config file into the modem's storage. This is a persistent operation.
    * Only the config files loaded into the modem's storage can be activated.
    *
    * @param [in] filePath    - it defines the path to the config file.
    * @param [in] configType  - type of the config file.
    * @param [in] cb          - callback to the response function.
    *
    * returns SUCCESS if the request to load config file is sent successfully.
    */
    virtual telux::common::Status loadConfigFile(std::string filePath, ConfigType configType,
            telux::common::ResponseCallback cb = nullptr) = 0;

    /**
    * Activates the config file on specified slot id. A file for activation must be loaded or
    * should already be present in modem's storage.
    *
    * @param [in] configType      - type of the config file.
    * @param [in] configId        - id of the config file.
    * @param [in] slotId          - it defines the slot id to be selected.
    * @param [in] cb              - callback to the response function.
    *
    * @returns SUCCESS if the request to activate config file is sent successfully.
    */
    virtual telux::common::Status activateConfig(ConfigType configType, ConfigId configId,
            int slotId = DEFAULT_SLOT_ID, telux::common::ResponseCallback cb = nullptr) = 0;

    /**
    * Get the currently active config file information for the specified slot id. In case
    * default config files are activated, would return error.
    *
    * @param [in] configType   - type of the config file.
    * @param [in] cb           - callback to the response function.
    * @param [in] slotId       - it defines the slot id to be selected.
    *
    * @returns SUCCESS if the request to get active config information is sent successfully.
    */
    virtual telux::common::Status getActiveConfig(ConfigType configType,
            GetActiveConfigCallback cb, int slotId = DEFAULT_SLOT_ID) = 0;

   /**
    * Deactivates the config file for the specified slot id.
    *
    * @param [in] configType   - type of the config file.
    * @param [in] slotId       - slot id to be selected for deactivation of config.
    * @param [in] cb           - callback to the response function.
    *
    * @returns SUCCESS if the request to deactivate config file is sent successfully
    */
    virtual telux::common::Status deactivateConfig(ConfigType configType,
            int slotId = DEFAULT_SLOT_ID, telux::common::ResponseCallback cb = nullptr) = 0;

   /**
    * Deletes the config file from the modem's storage.
    *
    * @param [in] configType      - type of the config file.
    * @param [in] configId        - id of the config file. This parameter is optional if not
    *                               provided all the config files of the given config type are
    *                               deleted from modem's storage.
    * @param [in] cb              - callback to the Response function.
    *
    * @returns SUCCESS if the request to delete config file is sent successfully
    */
    virtual telux::common::Status deleteConfig(ConfigType configType,
            ConfigId configId = "", telux::common::ResponseCallback cb = nullptr) = 0;

    /**
    * Fetching the mode of config auto selection for specified slot id.
    *
    * @param [in] cb        - callback to the response function.
    * @param [in] slotId    - slot id of config.
    *
    * @returns SUCCESS if the request to get selection mode is sent successfully
    */
    virtual telux::common::Status getAutoSelectionMode(
            GetAutoSelectionModeCallback cb, int slotId = DEFAULT_SLOT_ID) = 0;

    /**
    * Setting the mode of config auto selection for specified slot id.
    *
    * @param [in] mode            - auto selection mode status.
    * @param [in] slotId          - slot id of the config.
    * @param [in] cb              - callback to the response function.
    *
    * @returns SUCCESS if the request to set selection mode is sent successfully.
    */
    virtual telux::common::Status setAutoSelectionMode(AutoSelectionMode mode,
            int slotId = DEFAULT_SLOT_ID, telux::common::ResponseCallback cb = nullptr) = 0;

    /**
    * Registeres the listener for indications.
    *
    * @param [in] listener      - pointer to implemented listener.
    *
    * @returns SUCCESS if the request to register listener is sent successfully.
    */
    virtual telux::common::Status registerListener(
            std::weak_ptr<IModemConfigListener> listener) = 0;

    /**
    * Deregisteres the listener from indications.
    *
    * @param [in] listener      - pointer to registered listener.
    *
    * @returns SUCCESS if the request to deregister listener is sent successfully.
    */
    virtual telux::common::Status deregisterListener(
            std::weak_ptr<IModemConfigListener> listener) = 0;

    /**
    * Destructor of IModemConfigManager
    */
    virtual ~IModemConfigManager() {};
};

/** @} */ /* end_addtogroup telematics_config_manager */
} // end of namespace config

} // end of namespace telux

#endif // TELUX_CONFIG_MODEMCONFIGMANAGER_HPP





