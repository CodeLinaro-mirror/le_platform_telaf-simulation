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
 * @file  TcuActivityDefines.hpp
 * @brief Contains data structures, data types and macros used with power management APIs.
 */

#include <string>
#include <telux/common/CommonDefines.hpp>

#ifndef TELUX_POWER_TCUACTIVITYDEFINES_HPP
#define TELUX_POWER_TCUACTIVITYDEFINES_HPP

namespace telux {
namespace power {

/** @addtogroup telematics_power_manager
 * @{ */

/**
 * A client represented with a client and machine name pair.
 * First element is client's name and second element is machine's name.
 */
typedef std::pair<std::string, std::string> ClientInfo;

/**
 * Power states.
 */
enum class TcuActivityState {
    /** Current power state is unknown */
    UNKNOWN,

    /** Master client uses it to indicate that the given machine(s) should suspend (enter
     *  lowest power state). Slave client receives it when the machine is about to suspend. */
    SUSPEND,

    /** Master client uses it to indicate that the given machine(s) should resume (resume
     *  operating at normal power level). Slave client receives it when the machine is about
     *  to suspend. */
    RESUME,

    /** Master client uses it to indicate that the given machine(s) should power-off.
     *  Slave client receives it when the machine is about to get powered-off. */
    SHUTDOWN
};

/**
 * Acknowledgement to accept or deny to transition to the notified power state.
 */
enum class StateChangeResponse {
    ACK,    /**< Ready to change state */
    NACK    /**< Not ready to change state */
};

/**
 * Defines a client's role in the power management. A master client can cause
 * power state change whereas a slave client listens to the state change. In
 * a system there can be only one master client and any number of slave clients.
 */
enum class ClientType {
    SLAVE,     /**< Slave client */
    MASTER,    /**< Master client */
};

/**
 * Confirms whether a machine is registered with the power management framework or not.
 */
enum class MachineEvent {
    AVAILABLE,      /**< Machine is registered */
    UNAVAILABLE     /**< Machine is unregistered (for example, crashed, rebooted, shutdown) */
};

/**
 * Collectively represents all the machines on the platform. For example, on hypervisor
 * based system it includes hostvm, televm and fotavm.
 */
static const std::string ALL_MACHINES = "ALL_MACHINES";

/**
 * Machine on which the caller process is running. On hypervisor based system
 * local machine is the virtual machine on which the caller process is running.
 */
static const std::string LOCAL_MACHINE = "LOCAL_MACHINE";

/**
 * ITcuActivityManager configuration.
 */
struct ClientInstanceConfig {
    /**
     * @ref ClientType (master or slave).
     */
    ClientType clientType = ClientType::SLAVE;

    /**
     * Uniquely identifies a client. This name is passed back to the client by
     * @ref ITcuActivityListener::onSlaveAckStatusUpdate.
     *
     * It is mandatory and must be unique. To maintain uniqueness, a tuple of machine
     * name, process name and process ID can be used (machineName_ProcessName_ProcessId).
     */
    std::string clientName;

    /**
     * For slave clients, specifies machine(s) whose power state change it will listen to.
     * @ref ALL_MACHINES can be used to listen to all machines. By default, local machine
     * on which the client process is running is listened.
     *
     * For master clients it is unused.
     */
    std::string machineName = LOCAL_MACHINE;
};

/**
 * Defines the acknowledgements to TCU-activity states. The client process sends this after
 * processing the TcuActivityState notification, indicating that it is prepared for state
 * transition.
 *
 * Acknowledgement for TcuActivityState::RESUME is not required, as the state transition
 * has already happened.
 *
 * @deprecated  The API @ref ITcuActivityManager::sendActivityStateAck (TCUActivityStateAck)
 *              that uses this enum is deprecated. Instead, use
 *              @ref ITcuActivityManager::sendActivityStateAck (StateChangeResponse,
 *              TcuActivityState).
 */
enum class TcuActivityStateAck {
    SUSPEND_ACK,    /**< processed TcuActivityState::SUSPEND notification */
    SHUTDOWN_ACK,   /**< processed TcuActivityState::SHUTDOWN notification */
};

/** @} */ /* end_addtogroup telematics_power_manager */

}  // end of namespace power
}  // end of namespace telux

#endif // TELUX_POWER_TCUACTIVITYDEFINES_HPP
