/*
 *  Copyright (c) 2017-2020, The Linux Foundation. All rights reserved.
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
 * @file       CardDefines.hpp
 * @brief      CardDefines provides the enumerations required for Card Services
 */

#ifndef TELUX_TEL_CARDDEFINES_HPP
#define TELUX_TEL_CARDDEFINES_HPP

#include <string>
#include <sstream>
#include <vector>

#include <telux/common/CommonDefines.hpp>

namespace telux {

namespace tel {

/** @addtogroup telematics_card
 * @{ */

/**
 * Defines all state of Card like absent, present etc
 */
enum class CardState {
   CARDSTATE_UNKNOWN = -1,   /**< Unknown card state */
   CARDSTATE_ABSENT = 0,     /**< Card is absent */
   CARDSTATE_PRESENT = 1,    /**< Card is present */
   CARDSTATE_ERROR = 2,      /**< Card is having error, either card is removed and not readable */
   CARDSTATE_RESTRICTED = 3, /**< Card is present but not usable due to carrier restrictions.*/
};

/**
 * Defines the reasons for error in CardState
 */
enum class CardError {
   UNKNOWN,             /**< Unknown error */
   POWER_DOWN,          /**< Power down */
   POLL_ERROR,          /**< Poll error */
   NO_ATR_RECEIVED,     /**< No ATR received */
   VOLT_MISMATCH,       /**< Volt mismatch */
   PARITY_ERROR,        /**< Parity error */
   POSSIBLY_REMOVED,    /**< Unknown, possibly removed */
   TECHNICAL_PROBLEMS,  /**< Card returned technical problems */
   NULL_BYTES,          /**< Card returned NULL bytes */
   SAP_CONNECTED,       /**< Terminal in SAP mode */
   CMD_TIMEOUT,         /**< Command timeout error */
};

/**
 * Defines all types of card locks which uses in PIN management APIs
 */
enum class CardLockType {
   PIN1 = 0, /**<Lock type is PIN1 */
   PIN2 = 1, /**<Lock type is PIN2 */
   PUK1 = 2, /**<Lock type is Pin Unblocking Key1 */
   PUK2 = 3, /**<Lock type is Pin Unblocking Key2 */
   FDN  = 4   /**<Lock type is Fixed Dialing Number */
};

/**
 * Defines all type of UICC application such as SIM, RUIM, USIM, CSIM and ISIM.
 */
enum AppType {
   APPTYPE_UNKNOWN = 0, /**< Unknown application type */
   APPTYPE_SIM = 1,     /**< UICC application type is SIM */
   APPTYPE_USIM = 2,    /**< UICC application type is USIM */
   APPTYPE_RUIM = 3,    /**< UICC application type is RSIM */
   APPTYPE_CSIM = 4,    /**< UICC application type is CSIM */
   APPTYPE_ISIM = 5,    /**< UICC application type is ISIM */
};

/**
 * Defines all application states.
 */
enum AppState {
   APPSTATE_UNKNOWN = 0,            /**< Unknown application state */
   APPSTATE_DETECTED = 1,           /**< Application state is detected */
   APPSTATE_PIN = 2,                /**< If PIN1 or UPin is required */
   APPSTATE_PUK = 3,                /**< If PUK1 or Puk for UPin is required */
   APPSTATE_SUBSCRIPTION_PERSO = 4, /**< PersoSubstate should be look at
                                         when application state is assigned to this value */
   APPSTATE_READY = 5,              /**< Application state is ready */
   APPSTATE_ILLEGAL = 6,            /**< Application state is illegal */
};

/**
 * The APDU response with status for transmit APDU operation.
 */
struct IccResult {
   int sw1;               /**< Status word 1 for command processing status */
   int sw2;               /**< Status word 2 for command processing qualifier */
   std::string payload;   /**< response as a hex string */
   std::vector<int> data; /**< vector of raw data received as part of response
                               to the card services request */
   const std::string toString() const {
      std::stringstream ss;
      ss << "sw1: " << sw1 << ", sw2: " << sw2 << ", payload: " << payload << ", data:";
      for(auto &i : data) {
         ss << i << " ";
      }
      return ss.str();
   };
};
/** @} */ /* end_addtogroup telematics_card */

/**
 * Defines structure of elementary file (EF).
 */
struct IccFile {
   uint16_t fileId;               /**< Elementary file identifier */
   std::string filePath;          /**< File path of the elementary file */
};


/**
 * Defines session types to route request to correct card on given slot and correct
 * application within the card.
 */
enum class SessionType {
    UNKNOWN = -1,               /**< Unknown refresh session type */
    PRIMARY = 0,                /**< Accesses the USIM application (for UICC) used to acquire cellular service network on primary slot. */
    SECONDARY = 2,              /**< Accesses the USIM application (for UICC) used to acquire cellular service network on secondary slot. */
    NONPROVISIONING_SLOT_1 = 4, /**< Accesses a nonprovisioning application available on the UICC in slot 1. The nonprovisioning application can be an ISIM or a USIM currently not used to acquire the network. The application is specified using the AID, as reported via telux::tel::ICardApp::getAppId. */
    NONPROVISIONING_SLOT_2 = 5, /**< Accesses a nonprovisioning application available on the UICC in slot 2. The nonprovisioning application can be an ISIM or a USIM currently not used to acquire the network. The application is specified using the AID, as reported via telux::tel::ICardApp::getAppId. */
    CARD_ON_SLOT_1 = 6,         /**< Accesses files that are not in any application of the card in slot 1. (i.e., to access the global phonebook or the EF-DIR). */
    CARD_ON_SLOT_2 = 7          /**< Accesses files that are not in any application of the card in slot 2. (i.e., to access the global phonebook or the EF-DIR). */
};

/**
 * Defines the session type and application identifier for SIM refresh so that routing
 * to the correct card and the correct application within the card can happen.
 */
struct RefreshParams {
   SessionType sessionType;      /**< Session type */
   std::string aid;              /**< Application identifier, used for telux::tel::SessionType::NONPROVISIONING_SLOT_1 or telux::tel::SessionType::NONPROVISIONING_SLOT_2 */
};

/**
 * Defines the stage of the card refresh procedure
 */
enum class RefreshStage {
    UNKNOWN = -1,              /**< Unknown refresh stage */
    WAITING_FOR_VOTES = 0,     /**< Waiting for the refresh action to be voted on. At this stage, the modem is awaiting votes from all clients participating in the voting process.*/
    STARTING = 1,              /**< Refresh procedure starting. */
    ENDED_WITH_SUCCESS = 2,    /**< Refresh ended successfully */
    ENDED_WITH_FAILURE = 3     /**< Refresh failed */
};

/**
 * Defines the card refresh mode
 */
enum class RefreshMode {
    UNKNOWN = -1,       /**< Unknown refresh mode. */
    RESET = 0,          /**< Reset the card and complete UICC initialization procedure is performed. */
    INIT = 1,           /**< Indicates the initialization of card application.*/
    INIT_FCN = 2,       /**< Indicates the initialization of card application and the elementary files(EFs) on the card application has changed. */
    FCN = 3,            /**< Indicates the elementary files(EFs) on the card application has changed. */
    INIT_FULL_FCN = 4,  /**< Combination of both INIT and full FCN, i.e., the card application is initialized and several elementary files (EFs) have been changed. */
    RESET_APP = 5,      /**< Reset UICC application and performs initialization of application. */
    RESET_3G = 6        /**< Reset 3G session. This mode is equivalent to INIT_FCN and additionally some applications procedure are followed at modem. */
};

}  // End of namespace tel

}  // End of namespace telux

#endif // TELUX_TEL_CARDDEFINES_HPP
