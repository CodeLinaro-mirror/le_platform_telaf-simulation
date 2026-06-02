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
 * @file  TcuActivityManager.hpp
 * @brief Provides APIs to change power state (suspend/resume/shutdown) of the machine(s) and
 *        manage activities. Defines interfaces for registering listeners for the power state
 *        changes, and handling acknowledgments from the clients. A master client can trigger
 *        state change, while slave clients respond to this change. It supports multiple
 *        machines and integrates with the platform's power management framework to manage
 *        machine state changes effectively.
 */

#ifndef TELUX_POWER_TCUACTIVITYMANAGER_HPP
#define TELUX_POWER_TCUACTIVITYMANAGER_HPP

#include <future>
#include <memory>
#include <vector>

#include <telux/common/CommonDefines.hpp>
#include <telux/power/TcuActivityListener.hpp>

namespace telux {
namespace power {

/** @addtogroup telematics_power_manager
 * @{ */

/**
 * @brief   ITcuActivityManager provides interface to register and de-register listeners to get
 *          TCU-activity state updates. And also API to initiate TCU-activity state transition.
 *
 * An application can get the appropriate TCU-activity manager (i.e. @ref ClientType::SLAVE or
 * @ref ClientType::MASTER) object from the power factory. The TCU-activity manager configured as
 * the @ref ClientType::MASTER is responsible for triggering state transitions. TCU-activity manager
 * configured as a @ref ClientType::SLAVE is responsible for listening to state change indications
 * and acknowledging when it performs necessary tasks and prepares for the state transition. A
 * machine in this power management framework represents an application processor subsystem or a
 * host/guest virtual machine on hypervisor based platforms.
 *
 * - Only one @ref ClientType::MASTER is allowed in the system. This master can exist only on
 *   the primary/host machine and not on the guest virtual machine or an external application
 *   processor (EAP).
 * - It is expected that all processes interested in a TCU-activity state change should register as
 *   @ref ClientType::SLAVE.
 * - When the @ref ClientType::MASTER changes the TCU-activate state, @ref ClientType::SLAVEs
 *   connected to the impacted machine are notified.
 * - @ref ClientType::MASTER can trigger the TCU-activity state change of a specific machine or all
 *   machines at once.
 * - If the @ref ClientType::SLAVE wants to differentiate between a state change indication that is
 *   the result of a trigger for all machines or a trigger for its specific machines, it can be
 *   detected using the machine name provided in the listener API.
 * - When the @ref ClientType::MASTER triggers an all machines TCU-activity state change, only the
 *   machines that are not in the desired state will undergo the state transition, and the
 *   @ref ClientType::SLAVEs to those machines will be notified.
 * - In the case of
 *   - @ref TcuActivityState::SUSPEND or @ref TcuActivityState::SHUTDOWN trigger:
 *      - After becoming ready for state change, all @ref ClientType::SLAVE should acknowledge back.
 *      - The @ref ClientType::MASTER will get notification about the consolidated acknowledgement
 *        status of all @ref ClientType::SLAVEs.
 *      - On getting a successful consolidated acknowledgement from all the @ref ClientType::SLAVE
 *        for the suspend trigger, the power framework allows the respective machine to suspend. On
 *        getting a successful consolidated acknowledgement from all the @ref ClientType::SLAVEs for
 *        the shutdown trigger, the power framework triggers the respective machine shutdown without
 *        waiting further.
 *      - If the @ref ClientType::SLAVE sends a NACK to indicate that it is not ready for state
 *        transition or fails to acknowledge before the configured time, then the
 *        @ref ClientType::MASTER will get to know via a consolidated/slave acknowledgement status
 *        notification.
 *      - In such failed cases, if the @ref ClientType::MASTER wants to stop the state transition
 *        considering the information in the consolidated acknowledgement, then the
 *        @ref ClientType::MASTER is allowed to trigger a new TCU-activity state change, or else the
 *        state transition will proceed after the configured timeout.
 *   - @ref TcuActivityState::RESUME trigger:
 *      - Power framework will prevent the respective machine from going into suspend.
 *      - No acknowledgement will be required from @ref ClientType::SLAVE and the
 *        @ref ClientType::MASTER will not be getting consolidated/slave acknowledgement as machine
 *        will be already resumed.
 *
 * When the application is notified about the service being unavailable, the TCU-activity state
 * notifications will be inactive. After the service becomes available, the existing listener
 * registrations will be maintained.
 */
class ITcuActivityManager {
 public:

   /**
    * Gets the power management service's functional status.
    *
    * @returns @ref telux::common::ServiceStatus
    */
   virtual telux::common::ServiceStatus getServiceStatus() = 0;

    /**
     * Registers the listener to receive power state change, machine availability update
     * and response from the slave clients.
     *
     * @param[in] listener Receives updates
     *
     * @returns @ref telux::common::Status::SUCCESS if the listener is registered,
     *          otherwise, an appropriate error code
     */
    virtual telux::common::Status registerListener(
        std::weak_ptr<ITcuActivityListener> listener) = 0;

