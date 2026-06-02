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
 * @file       RemoteSimManager.hpp
 * @brief      Remote SIM Manager is a primary interface for remote card operations.
 *             This allows a device to use a SIM card on another device for its WWAN
 *             modem functionality. The SIM provider service is the endpoint that
 *             interfaces with the SIM card (e.g. over bluetooth) and sends/receives
 *             data to the other endpoint, the modem. The modem sends requests to the
 *             SIM provider service to interact with the SIM card (e.g. power up,
 *             transmit APDU, etc.), and is notified of events (e.g. card errors,
 *             resets, etc.). This API is used by the SIM provider endpoint to provide
 *             a SIM card to the modem.
 */

#ifndef TELUX_TEL_REMOTESIMMANAGER_HPP
#define TELUX_TEL_REMOTESIMMANAGER_HPP

#include <future>
#include <memory>

#include <telux/tel/RemoteSimDefines.hpp>
#include <telux/tel/RemoteSimListener.hpp>

namespace telux {

namespace tel {

/** @addtogroup telematics_remote_sim
 * @{ */

/**
 * @brief IRemoteSimManager provides APIs for remote SIM related operations. This allows
 *        a device to use a SIM card on another device for its WWAN modem functionality.
 *        The SIM provider service is the endpoint that interfaces with the SIM card
 *        (e.g. over bluetooth) and sends/receives data to the other endpoint, the modem.
 *        The modem sends requests to the SIM provider service to interact with the SIM
 *        card (e.g. power up, transmit APDU, etc.), and is notified of events (e.g. card
 *        errors, resets, etc.). This API is used by the SIM provider endpoint to provide
 *        a SIM card to the modem.
 */
class IRemoteSimManager {
public:
    /**
     * Checks the status of remote SIM subsystem and returns the result.
     *
     * @deprecated Use IRemoteSimManager::getServiceStatus() API.
     *
     * @returns True if remote SIM subsystem is ready for service otherwise false.
     */
    virtual bool isSubsystemReady() = 0;

    /**
     * Wait for remote SIM subsystem to be ready.
     *
     * @deprecated Use InitResponseCb in PhoneFactory::getRemoteSimManager instead,
     *             to get notified about subsystem readiness.
     *
     * @returns  A future that caller can wait on to be notified when remote SIM
     *           subsystem is ready.
     */
    virtual std::future<bool> onSubsystemReady() = 0;

    /**
    * This status indicates whether the IRemoteSimManager object is in a usable state.
    *
    * @returns SERVICE_AVAILABLE    - If Remote Sim Manager is ready for service.
    *          SERVICE_UNAVAILABLE  - If Remote Sim Manager is temporarily unavailable.
    *          SERVICE_FAILED       - If Remote Sim Manager encountered an irrecoverable
    *                                 failure.
    *
    */
    virtual telux::common::ServiceStatus getServiceStatus() = 0;

    /**
     * Send reset command to the modem to reset state variables.
     *
     * @param [out] callback   Callback function pointer to get the response of sendReset.
     *
     * @returns Status of sendReset i.e. success or suitable status code.
     */
    virtual telux::common::Status sendReset(telux::common::ResponseCallback callback = nullptr) = 0;

    /**
     * Send connection available event to the modem.
     *
     * @param [out] callback        Callback function pointer to get the response.
     *
     * @returns Status of sendConnectionAvailable i.e. success or suitable status code.
     */
    virtual telux::common::Status sendConnectionAvailable(
        telux::common::ResponseCallback callback = nullptr) = 0;

    /**
     * Send connection unavailable event to the modem.
     *
     * @param [out] callback    Callback function pointer to get the response.
     *
     * @returns Status of sendConnectionUnavailable i.e. success or suitable status code.
     */
    virtual telux::common::Status sendConnectionUnavailable(
        telux::common::ResponseCallback callback = nullptr) = 0;

