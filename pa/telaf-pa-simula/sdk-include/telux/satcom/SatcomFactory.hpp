/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file       SatcomFactory.hpp
 *
 * @brief      SatcomFactory is the central factory to create all satcom instances
 *
 */

#ifndef TELUX_SATCOM_SATCOMFACTORY_HPP
#define TELUX_SATCOM_SATCOMFACTORY_HPP

#include <memory>

#include <telux/common/CommonDefines.hpp>

#include <telux/satcom/NtnManager.hpp>

namespace telux {
namespace satcom {

/** @addtogroup telematics_satcom
 * @{ */

class SatcomFactory {
 public:
    /**
     * Get Satcom Factory instance.
     */
    static SatcomFactory &getInstance();

    /**
     * Get NtnManager
     *
     * @param [in] clientCallback   Optional callback to get the initialization status of
     *                              NtnManager @ref telux::common::InitResponseCb
     *
     * @returns instance of INtnManager
     *
     */
    virtual std::shared_ptr<telux::satcom::INtnManager> getNtnManager(
        telux::common::InitResponseCb clientCallback) = 0;
#ifndef TELUX_DOXY_SKIP
 protected:
    SatcomFactory();
    virtual ~SatcomFactory();
#endif

 private:
    SatcomFactory(const SatcomFactory &)            = delete;
    SatcomFactory &operator=(const SatcomFactory &) = delete;
};

/** @} */ /* end_addtogroup telematics_satcom */
}  // namespace satcom
}  // namespace telux

#endif  // TELUX_SATCOM_SATCOMFACTORY_HPP
