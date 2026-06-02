/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file  CryptoAcceleratorManager.hpp
 * @brief CryptoAcceleratorManager provides support for elliptic curve
 *        cryptography (ECC) operations using a dedicated hardware block.
 */

#ifndef TELUX_SEC_CRYPTOACCELERATORMANAGER_HPP
#define TELUX_SEC_CRYPTOACCELERATORMANAGER_HPP

#include <cstdint>
#include <memory>
#include <vector>

#include <telux/common/CommonDefines.hpp>

namespace telux {
namespace sec {

/** @addtogroup telematics_sec_mgmt
 * @{ */

/**
 * Defines how the user gets verification and calculation results.
 */
enum class Mode {
    /**
     * @ref ICryptoAcceleratorManager::eccVerifyDigest() and
     * @ref ICryptoAcceleratorManager::ecqvPointMultiplyAndAdd() APIs are used
     * to send verification and calculation data and obtain results synchronously.
     */
    MODE_SYNC,
    /**
     * @ref ICryptoAcceleratorManager::eccPostDigestForVerification()
     * and @ref ICryptoAcceleratorManager::ecqvPostDataForMultiplyAndAdd()
     * APIs are used to send verification and calculation data. Results are
     * obtained via @ref ICryptoAcceleratorManager::getAsyncResults() API.
     */
    MODE_ASYNC_POLL,
    /**
     * @ref ICryptoAcceleratorManager::eccPostDigestForVerification() and
     * @ref ICryptoAcceleratorManager::ecqvPostDataForMultiplyAndAdd()
     * APIs are used to send verification and calculation data. Results are
     * obtained asynchronously in @ref ICryptoAcceleratorListener::onVerificationResult()
     * and @ref ICryptoAcceleratorListener::onCalculationResult() callbacks.
     */
    MODE_ASYNC_LISTENER
};

/**
 * Relative priority of the request.
 */
enum class RequestPriority {
    /** High priority */
    REQ_PRIORITY_HIGH,
    /** Lower priority (compared to high priority data) */
    REQ_PRIORITY_NORMAL
};

/**
 * Elliptic curve used by ECC algorithm.
 */
enum class ECCCurve {
    /* ECC curve SM2 */
    CURVE_SM2,
    /* ECC curve NIST-256 */
    CURVE_NISTP256,
    /* ECC curve NIST-384 */
    CURVE_NISTP384,
    /* ECC curve Brainpool-256 */
    CURVE_BRAINPOOLP256R1,
    /* ECC curve Brainpool-384 */
    CURVE_BRAINPOOLP384R1
};

/**
 * Type of operation carried by crypto accelerator.
 */
enum class OperationType {
    /* Signature verification */
    OP_TYPE_VERIFY,
    /* ECC point calculation */
    OP_TYPE_CALCULATE
};

/**
 * Represents a point on an elliptic curve.
 */
struct ECCPoint {
    uint8_t *x;
    /**< X-coordinate */
    size_t xLength;
    /**< Length of X-coordinate in little endian order */
    uint8_t *y;
    /**< Y-coordinate */
    size_t yLength;
    /**< Length of Y-coordinate in little endian order */
};

/**
 * Represents digest of the data whose signature is to be verified.
 */
struct DataDigest {
    uint8_t *digest;
    /**< Digest of the data to be processed in little endian order */
    size_t digestLength;
    /**< Length of the digest */
};

/**
 * Represents signature of the digest to be verified.
 */
struct Signature {
    uint8_t *rSignature;
    /**< The r-component of the signature {r,s} in little endian order */
    uint8_t *sSignature;
    /**< The s-component of the signature {r,s} in little endian order */
    size_t rsLength;
    /**< Length of the signature {r,s} */
};

/**
 * Represents scalar value to be used with an ECQV operation.
 */
struct Scalar {
    /* Scalar value to use for point multiply and add ECQV operation
     * in little endian order */
    uint8_t *scalar;
    /* Length of the scalar */
    size_t scalarLength;
};

/**
 * Length of the unparsed raw result from the crypto accelerator.
 */
static const uint32_t CA_RESULT_DATA_LENGTH = 96;

/**
 * Represents a result obtained from the crypto accelerator. The value of
 * an individual field must only be interpreted through helper methods in
 * @ref ResultParser.
 */
struct OperationResult {
    /* Unused */
    uint32_t reserved:4;
    /* Unique identifier of the request that corresponds to these results */
    uint32_t id:12;
    /* ECC verification or ECQV calculation result */
    uint32_t operationType:3;
    /* Indicates if ECC verification failed or passed, or ECQV calculation
     * succeeded or not */
    uint32_t result:4;
    /* Provides a more granluar error code specific to the cryptographic hardware */
    uint32_t errCode:9;
    /* Contains r'prime for verification or ECC point for calculation */
    uint8_t data[CA_RESULT_DATA_LENGTH];
};

/**
 * Receives ECC signature verification and ECQV calculation result.
 */
class ICryptoAcceleratorListener : public telux::common::IServiceStatusListener {
 public:
    /**
     * Invoked to provide an ECC signature verification result.
     *
     * @param[in] uniqueId   Unique request identifier. This is the same as what was passed
     *                       to @ref ICryptoAcceleratorManager::eccPostDigestForVerification()
     *
     * @param[in] errorCode  telux::common::ErrorCode::SUCCESS, if signature passed validation,
     *                       telux::common::ErrorCode::VERIFICATION_FAILED if all inputs were
     *                       correct, verification completed and signature was invalid, an
     *                       appropriate error code in all other cases
     *
     * @param[in] resultData Contains the r' (computed r-component of the signature)
     *
     */
    virtual void onVerificationResult(uint32_t uniqueId, telux::common::ErrorCode errorCode,
        std::vector<uint8_t> resultData) { }

