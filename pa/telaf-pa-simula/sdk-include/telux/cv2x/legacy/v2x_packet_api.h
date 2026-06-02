/*
 *  Copyright (c) 2021, The Linux Foundation. All rights reserved.
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
  @file v2x_packet_api.h

  @addtogroup telematics_cv2x_c_packet
  Provide utilities and structures for CV2X packet analysis.
 */

#ifndef TELUX_CV2X_LEGACY_V2X_PACKET_API_H
#define TELUX_CV2X_LEGACY_V2X_PACKET_API_H

#include "telux/cv2x/legacy/v2x_common.pb.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @addtogroup telematics_cv2x_c_packet
@{ */

/**
 * meata data validity mask of each received packet, used by @ref rx_packet_meta_data_t
 */
#define META_DATA_MASK_SFN 0x01
#define META_DATA_MASK_SUB_CHANNEL_INDEX 0x02
#define META_DATA_MASK_SUB_CHANNEL_NUM   0x04
#define META_DATA_MASK_PRX_RSSI 0x08
#define META_DATA_MASK_DRX_RSSI 0x10
#define META_DATA_MASK_L2_DEST 0x20
#define META_DATA_MASK_SCI_FORMAT1 0x40
#define META_DATA_MASK_DELAY_ESTI 0x80

/**
 * Contains the detailed meta data report of a packet received.
 *
 **/
typedef struct {
    uint32_t validity;         /**< Validity of the meta data */
    uint16_t sfn;              /**< System Frame Number * 10 + subframe number */
    uint8_t sub_channel_index; /**< The subchannel used for transmission */
    uint8_t sub_channel_num;   /**< Number of subchannels in the Rx pool */
    int8_t prx_rssi;           /**< RSSI of primary receive signal, in dBm */
    int8_t drx_rssi;           /**< RSSI of diversity receive signal, in dBm */
    uint32_t l2_destination_id;/**< L2 destination ID */
    uint32_t sci_format1_info; /**< SCI format1, 3GPP TS 36.213 section 14.1 */
    int32_t delay_estimation;  /**< Packet delay estimation, in Ts (1/(15000 * 2048) seconds) */
} rx_packet_meta_data_t;

/**
     Parse the received packet's meta data from the payload

     @datatypes
     @ref rx_packet_meta_data_t

     @param[in]      payload       Pointer to the received message which may contains
                                   the meta data reports
     @param[in]      length        Length of the received message in byte
     @param[out]     meta_data     Pointer to the meta data structure array
     @param[in,out]  num           array size of meta_data as input,
                                   be assigned to the number of meta data reports parsed out.
                                   The caller can use this value to index the array meta_data.
     @param[out]     meta_data_len length of the meta data in byte parsed out from the payload

     @detdesc
     This function extracts the received packet's meta data from the payload, there maybe several
     meta data reports in the received payload.

     @return
     #V2X_STATUS_SUCCESS.
     @par
     Otherwise:
     - #V2X_STATUS_FAIL -- Other failure.
 */
extern v2x_status_enum_type v2x_parse_rx_meta_data(const uint8_t *payload, uint32_t length,
    rx_packet_meta_data_t *meta_data, size_t *num, size_t *meta_data_len);

/** @} *//* end_addtogroup telematics_cv2x_c_packet */

#ifdef __cplusplus
}
#endif

#endif // TELUX_CV2X_LEGACY_V2X_PACKET_API_H
