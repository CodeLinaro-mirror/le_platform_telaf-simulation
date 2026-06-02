/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file  CryptoManager.hpp
 *
 * @brief The CryptoManager class is used to manage security keys and perform
 *        certain cryptographic operations such as signing, verification,
 *        encryption, and decryption.
 */

#ifndef TELUX_SEC_CRYPTOMANAGER_HPP
#define TELUX_SEC_CRYPTOMANAGER_HPP

#include <vector>
#include <string>

#include "telux/common/CommonDefines.hpp"
#include <telux/sec/CryptoDefines.hpp>

namespace telux {
namespace sec {

/** @addtogroup telematics_sec_mgmt
 * @{ */

/**
 * @brief ICryptoManager provides key management and crypto operation support.
 *        It uses trusted hardware bound cryptography. All keys generated are bound
 *        to the device cryptographically.
 */
class ICryptoManager {

 public:
   /**
    * Generates key and provides it in the form of a corresponding key blob. The
    * key's secret is encrypted in this key blob.
    *
    * On platforms with access control enabled, the caller needs to have TELUX_SEC_KEY_OPS
    * permission to successfully invoke this API.
    *
    * @param [in] cryptoParam Specifications of the key.
    * @param [out] keyBlob Key blob representing the key.
    *
    * @returns @ref telux::common::ErrorCode as appropriate.
    *
    */
   virtual telux::common::ErrorCode generateKey(
                    std::shared_ptr<ICryptoParam> cryptoParam,
                    std::vector<uint8_t> &keyBlob) = 0;

   /**
    * Creates a key blob from the given key data.
    *
    * On platforms with access control enabled, the caller needs to have TELUX_SEC_KEY_OPS
    * permission to successfully invoke this API.
    *
    * @param [in] cryptoParam Specifications of the key
    * @param [in] keyFmt Format in which the key should be imported (@ref KeyFormat)
    * @param [in] keyData Key's data, in the specified format, to be imported.
    * @param [out] keyBlob Key blob created from the given key data.
    *
    * @returns @ref telux::common::ErrorCode as appropriate.
    *
    */
   virtual telux::common::ErrorCode importKey(
                    std::shared_ptr<ICryptoParam> cryptoParam,
                    telux::sec::KeyFormat keyFmt,
                    std::vector<uint8_t> const &keyData,
                    std::vector<uint8_t> &keyBlob) = 0;

   /**
    * Generates equivalent key data from the given key blob.
    *
    * On platforms with access control enabled, the caller needs to have TELUX_SEC_KEY_OPS
    * permission to successfully invoke this API.
    *
    * @param [in] keyFmt @ref KeyFormat Format in which key should be exported.
    * @param [in] keyBlob Key blob representing the key to be exported.
    * @param [out] keyData Key's data generated from the given key blob.
    *
    * @returns @ref telux::common::ErrorCode as appropriate.
    *
    */
   virtual telux::common::ErrorCode exportKey(
                    telux::sec::KeyFormat keyFmt,
                    std::vector<uint8_t> const &keyBlob,
                    std::vector<uint8_t> &keyData) = 0;

   /**
    * Upgrades the given key if it has expired. For example, This API can be used when
    * a key has expired due to a system software upgrade.
    *
    * On platforms with access control enabled, the caller needs to have TELUX_SEC_KEY_OPS
    * permission to successfully invoke this API.
    *
    * @param [in] cryptoParam Input parameters passed to the upgrade algorithm. Specifically,
    *                         unique data should be set if it was used when the key was
    *                         originally created.
    * @param [in] oldKeyBlob Key blob representing the key to be upgraded.
    * @param [out] newKeyBlob Key blob representing the upgraded key.
    *
    * @returns @ref telux::common::ErrorCode as appropriate.
    *
    */
   virtual telux::common::ErrorCode upgradeKey(
                    std::shared_ptr<ICryptoParam> cryptoParam,
                    std::vector<uint8_t> const &oldKeyBlob,
                    std::vector<uint8_t> &newKeyBlob) = 0;