    /**
     * Invoked to provide an ECQV calculation result.
     *
     * @param[in] uniqueId   Unique request identifier. This is the same as what was passed
     *                       to @ref ICryptoAcceleratorManager::ecqvPostDataForMultiplyAndAdd()
     *
     * @param[in] errorCode  telux::common::ErrorCode::SUCCESS, if calculation succeeded,
     *                       otherwise, an appropriate error code
     *
     * @param[in] resultData Output point Q (Q=kP+A). For @ref CURVE_SM2, @ref CURVE_NISTP256
     *                       and @ref CURVE_BRAINPOOLP256R1, byte from 0 to 31 contains
     *                       x-coordinate, and byte from 32 to 63 contains y-coordinate. For
     *                       @ref CURVE_NISTP384 and @ref CURVE_BRAINPOOLP384R1, byte from 0
     *                       to 47 contains x-coordinate, and byte from 48 to 95 contains
     *                       y-coordinate.
     *
     */
    virtual void onCalculationResult(uint32_t uniqueId, telux::common::ErrorCode errorCode,
        std::vector<uint8_t> resultData) { }

    /**
     * Destructor for ICryptoAcceleratorListener.
     */
    virtual ~ICryptoAcceleratorListener() { }
};

/**
 * Provides support for ECC based signature verification and calculation related
 * crypto operations.
 *
 * APIs with asynchronous and synchronous semantics are provided for the same
 * operation, providing flexibility to optimally support multiple client solutions.
 *
 * Clients that prefer to invoke verifications from a thread and consume the results
 * on a different thread should use the asynchronous APIs. Clients that prefer to
 * invoke verification APIs and block until the result is ready, should use the
 * synchronous APIs.
 */
class ICryptoAcceleratorManager {
 public:

    //****** MODE_ASYNC_LISTENER/MODE_ASYNC_POLL - Asynchronous APIs ******//

    /**
     * Sends hashed ECC data to the crypto accelerator for integrity verification
     * using the given public key and signature.
     *
     * Verification result is received by the
     * @ref ICryptoAcceleratorListener::onVerificationResult() method for
     * @ref MODE_ASYNC_LISTENER. For @ref MODE_ASYNC_POLL, @ref getAsyncResults()
     * is used to obtain the results.
     *
     * @param[in] digest    Digest of data
     *
     * @param[in] publicKey Uncompressed public key used to verify the signature
     *
     * @param[in] signature Signature of the digest
     *
     * @param[in] curve     ECC curve on which given public key lies
     *
     * @param[in] uniqueId  Unique identifier for each request. This number must be
     *                      unique across all requests for which results are pending.
     *                      Once the result for a request is received, the same number
     *                      can be reused. Valid value range is 0 <= uniqueId <= 4095.
     *
     * @param[in] priority  Relative priority indicating this digest should be verified
     *                      before any other low priority digest
     *
     * @returns @ref telux::common::ErrorCode::SUCCESS, if the data is sent to the
     *          accelerator, otherwise an appropriate error code
     *
     */
    virtual telux::common::ErrorCode eccPostDigestForVerification(
                const DataDigest& digest,
                const ECCPoint& publicKey,
                const Signature& signature,
                telux::sec::ECCCurve curve,
                uint32_t uniqueId,
                telux::sec::RequestPriority priority) = 0;

