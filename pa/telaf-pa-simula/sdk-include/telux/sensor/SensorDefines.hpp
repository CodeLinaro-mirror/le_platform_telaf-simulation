/*
 * Copyright (c) 2021, The Linux Foundation. All rights reserved.
 *
 * Not a contribution.
 *
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/*
 * Changes from Qualcomm Technologies, Inc. are provided under the following license:
 *
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file       SensorDefines.hpp
 * @brief      SensorDefines contains enumerations and variables used for sensor services
 *
 */

#ifndef TELUX_SENSOR_SENSORDEFINES_HPP
#define TELUX_SENSOR_SENSORDEFINES_HPP

#include <bitset>
#include <string>
#include <vector>

namespace telux {
namespace sensor {

/** @addtogroup telematics_sensor_service
 * @{ */

/**
 * @brief Enumeration of different sensors available
 */
enum class SensorType {
    /** ID for the accelerometer sensor */
    ACCELEROMETER = 1,
    /** ID for the gyroscope sensor */
    GYROSCOPE = 4,
    /** ID for the uncalibrated gyroscope sensor */
    GYROSCOPE_UNCALIBRATED = 16,
    /** ID for the uncalibrated accelerometer sensor */
    ACCELEROMETER_UNCALIBRATED = 35,
    /** Denotes that the sensor type is either unknown or invalid */
    INVALID = 0xFF,
};

/**
 * @brief Information related to sensor
 */
struct SensorInfo {
    /** Unique identifier for the sensor. */
    int id;

    /** The type of sensor, @ref telux::sensor::SensorType */
    SensorType type;

    /** The name of the sensor This name is used to get a reference to a sensor with @ref
     * telux::sensor::ISensorManager::getSensorClient
     */
    std::string name;

    /** The name of the vendor */
    std::string vendor;

    /**
     * List of supported sampling rates by the sensor hardware, number of samples per second (Hz)
     */
    std::vector<float> samplingRates;

    /**
     * The maximum sampling rate the sensor can be configured for. This can be set in
     * /etc/sensors.conf for each sensor and should be less than the maximum sampling rate supported
     * by the sensor hardware, number of samples per second (Hz)
     *
     * This attribute should be considered while using the API @ref
     * telux::sensor::ISensorClient::configure
     */
    float maxSamplingRate;

    /**
     * Maximum batch count supported by the sensor, i.e. the maximum number of sensor events that
     * the underlying framework can buffer.
     *
     * This attribute should be considered while using the API @ref
     * telux::sensor::ISensorClient::configure
     */
    uint32_t maxBatchCountSupported;

    /**
     * Minimum batch count supported by the sensor. This is set in /etc/sensors.conf for each
     * sensor.
     *
     * This attribute should be considered while using the API @ref
     * telux::sensor::ISensorClient::configure
     */
    uint32_t minBatchCountSupported;

    /**
     * The range offered by the sensor. This configuration can be set in /etc/sensors.conf for each
     * sensor.
     *
     * For accelerometers, this is the number of Gs (force per unit mass due to gravity) in either
     * direction (+/-) on each axis
     *
     * For gyroscopes, this is the number of degrees per second (dps) in either direction (+/-)
     * along each axis
     */
    int range;

    /**
     * The version of the sensor considering the hardware part and the driver
     */
    int version;

    /**
     * This is the smallest difference between two values reported by this sensor, in meter per
     * second per second for accelerometer, radians per second for gyroscope
     */
    float resolution;

