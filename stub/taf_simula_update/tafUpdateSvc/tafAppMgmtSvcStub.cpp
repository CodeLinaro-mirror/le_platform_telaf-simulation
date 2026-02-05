/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @page c_tafAppMgmt Application Management Service
 *
 * @ref taf_appMgmt_interface.h "API Reference"
 *
 * <HR>
 *
 * The Application Management service APIs help to control and obtain information about
 * applications.
 *
 * @section taf_appMgmt_binding IPC interfaces binding
 *
 * All functions of this API are provided by the @b tafUpdateSvc service.
 *
 * The following example shows how to bind to the application management service.
 * @verbatim

   bindings:
   {
       clientExe.clientComponent.taf_appMgmt -> tafUpdateSvc.taf_appMgmt
   }

   @endverbatim
 *
 * @section c_taf_appMgmt_control Application Control
 *
 * - taf_appMgmt_Start() starts an application.
 *
 * - taf_appMgmt_Stop() stops an application.
 *
 * - taf_appMgmt_Uninstall() uninstalls an application.
 *
 * @section c_taf_appMgmt_information Application Information Query
 *
 * Users can get application information through the app list using the following functions.
 *
 *  - taf_appMgmt_CreateAppList() / taf_appMgmt_DeleteAppList() creates and deletes app list
 *    references.
 *
 *  - taf_appMgmt_GetFirstApp() / taf_appMgmt_GetNextApp() get the first or next app reference.
 *
 *  - taf_appMgmt_GetAppDetails() gets the operator details.
 *
 *  - taf_appMgmt_GetState() gets the current application state.
 *
 * The following example illustrates how to obtain application information.
 *
 * @code
 *
 *   // Create list.
 *   listRef = taf_appMgmt_CreateAppList();
 *
 *   // Traverse list
 *   appRef = taf_appMgmt_GetFirstApp(listRef);
 *   while (appRef != nullptr) {
 *       taf_appMgmt_GetAppDetails(appRef, appInfoPtr);
 *
 *       // Do something with appInfoPtr.
 *
 *       appRef = taf_appMgmt_GetNextApp(listRef);
 *   }
 *
 *   // Delete list.
 *   taf_appMgmt_DeleteAppList(listRef);
 *
 *  @endcode
 *
 * <HR>
 *
 */


#include "legato.h"
#include "interfaces.h"


//--------------------------------------------------------------------------------------------------
/**
 * Gets the application's current state.
 *
 * @return
 *  - TAF_APPMGMT_STATE_STOPPED -- App is not running.
 *  - TAF_APPMGMT_STATE_STARTED -- App is running.
 */
//--------------------------------------------------------------------------------------------------
taf_appMgmt_AppState_t taf_appMgmt_GetState
(
    const char* LE_NONNULL appName
        ///< [IN] App name.
)
{
    return (taf_appMgmt_AppState_t) 0;
}
//--------------------------------------------------------------------------------------------------
/**
 * Gets the application's version.
 *
 * @return
 *  - LE_BAD_PARAMETER -- Bad parameter(s).
 *  - LE_NOT_FOUND -- App not found.
 *  - LE_OK -- Success.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_appMgmt_GetVersion
(
    const char* LE_NONNULL appName,
        ///< [IN] App name.
    char* version,
        ///< [OUT] App version.
    size_t versionSize
        ///< [IN]
)
{
    return LE_NOT_IMPLEMENTED;
}
//--------------------------------------------------------------------------------------------------
/**
 * Starts an app.
 *
 * @return
 *  - LE_DUPLICATE -- App is already running.
 *  - LE_BUSY -- System is busy.
 *  - LE_FAULT -- Failure.
 *  - LE_OK -- Success.
 *
 * @note Installed apps are activated in a probation state when started for the first time.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_appMgmt_Start
(
    const char* LE_NONNULL appName
        ///< [IN] App name.
)
{
    return LE_NOT_IMPLEMENTED;
}
//--------------------------------------------------------------------------------------------------
/**
 * Stops an app.
 *
 * @return
 *  - LE_NOT_FOUND -- App not found or not currently running; see note.
 *  - LE_OK -- Success.
 *
 * @note Installed apps are not stopped if they are running in a Probation state.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_appMgmt_Stop
(
    const char* LE_NONNULL appName
        ///< [IN] App name.
)
{
    return LE_NOT_IMPLEMENTED;
}
//--------------------------------------------------------------------------------------------------
/**
 * Uninstalls an app.
 *
 * @return
 *  - LE_NOT_FOUND -- App not found.
 *  - LE_BUSY -- System is busy.
 *  - LE_FAULT -- Failure.
 *  - LE_OK -- Success.
 *
 * @note Installed app is not allow to uninstall if in probation state.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_appMgmt_Uninstall
(
    const char* LE_NONNULL appName
        ///< [IN] App name.
)
{
    return LE_NOT_IMPLEMENTED;
}
//--------------------------------------------------------------------------------------------------
/**
 * Creates an app list.
 *
 * @return
 *  - Non-null pointer -- The reference to the app list.
 *  - Null pointer -- Internal error.
 */
//--------------------------------------------------------------------------------------------------
taf_appMgmt_AppListRef_t taf_appMgmt_CreateAppList
(
    void
)
{
    return NULL;
}
//--------------------------------------------------------------------------------------------------
/**
 * Deletes app list.
 *
 * @return
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_NOT_FOUND -- Reference not found.
 *  - LE_OK -- Success.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_appMgmt_DeleteAppList
(
    taf_appMgmt_AppListRef_t appListRef
        ///< [IN] The reference to the app list.
)
{
    return LE_NOT_IMPLEMENTED;
}
//--------------------------------------------------------------------------------------------------
/**
 * Gets the first app.
 *
 * @return
 *  - Non-null pointer -- The reference to the first app.
 *  - Null pointer -- Internal error or empty list.
 */
//--------------------------------------------------------------------------------------------------
taf_appMgmt_AppRef_t taf_appMgmt_GetFirstApp
(
    taf_appMgmt_AppListRef_t appListRef
        ///< [IN] The reference to the app list.
)
{
    return NULL;
}
//--------------------------------------------------------------------------------------------------
/**
 * Gets the next app based on the current position in the list.
 *
 * @return
 *  - Non-null pointer -- The reference to the next app.
 *  - Null pointer -- Internal error or empty list.
 */
//--------------------------------------------------------------------------------------------------
taf_appMgmt_AppRef_t taf_appMgmt_GetNextApp
(
    taf_appMgmt_AppListRef_t appListRef
        ///< [IN] The reference to the app list.
)
{
    return NULL;
}
//--------------------------------------------------------------------------------------------------
/**
 * Gets app information.
 *
 * @return
 *  - LE_BAD_PARAMETER -- Bad parameters.
 *  - LE_NOT_FOUND -- Reference not found.
 *  - LE_OK -- Success.
 *
 * @note Get static information when creating app list.
 */
//--------------------------------------------------------------------------------------------------
le_result_t taf_appMgmt_GetAppDetails
(
    taf_appMgmt_AppRef_t appRef,
        ///< [IN] The reference to the app.
    taf_appMgmt_AppInfo_t * appInfoPtr
        ///< [OUT] App information.
)
{
    return LE_NOT_IMPLEMENTED;
}
