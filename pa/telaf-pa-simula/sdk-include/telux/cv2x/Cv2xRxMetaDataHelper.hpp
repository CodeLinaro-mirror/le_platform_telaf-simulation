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
 * Cv2x Rx packets helper class - Help to parse the received packets' meta
 * data at the beginning of the payload, any applications that have enabled
 * the received packets' meta data should use this helper class before using
 * the real payload.
 */

#ifndef TELUX_CV2X_CV2XRXMETADATAHELPER_HPP
#define TELUX_CV2X_CV2XRXMETADATAHELPER_HPP

#include <bitset>
#include <vector>
#include <memory>

#include <telux/common/CommonDefines.hpp>

namespace telux {
namespace cv2x {

/** @addtogroup telematics_cv2x_cpp
 * @{ */

/**
 * Specify set of RX Meta data that contribute to received packet's meta data report.
 * Used in @ref RxPacketMetaDataReport
 */
enum RxMetaDataValidityType {
    /**< Bit mask to specify whether sfn is valid in @ref RxPacketMetaDataReport */
    RX_SUBFRAME_NUMBER = (1 << 0),
    /**< Bit mask to specify whether subChannelIndex is valid in @ref RxPacketMetaDataReport */
    RX_SUBCHANNEL_INDEX = (1 << 1),
    /**< Bit mask to specify whether subChannelNum is valid in @ref RxPacketMetaDataReport */
    RX_SUBCHANNEL_NUMBER = (1 << 2),
    /**< Bit mask to specify whether rssi0 is valid in @ref RxPacketMetaDataReport */
    RX_PRX_RSSI = (1 << 3),
    /**< Bit mask to specify whether rssi1 is valid in @ref RxPacketMetaDataReport */
    RX_DRX_RSSI = (1 << 4),
    /**< Bit mask to specify whether l2DestinationId is valid in @ref RxPacketMetaDataReport */
    RX_L2_DEST_ID = (1 << 5),
    /**< Bit mask to specify whether sciFormat1Info is valid in @ref RxPacketMetaDataReport */
    RX_SCI_FORMAT1 = (1 << 6),
    /**< Bit mask to specify whether delayEstimation is valid in @ref RxPacketMetaDataReport */
    RX_DELAY_ESTIMATION = (1 << 7)
};

/*Bit mask containing bits from @ref RxMetaDataValidityType */
using RxMetaDataValidity = uint32_t;

/**
 * Contains the detailed meta data report of a packet received.
 *
 * The meta data report comes from the same data interface as the packet itself, it consist of
 * RF RSSI (received signal strength indicator) status, 32-bit SCI Format 1 (3GPP TS 36.213,
 * section 14.1), packet delay estimation, L2 destination ID, and the resource blocks used for
 * the packet's transmission: subframe, subchannel index.
 *
 * The meta data is always received after the successful receipt of the corresponding packet.
 * In order to associate the meta data report with the specific packet, the sfn and
 * subChannelIndex should be present in the packet's payload and the meta data report, so the
 * meta data report can be matched up to the packet.
 *
 * There is no guarantee that all items listed above will be presented, @ref metaDataMask
 * need to be used for the validity. Use @ref telux::cv2x::Cv2xRxMetaDataHelper::getRxMetaDataInfo
 * to extract the meta data.
 *
 **/
struct RxPacketMetaDataReport {
    RxMetaDataValidity metaDataMask; /**< Contains meta data validity */
    uint16_t sfn;                    /**< System Frame Number * 10 + subframe number */
    uint8_t subChannelIndex;         /**< The subchannel used for transmission */
    uint8_t subChannelNum;           /**< Number of subchannels in the Rx pool */
    int8_t prxRssi;                  /**< RSSI of PRx in dBm */
    int8_t drxRssi;                  /**< RSSI of DRx in dBm */
    uint32_t l2DestinationId;        /**< L2 destination ID */
    uint32_t sciFormat1Info;         /**< SCI format1, 3GPP TS 36.213 section 14.1 */
    int32_t delayEstimation;         /**< Packet delay estimation, in Ts (1/(15000 * 2048) seconds) */
};

/*
 * The received packet's meta data when presented start with 0xFF(START), end with 0x01(END),
 * between the START and END are meta data in form of TLVs, each TLV would have:
 * - 1 byte Type number
 * - 1 byte Length which specify how many bytes the value occupies
 * - Length bytes of the value
 *
 */

class Cv2xRxMetaDataHelper {
public:
    /*
     * Parse the rx meta data information from the payload. If the received packets'
     * meta data report is enabled (using @ref telux::cv2x::enableRxMetaDataReport),
     * use this method to extract the meta data report before processing the real cv2x
     * message in the payload.
     *
     * @param [in]  payload       - the pointer to the received packet's data
     * @param [in]  payloadLength - received packet's length
     * @param [out] metaDataLen   - meta data length parsed
     * @param [out] metaDatas     - Rx meta data reports parsed out
     *
     * @Returns SUCCESS if no error occurred.
     *
     */
    static telux::common::Status getRxMetaDataInfo(const uint8_t* payload, uint32_t payloadLength,
        size_t& metaDataLen, std::shared_ptr<std::vector<RxPacketMetaDataReport>> metaDatas);
};

/** @} */ /* end_addtogroup telematics_cv2x_cpp */

}  // end namespace cv2x
}  // end namespace telux

#endif // TELUX_CV2X_CV2XRXMETADATAHELPER_HPP
