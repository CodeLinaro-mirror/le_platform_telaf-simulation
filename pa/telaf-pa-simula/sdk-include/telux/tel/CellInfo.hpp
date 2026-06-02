/*
 *  Copyright (c) 2018, 2021 The Linux Foundation. All rights reserved.
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
 * @file       CellInfo.hpp
 * @brief      CellInfo is a container class for information about a Cell such as
 *             Cell Identify and Signal Strength.
 */

#ifndef TELUX_TEL_CELLINFO_HPP
#define TELUX_TEL_CELLINFO_HPP

#include <memory>
#include <string>

#include "telux/tel/SignalStrength.hpp"

namespace telux {

namespace tel {

/** @addtogroup telematics_phone
 * @{ */

/**
 * Defines all the cell info types.
 */
enum class CellType {
   GSM = 1,
   CDMA = 2,
   LTE = 3,
   WCDMA = 4,
   TDSCDMA = 5,
   NR5G = 6,
   NB1_NTN = 7,
};

/**
 * GsmCellIdentity class provides methods to get mobile country code, mobile network
 * code, location area code, cell identity, absolute RF channel number and
 * base station identity code.
 */
class GsmCellIdentity {
public:
   GsmCellIdentity(std::string mcc, std::string mnc, int lac, int cid, int arfcn, int bsic);
   /**
    * Get the Mobile Country Code.
    *
    * @returns Mcc value.
    *
    * @deprecated Use getMobileCountryCode() API instead
    */
   const int getMcc();

   /**
    * Get the Mobile Network Code.
    *
    * @returns Mnc value.
    *
    * @deprecated Use getMobileNetworkCode() API instead
    */
   const int getMnc();

   /**
    * Get the Mobile Country Code.
    *
    * @returns Mcc value.
    *
    */
   const std::string getMobileCountryCode();

   /**
    * Get the Mobile Network Code.
    *
    * @returns Mnc value.
    *
    */
   const std::string getMobileNetworkCode();

   /**
    * Get the location area code.
    *
    * @returns Location area code.
    */
   const int getLac();

   /**
    * Get the cell identity.
    *
    * @returns Cell identity.
    */
   const int getIdentity();

   /**
    * Get the absolute RF channel number.
    *
    * @returns Absolute RF channel number.
    */
   const int getArfcn();

   /**
    * Get the base station identity code.
    *
    * @returns Base station identity code.
    */
   const int getBaseStationIdentityCode();

private:
   std::string mcc_;
   std::string mnc_;
   int lac_;
   int cid_;
   int arfcn_;
   int bsic_;
};

/**
 * CdmaCellIdentity class provides methods to get the network identifier, system identifier, base
 * station identifier, longitude and latitude.
 *
 * @deprecated As of version 1.53.0 this API is no longer supported.
 */
class CdmaCellIdentity {
public:
   CdmaCellIdentity(int networkId, int systemId, int baseStationId, int longitude, int latitude);
   /**
    * Get the network identifier.
    *
    * @returns Network identifier.
    */
   const int getNid();

   /**
    * Get the system identifier.
    *
    * @returns System identifier.
    */
   const int getSid();

   /**
    * Get the base station identifier.
    *
    * @returns Base station identifier.
    */
   const int getBaseStationId();

   /**
    * Get the longitude.
    *
    * @returns Longitude.
    */
   const int getLongitude();

   /**
    * Get the latitude.
    *
    * @returns Latitude.
    */
   const int getLatitude();

private:
   int nid_;
   int sid_;
   int stationId_;
   int longitude_;
   int latitude_;
};

/**
 * LteCellIdentity class provides methods to get the mobile country code, mobile network
 * code, cell identity, physical cell identifier, tracking area code and absolute Rf channel number.
 */
class LteCellIdentity {
public:
   LteCellIdentity(std::string mcc, std::string mnc, int ci, int pci, int tac, int earfcn);
   /**
    * Get the Mobile Country Code.
    *
    * @returns Mcc value.
    *
    * @deprecated Use getMobileCountryCode() API instead
    */
   const int getMcc();

   /**
    * Get the Mobile Network Code.
    *
    * @returns Mnc value.
    *
    * @deprecated Use getMobileNetworkCode() API instead
    */
   const int getMnc();

   /**
    * Get the Mobile Country Code.
    *
    * @returns Mcc value.
    *
    */
   const std::string getMobileCountryCode();

   /**
    * Get the Mobile Network Code.
    *
    * @returns Mnc value.
    *
    */
   const std::string getMobileNetworkCode();

   /**
    * Get the cell identity.
    *
    * @returns Cell identity.
    */
   const int getIdentity();

