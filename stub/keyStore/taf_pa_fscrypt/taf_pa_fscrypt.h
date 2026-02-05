/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef TAF_PA_FSCRYPT_H
#define TAF_PA_FSCRYPT_H

#ifdef TAF_PA_DEFAULT
#define TAF_PA_WEAK __attribute__((weak))
#else
#define TAF_PA_WEAK
#endif

#include "legato.h"
#include "interfaces.h"

//--------------------------------------------------------------------------------------------------
/**
 * FS-Crypt key definitions
 */
//--------------------------------------------------------------------------------------------------
#define FSC_MAX_KEY_SIZE 64

//--------------------------------------------------------------------------------------------------
/**
 * Reference to a key file object
 */
//--------------------------------------------------------------------------------------------------
typedef void* KeyMgt_KeyFileRef_t;

//--------------------------------------------------------------------------------------------------
/**
 * PA initialization.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED TAF_PA_WEAK void taf_pa_fsc_Init
(
    void* cryptoFunc
);

//--------------------------------------------------------------------------------------------------
/**
 * Get a key file reference by key name.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED TAF_PA_WEAK le_result_t taf_pa_fsc_GetKey
(
    le_msg_SessionRef_t clientSessionRef,   ///< [IN] Client session reference
    const char* dirName,                    ///< [IN] dir Name
    KeyMgt_KeyFileRef_t* keyFileRefPtr,     ///< [OUT] Key file reference.
    uint8_t* key,                           ///< [OUT] Raw key
    size_t keyLen                           ///< [OUT] Length of raw key
);

//--------------------------------------------------------------------------------------------------
/**
 * Create AES key and return a key file reference.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED TAF_PA_WEAK le_result_t taf_pa_fsc_GenerateAesKey
(
    le_msg_SessionRef_t clientSessionRef,   ///< [IN] Client session reference
    const char* dirName,                    ///< [IN] dir Name
    KeyMgt_KeyFileRef_t* keyFileRefPtr,     ///< [OUT] Key file reference
    uint8_t* key,                           ///< [OUT] Raw key
    size_t keyLen                           ///< [OUT] Length of raw key
);

//--------------------------------------------------------------------------------------------------
/**
 * Delete a key file.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED TAF_PA_WEAK le_result_t taf_pa_fsc_DeleteKey
(
    le_msg_SessionRef_t clientSessionRef, ///< [IN] Client session reference
    KeyMgt_KeyFileRef_t keyFileRef        ///< [IN] Key file reference
);

#endif // TAF_PA_FSCRYPT_H