    /**
     * Deregisters the given listener registered previously with
     * @ref ITcuActivityManager::registerListener().
     *
     * @param[in] listener Listener to deregister
     *
     * @returns @ref telux::common::Status::SUCCESS if the listener is deregistered,
     *          otherwise, an appropriate error code
     */
    virtual telux::common::Status deregisterListener(
        std::weak_ptr<ITcuActivityListener> listener) = 0;

    /**
     * Register the given listener to listen power management service's functional
     * status change.
     *
     * @param[in] listener Receives status change updates
     *
     * @returns @ref telux::common::Status::SUCCESS if the listener is registered,
     *          otherwise, an appropriate error code
     */
    virtual telux::common::Status registerServiceStateListener(
        std::weak_ptr<telux::common::IServiceStatusListener> listener) = 0;

    /**
     * Deregisters the given listener registered previously with
     * @ref ITcuActivityManager::registerServiceStateListener().
     *
     * @param[in] listener Listener to deregister
     *
     * @returns @ref telux::common::Status::SUCCESS if the listener is deregistered,
     *          otherwise, an appropriate error code
     */
    virtual telux::common::Status deregisterServiceStateListener(
        std::weak_ptr<telux::common::IServiceStatusListener> listener) = 0;

    /**
     * Gets machine's platform name on which the caller process is running. It can be used
     * to identify the local machine on a platform with multiple machines registered with
     * the platform's power management framework.
     *
     * @param[out] machineName Machine name (example, qcom,mdm or qcom,eap, etc.)
     *
     * @returns @ref telux::common::Status::SUCCESS if the machine name is obtained,
     *          otherwise, an appropriate error code
     */
    virtual telux::common::Status getMachineName(std::string &machineName) = 0;

    /**
     * Provides name of all the machines currently registered with the platform's power management
     * framework.
     *
     * A master client can identify a particular machine by its name and then it can alter
     * the state of that particular machine by passing the machine name to the setActivityState()
     * API.
     *
     * @param[out] machineNames List of machine names
     *
     * @returns @ref telux::common::Status::SUCCESS if all machine names are obtained,
     *          otherwise, an appropriate error code
     */
    virtual telux::common::Status getAllMachineNames(std::vector<std::string> &machineNames) = 0;

    /**
     * Initiates transition to the power state specified by @ref TcuActivityState.
     *
     * Used by the master client only.
     *
     * On platforms with access control enabled, caller needs to have TELUX_POWER_CONTROL_STATE
     * permission to invoke this API successfully.
     *
     * Guest VM can only be suspended/resumed. It cannot be shut down using this API.
     *
     * @param[in] state Power state to which to change
     *
     * @param[in] machineName @ref LOCAL_MACHINE if the machine on which client is running should
     *                        enter this state, @ref ALL_MACHINES if all the machines should enter
     *                        this state, else a specific machine (obtained from getMachineName()
     *                        API) that should enter this state.
     *
     * @param[in] callback Optional, receives telux::common::ErrorCode::SUCCESS if the power
     *                     management framework confirms that it has received state transition
     *                     request
     *
     * @returns @ref telux::common::Status::SUCCESS if the state transition is initiated,
     *          otherwise, an appropriate error code
     */
    virtual telux::common::Status setActivityState(TcuActivityState state,
        std::string machineName, telux::common::ResponseCallback callback = nullptr) = 0;

    /**
     * For the master client, gives the @ref TcuActivityState of the machine
     * specified by machineName.
     *
     * For the slave client, gives the @ref TcuActivityState of the local machine.
     *
     * @param[in] machineName Machine's name for which the @ref TcuActivityState
     *                        is requested. Machine names can be obtained
     *                        through @ref getAllMachines().
     *
     * @param[out] state      Current @ref TcuActivityState for the given machineName
     *                        specified by @ref TcuActivityState.
     *
     * @returns          @ref telux::common::ErrorCode::SUCCESS if the state is retrieved,
     *                   @ref telux::common::ErrorCode::OPERATION_NOT_ALLOWED if a slave client
     *                   tries to get state of a machine other than the one they're running on,
     *                   an appropriate error code otherwise.
     *
     * @note Eval: This is a new API and is being evaluated. It is subject
     *              to change and could break backwards compatibility.
    */
    virtual telux::common::ErrorCode getActivityState(std::string machineName,
        TcuActivityState &state) = 0;


