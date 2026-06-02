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
 * @file       ThermalManager.hpp
 *
 * @brief      Thermal Manager is a primary interface for thermal zones (sensors) and
 *             thermal cooling devices to get list of sensor temperature readings,
 *             trip point information.
 */

#ifndef TELUX_THERM_THERMALMANAGER_HPP
#define TELUX_THERM_THERMALMANAGER_HPP

#include <bitset>
#include <vector>
#include <string>
#include <memory>

#include "telux/common/CommonDefines.hpp"
#include "telux/therm/ThermalDefines.hpp"
#include "telux/therm/ThermalListener.hpp"

namespace telux {
namespace therm {

/** @addtogroup telematics_therm_management
 * @{ */

class IThermalZone;
class ICoolingDevice;
class ITripPoint;

/**
 * Defines the type of trip points, it can be one of the values for
 * ACPI (Advanced Configuration and Power Interface) thermal zone
 */
enum class TripType {
    UNKNOWN,           /**< Trip type is unknown */
    CRITICAL,          /**< Trip point at which system shuts down */
    HOT,               /**< Trip point to notify emergency */
    PASSIVE,           /**< Trip point at which kernel lowers the CPU's frequency and throttle the processor down */
    ACTIVE,            /**< Trip point at which processor fan turns on */
    CONFIGURABLE_HIGH, /**< Triggering threshold at which mitigation starts. This type is added to support legacy targets*/
    CONFIGURABLE_LOW   /**< Clearing threshold at which mitigation stops. This type is added to support legacy targets*/
};

/**
 * Defines the event of trip.
 */
enum class TripEvent {
    NONE = -1,     /**< Trip event is none */
    CROSSED_UNDER, /**< This event will be triggered when the temperature decreases and crosses
                        below the configured trip minus hysteresis temp. This event will not be
                        triggered again, if the temperature remains below the trip temperature.
                        For Example: Below scenario considered as CROSSED_UNDER.
                        Prev temp: 27000 milli degree Celsius,
                        Trip temp: 25000 milli degree Celsius, Hyst: 5000 milli degree Celsius,
                        Curr Temp: 19000 milli degree Celsius,
                        Below scenario will not generate CROSSED_UNDER event again.
                        Prev temp: 19000 milli degree Celsius,
                        Trip temp: 25000 milli degree Celsius, Hyst: 5000 milli degree Celsius,
                        Curr Temp: 18000 milli degree Celsius / 22000 milli degree Celsius*/
    CROSSED_OVER   /**< This event will be triggered when the temperature increases and crosses
                        over the configured trip temperature. This event will not be triggered
                        again, if the temperature remains over the trip temperature.
                        For Example: Below scenario considered as CROSSED_OVER.
                        Prev temp: 24000 milli degree Celsius,
                        Trip temp: 25000 milli degree Celsius, Curr Temp: 26000 milli degree Celsius,
                        Below scenario will not generate CROSSED_OVER event again.
                        Prev temp: 26000 milli degree Celsius, Trip temp: 25000 milli degree Celsius,
                        Curr Temp: 27000 milli degree Celsius*/
};

/**
 * Defines the trip points to which cooling device is bound.
 */
struct BoundCoolingDevice {
    int coolingDeviceId; /**< Cooling device Id associated with trip points */
    std::vector<std::shared_ptr<ITripPoint>> bindingInfo; /**< List of trippoints bound to the
                                                                cooling device */
};

/**
 * Defines some of the notifications supported by IThermalListener which can be dynamically
 * disabled/enabled.
 */
enum ThermalNotificationType {
    TNT_TRIP_UPDATE,       /* Enables onTripEvent() notification*/
    TNT_CDEV_LEVEL_UPDATE, /* Enables onCoolingDeviceLevelUpdate() notification*/
    TNT_MAX_TYPE,
};

/**
 * Bit mask that denotes a set of notifications defined in ThermalNotificationType
 */
using ThermalNotificationMask = std::bitset<16>;

/**
 * @brief   IThermalManager provides interface to get thermal zone and cooling device information.
 */
class IThermalManager {
 public:
    /**
     * This status indicates whether the object is in a usable state.
     *
     * @returns  @ref telux::common::ServiceStatus
     *
     */
    virtual telux::common::ServiceStatus getServiceStatus() = 0;

