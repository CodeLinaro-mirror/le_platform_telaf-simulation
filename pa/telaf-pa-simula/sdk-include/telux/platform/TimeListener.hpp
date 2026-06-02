/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */


/**
 * @file    TimeListener.hpp
 *
 * @brief   TimeListener provides callback methods for listening to the time information.
 *          Client needs to implement these methods. The methods in listener can be invoked
 *          from multiple threads.So the client needs to make sure that the implementation
 *          is thread-safe.
 */

#ifndef TELUX_PLATFORM_TIMELISTENER_HPP
#define TELUX_PLATFORM_TIMELISTENER_HPP

#include <cstdint>
#include <telux/common/SDKListener.hpp>

namespace telux {

namespace platform {

/** @addtogroup telematics_platform_time
 * @{ */

/**
 * @brief Listener class for getting time information.
 *        The client needs to implement these methods as briefly as possible and avoid blocking
 *        calls in it. The methods in this class can be invoked from multiple different threads.
 *        Client needs to make sure that the implementation is thread-safe.
 */
class ITimeListener : public telux::common::ISDKListener {
 public:

    /**
     * This function is called every 100 milliseconds after registering a listener by
     * invoking @ref ITimeManager::registerListener.
     * The utc reported via this API is derived from location fix, utc value zero
     * means there is no valid utc derived from location fix.
     *
     * On platforms with Access control enabled, the client needs to have
     * TELUX_LOC_DATA permission for this API to be invoked.
     *
     * @param [out] utcInMs - Milliseconds since Jan 1, 1970.
     *
     */
    virtual void onGnssUtcTimeUpdate(const uint64_t utcInMs) {}

    /**
     * This function is called every second after registering a listener by
     * invoking @ref ITimeManager::registerListener.
     * In order for this API to be invoked, the vehicle needs to be in an
     * area of no GNSS coverage and select a roadside unit as the
     * synchronization reference, and a client (like an ITS stack) needs
     * to have injected a coarse UTC time using @ref
     * telux::cv2x::ICv2xRadioManager::injectCoarseUtcTime().
     *
     * @param [out] utcInMs - Milliseconds since Jan 1, 1970. 0 if no time
     *                        available via SLSS (Sidelink Synchronisation Signal).
     *
     */
    virtual void onCv2xUtcTimeUpdate(const uint64_t utcInMs) {}

    /**
     * Destructor of ITimeListener
     */
    virtual ~ITimeListener() {
    }
};

/** @} */ /* end_addtogroup telematics_platform_time */

}  // end of namespace platform

}  // end of namespace telux

#endif // TELUX_PLATFORM_TIMELISTENER_HPP
