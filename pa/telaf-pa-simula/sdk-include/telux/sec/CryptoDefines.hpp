/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file CryptoDefines.hpp
 *
 * @brief Defines data types used by TelSDK security framework and applications.
 */

#ifndef TELUX_SEC_CRYPTODEFINES_HPP
#define TELUX_SEC_CRYPTODEFINES_HPP

#include <vector>
#include <cstdint>
#include <memory>

namespace telux {
namespace sec {

/** @addtogroup telematics_sec_mgmt
 * @{ */

/**
 * Specifies the operation for which the key can be used.
 * A key can be used for multiple operation types.
 */
enum CryptoOperation {
    CRYPTO_OP_ENCRYPT = (1 << 1),  /**< Key will be used for encryption. */
    CRYPTO_OP_DECRYPT = (1 << 2),  /**< Key will be used for decryption. */
    CRYPTO_OP_SIGN    = (1 << 3),  /**< Key will be used for signing. */
    CRYPTO_OP_VERIFY  = (1 << 4)   /**< Key will be used for verification. */
};

/**
 * List of operation types consisting of entries from @ref CryptoOperation.
 * Multiple values can be OR'ed together, for example, (CRYPTO_OP_ENCRYPT | CRYPTO_OP_DECRYPT).
 */
using CryptoOperationTypes = int32_t;

/**
 * Specifies the block cipher mode(s) with which the AES key may be used.
 */
enum BlockMode {
    BLOCK_MODE_ECB = (1 << 1),  /**< Electronic code block mode */
    BLOCK_MODE_CBC = (1 << 2),  /**< Cipher block chain mode */
    BLOCK_MODE_CTR = (1 << 3),  /**< Counter-based mode */
    BLOCK_MODE_GCM = (1 << 4)   /**< Galois/counter mode */
};

/**
 * List of block mode types consisting of entries from @ref BlockMode.
 * Multiple values can be OR'ed together, for example, (BLOCK_MODE_ECB | BLOCK_MODE_CBC).
 */
using BlockModeTypes = int32_t;

/**
 * Padding modes that may be applied to plain text for encryption operations.
 * Only cryptographically-appropriate pairs are specified here.
 */
enum Padding {
    PADDING_NONE               = (1 << 1),  /**< No padding. */
    PADDING_RSA_OAEP           = (1 << 2),  /**< RSA optimal asymmetric encryption padding. */
    PADDING_RSA_PSS            = (1 << 3),  /**< RSA probabilistic signature scheme. */
    PADDING_RSA_PKCS1_1_5_ENC  = (1 << 4),  /**< RSA PKCS#1 v1.5 padding for encryption. */
    PADDING_RSA_PKCS1_1_5_SIGN = (1 << 5),  /**< RSA PKCS#1 v1.5 padding for signing. */
    PADDING_PKCS7              = (1 << 6)   /**< Public-key cryptography standard. */
};

/**
 * List of padding types to use consisting of entries from @ref Padding.
 * Multiple values can be OR'ed together, for example, (PADDING_PKCS7 | PADDING_RSA_PSS).
 */
using PaddingTypes = int32_t;

/**
 * Specifies the digest algorithms that may be used with the key to perform signing
 * and verification operations using RSA, ECDSA, and HMAC keys. The digest used during
 * signing or verification must match the digest associated with the key when the key
 * was generated.
 */
enum Digest {
    DIGEST_NONE      = (1 << 1),  /**< No digest. */
    DIGEST_MD5       = (1 << 2),  /**< Message-digest algorithm. */
    DIGEST_SHA1      = (1 << 3),  /**< Secure hash algorithm 1 */
    DIGEST_SHA_2_224 = (1 << 4),  /**< Secure hash algorithm 2, digest 224. */
    DIGEST_SHA_2_256 = (1 << 5),  /**< Secure hash algorithm 2, digest 256. */
    DIGEST_SHA_2_384 = (1 << 6),  /**< Secure hash algorithm 2, digest 384. */
    DIGEST_SHA_2_512 = (1 << 7)   /**< Secure hash algorithm 2, digest 512. */
};

/**
 * List of digest types to use consisting of entries from @ref Digest.
 * Multiple values can be OR'ed together, for example, (DIGEST_SHA_2_256 | DIGEST_SHA_2_512).
 */
using DigestTypes = int32_t;

/**
 * Algorithm for signing, verification, encryption, and decryption operations.
 */
enum Algorithm {
    ALGORITHM_UNKNOWN,  /**< Unspecified algorithm. */
    ALGORITHM_RSA,      /**< RSA (Rivest–Shamir–Adleman) algorithm. */
    ALGORITHM_EC,       /**< Elliptic-curve algorithm. */
    ALGORITHM_AES,      /**< Advanced encryption standard algorithm. */
    ALGORITHM_HMAC      /**< Hash-based message authentication code algorithm. */
};

/**
 * Specifies the algorithm to use; valid values are listed in @ref Algorithm.
 */
using AlgorithmTypes = int32_t;

/**
 * NIST curves used with ECDSA.
 */
enum Curve {
    CURVE_P_224, /**< NIST curve P-224. */
    CURVE_P_256, /**< NIST curve P-256. */
    CURVE_P_384, /**< NIST curve P-384. */
    CURVE_P_521  /**< NIST curve P-521. */
};

/**
 * Specifies the curve to use; valid values are listed in @ref Curve.
 */
using CurveTypes = int32_t;

/**
 * Formats for key import and export.
 */
enum KeyFormat {
    KEY_FORMAT_X509,    /**< Public key export. */
    KEY_FORMAT_PKCS8,   /**< Asymmetric key pair import. */
    KEY_FORMAT_RAW      /**< Symmetric key import and export. */
};

/**
 * Specifies how a crypto operation should be performed. An instance
 * of this must be created only thorough @ref CryptoParamBuilder.
 */
class ICryptoParam {
  public:
    virtual ~ ICryptoParam() {};
};

/**
 * Represents encrypted data and optional nonce.
 */
struct EncryptedData {
    std::vector<uint8_t> encryptedText; /**< Encrypted text. */
    std::vector<uint8_t> nonce; /**< Generated nonce. */
};

/** @} */ /* end_addtogroup telematics_sec_mgmt */

}  // end of namespace sec
}  // end of namespace telux

#endif // TELUX_SEC_CRYPTODEFINES_HPP
