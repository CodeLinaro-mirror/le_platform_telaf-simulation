// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "ServingSystemManager.hpp"

#include "../common/EventCast.hpp"
#include "../common/ListenerDispatchAO.hpp"
#include "../common/Log.hpp"
#include "Signals.hpp"
#include "generated/cpp/topics.h"

#include <chart/event.hpp>
#include <chart/hsm.hpp>
#include <chart/spy.hpp>
#include <future>
#include <nlohmann/json.hpp>

namespace telux::tel::simula {

using namespace TelSignals;

namespace {

using common::simula::Envelope;
using common::simula::event_cast;

telux::tel::RadioTechnology
wireToRadioTechnology(const std::string& s)
{
    if (s == "RADIO_TECH_GPRS")     return telux::tel::RadioTechnology::RADIO_TECH_GPRS;
    if (s == "RADIO_TECH_EDGE")     return telux::tel::RadioTechnology::RADIO_TECH_EDGE;
    if (s == "RADIO_TECH_UMTS")     return telux::tel::RadioTechnology::RADIO_TECH_UMTS;
    if (s == "RADIO_TECH_LTE")      return telux::tel::RadioTechnology::RADIO_TECH_LTE;
    if (s == "RADIO_TECH_GSM")      return telux::tel::RadioTechnology::RADIO_TECH_GSM;
    if (s == "RADIO_TECH_TD_SCDMA") return telux::tel::RadioTechnology::RADIO_TECH_TD_SCDMA;
    if (s == "RADIO_TECH_LTE_CA")   return telux::tel::RadioTechnology::RADIO_TECH_LTE_CA;
    if (s == "RADIO_TECH_NR5G")     return telux::tel::RadioTechnology::RADIO_TECH_NR5G;
    LOG_WARN(
      "[ServingSystemManager] wireToRadioTechnology: unrecognized value \"%s\" -- defaulting to RADIO_TECH_UNKNOWN",
      s.c_str()
    );
    return telux::tel::RadioTechnology::RADIO_TECH_UNKNOWN;
}

telux::tel::ServiceDomain
wireToServiceDomain(const std::string& s)
{
    if (s == "NO_SRV")  return telux::tel::ServiceDomain::NO_SRV;
    if (s == "CS_ONLY") return telux::tel::ServiceDomain::CS_ONLY;
    if (s == "PS_ONLY") return telux::tel::ServiceDomain::PS_ONLY;
    if (s == "CS_PS")   return telux::tel::ServiceDomain::CS_PS;
    if (s == "CAMPED")  return telux::tel::ServiceDomain::CAMPED;
    LOG_WARN(
      "[ServingSystemManager] wireToServiceDomain: unrecognized value \"%s\" -- defaulting to UNKNOWN",
      s.c_str()
    );
    return telux::tel::ServiceDomain::UNKNOWN;
}

telux::tel::ServiceRegistrationState
wireToServiceRegistrationState(const std::string& s)
{
    if (s == "NO_SERVICE")       return telux::tel::ServiceRegistrationState::NO_SERVICE;
    if (s == "LIMITED_SERVICE")  return telux::tel::ServiceRegistrationState::LIMITED_SERVICE;
    if (s == "IN_SERVICE")       return telux::tel::ServiceRegistrationState::IN_SERVICE;
    if (s == "LIMITED_REGIONAL") return telux::tel::ServiceRegistrationState::LIMITED_REGIONAL;
    if (s == "POWER_SAVE")       return telux::tel::ServiceRegistrationState::POWER_SAVE;
    LOG_WARN(
      "[ServingSystemManager] wireToServiceRegistrationState: unrecognized value \"%s\" -- defaulting to UNKNOWN",
      s.c_str()
    );
    return telux::tel::ServiceRegistrationState::UNKNOWN;
}

telux::tel::EndcAvailability
wireToEndcAvailability(const std::string& s)
{
    if (s == "AVAILABLE")   return telux::tel::EndcAvailability::AVAILABLE;
    if (s == "UNAVAILABLE") return telux::tel::EndcAvailability::UNAVAILABLE;
    LOG_WARN(
      "[ServingSystemManager] wireToEndcAvailability: unrecognized value \"%s\" -- defaulting to UNKNOWN",
      s.c_str()
    );
    return telux::tel::EndcAvailability::UNKNOWN;
}

telux::tel::DcnrRestriction
wireToDcnrRestriction(const std::string& s)
{
    if (s == "RESTRICTED")   return telux::tel::DcnrRestriction::RESTRICTED;
    if (s == "UNRESTRICTED") return telux::tel::DcnrRestriction::UNRESTRICTED;
    LOG_WARN(
      "[ServingSystemManager] wireToDcnrRestriction: unrecognized value \"%s\" -- defaulting to UNKNOWN",
      s.c_str()
    );
    return telux::tel::DcnrRestriction::UNKNOWN;
}

telux::tel::LteCsCapability
wireToLteCsCapability(const std::string& s)
{
    if (s == "FULL_SERVICE")        return telux::tel::LteCsCapability::FULL_SERVICE;
    if (s == "CSFB_NOT_PREFERRED")  return telux::tel::LteCsCapability::CSFB_NOT_PREFERRED;
    if (s == "SMS_ONLY")            return telux::tel::LteCsCapability::SMS_ONLY;
    if (s == "LIMITED")             return telux::tel::LteCsCapability::LIMITED;
    if (s == "BARRED")              return telux::tel::LteCsCapability::BARRED;
    LOG_WARN(
      "[ServingSystemManager] wireToLteCsCapability: unrecognized value \"%s\" -- defaulting to UNKNOWN",
      s.c_str()
    );
    return telux::tel::LteCsCapability::UNKNOWN;
}

// telux::tel::RFBand/RFBandWidth enumerator names, reused verbatim on the
// wire -- see registry/radio.yaml's request_rf_band_info.rsp schema
// comment. Exhaustive by construction (every enumerator in
// ServingSystemDefines.hpp has a branch here).
telux::tel::RFBand
wireToRFBand(const std::string& s)
{
    if (s == "BC_0") return telux::tel::RFBand::BC_0;
    if (s == "BC_1") return telux::tel::RFBand::BC_1;
    if (s == "BC_3") return telux::tel::RFBand::BC_3;
    if (s == "BC_4") return telux::tel::RFBand::BC_4;
    if (s == "BC_5") return telux::tel::RFBand::BC_5;
    if (s == "BC_6") return telux::tel::RFBand::BC_6;
    if (s == "BC_7") return telux::tel::RFBand::BC_7;
    if (s == "BC_8") return telux::tel::RFBand::BC_8;
    if (s == "BC_9") return telux::tel::RFBand::BC_9;
    if (s == "BC_10") return telux::tel::RFBand::BC_10;
    if (s == "BC_11") return telux::tel::RFBand::BC_11;
    if (s == "BC_12") return telux::tel::RFBand::BC_12;
    if (s == "BC_13") return telux::tel::RFBand::BC_13;
    if (s == "BC_14") return telux::tel::RFBand::BC_14;
    if (s == "BC_15") return telux::tel::RFBand::BC_15;
    if (s == "BC_16") return telux::tel::RFBand::BC_16;
    if (s == "BC_17") return telux::tel::RFBand::BC_17;
    if (s == "BC_18") return telux::tel::RFBand::BC_18;
    if (s == "BC_19") return telux::tel::RFBand::BC_19;
    if (s == "GSM_450") return telux::tel::RFBand::GSM_450;
    if (s == "GSM_480") return telux::tel::RFBand::GSM_480;
    if (s == "GSM_750") return telux::tel::RFBand::GSM_750;
    if (s == "GSM_850") return telux::tel::RFBand::GSM_850;
    if (s == "GSM_900_EXTENDED") return telux::tel::RFBand::GSM_900_EXTENDED;
    if (s == "GSM_900_PRIMARY") return telux::tel::RFBand::GSM_900_PRIMARY;
    if (s == "GSM_900_RAILWAYS") return telux::tel::RFBand::GSM_900_RAILWAYS;
    if (s == "GSM_1800") return telux::tel::RFBand::GSM_1800;
    if (s == "GSM_1900") return telux::tel::RFBand::GSM_1900;
    if (s == "WCDMA_2100") return telux::tel::RFBand::WCDMA_2100;
    if (s == "WCDMA_PCS_1900") return telux::tel::RFBand::WCDMA_PCS_1900;
    if (s == "WCDMA_DCS_1800") return telux::tel::RFBand::WCDMA_DCS_1800;
    if (s == "WCDMA_1700_US") return telux::tel::RFBand::WCDMA_1700_US;
    if (s == "WCDMA_850") return telux::tel::RFBand::WCDMA_850;
    if (s == "WCDMA_800") return telux::tel::RFBand::WCDMA_800;
    if (s == "WCDMA_2600") return telux::tel::RFBand::WCDMA_2600;
    if (s == "WCDMA_900") return telux::tel::RFBand::WCDMA_900;
    if (s == "WCDMA_1700_JAPAN") return telux::tel::RFBand::WCDMA_1700_JAPAN;
    if (s == "WCDMA_1500_JAPAN") return telux::tel::RFBand::WCDMA_1500_JAPAN;
    if (s == "WCDMA_850_JAPAN") return telux::tel::RFBand::WCDMA_850_JAPAN;
    if (s == "E_UTRA_OPERATING_BAND_1") return telux::tel::RFBand::E_UTRA_OPERATING_BAND_1;
    if (s == "E_UTRA_OPERATING_BAND_2") return telux::tel::RFBand::E_UTRA_OPERATING_BAND_2;
    if (s == "E_UTRA_OPERATING_BAND_3") return telux::tel::RFBand::E_UTRA_OPERATING_BAND_3;
    if (s == "E_UTRA_OPERATING_BAND_4") return telux::tel::RFBand::E_UTRA_OPERATING_BAND_4;
    if (s == "E_UTRA_OPERATING_BAND_5") return telux::tel::RFBand::E_UTRA_OPERATING_BAND_5;
    if (s == "E_UTRA_OPERATING_BAND_6") return telux::tel::RFBand::E_UTRA_OPERATING_BAND_6;
    if (s == "E_UTRA_OPERATING_BAND_7") return telux::tel::RFBand::E_UTRA_OPERATING_BAND_7;
    if (s == "E_UTRA_OPERATING_BAND_8") return telux::tel::RFBand::E_UTRA_OPERATING_BAND_8;
    if (s == "E_UTRA_OPERATING_BAND_9") return telux::tel::RFBand::E_UTRA_OPERATING_BAND_9;
    if (s == "E_UTRA_OPERATING_BAND_10") return telux::tel::RFBand::E_UTRA_OPERATING_BAND_10;
    if (s == "E_UTRA_OPERATING_BAND_11") return telux::tel::RFBand::E_UTRA_OPERATING_BAND_11;
    if (s == "E_UTRA_OPERATING_BAND_12") return telux::tel::RFBand::E_UTRA_OPERATING_BAND_12;
    if (s == "E_UTRA_OPERATING_BAND_13") return telux::tel::RFBand::E_UTRA_OPERATING_BAND_13;
    if (s == "E_UTRA_OPERATING_BAND_14") return telux::tel::RFBand::E_UTRA_OPERATING_BAND_14;
    if (s == "E_UTRA_OPERATING_BAND_17") return telux::tel::RFBand::E_UTRA_OPERATING_BAND_17;
    if (s == "E_UTRA_OPERATING_BAND_33") return telux::tel::RFBand::E_UTRA_OPERATING_BAND_33;
    if (s == "E_UTRA_OPERATING_BAND_34") return telux::tel::RFBand::E_UTRA_OPERATING_BAND_34;
    if (s == "E_UTRA_OPERATING_BAND_35") return telux::tel::RFBand::E_UTRA_OPERATING_BAND_35;
    if (s == "E_UTRA_OPERATING_BAND_36") return telux::tel::RFBand::E_UTRA_OPERATING_BAND_36;
    if (s == "E_UTRA_OPERATING_BAND_37") return telux::tel::RFBand::E_UTRA_OPERATING_BAND_37;
    if (s == "E_UTRA_OPERATING_BAND_38") return telux::tel::RFBand::E_UTRA_OPERATING_BAND_38;
    if (s == "E_UTRA_OPERATING_BAND_39") return telux::tel::RFBand::E_UTRA_OPERATING_BAND_39;
    if (s == "E_UTRA_OPERATING_BAND_40") return telux::tel::RFBand::E_UTRA_OPERATING_BAND_40;
    if (s == "E_UTRA_OPERATING_BAND_18") return telux::tel::RFBand::E_UTRA_OPERATING_BAND_18;
    if (s == "E_UTRA_OPERATING_BAND_19") return telux::tel::RFBand::E_UTRA_OPERATING_BAND_19;
    if (s == "E_UTRA_OPERATING_BAND_20") return telux::tel::RFBand::E_UTRA_OPERATING_BAND_20;
    if (s == "E_UTRA_OPERATING_BAND_21") return telux::tel::RFBand::E_UTRA_OPERATING_BAND_21;
    if (s == "E_UTRA_OPERATING_BAND_24") return telux::tel::RFBand::E_UTRA_OPERATING_BAND_24;
    if (s == "E_UTRA_OPERATING_BAND_25") return telux::tel::RFBand::E_UTRA_OPERATING_BAND_25;
    if (s == "E_UTRA_OPERATING_BAND_41") return telux::tel::RFBand::E_UTRA_OPERATING_BAND_41;
    if (s == "E_UTRA_OPERATING_BAND_42") return telux::tel::RFBand::E_UTRA_OPERATING_BAND_42;
    if (s == "E_UTRA_OPERATING_BAND_43") return telux::tel::RFBand::E_UTRA_OPERATING_BAND_43;
    if (s == "E_UTRA_OPERATING_BAND_23") return telux::tel::RFBand::E_UTRA_OPERATING_BAND_23;
    if (s == "E_UTRA_OPERATING_BAND_26") return telux::tel::RFBand::E_UTRA_OPERATING_BAND_26;
    if (s == "E_UTRA_OPERATING_BAND_32") return telux::tel::RFBand::E_UTRA_OPERATING_BAND_32;
    if (s == "E_UTRA_OPERATING_BAND_125") return telux::tel::RFBand::E_UTRA_OPERATING_BAND_125;
    if (s == "E_UTRA_OPERATING_BAND_126") return telux::tel::RFBand::E_UTRA_OPERATING_BAND_126;
    if (s == "E_UTRA_OPERATING_BAND_127") return telux::tel::RFBand::E_UTRA_OPERATING_BAND_127;
    if (s == "E_UTRA_OPERATING_BAND_28") return telux::tel::RFBand::E_UTRA_OPERATING_BAND_28;
    if (s == "E_UTRA_OPERATING_BAND_29") return telux::tel::RFBand::E_UTRA_OPERATING_BAND_29;
    if (s == "E_UTRA_OPERATING_BAND_30") return telux::tel::RFBand::E_UTRA_OPERATING_BAND_30;
    if (s == "E_UTRA_OPERATING_BAND_66") return telux::tel::RFBand::E_UTRA_OPERATING_BAND_66;
    if (s == "E_UTRA_OPERATING_BAND_250") return telux::tel::RFBand::E_UTRA_OPERATING_BAND_250;
    if (s == "E_UTRA_OPERATING_BAND_46") return telux::tel::RFBand::E_UTRA_OPERATING_BAND_46;
    if (s == "E_UTRA_OPERATING_BAND_27") return telux::tel::RFBand::E_UTRA_OPERATING_BAND_27;
    if (s == "E_UTRA_OPERATING_BAND_31") return telux::tel::RFBand::E_UTRA_OPERATING_BAND_31;
    if (s == "E_UTRA_OPERATING_BAND_71") return telux::tel::RFBand::E_UTRA_OPERATING_BAND_71;
    if (s == "E_UTRA_OPERATING_BAND_47") return telux::tel::RFBand::E_UTRA_OPERATING_BAND_47;
    if (s == "E_UTRA_OPERATING_BAND_48") return telux::tel::RFBand::E_UTRA_OPERATING_BAND_48;
    if (s == "E_UTRA_OPERATING_BAND_67") return telux::tel::RFBand::E_UTRA_OPERATING_BAND_67;
    if (s == "E_UTRA_OPERATING_BAND_68") return telux::tel::RFBand::E_UTRA_OPERATING_BAND_68;
    if (s == "E_UTRA_OPERATING_BAND_49") return telux::tel::RFBand::E_UTRA_OPERATING_BAND_49;
    if (s == "E_UTRA_OPERATING_BAND_85") return telux::tel::RFBand::E_UTRA_OPERATING_BAND_85;
    if (s == "E_UTRA_OPERATING_BAND_72") return telux::tel::RFBand::E_UTRA_OPERATING_BAND_72;
    if (s == "E_UTRA_OPERATING_BAND_73") return telux::tel::RFBand::E_UTRA_OPERATING_BAND_73;
    if (s == "E_UTRA_OPERATING_BAND_86") return telux::tel::RFBand::E_UTRA_OPERATING_BAND_86;
    if (s == "E_UTRA_OPERATING_BAND_53") return telux::tel::RFBand::E_UTRA_OPERATING_BAND_53;
    if (s == "E_UTRA_OPERATING_BAND_87") return telux::tel::RFBand::E_UTRA_OPERATING_BAND_87;
    if (s == "E_UTRA_OPERATING_BAND_88") return telux::tel::RFBand::E_UTRA_OPERATING_BAND_88;
    if (s == "E_UTRA_OPERATING_BAND_70") return telux::tel::RFBand::E_UTRA_OPERATING_BAND_70;
    if (s == "TDSCDMA_BAND_A") return telux::tel::RFBand::TDSCDMA_BAND_A;
    if (s == "TDSCDMA_BAND_B") return telux::tel::RFBand::TDSCDMA_BAND_B;
    if (s == "TDSCDMA_BAND_C") return telux::tel::RFBand::TDSCDMA_BAND_C;
    if (s == "TDSCDMA_BAND_D") return telux::tel::RFBand::TDSCDMA_BAND_D;
    if (s == "TDSCDMA_BAND_E") return telux::tel::RFBand::TDSCDMA_BAND_E;
    if (s == "TDSCDMA_BAND_F") return telux::tel::RFBand::TDSCDMA_BAND_F;
    if (s == "NR5G_BAND_1") return telux::tel::RFBand::NR5G_BAND_1;
    if (s == "NR5G_BAND_2") return telux::tel::RFBand::NR5G_BAND_2;
    if (s == "NR5G_BAND_3") return telux::tel::RFBand::NR5G_BAND_3;
    if (s == "NR5G_BAND_5") return telux::tel::RFBand::NR5G_BAND_5;
    if (s == "NR5G_BAND_7") return telux::tel::RFBand::NR5G_BAND_7;
    if (s == "NR5G_BAND_8") return telux::tel::RFBand::NR5G_BAND_8;
    if (s == "NR5G_BAND_20") return telux::tel::RFBand::NR5G_BAND_20;
    if (s == "NR5G_BAND_28") return telux::tel::RFBand::NR5G_BAND_28;
    if (s == "NR5G_BAND_38") return telux::tel::RFBand::NR5G_BAND_38;
    if (s == "NR5G_BAND_41") return telux::tel::RFBand::NR5G_BAND_41;
    if (s == "NR5G_BAND_50") return telux::tel::RFBand::NR5G_BAND_50;
    if (s == "NR5G_BAND_51") return telux::tel::RFBand::NR5G_BAND_51;
    if (s == "NR5G_BAND_66") return telux::tel::RFBand::NR5G_BAND_66;
    if (s == "NR5G_BAND_70") return telux::tel::RFBand::NR5G_BAND_70;
    if (s == "NR5G_BAND_71") return telux::tel::RFBand::NR5G_BAND_71;
    if (s == "NR5G_BAND_74") return telux::tel::RFBand::NR5G_BAND_74;
    if (s == "NR5G_BAND_75") return telux::tel::RFBand::NR5G_BAND_75;
    if (s == "NR5G_BAND_76") return telux::tel::RFBand::NR5G_BAND_76;
    if (s == "NR5G_BAND_77") return telux::tel::RFBand::NR5G_BAND_77;
    if (s == "NR5G_BAND_78") return telux::tel::RFBand::NR5G_BAND_78;
    if (s == "NR5G_BAND_79") return telux::tel::RFBand::NR5G_BAND_79;
    if (s == "NR5G_BAND_80") return telux::tel::RFBand::NR5G_BAND_80;
    if (s == "NR5G_BAND_81") return telux::tel::RFBand::NR5G_BAND_81;
    if (s == "NR5G_BAND_82") return telux::tel::RFBand::NR5G_BAND_82;
    if (s == "NR5G_BAND_83") return telux::tel::RFBand::NR5G_BAND_83;
    if (s == "NR5G_BAND_84") return telux::tel::RFBand::NR5G_BAND_84;
    if (s == "NR5G_BAND_85") return telux::tel::RFBand::NR5G_BAND_85;
    if (s == "NR5G_BAND_257") return telux::tel::RFBand::NR5G_BAND_257;
    if (s == "NR5G_BAND_258") return telux::tel::RFBand::NR5G_BAND_258;
    if (s == "NR5G_BAND_259") return telux::tel::RFBand::NR5G_BAND_259;
    if (s == "NR5G_BAND_260") return telux::tel::RFBand::NR5G_BAND_260;
    if (s == "NR5G_BAND_261") return telux::tel::RFBand::NR5G_BAND_261;
    if (s == "NR5G_BAND_12") return telux::tel::RFBand::NR5G_BAND_12;
    if (s == "NR5G_BAND_25") return telux::tel::RFBand::NR5G_BAND_25;
    if (s == "NR5G_BAND_34") return telux::tel::RFBand::NR5G_BAND_34;
    if (s == "NR5G_BAND_39") return telux::tel::RFBand::NR5G_BAND_39;
    if (s == "NR5G_BAND_40") return telux::tel::RFBand::NR5G_BAND_40;
    if (s == "NR5G_BAND_65") return telux::tel::RFBand::NR5G_BAND_65;
    if (s == "NR5G_BAND_86") return telux::tel::RFBand::NR5G_BAND_86;
    if (s == "NR5G_BAND_48") return telux::tel::RFBand::NR5G_BAND_48;
    if (s == "NR5G_BAND_14") return telux::tel::RFBand::NR5G_BAND_14;
    if (s == "NR5G_BAND_13") return telux::tel::RFBand::NR5G_BAND_13;
    if (s == "NR5G_BAND_18") return telux::tel::RFBand::NR5G_BAND_18;
    if (s == "NR5G_BAND_26") return telux::tel::RFBand::NR5G_BAND_26;
    if (s == "NR5G_BAND_30") return telux::tel::RFBand::NR5G_BAND_30;
    if (s == "NR5G_BAND_29") return telux::tel::RFBand::NR5G_BAND_29;
    if (s == "NR5G_BAND_53") return telux::tel::RFBand::NR5G_BAND_53;
    if (s == "NR5G_BAND_46") return telux::tel::RFBand::NR5G_BAND_46;
    if (s == "NR5G_BAND_91") return telux::tel::RFBand::NR5G_BAND_91;
    if (s == "NR5G_BAND_92") return telux::tel::RFBand::NR5G_BAND_92;
    if (s == "NR5G_BAND_93") return telux::tel::RFBand::NR5G_BAND_93;
    if (s == "NR5G_BAND_94") return telux::tel::RFBand::NR5G_BAND_94;
    LOG_WARN(
      "[ServingSystemManager] wireToRFBand: unrecognized value \"%s\" -- defaulting to INVALID",
      s.c_str()
    );
    return telux::tel::RFBand::INVALID;
}

telux::tel::RFBandWidth
wireToRFBandWidth(const std::string& s)
{
    if (s == "LTE_BW_NRB_6") return telux::tel::RFBandWidth::LTE_BW_NRB_6;
    if (s == "LTE_BW_NRB_15") return telux::tel::RFBandWidth::LTE_BW_NRB_15;
    if (s == "LTE_BW_NRB_25") return telux::tel::RFBandWidth::LTE_BW_NRB_25;
    if (s == "LTE_BW_NRB_50") return telux::tel::RFBandWidth::LTE_BW_NRB_50;
    if (s == "LTE_BW_NRB_75") return telux::tel::RFBandWidth::LTE_BW_NRB_75;
    if (s == "LTE_BW_NRB_100") return telux::tel::RFBandWidth::LTE_BW_NRB_100;
    if (s == "NR5G_BW_NRB_5") return telux::tel::RFBandWidth::NR5G_BW_NRB_5;
    if (s == "NR5G_BW_NRB_10") return telux::tel::RFBandWidth::NR5G_BW_NRB_10;
    if (s == "NR5G_BW_NRB_15") return telux::tel::RFBandWidth::NR5G_BW_NRB_15;
    if (s == "NR5G_BW_NRB_20") return telux::tel::RFBandWidth::NR5G_BW_NRB_20;
    if (s == "NR5G_BW_NRB_25") return telux::tel::RFBandWidth::NR5G_BW_NRB_25;
    if (s == "NR5G_BW_NRB_30") return telux::tel::RFBandWidth::NR5G_BW_NRB_30;
    if (s == "NR5G_BW_NRB_40") return telux::tel::RFBandWidth::NR5G_BW_NRB_40;
    if (s == "NR5G_BW_NRB_50") return telux::tel::RFBandWidth::NR5G_BW_NRB_50;
    if (s == "NR5G_BW_NRB_60") return telux::tel::RFBandWidth::NR5G_BW_NRB_60;
    if (s == "NR5G_BW_NRB_80") return telux::tel::RFBandWidth::NR5G_BW_NRB_80;
    if (s == "NR5G_BW_NRB_90") return telux::tel::RFBandWidth::NR5G_BW_NRB_90;
    if (s == "NR5G_BW_NRB_100") return telux::tel::RFBandWidth::NR5G_BW_NRB_100;
    if (s == "NR5G_BW_NRB_200") return telux::tel::RFBandWidth::NR5G_BW_NRB_200;
    if (s == "NR5G_BW_NRB_400") return telux::tel::RFBandWidth::NR5G_BW_NRB_400;
    if (s == "GSM_BW_NRB_2") return telux::tel::RFBandWidth::GSM_BW_NRB_2;
    if (s == "TDSCDMA_BW_NRB_2") return telux::tel::RFBandWidth::TDSCDMA_BW_NRB_2;
    if (s == "WCDMA_BW_NRB_5") return telux::tel::RFBandWidth::WCDMA_BW_NRB_5;
    if (s == "WCDMA_BW_NRB_10") return telux::tel::RFBandWidth::WCDMA_BW_NRB_10;
    if (s == "NR5G_BW_NRB_70") return telux::tel::RFBandWidth::NR5G_BW_NRB_70;
    LOG_WARN(
      "[ServingSystemManager] wireToRFBandWidth: unrecognized value \"%s\" -- defaulting to INVALID_BANDWIDTH",
      s.c_str()
    );
    return telux::tel::RFBandWidth::INVALID_BANDWIDTH;
}

// telux::tel::RatPrefType enumerators with their PREF_ prefix stripped --
// see registry/radio.yaml's set_rat_preference.req schema comment.
std::string
ratPrefToWire(telux::tel::RatPrefType t)
{
    switch (t)
    {
        case telux::tel::RatPrefType::PREF_CDMA_1X:   return "CDMA_1X";
        case telux::tel::RatPrefType::PREF_CDMA_EVDO: return "CDMA_EVDO";
        case telux::tel::RatPrefType::PREF_GSM:       return "GSM";
        case telux::tel::RatPrefType::PREF_WCDMA:     return "WCDMA";
        case telux::tel::RatPrefType::PREF_LTE:       return "LTE";
        case telux::tel::RatPrefType::PREF_TDSCDMA:   return "TDSCDMA";
        case telux::tel::RatPrefType::PREF_NR5G:      return "NR5G";
        case telux::tel::RatPrefType::PREF_NR5G_NSA:  return "NR5G_NSA";
        case telux::tel::RatPrefType::PREF_NR5G_SA:   return "NR5G_SA";
        default:                                        return "NB1_NTN";
    }
}

telux::tel::RatPrefType
wireToRatPref(const std::string& s)
{
    if (s == "CDMA_1X")   return telux::tel::RatPrefType::PREF_CDMA_1X;
    if (s == "CDMA_EVDO") return telux::tel::RatPrefType::PREF_CDMA_EVDO;
    if (s == "GSM")       return telux::tel::RatPrefType::PREF_GSM;
    if (s == "WCDMA")     return telux::tel::RatPrefType::PREF_WCDMA;
    if (s == "LTE")       return telux::tel::RatPrefType::PREF_LTE;
    if (s == "TDSCDMA")   return telux::tel::RatPrefType::PREF_TDSCDMA;
    if (s == "NR5G")      return telux::tel::RatPrefType::PREF_NR5G;
    if (s == "NR5G_NSA")  return telux::tel::RatPrefType::PREF_NR5G_NSA;
    if (s == "NR5G_SA")   return telux::tel::RatPrefType::PREF_NR5G_SA;
    LOG_WARN(
      "[ServingSystemManager] wireToRatPref: unrecognized value \"%s\" -- defaulting to PREF_NB1_NTN",
      s.c_str()
    );
    return telux::tel::RatPrefType::PREF_NB1_NTN;
}

constexpr auto kRpcTimeout = std::chrono::seconds(30);

struct StateIndPld
{
    Envelope env;
};

struct SetRatPreferencePld
{
    telux::tel::RatPreference ratPref;
    telux::common::ResponseCallback cb;
};

struct RequestRatPreferencePld
{
    telux::tel::RatPreferenceCallback cb;
};

struct RequestRFBandInfoPld
{
    telux::tel::RFBandInfoCallback cb;
};

struct SetInitCbPld
{
    telux::common::InitResponseCb cb;
};

}  // namespace

chart::Status
TelServNotReady_St(chart::Hsm*, chart::Event const*);
chart::Status
TelServReady_St(chart::Hsm*, chart::Event const*);

SimulaTelServingSystemManager::SimulaTelServingSystemManager(
  int slotId,
  common::simula::IModemBridge& bridge,
  telux::common::InitResponseCb initCb
)
    : chart::ActiveObject("TelServingSystemManager")
    , bridge_(bridge)
    , slotId_(slotId)
{
    if (initCb)
        init_cbs_.push_back(std::move(initCb));
}

SimulaTelServingSystemManager::~SimulaTelServingSystemManager()
{
    // Withdraw from the bridge before anything else: the callbacks below
    // capture raw `this` and the bridge holds its own copies. unsubscribe_*
    // is only a queued removal, so the drain() fence inside is what actually
    // makes "no callback can reach us" true.
    unsubscribeFromBridge_();
    stop();
}

void
SimulaTelServingSystemManager::unsubscribeFromBridge_()
{
    bridge_.unsubscribe_event(topics::radio::subsys_ready_serv::ind);
    bridge_.unsubscribe_event(topics::radio::sys_info::ind);
    bridge_.unsubscribe_event(topics::radio::dc_status::ind);
    bridge_.unsubscribe_event(topics::radio::rat_pref::ind);
    bridge_.unsubscribe_event(topics::radio::lte_cs_capability::ind);
    bridge_.unsubscribe_connectivity(conn_token_);
    conn_token_ = 0;
    bridge_.drain();
}

void
SimulaTelServingSystemManager::start()
{
    if (running())
        return;
    LOG_INFO("[ServingSystemManager] start() slot=%d", slotId_);
    set_instrument(std::make_unique<chart::SpyInstrument>(name()));
    start_at(TelServNotReady_St);

    bridge_.subscribe_event(
      topics::radio::subsys_ready_serv::ind,
      "radio.subsys_ready_serv.ind",
      [this](std::string_view topic, const Envelope& env) { handleReadinessInd_(topic, env); }
    );
    bridge_.subscribe_event(
      topics::radio::sys_info::ind,
      "radio.sys_info.ind",
      [this](std::string_view topic, const Envelope& env) { handleSysInfoInd_(topic, env); }
    );
    bridge_.subscribe_event(
      topics::radio::dc_status::ind,
      "radio.dc_status.ind",
      [this](std::string_view topic, const Envelope& env) { handleDcStatusInd_(topic, env); }
    );
    bridge_.subscribe_event(
      topics::radio::rat_pref::ind,
      "radio.rat_pref.ind",
      [this](std::string_view topic, const Envelope& env) { handleRatPrefInd_(topic, env); }
    );
    bridge_.subscribe_event(
      topics::radio::lte_cs_capability::ind,
      "radio.lte_cs_capability.ind",
      [this](std::string_view topic, const Envelope& env) { handleLteCsCapabilityInd_(topic, env); }
    );
    conn_token_ = bridge_.subscribe_connectivity([this](bool operational) {
        auto pld = std::make_shared<bool>(operational);
        post_fifo({ BridgeConnectivityChanged_Signal, pld });
    });
}

void
SimulaTelServingSystemManager::setInitCallback(telux::common::InitResponseCb cb)
{
    LOG_DEBUG("[ServingSystemManager] setInitCallback slot=%d hasCb=%d", slotId_, cb ? 1 : 0);
    if (!cb)
        return;
    // Already Ready: TelServReady_St's Entry has fired and will not fire
    // again for this caller, so queuing into init_cbs_ would strand it.
    if (isReadyDerived_())
    {
        cb(telux::common::ServiceStatus::SERVICE_AVAILABLE);
        return;
    }
    // Not Ready -- hand off through the AO; init_cbs_ is appended to and read
    // only on the worker thread.
    auto pld = std::make_shared<SetInitCbPld>();
    pld->cb = std::move(cb);
    post_fifo({ SetInitCb_Signal, pld });
}

void
SimulaTelServingSystemManager::handleReadinessInd_(std::string_view /*topic*/, const Envelope& env)
{
    LOG_DEBUG("[ServingSystemManager] handleReadinessInd_ fired corrId=%s", env.corrId.c_str());
    auto pld = std::make_shared<StateIndPld>();
    pld->env = env;
    post_fifo({ ReadinessEvt_Signal, pld });
}

void
SimulaTelServingSystemManager::handleSysInfoInd_(std::string_view /*topic*/, const Envelope& env)
{
    LOG_DEBUG("[ServingSystemManager] handleSysInfoInd_ fired corrId=%s", env.corrId.c_str());
    auto pld = std::make_shared<StateIndPld>();
    pld->env = env;
    post_fifo({ SysInfoEvt_Signal, pld });
}

void
SimulaTelServingSystemManager::handleDcStatusInd_(std::string_view /*topic*/, const Envelope& env)
{
    LOG_DEBUG("[ServingSystemManager] handleDcStatusInd_ fired corrId=%s", env.corrId.c_str());
    auto pld = std::make_shared<StateIndPld>();
    pld->env = env;
    post_fifo({ DcStatusEvt_Signal, pld });
}

void
SimulaTelServingSystemManager::handleRatPrefInd_(std::string_view /*topic*/, const Envelope& env)
{
    LOG_DEBUG("[ServingSystemManager] handleRatPrefInd_ fired corrId=%s", env.corrId.c_str());
    auto pld = std::make_shared<StateIndPld>();
    pld->env = env;
    post_fifo({ RatPrefEvt_Signal, pld });
}

void
SimulaTelServingSystemManager::handleLteCsCapabilityInd_(std::string_view /*topic*/, const Envelope& env)
{
    LOG_DEBUG("[ServingSystemManager] handleLteCsCapabilityInd_ fired corrId=%s", env.corrId.c_str());
    auto pld = std::make_shared<StateIndPld>();
    pld->env = env;
    post_fifo({ LteCsCapabilityEvt_Signal, pld });
}

void
SimulaTelServingSystemManager::broadcastToListeners_(
  std::function<void(const std::shared_ptr<telux::tel::IServingSystemListener>&)> invoke
)
{
    auto task = std::make_shared<common::simula::DispatchTask>();
    task->debug_tag = "TelServingSystemManager::broadcastToListeners_";
    {
        std::lock_guard<std::mutex> lk(listeners_mutex_);
        for (auto& weak : listeners_)
        {
            if (auto sp = weak.lock())
                task->listeners.push_back(sp);
        }
    }
    if (task->listeners.empty())
        return;
    task->invoker = [invoke](std::shared_ptr<void> raw) {
        invoke(std::static_pointer_cast<telux::tel::IServingSystemListener>(raw));
    };
    common::simula::ListenerDispatchAO::instance().enqueue(std::move(task));
}

// ---------------------------------------------------------------------------
// Readiness

bool
SimulaTelServingSystemManager::isReadyDerived_() const
{
    return const_cast<SimulaTelServingSystemManager*>(this)->current_state() == TelServReady_St;
}

void
SimulaTelServingSystemManager::publishStatus_(telux::common::ServiceStatus s)
{
    last_status_.store(s);
}

bool
SimulaTelServingSystemManager::isSubsystemReady()
{
    return isReadyDerived_();
}

std::future<bool>
SimulaTelServingSystemManager::onSubsystemReady()
{
    std::promise<bool> p;
    p.set_value(isReadyDerived_());
    return p.get_future();
}

telux::common::ServiceStatus
SimulaTelServingSystemManager::getServiceStatus()
{
    return last_status_.load();
}

// ---------------------------------------------------------------------------
// telux::tel::IServingSystemManager

telux::common::Status
SimulaTelServingSystemManager::setRatPreference(
  telux::tel::RatPreference ratPref,
  telux::common::ResponseCallback callback
)
{
    LOG_DEBUG(
      "[ServingSystemManager] setRatPreference slot=%d ratPrefCount=%zu",
      slotId_,
      static_cast<size_t>(ratPref.count())
    );
    if (!isReadyDerived_())
        return telux::common::Status::NOTREADY;
    auto pld = std::make_shared<SetRatPreferencePld>();
    pld->ratPref = ratPref;
    pld->cb = std::move(callback);
    post_fifo({ SetRatPreference_Signal, pld });
    return telux::common::Status::SUCCESS;
}

telux::common::Status
SimulaTelServingSystemManager::requestRatPreference(telux::tel::RatPreferenceCallback callback)
{
    LOG_DEBUG("[ServingSystemManager] requestRatPreference slot=%d", slotId_);
    if (!isReadyDerived_())
        return telux::common::Status::NOTREADY;
    auto pld = std::make_shared<RequestRatPreferencePld>();
    pld->cb = std::move(callback);
    post_fifo({ RequestRatPreference_Signal, pld });
    return telux::common::Status::SUCCESS;
}

telux::common::Status
SimulaTelServingSystemManager::setServiceDomainPreference(
  telux::tel::ServiceDomainPreference,
  telux::common::ResponseCallback
)
{
    // No wire RPC for service-domain preference 
    return telux::common::Status::NOTSUPPORTED;
}

telux::common::Status
SimulaTelServingSystemManager::requestServiceDomainPreference(
  telux::tel::ServiceDomainPreferenceCallback
)
{
    return telux::common::Status::NOTSUPPORTED;
}

telux::common::Status
SimulaTelServingSystemManager::getSystemInfo(telux::tel::ServingSystemInfo& sysInfo)
{

    if (!isReadyDerived_())
    {
        sysInfo = telux::tel::ServingSystemInfo{};
        return telux::common::Status::NOTREADY;
    }
    std::lock_guard<std::mutex> lk(sys_info_mutex_);
    sysInfo = last_sys_info_;
    return telux::common::Status::SUCCESS;
}

telux::tel::DcStatus
SimulaTelServingSystemManager::getDcStatus()
{
    std::lock_guard<std::mutex> lk(sys_info_mutex_);
    return last_dc_status_;
}

telux::common::Status
SimulaTelServingSystemManager::requestNetworkTime(telux::tel::NetworkTimeResponseCallback)
{
    // Network time is out of scope for the requested radio API list.
    return telux::common::Status::NOTSUPPORTED;
}

telux::common::Status
SimulaTelServingSystemManager::requestLteSib16NetworkTime(telux::tel::NetworkTimeResponseCallback)
{
    return telux::common::Status::NOTSUPPORTED;
}

telux::common::Status
SimulaTelServingSystemManager::requestNr5gRrcUtcTime(telux::tel::NetworkTimeResponseCallback)
{
    return telux::common::Status::NOTSUPPORTED;
}

telux::common::Status
SimulaTelServingSystemManager::requestRFBandInfo(telux::tel::RFBandInfoCallback callback)
{
    LOG_DEBUG("[ServingSystemManager] requestRFBandInfo slot=%d", slotId_);
    if (!isReadyDerived_())
        return telux::common::Status::NOTREADY;
    auto pld = std::make_shared<RequestRFBandInfoPld>();
    pld->cb = std::move(callback);
    post_fifo({ RequestRFBandInfo_Signal, pld });
    return telux::common::Status::SUCCESS;
}

telux::common::Status
SimulaTelServingSystemManager::getNetworkRejectInfo(telux::tel::NetworkRejectInfo& rejectInfo)
{
    rejectInfo = telux::tel::NetworkRejectInfo{};
    return telux::common::Status::NOTSUPPORTED;
}

telux::common::Status
SimulaTelServingSystemManager::getCallBarringInfo(std::vector<telux::tel::CallBarringInfo>& barringInfo)
{
    barringInfo.clear();
    return telux::common::Status::NOTSUPPORTED;
}

telux::common::Status
SimulaTelServingSystemManager::getSmsCapabilityOverNetwork(telux::tel::SmsCapability& smsCapability)
{
    smsCapability = telux::tel::SmsCapability{};
    return telux::common::Status::NOTSUPPORTED;
}

telux::common::Status
SimulaTelServingSystemManager::getLteCsCapability(telux::tel::LteCsCapability& lteCapability)
{
    if (!isReadyDerived_())
    {
        lteCapability = telux::tel::LteCsCapability::UNKNOWN;
        return telux::common::Status::NOTREADY;
    }
    std::lock_guard<std::mutex> lk(sys_info_mutex_);
    lteCapability = last_lte_cs_capability_;
    return telux::common::Status::SUCCESS;
}

telux::common::Status
SimulaTelServingSystemManager::requestRFBandPreferences(telux::tel::RFBandPrefCallback)
{
    return telux::common::Status::NOTSUPPORTED;
}

telux::common::Status
SimulaTelServingSystemManager::setRFBandPreferences(
  std::shared_ptr<telux::tel::IRFBandList>,
  telux::common::ResponseCallback
)
{
    return telux::common::Status::NOTSUPPORTED;
}

telux::common::Status
SimulaTelServingSystemManager::requestRFBandCapability(telux::tel::RFBandCapabilityCallback)
{
    return telux::common::Status::NOTSUPPORTED;
}

telux::common::ErrorCode
SimulaTelServingSystemManager::setHplmnSearchTime(uint32_t)
{
    return telux::common::ErrorCode::NOT_SUPPORTED;
}

telux::common::ErrorCode
SimulaTelServingSystemManager::getHplmnSearchTime(uint32_t& time)
{
    time = 0;
    return telux::common::ErrorCode::NOT_SUPPORTED;
}

telux::common::Status
SimulaTelServingSystemManager::registerListener(
  std::weak_ptr<telux::tel::IServingSystemListener> listener,
  telux::tel::ServingSystemNotificationMask
)
{
    std::lock_guard<std::mutex> lk(listeners_mutex_);
    listeners_.push_back(std::move(listener));
    return telux::common::Status::SUCCESS;
}

telux::common::Status
SimulaTelServingSystemManager::deregisterListener(
  std::weak_ptr<telux::tel::IServingSystemListener> listener,
  telux::tel::ServingSystemNotificationMask
)
{
    std::lock_guard<std::mutex> lk(listeners_mutex_);
    auto target = listener.lock();
    listeners_.erase(
      std::remove_if(
        listeners_.begin(),
        listeners_.end(),
        [&](const std::weak_ptr<telux::tel::IServingSystemListener>& w) {
            auto sp = w.lock();
            return !sp || (target && sp == target);
        }
      ),
      listeners_.end()
    );
    return telux::common::Status::SUCCESS;
}

// ---------------------------------------------------------------------------
// State handlers

chart::Status
TelServNotReady_St(chart::Hsm* h, chart::Event const* e)
{
    auto* self = static_cast<SimulaTelServingSystemManager*>(h);
    switch (e->sig)
    {
        case chart::Entry_Signal:
            LOG_INFO("[ServingSystemManager] -> NotReady");
            return chart::Status::HANDLED;
        case chart::Exit_Signal:
            return chart::Status::HANDLED;
        case ReadinessEvt_Signal:
        {
            auto pld = event_cast<StateIndPld>(*e);
            std::string status =
              pld->env.data ? pld->env.data->value("status", std::string()) : std::string();
            LOG_DEBUG("[ServingSystemManager] ReadinessEvt_Signal (NotReady) status=%s", status.c_str());
            if (pld->env.data && pld->env.data->value("status", std::string()) == "AVAILABLE")
                return self->to(TelServReady_St);
            return chart::Status::HANDLED;
        }
        case BridgeConnectivityChanged_Signal:
        case SysInfoEvt_Signal:
        case DcStatusEvt_Signal:
        case RatPrefEvt_Signal:
        case LteCsCapabilityEvt_Signal:
            return chart::Status::HANDLED;
        // Arm the callback so the transition into Ready fires it. Every
        // caller's callback is queued -- see init_cbs_'s header comment --
        // so a repeat cache-hit caller from the factory never strands an
        // earlier one still waiting on its own callback.
        case SetInitCb_Signal:
        {
            auto pld = event_cast<SetInitCbPld>(*e);
            self->init_cbs_.push_back(pld->cb);
            return chart::Status::HANDLED;
        }

        case SetRatPreference_Signal:
        {
            auto pld = event_cast<SetRatPreferencePld>(*e);
            LOG_WARN(
              "[ServingSystemManager] SetRatPreference_Signal stale request failed with "
              "SUBSYSTEM_UNAVAILABLE -- NotReady before response"
            );
            if (pld->cb)
                pld->cb(telux::common::ErrorCode::SUBSYSTEM_UNAVAILABLE);
            return chart::Status::HANDLED;
        }
        case RequestRatPreference_Signal:
        {
            auto pld = event_cast<RequestRatPreferencePld>(*e);
            LOG_WARN(
              "[ServingSystemManager] RequestRatPreference_Signal stale request failed with "
              "SUBSYSTEM_UNAVAILABLE -- NotReady before response"
            );
            if (pld->cb)
            {
                telux::tel::RatPreference ratPref;
                pld->cb(ratPref, telux::common::ErrorCode::SUBSYSTEM_UNAVAILABLE);
            }
            return chart::Status::HANDLED;
        }
        case RequestRFBandInfo_Signal:
        {
            auto pld = event_cast<RequestRFBandInfoPld>(*e);
            LOG_WARN(
              "[ServingSystemManager] RequestRFBandInfo_Signal stale request failed with "
              "SUBSYSTEM_UNAVAILABLE -- NotReady before response"
            );
            if (pld->cb)
                pld->cb(
                  telux::tel::RFBandInfo{}, telux::common::ErrorCode::SUBSYSTEM_UNAVAILABLE
                );
            return chart::Status::HANDLED;
        }
        default:
            return self->super(&chart::Hsm::top);
    }
}

chart::Status
TelServReady_St(chart::Hsm* h, chart::Event const* e)
{
    auto* self = static_cast<SimulaTelServingSystemManager*>(h);
    switch (e->sig)
    {
        case chart::Entry_Signal:
        {
            LOG_INFO("[ServingSystemManager] -> Ready");
            self->publishStatus_(telux::common::ServiceStatus::SERVICE_AVAILABLE);
            if (!self->init_cbs_.empty())
            {
                std::vector<telux::common::InitResponseCb> cbs;
                cbs.swap(self->init_cbs_);
                for (auto& cb : cbs)
                    if (cb)
                        cb(telux::common::ServiceStatus::SERVICE_AVAILABLE);
            }
            else
            {
                self->broadcastToListeners_(
                  [](const std::shared_ptr<telux::tel::IServingSystemListener>& l) {
                      l->onServiceStatusChange(telux::common::ServiceStatus::SERVICE_AVAILABLE);
                  }
                );
            }
            return chart::Status::HANDLED;
        }
        case chart::Exit_Signal:
            self->broadcastToListeners_(
              [](const std::shared_ptr<telux::tel::IServingSystemListener>& l) {
                  l->onServiceStatusChange(telux::common::ServiceStatus::SERVICE_UNAVAILABLE);
              }
            );
            return chart::Status::HANDLED;

        case ReadinessEvt_Signal:
        {
            auto pld = event_cast<StateIndPld>(*e);
            if (!pld->env.data)
                return chart::Status::HANDLED;
            auto state = pld->env.data->value("status", std::string());
            LOG_DEBUG("[ServingSystemManager] ReadinessEvt_Signal (Ready) status=%s", state.c_str());
            if (state == "UNAVAILABLE")
            {
                self->publishStatus_(telux::common::ServiceStatus::SERVICE_UNAVAILABLE);
                return self->to(TelServNotReady_St);
            }
            return chart::Status::HANDLED;
        }

        case SetInitCb_Signal:
        {
            auto pld = event_cast<SetInitCbPld>(*e);
            if (pld->cb)
                pld->cb(telux::common::ServiceStatus::SERVICE_AVAILABLE);
            return chart::Status::HANDLED;
        }

        case BridgeConnectivityChanged_Signal:
        {
            auto pld = event_cast<bool>(*e);
            if (pld && !*pld)
            {
                self->publishStatus_(telux::common::ServiceStatus::SERVICE_UNAVAILABLE);
                return self->to(TelServNotReady_St);
            }
            return chart::Status::HANDLED;
        }

        case SysInfoEvt_Signal:
        {
            auto pld = event_cast<StateIndPld>(*e);
            if (!pld->env.data)
                return chart::Status::HANDLED;
            telux::tel::ServingSystemInfo sysInfo{};
            sysInfo.rat = wireToRadioTechnology(pld->env.data->value("rat", std::string()));
            sysInfo.domain = wireToServiceDomain(pld->env.data->value("domain", std::string()));
            sysInfo.state =
              wireToServiceRegistrationState(pld->env.data->value("state", std::string()));
            LOG_DEBUG(
              "[ServingSystemManager] SysInfoEvt_Signal rat=%d domain=%d state=%d",
              static_cast<int>(sysInfo.rat),
              static_cast<int>(sysInfo.domain),
              static_cast<int>(sysInfo.state)
            );
            {
                std::lock_guard<std::mutex> lk(self->sys_info_mutex_);
                self->last_sys_info_ = sysInfo;
            }
            self->broadcastToListeners_(
              [sysInfo](const std::shared_ptr<telux::tel::IServingSystemListener>& l) {
                  l->onSystemInfoChanged(sysInfo);
              }
            );
            return chart::Status::HANDLED;
        }

        case DcStatusEvt_Signal:
        {
            auto pld = event_cast<StateIndPld>(*e);
            if (!pld->env.data)
                return chart::Status::HANDLED;
            telux::tel::DcStatus dcStatus{};
            dcStatus.endcAvailability =
              wireToEndcAvailability(pld->env.data->value("endcAvailability", std::string()));
            dcStatus.dcnrRestriction =
              wireToDcnrRestriction(pld->env.data->value("dcnrRestriction", std::string()));
            LOG_DEBUG(
              "[ServingSystemManager] DcStatusEvt_Signal endcAvailability=%d dcnrRestriction=%d",
              static_cast<int>(dcStatus.endcAvailability),
              static_cast<int>(dcStatus.dcnrRestriction)
            );
            {
                std::lock_guard<std::mutex> lk(self->sys_info_mutex_);
                self->last_dc_status_ = dcStatus;
            }
            self->broadcastToListeners_(
              [dcStatus](const std::shared_ptr<telux::tel::IServingSystemListener>& l) {
                  l->onDcStatusChanged(dcStatus);
              }
            );
            return chart::Status::HANDLED;
        }

        case LteCsCapabilityEvt_Signal:
        {
            auto pld = event_cast<StateIndPld>(*e);
            if (!pld->env.data)
                return chart::Status::HANDLED;
            auto lteCapability =
              wireToLteCsCapability(pld->env.data->value("lteCsCapability", std::string()));
            LOG_DEBUG(
              "[ServingSystemManager] LteCsCapabilityEvt_Signal lteCsCapability=%d",
              static_cast<int>(lteCapability)
            );
            {
                std::lock_guard<std::mutex> lk(self->sys_info_mutex_);
                self->last_lte_cs_capability_ = lteCapability;
            }
            self->broadcastToListeners_(
              [lteCapability](const std::shared_ptr<telux::tel::IServingSystemListener>& l) {
                  l->onLteCsCapabilityChanged(lteCapability);
              }
            );
            return chart::Status::HANDLED;
        }

        case RatPrefEvt_Signal:
        {
            auto pld = event_cast<StateIndPld>(*e);
            if (!pld->env.data)
                return chart::Status::HANDLED;
            telux::tel::RatPreference ratPref;
            for (const auto& r : pld->env.data->value("ratPrefs", nlohmann::json::array()))
                ratPref.set(static_cast<size_t>(wireToRatPref(r.get<std::string>())));
            LOG_DEBUG(
              "[ServingSystemManager] RatPrefEvt_Signal ratPrefCount=%zu",
              static_cast<size_t>(ratPref.count())
            );
            self->broadcastToListeners_(
              [ratPref](const std::shared_ptr<telux::tel::IServingSystemListener>& l) {
                  l->onRatPreferenceChanged(ratPref);
              }
            );
            return chart::Status::HANDLED;
        }

        case SetRatPreference_Signal:
        {
            auto pld = event_cast<SetRatPreferencePld>(*e);
            nlohmann::json ratPrefs = nlohmann::json::array();
            for (size_t i = 0; i < pld->ratPref.size(); ++i)
            {
                if (pld->ratPref.test(i))
                    ratPrefs.push_back(ratPrefToWire(static_cast<telux::tel::RatPrefType>(i)));
            }
            nlohmann::json data = nlohmann::json::object();
            data["ratPrefs"] = std::move(ratPrefs);
            data["slot"] = self->slotId_;
            auto req = common::simula::makeRequestEnvelope(self->bridge_.currentPaId(), std::move(data));
            auto cb = pld->cb;
            LOG_DEBUG(
              "[ServingSystemManager] SetRatPreference_Signal send_request slot=%d corrId=%s",
              self->slotId_,
              req.corrId.c_str()
            );
            self->bridge_.send_request(
              topics::radio::set_rat_preference::req,
              "radio.set_rat_preference.rsp",
              req,
              [cb, corrId = req.corrId](std::optional<Envelope> rsp) {
                  if (!cb)
                      return;
                  if (!rsp || rsp->error)
                  {
                      LOG_WARN(
                        "[ServingSystemManager] SetRatPreference_Signal response timeout/error corrId=%s",
                        corrId.c_str()
                      );
                      cb(telux::common::ErrorCode::OPERATION_TIMEOUT);
                      return;
                  }
                  LOG_DEBUG(
                    "[ServingSystemManager] SetRatPreference_Signal response success corrId=%s",
                    corrId.c_str()
                  );
                  cb(telux::common::ErrorCode::SUCCESS);
              },
              kRpcTimeout
            );
            return chart::Status::HANDLED;
        }

        case RequestRatPreference_Signal:
        {
            auto pld = event_cast<RequestRatPreferencePld>(*e);
            nlohmann::json data = nlohmann::json::object();
            data["slot"] = self->slotId_;
            auto req = common::simula::makeRequestEnvelope(self->bridge_.currentPaId(), std::move(data));
            auto cb = pld->cb;
            LOG_DEBUG(
              "[ServingSystemManager] RequestRatPreference_Signal send_request slot=%d corrId=%s",
              self->slotId_,
              req.corrId.c_str()
            );
            self->bridge_.send_request(
              topics::radio::request_rat_preference::req,
              "radio.request_rat_preference.rsp",
              req,
              [cb, corrId = req.corrId](std::optional<Envelope> rsp) {
                  if (!cb)
                      return;
                  telux::tel::RatPreference ratPref;
                  if (!rsp || rsp->error || !rsp->data)
                  {
                      LOG_WARN(
                        "[ServingSystemManager] RequestRatPreference_Signal response timeout/error corrId=%s",
                        corrId.c_str()
                      );
                      cb(ratPref, telux::common::ErrorCode::OPERATION_TIMEOUT);
                      return;
                  }
                  for (const auto& r : rsp->data->value("ratPrefs", nlohmann::json::array()))
                      ratPref.set(static_cast<size_t>(wireToRatPref(r.get<std::string>())));
                  LOG_DEBUG(
                    "[ServingSystemManager] RequestRatPreference_Signal response success corrId=%s ratPrefCount=%zu",
                    corrId.c_str(),
                    static_cast<size_t>(ratPref.count())
                  );
                  cb(ratPref, telux::common::ErrorCode::SUCCESS);
              },
              kRpcTimeout
            );
            return chart::Status::HANDLED;
        }

        case RequestRFBandInfo_Signal:
        {
            auto pld = event_cast<RequestRFBandInfoPld>(*e);
            nlohmann::json data = nlohmann::json::object();
            data["slot"] = self->slotId_;
            auto req = common::simula::makeRequestEnvelope(self->bridge_.currentPaId(), std::move(data));
            auto cb = pld->cb;
            LOG_DEBUG(
              "[ServingSystemManager] RequestRFBandInfo_Signal send_request slot=%d corrId=%s",
              self->slotId_,
              req.corrId.c_str()
            );
            self->bridge_.send_request(
              topics::radio::request_rf_band_info::req,
              "radio.request_rf_band_info.rsp",
              req,
              [cb, corrId = req.corrId](std::optional<Envelope> rsp) {
                  if (!cb)
                      return;
                  if (!rsp || rsp->error || !rsp->data)
                  {
                      LOG_WARN(
                        "[ServingSystemManager] RequestRFBandInfo_Signal response timeout/error corrId=%s",
                        corrId.c_str()
                      );
                      cb(telux::tel::RFBandInfo{}, telux::common::ErrorCode::OPERATION_TIMEOUT);
                      return;
                  }
                  telux::tel::RFBandInfo info{};
                  info.band = wireToRFBand(rsp->data->value("band", std::string()));
                  info.channel = rsp->data->value("channel", 0u);
                  info.bandWidth = wireToRFBandWidth(rsp->data->value("bandWidth", std::string()));
                  LOG_DEBUG(
                    "[ServingSystemManager] RequestRFBandInfo_Signal response success corrId=%s band=%d bandWidth=%d",
                    corrId.c_str(),
                    static_cast<int>(info.band),
                    static_cast<int>(info.bandWidth)
                  );
                  cb(info, telux::common::ErrorCode::SUCCESS);
              },
              kRpcTimeout
            );
            return chart::Status::HANDLED;
        }

        default:
            return self->super(&chart::Hsm::top);
    }
}

CHART_NAMED_STATE(TelServNotReady_St, "TelServingSystemManager::NotReady");
CHART_NAMED_STATE(TelServReady_St,    "TelServingSystemManager::Ready");

}  // namespace telux::tel::simula
