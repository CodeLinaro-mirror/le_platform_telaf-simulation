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
 * @file       RemoteSimListener.hpp
 *
 * @brief      Listener interface for receiving remote SIM notifications. The methods
 *             in listener can be invoked from multiple different threads. The
 *             implementation should be thread safe.
 */

#ifndef TELUX_TEL_REMOTESIMLISTENER_HPP
#define TELUX_TEL_REMOTESIMLISTENER_HPP

#include <telux/common/CommonDefines.hpp>
#include <telux/tel/RemoteSimDefines.hpp>

namespace telux {

namespace tel {

/** @addtogroup telematics_remote_sim
 * @{ */

/**
 * @brief A listener class for getting remote SIM notifications
 *
 * The methods in listener can be invoked from multiple different threads. The
 * implementation should be thread safe.
 */
class IRemoteSimListener : public common::IServiceStatusListener {
public:
   /**
    * This function is called when the modem wants to transmit a command APDU.
    *
    * @param    [in] id     Identifier for a command and response APDU pair
    * @param    [in] apdu   APDU request sent to the control point (max size = 261, per ETSI TS
    *                                                               102 221, section 10.1.4)
    */
    virtual void onApduTransfer(const unsigned int id, const std::vector<uint8_t> &apdu) {
    }

   /**
    * This function is called when the modem wants to establish a connection.
    */
    virtual void onCardConnect() {
    }

   /**
    * This function is called when the modem wants to tear down a connection.
    */
    virtual void onCardDisconnect() {
    }

   /**
    * This function is called when the modem wants to power up the card.
    */
    virtual void onCardPowerUp() {
    }

   /**
    * This function is called when the modem wants to power down the card.
    */
    virtual void onCardPowerDown() {
    }

   /**
    * This function is called when the modem wants to warm reset the card.
    */
    virtual void onCardReset() {
    }

   /**
    * Destructor of IRemoteSimListener
    */
    virtual ~IRemoteSimListener() {
    }
};

/** @} */ /* end_addtogroup telematics_remote_sim */

} // end of namespace tel

} // End of namespace telux

#endif // TELUX_TEL_REMOTESIMLISTENER_HPP
