/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */


/**
 * @file    AntennaListener.hpp
 *
 * @brief   AntennaListener provides callback methods to listen for service status change
 *          notifications.
 *          The client needs to implement these methods. AntennaListener methods can be
 *          invoked from multiple threads, so the client needs to ensure that the
 *          implementation is thread-safe.
 */

#ifndef TELUX_PLATFORM_HARDWARE_ANTENNALISTENER_HPP
#define TELUX_PLATFORM_HARDWARE_ANTENNALISTENER_HPP

#include <telux/common/CommonDefines.hpp>

namespace telux {

namespace platform {

namespace hardware {
/** @addtogroup telematics_platform_hardware_antenna
 * @{ */

/**
 * @brief Listen class to get antenna configuration related notifications.
 *        The client needs to implement these methods as briefly as possible and avoid blocking
 *        calls. Class methods can be invoked from multiple threads, so the client needs to
 *        ensure that the implementation is thread-safe.
 */
class IAntennaListener : public common::IServiceStatusListener {
 public:

    /**
     * This function is called whenever any active cellular antenna is changed.
     *
     * On platforms with access control enabled, the caller needs to have
     * TELUX_PLATFORM_ANTENNA_MGMT permission to receive this notification.
     *
     * @param [in] antIndex        Indicates which antenna is now active.
     *
     */
    virtual void onActiveAntennaChange(int antIndex) {
    }

    /**
     * IAntennaListener destructor.
     */
    virtual ~IAntennaListener() {
    }
};

/** @} */ /* end_addtogroup telematics_platform_hardware_antenna */
}  // end of namespace hardware

}  // end of namespace platform

}  // end of namespace telux

#endif // TELUX_PLATFORM_HARDWARE_ANTENNALISTENER_HPP
