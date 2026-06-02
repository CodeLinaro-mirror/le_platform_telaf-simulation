/*
 *  Copyright (c) 2020, The Linux Foundation. All rights reserved.
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
  @file v2x_config_api.h

  @addtogroup telematics_cv2x_c_config
  Abstraction of the CV2X configuration relevant interfaces.
 */

#ifndef TELUX_CV2X_LEGACY_V2X_CONFIG_API_H
#define TELUX_CV2X_LEGACY_V2X_CONFIG_API_H

#include "v2x_common.pb.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @addtogroup telematics_cv2x_c_config
@{ */

/**
    V2X configuration source types listed in ascending order of priority.
    The system always uses the V2X configuration with the highest priority
    if multiple V2X configuration sources exist.
 */
typedef enum{
    V2X_CONFIG_SOURCE_UNKNOWN = 0u,   /**< V2X config file source is unknown */
    V2X_CONFIG_SOURCE_PRECONFIG = 1u, /**< V2X config file source is preconfig */
    V2X_CONFIG_SOURCE_SIM_CARD = 2u,  /**< V2X config file source is SIM card */
    V2X_CONFIG_SOURCE_OMA_DM = 4u,    /**< V2X config file source is OMA-DM */
}v2x_config_soure_t;

/**
    Events relevant to CV2X config file.
 */
typedef enum{
    V2X_CONFIG_EVENT_CHANGED = 0u,  /**< V2X config file is changed */
    V2X_CONFIG_EVENT_EXPIRED = 1u,  /**< V2X config file is expired */
}v2x_config_event_t;

/**
    Information about any update to a V2X config file.
 */
typedef struct {
    v2x_config_soure_t source;       /**< The type of the V2X config file. */
    v2x_config_event_t event;        /**< Config file event. @newpagetable */
} v2x_config_event_info_t;

/**
    Callback made when the v2x config file changes.

    @datatypes
    #v2x_config_event_info_t

    @param[in] config   V2X config file information.

    @newpage
*/
typedef void (*cv2x_config_event_listener)(v2x_config_event_info_t info);


/**
    Register listener for any updates to CV2X configuration.

    @datatypes
    cv2x_config_event_listener

    @param[in] callback        Callback function of @ref cv2x_config_event_listener
                               structure that is used during CV2X config events
                               (such as CV2X configuration expriy or changed). \n
                               @vertspace{3}

    @detdesc
    This function should be called before calling @ref v2x_update_configuration
    or @ref v2x_retrieve_configuration if the caller has interest in the notification
    of V2X configuration events.
    @par
    V2X_CONFIG_EVENT_CHANGED @ref v2x_config_event_t is sent to registered CV2X
    configuration listeners if the content of the active V2X configuration file is
    changed by calling @ref v2x_update_configuration or the active V2X configuration
    file source @ref v2x_config_soure_t is changed from one type to another.
    @par
    V2X_CONFIG_EVENT_EXPIRED @ref v2x_config_event_t is sent to registered CV2X
    configuration listeners when the active V2X configuration file is expired.

    @return
    #V2X_STATUS_SUCCESS.
    @par
    #V2X_STATUS_FAIL -- If there is an error. @newpage
 */
v2x_status_enum_type v2x_register_for_config_change_ind(cv2x_config_event_listener callback);

/**
    Updates the OMA-DM V2X radio configuration file.

    On platforms with access control enabled, the caller needs to have TELUX_CV2X_CONFIG
    permission to successfully invoke this API.

    @param[in] config_file_path  Pointer to the path of the configuration file.

    @detdesc
    The V2X radio status must be INACTIVE. If the V2X status is
    ACTIVE or SUSPENDED (see #v2x_event_t), call stop_v2x_mode() first.
    @par
    The functionality of V2X configuration expiration is supported by adding an expiration
    leaf to the V2X configuration file passed in. When the active configuration expires,
    the system fallbacks to a lower priority V2X configuration @ref v2x_config_soure_t
    if existed. If the V2X stauts is active, it changes to suspended when the active V2X
    configuration expires and then changes to active after the system fallbacks to a lower
    priority V2X configuration or changes to inactive if no V2X configuration is available.
    @par
    This call is a blocking call. When it returns the configuration has
    been updated, assuming no error.

    @return
    #V2X_STATUS_SUCCESS.
    @par
    Otherwise:
    - #V2X_STATUS_EALREADY -- Failure because V2X status is not #V2X_INACTIVE.
    - #V2X_STATUS_FAIL -- Other failure.

    @dependencies
    V2X radio status must be #V2X_INACTIVE (#v2x_event_t). @newpage
 */
extern v2x_status_enum_type v2x_update_configuration(const char *config_file_path);

/**
    Retrieve the V2X radio configuration file.

    On platforms with access control enabled, the caller needs to have TELUX_CV2X_CONFIG
    permission to successfully invoke this API.

    @param[in] config_file_path  Pointer to the path of the configuration file.

    @par
    This call is a blocking call. When it returns the configuration has
    been read out, assuming no error.

    @return
    #V2X_STATUS_SUCCESS.
    @par
    Otherwise:
    - #V2X_STATUS_FAIL -- Other failure.
 */
extern v2x_status_enum_type v2x_retrieve_configuration(const char *config_file_path);

/** @} *//* end_addtogroup telematics_cv2x_c_config */

#ifdef __cplusplus
}
#endif

#endif // TELUX_CV2X_LEGACY_V2X_CONFIG_API_H
