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
 * @file      CellBroadcastDefines.hpp
 * @brief     CellBroadcastDefines contains enumerations and variables used for
 *            cellbroadcast functionality.
 */
#ifndef TELUX_TEL_CELLBROADCASTDEFINES_HPP
#define TELUX_TEL_CELLBROADCASTDEFINES_HPP

namespace telux {

namespace tel {

/** @addtogroup telematics_cellbroadcast
 * @{ */

/**
 * Defines cellbroadcast message filter. Refer spec 3GPP TS 23.041 9.4.1.2.2
 * for message identifier.
 * Eg: If user want to receive from 0x1112 to 0x1116 then, startMessageId is 0x1112
 *     and endMessageId is 0x1116.
 * If user want to receive only 0x1112, then both startMessageId and endMessageId is 0x1112.
 */
struct CellBroadcastFilter {
   int startMessageId; /**< Intended to receive start from which MessageType */
   int endMessageId;   /**< Intended to receive upto which MessageType */
};

/**
 * Defines geographical scope of cell broadcast.
 */
enum class GeographicalScope {
   CELL_WIDE_IMMEDIATE = 0, /**< Cell wide geographical scope with immediate display */
   PLMN_WIDE = 1,           /**< PLMN wide geographical scope */
   LA_WIDE = 2,             /** Location / Service/ Tracking area wide geographical
                                scope (GSM/UMTS/E-UTRAN/NG-RAN). */
   CELL_WIDE = 3            /**< Cell wide geographical scope */
};

/**
 * Defines priority for cell broadcast message.
 */
enum class MessagePriority {
   UNKNOWN = -1,        /**< Unknown message priority */
   NORMAL = 0,          /**< Normal message priority */
   EMERGENCY = 1        /**< Emergency message priority */
};

/**
 * Defines message type for cell broadcast message.
 */
enum class MessageType {
   UNKNOWN = -1,    /**< Unknown message type */
   ETWS = 0,        /**< Earthquake and Tsunami Warning System */
   CMAS = 1         /**< Commercial Mobile Alert System */
};

/**
 * Defines warning type for ETWS cell broadcast message.
 */
enum class EtwsWarningType {
   UNKNOWN = -1,                 /**< Unknown ETWS warning type */
   EARTHQUAKE = 0,               /**< ETWS warning type for earthquake */
   TSUNAMI = 1,                  /**< ETWS warning type for tsunami */
   EARTHQUAKE_AND_TSUNAMI = 2,   /**< ETWS warning type for earthquake and tsunami */
   TEST_MESSAGE = 3,             /**< ETWS warning type for test messages */
   OTHER_EMERGENCY = 4,          /**< ETWS warning type for other emergency types */
};

/**
 * Defines message class for CMAS cell broadcast message.
 */
enum class CmasMessageClass {
   UNKNOWN = -1,                       /**< CMAS category for warning types that are reserved for future extension */
   PRESIDENTIAL_LEVEL_ALERT = 0,       /**< Presidential-level alert (Korean Public Alert System Class 0 message) */
   EXTREME_THREAT = 1,                 /**< Extreme threat to life and property (Korean Public Alert System Class 1 message)*/
   SEVERE_THREAT = 2,                  /**< Severe threat to life and property (Korean Public Alert System Class 1 message). */
   CHILD_ABDUCTION_EMERGENCY = 3,      /**< Child abduction emergency (AMBER Alert) */
   REQUIRED_MONTHLY_TEST = 4,          /**< CMAS test message */
   CMAS_EXERCISE = 5,                  /**< CMAS exercise */
   OPERATOR_DEFINED_USE = 6,           /**< CMAS category for operator defined use */
};

/**
 * Defines severity type for CMAS cell broadcast message.
 */
enum class CmasSeverity {
   UNKNOWN = -1,        /**< CMAS alert severity is unknown. The severity is available for all
                            GSM/UMTS alerts except for the Presidential-level alert class
                            (Korean Public Alert System Class 0). */
   EXTREME = 0,         /**< Extraordinary threat to life or property */
   SEVERE = 1,          /**< Significant threat to life or property */
};

/**
 * Defines urgency type for CMAS cell broadcast message.
 */
enum class CmasUrgency {
   UNKNOWN = -1,         /**< CMAS alert urgency is unknown. The urgency is available for all
                            GSM/UMTS alerts except for the Presidential-level alert class
                            (Korean Public Alert System Class 0). */
   IMMEDIATE = 0,         /**< Responsive action should be taken immediately */
   EXPECTED = 1,          /**< Responsive action should be taken within the next hour */
};

/**
 * Defines certainty type for CMAS cell broadcast message.
 */
enum class CmasCertainty {
   UNKNOWN = -1,        /**< CMAS alert certainty is unknown. The certainty is available for all
                            GSM/UMTS alerts except for the Presidential-level alert class
                            (Korean Public Alert System Class 0). */
   OBSERVED = 0,         /**< Determined to have occurred or to be ongoing. */
   LIKELY = 1,           /**< Likely (probability > ~50%) */
};

/**
 * Defines geometry type specified in wireless emergency alert.
 */
enum class GeometryType {
   UNKNOWN = -1,    /**< Unknown geometry type */
   POLYGON = 0,     /**< Polygon geometry type */
   CIRCLE = 1       /**< Circle geometry type */
};



/** @} */ /* end_addtogroup telematics_cellbroadcast */

}  // End of namespace tel

}  // End of namespace telux

#endif // TELUX_TEL_CELLBROADCASTDEFINES_HPP
