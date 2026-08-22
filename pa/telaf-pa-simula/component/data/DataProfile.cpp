// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear
//
// Minimal DataProfile implementation for the simulation libtelux_data.so.
// The real SDK provides this; here we supply enough for the simula PA to
// construct and return DataProfile objects to tafDataCallSvc.

#include <sstream>
#include <telux/data/DataProfile.hpp>

namespace telux {
namespace data {

DataProfile::DataProfile(
  int id,
  const std::string& name,
  const std::string& apn,
  const std::string& username,
  const std::string& password,
  IpFamilyType ipFamilyType,
  TechPreference techPref,
  AuthProtocolType authType,
  ApnTypes apnTypes,
  EmergencyCapability emergencyAllowed,
  bool clatEnabled
)
    : id_(id)
    , name_(name)
    , apn_(apn)
    , username_(username)
    , password_(password)
    , ipFamilyType_(ipFamilyType)
    , techPref_(techPref)
    , authType_(authType)
    , apnTypes_(apnTypes)
    , emergencyAllowed_(emergencyAllowed)
    , clatEnabled_(clatEnabled)
{}

int
DataProfile::getId()
{
    return id_;
}
std::string
DataProfile::getName()
{
    return name_;
}
std::string
DataProfile::getApn()
{
    return apn_;
}
std::string
DataProfile::getUserName()
{
    return username_;
}
std::string
DataProfile::getPassword()
{
    return password_;
}
TechPreference
DataProfile::getTechPreference()
{
    return techPref_;
}
AuthProtocolType
DataProfile::getAuthProtocolType()
{
    return authType_;
}
IpFamilyType
DataProfile::getIpFamilyType()
{
    return ipFamilyType_;
}
ApnTypes
DataProfile::getApnTypes()
{
    return apnTypes_;
}
EmergencyCapability
DataProfile::getIsEmergencyAllowed()
{
    return emergencyAllowed_;
}
bool
DataProfile::isClatEnabled()
{
    return clatEnabled_;
}

std::string
DataProfile::toString()
{
    std::ostringstream oss;
    oss << "DataProfile{id=" << id_ << ",apn=" << apn_ << "}";
    return oss.str();
}

}  // namespace data
}  // namespace telux