    /**
     * The maximum range this sensor offers, in meter per second per second for accelerometer,
     * radians per second for gyroscope. This attribute depends on the @ref SensorInfo::range of
     * the sensor set in the configuration file. For example, a range of 1G results in a maximum
     * range of approximately 9.8 m/s/s and a range of 2G gives a maximum range of about 19.6 m/s/s.
     */
    float maxRange;
};

/**
 * Enumeration listing the different configuration parameters in @ref SensorConfiguration
 */
enum SensorConfigParams {
    /** Corresponds to SensorConfiguration::samplingRate */
    SAMPLING_RATE,
    /** Corresponds to SensorConfiguration::batchCount */
    BATCH_COUNT,
    /** Corresponds to SensorConfiguration::isRotated */
    ROTATE,
    SENSOR_CONFIG_NUM_PARAMS
};

using SensorConfigMask = std::bitset<SensorConfigParams::SENSOR_CONFIG_NUM_PARAMS>;

/**
 * @brief Configurable parameters of a sensor
 */
struct SensorConfiguration {
    /**
     * The sampling rate for the sensor, number of samples per second (Hz)
     *
     * In case of @ref telux::sensor::ISensorClient::configure, the requested sampling rate should
     * be one of the sampling rates provided in the @ref telux::sensor::SensorInfo::samplingRates
     * and should be less than the @ref telux::sensor::SensorInfo::maxSamplingRate.
     *
     * If the requested sampling rate is less than the minimum value in the @ref
     * telux::sensor::SensorInfo::samplingRates, it will be set to the least of the values in @ref
     * telux::sensor::SensorInfo::samplingRates
     *
     * If the requested sampling rate is not one of the supported sampling rates in @ref
     * telux::sensor::SensorInfo::samplingRates, the requested value is floored to the
     * nearest value in @ref telux::sensor::SensorInfo::samplingRates
     *
     * Consider @ref telux::sensor::SensorInfo::samplingRates having values 12, 26, 52.
     * If requested sampling rate in configure API is 7, the sampling rate considered by the sensor
     * framework would be 12.
     * If requested sampling rate in configure API is 51, the sampling rate considered by the sensor
     * framework would be 26.
     *
     * In case of a configuration update received via
     * @ref telux::sensor::ISensorEventListener::onConfigurationUpdate, the current sampling rate
     * configuration is passed to the listener
     *
     */
    float samplingRate;

    /**
     * The batch count of the sensor.
     *
     * Batch count is the count of number of samples the underlying framework would buffer before
     * notifying the client of the data. The intention is to reduce the number of interactions
     * between the hardware, framework and the user application to reduce power consumption,
     * improve compute efficiency and reduce number of interactions between different components.
     * It is important to consider latency while deciding the batch count for a sensor. Higher the
     * batch count, more is the latency for the samples.
     *
     * In case of @ref telux::sensor::ISensorClient::configure, the requested batch count should be
     * lesser than the maximum supported batch count
     * @ref telux::sensor::SensorInfo::maxBatchCountSupported. Also, the batch count considered is
     * impacted by the @ref telux::sensor::SensorInfo::minBatchCountSupported.
     *
     * If the requested batch count is less than @ref
     * telux::sensor::SensorInfo::minBatchCountSupported, it will be set to @ref
     * telux::sensor::SensorInfo::minBatchCountSupported
     *
     * If the requested batch count is not a multiple of
     * @ref telux::sensor::SensorInfo::minBatchCountSupported, the requested value is floored to the
     * nearest multiple of @ref telux::sensor::SensorInfo::minBatchCountSupported
     *
     * Consider @ref telux::sensor::SensorInfo::minBatchCountSupported having a value of 7.
     * If requested batchCount in configure API is 2, the batchCount considered by the sensor
     * framework would be 7.
     * If requested batchCount in configure API is 23, the batchCount
     * considered by the sensor framework would be 21.
     *
     * In case of a configuration update
     * @ref telux::sensor::ISensorEventListener::onConfigurationUpdate, this field indicates the
     * current configuration for batch count.
     */
    uint32_t batchCount;

    /**
     * Configures if the sensor data provided is rotated as per Euler angles set.
     *
     * In case of @ref telux::sensor::ISensorClient::configure, if the flag "isRotated" is set,
     * the data provided via @ref telux::sensor::ISensorEventListener::onEvent will be rotated
     * based on Euler angle set via telux::sensor::ISensorManager::setEulerAngleConfig.
     *
     * In case of a configuration update received via
     * @ref telux::sensor::ISensorEventListener::onConfigurationUpdate, the current rotation
     * configuration is provided to the listener.
     *
     */
    bool isRotated;

    /**
     * Bitset indicating the validity of the received sensor configuration via @ref
     * telux::sensor::ISensorClient::getConfiguration and @ref
     * telux::sensor::ISensorEventListener::onConfigurationUpdate. The configuration items that were
     * never set would have return false when tested for using std::bitset::test.
     *
     * Further, this bitset should be set by the user to indicate the valid fields while configuring
     * the sensor using @ref telux::sensor::ISensorClient::configure.
     * For continuous stream of data from a sensor, the validity of SAMPLING_RATE, BATCH_COUNT and
     * ROTATE from @ref SensorConfigParams should be considered. If the sensor had been already
     * configured with sampling rate, batch count and rotation required, it is possible to
     * reconfigure the sensor partially with just one of these attributes and setting the required
     * validity flag.
     */
    SensorConfigMask validityMask;