   /**
    * Get the physical cell identifier.
    *
    * @returns Physical cell identifier.
    */
   const int getPhysicalCellId();

   /**
    * Get the tracking area code.
    *
    * @returns Tracking area code.
    */
   const int getTrackingAreaCode();

   /**
    * Get the absolute RF channel number.
    *
    * @returns Absolute RF channel number.
    */
   const int getEarfcn();

private:
   std::string mcc_;
   std::string mnc_;
   int ci_;
   int pci_;
   int tac_;
   int earfcn_;
};

/**
 * WcdmaCellIdentity class provides methods to get the mobile country code, mobile network
 * code, location area code, cell identitifier, primary scrambling code and absolute RF channel
 * number.
 */
class WcdmaCellIdentity {
public:
   WcdmaCellIdentity(std::string mcc, std::string mnc, int lac, int cid, int psc, int uarfcn);
   /**
    * Get the Mobile Country Code.
    *
    * @returns Mcc value.
    *
    * @deprecated Use getMobileCountryCode() API instead
    */
   const int getMcc();

   /**
    * Get the Mobile Network Code.
    *
    * @returns Mnc value.
    *
    * @deprecated Use getMobileNetworkCode() API instead
    */
   const int getMnc();

   /**
    * Get the Mobile Country Code.
    *
    * @returns Mcc value.
    *
    */
   const std::string getMobileCountryCode();

   /**
    * Get the Mobile Network Code.
    *
    * @returns Mnc value.
    *
    */
   const std::string getMobileNetworkCode();

   /**
    * Get the location area code.
    *
    * @returns Location area code.
    */
   const int getLac();

   /**
    * Get the cell identity.
    *
    * @returns Cell identity.
    */
   const int getIdentity();

   /**
    * Get the primary scrambling code.
    *
    * @returns Primary scrambling code.
    */
   const int getPrimaryScramblingCode();

   /**
    * Get the absolute RF channel number.
    *
    * @returns Absolute RF channel number.
    */
   const int getUarfcn();

private:
   std::string mcc_;
   std::string mnc_;
   int lac_;
   int cid_;
   int psc_;
   int uarfcn_;
};

/**
 * TdscdmaCellIdentity class provides methods to get the mobile country code, mobile
 * network code, location area code, cell identity and cell parameters identifier.
 *
 * @deprecated As of version 1.53.0 this API is no longer supported.
 */
class TdscdmaCellIdentity {
public:
   TdscdmaCellIdentity(std::string mcc, std::string mnc, int lac, int cid, int cpid);
   /**
    * Get the Mobile Country Code.
    *
    * @returns Mcc value.
    *
    * @deprecated Use getMobileCountryCode() API instead
    */
   const int getMcc();

   /**
    * Get the Mobile Network Code.
    *
    * @returns Mnc value.
    *
    * @deprecated Use getMobileNetworkCode() API instead
    */
   const int getMnc();

   /**
    * Get the Mobile Country Code.
    *
    * @returns Mcc value.
    *
    */
   const std::string getMobileCountryCode();

   /**
    * Get the Mobile Network Code.
    *
    * @returns Mnc value.
    *
    */
   const std::string getMobileNetworkCode();

   /**
    * Get the location area code
    *
    * @returns Location area code.
    */
   const int getLac();

   /**
    * Get the cell identity.
    *
    * @returns Cell identity.
    */
   const int getIdentity();

   /**
    * Get the cell parameters identifier.
    *
    * @returns Cell parameters identifier.
    */
   const int getParametersId();

private:
   std::string mcc_;
   std::string mnc_;
   int lac_;
   int cid_;
   int cpid_;
};

/**
 * Nr5gCellIdentity class provides methods to get the mobile country code, mobile network
 * code, cell identity, physical cell identifier, tracking area code and absolute RF channel number
 * information of the Serving cell
 */
class Nr5gCellIdentity {
public:
   Nr5gCellIdentity(std::string mcc, std::string mnc, uint64_t ci, uint32_t pci, int32_t tac,
        int32_t arfcn);
   /**
    * Get the Mobile Country Code.
    *
    * @returns Mcc value.
    *
    */
   const std::string getMobileCountryCode();

   /**
    * Get the Mobile Network Code.
    *
    * @returns Mnc value.
    *
    */
   const std::string getMobileNetworkCode();

   /**
    * Get the cell identity.
    *
    * @returns Cell identity.
    *
    */
   const uint64_t getIdentity();

   /**
    * Get the physical cell identifier.
    *
    * @returns Physical cell identifier.
    *
    */
   const uint32_t getPhysicalCellId();

