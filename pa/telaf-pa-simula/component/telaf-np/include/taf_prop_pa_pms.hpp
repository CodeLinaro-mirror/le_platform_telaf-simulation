/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef __TAF_PROP_PA_PMS_HPP__
#define __TAF_PROP_PA_PMS_HPP__

#include "taf_prop_common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    taf_prop_pa_pms_Result_OK,
    taf_prop_pa_pms_Result_FAULT,
    taf_prop_pa_pms_Result_TIMEOUT,
    taf_prop_pa_pms_Result_NOT_FOUND,
    taf_prop_pa_pms_Result_BAD_PARAMETER,
    taf_prop_pa_pms_Result_SVC_UNAVAILABLE,
    taf_prop_pa_pms_Result_NOT_IMPLEMENTED,

    taf_prop_pa_pms_Result_MAX,
} taf_prop_pa_pms_Result_t;

//--------------------------------------------------------------------------------------------------
/**
 * The reference type for no-ship pms Mpss active-object (opaque handle)
 */
//--------------------------------------------------------------------------------------------------
typedef struct taf_prop_pa_pms_Mpss * taf_prop_pa_pms_MpssRef_t;


//--------------------------------------------------------------------------------------------------
/**
 * Error codes for callback function
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    /* MPSS-side wakeup service is unavailable */
    TAF_PROP_PA_PMS_ERR_SVC_GONE = 0,

} taf_prop_pa_pms_ErrCode_t;

//--------------------------------------------------------------------------------------------------
/**
 * The collection of bitset about Modem wakeup sources
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    MODEM_WS_INCOMING_SMS     = 0x0001,
    MODEM_WS_INCOMING_VCALL   = 0x0002,
    MODEM_WS_SIM_PROFILE_SWAP = 0x0004,
    MODEM_WS_NAS_SYS_INFO     = 0x0008,

} taf_prop_pa_pms_ModemWakeupSource_t;

//--------------------------------------------------------------------------------------------------
/**
 * Parse the command-line arguments for options.
 */
//--------------------------------------------------------------------------------------------------
typedef void ( * taf_prop_pa_pms_ErrCallback )
(
    taf_prop_pa_pms_ErrCode_t errCode,
    void * errCbCtx
);

//--------------------------------------------------------------------------------------------------
/**
 * Initialize the MQTT bridge link and return the reference handle for other APIs.
 *
 * @return
 *  - taf_prop_pa_pms_Result_OK            Function succeeded.
 *  - taf_prop_pa_pms_Result_BAD_PARAMETER Bad parameters.
 *  - taf_prop_pa_pms_Result_DUPLICATE     Duplicated to call this function.
 *  - taf_prop_pa_pms_Result_FAULT         Failed to initialize the MQTT bridge link toward MPSS.
 *
 * @b NOTE: DO NOT call taf_prop_pa_pms_XXX APIs in callback function, otherwise deadlock will occur!
 */
//--------------------------------------------------------------------------------------------------
PROP_SHARED taf_prop_pa_pms_Result_t taf_prop_pa_pms_Init
(
    taf_prop_pa_pms_MpssRef_t * mpssRefPtr,  ///< [IN/OUT] Reference handle for active-object.
    taf_prop_pa_pms_ErrCallback errCbFn,     ///< [IN] Error notification callback function.
    void * errCbCtx                        ///< [IN] Callback function context.
);

//--------------------------------------------------------------------------------------------------
/**
 * Release the passed reference pointer (the MQTT bridge is process-shared).
 *
 * @return
 *  - taf_prop_pa_pms_Result_OK            Function succeeded.
 *  - taf_prop_pa_pms_Result_BAD_PARAMETER Bad parameters.
 *  - taf_prop_pa_pms_Result_NOT_PERMITTED Can not perform this function due to some bad preconditions.
 */
//--------------------------------------------------------------------------------------------------
PROP_SHARED taf_prop_pa_pms_Result_t taf_prop_pa_pms_Deinit
(
    taf_prop_pa_pms_MpssRef_t * mpssRefPtr   ///< [IN] Pointer for the recorded reference handle
);

//--------------------------------------------------------------------------------------------------
/**
 * Set the wakeup source filter(bitset) for specific MPSS services.
 *
 * @return
 *  - taf_prop_pa_pms_Result_OK            Function succeeded.
 *  - taf_prop_pa_pms_Result_BAD_PARAMETER Bad parameters.
 *  - taf_prop_pa_pms_Result_NOT_PERMITTED Can not perform this function due to some bad preconditions.
 *  - taf_prop_pa_pms_Result_TIMEOUT       No response received from MPSS within the RPC timeout.
 *  - taf_prop_pa_pms_Result_FAULT         MPSS rejected the bitset.
 */
//--------------------------------------------------------------------------------------------------
PROP_SHARED taf_prop_pa_pms_Result_t taf_prop_pa_pms_SetWsFilter
(
    taf_prop_pa_pms_MpssRef_t mpssRef,         ///< [IN] Recorded reference from _Init API.
    taf_prop_pa_pms_ModemWakeupSource_t bitset ///< [IN] Filter bitset for special indications.
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the wakeup source filter(bitset) from MPSS (the modem ground truth).
 *
 * @return
 *  - taf_prop_pa_pms_Result_OK            Function succeeded.
 *  - taf_prop_pa_pms_Result_BAD_PARAMETER Bad parameters.
 *  - taf_prop_pa_pms_Result_NOT_PERMITTED Can not perform this function due to some bad preconditions.
 *  - taf_prop_pa_pms_Result_TIMEOUT       No response received from MPSS within the RPC timeout.
 *  - taf_prop_pa_pms_Result_FAULT         MPSS failed to report the bitset.
 */
//--------------------------------------------------------------------------------------------------
PROP_SHARED taf_prop_pa_pms_Result_t taf_prop_pa_pms_GetWsFilter
(
    taf_prop_pa_pms_MpssRef_t mpssRef,          ///< [IN]  Recorded reference from _Init API.
    taf_prop_pa_pms_ModemWakeupSource_t *bitset ///< [OUT] Configured filter bitset in remote sevice
);

//--------------------------------------------------------------------------------------------------
/**
 * Enable all wakeup source indications for MPSS.
 *
 * @return
 *  - taf_prop_pa_pms_Result_OK            Function succeeded.
 *  - taf_prop_pa_pms_Result_BAD_PARAMETER Bad parameters.
 *  - taf_prop_pa_pms_Result_NOT_PERMITTED Can not perform this function due to some bad preconditions.
 *  - taf_prop_pa_pms_Result_TIMEOUT       No response received from MPSS within the RPC timeout.
 *  - taf_prop_pa_pms_Result_FAULT         Failed to enable all indications for MPSS side.
 */
//--------------------------------------------------------------------------------------------------
PROP_SHARED taf_prop_pa_pms_Result_t taf_prop_pa_pms_EnableAllWs
(
    taf_prop_pa_pms_MpssRef_t mpssRef
);

#ifdef __cplusplus
}
#endif

#endif
