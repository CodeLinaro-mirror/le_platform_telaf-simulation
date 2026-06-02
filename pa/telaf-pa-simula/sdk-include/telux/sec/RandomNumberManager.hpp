/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file RandomNumberManager.hpp
 *
 * @brief Defines data types used by TelSDK security framework and applications.
 */

#ifndef TELUX_SEC_RANDOMNUMBERMANAGER_HPP
#define TELUX_SEC_RANDOMNUMBERMANAGER_HPP

#include <vector>
#include <memory>

#include "telux/common/CommonDefines.hpp"

namespace telux {
namespace sec {

/** @addtogroup telematics_sec_mgmt
 * @{ */

/**
 * Specifies source of the random number generator.
 */
enum class RNGSource {

    /**
     * True random number generator (TRNG) on Qualcomm Technologies Inc.
     * (QTI) platform. This is FIPS compliant.
     */
    QTI_HW_TRNG = 1,

    /**
     * Linux /dev/random device is used as the random number provider.
     * It is based on ChaCha20 stream cipher and uses events from timer,
     * platform, bootloader, hardware random number generator, interrupts,
     * input and disk devices for entropy purpose.
     */
    DEV_RANDOM = 2,
};

/**
 * @brief IRandomNumberManager can be used to generate random number/data.
 */
class IRandomNumberManager {

 public:
   /**
    * Gets a 32 bit random number.
    *
    * @param [out] generatedNumber random number generated
    *
    * @returns @ref telux::common::ErrorCode::SUCCESS if the random number is
    *          generated successfully otherwise an appropriate error code.
    *
    * @note Eval: This is a new API and is being evaluated. It is subject
                  to change and could break backwards compatibility.
    */
   virtual telux::common::ErrorCode getRandomNumber(uint32_t &generatedNumber) = 0;

   /**
    * Gets a 64 bit random number.
    *
    * @param [out] generatedNumber random number generated
    *
    * @returns @ref telux::common::ErrorCode::SUCCESS if the random number is
    *          generated successfully otherwise an appropriate error code.
    *
    * @note Eval: This is a new API and is being evaluated. It is subject
                  to change and could break backwards compatibility.
    */
   virtual telux::common::ErrorCode getRandomNumber(uint64_t &generatedNumber) = 0;

   /**
    * Gets random data bytes up to the length defined by generatedData.size().
    * The dataLength gives how many bytes are actually generated.
    *
    * @param [out] generatedData will contain random data
    *
    * @param[out] dataLength number of bytes generated
    *
    * @returns @ref telux::common::ErrorCode::SUCCESS if the random data is
    *          generated successfully otherwise an appropriate error code.
    *
    * @note Eval: This is a new API and is being evaluated. It is subject
                  to change and could break backwards compatibility.
    */
   virtual telux::common::ErrorCode getRandomData(
        std::vector<uint8_t> &generatedData, size_t &dataLength) = 0;

   /**
    * Destroys the IRandomNumberManager instance. Performs cleanup as applicable.
    */
   virtual ~IRandomNumberManager() {};
};

/** @} */ /* end_addtogroup telematics_sec_mgmt */

}  // end of namespace sec
}  // end of namespace telux

#endif // TELUX_SEC_RANDOMNUMBERMANAGER_HPP
