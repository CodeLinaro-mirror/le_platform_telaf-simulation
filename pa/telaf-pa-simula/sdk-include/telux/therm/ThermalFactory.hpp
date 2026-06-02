/*
 *  Copyright (c) 2019-2020, The Linux Foundation. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *    * Neither the name of The Linux Foundation nor the names of its
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 *  ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 *  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 *  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 *  OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 *  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * Changes from Qualcomm Technologies, Inc. are provided under the following license:
 *
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file       ThermalFactory.hpp
 * @brief      ThermalFactory allows creation of thermal manager.
 */

#ifndef TELUX_THERM_THERMALFACTORY_HPP
#define TELUX_THERM_THERMALFACTORY_HPP

#include <memory>

#include <telux/therm/ThermalManager.hpp>
#include <telux/therm/ThermalShutdownManager.hpp>

namespace telux {

namespace therm {

/** @addtogroup telematics_therm_management
 * @{ */

/**
 * @brief   ThermalFactory allows creation of thermal manager.
 */
class ThermalFactory {
 public:
    /**
     * Get Thermal Factory instance.
     */
    static ThermalFactory &getInstance();

    /**
     * Get thermal manager instance associated with a @ref telux::common::ProcType to get list of
     * thermal zones (sensors) and cooling devices supported by the device
     *
     * On platforms with Access control enabled, Caller needs to have TELUX_THERM_DATA_READ
     * permission to invoke this API successfully.
     *
     * @param [in] callback  Optional callback pointer to get the response of the manager
     *                       initialization.
     *
     * @param [in] oprType   Operation type @ref telux::common::ProcType. Local operation type
     *                       fetches the thermal zones information where the application is running.
     *                       Remote operation type fetches the thermal zones information of modem
     *                       if the application is running on external application processor(EAP)
     *                       and vice versa.
     *
     * @returns Pointer of IThermalManager object.
     *
     */
    virtual std::shared_ptr<IThermalManager> getThermalManager(
        telux::common::InitResponseCb callback = nullptr,
        telux::common::ProcType operType = telux::common::ProcType::LOCAL_PROC)
        = 0;

    /**
     * Get thermal shutdown manager instance to control automatic thermal shutdown and get relevant
     * notifications
     *
     * On platforms with Access control enabled, Caller needs to have TELUX_THERM_SHUTDOWN_CTRL
     * permission to invoke this API successfully.
     *
     * @param [in] callback  Optional callback pointer to get the response of the manager
     *                       initialization.
     *
     * @returns Pointer of IThermalShutdownManager object.
     */
    virtual std::shared_ptr<IThermalShutdownManager> getThermalShutdownManager(
        telux::common::InitResponseCb callback = nullptr)
        = 0;

#ifndef TELUX_DOXY_SKIP
 protected:
    ThermalFactory();
    virtual ~ThermalFactory();
#endif

 private:
    ThermalFactory(const ThermalFactory &) = delete;
    ThermalFactory &operator=(const ThermalFactory &) = delete;
};

/** @} */ /* end_addtogroup telematics_therm_management */

}  // End of namespace therm

}  // End of namespace telux

#endif // TELUX_THERM_THERMALFACTORY_HPP
