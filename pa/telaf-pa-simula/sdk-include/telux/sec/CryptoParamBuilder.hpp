/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file CryptoParamBuilder.hpp
 *
 * @brief Helps setup input parameters for a given crypto operation.
 */

#ifndef TELUX_SEC_CRYPTOPARAMBUILDER_HPP
#define TELUX_SEC_CRYPTOPARAMBUILDER_HPP

#include <memory>
#include <vector>

#include <telux/sec/CryptoDefines.hpp>

namespace telux {
namespace sec {

/** @addtogroup telematics_sec_mgmt
 * @{ */

/**
 * CryptoParamBuilder helps setup input parameters for a given crypto operation.
 */
class CryptoParamBuilder {

  public:
    /** Allocates an instance of CryptoParamBuilder. */
    CryptoParamBuilder();

    /** When generating keys, specifies with which algorithm the keys will be used.
        For crypto operations, specifies the algorithm to use. Use @ref telux::sec::Algorithm
        enumeration to define this. */
    CryptoParamBuilder setAlgorithm(AlgorithmTypes algorithm);

    /** When generating keys, specifies the crypto operation(s) for which the key will be
        used. For crypto operations, specifies the operation itself (encrypting/decrypting/
        signing/verifying). Use @ref telux::sec::CryptoOperation enumeration to define
        this. Multiple operation values can be OR'ed (|). */
    CryptoParamBuilder setCryptoOperation(CryptoOperationTypes operation);

    /** When generating keys, specifies the digest algorithm(s) that may be used with
        the key to perform signing and verifying operations using RSA, ECDSA, and HMAC
        keys. For crypto operations, specifies exact digest algorithm to be used. Use
        @ref telux::sec::Digest enumeration to define this. Multiple values can be OR'ed (|). */
    CryptoParamBuilder setDigest(DigestTypes digest);

    /** When generating keys, specifies the padding modes that may be used with the RSA
        and AES key. For crypto operations, specifies the exact padding to be used. Use
        @ref telux::sec::Padding enumeration to define this. Multiple padding values can be
        OR'ed (|). */
    CryptoParamBuilder setPadding(PaddingTypes padding);

    /** When generating keys, specifies the size in bits, of the key, measured in the
        regular way for the key's algorithm.
        - For RSA keys, specifies the size of the public modulus.
        - For AES keys, specifies length of the secret key material.
        - For HMAC keys, specifies the key size in bits.
        - For EC keys, selects the EC group. */
    CryptoParamBuilder setKeySize(int32_t keySize);

    /** When generating keys, specifies minimum length of the MAC in bits that can be
        requested or verified with this key for HMAC keys and AES keys that support GCM
        mode.*/
    CryptoParamBuilder setMinimumMacLength(int32_t minMacLength);

    /** For crypto operations, specifies requested length of a MAC or GCM (which is guaranteed
        to be no less then minimum length of the MAC/GCM used when generating the key). */
    CryptoParamBuilder setMacLength(int32_t macLength);

    /** When generating keys, specifies the block cipher mode(s) with which this key can
        be used. For crypto operations, specifies the exact block mode to be used. Use
        @ref telux::sec::BlockMode enumeration to define this. Multiple block mode
        values can be OR'ed (|). */
    CryptoParamBuilder setBlockMode(BlockModeTypes blockMode);

    /** When generating the keys using an EC algorithm, only key size, only curve, or both key
        size and curve can be specified. If only key size is specified, the appropriate NIST
        curve is selected automatically. If only curve is specified, the given curve is used.
        If both are specified, the given curve is used and key size is validated. */
    CryptoParamBuilder setCurve(int32_t curve);

    /** When generating AES key, if callerNonce is set to true, it specifies that an
        explicit nonce will be supplied by the caller during encryption and decryption using
        @ref setInitVector(). If the callerNonce is set to false (or not set), platform will
        generate the nonce during encryption. This nonce should be passed during decryption. */
    CryptoParamBuilder setCallerNonce(bool callerNonce);

    /** When generating an RSA key, specifies the value of the public exponent for an
        RSA key pair (necessary for all RSA keys). */
    CryptoParamBuilder setPublicExponent(uint64_t publicExponent);

    /** When performing AES crypto operations, specifies the initialization vector to be used. */
    CryptoParamBuilder setInitVector(std::vector<uint8_t> initVector);

    /** When generating or importing a key, an optional arbitrary value can be supplied through
        this method. In all subsequent use of the key, this value must be supplied again. The
        data given is bound to the key cryptographically. This data ties the key to the
        caller. */
    CryptoParamBuilder setUniqueData(std::vector<uint8_t> uniqueData);

    /** When encrypting/decrypting data, this specifies optional associated data to be used.
        This is applicable only for AES-GCM algorithm. */
    CryptoParamBuilder setAssociatedData(std::vector<uint8_t> associatedData);

    /** Creates an instance of ICryptoParam based on the setter methods invoked
     * on the builder. After building the builder's state is reset. */
    std::shared_ptr<ICryptoParam> build(void);

  private:
    std::shared_ptr<ICryptoParam> cryptoParam_;
};

/** @} */ /* end_addtogroup telematics_sec_mgmt */

}  // End of namespace sec
}  // End of namespace telux

#endif // TELUX_SEC_CRYPTOPARAMBUILDER_HPP
