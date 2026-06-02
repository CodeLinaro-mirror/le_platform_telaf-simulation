/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */


/**
 * @file       ConfigManager.hpp
 * @brief      Singleton class to update and retrieve the parameter configurations
 *             for an application dynamically. Also, notify the application in case of
 *             any updates in the configurations.
 */

#ifndef TELUX_CONFIG_CONFIGMANAGER_HPP
#define TELUX_CONFIG_CONFIGMANAGER_HPP

#include <map>
#include <memory>
#include <string>

#include <telux/common/CommonDefines.hpp>

namespace telux {

namespace config {
/** @addtogroup telematics_config_manager
 * @{ */

/**
 * @brief   IConfigListener interface is used to receive notifications related to
 *          any updates in the configurations dynamically.
 */
class IConfigListener : public common::IServiceStatusListener {
 public:
    /**
     * This API is invoked when there is any update in the configurations dynamically.
     *
     * @param [in] key - The key updated in the configurations.
     *
     * @param [in] value - The corresponding value for the key
     *                     that was updated in the configurations.
     *
     */
    virtual void onConfigUpdate(std::string key, std::string value) {}

    virtual ~IConfigListener() {}
};

/**
 * @brief   IConfigManager provides APIs to retrieve an instance of the manager,
 *          APIs for processes to update and retrieve configurations dynamically.
 */
class IConfigManager {
 public:
   /**
    * This status indicates whether the manager object is in a usable state or not.
    *
    * @returns SERVICE_AVAILABLE    -  if apps config manager is ready to use.
    *          SERVICE_UNAVAILABLE  -  if apps config manager is temporarily unavailable to use.
    *          SERVICE_FAILED       -  if apps config manager encountered an irrecoverable failure
    *                                  and can not be used.
    */
    virtual telux::common::ServiceStatus getServiceStatus() = 0;


   /**
    * This API is used to register a listener for getting the updates
    * when the configurations are updated dynamically.
    *
    * @param [in] listener - Pointer of object that processes the notification.
    *
    * @returns Status of registerForUpdates i.e success or suitable status code.
    *
    */
    virtual telux::common::Status registerListener(
        std::weak_ptr<IConfigListener> listener) = 0;

    /**
    * This API is used to deregister a listener from getting the updates
    * when the configurations are updated dynamically.
    *
    * @param [in] listener - Pointer of object that processes the notification.
    *
    * @returns Status of registerForUpdates i.e success or suitable status code.
    *
    */
    virtual telux::common::Status deregisterListener(
        std::weak_ptr<IConfigListener> listener) = 0;

   /**
    * This API is used to update the key and the corresponding value in the
    * configurations dynamically.
    *
    * On platforms with Access control enabled, if -
    * 1. /etc/tel.conf needs to be updated - caller needs to have TELUX_SET_GLOBAL_CONFIG
    *   permission to invoke this API successfully.
    * 2. App specific conf needs to be updated - caller needs to have TELUX_SET_LOCAL_CONFIG
    *   permission to invoke this API successfully.
    *
    * In order to update any configuration onto the file, all the permissions needed -
    * DAC permissions and sepolicy requirements, should be taken care by the application.
    *
    * The API does not perform any strict checking for the value being set.
    * Please refer to tel.conf for valid values for configuration items.
    *
    * @param [in] key - The key that needs to be updated in the configurations.
    *
    * @param [in] value - The corresponding value for the key that needs the update
    *                     in the configurations.
    *
    * @returns Status of setConfig i.e success or suitable status code.
    *
    */
    virtual telux::common::Status setConfig(const std::string key, const std::string value) = 0;

    /**
    * This API is used to retrieve the value for the corresponding key from the
    * configurations dynamically.
    *
    * @param [in] key - The key whose value is to be retrieved from the configurations.
    *
    * @returns The value for the key passed.
    *
    */
    virtual const std::string getConfig(const std::string key) = 0;

    /**
    * This API is used to retrieve all the configurations for the application at present.
    *
    * @returns A map of key-value pairs depicting all the application's configurations at present.
    *
    */
    virtual const std::map<std::string, std::string> getAllConfigs() = 0;

    /**
    * Destructor of IConfigManager
    */
    virtual ~IConfigManager() {};
};
/** @} */ /* end_addtogroup telematics_config_manager */
}  // end of namespace config

}  // end of namespace telux

#endif // TELUX_CONFIG_CONFIGMANAGER_HPP