    /**
     * Send card reset event to the modem.
     *
     * @param [in]  atr         Answer to Reset bytes (max size = 32, per ISO/IEC 7816-3:2006
     *                                                 section 8.1).
     * @param [out] callback    Callback function pointer to get the response of sendCardReset.
     *
     * @returns Status of sendCardReset i.e. success or suitable status code.
     */
    virtual telux::common::Status sendCardReset(const std::vector<uint8_t> &atr,
                                                telux::common::ResponseCallback callback = nullptr)
        = 0;

    /**
     * Send card error event to the modem.
     *
     * @param [in]  cause       Card Error cause.
     * @param [out] callback    Callback function pointer to get the response of sendCardError.
     *
     * @returns Status of sendCardError i.e. success or suitable status code.
     */
    virtual telux::common::Status sendCardError(
        const CardErrorCause cause = CardErrorCause::INVALID,
        telux::common::ResponseCallback callback = nullptr) = 0;

    /**
     * Send card inserted event to the modem.
     *
     * @param [in]  atr         Answer to Reset bytes (max size = 32, per ISO/IEC 7816-3:2006
     *                                                 section 8.1).
     * @param [out] callback    Callback function pointer to get the response of sendCardInserted.
     *
     * @returns Status of sendCardInserted i.e. success or suitable status code.
     */
    virtual telux::common::Status sendCardInserted(const std::vector<uint8_t> &atr,
        telux::common::ResponseCallback callback = nullptr) = 0;

    /**
     * Send card removed event to the modem.
     *
     * @param [out] callback    Callback function pointer to get the response of sendCardRemoved.
     *
     * @returns Status of sendCardRemoved i.e. success or suitable status code.
     */
    virtual telux::common::Status sendCardRemoved(
        telux::common::ResponseCallback callback = nullptr) = 0;

    /**
     * Send card wakeup event to the modem.
     *
     * @param [out] callback    Callback function pointer to get the response of sendCardWakeup.
     *
     * @returns Status of sendCardWakeup i.e. success or suitable status code.
     */
    virtual telux::common::Status sendCardWakeup(
        telux::common::ResponseCallback callback = nullptr) = 0;

    /**
     * Sends an APDU message to the modem, in response to a previous APDU sent by the modem.
     *
     * @param [in]  id          Identifier for command and response APDU pair.
     * @param [in]  apdu        Response APDU (max size = 1024).
     * @param [in]  isSuccess   Whether APDU transaction completed successfully.
     * @param [in]  totalSize   Total length of the APDU message (used when the response is larger
                                                                  than 1024 bytes and must be passed
                                                                  in multiple segments).
     * @param [in]  offset      Offset of this APDU segment in the original message.
     * @param [out] callback    Callback function pointer to get the response of sendApdu.
     *
     * @returns Status of sendApdu i.e. success or suitable status code.
     */
    virtual telux::common::Status sendApdu(const unsigned int id, const std::vector<uint8_t> &apdu,
                                           const bool isSuccess = true,
                                           const unsigned int totalSize = 0,
                                           const unsigned int offset = 0,
                                           telux::common::ResponseCallback callback = nullptr) = 0;

    /**
     * Register a listener for specific updates from the modem.
     *
     * @param [in] listener    Pointer of IRemoteSimListener object that
     *                         processes the notification
     *
     * @returns Status of registerListener i.e success or suitable status code.
     */
    virtual telux::common::Status registerListener(std::weak_ptr<IRemoteSimListener> listener) = 0;

    /**
     * Deregister the previously added listener.
     *
     * @param [in] listener    Previously registered IRemoteSimListener
     *                         that needs to be deregistered
     *
     * @returns Status of deregisterListener success or suitable status code
     */
    virtual telux::common::Status deregisterListener(std::weak_ptr<IRemoteSimListener> listener)
        = 0;

    /**
     * Get associated slot ID for the RemoteSimManager
     *
     * @returns The slot ID associated with this IRemoteSimManager
     */
    virtual int getSlotId() = 0;

    /**
     * Destructor of IRemoteSimManager
     */
    virtual ~IRemoteSimManager(){};

}; // End of IRemoteSimManager

/** @} */ /* end_addtogroup telematics_remote_sim */

} // End of namespace tel

} // End of namespace telux

#endif // TELUX_TEL_REMOTESIMMANAGER_HPP
