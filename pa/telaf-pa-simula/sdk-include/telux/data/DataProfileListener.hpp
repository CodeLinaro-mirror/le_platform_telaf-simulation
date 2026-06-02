/*
 *  Copyright (c) 2018-2020, The Linux Foundation. All rights reserved.
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
 * @file       DataProfileListener.hpp
 *
 * @brief      Listener interface for receiving data profile change events.
 */

#ifndef TELUX_DATA_DATAPROFILELISTENER_HPP
#define TELUX_DATA_DATAPROFILELISTENER_HPP

#include <telux/common/SDKListener.hpp>
#include <telux/common/CommonDefines.hpp>
#include "telux/data/DataDefines.hpp"

namespace telux {

namespace data {

/** @addtogroup telematics_data
 * @{ */

/**
 * @brief Listener class for getting profile change notification
 *
 *        The methods in the listener can be invoked from multiple threads.
 *        It is client's responsibility to make sure the implementation is thread safe.
 */
class IDataProfileListener : public telux::common::ISDKListener {
public:
   /**
    *
    * This function is called when service status changes.
    *
    *  @param [in] status - @ref ServiceStatus
    */
   virtual void onServiceStatusChange(telux::common::ServiceStatus status) {};

   /**
    * This function is called when profile change happens.
    *
    * @param [in] profileId - ID of the updated profile.
    * @param [in] techPreference - @ref TechPreference.
    * @param [in] event - Event that caused the change in profile.
    */
   virtual void onProfileUpdate(int profileId, TechPreference techPreference,
      ProfileChangeEvent event) {
   }

   /**
    * Destructor of IDataProfileListener
    */
   virtual ~IDataProfileListener() {
   }
};
/** @} */ /* end_addtogroup telematics_data */
}  // end of namespace data

}  // end of namespace telux

#endif // TELUX_DATA_DATAPROFILELISTENER_HPP