    /**
     * Sends data to the crypto accelerator to perform a point multiplication and addition
     * for 'Short Weierstrass' curves; Q=kP+A.
     *
     * Calculation result is received by the @ref ICryptoAcceleratorListener::onCalculationResult()
     * method for @ref MODE_ASYNC_LISTENER. For @ref MODE_ASYNC_POLL, @ref getAsyncResults()
     * is used to obtain the results.
     *
     * @param[in] multiplicandPoint Point to multiply (P). In context of public key
     *                              reconstruction, it represents the reconstruction value
     *
     * @param[in] addendPoint       Point to add (A). In context of public key
     *                              reconstruction, it represents the CA public key
     *
     * @param[in] scalar            Scalar for the scalar multiplication (k). In context
     *                              of public key reconstruction, it represents the hash
     *                              construct
     *
     * @param[in] curve             ECC curve associated with point P and A
     *
     * @param[in] uniqueId          Unique identifier for each request. This number must
     *                              be unique across all requests for which results are
     *                              pending. Once the result for a request is received, the
     *                              the same number can be reused. Valid value range is
     *                              0 <= uniqueId <= 4095.
     *
     * @param[in] priority          Relative priority indicating this calculation should be
     *                              performed before any other low priority operation
     *
     * @returns @ref telux::common::ErrorCode::SUCCESS, if the data is sent to the
     *          accelerator, otherwise an appropriate error code
     *
     */
    virtual telux::common::ErrorCode ecqvPostDataForMultiplyAndAdd(
                const ECCPoint& multiplicandPoint,
                const ECCPoint& addendPoint,
                const Scalar& scalar,
                telux::sec::ECCCurve curve,
                uint32_t uniqueId,
                telux::sec::RequestPriority priority) = 0;

    /**
     * When using Mode::MODE_ASYNC_POLL,
     * @ref ICryptoAcceleratorManager::eccPostDigestForVerification() and
     * @ref ICryptoAcceleratorManager::ecqvPostDataForMultiplyAndAdd() APIs are
     * used to send request.
     *
     * The result of these request is obtained asynchronously using this method.
     * It blocks until result(s) is available or timeout occurs.
     *
     * Caller should allocate sufficient memory pointed by 'results'.
     *
     * @param[out] results          Buffer that will contain the results
     *
     * @param[in] numResultsToRead  Number of the results to read
     *
     * @param[in] timeout           Time to wait (in milliseconds) for the result(s).
     *                              Specifying a negative value means an infinite timeout.
     *                              Zero value means return immediately (there may or may
     *                              not be any results read).
     *
     * @param[out] numResultsRead   Number of results actually read
     *
     * @returns @ref telux::common::ErrorCode::SUCCESS, if the result(s) are obtained
     *          successfully, otherwise an appropriate error code
     *
     */
    virtual telux::common::ErrorCode getAsyncResults(std::vector<OperationResult>& results,
                uint32_t numResultsToRead, int32_t timeout, uint32_t& numResultsRead) = 0;

    //*********** MODE_SYNC - Synchronous APIs ***********//

    /**
     * Verifies the signature of the digest using given public key.
     *
     * @param[in] digest      Digest of data
     *
     * @param[in] publicKey   Uncompressed public key used to verify the signature
     *
     * @param[in] signature   Signature of the digest
     *
     * @param[in] curve       ECC curve on which given public key lies
     *
     * @param[in] uniqueId    Unique identifier for each request. This number must be
     *                        unique across all requests for which results are pending.
     *                        Once the result for a request is received, the same number
     *                        can be reused. Valid value range is 0 <= uniqueId <= 4095.
     *
     * @param[in] priority    Relative priority indicating this digest should be verified
     *                        before any other low priority digest
     *
     * @param[out] resultData Contains the r' prime (computed r-component of the signature)
     *
     * @returns @ref telux::common::ErrorCode::SUCCESS, if signature passed validation,
     *          telux::common::ErrorCode::VERIFICATION_FAILED if all inputs were correct,
     *          verification completed and signature was invalid, an appropriate error
     *          code in all other cases
     *
     */
    virtual telux::common::ErrorCode eccVerifyDigest(
                const DataDigest& digest,
                const ECCPoint& publicKey,
                const Signature& signature,
                telux::sec::ECCCurve curve,
                uint32_t uniqueId,
                telux::sec::RequestPriority priority,
                std::vector<uint8_t>& resultData) = 0;

