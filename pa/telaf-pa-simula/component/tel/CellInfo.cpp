// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear
//
// CellInfo.cpp -- out-of-line definitions for telux::tel::CellInfo, the seven
// per-RAT *CellInfo subclasses, and the seven *CellIdentity container classes.


#include <string>
#include <telux/tel/CellInfo.hpp>

namespace telux {
namespace tel {

namespace {

// getMcc()/getMnc() return int while the ctor takes the codes as strings (the
// wire/JSON form). Parse defensively: an empty or non-numeric code yields 0
// rather than throwing out of an accessor the PA calls unguarded.
int
toIntOrZero(const std::string& s)
{
    if (s.empty())
        return 0;
    try
    {
        return std::stoi(s);
    }
    catch (...)
    {
        return 0;
    }
}

}  // namespace

// ---------------------------------------------------------------------------
// GsmCellIdentity

GsmCellIdentity::GsmCellIdentity(
  std::string mcc,
  std::string mnc,
  int lac,
  int cid,
  int arfcn,
  int bsic
)
    : mcc_(std::move(mcc))
    , mnc_(std::move(mnc))
    , lac_(lac)
    , cid_(cid)
    , arfcn_(arfcn)
    , bsic_(bsic)
{
}

const int GsmCellIdentity::getMcc() { return toIntOrZero(mcc_); }
const int GsmCellIdentity::getMnc() { return toIntOrZero(mnc_); }
const std::string GsmCellIdentity::getMobileCountryCode() { return mcc_; }
const std::string GsmCellIdentity::getMobileNetworkCode() { return mnc_; }
const int GsmCellIdentity::getLac() { return lac_; }
const int GsmCellIdentity::getIdentity() { return cid_; }
const int GsmCellIdentity::getArfcn() { return arfcn_; }
const int GsmCellIdentity::getBaseStationIdentityCode() { return bsic_; }

// ---------------------------------------------------------------------------
// CdmaCellIdentity

CdmaCellIdentity::CdmaCellIdentity(
  int networkId,
  int systemId,
  int baseStationId,
  int longitude,
  int latitude
)
    : nid_(networkId)
    , sid_(systemId)
    , stationId_(baseStationId)
    , longitude_(longitude)
    , latitude_(latitude)
{
}

const int CdmaCellIdentity::getNid() { return nid_; }
const int CdmaCellIdentity::getSid() { return sid_; }
const int CdmaCellIdentity::getBaseStationId() { return stationId_; }
const int CdmaCellIdentity::getLongitude() { return longitude_; }
const int CdmaCellIdentity::getLatitude() { return latitude_; }

// ---------------------------------------------------------------------------
// LteCellIdentity

LteCellIdentity::LteCellIdentity(
  std::string mcc,
  std::string mnc,
  int ci,
  int pci,
  int tac,
  int earfcn
)
    : mcc_(std::move(mcc))
    , mnc_(std::move(mnc))
    , ci_(ci)
    , pci_(pci)
    , tac_(tac)
    , earfcn_(earfcn)
{
}

const int LteCellIdentity::getMcc() { return toIntOrZero(mcc_); }
const int LteCellIdentity::getMnc() { return toIntOrZero(mnc_); }
const std::string LteCellIdentity::getMobileCountryCode() { return mcc_; }
const std::string LteCellIdentity::getMobileNetworkCode() { return mnc_; }
const int LteCellIdentity::getIdentity() { return ci_; }
const int LteCellIdentity::getPhysicalCellId() { return pci_; }
const int LteCellIdentity::getTrackingAreaCode() { return tac_; }
const int LteCellIdentity::getEarfcn() { return earfcn_; }

// ---------------------------------------------------------------------------
// WcdmaCellIdentity

WcdmaCellIdentity::WcdmaCellIdentity(
  std::string mcc,
  std::string mnc,
  int lac,
  int cid,
  int psc,
  int uarfcn
)
    : mcc_(std::move(mcc))
    , mnc_(std::move(mnc))
    , lac_(lac)
    , cid_(cid)
    , psc_(psc)
    , uarfcn_(uarfcn)
{
}

const int WcdmaCellIdentity::getMcc() { return toIntOrZero(mcc_); }
const int WcdmaCellIdentity::getMnc() { return toIntOrZero(mnc_); }
const std::string WcdmaCellIdentity::getMobileCountryCode() { return mcc_; }
const std::string WcdmaCellIdentity::getMobileNetworkCode() { return mnc_; }
const int WcdmaCellIdentity::getLac() { return lac_; }
const int WcdmaCellIdentity::getIdentity() { return cid_; }
const int WcdmaCellIdentity::getPrimaryScramblingCode() { return psc_; }
const int WcdmaCellIdentity::getUarfcn() { return uarfcn_; }

// ---------------------------------------------------------------------------
// TdscdmaCellIdentity

TdscdmaCellIdentity::TdscdmaCellIdentity(
  std::string mcc,
  std::string mnc,
  int lac,
  int cid,
  int cpid
)
    : mcc_(std::move(mcc))
    , mnc_(std::move(mnc))
    , lac_(lac)
    , cid_(cid)
    , cpid_(cpid)
{
}

const int TdscdmaCellIdentity::getMcc() { return toIntOrZero(mcc_); }
const int TdscdmaCellIdentity::getMnc() { return toIntOrZero(mnc_); }
const std::string TdscdmaCellIdentity::getMobileCountryCode() { return mcc_; }
const std::string TdscdmaCellIdentity::getMobileNetworkCode() { return mnc_; }
const int TdscdmaCellIdentity::getLac() { return lac_; }
const int TdscdmaCellIdentity::getIdentity() { return cid_; }
const int TdscdmaCellIdentity::getParametersId() { return cpid_; }

// ---------------------------------------------------------------------------
// Nr5gCellIdentity

Nr5gCellIdentity::Nr5gCellIdentity(
  std::string mcc,
  std::string mnc,
  uint64_t ci,
  uint32_t pci,
  int32_t tac,
  int32_t arfcn
)
    : mcc_(std::move(mcc))
    , mnc_(std::move(mnc))
    , ci_(ci)
    , pci_(pci)
    , tac_(tac)
    , arfcn_(arfcn)
{
}

const std::string Nr5gCellIdentity::getMobileCountryCode() { return mcc_; }
const std::string Nr5gCellIdentity::getMobileNetworkCode() { return mnc_; }
const uint64_t Nr5gCellIdentity::getIdentity() { return ci_; }
const uint32_t Nr5gCellIdentity::getPhysicalCellId() { return pci_; }
const int32_t Nr5gCellIdentity::getTrackingAreaCode() { return tac_; }
const int32_t Nr5gCellIdentity::getArfcn() { return arfcn_; }

// ---------------------------------------------------------------------------
// Nb1NtnCellIdentity

Nb1NtnCellIdentity::Nb1NtnCellIdentity(
  std::string mcc,
  std::string mnc,
  int ci,
  int tac,
  int earfcn
)
    : mcc_(std::move(mcc))
    , mnc_(std::move(mnc))
    , ci_(ci)
    , tac_(tac)
    , earfcn_(earfcn)
{
}

const std::string Nb1NtnCellIdentity::getMobileCountryCode() { return mcc_; }
const std::string Nb1NtnCellIdentity::getMobileNetworkCode() { return mnc_; }
const int Nb1NtnCellIdentity::getIdentity() { return ci_; }
const int Nb1NtnCellIdentity::getTrackingAreaCode() { return tac_; }
const int Nb1NtnCellIdentity::getEarfcn() { return earfcn_; }

// ---------------------------------------------------------------------------
// CellInfo base
//
// type_ / registered_ are protected members each subclass ctor sets; the base
// has no ctor of its own declared in the header, so only the two virtuals need
// bodies here.

CellType
CellInfo::getType()
{
    return type_;
}

bool
CellInfo::isRegistered()
{
    return registered_ != 0;
}

// ---------------------------------------------------------------------------
// GsmCellInfo

GsmCellInfo::GsmCellInfo(int registered, GsmCellIdentity id, GsmSignalStrengthInfo ssInfo)
    : id_(std::move(id))
    , ssInfo_(std::move(ssInfo))
{
    type_ = CellType::GSM;
    registered_ = registered;
}

GsmCellIdentity
GsmCellInfo::getCellIdentity()
{
    return id_;
}

GsmSignalStrengthInfo
GsmCellInfo::getSignalStrengthInfo()
{
    return ssInfo_;
}

// ---------------------------------------------------------------------------
// CdmaCellInfo

CdmaCellInfo::CdmaCellInfo(int registered, CdmaCellIdentity id, CdmaSignalStrengthInfo ssInfo)
    : id_(std::move(id))
    , ssInfo_(std::move(ssInfo))
{
    type_ = CellType::CDMA;
    registered_ = registered;
}

CdmaCellIdentity
CdmaCellInfo::getCellIdentity()
{
    return id_;
}

CdmaSignalStrengthInfo
CdmaCellInfo::getSignalStrengthInfo()
{
    return ssInfo_;
}

// ---------------------------------------------------------------------------
// LteCellInfo

LteCellInfo::LteCellInfo(int registered, LteCellIdentity id, LteSignalStrengthInfo ssInfo)
    : id_(std::move(id))
    , ssInfo_(std::move(ssInfo))
{
    type_ = CellType::LTE;
    registered_ = registered;
}

LteCellIdentity
LteCellInfo::getCellIdentity()
{
    return id_;
}

LteSignalStrengthInfo
LteCellInfo::getSignalStrengthInfo()
{
    return ssInfo_;
}

// ---------------------------------------------------------------------------
// WcdmaCellInfo

WcdmaCellInfo::WcdmaCellInfo(int registered, WcdmaCellIdentity id, WcdmaSignalStrengthInfo ssInfo)
    : id_(std::move(id))
    , ssInfo_(std::move(ssInfo))
{
    type_ = CellType::WCDMA;
    registered_ = registered;
}

WcdmaCellIdentity
WcdmaCellInfo::getCellIdentity()
{
    return id_;
}

WcdmaSignalStrengthInfo
WcdmaCellInfo::getSignalStrengthInfo()
{
    return ssInfo_;
}

// ---------------------------------------------------------------------------
// TdscdmaCellInfo

TdscdmaCellInfo::TdscdmaCellInfo(
  int registered,
  TdscdmaCellIdentity id,
  TdscdmaSignalStrengthInfo ssInfo
)
    : id_(std::move(id))
    , ssInfo_(std::move(ssInfo))
{
    type_ = CellType::TDSCDMA;
    registered_ = registered;
}

TdscdmaCellIdentity
TdscdmaCellInfo::getCellIdentity()
{
    return id_;
}

TdscdmaSignalStrengthInfo
TdscdmaCellInfo::getSignalStrengthInfo()
{
    return ssInfo_;
}

// ---------------------------------------------------------------------------
// Nr5gCellInfo

Nr5gCellInfo::Nr5gCellInfo(int registered, Nr5gCellIdentity id, Nr5gSignalStrengthInfo ssInfo)
    : id_(std::move(id))
    , ssInfo_(std::move(ssInfo))
{
    type_ = CellType::NR5G;
    registered_ = registered;
}

Nr5gCellIdentity
Nr5gCellInfo::getCellIdentity()
{
    return id_;
}

Nr5gSignalStrengthInfo
Nr5gCellInfo::getSignalStrengthInfo()
{
    return ssInfo_;
}

// ---------------------------------------------------------------------------
// Nb1NtnCellInfo

Nb1NtnCellInfo::Nb1NtnCellInfo(
  int registered,
  Nb1NtnCellIdentity id,
  Nb1NtnSignalStrengthInfo ssInfo
)
    : id_(std::move(id))
    , ssInfo_(std::move(ssInfo))
{
    type_ = CellType::NB1_NTN;
    registered_ = registered;
}

Nb1NtnCellIdentity
Nb1NtnCellInfo::getCellIdentity()
{
    return id_;
}

Nb1NtnSignalStrengthInfo
Nb1NtnCellInfo::getSignalStrengthInfo()
{
    return ssInfo_;
}

}  // namespace tel
}  // namespace telux