    /**
     * Registers the listener for Thermal Manager indications.
     *
     * @param [in] listener      - pointer to implemented listener.
     * @param [in] mask          - Bit mask representing a set of notifications that needs
     *                             to be registered - @ref ThermalNotificationType
     *                             Notifications under IThermalListener that are not listed
     *                             in @ref ThermalNotificationType would always be registered
     *                             by default when this API is invoked. In the absence of this
     *                             optional parameter, all the notifications will be registered.
     *                             Bits that are not set in the mask are ignored and do not have
     *                             any effect on registration or deregistration. To deregister,
     *                             the API @ref deregisterListener should be used.
     *                             For Example: API invoked with mask: 0x0001 enables onTripEvent
     *                             notification, next invocation with mask: 0x0002 enables
     *                             onCoolingDeviceLevelUpdate notification and previous
     *                             registration for onTripEvent remains intact.
     *
     * @returns status of the registration request.
     *
     */
    virtual telux::common::Status registerListener(
        std::weak_ptr<IThermalListener> listener, ThermalNotificationMask mask = 0xFFFF)
        = 0;

    /**
     * Deregisters the previously registered listener.
     *
     * @param [in] listener      - pointer to registered listener that needs to be removed.
     * @param [in] mask          - Bit mask that denotes a set of notifications that needs to be
     *                             de-registered - @ref ThermalNotificationType
     *                             Notifications under IThermalListener that are not listed in
     *                             @ref ThermalNotificationType would not be de-registered by
     *                             default. If the client does not specifies mask or sets all
     *                             the bits, this API de-registers all the notifications.
     *                             Bits that are not set in the mask are ignored and do not
     *                             have any effect on registration or deregistration,To register,
     *                             the API @ref registerListener should be used.
     *                             For Example: API invoked with mask: 0x0001 disables onTripEvent
     *                             notification, next invocation with mask: 0x0002 disables
     *                             onCoolingDeviceLevelUpdate notification.
     *                             mask: 0x0000 is invalid options and API invoked with mask
     *                             0x0000 will be ignored.
     *
     * @returns status of the deregistration request.
     *
     */
    virtual telux::common::Status deregisterListener(
        std::weak_ptr<IThermalListener> listener, ThermalNotificationMask mask = 0xFFFF)
        = 0;

    /**
     * Retrieves the list of thermal zone info like type, temperature and trip points.
     *
     * @returns List of thermal zones.
     */
    virtual std::vector<std::shared_ptr<IThermalZone>> getThermalZones() = 0;

    /**
     * Retrieves the list of thermal cooling device info like type, maximum throttle state and
     * currently requested throttle state.
     *
     * @returns List of cooling devices.
     */
    virtual std::vector<std::shared_ptr<ICoolingDevice>> getCoolingDevices() = 0;

    /**
     * Retrieves the thermal zone details like temperature, type and trip point info
     * for the given thermal zone identifier.
     *
     * @param [in] thermalZoneId     Thermal zone identifier
     *
     * @returns Pointer to thermal zone.
     */
    virtual std::shared_ptr<IThermalZone> getThermalZone(int thermalZoneId) = 0;

    /**
     * Retrieves the cooling device details like type of the device, maximum cooling level and
     * current cooling level for the given cooling device identifier.
     *
     * @param [in] coolingDeviceId     Cooling device identifier
     *
     * @returns Pointer to cooling device.
     */
    virtual std::shared_ptr<ICoolingDevice> getCoolingDevice(int coolingDeviceId) = 0;

    /**
     * Destructor of IThermalManager
     */
    virtual ~IThermalManager(){};
};

/**
 * @brief   ITripPoint provides interface to get trip point type, trip point temperature
 *          and hysteresis value for that trip point.
 */
class ITripPoint {
 public:
    /**
     * Retrieves trip point type.
     *
     * @returns Type of trip point if available else return UNKNOWN.
     *          - @ref TripType
     */
    virtual TripType getType() const = 0;

