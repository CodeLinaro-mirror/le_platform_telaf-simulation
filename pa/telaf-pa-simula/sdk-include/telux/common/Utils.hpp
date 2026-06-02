/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file  Utils.hpp
 * @brief Provides utility functions to perform error conversions.
 */

#ifndef TELUX_COMMON_UTILS_HPP
#define TELUX_COMMON_UTILS_HPP

#include <string>

#include <telux/common/CommonDefines.hpp>

namespace telux {
namespace common {

class Utils {
public:
    /**
     * Converts @ref telux::common::ErrorCode to string.
     *
     * @param [in] error @ref telux::common::ErrorCode
     *
     * @returns string description of the @ref telux::common::ErrorCode.
     */
    static std::string getErrorCodeAsString(ErrorCode error);

    /**
     * Converts error to string.
     *
     * @param [in] integer value of the @ref telux::common::ErrorCode.
     *
     * @returns string description of the @ref telux::common::ErrorCode.
     */
    static std::string getErrorCodeAsString(int error);

};

}  // End of namespace common
}  // End of namespace telux

#endif // TELUX_COMMON_UTILS_HPP
