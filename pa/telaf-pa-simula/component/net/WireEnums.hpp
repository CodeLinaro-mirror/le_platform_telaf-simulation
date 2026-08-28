// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear
//
// WireEnums.hpp - shared wire-string <-> telux-enum mapping for the net
// business domain.
//
// Extracted so the five net managers cannot disagree about how a given enum
// is spelled on the wire: BackhaulType appears in net_vlan, net_l2tp, net_nat
// and net_settings payloads, and a divergence between two of them would be an
// interop bug MPSS-side schema validation could not catch (both spellings are
// valid enum members, just of different values).
//
// Every to-wire mapping is exhaustive over the enum; every from-wire mapping
// falls back to the enum's own UNKNOWN member (or its documented default)
// rather than guessing a real value.

#ifndef TELUX_DATA_NET_SIMULA_WIRE_ENUMS_HPP
#define TELUX_DATA_NET_SIMULA_WIRE_ENUMS_HPP

#include <nlohmann/json.hpp>
#include <string>
#include <telux/data/DataDefines.hpp>

namespace telux::data::net::simula::wire {

// --- InterfaceType ---------------------------------------------------------

inline std::string
fromIfaceType(telux::data::InterfaceType t)
{
    using IT = telux::data::InterfaceType;
    switch (t)
    {
        case IT::WLAN:          return "WLAN";
        case IT::ETH:           return "ETH";
        case IT::ECM:           return "ECM";
        case IT::RNDIS:         return "RNDIS";
        case IT::MHI:           return "MHI";
        case IT::VMTAP0:        return "VMTAP0";
        case IT::VMTAP1:        return "VMTAP1";
        case IT::ETH2:          return "ETH2";
        case IT::AP_PRIMARY:    return "AP_PRIMARY";
        case IT::AP_SECONDARY:  return "AP_SECONDARY";
        case IT::AP_TERTIARY:   return "AP_TERTIARY";
        case IT::AP_QUATERNARY: return "AP_QUATERNARY";
        default:                return "UNKNOWN";
    }
}

inline telux::data::InterfaceType
toIfaceType(const std::string& s)
{
    using IT = telux::data::InterfaceType;
    if (s == "WLAN")          return IT::WLAN;
    if (s == "ETH")           return IT::ETH;
    if (s == "ECM")           return IT::ECM;
    if (s == "RNDIS")         return IT::RNDIS;
    if (s == "MHI")           return IT::MHI;
    if (s == "VMTAP0")        return IT::VMTAP0;
    if (s == "VMTAP1")        return IT::VMTAP1;
    if (s == "ETH2")          return IT::ETH2;
    if (s == "AP_PRIMARY")    return IT::AP_PRIMARY;
    if (s == "AP_SECONDARY")  return IT::AP_SECONDARY;
    if (s == "AP_TERTIARY")   return IT::AP_TERTIARY;
    if (s == "AP_QUATERNARY") return IT::AP_QUATERNARY;
    return IT::UNKNOWN;
}

// --- BackhaulType ----------------------------------------------------------

// Returns "" for MAX_SUPPORTED: that member is a count sentinel, not a real
// backhaul, and the schemas have no string for it. An empty result is omitted
// from the payload so MPSS-side validation rejects the request rather than the
// PA silently substituting a different backhaul.
inline std::string
fromBackhaul(telux::data::BackhaulType t)
{
    using BT = telux::data::BackhaulType;
    switch (t)
    {
        case BT::ETH:  return "ETH";
        case BT::USB:  return "USB";
        case BT::WLAN: return "WLAN";
        case BT::WWAN: return "WWAN";
        case BT::BLE:  return "BLE";
        default:       return "";
    }
}

inline telux::data::BackhaulType
toBackhaul(const std::string& s)
{
    using BT = telux::data::BackhaulType;
    if (s == "USB")  return BT::USB;
    if (s == "WLAN") return BT::WLAN;
    if (s == "WWAN") return BT::WWAN;
    if (s == "BLE")  return BT::BLE;
    return BT::ETH;
}

// --- IpFamilyType ----------------------------------------------------------

inline std::string
fromIpFamily(telux::data::IpFamilyType t)
{
    using FT = telux::data::IpFamilyType;
    switch (t)
    {
        case FT::IPV4:   return "IPV4";
        case FT::IPV6:   return "IPV6";
        case FT::IPV4V6: return "IPV4V6";
        default:         return "UNKNOWN";
    }
}

inline telux::data::IpFamilyType
toIpFamily(const std::string& s)
{
    using FT = telux::data::IpFamilyType;
    if (s == "IPV4")   return FT::IPV4;
    if (s == "IPV6")   return FT::IPV6;
    if (s == "IPV4V6") return FT::IPV4V6;
    return FT::UNKNOWN;
}

// --- BackhaulInfo ----------------------------------------------------------

// slotId/profileId are emitted only for WWAN: the SDK documents them as
// don't-care for every other backhaul, and MPSS discards them there, so
// sending them would imply a meaning they do not have.
inline nlohmann::json
fromBackhaulInfo(const telux::data::BackhaulInfo& bh)
{
    nlohmann::json j = nlohmann::json::object();
    auto name = fromBackhaul(bh.backhaul);
    if (!name.empty())
        j["backhaul"] = name;
    if (bh.backhaul == telux::data::BackhaulType::WWAN)
    {
        j["slot"] = static_cast<int>(bh.slotId);
        j["profileId"] = bh.profileId;
    }
    // -1 is BackhaulInfo::vlanId's documented "unset" default.
    if (bh.vlanId >= 0)
        j["bhVlanId"] = bh.vlanId;
    return j;
}

inline telux::data::BackhaulInfo
toBackhaulInfo(const nlohmann::json& j)
{
    telux::data::BackhaulInfo bh{};
    bh.backhaul = toBackhaul(j.value("backhaul", std::string()));
    if (j.contains("slot"))
        bh.slotId = static_cast<::SlotId>(j.value("slot", static_cast<int>(DEFAULT_SLOT_ID)));
    if (j.contains("profileId"))
        bh.profileId = j.value("profileId", -1);
    if (j.contains("bhVlanId"))
        bh.vlanId = j.value("bhVlanId", -1);
    return bh;
}

}  // namespace telux::data::net::simula::wire

#endif  // TELUX_DATA_NET_SIMULA_WIRE_ENUMS_HPP
