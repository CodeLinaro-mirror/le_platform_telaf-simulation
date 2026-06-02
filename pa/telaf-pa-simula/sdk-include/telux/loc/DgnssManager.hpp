/*
 *  Copyright (c) 2020-2021, The Linux Foundation. All rights reserved.
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
 * @file       DgnssManager.hpp
 * @brief      DgnssManager provides APIs to inject correction data into
 *             modem location subsystem for location data correction.
 *
 */

#ifndef TELUX_LOC_DGNSSMANAGER_HPP
#define TELUX_LOC_DGNSSMANAGER_HPP

#include <future>
#include <memory>

#include "telux/common/CommonDefines.hpp"
#include "telux/loc/LocationDefines.hpp"
#include "telux/loc/DgnssListener.hpp"

namespace telux {

namespace loc {

/** @addtogroup telematics_location
* @{ */

/**
 * @brief IRtcmManager provides interface to inject RTCM data into modem,
 * register event listener reported by cdfw(correction data framework).
 *
 */
class IDgnssManager {
public:

/**
 * Checks the status of location subsystems and returns the result.
 *
 * @returns True if location subsystem is ready for service otherwise false.
 *
 * @deprecated use getServiceStatus()
 *
 */
  virtual bool isSubsystemReady() = 0;

/**
 * This status indicates whether the object is in a usable state.
 *
 * @returns SERVICE_AVAILABLE    -  If Dgnss manager is ready for service.
 *          SERVICE_UNAVAILABLE  -  If Dgnss manager is temporarily unavailable.
 *          SERVICE_FAILED       -  If Dgnss manager encountered an irrecoverable failure.
 *
 */
  virtual telux::common::ServiceStatus getServiceStatus() = 0;

/**
 * Wait for location subsystem to be ready.
 *
 * @returns  A future that caller can wait on to be notified when location
 *           subsystem is ready.
 *
 * @deprecated The callback mechanism introduced in the
 * @ref LocationFactory::getDgnssManager() API will provide the similar notification
 * mechanism as onSubsystemReady(). This API will soon be removed from further releases.
 *
 */
  virtual std::future<bool> onSubsystemReady() = 0;

/**
 * Register a listener for Dgnss injection status update.
 *
 * @param [in] listener - Pointer of IDgnssStatusListener object that processes the notification.
 *
 * @returns Status of registerListener i.e success or suitable status code.
 *
 */
  virtual telux::common::Status
      registerListener(std::weak_ptr<IDgnssStatusListener> listener) = 0;

/**
 * deRegister a listener for Dgnss injection status update.
 *
 * @returns Status of registerListener i.e success or suitable status code.
 *
 */
  virtual telux::common::Status deRegisterListener(void) = 0;

/**
 * Create a Dgnss injection source.
 * Only one source is permitted at any given time. If a new source is to be used, user must call
 * releaseSource() to release previous source before calling this function.
 *
 * @param [in] dataFormat Dgnss injection data format.
 *
 * @returns Success of suitable status code
 *
 */
  virtual telux::common::Status createSource(DgnssDataFormat dataFormat) = 0;

/**
 * Release current Dgnss injection source (previously created by  createSource() call)
 * This function is to be called if it's determined that current injection data is not
 * suitable anymore, and a new source will be created and used as injection source.
 *
 * @returns none
 *
 */
  virtual telux::common::Status releaseSource(void) = 0;

/**
 * Inject correction data
 * This function is to be called when a source has been created, either through a explicit call to
 * createSource(), or after DgnssManager object was instantiated through the factory method(The
 * factory method create a default source for DgnssManager object).
 *
 * @param [in] buffer buffer contains the data to be injected.
 * @param [in] bufferSize size of the buffer.
 * @returns success or suitable status code.
 *
 */
  virtual telux::common::Status injectCorrectionData(const uint8_t* buffer, uint32_t bufferSize) = 0;

/**
 * Destructor of IRtcmManager
 */
  virtual ~IDgnssManager() {}
  ;
};
/** @} */ /* end_addtogroup telematics_location */

} // end of namespace loc

} // end of namespace telux

#endif // TELUX_LOC_DGNSSMANAGER_HPP
