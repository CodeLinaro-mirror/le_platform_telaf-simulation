/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @page c_tafpm Power Management Service
 *
 * @rst :ref:`API reference <File taf_pm_interface.h>` @endrst
 *
 * <HR>
 *
 * Components need access to the TelAF Power Manager to control the system's wake-up state.
 * Operations that need fast response times (e.g., maintaining call state or playing/recording a
 * media stream) result in high interrupt rates; keeping the system awake results in better
 * performance and power efficiency.
 *
 * TelAF Power Manager keeps the system awake when at least one of the registered components
 * requests a wakeup source to be held. When all wakeup sources are released, the system may enter a
 * suspend state depending on the status of other (unrelated) wakeup sources.
 *
 * @section taf_pm_binding IPC interfaces binding
 *
 * The functions of this API are provided by the @b tafPMSvc application service.
 *
 * The following example illustrates how to bind to PM services.
 * @verbatim
   bindings:
   {
       clientExe.clientComponent.taf_pm -> tafPMSvc.taf_pm
   }
   @endverbatim
 *
 * @section taf_pm_request Requesting and releasing a wakeup source
 *
 * TelAF Power Manager service provides basic APIs for requesting and releasing a wakeup source.
 * TelAF Power Manager's clients call taf_pm_NewWakeupSource() to create a wakeup source. This
 * function returns a @ref taf_pm_WakeupSourceRef_t type that can later be used to acquire and
 * release a wakeup source through taf_pm_StayAwake() and taf_pm_Relax(), respectively. Wakeup
 * sources are not reference-counted, which means multiple calls to taf_pm_StayAwake() can be canceled
 * by a single call to taf_pm_Relax().
 *
 * To have a reference-counted wakeup-source, set the TAF_PM_REF_COUNT bit in the opts argument.
 * When this bit is set, each taf_pm_StayAwake() increments a counter and multiple calls to
 * taf_pm_Relax() are necessary to release the wakeup source.
 * @verbatim
   char tag[10];

   snprintf(tag, sizeof(tag), "testpm");
   taf_pm_WakeupSourceRef_t ref = taf_pm_NewWakeupSource(1, tag);

   for(int i = 0; i < 5; i++) {
       tafPMTest_acquire(ref);
   }
   for(int i = 0; i < 5; i++) {
       tafPMTest_release(ref);
   }
   @endverbatim
 *
 * TelAF Power Manager service will automatically release and delete all wakeup sources held on.
 * behalf of an exiting or disconnecting client.
 *
 * Use the StateChange to register a notification callback function to be called before device state
 * triggers.
 * @verbatim
   stateChangehandlerRef = taf_pm_AddStateChangeHandler(TestStateChangeHandler, NULL);
   @endverbatim
 *
 * Use the StateChangeEx to register a notification callback function to be called for every state
 * change trigger.
 * @verbatim
   handlerExRef = taf_pm_AddStateChangeExHandler(TestStateChangeExHandler, NULL);
   @endverbatim
 *
 * @section taf_pm_getMachineList Get the available Virtual Machines
 *
 * TelAF Power Manager Service provides APIs to know the available machines on the device.
 * TelAF Power Manager's client calls taf_pm_GetMachineList() to get the machine available reference.
 * This function returns a @ref taf_pm_VMListRef_t that can be later used to get the name of each
 * machine, and also to delete the list reference using taf_pm_DeleteMachineList().
 * @verbatim
   taf_pm_VMListRef_t vmListRef = taf_pm_GetMachineList( );
   char name[32] = {0};
   if(!vmListRef) {
       LE_ERROR("List is null");
   }
   le_result_t res = taf_pm_GetFirstMachineName(vmListRef, name, 32);
   while(res == LE_OK)
   {
       LE_INFO("vm name : %s",name);
       res = taf_pm_GetNextMachineName(vmListRef, name, 32);
   }
   res = taf_pm_DeleteMachineList(vmListRef);
   @endverbatim
 *
 * @section taf_pm_SendStateChangeAck Acknowledge on the state change notification
 *
 * TelAF Power Manager Service should know the acknowledgement of clients to proceed
 * with the state change.
 * TelAF Power Manager's client calls taf_pm_SendStateChangeAck()
 * to send the acknowledgement for state change.
 * @verbatim
   taf_pm_SendStateChangeAck(powerStateRef, state, TAF_PM_PVM, TAF_PM_READY);
   @endverbatim
 *
 * <HR>
 *
 */


