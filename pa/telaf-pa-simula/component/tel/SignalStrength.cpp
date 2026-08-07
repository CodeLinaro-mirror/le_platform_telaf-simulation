// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear
//
// SignalStrength.cpp -- out-of-line definitions for telux::tel::SignalStrength
// and the per-RAT *SignalStrengthInfo container classes.


#include <map>
#include <telux/tel/SignalStrength.hpp>

// ---- GSM ----
#define MIN_GSM_LEVEL 0
#define MAX_GSM_LEVEL 31
#define GSM_MIN_BIT_ERROR_RATE 0
#define GSM_MAX_BIT_ERROR_RATE 7
#define GSM_MIN_TIMING_ADVANCE 0
#define GSM_MAX_TIMING_ADVANCE 219
#define GSM_DBM_CONVERSION_FACTOR -113
#define GSM_DBM_MULTIPLICATION_FACTOR 2

// ---- CDMA / EVDO ----
#define MIN_CDMA_DBM -120
#define MAX_CDMA_DBM 0
#define MIN_CDMA_ECIO -160
#define MAX_CDMA_ECIO 0
#define MIN_EVDO_DBM -120
#define MAX_EVDO_DBM 0
#define MIN_EVDO_ECIO -160
#define MAX_EVDO_ECIO 0
#define MIN_EVDO_SNR 0
#define MAX_EVDO_SNR 8

// ---- LTE ----
#define MIN_LTE_SIGNAL_STRENGTH 0
#define MAX_LTE_SIGNAL_STRENGTH 31
#define MIN_LTE_RSRP -140
#define MAX_LTE_RSRP -44
#define MIN_LTE_RSRQ -20
#define MAX_LTE_RSRQ -3
#define MIN_LTE_RSSNR_LEVEL -200
#define MAX_LTE_RSSNR_LEVEL 300
#define MIN_LTE_CQI 0
#define MAX_LTE_CQI 15
#define MIN_LTE_TIMING_ADVANCE 0
#define MAX_LTE_TIMING_ADVANCE 2147483646

// ---- WCDMA ----
#define MIN_WCDMA_LEVEL 0
#define MAX_WCDMA_LEVEL 31
#define MIN_WCDMA_BIT_ERROR_RATE 0
#define MAX_WCDMA_BIT_ERROR_RATE 7
#define MIN_WCDMA_ECIO -20
#define MAX_WCDMA_ECIO 0
#define MIN_WCDMA_RSCP -120
#define MAX_WCDMA_RSCP -24
#define WCDMA_DBM_CONVERSION_FACTOR -113
#define WCDMA_DBM_MULTIPLICATION_FACTOR 2

// ---- TDSCDMA ----
#define MIN_TDSCDMA_RSCP -120
#define MAX_TDSCDMA_RSCP -25

// ---- NR5G ----
#define MIN_NR5G_RSRP -140
#define MAX_NR5G_RSRP -44
#define MIN_NR5G_RSRQ -43
#define MAX_NR5G_RSRQ 20
#define MIN_NR5G_RSSNR_LEVEL -230
#define MAX_NR5G_RSSNR_LEVEL 400

// ---- NB1 NTN (same envelope as LTE per header docs) ----
#define MIN_NB1_SIGNAL_STRENGTH 0
#define MAX_NB1_SIGNAL_STRENGTH 31
#define MIN_NB1_RSRP -140
#define MAX_NB1_RSRP -44
#define MIN_NB1_RSRQ -20
#define MAX_NB1_RSRQ -3
#define MIN_NB1_RSSNR -200
#define MAX_NB1_RSSNR 300

// ---- RSSI (shared across RATs per header docs: [-100, -25]) ----
#define MIN_RSSI -100
#define MAX_RSSI -25