    /**
     * When a slave client receives notification in ITcuActivityListener::onTcuActivityStateUpdate,
     * it must acknowledge whether it agrees to enter the new power state or not through this
     * method. Based on this response, platform power management framework takes the next
     * appropriate step.
     *
     * Must be called only once per slave client irrespective of the number of listeners it
     * registered.
     *
     * There is no need to send response for @ref TcuActivityState::RESUME state.
     *
     * @param[in] ack @ref StateChangeResponse::ACK to agree to enter the new state, else
     *                @ref StateChangeResponse::NACK to deny
     *
     * @param[in] state State for which this acknowledgement is sent
     *
     * @returns @ref telux::common::Status::SUCCESS if the acknowledgement is sent, otherwise,
     *          an appropriate error code
     */
    virtual telux::common::Status sendActivityStateAck(
        StateChangeResponse ack, TcuActivityState state) = 0;

    /**
     * Explicitly enables/disables certain behavior in the modem peripheral subsystem (MPSS)
     * to conserve power. For example, specific functionalities like LTE and 5G search or
     * measurement are scaled down.
     *
     * Primarily intended for the master clients, should be used cautiously, as it could affect
     * WWAN functionalities.
     *
     * On platforms with access control enabled, caller needs to have TELUX_POWER_CONTROL_STATE
     * permission to invoke this API successfully.
     *
     * @param[in] state For @ref TcuActivityState::SUSPEND functionalities are throttled,
     *                  for @ref TcuActivityState::RESUME functionalities are unthrottled
     *
     * @returns @ref telux::common::Status::SUCCESS if the new state is set, otherwise,
     *          an appropriate error code
     */
    virtual telux::common::Status setModemActivityState(TcuActivityState state) = 0;

    /**
     * Returns true if the power management service is functionally ready, false otherwise.
     *
     * @returns True if service is ready, false otherwise
     *
     * @deprecated Use ITcuActivityManager::getServiceStatus() API.
     *             @ref telux::power::ITcuActivityManager::getServiceStatus
     */
    virtual bool isReady() = 0;

    /**
     * Provides a mechanism to wait for the power management service to be functionally
     * ready.
     *
     * @returns Future object on which the caller can wait
     *
     * @deprecated Use InitResponseCb in PowerFactory::getTcuActivityManager instead.
     */
    virtual std::future<bool> onReady() = 0;

    /**
     * Initiates a TCU-activity state transition.
     * If the platform is configured to change the modem activity state automatically when the TCU
     * activity state is changed, this API initiates the relevant internal operation.
     *
     * This API needs to be used cautiously, as it could change the power-state of the system and
     * may affect other processes.
     *
     * This API should only be invoked by a client that has instantiated the ITcuActivityManager
     * instance using ClientType::MASTER
     *
     * On platforms with access control enabled, the caller needs to have TELUX_POWER_CONTROL_STATE
     * permission to invoke this API successfully.
     *
     * @param[in] state        TCU-activity state that the system is intended to enter
     *
     * @param[in] callback     Optional callback to get the response for the TCU-activity state
     *                          transition command
     *
     * @returns Status of setActivityState i.e. success or suitable status code.
     *
     * @note    This API should not be used on virtual machines or on systems with hypervisor. The
     *          alternative API @ref setActivityState( TcuActivityState state,
     *          std::string machineName = "",telux::common::ResponseCallback callback = nullptr)
     *          should be used.
     *
     * @deprecated  Use @ref setActivityState(TcuActivityState state, std::string machineName,
     *              telux::common::ResponseCallback) instead.
     */
    virtual telux::common::Status setActivityState( TcuActivityState state,
        telux::common::ResponseCallback callback = nullptr) = 0;

    /**
     * When a slave client receives notification in ITcuActivityListener::onTcuActivityStateUpdate,
     * it must acknowledge whether it agrees to enter the new power state or not through this
     * method. Based on this response, power management framework takes the next appropriate step.
     *
     * Must be called only once per slave client irrespective of the number of listeners it
     * registered.
     *
     * @param[in] ack @ref StateChangeResponse::ACK to agree to enter the new state
     *
     * @returns @ref telux::common::Status::SUCCESS if the acknowledgement is sent, otherwise,
     *          an appropriate error code
     *
     * @deprecated  Use @ref sendActivityStateAck( TcuActivityState state,
     *              StateChangeResponse ack) instead.
     */
    virtual telux::common::Status sendActivityStateAck(TcuActivityStateAck ack) = 0;

    /**
     * Gets the current @ref TcuActivityState of the machine where the client is running on.
     *
     * @returns @ref TcuActivityState- the current TcuActivityState of the machine
     *                                 where the client is running on.
     *
     * @deprecated Use @ref getActivityState(std::string machineName,
     *             TcuActivityState &state) instead.
     */
    virtual TcuActivityState getActivityState() = 0;

    /**
     * Destructor of ITcuActivityManager.
     */
    virtual ~ITcuActivityManager(){};
};

/** @} */ /* end_addtogroup telematics_power_manager */

}  // end of namespace power
}  // end of namespace telux

#endif // TELUX_POWER_TCUACTIVITYMANAGER_HPP
