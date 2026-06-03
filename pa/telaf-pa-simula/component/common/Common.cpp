// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include <string>
#include <telux/common/DeviceConfig.hpp>
#include <telux/common/Utils.hpp>

namespace telux {
namespace common {

bool
DeviceConfig::isMultiSimSupported()
{
    return false;
}

std::string
Utils::getErrorCodeAsString(ErrorCode error)
{
    return std::to_string(static_cast<int>(error));
}

std::string
Utils::getErrorCodeAsString(int error)
{
    return std::to_string(error);
}

}  // namespace common
}  // namespace telux