    /**
     * Performs a point multiplication and addition for 'Short Weierstrass' curves;
     * Q=kP+A with the help of accelerator. This can be used, for example; to
     * reconstruct a public key, using 'Elliptic Curve Qu-Vanstone (ECQV)' implicit
     * certificate scheme.
     *
     * @param[in] multiplicandPoint Point to multiply (P). In context of public key
     *                              reconstruction, it represents the reconstruction value
     *
     * @param[in] addendPoint       Point to add (A). In context of public key
     *                              reconstruction, it represents the CA public key
     *
     * @param[in] scalar            Scalar for the scalar multiplication (k). In context
     *                              of public key reconstruction, it represents the hash
     *                              construct
     *
     * @param[in] curve             ECC curve associated with point P and A
     *
     * @param[in] uniqueId          Unique identifier for each request. This number must
     *                              be unique across all requests for which results are
     *                              pending. Once the result for a request is received, the
     *                              the same number can be reused. Valid value range is
     *                              0 <= uniqueId <= 4095.
     *
     * @param[in] priority          Relative priority indicating this calculation should be
     *                              performed before any other low priority operation
     *
     * @param[out] resultData       Output point Q (Q=kP+A). For @ref CURVE_SM2,
     *                              @ref CURVE_NISTP256 and @ref CURVE_BRAINPOOLP256R1, byte
     *                              from 0 to 31 contains x-coordinate, and byte from 32 to 63
     *                              contains y-coordinate. For @ref CURVE_NISTP384 and @ref
     *                              CURVE_BRAINPOOLP384R1, byte from 0 to 47 contains
     *                              x-coordinate, and byte from 48 to 95 contains y-coordinate.
     *
     * @returns @ref telux::common::ErrorCode::SUCCESS, if the calculation succeeded, otherwise
     *          an appropriate error code
     *
     */
    virtual telux::common::ErrorCode ecqvPointMultiplyAndAdd(
                const ECCPoint& multiplicandPoint,
                const ECCPoint& addendPoint,
                const Scalar& scalar,
                telux::sec::ECCCurve curve,
                uint32_t uniqueId,
                telux::sec::RequestPriority priority,
                std::vector<uint8_t>& resultData) = 0;

    /**
     * Destructor of ICryptoAcceleratorManager. Cleans up as applicable.
     */
    virtual ~ICryptoAcceleratorManager() {};
};

/**
 * Provides helpers to parse fields in the OperationResult.
 */
class ResultParser {
 public:
    /**
     * Gets the unique identifier associated with the result.
     *
     * @param[in] result Result obtained from @ref ICryptoAcceleratorManager::getAsyncResults()
     *
     * @returns Unique identifier associated with the result. This is the same as what was
     *          passed in request
     *
     */
    static uint32_t getId(const OperationResult& result);

    /**
     * Gets the type of operation corresponding to this result; values are
     * and OperationType::OP_TYPE_VERIFY and OperationType::OP_TYPE_CALCULATE.
     *
     * @param[in] result Result obtained from @ref ICryptoAcceleratorManager::getAsyncResults()
     *
     * @returns Operation type - OperationType::OP_TYPE_VERIFY for signature verification,
     *                           OperationType::OP_TYPE_CALCULATE for point calculation.
     *
     */
    static OperationType getOperationType(const OperationResult& result);

    /**
     * Indicates if the operation passed.
     *
     * @param[in] result Result obtained from @ref ICryptoAcceleratorManager::getAsyncResults()
     *
     * @returns For ECC verification, @ref telux::common::ErrorCode::SUCCESS, if signature
     *          passed validation, telux::common::ErrorCode::VERIFICATION_FAILED if all
     *          inputs were correct, verification completed and signature was invalid, an
     *          appropriate error code in all other cases. For ECQV calculation,
     *          @ref telux::common::ErrorCode::SUCCESS, if the calculation succeeded, an
     *          appropriate error code in all other cases
     *
     */
    static telux::common::ErrorCode getErrorCode(const OperationResult& result);

    /**
     * Provides a crypto accelerator hardware specific error code to further
     * identify the actual error. Should be used only if @ref getErrorCode() indicates
     * an error occurred.
     *
     * @param[in] result Result obtained from @ref ICryptoAcceleratorManager::getAsyncResults()
     *
     * @returns Error code - telux::common::ErrorCode::* as obtained from the accelerator
     *
     */
    static telux::common::ErrorCode getCAErrorCode(const OperationResult& result);

    /**
     * Gets the actual result data. For ECC verification, it contains r-prime and for ECQV
     * it contains coordinates.
     *
     * @param[in] result Result obtained from @ref ICryptoAcceleratorManager::getAsyncResults()
     *
     * @returns Pointer to the data, For ECC verification contains r-prime, For ECQV
     *          calculatio contains coordinates
     *
     */
    static uint8_t *getData(OperationResult& result);
};

/** @} */  // end_addtogroup telematics_sec_mgmt

}  // End of namespace sec
}  // End of namespace telux

#endif // TELUX_SEC_CRYPTOACCELERATORMANAGER_HPP
