/*
 *  Copyright (c) 2017-2019, The Linux Foundation. All rights reserved.
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
 * @file       LocationFactory.hpp
 * @brief      LocationFactory allows creation of location manager, location configurator
 *             and dgnss manager.
 */

#ifndef TELUX_LOC_LOCATIONFACTORY_HPP
#define TELUX_LOC_LOCATIONFACTORY_HPP

#include <map>
#include <memory>
#include <string>
#include <vector>

#include <telux/loc/LocationDefines.hpp>
#include <telux/loc/LocationManager.hpp>
#include <telux/loc/LocationConfigurator.hpp>
#include <telux/loc/DgnssManager.hpp>

namespace telux {

namespace loc {
/** @addtogroup telematics_location
 * @{ */

/**
 * @brief   LocationFactory allows creation of location manager.
 */
class LocationFactory {
public:
   /**
    * Get Location Factory instance.
    */
   static LocationFactory &getInstance();

   /**
    * Get instance of Location Manager
    *
    * @param[in] callback   Optional callback to get the response of the manager
    *                       initialization.
    *
    * @returns Pointer of ILocationManager object.
    */
   virtual std::shared_ptr<ILocationManager> getLocationManager(telux::common::InitResponseCb
       callback = nullptr) = 0;

   /**
    * Get instance of Location Configurator.
    *
    * @param[in] callback   Optional callback pointer to get the response of the manager
    *                       initialisation.
    *
    * @returns Pointer of ILocationConfigurator object.
    */
   virtual std::shared_ptr<ILocationConfigurator> getLocationConfigurator(
       telux::common::InitResponseCb callback = nullptr) = 0;

   /**
    * Get instance of Dgnss manager.
    *
    * @param[in] dataFormat @ref DgnssDataFormat RTCM injection data format
    * @param[in] callback   Optional callback pointer to get the response of the manager
    *                       initialisation.
    *
    * @returns Pointer of IDgnssManager object.
    */
   virtual std::shared_ptr<IDgnssManager> getDgnssManager(
       DgnssDataFormat dataFormat = DgnssDataFormat::DATA_FORMAT_RTCM_3,
           telux::common::InitResponseCb callback = nullptr) = 0;

#ifndef TELUX_DOXY_SKIP
protected:
   LocationFactory();
   ~LocationFactory();
#endif

private:
   LocationFactory(const LocationFactory &) = delete;
   LocationFactory &operator=(const LocationFactory &) = delete;
};
/** @} */ /* end_addtogroup telematics_location */
}  // end of namespace loc

}  // end of namespace telux

#endif // TELUX_LOC_LOCATIONFACTORY_HPP