    /**
     * Bitset indicating the parameters that were updated since last notification via @ref
     * telux::sensor::ISensorEventListener::onConfigurationUpdate
     */
    SensorConfigMask updateMask;
};

/**
 * @brief Euler angles for a sensor.
 */
struct EulerAngleConfig {
    float roll;
    /**< rotation around Y [0 , 360] degrees, Positive counter-clockwise. */
    float pitch;
    /**< rotation around X [0 , 360] degrees, Positive counter-clockwise. */
    float yaw;
    /**< rotation around Z [0 , 360] degrees, Positive counter-clockwise. */
};

/**
 * @brief Structure of a single sample from a motion sensor
 */
struct MotionSensorData {
    /** x-axis data, meter per second per second for accelerometer, radians per second for
     * gyroscope */
    float x;
    /** y-axis data, meter per second per second for accelerometer, radians per second for
     * gyroscope */
    float y;
    /** z-axis data, meter per second per second for accelerometer, radians per second for
     * gyroscope */
    float z;
};

/**
 * @brief Structure of a single sample from uncalibrated motion sensor
 */
struct UncalibratedMotionSensorData {

    /** Uncalibrated motion sensor data @ref MotionSensorData */
    MotionSensorData data;

    /** Bias for the uncalibrated data @ref MotionSensorData */
    MotionSensorData bias;
};

/**
 * @brief Structure of a single sensor event
 */
struct SensorEvent {
    /** Timestamp when the event was generated on the hardware, nanosecond since boot-up */
    uint64_t timestamp;

    /** Sensor data */
    union {
        /**
         * Calibrated data - should be accessed when the @ref SensorType that generated the sensor
         * event accounts for calibration - @ref SensorType::ACCELEROMETER or @ref
         * SensorType::GYROSCOPE
         */
        MotionSensorData calibrated;
        /**
         * Uncalibrated data - should be accessed when the @ref SensorType that generated the
         * sensor event provides uncalibrated data along with bias information - @ref
         * SensorType::ACCELEROMETER_UNCALIBRATED or @ref SensorType::GYROSCOPE_UNCALIBRATED
         */
        UncalibratedMotionSensorData uncalibrated;
    };
};

/**
 * @brief Feature offered by sensor hardware and/or software framework
 */
struct SensorFeature {
    /** Name of the feature */
    std::string name;
};

/**
 * @brief Structure of an event that is generated from a sensor feature
 */
struct SensorFeatureEvent {
    /**
     * Best estimate of timestamp indicating the time of occurance of the event,
     * nanosecond since boot-up
     */
    uint64_t timestamp;
    /** Name of the feature that generated the event */
    std::string name;
    /** The ID of the generated event */
    int id;
};

/**
 * @brief Types of self test the sensor can perform
 *
 */
enum class SelfTestType {
    /** To initiate self test with positive values */
    POSITIVE,
    /** To initiate self test with negative values */
    NEGATIVE,
    /** To initiate self test of all types in a single shot */
    ALL
};

/**
 * @brief Enum to represent the result of sensor self test
 *
 */
enum SensorResultType {
    /**
     * When sensor is busy, self-test is not possible and
     * historical result would be provided.
     */
    HISTORICAL,
    /**
     * When sensor is idle, self test is performed and
     * the current result would be provided.
     */
    CURRENT
};

/**
 * @brief Represents the result of the sensor self test when
 * @ref telux::sensor::ISensorClient::selfTest is invoked with the
 * @ref telux::sensor::SelfTestAdvancedParamsCallback callback.
 *
 */
struct SelfTestResultParams {
    /** Sensor result type as per @ref SensorResultType. */
    SensorResultType sensorResultType_;
    /** Boot timestamp in ns. Represents the timestamp of the self-test. */
    uint64_t timestamp_;
};

/** @} */ /* end_addtogroup telematics_sensor_service */
}  // namespace sensor
}  // namespace telux

#endif // TELUX_SENSOR_SENSORDEFINES_HPP
