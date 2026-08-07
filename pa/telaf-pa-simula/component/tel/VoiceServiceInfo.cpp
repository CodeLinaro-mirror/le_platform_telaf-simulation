// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear
//
// VoiceServiceInfo.cpp -- out-of-line definitions for telux::tel::VoiceServiceInfo.

#include <telux/tel/VoiceServiceInfo.hpp>

namespace telux {
namespace tel {

VoiceServiceInfo::VoiceServiceInfo(
  VoiceServiceState voiceServiceState,
  VoiceServiceDenialCause denialCause,
  RadioTechnology radioTech
)
    : voiceServiceState_(voiceServiceState)
    , denialCause_(denialCause)
    , radioTech_(radioTech)
{
}

VoiceServiceState
VoiceServiceInfo::getVoiceServiceState()
{
    return voiceServiceState_;
}

VoiceServiceDenialCause
VoiceServiceInfo::getVoiceServiceDenialCause()
{
    return denialCause_;
}

bool
VoiceServiceInfo::isEmergency()
{
    return (
      (voiceServiceState_ == VoiceServiceState::NOT_REG_AND_EMERGENCY_AVAILABLE_AND_NOT_SEARCHING)
      || (voiceServiceState_ == VoiceServiceState::NOT_REG_AND_EMERGENCY_AVAILABLE_AND_SEARCHING)
      || (voiceServiceState_ == VoiceServiceState::REG_DENIED_AND_EMERGENCY_AVAILABLE)
      || (voiceServiceState_ == VoiceServiceState::UNKNOWN_AND_EMERGENCY_AVAILABLE)
    );
}

bool
VoiceServiceInfo::isInService()
{
    return (
      voiceServiceState_ == VoiceServiceState::REG_HOME
      || voiceServiceState_ == VoiceServiceState::REG_ROAMING
    );
}

bool
VoiceServiceInfo::isOutOfService()
{
    return (
      voiceServiceState_ == VoiceServiceState::UNKNOWN
      || voiceServiceState_ == VoiceServiceState::NOT_REG_AND_NOT_SEARCHING
      || voiceServiceState_ == VoiceServiceState::NOT_REG_AND_SEARCHING
      || voiceServiceState_ == VoiceServiceState::REG_DENIED
    );
}

RadioTechnology
VoiceServiceInfo::getRadioTechnology()
{
    return radioTech_;
}

}  // namespace tel
}  // namespace telux
