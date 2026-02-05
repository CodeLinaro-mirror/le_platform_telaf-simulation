/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @page c_tafFwUpdate Firmware Update Service
 *
 * @ref taf_fwupdate_interface.h "API Reference"
 *
 * <HR>
 *
 * The Firmware Update service APIs are part of the Update service and support over-the-air (OTA)
 * firmware update solutions.
 *
 * @section taf_fwupdate_binding IPC interfaces binding
 *
 * The functions of this API are provided by the @b tafUpdateSvc service.
 *
 * The following example illustrates how to bind to the Firmware Update service:
 * @verbatim

   bindings:
   {
       clientExe.clientComponent.taf_fwupdate -> tafUpdateSvc.taf_fwupdate
   }

   @endverbatim
 *
 * @section c_taf_update_fota_function FOTA Functions
 *
 * - taf_fwupdate_RebootToActive() -- Reboots the target device to the active slot if firmware
 *   is installed successfully.
 *
 * - taf_fwupdate_GetFirmwareVersion() -- Gets the firmware version.
 *
 * - taf_fwupdate_Install() -- Supports firmware installation.
 *
 * <HR>
 *
 */


#include "legato.h"
#include "interfaces.h"


//--------------------------------------------------------------------------------------------------
/**
 * Reboots the target device to an active slot.
 */
//--------------------------------------------------------------------------------------------------
void taf_fwupdate_RebootToActive
(
    void
)
{
}
//--------------------------------------------------------------------------------------------------
/**
 * Gets the firmware version.
 *
 * @return
 *  - LE_BAD_PAREMETER -- Bad parameter(s).
 *  - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_fwupdate_GetFirmwareVersion
(
    char* version,
        ///< [OUT] Firmware version.
    size_t versionSize
        ///< [IN]
)
{
    return LE_NOT_IMPLEMENTED;
}
//--------------------------------------------------------------------------------------------------
/**
 * Installs firmware.
 *
 * @return
 *  - LE_OK -- Succeeded.
 *
 * @note Only used for local firmware update.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_fwupdate_Install
(
    void
)
{
    return LE_NOT_IMPLEMENTED;
}