   /**
    * Get the tracking area code.
    *
    * @returns Tracking area code.
    *
    */
   const int32_t getTrackingAreaCode();

   /**
    * Get the absolute RF channel number.
    *
    * @returns Absolute RF channel number. '-1' denotes that the value is unknown.
    *
    */
   const int32_t getArfcn();

private:
   std::string mcc_;
   std::string mnc_;
   uint64_t ci_;
   uint32_t pci_;
   int32_t tac_;
   int32_t arfcn_;
};

/**
 * Nb1NtnCellIdentity class provides methods to get the mobile country code, mobile network code,
 * cell identity, tracking area code and E-UTRA absolute radio frequency(RF) channel number.
 */
class Nb1NtnCellIdentity {
public:
   Nb1NtnCellIdentity(std::string mcc, std::string mnc, int ci, int tac, int earfcn);

   /**
    * Get the mobile country code.
    *
    * @returns mcc value.
    *
    */
   const std::string getMobileCountryCode();

   /**
    * Get the mobile network code.
    *
    * @returns mnc value.
    *
    */
   const std::string getMobileNetworkCode();

   /**
    * Get the cell identity.
    *
    * @returns Cell identity.
    */
   const int getIdentity();

   /**
    * Get the tracking area code.
    *
    * @returns Tracking area code.
    */
   const int getTrackingAreaCode();

   /**
    * Get the E-UTRA absolute radio frequency(RF) channel number.
    *
    * @returns E-UTRA absolute radio frequency(RF) channel number.
    */
   const int getEarfcn();

private:
   std::string mcc_;
   std::string mnc_;
   int ci_;
   int tac_;
   int earfcn_;
};

/**
 * CellInfo class provides cell info type and checks whether the current cell is registered
 * or not.
 */
class CellInfo {
public:
   /**
    * Get the cell type.
    *
    * @returns CellType.
    */
   virtual CellType getType();

   /**
    * Checks whether the current cell is registered or not.
    *
    * @returns If true cell is registered or vice-versa.
    */
   virtual bool isRegistered();

   virtual ~CellInfo() {
   }

protected:
   CellType type_;
   int registered_;
};

/**
 *GsmCellInfo class provides methods to get cell type, cell registration status, cell
 *identity and signal strength information.
 */
class GsmCellInfo : public CellInfo {
public:
   /**
    * GsmCellInfo constructor.
    * @param [in] registered - Registration status of the cell.
    * @param [in] id - GSM cell identity.
    * @param [in] ssInfo - GSM cell signal strength.
    */
   GsmCellInfo(int registered, GsmCellIdentity id, GsmSignalStrengthInfo ssInfo);

   /**
    * Get GSM cell identity information.
    *
    * @returns GsmCellIdentity.
    */
   GsmCellIdentity getCellIdentity();

   /**
    * Get GSM cell signal strength information.
    *
    * @returns GsmSignalStrengthInfo.
    */
   GsmSignalStrengthInfo getSignalStrengthInfo();

private:
   GsmCellIdentity id_;
   GsmSignalStrengthInfo ssInfo_;
};

/**
 *CdmaCellInfo class provides methods to get cell type, cell registration status, cell
 *identity and signal strength information.
 *
 * @deprecated As of version 1.53.0 this API is no longer supported.
 */
class CdmaCellInfo : public CellInfo {
public:
   /**
    * CdmaCellInfo constructor
    * @param [in] registered - Registration status of the cell.
    * @param [in] id - CDMA cell identity.
    * @param [in] ssInfo - CDMA cell signal strength.
    */
   CdmaCellInfo(int registered, CdmaCellIdentity id, CdmaSignalStrengthInfo ssInfo);

   /**
    * Get CDMA cell identity information.
    *
    * @returns CdmaCellIdentity.
    */
   CdmaCellIdentity getCellIdentity();

   /**
    * Get CDMA cell signal strength information.
    *
    * @returns CdmaSignalStrengthInfo.
    */
   CdmaSignalStrengthInfo getSignalStrengthInfo();

private:
   CdmaCellIdentity id_;
   CdmaSignalStrengthInfo ssInfo_;
};

/**
 *LteCellInfo class provides methods to get cell type, cell registration status, cell
 *identity and signal strength information.
 */
class LteCellInfo : public CellInfo {
public:
   /**
    * LteCellInfo constructor.
    * @param [in] registered - Registration status of the cell.
    * @param [in] id - LTE cell identity class.
    * @param [in] ssInfo - LTE cell signal strength.
    */
   LteCellInfo(int registered, LteCellIdentity id, LteSignalStrengthInfo ssInfo);