#include "legato.h"
#include "interfaces.h"


//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for EVENT 'taf_pm_StateChange'
 *
 * TCU state change event of the local process.
 */
//--------------------------------------------------------------------------------------------------
taf_pm_StateChangeHandlerRef_t taf_pm_AddStateChangeHandler
(
    taf_pm_StateChangeHandlerFunc_t handlerPtr,
        ///< [IN] The state change handler.
    void* contextPtr
        ///< [IN]
)
{
    LE_INFO("[Simulation TestDouble]: %s", __FUNCTION__);
    return NULL;
}
//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'taf_pm_StateChange'
 */
//--------------------------------------------------------------------------------------------------
void taf_pm_RemoveStateChangeHandler
(
    taf_pm_StateChangeHandlerRef_t handlerRef
        ///< [IN]
)
{
    LE_INFO("[Simulation TestDouble]: %s", __FUNCTION__);
}
//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for EVENT 'taf_pm_StateChangeEx'
 *
 * TCU state change event of the local process.
 *
 * @b NOTE: This is supported only on SA525M.
 */
//--------------------------------------------------------------------------------------------------
taf_pm_StateChangeExHandlerRef_t taf_pm_AddStateChangeExHandler
(
    taf_pm_StateChangeExHandlerFunc_t handlerPtr,
        ///< [IN] The PmMasterWake source change handler.
    void* contextPtr
        ///< [IN]
)
{
    LE_INFO("[Simulation TestDouble]: %s", __FUNCTION__);
    return NULL;
}
//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'taf_pm_StateChangeEx'
 */
//--------------------------------------------------------------------------------------------------
void taf_pm_RemoveStateChangeExHandler
(
    taf_pm_StateChangeExHandlerRef_t handlerRef
        ///< [IN]
)
{
    LE_INFO("[Simulation TestDouble]: %s", __FUNCTION__);
}
//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for EVENT 'taf_pm_ConsolidatedAckInfo'
 *
 * ConsolidatedAckInfo event.
 */
//--------------------------------------------------------------------------------------------------
taf_pm_ConsolidatedAckInfoHandlerRef_t taf_pm_AddConsolidatedAckInfoHandler
(
    taf_pm_ConsolidatedAckInfoHandlerFunc_t handlerPtr,
        ///< [IN] The ConsolidatedAck Info handler.
    void* contextPtr
        ///< [IN]
)
{
    return NULL;
}
//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'taf_pm_ConsolidatedAckInfo'
 */
//--------------------------------------------------------------------------------------------------
void taf_pm_RemoveConsolidatedAckInfoHandler
(
    taf_pm_ConsolidatedAckInfoHandlerRef_t handlerRef
        ///< [IN]
)
{
}
//--------------------------------------------------------------------------------------------------
/**
 * Creates a new wakeup source.
 *
 * @return
 *      - Reference to wakeup source (to be used later for acquiring/releasing).
 *      - Null when attempt failed.
 */
//--------------------------------------------------------------------------------------------------
taf_pm_WakeupSourceRef_t taf_pm_NewWakeupSource
(
    uint32_t createOpts,
        ///< [IN] Wakeup source options.
    const char* LE_NONNULL wsTag
        ///< [IN] Context-specific wakeup source tag.
)
{
    return NULL;
}
//--------------------------------------------------------------------------------------------------
/**
 * Acquires a passed wakeup source reference.
 *
 * @return
 *     - LE_OK             Wakeup source successfully acquired.
 *     - LE_BAD_PARAMETER  Invalid wakeup source reference.
 *     - LE_FAULT          Failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_pm_StayAwake
(
    taf_pm_WakeupSourceRef_t wsRef
        ///< [IN] Reference to the wakeup source to be acquired.
)
{
    return LE_NOT_IMPLEMENTED;
}
//--------------------------------------------------------------------------------------------------
/**
 * Releases a previously acquired wakeup source.
 *
 * @return
 *     - LE_OK             Wakeup source successfully released.
 *     - LE_BAD_PARAMETER  Invalid wakeup source reference.
 *     - LE_FAULT          Failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_pm_Relax
(
    taf_pm_WakeupSourceRef_t wsRef
        ///< [IN] Reference to the wakeup source to be released.
)
{
    return LE_NOT_IMPLEMENTED;
}
//--------------------------------------------------------------------------------------------------
/**
 * Gets the current system state of the local process.
 *
 * @return
 * State of the system.
 */
