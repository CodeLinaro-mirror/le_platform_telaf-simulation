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
 * @file      ECallDefines.hpp
 * @brief     ECallDefines contains enumerations and variables used for
 *            telephony subsystems.
 */
#ifndef TELUX_TEL_ECALLDEFINES_HPP
#define TELUX_TEL_ECALLDEFINES_HPP

#include <string>
#include <bitset>
#include <vector>
#include <cstdint>

namespace telux {

namespace tel {

/** @addtogroup telematics_phone
 * @{ */

/**
 * ECall Variant
 */
enum class ECallVariant {
   ECALL_TEST = 1,      /**< Initiate a test voice eCall with a configured telephone number stored
                           in the USIM. */
   ECALL_EMERGENCY = 2, /**< Initiate an emergency eCall. The trigger can be a manually initiated
                           eCall or automatically initiated eCall. */
   ECALL_VOICE = 4,     /**< Initiate a regular voice call with capability to transfer an MSD. */
};

/**
 * Emergency Call Type
 */
enum class EmergencyCallType {
   CALL_TYPE_ECALL = 12, /**<  eCall (0x0C) */
};

/**
 * MSD Transmission Status
 */
enum class ECallMsdTransmissionStatus {
   SUCCESS = 0,                  /**< In-band MSD transmission is successful */
   FAILURE = 1,                  /**< In-band MSD transmission failed */
   MSD_TRANSMISSION_STARTED = 2, /**< In-band MSD transmission started */
   NACK_OUT_OF_ORDER = 3,        /**< Out of order NACK message detected during in-band MSD
                                      transmission*/
   ACK_OUT_OF_ORDER = 4,         /**< Out of order ACK message detected during in-band MSD
                                      transmission*/
   START_RECEIVED = 5,           /**< SEND-MSD(START) is received and SYNC is locked during in-band
                                      MSD transmission*/
   LL_ACK_RECEIVED = 6,          /**< Link-Layer Acknowledgement(LL-ACK) is received during in-band
                                      MSD transmission*/
   OUTBAND_MSD_TRANSMISSION_STARTED = 10,    /**< Outband MSD transmission started in NG eCall */
   OUTBAND_MSD_TRANSMISSION_SUCCESS = 11,    /**< Outband MSD transmission succeeded in NG eCall
                                                  or Third Party Service (TPS) eCall */
   OUTBAND_MSD_TRANSMISSION_FAILURE = 12,    /**< Outband MSD transmission failed in NG eCall
                                                  or Third Party Service (TPS) eCall */
   LL_NACK_DUE_TO_T7_EXPIRY = 13,   /**< Link-Layer Acknowledgement(LL-NACK) is received during
                                         in-band MSD transmission due to expiry of T7 HLAP eCall
                                         timer */
   MSD_AL_ACK_CLEARDOWN = 14,   /**< Modem can cleardown the eCall after receipt of
                                     Application-Layer Acknowledgement(AL-LCK) during in-band MSD
                                     transmission */
};

/*
 * Represents reasons for performing redial of eCall or not.
 */

enum class ReasonType {
    NONE = 0,                   /**< Redial reason is NONE */
    CALL_ORIG_FAILURE = 1,      /**< Redial will be attempted due to eCall origination failure */
    CALL_DROP = 2,              /**< Redial will be attempted as the eCall is terminated before the
                                     reciept of MSD Transmission status */
    MAX_REDIAL_ATTEMPTED = 3,   /**< Redial will not be attempted as the maximum redial count
                                     is reached */
    CALL_CONNECTED = 4,         /**< Redial will not be attempted as the eCall was connected
                                     successfully. This notification
                                     @ref ICallListener:onECallRedial is triggered when application
                                     or PSAP terminates the eCall.*/
};

/*
 * Represents information about the redial eCall.
 */

struct ECallRedialInfo {
   bool willECallRedial; /**< Indicates whether redial of eCall will be attempted by modem or not */
   ReasonType reason; /**< Indicates the reason for redial of eCall to be performed or not */
};

/*
 * Represents the redial configuration type for eCall
 */

enum class RedialConfigType {
    CALL_DROP = 0,  /**< Redial configuration for eCall termination before reciept of MSD
                         Transmission status */
    CALL_ORIG = 1,  /**< Redial configuration for eCall origination failure */
};

/**
 * ECall category
 */
enum class ECallCategory {
   VOICE_EMER_CAT_AUTO_ECALL = 64, /**< Automatic emergency call */
   VOICE_EMER_CAT_MANUAL = 32,     /**< Manual emergency call */
};

/**
 * Represents a vehicle class as per European eCall MSD standard. i.e. EN 15722:2020.
 * Some of these values are only supported in certain MSD versions, so ensure to use supported
 * values in an MSD.
 * For example, TRAILERS_CLASS_O is not supported in MSD version-2 (as per A.1 in EN 15722:2015(E)),
 * but supported in in MSD version-3 (as per A.1 in EN 15722:2020).
 */
enum ECallVehicleType {
   PASSENGER_VEHICLE_CLASS_M1,
   BUSES_AND_COACHES_CLASS_M2,
   BUSES_AND_COACHES_CLASS_M3,
   LIGHT_COMMERCIAL_VEHICLES_CLASS_N1,
   HEAVY_DUTY_VEHICLES_CLASS_N2,
   HEAVY_DUTY_VEHICLES_CLASS_N3,
   MOTOR_CYCLES_CLASS_L1E,
   MOTOR_CYCLES_CLASS_L2E,
   MOTOR_CYCLES_CLASS_L3E,
   MOTOR_CYCLES_CLASS_L4E,
   MOTOR_CYCLES_CLASS_L5E,
   MOTOR_CYCLES_CLASS_L6E,
   MOTOR_CYCLES_CLASS_L7E,
   TRAILERS_CLASS_O,
   AGRI_VEHICLES_CLASS_R,
   AGRI_VEHICLES_CLASS_S,
   AGRI_VEHICLES_CLASS_T,
   OFF_ROAD_VEHICLES_G,
   SPECIAL_PURPOSE_MOTOR_CARAVAN_CLASS_SA,
   SPECIAL_PURPOSE_ARMOURED_VEHICLE_CLASS_SB,
   SPECIAL_PURPOSE_AMBULANCE_CLASS_SC,
   SPECIAL_PURPOSE_HEARCE_CLASS_SD,
   OTHER_VEHICLE_CLASS,
};

/**
 * Represents OptionalDataType class as per European eCall MSD standard. i.e. EN 15722.
 */
enum class ECallOptionalDataType {
   ECALL_DEFAULT,
};

/**
 * Represents the availability of some optional parameters in MSD as per European eCall MSD standard
 * EN 15722.
 */
struct ECallMsdOptionals {