   /**
    * Get LTE cell identity information.
    *
    * @returns LteCellIdentity.
    */
   LteCellIdentity getCellIdentity();

   /**
    * Get LTE cell signal strength information.
    *
    * @returns LteSignalStrengthInfo.
    */
   LteSignalStrengthInfo getSignalStrengthInfo();

private:
   LteCellIdentity id_;
   LteSignalStrengthInfo ssInfo_;
};

/**
 *WcdmaCellInfo class provides methods to get cell type, cell registration status, cell
 *identity and signal strength information.
 */
class WcdmaCellInfo : public CellInfo {
public:
   /**
    * WcdmaCellInfo constructor.
    * @param [in] registered - Registration status of the cell.
    * @param [in] id - WCDMA cell identity.
    * @param [in] ssInfo - WCDMA cell signal strength.
    */
   WcdmaCellInfo(int registered, WcdmaCellIdentity id, WcdmaSignalStrengthInfo ssInfo);

   /**
    * Get WCDMA cell identity information.
    *
    * @returns WcdmaCellIdentity.
    */
   WcdmaCellIdentity getCellIdentity();

   /**
    * Get WCDMA cell signal strength information.
    *
    * @returns WcdmaSignalStrengthInfo.
    */
   WcdmaSignalStrengthInfo getSignalStrengthInfo();

private:
   WcdmaCellIdentity id_;
   WcdmaSignalStrengthInfo ssInfo_;
};

/**
 *TdscdmaCellInfo class provides methods to get cell type, cell registration status, cell
 *identity and signal strength information.
 *
 * @deprecated As of version 1.53.0 this API is no longer supported.
 */
class TdscdmaCellInfo : public CellInfo {
public:
   /**
    * TdscdmaCellInfo constructor.
    * @param [in] registered - Registration status of the cell
    * @param [in] id - TDSCDMA cell identity.
    * @param [in] ssInfo - TDSCDMA cell signal strength.
    */
   TdscdmaCellInfo(int registered, TdscdmaCellIdentity id, TdscdmaSignalStrengthInfo ssInfo);

   /**
    * Get TDSCDMA cell identity information.
    *
    * @returns TdscdmaCellIdentity.
    */
   TdscdmaCellIdentity getCellIdentity();

   /**
    * Get TDSCDMA cell signal strength information.
    *
    * @returns TdscdmaSignalStrengthInfo.
    */
   TdscdmaSignalStrengthInfo getSignalStrengthInfo();

private:
   TdscdmaCellIdentity id_;
   TdscdmaSignalStrengthInfo ssInfo_;
};

/**
 * Nr5gCellInfo class provides methods to get cell type, cell registration status, cell
 * identity and signal strength information corresponding to the Serving cell.
 */
class Nr5gCellInfo : public CellInfo {
public:

   Nr5gCellInfo(int registered, Nr5gCellIdentity id, Nr5gSignalStrengthInfo ssInfo);

   /**
    * Get NR5G cell identity information.
    *
    * @returns Nr5gCellIdentity.
    *
    */
   Nr5gCellIdentity getCellIdentity();

   /**
    * Get NR5G cell signal strength information.
    *
    * @returns Nr5gSignalStrengthInfo.
    *
    */
   Nr5gSignalStrengthInfo getSignalStrengthInfo();

private:
   Nr5gCellIdentity id_;
   Nr5gSignalStrengthInfo ssInfo_;
};

/**
 * Nb1NtnCellInfo class provides methods to get cell type, cell registration status, cell
 * identity and signal strength information.
 */
class Nb1NtnCellInfo : public CellInfo {
public:
   /**
    * Nb1NtnCellInfo constructor.
    * @param [in] registered - Registration status of the cell.
    * @param [in] id - NB1 NTN cell identity class.
    * @param [in] ssInfo - NB1 NTN cell signal strength.
    */
   Nb1NtnCellInfo(int registered, Nb1NtnCellIdentity id, Nb1NtnSignalStrengthInfo ssInfo);

   /**
    * Get NB1 NTN cell identity information.
    *
    * @returns Nb1NtnCellIdentity.
    */
   Nb1NtnCellIdentity getCellIdentity();

   /**
    * Get NB1 NTN cell signal strength information.
    *
    * @returns Nb1NtnSignalStrengthInfo.
    */
   Nb1NtnSignalStrengthInfo getSignalStrengthInfo();

private:
   Nb1NtnCellIdentity id_;
   Nb1NtnSignalStrengthInfo ssInfo_;
};

/** @} */ /* end_addtogroup telematics_phone */

}  // End of namespace tel

}  // End of namespace telux

#endif // TELUX_TEL_CELLINFO_HPP