//--------------------------------------------------------------------------------------------------
taf_pm_State_t taf_pm_GetPowerState
(
    void
)
{
    LE_INFO("[Simulation TestDouble]: %s", __FUNCTION__);
    return (taf_pm_State_t) TAF_PM_STATE_RESUME;
}
//--------------------------------------------------------------------------------------------------
/**
 ** Sets the power state to all the Virtual Machines.
 **
 ** @return
 **     - LE_OK          Successfully changed the state of device.
 **     - LE_FAULT       Failed.
 **
 ** @b NOTE: This is supported only on SA525M.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_pm_SetAllVMPowerState
(
    taf_pm_State_t state
        ///< [IN] State to be set.
)
{
    return LE_NOT_IMPLEMENTED;
}
//--------------------------------------------------------------------------------------------------
/**
 ** Sets the power state to the respective Virtual Machine.
 **
 ** @return
 **     - LE_OK          Successfully changed the state of Virtual Machine.
 **     - LE_FAULT       Failed.
 **
 ** @b NOTE: This is supported only on SA525M.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_pm_SetVMPowerState
(
    taf_pm_State_t state,
        ///< [IN] State to be set.
    const char* LE_NONNULL machineName
        ///< [IN] Virtual Machine name of the state to be changed.
)
{
    return LE_NOT_IMPLEMENTED;
}
//--------------------------------------------------------------------------------------------------
/**
 * Gets the list of available Virtual Machines.
 *
 * @return
 *     - non-null pointer -- The Virtual Machines List reference.
 *     - null pointer -- Internal error.
 *
 * @b NOTE: This is supported only on SA525M.
 */
//--------------------------------------------------------------------------------------------------
taf_pm_VMListRef_t taf_pm_GetMachineList
(
    void
)
{
    return NULL;
}
//--------------------------------------------------------------------------------------------------
/**
 * Gets the first Virtual Machine name.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Bad parameters.
 *     - LE_NOT_FOUND -- Empty list.
 *
 * @b NOTE: This is supported only on SA525M.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_pm_GetFirstMachineName
(
    taf_pm_VMListRef_t vmListRef,
        ///< [IN] Virtual Machine list reference.
    char* name,
        ///< [OUT] The name of the Virtual Machine.
    size_t nameSize
        ///< [IN]
)
{
    return LE_NOT_IMPLEMENTED;
}
//--------------------------------------------------------------------------------------------------
/**
 * Gets the next Virtual Machine name based on the current position in the list.
 *
 * @return
 *     - LE_OK -- Succeeded.
 *     - LE_BAD_PARAMETER -- Bad parameters.
 *     - LE_NOT_FOUND -- The end of the list.
 *
 * @b NOTE: This is supported only on SA525M.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_pm_GetNextMachineName
(
    taf_pm_VMListRef_t vmListRef,
        ///< [IN] Virtual Machine list reference.
    char* name,
        ///< [OUT] The name of the Virtual Machine.
    size_t nameSize
        ///< [IN]
)
{
    return LE_NOT_IMPLEMENTED;
}
//--------------------------------------------------------------------------------------------------
/**
 *  Deletes the Vitual Machine list.
 *
 * @return
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_OK -- Succeeded.
 *
 * @b NOTE: This is supported only on SA525M.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_pm_DeleteMachineList
(
    taf_pm_VMListRef_t vmLsitRef
        ///< [IN] Virtual Machine list reference.
)
{
    return LE_NOT_IMPLEMENTED;
}
//--------------------------------------------------------------------------------------------------
/**
 *  Receives the acknowledgement from clients for state change.
 *
 * @b NOTE: This is supported only on SA525M.
 */
