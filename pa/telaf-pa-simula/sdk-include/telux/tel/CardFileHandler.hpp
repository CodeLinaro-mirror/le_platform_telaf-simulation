/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file      CardFileHandler.hpp
 * @brief     Card file handler is a primary interface for reading from elementary file(EF) on SIM
 *            and writing to elementary file(EF) on SIM. Provide API to get EF attributes like file
 *            size, record size, and the number of records in EF.
 */

#ifndef TELUX_TEL_CARDFILEHANDLER_HPP
#define TELUX_TEL_CARDFILEHANDLER_HPP

#include <string>
#include <vector>

#include "telux/tel/CardDefines.hpp"
#include "telux/common/CommonDefines.hpp"

namespace telux {
namespace tel {

/** @addtogroup telematics_card
 * @{ */

/**
 * Defines supported elementary file(EF) types.
 */
enum class EfType {
    UNKNOWN = 0,               /**< Unknown EF type */
    TRANSPARENT,               /**< Transparent EF */
    LINEAR_FIXED,              /**< Linear Fixed EF */
};

/**
 * SIM Elementary file attributes.
 */
struct FileAttributes {
    uint16_t fileSize;     /**< File size of transparent or linear fixed file.*/
    uint16_t recordSize;   /**< Size of the file record. Applicable only for telux::tel::EfType::LINEAR_FIXED.*/
    uint16_t recordCount;  /**< The number of records in a file. Applicable only for telux::tel::EfType::LINEAR_FIXED.*/
};

/**
 * This function is invoked when elementary file(EF) operations like either reading/writing a
 * single record to a linear fixed file or reading/writing the data to the transparent file are
 * performed.
 *
 * @param [in] error               @ref telux::common::ErrorCode
 * @param [in] result              For read operation @ref telux::tel::IccResult::data contains
 *                                 either record corresponding to linear fixed file or data
 *                                 corresponding to the transparent file. For write operation
 *                                 telux::tel::IccResult::data and telux::tel::IccResult::payload is
 *                                 empty.
 *
 */
using EfOperationCallback = std::function<void(telux::common::ErrorCode error, IccResult result)>;

/**
 * This function is called when an elementary file(EF) operation like reading all records from a
 * linear fixed file is performed.
 *
 * @param [in] error               @ref telux::common::ErrorCode
 * @param [in] records             List of records returned for EF read operation from a linear
 *                                 fixed file. If the reading of any of the records from the file
 *                                 fails then the records returned will be empty.
 *
 */
using EfReadAllRecordsCallback = std::function<void(telux::common::ErrorCode error,
    std::vector<IccResult> records)>;

/**
 * This function is called when an elementary file operation like getting file attributes is
 * performed.
 *
 * @param [in] error        @ref telux::common::ErrorCode
 * @param [in] result       @ref telux::tel::IccResult for elementary file operation like get SIM
 *                          file attributes.
 * @param [in] attributes   @ref telux::tel::FileAttributes contain EF file information like file
 *                          type and file size etc.
 *
 */
using EfGetFileAttributesCallback = std::function<void(telux::common::ErrorCode error,
    IccResult result, FileAttributes attributes)>;

/**
 *@brief ICardFileHandler provides APIs for reading from an elementary file(EF) on SIM and writing
 *       to EF on SIM. Provide API to get EF attributes like file size, record size, and the number
 *       of records in EF.
 *
 */
class ICardFileHandler {
 public:

    /**
     * Read a record from a SIM linear fixed elementary file (EF).
     *
     * @param [in] filePath       File path of the elementary file to be read
     *                            Refer ETSI GTS GSM 11.11 V5.3.0 6.5. For example to read EF FDN
     *                            corresponding to USIM app the file path is "3F007FFF"
     * @param [in] fileId         Elementary file identifier. For example File Id for EF FDN is
     *                            0x6F3B
     * @param [in] recordNum      Record number is 1-based (not 0-based)
     * @param [in] aid            Application identifier is optional for reading EF that is not part
     *                            of card application
     * @param [in] callback       Callback function to get the response of
     *                            readEFLinearFixed request
     *
     * @returns - Status of readEFLinearFixed i.e. success or suitable status code
     *
     */

    virtual telux::common::Status readEFLinearFixed(std::string filePath, uint16_t fileId,
        int recordNum, std::string aid, EfOperationCallback callback) = 0;

    /**
     * Read all records from a SIM linear fixed elementary file (EF).
     *
     * @param [in] filePath       File path of the elementary file to be read
     *                            Refer ETSI GTS GSM 11.11 V5.3.0 6.5. For example to read EF FDN
     *                            corresponding to USIM app the file path is "3F007FFF"
     * @param [in] fileId         Elementary file identifier. For example File Id for EF FDN is
     *                            0x6F3B
     * @param [in] aid            Application identifier is optional for reading EF that is not part
     *                            of card application
     * @param [in] callback       Callback function to get the response of
     *                            readEFLinearFixedAll request
     *
     * @returns - Status of readEFLinearFixedAll i.e. success or suitable status code
     *
     */

