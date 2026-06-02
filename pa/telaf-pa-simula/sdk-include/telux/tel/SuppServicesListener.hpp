/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file       SuppServicesListener.hpp
 *
 * @brief      ISuppServicesListener provides callback methods for listening to
 *             notifications about supplementary services. Client needs to implement this
 *             interface to get access to notifications. The methods in listener can be
 *             invoked from multiple different threads. The implementation should be
 *             thread-safe.
 */

#ifndef TELUX_TEL_SUPPSERVICESLISTENER_HPP
#define TELUX_TEL_SUPPSERVICESLISTENER_HPP

#include <telux/common/CommonDefines.hpp>

namespace telux {
namespace tel {

/** @addtogroup telematics_supp_services
 * @{ */

/**
 * @brief     A listener class for receiving supplementary services notifications.
 *            The methods in listener can be invoked from multiple different
 *            threads. The implementation should be thread safe.
 */

class ISuppServicesListener : public telux::common::IServiceStatusListener {

public:
    /**
     * @brief Destroy the ISuppServicesListener object
     *
     */
    virtual ~ISuppServicesListener() {}
};

/** @} */ /* end_addtogroup telematics_supp_services */

} // end of namespace tel
} // end of namespace telux

#endif // TELUX_TEL_SUPPSERVICESLISTENER_HPP