   ECallOptionalDataType optionalDataType; /**< Type of optional data */
   bool optionalDataPresent;               /**< Availability of Optional data: true - Present or false - Absent */
   bool recentVehicleLocationN1Present;    /**< Availability of Recent Vehicle Location N1 data: true - Present or false - Absent. In MSD version-3 (as per EN 15722:2020), as recentVehicleLocationN1 is mandatory, this should be set to true by client*/
   bool recentVehicleLocationN2Present;    /**< Availability of Recent Vehicle Location N2 data: true - Present or false - Absent. In MSD version-3 (as per EN 15722:2020), as recentVehicleLocationN2 is mandatory, this should be set to true by client*/
   bool numberOfPassengersPresent;         /**< Availability of number of seat belts fastened data: true - Present or false - Absent*/
   ECallMsdOptionals() : optionalDataType(ECallOptionalDataType::ECALL_DEFAULT),
        optionalDataPresent(false),
        recentVehicleLocationN1Present(false),
        recentVehicleLocationN2Present(false),
        numberOfPassengersPresent(false) {
   }
};

/**
 * Represents ECallMsdControlBits structure as per European eCall MSD standard. i.e. EN 15722.
 */
struct ECallMsdControlBits {
   bool automaticActivation;  /**< auto / manual activation */
   bool testCall;             /**< test / emergency call */
   bool positionCanBeTrusted; /**< false if coincidence < 95% of reported pos within +/- 150m */
   ECallVehicleType vehicleType : 5; /**< Represents a vehicle class as per EN 15722 */
   ECallMsdControlBits() : automaticActivation(false),
        testCall(false),
        positionCanBeTrusted(false),
        vehicleType(ECallVehicleType::PASSENGER_VEHICLE_CLASS_M1) {
   }
};

/**
 * Represents VehicleIdentificationNumber structure as per European eCall MSD standard.
 * i.e. EN 15722. Vehicle Identification Number confirming ISO3779.
 */
struct ECallVehicleIdentificationNumber {
   std::string isowmi;          /**< World Manufacturer Index (WMI) */
   std::string isovds;          /**< Vehicle Type Descriptor (VDS) */
   std::string isovisModelyear; /**< Model year from Vehicle Identifier Section (VIS) */
   std::string isovisSeqPlant;  /**< Plant code + sequential number from VIS */
   ECallVehicleIdentificationNumber() : isowmi(""),
        isovds(""),
        isovisModelyear(""),
        isovisSeqPlant("") {
   }
};

/**
 * Represents VehiclePropulsionStorageType structure as per European eCall MSD standard.
 * i.e. EN 15722. Vehicle Propulsion type (energy storage):  True- Present, False - Absent
 */
struct ECallVehiclePropulsionStorageType {
   bool gasolineTankPresent;  /**< Represents the presence of Gasoline Tank in the vehicle. */
   bool dieselTankPresent;    /**< Represents the presence of Diesel Tank in the vehicle   */
   bool compressedNaturalGas; /**< Represents the presence of CNG in the vehicle   */
   bool liquidPropaneGas;     /**< Represents the presence of Liquid Propane Gas in the vehicle   */
   bool electricEnergyStorage; /**< Represents the presence of Electronic Storage in the vehicle */
   bool hydrogenStorage;       /**< Represents the presence of Hydrogen Storage in the vehicle   */
   bool otherStorage; /**< Represents the presence of Other types of storage in the vehicle   */
   ECallVehiclePropulsionStorageType() : gasolineTankPresent(false),
        dieselTankPresent(false),
        compressedNaturalGas(false),
        liquidPropaneGas(false),
        electricEnergyStorage(false),
        hydrogenStorage(false),
        otherStorage(false) {
   }
};

/**
 * Represents VehicleLocation structure as per European eCall MSD standard. i.e. EN 15722.
 */
struct ECallVehicleLocation {
   int32_t positionLatitude;  /**< latitude in milliarcsec, range is (-2147483648 to 2147483647) */
   int32_t positionLongitude; /**< longitude in milliarcsec, range is (-2147483648 to 2147483647) */
   ECallVehicleLocation() : positionLatitude(0),
        positionLongitude(0) {
   }
};

/**
 * Represents VehicleLocationDelta structure as per European eCall MSD standard. i.e. EN 15722.
 * Delta with respect to Current Vehicle location.
 */
struct ECallVehicleLocationDelta {
   int16_t latitudeDelta;  /**<  ( 1 Unit = 100 milliarcseconds, range: -512 to 511) */
   int16_t longitudeDelta; /**<  ( 1 Unit = 100 milliarcseconds, range: -512 to 511) */
   ECallVehicleLocationDelta() : latitudeDelta(0),
        longitudeDelta(0) {
   }
};

///@cond DEV
struct ECallObjectId {
   uint8_t id1 : 4;
   uint8_t id2 : 4;
   uint16_t id3 : 14;
   uint16_t id4 : 14;
   uint16_t id5 : 14;
   uint16_t id6 : 14;
   uint16_t id7 : 14;
   uint16_t id8 : 14;
   uint16_t id9 : 14;
};

struct ECallDefaultOptions {
   ECallObjectId objId;      /**< OBJECT IDENTIFIER data type according to ASN.1 specification */
   std::string optionalData; /**< Optional Data */
};
/// @endcond

/**
 * Defines the impact location of the triggering incident as per Euro NCAP Technical
 * Bulletin TB 040.
 */
enum class ECallLocationOfImpact {
   UNKNOWN,                 /**<  Location of impact is unknown. */
   NONE,                    /**<  No triggering impact detected. */
   FRONT,                   /**<  At front of the car. */
   REAR,                    /**<  At rear of the car. */
   DRIVER_SIDE,             /**<  At the driver side of the car. */
   NON_DRIVER_SIDE,         /**<  At the other side of the car. */
   OTHER,                   /**<  At an unspecified location. */
};

/**
 * Defines delta-v parameters as per Euro NCAP Technical Bulletin TB 040.
 */
struct ECallDeltaV {
   uint8_t rangeLimit;   /**< Upper limit of the detection range for delta-v. The range is unsigned integer[100 to 255]. */
   int16_t deltaVX;      /**< Difference in velocity just before and just after (start of the) triggering incident measured over the X-axis of the vehicle coordinate system. The range is signed integer[-255 to 255]. */
   int16_t deltaVY;      /**< Difference in velocity just before and just after (start of the) triggering incident measured over the Y-axis of the vehicle coordinate system. The range is signed integer[-255 to 255]. */
};

/**
 * Optional additional data information as per Euro NCAP Technical Bulletin TB 040.
 */
struct ECallOptionalEuroNcapData {
   ECallLocationOfImpact locationOfImpact;  /**< The impact location of the triggering incident. */
   bool rollOverDetectedPresent = false;    /**< Availability of rollover detected:
                                                 true - Present or false - Absent */
   bool rollOverDetected = false;           /**< (Optional) Omitted if vehicle is not able to detect a rollover, else true or false. */
   ECallDeltaV deltaV;                      /**< Difference between velocity just after and just before impact (delta-v). */
};

/**
 * Optional additional data information for the emergency rescue service.
 */
struct ECallOptionalPdu {
   ECallDefaultOptions eCallDefaultOptions; /**< Optional information. This field is unused and deprecated. Use the other fields below, instead. */
   std::string oid;                     /**< Relative object identifier(OID) as per European standard i.e. EN 15722 */
   std::vector<uint8_t> data;           /**< Optional additional data content. */
};

/**
 * Data structure to hold all details required to construct an MSD.
 * Supports MSD version-2(as per EN 15722:2015) and MSD version-3(as per EN 15722:2020)
 */
struct ECallMsdData {
   ECallMsdOptionals optionals; 
    /**< Indicates presence of optional data fields in ECall MSD.
         In MSD version-2 (as per EN 15722:2015), the following data
         fields are optional:
         recentVehicleLocationN1, recentVehicleLocationN2,
         numberOfPassengers and optionalAdditionalData.
         However, in MSD version-3 (as per EN 15722:2020), the
         following data fields are optional:
         numberOfOccupants (replacing numberOfPassengers) and
         optionalAdditionalData. */
   uint8_t messageIdentifier = 1;   /**< Starts with 1 for each new eCall and to be incremented with every retransmission */
   ECallMsdControlBits control; /**< ECallMsdControlBits structure as per European standard i.e. EN 15722 */
   ECallVehicleIdentificationNumber vehicleIdentificationNumber; /**< VIN (vehicle identification number) according to ISO3779 */
   ECallVehiclePropulsionStorageType vehiclePropulsionStorage;   /**< VehiclePropulsionStorageType structure as per European standard i.e. EN 15722 */
   uint32_t timestamp = 0;                   /**< Seconds elapsed since midnight 01.01.1970 UTC */
   ECallVehicleLocation vehicleLocation; /**< VehicleLocation structure as per European standard. i.e. EN 15722 */
   uint8_t vehicleDirection = 0; /**< Direction of travel in 2 degrees steps from magnetic north */

