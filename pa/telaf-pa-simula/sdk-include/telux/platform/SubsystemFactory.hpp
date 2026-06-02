/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file  SubsystemFactory.hpp
 * @brief SubsystemFactory allows creation of ISubsystemManager.
 */

#ifndef TELUX_PLATFORM_SUBSYSTEMFACTORY_HPP
#define TELUX_PLATFORM_SUBSYSTEMFACTORY_HPP

#include <memory>

#include <telux/common/CommonDefines.hpp>
#include <telux/platform/SubsystemManager.hpp>

namespace telux {
namespace platform {

/** @addtogroup telematics_platform
 * @{ */

/**
 * @brief SubsystemFactory allows creation of ISubsystemManager.
 */
class SubsystemFactory {
 public:
    /**
     * Gets the SubsystemFactory instance.
     */
    static SubsystemFactory &getInstance();

    /**
     * Instantiates an ISubsystemManager instance that can be used to monitor
     * status of the various subsystems.
     *
     * @param[out] initCallback callback to get the status of the ISubsystemManager creation
     *
     * @returns ISubsystemManager instance, if ISubsystemManager is created successfully,
     *          otherwise nullptr.
     */
    virtual std::shared_ptr<ISubsystemManager> getSubsystemManager(
       telux::common::InitResponseCb initCallback = nullptr) = 0;

#ifndef TELUX_DOXY_SKIP
 protected:
    SubsystemFactory();
    virtual ~SubsystemFactory();
#endif

 private:
    SubsystemFactory(const SubsystemFactory &) = delete;
    SubsystemFactory &operator=(const SubsystemFactory &) = delete;
};

/** @} */ /* end_addtogroup telematics_platform */

}  // End of namespace platform
}  // End of namespace telux

#endif // TELUX_PLATFORM_SUBSYSTEMFACTORY_HPP