//--------------------------------------------------------------------------------------------------
void taf_pm_SendStateChangeAck
(
    taf_pm_PowerStateRef_t powerStateRef,
        ///< [IN] Power state reference for statechangehandler.
    taf_pm_State_t state,
        ///< [IN] The system state.
    taf_pm_NadVm_t vm,
        ///< [IN] The VM ID of the machine.
    taf_pm_ClientAck_t ack
        ///< [IN] The acknowledgement type.
)
{
}
//--------------------------------------------------------------------------------------------------
/**
 *  Gets the information of nacked clients for a state change.
 *
 * @b NOTE: This is supported only on SA525M.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_pm_GetNackClientInfo
(
    taf_pm_ConsolidatedAckInfoRef_t consolidatedAckInfoRef,
        ///< [IN] The reference of the consolidated
        ///< acknowledgement information.
    taf_pm_ClientInfo_t* nackClientsPtr,
        ///< [OUT] The nacked clients.
    size_t* nackClientsSizePtr
        ///< [INOUT]
)
{
    return LE_NOT_IMPLEMENTED;
}
//--------------------------------------------------------------------------------------------------
/**
 *  Gets the information of unresponsive clients for a state change.
 *
 * @b NOTE: This is supported only on SA525M.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_pm_GetUnrespClientInfo
(
    taf_pm_ConsolidatedAckInfoRef_t consolidatedAckInfoRef,
        ///< [IN] The reference of the consolidated
        ///< acknowledgement information.
    taf_pm_ClientInfo_t* unrespClientsPtr,
        ///< [OUT] The unresponsive clients.
    size_t* unrespClientsSizePtr
        ///< [INOUT]
)
{
    return LE_NOT_IMPLEMENTED;
}
//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for EVENT 'taf_pm_ModemAwake'
 *
 * Modem awake event.
 *
 * @instaging
 */
//--------------------------------------------------------------------------------------------------
taf_pm_ModemAwakeHandlerRef_t taf_pm_AddModemAwakeHandler
(
    taf_pm_ModemAwakeHandlerFunc_t handlerPtr,
        ///< [IN] The modem awake event handler.
    void* contextPtr,
        ///< [IN]
    taf_pm_NodeModemWsBitMask_t wsBitmask
        ///< [IN]
)
{
    return NULL;
}
//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'taf_pm_ModemAwake'
 */
//--------------------------------------------------------------------------------------------------
void taf_pm_RemoveModemAwakeHandler
(
    taf_pm_ModemAwakeHandlerRef_t handlerRef
        ///< [IN]
)
{
}

//--------------------------------------------------------------------------------------------------
/**
 *  Sets the power mode to facilitate the system level power management.
 *
 * @b NOTE: This is supported only on SA525M.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_pm_SetPowerMode
(
    taf_pm_PowerMode_t powerMode
        ///< [IN] to set system power mode when BUB status has changed.
)
{
    return LE_NOT_IMPLEMENTED;
}
//--------------------------------------------------------------------------------------------------
/**
 * Sets the whitelisted modem wakeup selection.
 *
 * @b NOTE: This is supported only on SA525M.
 *
 * @instaging
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_pm_SetModemWakeupSel
(
    taf_pm_NodeModemWsBitMask_t wsBitmask
        ///< [IN] Modem wakeup selection to be whitelisted.
)
{
    return LE_NOT_IMPLEMENTED;
}
//--------------------------------------------------------------------------------------------------
/**
 * Gets the whitelisted modem wakeup selection.
 *
 * @b NOTE: This is supported only on SA525M.
 *
 * @instaging
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_pm_GetModemWakeupSel
(
    taf_pm_NodeModemWsBitMask_t* wsBitmaskPtr
        ///< [OUT] Modem wakeup selection to be whitelisted.
)
{
    return LE_NOT_IMPLEMENTED;
}
//--------------------------------------------------------------------------------------------------
/**
 * Gets the last modem wakeup reason.
 *
 * @b NOTE: This is supported only on SA525M.
 *
 * @instaging
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_pm_GetModemAwakeReason
(
    taf_pm_NodeModemWsBitMask_t* wsBitmaskPtr
        ///< [OUT] Modem wakeup reason.
)
{
    return LE_NOT_IMPLEMENTED;
}



COMPONENT_INIT
{
    LE_INFO("[Simulation TestDouble]: Power Manager Service started");
}
