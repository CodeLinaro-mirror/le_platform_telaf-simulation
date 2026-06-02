/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */


/**
 * @file       AntennaManager.hpp
 * @brief      AntennaManager provides APIs related to antenna management, such as APIs to set
 *             or get the active antenna's configuration.
 */

#ifndef TELUX_PLATFORM_HARDWARE_ANTENNAMANAGER_HPP
#define TELUX_PLATFORM_HARDWARE_ANTENNAMANAGER_HPP

#include <memory>
#include <telux/common/CommonDefines.hpp>
#include <telux/platform/hardware/AntennaListener.hpp>

namespace telux {

namespace platform {

namespace hardware {
/** @addtogroup telematics_platform_hardware_antenna
 * @{ */

/**
 * This function is called with the response to the getActiveAntenna API.
 *
 * The callback can be invoked from multiple threads, so the client needs to
 * ensure that the implementation is thread-safe.
 *
 * @param [in] antIndex    Active physical antenna switch path index.
 * @param [in] error       Return code indicating whether the operation succeeded or not
 *                         @ref telux::common::ErrorCode.
 *
 */
using GetActiveAntCb
    = std::function<void(int antIndex, telux::common::ErrorCode error)>;

/**
 * @brief   IAntennaManager provides an interface to set and get the active antenna's configuration.
 */
class IAntennaManager {
 public:
    /**
     * Indicates whether the object is in a usable state.
     *
     * @returns @ref telux::common::ServiceStatus indicating the current status of the antenna
     *          configuration service.
     *
     */
    virtual telux::common::ServiceStatus getServiceStatus() = 0;

    /**
     * Registers the listener for antenna manager indications.
     *
     * @param [in] listener      Pointer to the implemented listener.
     *
     * @returns Status of the registration request.
     *
     */
    virtual telux::common::Status registerListener(std::weak_ptr<IAntennaListener> listener) = 0;

    /**
     * Deregisters the previously registered listener.
     *
     * @param [in] listener      Pointer to the registered listener that needs to be removed.
     *
     * @returns Status of the deregistration request.
     *
     */
    virtual telux::common::Status deregisterListener(std::weak_ptr<IAntennaListener> listener)
       = 0;

    /**
     * Switches the cellular antenna configuration between antennas when an antenna is damaged.
     * This API is to be invoked when the client detects that the currently active antenna is
     * broken and determines that a switch to another antenna is required to maintain cellular
     * services.
     * The index of the antenna is based on the order in which the antenna appears in the radio
     * frequency control (RFC). Across reboots or SSR, this configuration will not be persistent
     * and it will reset back to the initial antenna. Clients are required to call this API again
     * to switch to the desired antenna.
     *
     * On platforms with access control enabled, the caller needs to have
     * TELUX_PLATFORM_ANTENNA_MGMT permission to successfully invoke this API.
     *
     * @param [in] antIndex      Physical antenna switch path index to be set,
     *                           this index starts with 0.
     * @param [in] callback      Optional callback pointer to get the response
     *                           of the setActiveAntenna request.
     *
     * @returns Status of the setActiveAntenna request; either success or the suitable error code.
     *
     */
    virtual telux::common::Status
        setActiveAntenna(int antIndex, telux::common::ResponseCallback callback = nullptr) = 0;

    /**
     * Gets the current active cellular antenna configuration index of the device. Valid only when
     * the device is camped on network.
     *
     * On platforms with access control enabled, the caller needs to have
     * TELUX_PLATFORM_ANTENNA_MGMT permission to successfully invoke this API.
     *
     * @param [in] callback   Callback function to get the getActiveAntenna response.
     *
     * @returns Status of the getActiveAntenna request; either success or the suitable error code.
     *
     */
    virtual telux::common::Status getActiveAntenna(GetActiveAntCb callback) = 0;

    /**
     * IAntennaManager destructor.
     */
    virtual ~IAntennaManager(){};
};

/** @} */ /* end_addtogroup telematics_platform_hardware_antenna */
}  // end of namespace hardware

}  // end of namespace platform

}  // end of namespace telux

#endif // TELUX_PLATFORM_HARDWARE_ANTENNAMANAGER_HPP