   /**
    * Generates a signature to verify the integrity of the given data.
    *
    * On platforms with access control enabled, the caller needs to have TELUX_SEC_SIGN_OPS
    * permission to successfully invoke this API.
    *
    * @param [in] cryptoParam Input parameters passed to the signature generation algorithm.
    * @param [in] keyBlob Key blob to sign given data.
    * @param [in] plainText Data to be signed.
    * @param [out] signature Signature generated for the given data.
    *
    * @returns @ref telux::common::ErrorCode as appropriate.
    *
    */
   virtual telux::common::ErrorCode signData(
                    std::shared_ptr<ICryptoParam> cryptoParam,
                    std::vector<uint8_t> const &keyBlob,
                    std::vector<uint8_t> const &plainText,
                    std::vector<uint8_t> &signature) = 0;

   /**
    * Verifies integrity of the given data through its signature.
    *
    * On platforms with access control enabled, the caller needs to have TELUX_SEC_SIGN_OPS
    * permission to successfully invoke this API.
    *
    * @param [in] cryptoParam Input parameters passed to the signature validation algorithm.
    * @param [in] keyBlob Key blob to verify the given data.
    * @param [in] plainText Data to be verified.
    * @param [in] signature Signature of the data.
    *
    * @returns @ref telux::common::ErrorCode::SUCCESS if verification
               is passed otherwise telux::common::ErrorCode as appropriate.
    *
    */
   virtual telux::common::ErrorCode verifyData(
                    std::shared_ptr<ICryptoParam> cryptoParam,
                    std::vector<uint8_t> const &keyBlob,
                    std::vector<uint8_t> const &plainText,
                    std::vector<uint8_t> const &signature) = 0;

   /**
    * Encrypts data per the given inputs to the encryption algorithm.
    *
    * On platforms with access control enabled, the caller needs to have TELUX_SEC_ENCRYPTION_OPS
    * permission to successfully invoke this API.
    *
    * @param [in] cryptoParam Input parameters passed to the encryption algorithm.
    * @param [in] keyBlob Key blob to be used for encryption.
    * @param [in] plainText Data to be encrypted.
    * @param [out] encryptedData Encrypted data and nonce, if
    *              @ref CryptoParamBuilder::setCallerNonce() was not set when
    *              creating keys for encryption/decryption).
    *
    * @returns @ref telux::common::ErrorCode as appropriate.
    *
    */
   virtual telux::common::ErrorCode encryptData(
                    std::shared_ptr<ICryptoParam> cryptoParam,
                    std::vector<uint8_t> const &keyBlob,
                    std::vector<uint8_t> const &plainText,
                    std::shared_ptr<EncryptedData> &encryptedData) = 0;

   /**
    * Decrypts data per the given inputs to the decryption algorithm.
    *
    * On platforms with access control enabled, the caller needs to have TELUX_SEC_ENCRYPTION_OPS
    * permission to successfully invoke this API.
    *
    * @param [in] cryptoParam Input parameters passed to the decryption algorithm.
    * @param [in] keyBlob Key blob to be used for decryption.
    * @param [in] encryptedText Encrypted data to be decrypted.
    * @param [out] decryptedText Decrypted data.
    *
    * @returns @ref telux::common::ErrorCode as appropriate.
    *
    */
   virtual telux::common::ErrorCode decryptData(
                    std::shared_ptr<ICryptoParam> cryptoParam,
                    std::vector<uint8_t> const &keyBlob,
                    std::vector<uint8_t> const &encryptedText,
                    std::vector<uint8_t> &decryptedText) = 0;

   /**
    * Destroys the ICryptoManager instance. Performs cleanup as applicable.
    */
   virtual ~ICryptoManager() {};
};

/** @} */  // end_addtogroup telematics_sec_mgmt

}  // End of namespace sec
}  // End of namespace telux

#endif // TELUX_SEC_CRYPTOMANAGER_HPP
