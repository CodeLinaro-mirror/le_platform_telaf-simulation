/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file  DiagnosticsFactory.hpp
 * @brief DiagnosticsFactory allows creation of the IDiagLogManager.
 */

#ifndef TELUX_PLATFORM_DIAG_DIAGNOSTICSFACTORY_HPP
#define TELUX_PLATFORM_DIAG_DIAGNOSTICSFACTORY_HPP

#include <memory>
#include <mutex>

#include <telux/common/CommonDefines.hpp>
#include <telux/platform/diag/DiagLogManager.hpp>

namespace telux {
namespace platform {
namespace diag {

/** @addtogroup telematics_diagnostics
 * @{ */

/**
 * @brief DiagnosticsFactory allows creation of IDiagLogManager.
 */
class DiagnosticsFactory {
 public:
    /**
     * Gets a DiagnosticsFactory instance.
     */
    static DiagnosticsFactory &getInstance();

    /**
     * Provides an IDiagLogManager instance that can be used for log collection.
     *
     * @param[in] callback Optional, callback to know the status of the
     *                     IDiagLogManager initialization
     *
     * @returns IDiagLogManager instance
     */
    virtual std::shared_ptr<IDiagLogManager> getDiagLogManager(
        telux::common::InitResponseCb callback = nullptr) = 0;

#ifndef TELUX_DOXY_SKIP
 protected:
    DiagnosticsFactory();
    virtual ~DiagnosticsFactory();
#endif

 private:
    DiagnosticsFactory(const DiagnosticsFactory &) = delete;
    DiagnosticsFactory &operator=(const DiagnosticsFactory &) = delete;
};

/** @} */ /* end_addtogroup telematics_diagnostics */

} // end of namespace diag
} // end of namespace platform
} // end of namespace telux

#endif // TELUX_PLATFORM_DIAG_DIAGNOSTICSFACTORY_HPP
