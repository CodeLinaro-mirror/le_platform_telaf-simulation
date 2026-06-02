/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
/**
 * @file       ThermalListener.hpp
 *
 * @brief      IThermalListener - Interface for Thermal listener object. the clients needs
 *             to implement this interface to get access to thermal service notifications
 *             like onServiceStatusChange.
 *             The methods in listener can be invoked from multiple threads.So the client
 *             needs to make sure that the implementation is thread-safe.
 */

#ifndef TELUX_THERM_THERMALLISTENER_HPP
#define TELUX_THERM_THERMALLISTENER_HPP

#include <memory>
#include <vector>
#include <telux/common/CommonDefines.hpp>

namespace telux {
namespace therm {

/** @addtogroup telematics_therm_management
 * @{ */

class ITripPoint;
class ICoolingDevice;
enum class TripEvent;

/**
 * @brief Listener class for getting notifications when thermal service status changes.
 *        The client needs to implement these methods as briefly as possible and avoid blocking
 *        calls in it. The methods in this class can be invoked from multiple different threads.
 *        Client needs to make sure that the implementation is thread-safe.
 */
class IThermalListener : public telux::common::IServiceStatusListener {
 public:
    /**
     * Destructor of IThermalListener
     */
    virtual ~IThermalListener() {
    }

    /**
     * This function is called at the time of cooling device level update.
     * On platforms with Access control enabled, the client needs to have
     * TELUX_THERM_DATA_READ permission to receive this event.
     *
     * @param [in] coolingDevice - vector of cooling device for which the level has been
     *                             updated.
     */
    virtual void onCoolingDeviceLevelChange(std::shared_ptr<ICoolingDevice> coolingDevice) {
    }

    /**
     * This function is called at the time of trip event occurs.
     * On platforms with Access control enabled, the client needs to have
     * TELUX_THERM_DATA_READ permission to receive this event.
     *
     * @param [in] tripInfo  - Vector of the trip point for which trip event has been occured.
     * @param [in] tripEvent - Indicates trip event.
     *                       - NONE
     *                       - CROSSED_UNDER
     *                       - CROSSED_OVER
     */
    virtual void onTripEvent(std::shared_ptr<ITripPoint> tripPoint, TripEvent tripEvent) {
    }
};

/** @} */ /* end_addtogroup telematics_therm_management */

}  // end of namespace therm
}  // end of namespace telux

#endif // TELUX_THERM_THERMALLISTENER_HPP