namespace telux {
namespace tel {

namespace {

// Level threshold tables -- ported verbatim from the gRPC simulation so
// getLevel() is bit-identical to what tafRadioSvc saw before the MQTT cutover.
const std::map<SignalStrengthLevel, int> kGsmLevelMap{
    { SignalStrengthLevel::LEVEL_1, 0 },  { SignalStrengthLevel::LEVEL_2, 3 },
    { SignalStrengthLevel::LEVEL_3, 5 },  { SignalStrengthLevel::LEVEL_4, 8 },
    { SignalStrengthLevel::LEVEL_5, 12 },
};

const std::map<SignalStrengthLevel, int> kLteRsrpLevelMap{
    { SignalStrengthLevel::LEVEL_1, -140 }, { SignalStrengthLevel::LEVEL_2, -100 },
    { SignalStrengthLevel::LEVEL_3, -90 },  { SignalStrengthLevel::LEVEL_4, -80 },
    { SignalStrengthLevel::LEVEL_5, -70 },
};

const std::map<SignalStrengthLevel, int> kLteRssnrLevelMap{
    { SignalStrengthLevel::LEVEL_1, -200 }, { SignalStrengthLevel::LEVEL_2, -30 },
    { SignalStrengthLevel::LEVEL_3, 10 },   { SignalStrengthLevel::LEVEL_4, 45 },
    { SignalStrengthLevel::LEVEL_5, 130 },
};

const std::map<SignalStrengthLevel, int> kCdmaDbmMap{
    { SignalStrengthLevel::LEVEL_1, -110 }, { SignalStrengthLevel::LEVEL_2, -100 },
    { SignalStrengthLevel::LEVEL_3, -95 },  { SignalStrengthLevel::LEVEL_4, -85 },
    { SignalStrengthLevel::LEVEL_5, -75 },
};

const std::map<SignalStrengthLevel, int> kCdmaEcioMap{
    { SignalStrengthLevel::LEVEL_1, -160 }, { SignalStrengthLevel::LEVEL_2, -150 },
    { SignalStrengthLevel::LEVEL_3, -130 }, { SignalStrengthLevel::LEVEL_4, -110 },
    { SignalStrengthLevel::LEVEL_5, -90 },
};

const std::map<SignalStrengthLevel, int> kEvdoDbmMap{
    { SignalStrengthLevel::LEVEL_1, -115 }, { SignalStrengthLevel::LEVEL_2, -105 },
    { SignalStrengthLevel::LEVEL_3, -90 },  { SignalStrengthLevel::LEVEL_4, -75 },
    { SignalStrengthLevel::LEVEL_5, -65 },
};

const std::map<SignalStrengthLevel, int> kEvdoSnrMap{
    { SignalStrengthLevel::LEVEL_1, 0 }, { SignalStrengthLevel::LEVEL_2, 1 },
    { SignalStrengthLevel::LEVEL_3, 3 }, { SignalStrengthLevel::LEVEL_4, 5 },
    { SignalStrengthLevel::LEVEL_5, 7 },
};

const std::map<SignalStrengthLevel, int> kWcdmaLevelMap{
    { SignalStrengthLevel::LEVEL_1, 0 },  { SignalStrengthLevel::LEVEL_2, 3 },
    { SignalStrengthLevel::LEVEL_3, 5 },  { SignalStrengthLevel::LEVEL_4, 8 },
    { SignalStrengthLevel::LEVEL_5, 12 },
};

const std::map<SignalStrengthLevel, int> kNr5gRsrpLevelMap{
    { SignalStrengthLevel::LEVEL_1, -140 }, { SignalStrengthLevel::LEVEL_2, -110 },
    { SignalStrengthLevel::LEVEL_3, -90 },  { SignalStrengthLevel::LEVEL_4, -80 },
    { SignalStrengthLevel::LEVEL_5, -65 },
};

const std::map<SignalStrengthLevel, int> kNr5gRssnrLevelMap{
    { SignalStrengthLevel::LEVEL_1, -230 }, { SignalStrengthLevel::LEVEL_2, -50 },
    { SignalStrengthLevel::LEVEL_3, 50 },   { SignalStrengthLevel::LEVEL_4, 150 },
    { SignalStrengthLevel::LEVEL_5, 300 },
};

// std::map<SignalStrengthLevel, int> iterates in SignalStrengthLevel order
// (LEVEL_1..LEVEL_5 are 0..4, LEVEL_UNKNOWN is -1 and never a key here), so
// walking it ascending and keeping the last threshold <= val yields the level.
SignalStrengthLevel
calculateLevel(int val, const std::map<SignalStrengthLevel, int>& levelMap)
{
    SignalStrengthLevel level = SignalStrengthLevel::LEVEL_UNKNOWN;
    for (const auto& it : levelMap)
    {
        if (val >= it.second)
            level = it.first;
        else
            break;
    }
    return level;
}

int
inRange(int value, int min, int max)
{
    if (value < min || value > max)
        return INVALID_SIGNAL_STRENGTH_VALUE;
    return value;
}

}  // namespace

// ---------------------------------------------------------------------------
// SignalStrength -- aggregate holder

SignalStrength::SignalStrength(
  std::shared_ptr<LteSignalStrengthInfo> lteSignalStrengthInfo,
  std::shared_ptr<GsmSignalStrengthInfo> gsmSignalStrengthInfo,
  std::shared_ptr<CdmaSignalStrengthInfo> cdmaSignalStrengthInfo,
  std::shared_ptr<WcdmaSignalStrengthInfo> wcdmaSignalStrengthInfo,
  std::shared_ptr<TdscdmaSignalStrengthInfo> tdscdmaSignalStrengthInfo,
  std::shared_ptr<Nr5gSignalStrengthInfo> nr5gSignalStrengthInfo,
  std::shared_ptr<Nb1NtnSignalStrengthInfo> nb1NtnSignalStrengthInfo
)
    : lteSS_(std::move(lteSignalStrengthInfo))
    , gsmSS_(std::move(gsmSignalStrengthInfo))
    , cdmaSS_(std::move(cdmaSignalStrengthInfo))
    , wcdmaSS_(std::move(wcdmaSignalStrengthInfo))
    , tdscdmaSS_(std::move(tdscdmaSignalStrengthInfo))
    , nr5gSS_(std::move(nr5gSignalStrengthInfo))
    , nb1NtnSS_(std::move(nb1NtnSignalStrengthInfo))
{
}

std::shared_ptr<LteSignalStrengthInfo>
SignalStrength::getLteSignalStrength()
{
    return lteSS_;
}

std::shared_ptr<GsmSignalStrengthInfo>
SignalStrength::getGsmSignalStrength()
{
    return gsmSS_;
}

std::shared_ptr<CdmaSignalStrengthInfo>
SignalStrength::getCdmaSignalStrength()
{
    return cdmaSS_;
}

std::shared_ptr<WcdmaSignalStrengthInfo>
SignalStrength::getWcdmaSignalStrength()
{
    return wcdmaSS_;
}

std::shared_ptr<TdscdmaSignalStrengthInfo>
SignalStrength::getTdscdmaSignalStrength()
{
    return tdscdmaSS_;
}

std::shared_ptr<Nr5gSignalStrengthInfo>
SignalStrength::getNr5gSignalStrength()
{
    return nr5gSS_;
}

std::shared_ptr<Nb1NtnSignalStrengthInfo>
SignalStrength::getNb1NtnSignalStrength()
{
    return nb1NtnSS_;
}

// ---------------------------------------------------------------------------
// LteSignalStrengthInfo

LteSignalStrengthInfo::LteSignalStrengthInfo(
  int lteSignalStrength,
  int lteRsrp,
  int lteRsrq,
  int lteRssnr,
  int lteCqi,
  int timingAdvance,
  int lteRssi
)
{
    lteSignalStrength_ =
      inRange(lteSignalStrength, MIN_LTE_SIGNAL_STRENGTH, MAX_LTE_SIGNAL_STRENGTH);
    lteRssi_ = inRange(lteRssi, MIN_RSSI, MAX_RSSI);
    lteRsrp_ = inRange(lteRsrp, MIN_LTE_RSRP, MAX_LTE_RSRP);
    // lteAsu_ is declared in the header but has no accessor; the real SDK
    // derives an "arbitrary strength unit" from signal strength. Mirror the
    // clamped signal strength so the member is never left indeterminate.
    lteAsu_ = lteSignalStrength_;
    lteRsrq_ = inRange(lteRsrq, MIN_LTE_RSRQ, MAX_LTE_RSRQ);
    lteRssnr_ = inRange(lteRssnr, MIN_LTE_RSSNR_LEVEL, MAX_LTE_RSSNR_LEVEL);
    lteCqi_ = inRange(lteCqi, MIN_LTE_CQI, MAX_LTE_CQI);
    timingAdvance_ = inRange(timingAdvance, MIN_LTE_TIMING_ADVANCE, MAX_LTE_TIMING_ADVANCE);
}

const int
LteSignalStrengthInfo::getLteSignalStrength() const
{
    return lteSignalStrength_;
}

const int
LteSignalStrengthInfo::getRssi() const
{
    return lteRssi_;
}

const int
LteSignalStrengthInfo::getLteReferenceSignalReceiveQuality() const
{
    return lteRsrq_;
}

const int
LteSignalStrengthInfo::getLteReferenceSignalSnr() const
{
    return lteRssnr_;
}

const int
LteSignalStrengthInfo::getLteChannelQualityIndicator() const
{
    return lteCqi_;
}

const int
LteSignalStrengthInfo::getTimingAdvance() const
{
    return timingAdvance_;
}

const int
LteSignalStrengthInfo::getDbm() const
{
    return lteRsrp_;
}

const SignalStrengthLevel
LteSignalStrengthInfo::getLevel() const
{
    SignalStrengthLevel rsrpLevel = SignalStrengthLevel::LEVEL_UNKNOWN;
    if (lteRsrp_ >= MIN_LTE_RSRP && lteRsrp_ <= MAX_LTE_RSRP)
        rsrpLevel = calculateLevel(lteRsrp_, kLteRsrpLevelMap);

    SignalStrengthLevel rsnrLevel = SignalStrengthLevel::LEVEL_UNKNOWN;
    if (lteRssnr_ >= MIN_LTE_RSSNR_LEVEL && lteRssnr_ <= MAX_LTE_RSSNR_LEVEL)
        rsnrLevel = calculateLevel(lteRssnr_, kLteRssnrLevelMap);

    SignalStrengthLevel sigStrengthLevel = SignalStrengthLevel::LEVEL_UNKNOWN;
    if (lteSignalStrength_ >= MIN_LTE_SIGNAL_STRENGTH
        && lteSignalStrength_ <= MAX_LTE_SIGNAL_STRENGTH)
    {
        sigStrengthLevel = calculateLevel(lteRssnr_, kLteRssnrLevelMap);
    }

    // Give preference to rsrpLevel (matches the gRPC simulation exactly).
    if (rsrpLevel != SignalStrengthLevel::LEVEL_UNKNOWN
        && rsnrLevel != SignalStrengthLevel::LEVEL_UNKNOWN)
    {
        return (rsrpLevel > rsnrLevel ? rsrpLevel : rsnrLevel);
    }
    if (rsnrLevel != SignalStrengthLevel::LEVEL_UNKNOWN)
        return rsnrLevel;
    if (rsrpLevel != SignalStrengthLevel::LEVEL_UNKNOWN)
        return rsrpLevel;
    return sigStrengthLevel;
}

// ---------------------------------------------------------------------------
// GsmSignalStrengthInfo

GsmSignalStrengthInfo::GsmSignalStrengthInfo(
  int gsmSignalStrength,
  int gsmBitErrorRate,
  int timingAdvance,
  int gsmRssi
)
{
    gsmSignalStrength_ = inRange(gsmSignalStrength, MIN_GSM_LEVEL, MAX_GSM_LEVEL);
    rssi_ = inRange(gsmRssi, MIN_RSSI, MAX_RSSI);
    gsmBitErrorRate_ = inRange(gsmBitErrorRate, GSM_MIN_BIT_ERROR_RATE, GSM_MAX_BIT_ERROR_RATE);
    timingAdvance_ = inRange(timingAdvance, GSM_MIN_TIMING_ADVANCE, GSM_MAX_TIMING_ADVANCE);
}

const int
GsmSignalStrengthInfo::getGsmSignalStrength() const
{
    return gsmSignalStrength_;
}

const int
GsmSignalStrengthInfo::getRssi() const
{
    return rssi_;
}

const int
GsmSignalStrengthInfo::getGsmBitErrorRate() const
{
    return gsmBitErrorRate_;
}

const int
GsmSignalStrengthInfo::getTimingAdvance()
{
    return timingAdvance_;
}

const int
GsmSignalStrengthInfo::getDbm() const
{
    int dBm = inRange(gsmSignalStrength_, MIN_GSM_LEVEL, MAX_GSM_LEVEL);
    if (dBm != INVALID_SIGNAL_STRENGTH_VALUE)
        dBm = GSM_DBM_CONVERSION_FACTOR + (GSM_DBM_MULTIPLICATION_FACTOR * gsmSignalStrength_);
    return dBm;
}

const SignalStrengthLevel
GsmSignalStrengthInfo::getLevel() const
{
    // Valid values are 0-31, 99 as defined in TS 27.007 8.5
    if (gsmSignalStrength_ >= MIN_GSM_LEVEL && gsmSignalStrength_ <= MAX_GSM_LEVEL)
        return calculateLevel(gsmSignalStrength_, kGsmLevelMap);
    return SignalStrengthLevel::LEVEL_UNKNOWN;
}

// ---------------------------------------------------------------------------
// CdmaSignalStrengthInfo

CdmaSignalStrengthInfo::CdmaSignalStrengthInfo(
  int cdmaDbm,
  int cdmaEcio,
  int evdoDbm,
  int evdoEcio,
  int evdoSignalNoiseRatio
)
{
    cdmaDbm_ = inRange(cdmaDbm, MIN_CDMA_DBM, MAX_CDMA_DBM);
    cdmaEcio_ = inRange(cdmaEcio, MIN_CDMA_ECIO, MAX_CDMA_ECIO);
    evdoDbm_ = inRange(evdoDbm, MIN_EVDO_DBM, MAX_EVDO_DBM);
    evdoEcio_ = inRange(evdoEcio, MIN_EVDO_ECIO, MAX_EVDO_ECIO);
    evdoSignalNoiseRatio_ = inRange(evdoSignalNoiseRatio, MIN_EVDO_SNR, MAX_EVDO_SNR);
}

const int
CdmaSignalStrengthInfo::getCdmaEcio() const
{
    return cdmaEcio_;
}

const int
CdmaSignalStrengthInfo::getEvdoEcio() const
{
    return evdoEcio_;
}

const int
CdmaSignalStrengthInfo::getEvdoSignalNoiseRatio() const
{
    return evdoSignalNoiseRatio_;
}

const int
CdmaSignalStrengthInfo::getCdmaDbm() const
{
    return cdmaDbm_;
}

const int
CdmaSignalStrengthInfo::getEvdoDbm() const
{
    return evdoDbm_;
}

const int
CdmaSignalStrengthInfo::getDbm() const
{
    int cdmaDbm = (cdmaDbm_ > MAX_CDMA_DBM) ? cdmaDbm_ : MIN_CDMA_DBM;
    int evdoDbm = (evdoDbm_ > MAX_EVDO_DBM) ? evdoDbm_ : MIN_EVDO_DBM;
    return cdmaDbm < evdoDbm ? cdmaDbm : evdoDbm;
}

const SignalStrengthLevel
CdmaSignalStrengthInfo::getCdmaLevel() const
{
    SignalStrengthLevel cdmaDbmLevel = SignalStrengthLevel::LEVEL_UNKNOWN;
    if (cdmaDbm_ >= MIN_CDMA_DBM && cdmaDbm_ <= MAX_CDMA_DBM)
        cdmaDbmLevel = calculateLevel(cdmaDbm_, kCdmaDbmMap);

    SignalStrengthLevel cdmaEcioLevel = SignalStrengthLevel::LEVEL_UNKNOWN;
    if (cdmaEcio_ >= MIN_CDMA_ECIO && cdmaEcio_ <= MAX_CDMA_ECIO)
        cdmaEcioLevel = calculateLevel(cdmaEcio_, kCdmaEcioMap);

    return (cdmaDbmLevel < cdmaEcioLevel) ? cdmaDbmLevel : cdmaEcioLevel;
}

const SignalStrengthLevel
CdmaSignalStrengthInfo::getEvdoLevel() const
{
    SignalStrengthLevel evdoDbmLevel = SignalStrengthLevel::LEVEL_UNKNOWN;
    if (evdoDbm_ >= MIN_EVDO_DBM && evdoDbm_ <= MAX_EVDO_DBM)
        evdoDbmLevel = calculateLevel(evdoDbm_, kEvdoDbmMap);

    SignalStrengthLevel evdoSnrLevel = SignalStrengthLevel::LEVEL_UNKNOWN;
    if (evdoSignalNoiseRatio_ > MIN_EVDO_SNR && evdoSignalNoiseRatio_ <= MAX_EVDO_SNR)
        evdoSnrLevel = calculateLevel(evdoSignalNoiseRatio_, kEvdoSnrMap);

    return (evdoDbmLevel < evdoSnrLevel) ? evdoDbmLevel : evdoSnrLevel;
}

const SignalStrengthLevel
CdmaSignalStrengthInfo::getLevel() const
{
    SignalStrengthLevel cdmaLevel = getCdmaLevel();
    SignalStrengthLevel evdoLevel = getEvdoLevel();
    if (evdoLevel == SignalStrengthLevel::LEVEL_UNKNOWN)
        return cdmaLevel;
    if (cdmaLevel == SignalStrengthLevel::LEVEL_UNKNOWN)
        return evdoLevel;
    return (cdmaLevel < evdoLevel) ? cdmaLevel : evdoLevel;
}

// ---------------------------------------------------------------------------
// WcdmaSignalStrengthInfo

WcdmaSignalStrengthInfo::WcdmaSignalStrengthInfo(int signalStrength, int bitErrorRate)
{
    signalStrength_ = inRange(signalStrength, MIN_WCDMA_LEVEL, MAX_WCDMA_LEVEL);
    bitErrorRate_ = inRange(bitErrorRate, MIN_WCDMA_BIT_ERROR_RATE, MAX_WCDMA_BIT_ERROR_RATE);
    // The 2-arg ctor carries no ecio/rscp/rssi -- mark them unavailable rather
    // than leaving them indeterminate (the header documents
    // INVALID_SIGNAL_STRENGTH_VALUE as the "unavailable" sentinel).
    ecio_ = INVALID_SIGNAL_STRENGTH_VALUE;
    rscp_ = INVALID_SIGNAL_STRENGTH_VALUE;
    rssi_ = INVALID_SIGNAL_STRENGTH_VALUE;
}

WcdmaSignalStrengthInfo::WcdmaSignalStrengthInfo(
  int signalStrength,
  int bitErrorRate,
  int wcdmaEcio,
  int wcdmaRscp,
  int wcdmaRssi
)
{
    signalStrength_ = inRange(signalStrength, MIN_WCDMA_LEVEL, MAX_WCDMA_LEVEL);
    bitErrorRate_ = inRange(bitErrorRate, MIN_WCDMA_BIT_ERROR_RATE, MAX_WCDMA_BIT_ERROR_RATE);
    ecio_ = inRange(wcdmaEcio, MIN_WCDMA_ECIO, MAX_WCDMA_ECIO);
    rscp_ = inRange(wcdmaRscp, MIN_WCDMA_RSCP, MAX_WCDMA_RSCP);
    rssi_ = inRange(wcdmaRssi, MIN_RSSI, MAX_RSSI);
}

const SignalStrengthLevel
WcdmaSignalStrengthInfo::getLevel() const
{
    // Valid values are (0-31, 99) as defined in TS 27.007 8.5
    if (signalStrength_ >= MIN_WCDMA_LEVEL && signalStrength_ <= MAX_WCDMA_LEVEL)
        return calculateLevel(signalStrength_, kWcdmaLevelMap);
    return SignalStrengthLevel::LEVEL_UNKNOWN;
}

const int
WcdmaSignalStrengthInfo::getDbm() const
{
    int dBm = inRange(signalStrength_, MIN_WCDMA_LEVEL, MAX_WCDMA_LEVEL);
    if (dBm != INVALID_SIGNAL_STRENGTH_VALUE)
        dBm = WCDMA_DBM_CONVERSION_FACTOR + (WCDMA_DBM_MULTIPLICATION_FACTOR * signalStrength_);
    return dBm;
}

const int
WcdmaSignalStrengthInfo::getRssi() const
{
    return rssi_;
}

const int
WcdmaSignalStrengthInfo::getSignalStrength() const
{
    return signalStrength_;
}

const int
WcdmaSignalStrengthInfo::getBitErrorRate() const
{
    return bitErrorRate_;
}

const int
WcdmaSignalStrengthInfo::getEcio() const
{
    return ecio_;
}

const int
WcdmaSignalStrengthInfo::getRscp() const
{
    return rscp_;
}

// ---------------------------------------------------------------------------
// TdscdmaSignalStrengthInfo

TdscdmaSignalStrengthInfo::TdscdmaSignalStrengthInfo(int rscp)
{
    rscp_ = inRange(rscp, MIN_TDSCDMA_RSCP, MAX_TDSCDMA_RSCP);
}

const int
TdscdmaSignalStrengthInfo::getRscp() const
{
    return rscp_;
}

// ---------------------------------------------------------------------------
// Nr5gSignalStrengthInfo

Nr5gSignalStrengthInfo::Nr5gSignalStrengthInfo(int rsrp, int rsrq, int rssnr)
{
    rsrp_ = inRange(rsrp, MIN_NR5G_RSRP, MAX_NR5G_RSRP);
    rsrq_ = inRange(rsrq, MIN_NR5G_RSRQ, MAX_NR5G_RSRQ);
    rssnr_ = inRange(rssnr, MIN_NR5G_RSSNR_LEVEL, MAX_NR5G_RSSNR_LEVEL);
    // nr5gAsu_ is declared but has no accessor; keep it deterministic.
    nr5gAsu_ = rsrp_;
}

const int
Nr5gSignalStrengthInfo::getNr5gSignalStrength() const
{
    // Header documents [0, 97] "calculated based on RSRP". The gRPC simulation
    // exposed no such getter, so derive it the standard 3GPP way: RSRP dBm
    // offset into a 0-based scale, clamped, and UNAVAILABLE when RSRP is.
    if (rsrp_ == INVALID_SIGNAL_STRENGTH_VALUE)
        return INVALID_SIGNAL_STRENGTH_VALUE;
    int ss = rsrp_ - MIN_NR5G_RSRP;  // -140 dBm -> 0
    if (ss < 0)
        ss = 0;
    if (ss > 97)
        ss = 97;
    return ss;
}

const int
Nr5gSignalStrengthInfo::getDbm() const
{
    return rsrp_;
}

const int
Nr5gSignalStrengthInfo::getReferenceSignalReceiveQuality() const
{
    return rsrq_;
}

const int
Nr5gSignalStrengthInfo::getReferenceSignalSnr() const
{
    return rssnr_;
}

const SignalStrengthLevel
Nr5gSignalStrengthInfo::getLevel() const
{
    SignalStrengthLevel rsrpLevel = SignalStrengthLevel::LEVEL_UNKNOWN;
    if (rsrp_ >= MIN_NR5G_RSRP && rsrp_ <= MAX_NR5G_RSRP)
        rsrpLevel = calculateLevel(rsrp_, kNr5gRsrpLevelMap);

    SignalStrengthLevel rssnrLevel = SignalStrengthLevel::LEVEL_UNKNOWN;
    if (rssnr_ >= MIN_NR5G_RSSNR_LEVEL && rssnr_ <= MAX_NR5G_RSSNR_LEVEL)
        rssnrLevel = calculateLevel(rssnr_, kNr5gRssnrLevelMap);

    if (rsrpLevel != SignalStrengthLevel::LEVEL_UNKNOWN
        && rssnrLevel != SignalStrengthLevel::LEVEL_UNKNOWN)
    {
        return (rsrpLevel > rssnrLevel ? rsrpLevel : rssnrLevel);
    }
    if (rssnrLevel != SignalStrengthLevel::LEVEL_UNKNOWN)
        return rssnrLevel;
    if (rsrpLevel != SignalStrengthLevel::LEVEL_UNKNOWN)
        return rsrpLevel;
    return SignalStrengthLevel::LEVEL_UNKNOWN;
}

// ---------------------------------------------------------------------------
// Nb1NtnSignalStrengthInfo
//
// New in this header (absent from the gRPC simulation entirely). NB-IoT(NB1)
// NTN reuses LTE's RSRP/RSRQ/RSSNR envelopes per the header's documented
// ranges, so the LTE level tables are reused rather than inventing new ones.

Nb1NtnSignalStrengthInfo::Nb1NtnSignalStrengthInfo(
  int signalStrength,
  int rsrp,
  int rsrq,
  int rssnr,
  int rssi
)
{
    signalStrength_ = inRange(signalStrength, MIN_NB1_SIGNAL_STRENGTH, MAX_NB1_SIGNAL_STRENGTH);
    rsrp_ = inRange(rsrp, MIN_NB1_RSRP, MAX_NB1_RSRP);
    rsrq_ = inRange(rsrq, MIN_NB1_RSRQ, MAX_NB1_RSRQ);
    rssnr_ = inRange(rssnr, MIN_NB1_RSSNR, MAX_NB1_RSSNR);
    rssi_ = inRange(rssi, MIN_RSSI, MAX_RSSI);
}

const SignalStrengthLevel
Nb1NtnSignalStrengthInfo::getLevel() const
{
    SignalStrengthLevel rsrpLevel = SignalStrengthLevel::LEVEL_UNKNOWN;
    if (rsrp_ >= MIN_NB1_RSRP && rsrp_ <= MAX_NB1_RSRP)
        rsrpLevel = calculateLevel(rsrp_, kLteRsrpLevelMap);

    SignalStrengthLevel rssnrLevel = SignalStrengthLevel::LEVEL_UNKNOWN;
    if (rssnr_ >= MIN_NB1_RSSNR && rssnr_ <= MAX_NB1_RSSNR)
        rssnrLevel = calculateLevel(rssnr_, kLteRssnrLevelMap);

    if (rsrpLevel != SignalStrengthLevel::LEVEL_UNKNOWN
        && rssnrLevel != SignalStrengthLevel::LEVEL_UNKNOWN)
    {
        return (rsrpLevel > rssnrLevel ? rsrpLevel : rssnrLevel);
    }
    if (rssnrLevel != SignalStrengthLevel::LEVEL_UNKNOWN)
        return rssnrLevel;
    return rsrpLevel;
}

const int
Nb1NtnSignalStrengthInfo::getDbm() const
{
    return rsrp_;
}

const int
Nb1NtnSignalStrengthInfo::getRssi() const
{
    return rssi_;
}

const int
Nb1NtnSignalStrengthInfo::getSignalStrength() const
{
    return signalStrength_;
}

const int
Nb1NtnSignalStrengthInfo::getRsrq() const
{
    return rsrq_;
}

const int
Nb1NtnSignalStrengthInfo::getRssnr() const
{
    return rssnr_;
}

}  // namespace tel
}  // namespace telux
