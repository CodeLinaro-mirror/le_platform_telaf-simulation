/*
 *  Copyright (c) 2019-2021, The Linux Foundation. All rights reserved.
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
 * @file  PowerFactory.hpp
 * @brief PowerFactory allows creation of an ITcuActivityManager instance.
 */

#ifndef TELUX_POWER_POWERFACTORY_HPP
#define TELUX_POWER_POWERFACTORY_HPP

#include <memory>

#include <telux/power/TcuActivityManager.hpp>
#include <telux/power/TcuActivityDefines.hpp>

#include <telux/power/WakeupManager.hpp>

namespace telux {
namespace power {

/** @addtogroup telematics_power_manager
 * @{ */

/**
 * PowerFactory allows creation of an ITcuActivityManager.
 */
class PowerFactory {
 public:
    /**
     * Gets the PowerFactory instance.
     */
    static PowerFactory &getInstance();

    /**
     * Gets the ITcuActivityManager instance.
     *
     * The instance is configured for the given client type (master/slave role) and identified
     * with the given unique name.
     *
     * @param[in] config Describes the client
     *
     * @param[in] callback Optional, receives result of the ITcuActivityManager initialization
     *
     * @returns ITcuActivityManager instance
     *
     * @note Recommended for both hypervisor and non-hypervisor based systems.
     */
    virtual std::shared_ptr<ITcuActivityManager> getTcuActivityManager(
        ClientInstanceConfig config, telux::common::InitResponseCb callback = nullptr) = 0;

    /**
     * Gets the IWakeupManager instance.
     *
     * @param[in] callback Optional, receives result of the IWakeupManager initialization
     *
     * @returns IWakeupManager instance
     */
    virtual std::shared_ptr<IWakeupManager> getWakeupManager(
        telux::common::InitResponseCb callback = nullptr) = 0;

    /**
     * Gets the ITcuActivityManager instance.
     *
     * @param[in] clientType Defines the role; master or slave
     *
     * @param[in] procType Processor type on which the operations will be performed
     *                     @ref telux::common::ProcType. @ref telux::common::ProcType::REMOTE_PROC
     *                     is not supported.
     * @param[in] callback Optional, receives result of the ITcuActivityManager initialization
     *
     * @returns ITcuActivityManager instance
     *
     * @note        This cannot be used on hypervisor based systems. Alternative API
     *              @ref PowerFactory::getTcuActivityManager(ClientInstanceConfig config,
     *              telux::common::InitResponseCb callback) should be used.
     *
     * @deprecated  Use @ref PowerFactory::getTcuActivityManager(ClientInstanceConfig config,
     *              telux::common::InitResponseCb callback) instead.
     */
    virtual std::shared_ptr<ITcuActivityManager> getTcuActivityManager(
        ClientType clientType = ClientType::SLAVE,
        common::ProcType procType = common::ProcType::LOCAL_PROC,
        telux::common::InitResponseCb callback = nullptr) = 0;

#ifndef TELUX_DOXY_SKIP
 protected:
    PowerFactory();
    virtual ~PowerFactory();
#endif

 private:
    PowerFactory(const PowerFactory &) = delete;
    PowerFactory &operator=(const PowerFactory &) = delete;
};

/** @} */ /* end_addtogroup telematics_power_manager */

}  // end of namespace power
}  // end of namespace telux

#endif // TELUX_POWER_POWERFACTORY_HPP
