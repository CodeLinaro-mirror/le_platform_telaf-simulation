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
 * @file  TcuActivityListener.hpp
 * @brief Receives notifications when there is machine's power state change, machine's
 *        availability change or a consolidated acknowledgement from the slave clients.
 */

#ifndef TELUX_POWER_TCUACTIVITYLISTENER_HPP
#define TELUX_POWER_TCUACTIVITYLISTENER_HPP

#include <memory>
#include <vector>

#include <telux/common/SDKListener.hpp>
#include <telux/power/TcuActivityDefines.hpp>

namespace telux {
namespace power {

/** @addtogroup telematics_power_manager
 * @{ */

/**
 * Receives notifications when there is a machine's power state change, machine's
 * availability change or consolidated acknowledgement from the slave clients.
 *
 * It is recommended that the client should not perform any blocking/sleeping operation
 * from within methods in this class to ensure smooth transitions into different power
 * states. Also the implementation should be thread safe.
 */
class ITcuActivityListener : public telux::common::ISDKListener {
 public:
    /**
     * Called when the power state of the machine for which the client registered is
     * about to change. Called only for the slave clients not for the master client.
     *
     * Upon receiving this update, client must acknowledge with the appropriate response
     * @ref StateChangeResponsethrough using @ref ITcuActivityManager::sendActivityStateAck
     * so that the platform's power management framework can take the next appropriate step.
     *
     * When a slave client receives this update for suspend state, it is expected that it
     * should release all wakelocks and either pause or terminate operations that may prevent
     * the given machine from entering into low power state.
     *
     * @param[in] newState New power state
     *
     * @param[in] machineName Machine changing the state; @ref LOCAL_MACHINE or @ref ALL_MACHINES
     */
    virtual void onTcuActivityStateUpdate(TcuActivityState newState, std::string machineName) {}

    /**
     * Called only for the master client, provides consolidated responses from the slave clients.
     * This is not called for transitioning to resumed state.
     *
     * On platforms with access control enabled, the client needs to have TELUX_POWER_CONTROL_STATE
     * permission for this listener API to be invoked.
     *
     * @param[in] status telux::common::Status::SUCCESS if all the slaves responded with
     *                   StateChangeResponse::ACK, telux::common::Status::EXPIRED if at least one
     *                   slave did not respond within time limit, telux::common::Status::NOTREADY
     *                   if at least one slave responded with StateChangeResponse::NACK,
     *                   appropriate status code corresponding to the most number of clients if
     *                   they responded differently.
     *
     * @param[in] machineName Machine changing the state; @ref LOCAL_MACHINE or @ref ALL_MACHINES
     *
     * @param[in] unresponsiveClients Slaves that did not respond at all
     *
     * @param[in] nackResponseClients Slaves with @ref TcuActivityStateChangeResponse::NACK
     *                                response
     *
     * @note Recommended for both hypervisor and non-hypervisor based systems.
     */
    virtual void onSlaveAckStatusUpdate(const telux::common::Status status,
        const std::string machineName, const std::vector<ClientInfo> unresponsiveClients,
        const std::vector<ClientInfo> nackResponseClients) {}

    /**
     * Called when a machine registers/unregisters with the power management framework to
     * participate in the platform coordinated suspend/resume/shutdown state transitions.
     *
     * Primarily intended for the master client.
     *
     * @param[in] machineName  Machine (for example, qcom,mdm or qcom,eap, etc.)
     *
     * @param[in] machineEvent @ref MachineEvent::AVAILABLE if the machine is registered
     *                         @ref MachineEvent::UNAVAILABLE if the machine is unregistered
     */
    virtual void onMachineUpdate(const std::string machineName, const MachineEvent machineEvent) {}

    /**
     * Called only for the master client, provides consolidated responses from the slave clients.
     *
     * On platforms with access control enabled, the client needs to have TELUX_POWER_CONTROL_STATE
     * permission for this listener API to be invoked.
     *
     * @param[in] status Status of the slave client's acknowledgements
     *
     * @note        This API should not be used on virtual machines or on systems with hypervisor.
     *              The alternative API @ref onSlaveAckStatusUpdate(telux::common::Status status,
     *              std::string machineName, std::vector<std::string> unresponsiveClients,
     *              std::vector<std::string> nackResponseClients) should be used.
     *
     * @deprecated  Use @ref onSlaveAckStatusUpdate(const telux::common::Status status,
     *              const std::string machineName,
     *              const std::vector<std::pair<std::string, std::string>> unresponsiveClients,
     *              const std::vector<std::pair<std::string, std::string>> nackResponseClients)
     *              instead.
     */
    virtual void onSlaveAckStatusUpdate(telux::common::Status status) {}

    /**
     * Called when the power state is going to change.
     *
     * @param[in] newState New power state
     *
     * @deprecated Use @ref onTcuActivityStateUpdate(TcuActivityState newState,
     *             bool isGlobalStateChange) instead.
     */
    virtual void onTcuActivityStateUpdate(TcuActivityState newState) {}

    /**
     * Destructor of ITcuActivityListener.
     */
    virtual ~ITcuActivityListener() {}
};

/** @} */ /* end_addtogroup telematics_power_manager */

}  // end of namespace power
}  // end of namespace telux

#endif // TELUX_POWER_TCUACTIVITYLISTENER_HPP
