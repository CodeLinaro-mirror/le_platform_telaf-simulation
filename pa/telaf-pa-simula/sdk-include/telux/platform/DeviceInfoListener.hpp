/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */


/**
 * @file    DeviceInfoListener.hpp
 *
 * @brief   DeviceInfoListener provides callback methods for listening to get the service
 *          status changed notification.
 *          Client need to implement these methods. The methods in listener can be invoked
 *          from multiple threads.So the client needs to make sure that the implementation
 *          is thread-safe.
 */

#ifndef TELUX_PLATFORM_DEVICEINFOLISTENER_HPP
#define TELUX_PLATFORM_DEVICEINFOLISTENER_HPP

#include <telux/common/CommonDefines.hpp>

namespace telux {

namespace platform {

/** @addtogroup telematics_platform_deviceinfo
 * @{ */

/**
 * @brief Listener class for getting device info related notifications .
 *        The client needs to implement these methods as briefly as possible and avoid blocking
 *        calls in it. The methods in this class can be invoked from multiple different threads.
 *        Client needs to make sure that the implementation is thread-safe.
 */
class IDeviceInfoListener : public common::IServiceStatusListener {
 public:

    /**
     * Destructor of IDeviceInfoListener
     */
    virtual ~IDeviceInfoListener() {
    }
};

/** @} */ /* end_addtogroup telematics_platform_deviceinfo */

}  // end of namespace platform

}  // end of namespace telux

#endif // TELUX_PLATFORM_DEVICEINFOLISTENER_HPP