    virtual telux::common::Status readEFLinearFixedAll(std::string filePath, uint16_t fileId,
        std::string aid, EfReadAllRecordsCallback callback) = 0;

    /**
     * Read from a SIM transparent elementary file (EF).
     *
     * @param [in] filePath       File path of the elementary file to be read
     *                            Refer ETSI GTS GSM 11.11 V5.3.0 6.5. For example to read EF ICCID
     *                            the file path is "3F00"
     * @param [in] fileId         Elementary file identifier. For example File Id for EF ICCID is
     *                            0x2FE2
     * @param [in] size           If the size is zero then read the complete file otherwise, read
     *                            the first size bytes from EF.
     * @param [in] aid            Application identifier is optional for reading EF that is not part
     *                            of card application
     * @param [in] callback       Callback function to get the response of
     *                            readEFTransparent request
     *
     * @returns - Status of readEFTransparent i.e. success or suitable status code
     *
     */

    virtual telux::common::Status readEFTransparent(std::string filePath, uint16_t fileId, int size,
        std::string aid, EfOperationCallback callback) = 0;

    /**
     * Write a record in a SIM linear fixed elementary file (EF).
     *
     * @param [in] filePath       File path of the elementary file to be written.
     *                            Refer ETSI GTS GSM 11.11 V5.3.0 6.5. For example to update record
     *                            to EF FDN corresponding to USIM app the file path is "3F007FFF"
     * @param [in] fileId         Elementary file identifier. For example File Id for EF FDN is
     *                            0x6F3B
     * @param [in] recordNum      Record number is 1-based (not 0-based)
     * @param [in] data           Data represents record in the EF
     * @param [in] pin2           Pin2 for card holder verification(CHV2) operations, otherwise must
     *                            be empty.
     * @param [in] aid            Application identifier is optional for writing to EF that is not
     *                            part of card application.
     * @param [in] callback       Callback function to get the response of
     *                            writeEFLinearFixed request
     *
     * @returns - Status of writeEFLinearFixed i.e. success or suitable status code
     *
     */

    virtual telux::common::Status writeEFLinearFixed(std::string filePath, uint16_t fileId,
        int recordNum, std::vector<uint8_t> data, std::string pin2, std::string aid,
        EfOperationCallback callback) = 0;

    /**
     * Write in a SIM transparent elementary file (EF).
     *
     * @param [in] filePath       File path of the elementary file to be written
     *                            Refer ETSI GTS GSM 11.11 V5.3.0 6.5. For example to write to EF
     *                            ICCID the file path is "3F00"
     * @param [in] fileId         Elementary file identifier. For example File Id for EF ICCID is
     *                            0x2FE2
     * @param [in] data           Binary data to be written on the EF
     * @param [in] aid            Application identifier is optional for writing to EF that is not
     *                            part of card application.
     * @param [in] callback       Callback function to get the response of writeEFTransparent
     *                            request
     *
     * @returns - Status of writeEFTransparent i.e. success or suitable status code
     *
     */

    virtual telux::common::Status writeEFTransparent(std::string filePath, uint16_t fileId,
        std::vector<uint8_t> data, std::string aid, EfOperationCallback callback) = 0;

    /**
     * Get file attributes for SIM elementary file(EF).
     *
     * @param [in] efType         Elementary file type i.e. telux::tel::EfType
     * @param [in] filePath       File path of the elementary file to read file attributes
     *                            Refer ETSI GTS GSM 11.11 V5.3.0 6.5. For example to read file
     *                            attributes of EF ICCID the file path is "3F00"
     * @param [in] fileId         Elementary file identifier. For example File Id for EF ICCID is
     *                            0x2FE2
     * @param [in] aid            Application identifier is optional for EF that is not part of
     *                            card application.
     * @param [in] callback       Callback function to get the response of
     *                            requestEFAttributes request
     *
     * @returns - Status of requestEFAttributes i.e. success or suitable status code
     *
     */

    virtual telux::common::Status requestEFAttributes(EfType efType, std::string filePath,
        uint16_t fileId, std::string aid, EfGetFileAttributesCallback callback) = 0;

    /**
     * Get associated slot identifier for ICardFileHandler
     *
     * @returns telux::common::SlotId
     *
     */
    virtual SlotId getSlotId() = 0;

    virtual ~ICardFileHandler() {
    }

};

/** @} */ /* end_addtogroup telematics_card */

}  // End of namespace tel
}  // End of namespace telux

#endif // TELUX_TEL_CARDFILEHANDLER_HPP
