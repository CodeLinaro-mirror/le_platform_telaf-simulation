/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file       PlatformFactory.hpp
 *
 * @brief      PlatformFactory creates a set of managers which provide the corresponding
 *             platform services.
 */

#ifndef TELUX_PLATFORM_PLATFORMFACTORY_HPP
#define TELUX_PLATFORM_PLATFORMFACTORY_HPP

#include <memory>

#include <telux/common/CommonDefines.hpp>
#include <telux/platform/FsManager.hpp>
#include <telux/platform/DeviceInfoManager.hpp>
#include <telux/platform/TimeManager.hpp>

#include <telux/platform/hardware/AntennaManager.hpp>

namespace telux {

namespace platform {
/** @addtogroup telematics_platform
 * @{ */

/**
 * @brief   PlatformFactory allows creation of Platform services related classes.
 */

class PlatformFactory {
 public:
    /**
     * Get instance of platform Factory
     */
    static PlatformFactory &getInstance();

    /**
     * Get instance of filesystem manager (IFsManager). The filesystem manager supports
     * notification of filesystem events like EFS restore indications.
     *
     * @param [in] callback      Optional callback to get the initialization status of
     *                           FsManager. @ref telux::common::InitResponseCb
     *
     * @returns pointer of @ref IFsManager object.
     */
    virtual std::shared_ptr<IFsManager> getFsManager(
        telux::common::InitResponseCb callback = nullptr) = 0;

    /**
     * Get instance of device info manager (IDeviceInfoManager). The device info manager
     * supports device info request like retrieving IMEI and platform version.
     *
     * @param [in] callback      Optional callback to get the initialization status of
     *                           FsManager. @ref telux::common::InitResponseCb
     *
     * @returns pointer of @ref IDeviceInfoManager object.
     */
    virtual std::shared_ptr<IDeviceInfoManager> getDeviceInfoManager(
        telux::common::InitResponseCb callback = nullptr) = 0;

    /**
     * Gets a time manager (ITimeManger) instance. The time manager
     * supports registering for time reports.
     *
     * @param [in] callback      Optional callback to get the initialization status of
     *                           ITimeManager. @ref telux::common::InitResponseCb
     *
     * @returns ITimeManager instance or nullptr if time management is not supported.
     */
    virtual std::shared_ptr<ITimeManager> getTimeManager(
        telux::common::InitResponseCb callback = nullptr) = 0;

    /**
     * Gets an antenna manager (IAntennaManager) instance.
     *
     * @param [in] callback   Optional callback to get the initialization status of
     *                        antenna manager @ref telux::common::InitResponseCb
     *
     * @returns IAntennaManager instance or nullptr if antenna management is not
     * supported.
     *
     */
    virtual std::shared_ptr<hardware::IAntennaManager> getAntennaManager(
        telux::common::InitResponseCb callback = nullptr) = 0;

#ifndef TELUX_DOXY_SKIP
 protected:
    PlatformFactory();
    virtual ~PlatformFactory();
#endif

 private:
    PlatformFactory(const PlatformFactory &) = delete;
    PlatformFactory &operator=(const PlatformFactory &) = delete;
};

/** @} */ /* end_addtogroup telematics_platform */
}  // end of namespace platform

}  // end of namespace telux

#endif // TELUX_PLATFORM_PLATFORMFACTORY_HPP
