/*
 *  Copyright (c) 2020, The Linux Foundation. All rights reserved.
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
* @file       Cv2xTxRxSocket.hpp
*
* @brief      Cv2xRadio class encapsulates CV2X radio resource (socket for Tx and Rx).
* @brief      Represents a Cv2x socket associated with both Tx flow and
*             Rx subscription. Encapsulates the socket and socket address
*             for Tx and Rx.
*/



#ifndef TELUX_CV2X_CV2XTXRXSOCKET_HPP
#define TELUX_CV2X_CV2XTXRXSOCKET_HPP

#include <netinet/in.h>

#include <telux/cv2x/Cv2xRadioTypes.hpp>


namespace telux {

namespace cv2x {

/** @addtogroup telematics_cv2x_cpp
 * @{ */

/**
 * This is class encapsulates a Cv2xRadio socket for both Tx and Rx. It contains
 * the socket through which client applications can send and receive data. This class
 * is referenced in @ref ICv2xRadio::createCv2xTcpSocket and ICv2xRadio::closeCv2xTcpSocket.
 */
class ICv2xTxRxSocket {
public:

    /**
     * Accessor for Cv2xRadio socket ID. The socket ID should be unique within a process
     * but will not be unique between processes.
     *
     * @returns Cv2xRadio socket ID
     */
    virtual uint32_t getId() const = 0;

    /**
     * Accessor for service ID
     *
     * @returns The Service ID bound to the socket.
     */
    virtual uint32_t getServiceId() const = 0;

    /**
     * Accessor for the socket file descriptor
     *
     * @returns The socket fd.
     */
    virtual int getSocket() const = 0;

    /**
     * Accessor for the socket address description
     *
     * @returns The socket address
     */
    virtual struct sockaddr_in6 getSocketAddr() const = 0;

    /**
     * Accessor for the local port number bound to the socket
     *
     * @returns The local port number
     */
    virtual uint16_t getPortNum() const = 0;

    virtual ~ICv2xTxRxSocket() {}
};

/** @} */ /* end_addtogroup telematics_cv2x_cpp */

} // namespace cv2x

} // namespace telux

#endif // TELUX_CV2X_CV2XTXRXSOCKET_HPP