    /**
     * Retrieves the temperature above which certain trip point will be fired.
     *        - Units: MilliDegree Celsius
     *
     * @returns Threshold temperature
     */
    virtual int getThresholdTemp() const = 0;

    /**
     * Retrieves hysteresis value that is the difference between current temperature of the device
     * and the temperature above which certain trip point will be fired. Units: MilliDegree Celsius
     *
     * @returns Hysteresis value
     */
    virtual int getHysteresis() const = 0;

    /**
     * Retrieves the identifier for trip point.
     *
     * @returns Identifier for trip point
     */
    virtual int getTripId() const = 0;

    /**
     * Retrieves associated tzone id for a trip point.
     *
     * @returns Identifier for thermal zone
     */
    virtual int getTZoneId() const = 0;

    /**
     * Operator for compare two trip points
     *
     * @returns result of two trip points whether equal or not equal.
     */
    virtual bool operator==(const ITripPoint &rHs) const = 0;

    /**
     * Destructor of ITripPoint
     */
    virtual ~ITripPoint(){};
};

/**
 * @brief   IThermalZone provides interface to get type of the sensor, the current temperature
 *          reading, trip points and the cooling devices binded etc.
 */
class IThermalZone {
 public:
    /**
     * Retrieves the identifier for thermal zone.
     *
     * @returns Identifier for thermal zone
     */
    virtual int getId() const = 0;

    /**
     * Retrieves the type of sensor.
     *
     * @returns Sensor type
     */
    virtual std::string getDescription() const = 0;

    /**
     * Retrieves the current temperature of the device. Units: MilliDegree Celsius
     *
     * @returns Current temperature
     */
    virtual int getCurrentTemp() const = 0;

    /**
     * Retrieves the temperature of passive trip point for the zone. Default value is 0.
     *  Valid values: 0 (disabled) or greater than 1000 (enabled), Units: MilliDegree Celsius
     *
     * @returns Temperature of passive trip point
     */
    virtual int getPassiveTemp() const = 0;

    /**
     * Retrieves trip point information like trip type, trip temperature and hysteresis.
     *
     * @returns Trip point info list
     */
    virtual std::vector<std::shared_ptr<ITripPoint>> getTripPoints() const = 0;

    /**
     * Retrieves the list of cooling device and the associated trip points bound to cooling device
     * in given thermal zone.
     *
     * @returns  List of bound cooling device for the given thermal zone.
     */
    virtual std::vector<BoundCoolingDevice> getBoundCoolingDevices() const = 0;

    /**
     * Destructor of IThermalZone
     */
    virtual ~IThermalZone(){};
};

/**
 * @brief   ICoolingDevice provides interface to get type of the cooling device, the maximum
 *          throttle state and the currently requested throttle state of the cooling device.
 */
class ICoolingDevice {
 public:
    /**
     * Retrieves the identifier of the thermal cooling device.
     *
     * @returns Cooling device identifier
     */
    virtual int getId() const = 0;

    /**
     * Retrieves the type of the cooling device.
     *
     * @returns Cooling device type
     */
    virtual std::string getDescription() const = 0;

    /**
     * Retrieves the maximum cooling level of the cooling device.
     *
     * @returns Maximum cooling level of the thermal cooling device
     */
    virtual int getMaxCoolingLevel() const = 0;

    /**
     * Retrieves the current cooling level of the cooling device.
     * This value can be between 0 and max cooling level.
     * Max cooling level is different for different cooling devices
     * like fan, processor etc.
     *
     * @returns Current cooling level of the thermal cooling device
     */
    virtual int getCurrentCoolingLevel() const = 0;

    /**
     * Destructor of ICoolingDevice
     */
    virtual ~ICoolingDevice(){};
};

/** @} */  // end_addtogroup telematics_therm_management

}  // end of namespace therm
}  // end of namespace telux

#endif // TELUX_THERM_THERMALMANAGER_HPP