   ECallVehicleLocationDelta recentVehicleLocationN1; /**< Change in latitude and longitude compared to the last MSD transmission. Optional field for MSD version-2 */
   ECallVehicleLocationDelta recentVehicleLocationN2; /**< Change in latitude and longitude compared to the last but one MSD transmission. Optional field for MSD version-2 */
   uint8_t numberOfPassengers = 0;   /**< Number of occupants in the vehicle. Optional field for MSD version-2 and version-3 */
   /** Optional information for the emergency rescue service
    * (103 bytes, ASN.1 encoded); may also point to an address, where this information is located
    */
   ECallOptionalPdu optionalPdu; /**< Optional additional data information for the emergency rescue
                                      service. */
   uint8_t msdVersion = 2;       /**< MSD format version that is being used */
};

/**
 * Represents eCall operating mode
 */
enum class ECallMode {
   NORMAL = 0,     /**< eCall and normal voice calls are allowed */
   ECALL_ONLY = 1, /**< Only eCall is allowed */
   NONE = 2,       /**< Invalid mode */
};

/**
 * Represents eCall operating mode change reason
 */
enum class ECallModeReason {
   NORMAL = 0,      /**< eCall operating mode changed due to normal operation like
                        setting of eCall mode */
   ERA_GLONASS = 1, /**< eCall operating mode changed due to ERA-GLONASS operation */
};

/**
 *  Represents eCall operating mode information
 */
struct ECallModeInfo {
   ECallMode mode;         /**< Represents eCall operating mode */
   ECallModeReason reason; /**< Represents eCall operating mode change reason */
};

/**
 * Represents the status of an eCall High Level Application Protocol(HLAP) timer that is maintained
 * by the UE state machine.
 */
enum class HlapTimerStatus {
   UNKNOWN = -1,             /**< Unknown */
   INACTIVE,                 /**< eCall Timer is Inactive i.e it has not started or it has stopped/expired */
   ACTIVE,                   /**< eCall Timer is Active i.e it has started but not yet stopped/expired */
};

/**
 * Represents an event causing a change in the the status of eCall High Level Application Protocol
 * (HLAP) timer that is maintained by the UE state machine.
 *
 * The timer STARTED notification is provided when the timer moves from INACTIVE to ACTIVE state.
 * The timer STOPPED notification is provided when the timer moves from ACTIVE to INACTIVE state,
 * after its underlying condition is satisfied.
 * The timer EXPIRED notification is provided when the timer moves from ACTIVE to INACTIVE state,
 * after its underlying condition not satisfied until its timeout.
 * The timer RESUMED notification is provided when the application restarts the timer after events
 * like modem reset or a change of modem operating mode from low power mode to online using @ref
 * telux::tel::ICallManager::restartECallHlapTimer().
 */
enum class HlapTimerEvent {
   UNKNOWN = -1,             /**< Unknown */
   UNCHANGED,                /**< No change in timer status */
   STARTED,                  /**< eCall Timer is started */
   STOPPED,                  /**< eCall Timer is stopped */
   EXPIRED,                  /**< eCall Timer is expired */
   RESUMED,                  /**< eCall Timer is resumed. Applicable only for T9 and T10 timers */
};

/**
 * Represents status of various eCall High Level Application Protocol(HLAP) timers that are
 * maintained by UE state machine. This does not retrieve status of timers maintained by the PSAP.
 * The timers are represented according to EN 16062:2015 standard.
 */
struct ECallHlapTimerStatus {
   HlapTimerStatus t2;   /**< T2 Timer status */
   HlapTimerStatus t5;   /**< T5 Timer status */
   HlapTimerStatus t6;   /**< T6 Timer status */
   HlapTimerStatus t7;   /**< T7 Timer status */
   HlapTimerStatus t9;   /**< T9 Timer status */
   HlapTimerStatus t10;  /**< T10 Timer status */
};

/**
 * Represents events that changes the status of various eCall High Level Application Protocol(HLAP)
 * timers that are maintained by UE state machine. This does not retrieve events of timers
 * maintained by the PSAP.
 * The timers are represented according to EN 16062:2015 standard.
 */
struct ECallHlapTimerEvents {
   HlapTimerEvent t2;   /**< T2 Timer event */
   HlapTimerEvent t5;   /**< T5 Timer event */
   HlapTimerEvent t6;   /**< T6 Timer event */
   HlapTimerEvent t7;   /**< T7 Timer event */
   HlapTimerEvent t9;   /**< T9 Timer event */
   HlapTimerEvent t10;  /**< T10 Timer event */
};

/**
 * Represents custom SIP headers for content type and accept info for a PSAP.
 * This provides clients the ability to transfer custom SIP headers with the SIP INVITE
 * that is sent as part of call connect on TPS eCall over IMS.
 * The value corresponding to these data fields should be recognised by a PSAP
 * otherwise no acknowledgement would be received by device.
 */
struct CustomSipHeader {
   std::string contentType;    /**< Type of data being transmitted and should be filled as per RFC 8147 i.e MSD. Max Length 128 bytes */
   std::string acceptInfo;     /**< SIP Accept header. Max length 128 bytes */
};
static const std::string CONTENT_HEADER = "application/EmergencyCallData.eCall.MSD"; /**< Default
                                                     value for CustomSipHeader::contentType */

/**
 * Represents the type of an eCall High Level Application Protocol(HLAP) timer that is maintained
 * by the UE state machine.
 * The timers are represented according to EN 16062:2015 standard.
 */
enum class HlapTimerType {
   UNKNOWN_TIMER = 0,       /**< eCall unknown timer */
   T2_TIMER = 2,            /**< eCall T2 timer */
   T5_TIMER = 5,            /**< eCall T5 timer  */
   T6_TIMER = 6,            /**< eCall T6 timer  */
   T7_TIMER = 7,            /**< eCall T7 timer  */
   T9_TIMER = 9,            /**< eCall T9 timer  */
   T10_TIMER = 10,          /**< eCall T10 timer  */
};

/**
 * Configuration that represents the type of the number to be dialed when an automotive emergency
 * call is initiated.
 */
enum class ECallNumType {
   DEFAULT,         /* Default configured number is dialed */
   OVERRIDDEN,      /* User configured/overridden number is dialed */
};

/**
 * Defines the supported ECall configuration parameters
 */
enum EcallConfigType {
    ECALL_CONFIG_MUTE_RX_AUDIO,        /**< Mute the local audio device during MSD transmission */
    ECALL_CONFIG_NUM_TYPE,             /**< Decides which number needs to be dialed when an eCall
                                            is initiated */
    ECALL_CONFIG_OVERRIDDEN_NUM,       /**< User configured/overridden number that will be dialed
                                            for eCall */
    ECALL_CONFIG_USE_CANNED_MSD,       /**< Use the pre-defined MSD in modem for eCall */
    ECALL_CONFIG_GNSS_UPDATE_INTERVAL, /**< Time interval in milliseconds, at which modem updates
                                            the GNSS information in its internally generated MSD */
    ECALL_CONFIG_T2_TIMER,             /**< T2 timer value */
    ECALL_CONFIG_T7_TIMER,             /**< T7 timer value */
    ECALL_CONFIG_T9_TIMER,             /**< T9 timer value */
    ECALL_CONFIG_MSD_VERSION,          /**< MSD version to be used by modem when it internally
                                            generates MSD i.e when MSD is not sent by application
                                            and also canned MSD is not used */
    ECALL_CONFIG_COUNT,
};

/**
 * Represents timers that need to be restarted by the application after a modem reset or when the
 * operating mode of the device changes from low power mode to online.
 */
enum class EcallHlapTimerId {
    UNKNOWN = 0,                  /**< Unknown timer ID. */
    T9 = 5,                       /**< Timer ID for T9 timer for a regulatory eCall or test eCall. Applicable for both the eCall operating modes ( Normal and eCall only ). */
    T10 = 6,                      /**< Timer ID for T10 timer for a regulatory eCall or test eCall. Applicable for eCall only operating mode. */
};

/**
 * Bit mask that denotes which of the ECall configuration parameters defined in EcallConfigType
 * enum are valid(and to be considered) in the provided EcallConfig structure.
 * For example, if the configuration related to Canned MSD is provided, then
 * EcallConfigValidity valid = (1 << ECALL_CONFIG_USE_CANNED_MSD).
 */
using EcallConfigValidity = std::bitset<ECALL_CONFIG_COUNT>;

/**
 * Represents various configuration parameters related to automotive emergency call
 */
struct EcallConfig {
   EcallConfigValidity configValidityMask;   /**< Indicates the valid configuration parameters in the structure. A bit set to 1 denotes that the corresponding configuration parameter is valid */
   bool muteRxAudio;    /**< Mute the local audio device(ex: speaker) during MSD transmission */
   ECallNumType numType;    /**< Represents the type of number to be dialed when eCall is initiated */
   std::string overriddenNum; /**< User configured/overridden number that will be dialed when ECallNumType configuration parameter is set to OVERRIDE */
   bool useCannedMsd;   /**< Use the pre-defined MSD in modem for eCall */
   uint32_t gnssUpdateInterval; /**< Time interval in milliseconds at which the modem updates the GNSS information, in its internally generated MSD */
   uint32_t t2Timer;    /**< T2 timer value in milliseconds, according to EN 16062:2015 standard */
   uint32_t t7Timer;    /**< T7 timer value in milliseconds, according to EN 16062:2015 standard */
   uint32_t t9Timer;    /**< T9 timer value in milliseconds, according to EN 16062:2015 standard. Minimum value should be 3600000 */
   uint8_t msdVersion;  /**< MSD version to be used by modem when it internally generates MSD for transmission. Supported values are 1 and 2 only. This setting has no relevance when an eCall is initiated using @ref ICallManager APIs, which expects a valid MSD from the application */
};

/** @} */ /* end_addtogroup telematics_phone */

}  // End of namespace tel

}  // End of namespace telux

#endif // TELUX_TEL_ECALLDEFINES_HPP
