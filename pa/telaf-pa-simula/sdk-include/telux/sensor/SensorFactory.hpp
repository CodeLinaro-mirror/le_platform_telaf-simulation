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
 * @file       SensorFactory.hpp
 *
 * @brief      SensorFactory is the central factory to create all sensor object instances
 *
 */

#ifndef TELUX_SENSOR_SENSORFACTORY_HPP
#define TELUX_SENSOR_SENSORFACTORY_HPP

#include <memory>

#include <telux/common/CommonDefines.hpp>
#include <telux/sensor/SensorDefines.hpp>
#include <telux/sensor/SensorManager.hpp>
#include <telux/sensor/SensorFeatureManager.hpp>

namespace telux {
namespace sensor {

/** @addtogroup telematics_sensor_service
 * @{ */

/**
 *@brief SensorFactory is the central factory to create instances of sensor objects
 *
 */
class SensorFactory {
 public:
    /**
     * Get Sensor Factory instance.
     *
     * @returns The singleton instance of SensorFactory object
     *
     */
    static SensorFactory &getInstance();

    /**
     * Get an instance of Sensor Manager. The ownership of the returned object is with the caller of
     * this method. The reference to the instance is not held by the SensorFactory. If the returned
     * reference is released, any request for ISensorManager shall result in creation of a new
     * instance
     *
     * @param [in] clientCallback  Optional callback to get the initialization status of
     *                             SensorManager
     *                             @ref telux::common::InitResponseCb
     *
     * @returns                    An instance of ISensorManager
     *                             If the initialization of the manager and underlying system fails,
     *                             nullptr is returned
     *
     */
    virtual std::shared_ptr<ISensorManager> getSensorManager(
        telux::common::InitResponseCb clientCallback = nullptr) = 0;

    /**
     * Get an instance of Sensor Feature Manager. The ownership of the returned object is with the
     * caller of this method. The reference to the instance is not held by the SensorFactory. If the
     * returned reference is released, any request for ISensorFeatureManager shall result in
     * creation of a new instance
     *
     * @param [in] clientCallback  Optional callback to get the initialization status of
     *                             SensorFeatureManager
     *                             @ref telux::common::InitResponseCb
     *
     * @returns                    An instance of ISensorFeatureManager
     *                             If the initialization of the manager and underlying system fails,
     *                             nullptr is returned
     *
     */
    virtual std::shared_ptr<ISensorFeatureManager> getSensorFeatureManager(
        telux::common::InitResponseCb clientCallback = nullptr) = 0;

#ifndef TELUX_DOXY_SKIP
 protected:
    SensorFactory();
    virtual ~SensorFactory();
#endif

 private:
    SensorFactory(const SensorFactory &) = delete;
    SensorFactory &operator=(const SensorFactory &) = delete;
};

/** @} */ /* end_addtogroup telematics_sensor_service */
}  // namespace sensor
}  // namespace telux

#endif // TELUX_SENSOR_SENSORFACTORY_HPP
