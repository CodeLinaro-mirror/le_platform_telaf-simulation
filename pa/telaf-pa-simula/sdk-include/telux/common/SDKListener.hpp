/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */


/*
 * @file       SDKListener.hpp
 * @brief This is an empty base class for all Listener classes.
 * It allows child classes to inherit and implement the required notification methods.
 */

#ifndef TELUX_COMMON_SDKLISTENER_HPP
#define TELUX_COMMON_SDKLISTENER_HPP


namespace telux {
namespace common {

class ISDKListener {
 public:
    virtual ~ISDKListener() {
    }
};

}
}

#endif //TELUX_COMMON_SDKLISTENER_HPP