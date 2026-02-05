/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @page c_tafUpdate Update Service
 *
 * @ref taf_update_interface.h "API Reference"
 *
 * <HR>
 *
 * The Update Service APIs are designed for over-the-air (OTA) solutions. The OTA package is encapsulated
 * with a QOTA header, which can be uploaded to the cloud server and downloaded to the target device.
 * In a QOTA header, the package can be marked as firmware or application bundle. The Update Service
 * supports installing firmware and applications and it parses the bundle when the OTA package is
 * downloaded on the target device. Users can register a handler to get detailed update progress.
 *
 * @note Currently update service does not support firmware and application updates at the same
 *       time. Users should install the OTA package after it's downloaded successfully.
 *
 * @section taf_update_binding IPC interfaces binding
 *
 * The functions of this API are provided by the @b tafUpdateSvc service.
 *
 * The following example illustrates how to bind to the Update Service.
 * @verbatim

   bindings:
   {
       clientExe.clientComponent.taf_update -> tafUpdateSvc.taf_update
   }

   @endverbatim
 *
 * @section c_taf_update_ota_function OTA Functions
 *
 * - taf_update_Download() -- Downloads a package from a cloud server.
 *
 * - taf_update_Install() -- Installs firmware or application on the target device.
 *
 * - taf_update_AddStateHandler() / taf_update_RemoveStateHandler() -- Adds/removes an update states handler.
 *
 *
 * The following example illustrates registering a handler for updated states.
 *
 * Use taf_update_AddStateHandler() to register the handler as a callback for update state and handle
 * the different state when an indication arrives.
 *
 * @code
 *
 *   void UpdateStateHandler(taf_update_StateInd_t* indication, void* contextPtr)
 *   {
 *       switch (indication->state) {
 *           case TAF_UPDATE_IDLE:
 *
 *               // Check download consent from user.
 *
 *               // Start to download.
 *               taf_update_Download();
 *
 *               break;
 *           case TAF_UPDATE_DOWNLOADING:
 *
 *               // Show download progress.
 *               LE_INFO("Downloading %d%% .", indication->percent);
 *
 *               break;
 *           case TAF_UPDATE_DOWNLOAD_FAIL:
 *
 *               LE_ERROR("Download fail.");
 *
 *               break;
 *           case TAF_UPDATE_DOWNLOAD_SUCCESS:
 *
 *               // Check install consent from user.
 *
 *               // Start to install.
 *               if (indication->ota == TAF_UPDATE_PACKAGE_FOTA) {
 *
 *                   LE_INFO("Install firmware.");
 *
 *                   LE_ASSERT(taf_update_Install(TAF_UPDATE_FOTA, indication->name) == LE_OK);
 *
 *               } else {
 *
 *                   LE_INFO("Install application.");
 *
 *                   // Show app name.
 *                   if (indication->name != NULL) {
 *                       LE_INFO("App name is %s", indication->name);
 *                       LE_ASSERT(taf_update_Install(TAF_UPDATE_SOTA, indication->name) == LE_OK);
 *                   }
 *               }
 *               break;
 *           case TAF_UPDATE_INSTALLING:
 *
 *               // Show install progress.
 *               LE_INFO("Installing %d%% .", indication->percent);
 *
 *               break;
 *           case TAF_UPDATE_INSTALL_SUCCESS:
 *
 *               // Reboot to active for firmware upgrade or start app probation.
 *
 *               break;
 *           case TAF_UPDATE_PROBATION:
 *
 *               // Do not perform any OTA operations during probation.
 *
 *               break;
 *        }
 *   }
 *
 *   // Register handler for update state.
 *   handlerRef = taf_update_AddStateHandler((taf_update_StateHandlerFunc_t)UpdateStateHandler, NULL);
 *   LE_ASSERT(handlerRef != NULL);
 *
 *   @endcode
 *
 * <HR>
 *
 */


#include "legato.h"
#include "interfaces.h"

static void StateLayeredHandler
(
    void* reportPtr,       ///< Indictaion to be reproted.
    void* layerHandlerFunc ///< Layered handler function
)
{
    taf_update_StateHandlerFunc_t handlerFunc =
        (taf_update_StateHandlerFunc_t)layerHandlerFunc;

    handlerFunc((taf_update_StateInd_t*)reportPtr, le_event_GetContextPtr());
}

typedef struct UpdateObject
{
    le_event_Id_t stateEvId;
    void (*stateLayberHandler)(void*, void*);
} UpdateObject_t;

static UpdateObject_t self;

//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for EVENT 'taf_update_State'
 *
 * Event to report update progress.
 */
//--------------------------------------------------------------------------------------------------
taf_update_StateHandlerRef_t taf_update_AddStateHandler
(
    taf_update_StateHandlerFunc_t handlerPtr,
        ///< [IN] Handler for update state.
    void* contextPtr
        ///< [IN]
)
{
    le_event_HandlerRef_t handlerRef = le_event_AddLayeredHandler("UpdateStateHandler",
                                                                  self.stateEvId,
                                                                  self.stateLayberHandler,
                                                                  (void*)handlerPtr);
    le_event_SetContextPtr(handlerRef, contextPtr);

    return (taf_update_StateHandlerRef_t)handlerRef;
}
//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'taf_update_State'
 */
