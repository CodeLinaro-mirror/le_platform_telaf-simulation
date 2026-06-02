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
 * @file      RemoteSimDefines.hpp
 * @brief     RemoteSimDefines contains enumerations and variables used for the
 *            Remote SIM subsystem.
 */

#ifndef TELUX_TEL_REMOTESIMDEFINES_HPP
#define TELUX_TEL_REMOTESIMDEFINES_HPP

#include <cstdint>
#include <vector>

namespace telux {

namespace tel {

/** @addtogroup telematics_remote_sim
 * @{ */

/**
 * Defines the card error cause, sent to the modem by the SIM provider
 */
enum class CardErrorCause {
    INVALID = -1,               /**< Card error cause value will not be passed to modem */
    UNKNOWN_ERROR = 0,          /**< Unknown error */
    NO_LINK_ESTABLISHED = 1,    /**< No link was established */
    COMMAND_TIMEOUT = 2,        /**< Command timeout */
    POWER_DOWN = 3,             /**< Error due to a card power down */
};

/** @} */ /* end_addtogroup telematics_remote_sim */

} // End of namespace tel

} // End of namespace telux

#endif // TELUX_TEL_REMOTESIMDEFINES_HPP
