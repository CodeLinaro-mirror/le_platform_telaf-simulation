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
 * @file       SensorManager.hpp
 *
 * @brief      Sensor Manager class provides the APIs to interact with the sensors service.
 */

#ifndef TELUX_SENSOR_SENSORMANAGER_HPP
#define TELUX_SENSOR_SENSORMANAGER_HPP

#include <memory>

#include <telux/common/CommonDefines.hpp>
#include <telux/sensor/Sensor.hpp>
#include <telux/sensor/SensorDefines.hpp>

namespace telux {
namespace sensor {

/** @addtogroup telematics_sensor_control
 * @{ */

/**
 * @brief   Sensor Manager class provides APIs to interact with the sensor sub-system and get access
 *          to other sensor objects which can be used to configure, activate or get data from the
 *          individual sensors available - Gyro, Accelero, etc.
 */
class ISensorManager {
 public:
    /**
     * Checks the status of sensor sub-system and returns the result.
     *
     * @returns the status of sensor sub-system status @ref telux::common::ServiceStatus
     *
     */
    virtual telux::common::ServiceStatus getServiceStatus() = 0;

    /**
     * Get information related to the sensors available in the system.
     *
     * @param [out] info    List of information on sensors available in the system
     *                      @ref telux::sensor::SensorInfo
     *
     * @returns             status of the request @ref telux::common::Status
     *
     */
    virtual telux::common::Status getAvailableSensorInfo(std::vector<SensorInfo> &info) = 0;

    /**
     * Get an instance of ISensorClient to interact with the underlying sensor.
     * The provided instance is not a singleton. Everytime this method is called a new sensor
     * object is created. It is the caller's responsibility to manage the object's lifetime.
     * Every instance of the sensor returned acts as new client and can configure the underlying
     * sensor with it's own configuration and it's own callbacks for
     * @ref telux::sensor::SensorEvent and configuration update among other events
     * @ref telux::sensor::ISensorEventListener.
     *
     * @param [out] sensor -    An instance of @ref telux::sensor::ISensorClient to interact with
     *                          the underlying sensor is provided as a result of the method
     *                          If the initialization of the sensor and underlying system
     *                          fails, sensor is set to nullptr
     *
     * @param [in]  name -      The unique name of the sensor @ref telux::sensor::SensorInfo::name
     *                          that was provided in the list of sensor information by
     *                          @ref telux::sensor::ISensorManager::getAvailableSensorInfo
     *
     * @returns                 Status of request @ref telux::common::Status
     *
     * @deprecated Use getSensorClient API.
     *
     */
    virtual telux::common::Status getSensor(
        std::shared_ptr<ISensorClient> &sensor, std::string name) = 0;

    /**
     * Get an instance of ISensorClient to interact with the underlying sensor.
     * The provided instance is not a singleton. Everytime this method is called a new sensor
     * object is created. It is the caller's responsibility to manage the object's lifetime.
     * Every instance of the sensor returned acts as new client and can configure the underlying
     * sensor with it's own configuration and it's own callbacks for
     * @ref telux::sensor::SensorEvent and configuration update among other events
     * @ref telux::sensor::ISensorEventListener.
     *
     * @param [out] sensor -    An instance of @ref telux::sensor::ISensorClient to interact with
     *                          the underlying sensor is provided as a result of the method
     *                          If the initialization of the sensor and underlying system
     *                          fails, sensor is set to nullptr
     *
     * @param [in]  name -      The unique name of the sensor @ref telux::sensor::SensorInfo::name
     *                          that was provided in the list of sensor information by
     *                          @ref telux::sensor::ISensorManager::getAvailableSensorInfo
     *
     * @returns                 Status of request @ref telux::common::Status
     *
     */
    virtual telux::common::Status getSensorClient(
        std::shared_ptr<ISensorClient> &sensor, std::string name) = 0;

    /**
     * This API is called to set Euler angles, used for sensor rotation matrix.
     *
     * The sensor data should always be obtained w.r.t the vehicular frame. This API accepts the
     * Euler angles which are used to compute the rotational matrix and provide the final rotated
     * sensor data to the clients. It is advised to set the Euler angles by calling this API before
     * activating any sensor clients.
     *
     *
     * On platforms with access control enabled, caller needs to have TELUX_SENSOR_PRIVILEGED_OPS
     * permission to invoke this API successfully.
     *
     *  @param[in]   eulerAngleConfig - The Euler angle configuration.
     *
     * @returns status of Euler angle update request - @ref telux::common::Status
     *
     */
    virtual telux::common::Status setEulerAngleConfig(
        EulerAngleConfig eulerAngleConfig) = 0;

    /**
     * Destructor for ISensorManager
     */
    virtual ~ISensorManager(){};
};

/** @} */ /* end_addtogroup telematics_sensor_control */
}  // namespace sensor
}  // namespace telux

#endif // TELUX_SENSOR_SENSORMANAGER_HPP