//--------------------------------------------------------------------------------------------------
void taf_update_RemoveStateHandler
(
    taf_update_StateHandlerRef_t handlerRef
        ///< [IN]
)
{
    le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);
}
//--------------------------------------------------------------------------------------------------
/**
 * Downloads an OTA package from a cloud server.
 *
 * @note Update service will parse the download package and remove the QOTA header once the download
 *         is complete.
 */
//--------------------------------------------------------------------------------------------------
void taf_update_Download
(
    void
)
{
}
//--------------------------------------------------------------------------------------------------
/**
 * Installs an OTA package on the target device.
 *
 * @note QOTA header should be removed before calling this API.
 *
 * @return
 * - LE_FAULT -- Failed.
 * - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_update_Install
(
    taf_update_OTA_t ota,
        ///< [IN] OTA workflow.
    const char* LE_NONNULL name
        ///< [IN] OTA package name.
)
{
    return LE_NOT_IMPLEMENTED;
}
//--------------------------------------------------------------------------------------------------
/**
 * Gets download session.
 *
 * @return
 * - LE_FAULT -- Failed.
 * - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_update_GetDownloadSession
(
    const char* LE_NONNULL cfgFile,
        ///< [IN] Configuration file for download session.
    taf_update_SessionRef_t* sessionPtr
        ///< [OUT] Download session reference.
)
{
    return LE_NOT_IMPLEMENTED;
}
//--------------------------------------------------------------------------------------------------
/**
 * Starts download.
 *
 * @return
 * - LE_FAULT -- Failed.
 * - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_update_StartDownload
(
    taf_update_SessionRef_t session
        ///< [IN] Download session reference.
)
{
    return LE_NOT_IMPLEMENTED;
}
//--------------------------------------------------------------------------------------------------
/**
 * Pauses download.
 *
 * @return
 * - LE_FAULT -- Failed.
 * - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_update_PauseDownload
(
    taf_update_SessionRef_t session
        ///< [IN] Download session reference.
)
{
    return LE_NOT_IMPLEMENTED;
}
//--------------------------------------------------------------------------------------------------
/**
 * Resumes download.
 *
 * @return
 * - LE_FAULT -- Failed.
 * - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_update_ResumeDownload
(
    taf_update_SessionRef_t session
        ///< [IN] Download session reference.
)
{
    return LE_NOT_IMPLEMENTED;
}
//--------------------------------------------------------------------------------------------------
/**
 * Cancels download.
 *
 * @return
 * - LE_FAULT -- Failed.
 * - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_update_CancelDownload
(
    taf_update_SessionRef_t session
        ///< [IN] Download session reference.
)
{
    return LE_NOT_IMPLEMENTED;
}
//--------------------------------------------------------------------------------------------------
/**
 * Gets installation session.
 *
 * @return
 * - LE_FAULT -- Failed.
 * - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_update_GetInstallationSession
(
    taf_update_PackageType_t pkgType,
        ///< [IN] Package type for installation.
    const char* LE_NONNULL cfgFile,
        ///< [IN] Configuration file for installation session.
    taf_update_SessionRef_t* sessionPtr
        ///< [OUT] Installation session reference.
)
{
    return LE_OK;
}
//--------------------------------------------------------------------------------------------------
/**
 * Checks prerequisites for installation.
 *
 * @return
 * - LE_FAULT -- Failed.
 * - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_update_InstallPreCheck
(
    taf_update_SessionRef_t session,
        ///< [IN] Installation session reference.
    const char* LE_NONNULL manifest
        ///< [IN] Manifest for pre-check.
)
{
    return LE_NOT_IMPLEMENTED;
}
//--------------------------------------------------------------------------------------------------
/**
 * Start installation.
 *
 * @return
 * - LE_FAULT -- Failed.
 * - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_update_StartInstall
(
    taf_update_SessionRef_t session,
        ///< [IN] Installation session reference.
    const char* LE_NONNULL pkgPath
        ///< [IN] Package path.
)
{
    taf_update_StateInd_t report;
    report.percent = (uint32_t) 100;
    report.state = TAF_UPDATE_INSTALL_SUCCESS;
    le_utf8_Copy(report.name, "firmware update session", TAF_UPDATE_SESSION_NAME_LEN, NULL);

    le_event_Report(self.stateEvId, &report, sizeof(report));
    LE_INFO("[INSTALL] --> Done.(%s) report to handlers", pkgPath);

    return LE_OK;
}
//--------------------------------------------------------------------------------------------------
/**
 * Verification post installation, check if flashing properly on inactive bank.
 *
 * @return
 * - LE_FAULT -- Failed.
 * - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_update_InstallPostCheck
(
    taf_update_SessionRef_t session
        ///< [IN] Installation session reference.
)
{
    return LE_NOT_IMPLEMENTED;
}
//--------------------------------------------------------------------------------------------------
/**
 * Gets the active bank.
 *
 * @return
 * - LE_FAULT -- Failed.
 * - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_update_GetActiveBank
(
    taf_update_SessionRef_t session,
        ///< [IN] Installation session reference.
    taf_update_Bank_t* bankPtr
        ///< [OUT] The active bank.
)
{
    return LE_NOT_IMPLEMENTED;
}
//--------------------------------------------------------------------------------------------------
/**
 * Activation verification, check if each component has the updated version.
 *
 * @return
 * - LE_FAULT -- Failed.
 * - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_update_VerifyActivation
(
    taf_update_SessionRef_t session,
        ///< [IN] Installation session reference.
    const char* LE_NONNULL manifest
        ///< [IN] Manifest for activation verification.
)
{
    return LE_NOT_IMPLEMENTED;
}
//--------------------------------------------------------------------------------------------------
/**
 * Rollback to previous configurations to keep pesistency.
 *
 * @return
 * - LE_FAULT -- Failed.
 * - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_update_Rollback
(
    taf_update_SessionRef_t session
        ///< [IN] Installation session reference.
)
{
    return LE_NOT_IMPLEMENTED;
}
//--------------------------------------------------------------------------------------------------
/**
 * Bank synchronization.
 *
 * @return
 * - LE_FAULT -- Failed.
 * - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_update_Sync
(
    taf_update_SessionRef_t session
        ///< [IN] Installation session reference.
)
{
    return LE_NOT_IMPLEMENTED;
}
//--------------------------------------------------------------------------------------------------
/**
 * Resumes activation.
 *
 * @return
 * - LE_FAULT -- Failed.
 * - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_update_ResumeActivation
(
    taf_update_SessionRef_t session
        ///< [IN] Installation session reference.
)
{
    return LE_NOT_IMPLEMENTED;
}
//--------------------------------------------------------------------------------------------------
/**
 * Pause installation.
 *
 * @return
 * - LE_FAULT -- Failed.
 * - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_update_PauseInstall
(
    taf_update_SessionRef_t session
        ///< [IN] Installation session reference.
)
{
    return LE_NOT_IMPLEMENTED;
}
//--------------------------------------------------------------------------------------------------
/**
 * Resume installation.
 *
 * @return
 * - LE_FAULT -- Failed.
 * - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_update_ResumeInstall
(
    taf_update_SessionRef_t session
        ///< [IN] Installation session reference.
)
{
    return LE_NOT_IMPLEMENTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Cancel installation.
 *
 * @return
 * - LE_FAULT -- Failed.
 * - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_update_CancelInstall
(
    taf_update_SessionRef_t session
        ///< [IN] Installation session reference.
)
{
    return LE_NOT_IMPLEMENTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Erases a bank.
 *
 * @return
 * - LE_FAULT -- Failed.
 * - LE_OK -- Succeeded.
 * - LE_UNSUPPORTED -- Unsupported.
 *
 * @b NOTE: The SBL partition will not be erased.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_update_EraseBank
(
    taf_update_SessionRef_t session,
        ///< [IN] Installation session reference.
    taf_update_Bank_t bank
        ///< [IN] The bank to be erased.
)
{
    return LE_NOT_IMPLEMENTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Pauses activation.
 *
 * @return
 * - LE_FAULT -- Failed.
 * - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_update_PauseActivation
(
    taf_update_SessionRef_t session
        ///< [IN] Installation session reference.
)
{
    return LE_NOT_IMPLEMENTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Starts AB sync.
 *
 * @return
 * - LE_FAULT -- Failed.
 * - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_update_StartSync
(
    taf_update_SessionRef_t session
        ///< [IN] Sync session reference.
)
{
    return LE_NOT_IMPLEMENTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Pauses AB sync.
 *
 * @return
 * - LE_FAULT -- Failed.
 * - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_update_PauseSync
(
    taf_update_SessionRef_t session
        ///< [IN] Sync session reference.
)
{
    return LE_NOT_IMPLEMENTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Resumes AB sync.
 *
 * @return
 * - LE_FAULT -- Failed.
 * - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_update_ResumeSync
(
    taf_update_SessionRef_t session
        ///< [IN] Sync session reference.
)
{
    return LE_NOT_IMPLEMENTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Cancels AB sync.
 *
 * @return
 * - LE_FAULT -- Failed.
 * - LE_OK -- Succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_update_CancelSync
(
    taf_update_SessionRef_t session
        ///< [IN] Sync session reference.
)
{
    return LE_NOT_IMPLEMENTED;
}

COMPONENT_INIT
{
    self.stateEvId = le_event_CreateId("stateEvId", sizeof(taf_update_StateInd_t));
    self.stateLayberHandler = StateLayeredHandler;
    LE_INFO(" [simulation] %s --> DONE", __FUNCTION__);
}
