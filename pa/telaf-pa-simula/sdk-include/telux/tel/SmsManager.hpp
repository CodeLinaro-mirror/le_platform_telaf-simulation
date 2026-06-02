/*
 *  Copyright (c) 2017-2021 The Linux Foundation. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *    * Neither the name of The Linux Foundation nor the names of its
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 *  ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 *  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 *  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 *  OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 *  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Changes from Qualcomm Technologies, Inc. are provided under the following license:
 *
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file       SmsManager.hpp
 * @brief      SmsManager class is the primary interface to manage SMS operations such as
 *             send and receive SMS text and encoded PDU buffer(s). This class handles single part
 *             and multi-part messages.
 *
 */

#ifndef TELUX_TEL_SMSMANAGER_HPP
#define TELUX_TEL_SMSMANAGER_HPP

#include <memory>
#include <string>
#include <vector>
#include <telux/common/CommonDefines.hpp>

namespace telux {
namespace tel {

/** @addtogroup telematics_sms
 * @{ */

class ISmsListener;
class ISmscAddressCallback;

/**
 * @brief Specifies the encoding of the SMS message
 */
enum class SmsEncoding {
   GSM7,    /**< GSM 7-bit default alphabet encoding */
   GSM8,    /**< GSM 8-bit data encoding */
   UCS2,    /**< UCS-2 encoding */
   UNKNOWN, /**< Unknown encoding */
};

/**
 * @brief Specifies the SMS tag type. All incoming messages will be received and stored with
 * tag as MT_NOT_READ. It is the client's responsibility to update the tag to MT_READ using
 * @ref telux::tel::ISmsManager::setTag whenever the message is considered read.
 */
enum class SmsTagType {
   UNKNOWN = -1,   /**< Unknown tag type */
   MT_READ,        /**< MT message marked as read */
   MT_NOT_READ,    /**< MT message marked as not read */
};

/**
 * @brief Specifies the type of delete operation to be performed.
 */
enum class DeleteType {
   UNKNOWN = -1,               /**< Unknown delete type */
   DELETE_ALL,                 /**< Delete all message from memory storage */
   DELETE_MESSAGES_BY_TAG,     /**< Deletes all messages from the memory storage that
                                    match the specific message tag */
   DELETE_MSG_AT_INDEX,        /**< Deletes only the message at the specific index
                                    from the memory storage */
};

/**
 * @brief Specifies the SMS storage type for incoming message.
 */
enum class StorageType {
   UNKNOWN = -1,     /**< Unknown storage type */
   NONE,             /**< This indicates SMS not stored on any of storage and is directly notified to client. This is the default storage type */
   SIM,              /**< This indicates SMS is stored on SIM */
};

/**
 * @brief Specify delete information used for deleting message on storage.
 */
struct DeleteInfo {
    DeleteType delType; /**< Specifies the type of delete operation to be performed */
    SmsTagType tagType; /**< 1. If SMS tag type is set to @ref telux::tel::SmsTagType::UNKNOWN and delType is set to @ref telux::tel::DeleteType::DELETE_ALL then all messages on the storage would be deleted.\n 2. To delete all messages of a particular tag, set tagType to the particular tag like @ref telux::tel::SmsTagType::MT_READ and delType to @ref telux::tel::DeleteType::DELETE_MESSAGES_BY_TAG */
    uint32_t msgIndex;  /**< To delete message at specific index, specify msgIndex and delType as @ref telux::tel::DeleteType::DELETE_MSG_AT_INDEX */
};

/**
 * @brief Provides certain attributes of an SMS message.
 */
struct SmsMetaInfo {
   uint32_t msgIndex;    /**< Message index on storage */
   SmsTagType tagType;   /**< SMS tag type */
};

/**
 * @brief Contains structure of message attributes like encoding type, number
 * of segments, characters left in last segment
 */
struct MessageAttributes {
   SmsEncoding encoding;               /**< Data encoding type */
   int numberOfSegments;               /**< Number of segments */
   int segmentSize;                    /**< Max size of each segment */
   int numberOfCharsLeftInLastSegment; /**< characters left in last segment */
};

using PduBuffer = std::vector<uint8_t>;

/**
 * @brief Structure containing information about the part of multi-part SMS such as concatenated
 * message reference number, number of segments and segment number. During concatenation this
 * information along with originating address helps in associating each part of the multi-part
 * message to the corresponding multi-part message.
 */

struct MessagePartInfo {
   uint16_t refNumber;                     /**< Concatenated message reference number as per spec 3GPP TS 23.040 9.2.3.24.1. For each part of multipart message this message reference will be the same */
   uint8_t numberOfSegments;               /**< Number of segments */
   uint8_t segmentNumber;                  /**< Segment Number */

};

/**
 * @brief Data structure represents an incoming SMS. This is applicable for single part message
 *       or part of the multipart message.
 */
class SmsMessage {
public:
   SmsMessage(std::string text, std::string sender, std::string receiver, SmsEncoding encoding,
              std::string pdu, PduBuffer pduBuffer, std::shared_ptr<MessagePartInfo> info);

   SmsMessage(std::string text, std::string sender, std::string receiver, SmsEncoding encoding,
              std::string pdu, PduBuffer pduBuffer, std::shared_ptr<MessagePartInfo> info,
              bool isMetaInfoValid, SmsMetaInfo metaInfo);

   /**
    * Get the message text for the single part message or part of the multipart message.
    *
    * @returns String containing SMS message.
    */
   const std::string &getText() const;

   /**
    * Get the originating address (sender address).
    *
    * @returns String containing sender address.
    */
   const std::string &getSender() const;

   /**
    * Get the destination address (receiver address).
    *
    * @returns String containing receiver address
    */
   const std::string &getReceiver() const;

   /**
    * Get encoding format used for the single part message or part of the multipart message.
    *
    * @returns SMS message encoding used.
    */
   SmsEncoding getEncoding() const;

   /**
    * Get the raw PDU for the single part message or part of the multipart message.
    *
    * @returns String containing raw PDU content.
    *
    * @deprecated Use API SmsMessage::getRawPdu
    */
   const std::string &getPdu() const;

   /**
    * Get the raw PDU buffer for the single part message or part of the multipart message.
    *
    * @returns Buffer containing raw PDU content.
    *
    */
   PduBuffer getRawPdu() const;

   /**
    * Applicable for multi-part SMS only. Get the information such as segment number, number of
    * segments and concatenated reference number corresponding to the part of multi-part SMS.
    *
    * @returns If a message is single part SMS the method returns null otherwise returns
    *          message part information.
    *
    */
   std::shared_ptr<MessagePartInfo> getMessagePartInfo();

   /**
    * Get the text related informative representation of this object.
    *
    * @returns String containing informative string.
    */
   const std::string toString() const;

   /**
    * Get meta information of SMS stored in storage. There is no meta information when storage type
    * is none.
    *
    * @param [out] metaInfo          Meta information about SMS message stored in storage.
    *
    * @returns Status of getMetaInfo i.e. success or suitable error code.
    *
    */
   telux::common::Status getMetaInfo(SmsMetaInfo &metaInfo);

private:
   std::string text_;                                    /**< Message text */
   std::string sender_;                                  /**< Originating address (sender) */
   std::string receiver_;                                /**< Destination address (receiver) */
   SmsEncoding encoding_;                                /**< Encoding of the SMS message */
   std::string pdu_;                                     /**< Raw PDU content. This is deprecated use rawPdu_ */
   PduBuffer rawPdu_;                                    /**< Raw PDU content */
   std::shared_ptr<MessagePartInfo> msgPartInfo_;        /**< Information related to part of
                                                              multi-part message */
   bool isMetaInfoValid_;                                /**< If true meta information is valid otherwise not */
   SmsMetaInfo metaInfo_;                                /**< Meta information related to SMS stored on SIM */
};

/**
 * This function is called in response to sending a single part or multi-part SMS. This response
 * callback is invoked  when a single part message is sent or when all the parts of a multi-part
 * message is sent. This function is called in response to telux::tel::ISmsManager::sendSms and
 * telux::tel::ISmsManager::sendRawSms APIs.
 *
 * The callback can be invoked from multiple different threads.
 * The implementation should be thread safe.
 *
 * @param [in] msgRefs        This parameter represent the unique message reference number(s)
 *                            corresponding to single/multi-part message that we successfully sent.
 *                            When part of a message is delivered, the notification API i.e
 *                            @ref telux::tel::ISmsListener::onDeliveryReport will be invoked
 *                            with the message reference number corresponding to that part.
 * @param [in] errorCode      If sending any part of a multi-part message fails or a single part
 *                            message fails this API will return an @ref telux:common::ErrorCode
 *                            corresponding to the failure.
 *
 */
using SmsResponseCb = std::function<void(std::vector<int> msgRefs,
   telux::common::ErrorCode errorCode)>;

/**
 * This function can be invoked in response to getting a list of message information for the
 * messages saved in SIM storage. To get message detail at a specific index on storage,
 * @ref telux::tel::ISmsManager::readMessage API should be invoked. The callback can be
 * invoked from multiple different threads. The implementation should be thread-safe.
 *
 * @param [in] infos          List of SMS message meta information i.e
 *                            @ref telux::tel::SmsMetaInfo.
 * @param [in] errorCode      Return code which indicates whether the operation
 *                            succeeded or not. @ref telux::common::ErrorCode.
 *
 */
using RequestSmsInfoListCb = std::function<void(std::vector<SmsMetaInfo> infos,
   telux::common::ErrorCode errorCode)>;

/**
 * This function can be invoked in response to reading SMS message on SIM storage. The callback can
 * be from multiple different threads. The implementation should be thread-safe.
 *
 * @param [in] message        @ref telux::tel::SmsMessage
 * @param [in] errorCode      Return code which indicates whether the operation
 *                            succeeded or not.  @ref telux::common::ErrorCode.
 *
 */
using ReadSmsMessageCb = std::function<void(SmsMessage message,
   telux::common::ErrorCode errorCode)>;

/**
 * This function can be invoked in response to request for preferred SMS storage. The callback can
 * be from multiple different threads. The implementation should be thread-safe.
 *
 * @param [in] type           Preferred @ref telux::tel::StorageType
 * @param [in] errorCode      Return code which indicates whether the operation
 *                            succeeded or not.  @ref telux::common::ErrorCode.
 *
 */
using RequestPreferredStorageCb = std::function<void(StorageType type,
   telux::common::ErrorCode errorCode)>;

/**
 * This function can be invoked in response to request for storage details. The callback can be
 * from multiple different threads. The implementation should be thread-safe.
 *
 * @param [in] maxCount              Maximum number of messages allowed for SIM storage.
 * @param [in] availableCount        Available count in terms of SIM messages.
 * @param [in] errorCode             Return code which indicates whether the operation
 *                                   succeeded or not.  @ref telux::common::ErrorCode.
 *
 */
using RequestStorageDetailsCb = std::function<void(uint32_t maxCount, uint32_t availableCount,
   telux::common::ErrorCode errorCode)>;

/**
 * @brief SmsManager class is the primary interface to manage SMS operations such as
 *        send and receive an SMS text and raw encoded PDU(s). This class handles single part and
 *        multi-part messages.
 */
class ISmsManager {
public:

   /**
    * This status indicates whether the ISmsManager object is in a usable state.
    *
    * @returns @ref telux::common::ServiceStatus
    *
    */
   virtual telux::common::ServiceStatus getServiceStatus() = 0;

   /**
    * Send single or multipart SMS to the destination address. When registered on IMS the SMS will
    * be attempted over IMS. If sending SMS over IMS fails, an automatic retry would be attempted to
    * send the message over CS. Only support UCS2 format, GSM 7 bit default alphabet and does not
    * support National language shift tables. The SMS is sent directly not stored on storage.
    *
    * On platforms with access control enabled, caller needs to have TELUX_TEL_SMS_OPS permission
    * to invoke this API successfully.
    *
    * @param [in] message                 Message text to be send.
    * @param [in] receiverAddress         Receiver or destination address
    * @param [in] deliveryReportNeeded    Delivery status received in the listener API
    *                                     @ref telux::tel::ISmsListener if deliveryReportNeeded is
    *                                     true. Provided recipient responds to SMSC before the
    *                                     validity period expires. If deliveryReportNeeded is false
    *                                     delivery report will not be received.
    * @param [in] sentCallback            Optional callback pointer to get the sent response for
    *                                     single part or multi-part SMS.
    * @param [in] smscAddr                SMS is sent to SMSC address. If SMSC address is empty then
    *                                     pre-configured SMSC address is used.
    *
    * @returns Status of sendSms i.e. success or suitable error code.
    *
    */
   virtual telux::common::Status sendSms(std::string message, std::string receiverAddress,
      bool deliveryReportNeeded, SmsResponseCb sentCallback = nullptr,
      std::string smscAddr = "") = 0;

   /**
    * Send an SMS that is provided as a raw encoded PDU(s). When registered on IMS the SMS will
    * be attempted over IMS. If sending SMS over IMS fails, an automatic retry would be attempted to
    * send the message over CS. If the SMS is a multi-part message, the API expects multiple PDU
    * to be passed to it. The SMS is sent directly not stored on storage.
    *
    * On platforms with access control enabled, caller needs to have TELUX_TEL_SMS_OPS permission
    * to invoke this API successfully.
    *
    * @param [in] rawPdus             Each element in the vector represents a part of a multipart
    *                                 message. For single part message the vector will have single
    *                                 element.
    * @param [in] sentCallback        Optional callback to get the sent response for single part or
    *                                 multi-part SMS.
    *
    * @returns Status of sendRawSms i.e. success or suitable error code.
    *
    */
   virtual telux::common::Status sendRawSms(const std::vector<PduBuffer> rawPdus,
      SmsResponseCb sentCallback = nullptr) = 0;

   /**
    * Request for Short Messaging Service Center (SMSC) Address.Purpose of SMSC is to store,
    * forward, convert and deliver Short Message Service (SMS) messages.
    *
    * On platforms with access control enabled, caller needs to have TELUX_TEL_SMS_CONFIG permission
    * to invoke this API successfully.
    *
    * @param [in] callback        Optional callback pointer to get the response
    *                             of Smsc address request
    *
    * @returns Status of getSmscAddress i.e. success or suitable error code.
    */
   virtual telux::common::Status requestSmscAddress(std::shared_ptr<ISmscAddressCallback> callback
                                                    = nullptr)
      = 0;

   /**
    * Sets the Short Message Service Center(SMSC) address on the device.
    *
    * On platforms with access control enabled, caller needs to have TELUX_TEL_SMS_CONFIG permission
    * to invoke this API successfully.
    *
    * This will change the SMSC address for all the SMS messages sent from any
    * app.
    *
    * @param [in] smscAddress    SMSC address
    * @param [in] callback       Optional callback pointer to get the response
    *                            of set SMSC address
    *
    * @returns Status of setSmscAddress i.e. success or suitable error code.
    */
   virtual telux::common::Status setSmscAddress(const std::string &smscAddress,
                                                telux::common::ResponseCallback callback = nullptr)
      = 0;

   /**
    * Requests a list of message information for the messages saved in SIM storage.
    *
    * On platforms with access control enabled, the caller needs to have TELUX_TEL_SMS_STORAGE
    * permission to invoke this API successfully.
    *
    * @param [in] type           Specifies the tag type of the SMS message that should be matched
    *                            when retrieving the list. Specifying
    *                            @ref telux::tel::SmsTagType::UNKNOWN will retrieve all the messages
    *                            from storage.
    * @param [in] callback       Callback  to get the response of request SMS messages info.
    *
    * @returns Status of requestSmsMessageList i.e. success or suitable error code.
    *
    */
   virtual telux::common::Status requestSmsMessageList(SmsTagType type,
      RequestSmsInfoListCb callback) = 0;

   /**
    * Retrieve a particular message from SIM storage matching the index.
    *
    * On platforms with access control enabled, the caller needs to have TELUX_TEL_SMS_STORAGE
    * permission to invoke this API successfully.
    *
    * @param [in] messageIndex   SMS index on storage.
    * @param [in] callback       Callback to get the response of read SMS message from storage .
    *
    * @returns Status of readMessage i.e. success or suitable error code.
    *
    */
   virtual telux::common::Status readMessage(uint32_t messageIndex, ReadSmsMessageCb callback) = 0;

   /**
    * Delete specific SMS based on message index or delete messages on SIM storage based on
    * @ref telux::tel::SmsTagType or delete all messages from SIM storage.
    *
    * On platforms with access control enabled, the caller needs to have TELUX_TEL_SMS_STORAGE
    * permission to invoke this API successfully.
    *
    * @param [in] info           Specify delete information based on which messages are deleted
    * @param [in] callback       Optional callback to get the response of delete SMS message from
    *                            storage .
    *
    * @returns Status of deleteMessage i.e. success or suitable error code.
    *
    */
   virtual telux::common::Status deleteMessage(DeleteInfo info,
      telux::common::ResponseCallback callback = nullptr) = 0;

   /**
    * Request preferred storage for incoming SMS.
    *
    * On platforms with access control enabled, the caller needs to have TELUX_TEL_SMS_CONFIG
    * permission to invoke this API successfully.
    *
    * @param [in] callback      Callback to get the response of get preferred storage type .
    *
    * @returns Status of requestPreferredStorage i.e. success or suitable error code.
    *
    */
   virtual telux::common::Status requestPreferredStorage(RequestPreferredStorageCb callback) = 0;

   /**
    * Set the preferred storage for incoming SMS. All future messages that arrive will be stored
    * on the storage set in this API, if any. Messages in the current storage will not be moved
    * to the new storage. If client does not require messages to be stored by the platform,
    * then the storage could be set to @ref telux::tel::StorageType::NONE.
    *
    * On platforms with access control enabled, the caller needs to have TELUX_TEL_SMS_CONFIG
    * permission to invoke this API successfully.
    *
    * @param [in] storageType    @ref telux::tel::StorageType
    * @param [in] callback       Optional callback to get the response of set preferred storage.
    *
    * @returns Status of setPreferredStorage i.e. success or suitable error code.
    *
    */
   virtual telux::common::Status setPreferredStorage(StorageType storageType,
      telux::common::ResponseCallback callback = nullptr) = 0;

   /**
    * Update the tag of the incoming message stored in SIM storage as read/unread
    *
    * On platforms with access control enabled, the caller needs to have TELUX_TEL_SMS_OPS
    * permission to invoke this API successfully.
    *
    * @param [in] msgIndex       Message index corresponding to message in storage for which tag
    *                            needs to be updated.
    * @param [in] tagType        @ref telux::tel::SmsTagType. The applicable tag types are
    *                            MT_READ and MT_NOT_READ.
    * @param [in] callback       Optional callback to get the response of updating the tag.
    *
    * @returns Status of setTag i.e. success or suitable error code.
    *
    */
   virtual telux::common::Status setTag(uint32_t msgIndex, SmsTagType tagType,
      telux::common::ResponseCallback callback = nullptr) = 0;

   /**
    * Request details about SIM storage like total size and available size in terms of number of
    * messages.
    *
    * On platforms with access control enabled, the caller needs to have TELUX_TEL_SMS_CONFIG
    * permission to invoke this API successfully.
    *
    * @param [in] callback      Callback to get the response of storage detail request.
    *
    * @returns Status of requestStorageDetails i.e. success or suitable error code.
    *
    */
   virtual telux::common::Status requestStorageDetails(RequestStorageDetailsCb callback) = 0;

   /**
    * Calculate message attributes for the given message.
    *
    * @param [in] message         Message to send
    *
    * @returns MessageAttributes structure containing encoding type, number of
    * segments, max size of segment and characters left in last segment.
    */
   virtual MessageAttributes calculateMessageAttributes(const std::string &message) = 0;

   /**
    * Get associated phone id for this SMSManager.
    *
    * @returns PhoneId.
    */
   virtual int getPhoneId() = 0;

   /**
    * Register a listener for Sms events
    *
    * @param [in] listener    Pointer to ISmsListener object which receives event
    *                         corresponding to SMS
    *
    * @returns Status of registerListener i.e. success or suitable error code.
    */
   virtual telux::common::Status registerListener(std::weak_ptr<ISmsListener> listener) = 0;

   /**
    * Remove a previously added listener.
    *
    * @param [in] listener    Pointer to ISmsListener object
    *
    * @returns Status of removeListener i.e. success or suitable error code.
    */
   virtual telux::common::Status removeListener(std::weak_ptr<ISmsListener> listener) = 0;

   /**
    * Send SMS to the destination address. When registered on IMS the SMS will be attempted over
    * IMS. If sending SMS over IMS fails, an automatic retry would be attempted to send the message
    * over CS. Only support UCS2 format, GSM 7 bit default alphabet and does not support National
    * language shift tables.
    *
    * On platforms with access control enabled, caller needs to have TELUX_TEL_SMS_OPS permission
    * to invoke this API successfully.
    *
    * @param [in] message           Message text to be sent
    * @param [in] receiverAddress   Receiver or destination address
    * @param [in] sentCallback      Optional callback pointer to get the response
    *                               of send SMS request.
    * @param [in] deliveryCallback  Optional callback pointer to get message
    *                               delivery status
    *
    * @deprecated Use API ISmsManager::sendSms(const std::string &message,
         const std::string &receiverAddress, bool deliveryReportNeeded = true,
         SmsResponseCb callback = nullptr, std::string smscAddr = "")
    *
    * @returns Status of sendSms i.e. success or suitable error code.
    *
    */
   virtual telux::common::Status
      sendSms(const std::string &message, const std::string &receiverAddress,
              std::shared_ptr<telux::common::ICommandResponseCallback> sentCallback = nullptr,
              std::shared_ptr<telux::common::ICommandResponseCallback> deliveryCallback = nullptr)
      = 0;

   virtual ~ISmsManager(){};
};

/**
 * @brief A listener class receives notification for the incoming message(s) and delivery report
 *       for sent message(s).
 *
 * The methods in listener can be invoked from multiple different threads. The
 * implementation should be thread safe.
 */
class ISmsListener : public telux::common::IServiceStatusListener {
public:
   /**
    * This function will be invoked when a single part message is received or when a part of a
    * multi-part message is received. If the SMS preferred storage is to store the SMS in storage
    * i.e SIM then the SMS will be first stored in storage and then this API will be invoked.
    *
    * On platforms with access control enabled, the client needs to have TELUX_TEL_SMS_LISTEN
    * permission to invoke this API successfully.
    *
    * @param [in] phoneId      Unique identifier per SIM slot. Phone on which the message is
    *                          received.
    * @param [in] message   Pointer to SmsMessage object
    */
   virtual void onIncomingSms(int phoneId, std::shared_ptr<SmsMessage> message) {
   }

   /**
    * This function will be invoked when either a single part message is received, or when all the
    * parts of a multipart message have been received. This API is invoked only once all parts of a
    * message are received. In case of a single part message, it will be invoked as soon as it is
    * received. In case of multi-part, the implementation waits for all parts of the message to
    * arrive and then invokes this API. If the SMS preferred storage is to store the SMS in storage
    * i.e SIM then the messages will be first stored in storage and then this API will be
    * invoked.
    *
    * On platforms with access control enabled, the client needs to have TELUX_TEL_SMS_LISTEN
    * permission to invoke this API successfully.
    *
    * @param [in] phoneId           Unique identifier per SIM slot. Phone on which the message is
    *                               received.
    * @param [in] messages          Pointer to list of SmsMessage received corresponding to
    *                               single part or all parts of multipart message.
    *
    */
   virtual void onIncomingSms(int phoneId, std::shared_ptr<std::vector<SmsMessage>> messages) {
   }

   /**
    * This function will be invoked when either a delivery report for a single part message is
    * received or when the delivery report for part of a multi-part message is received. In order
    * to determine delivery of all parts of the multi-part message, the client application shall
    * compare message reference received in the delivery indications with message references
    * received in @ref telux::tel::SmsResponseCb.
    *
    * On platforms with access control enabled, the client needs to have TELUX_TEL_SMS_OPS
    * permission to invoke this API successfully.
    *
    * @param [in] phoneId             Unique identifier per SIM slot. Phone on which the message is
    *                                 received.
    * @param [in] msgRef              Message reference number (as per spec 3GPP TS 23.040 9.2.2.3)
    *                                 for a single part message or part of multipart message.
    * @param [in] receiverAddress     Receiver or destination address
    * @param [in] error               @ref telux::common::ErrorCode
    *
    */
   virtual void onDeliveryReport(int phoneId, int msgRef, std::string receiverAddress,
      telux::common::ErrorCode error) {
   }

   /**
    * This function will be invoked when SMS storage is full.
    *
    * On platforms with access control enabled, the client needs to have TELUX_TEL_SMS_STORAGE
    * permission to invoke this API successfully.
    *
    * @param [in] phoneId             Unique identifier per SIM slot. Phone on which the message is
    *                                 received.
    * @param [in] type                @ref telux::tel::StorageType. Applicable storage type
    *                                 StorageType::SIM
    *
    */
   virtual void onMemoryFull(int phoneId, StorageType type) {
   }

   virtual ~ISmsListener() {
   }
};

/**
 * Interface for SMS callback object. Client needs to implement this interface
 * to get single shot responses for send SMS.
 *
 * The methods in callback can be invoked from multiple different threads. The
 * implementation should be thread safe.
 */
class ISmscAddressCallback : public telux::common::ICommandCallback {
public:
   /**
    * This function is called with the response to the Smsc address request.
    *
    * @param [in] address    Smsc address
    * @param [in] error      @ref telux::common::ErrorCode
    */
   virtual void smscAddressResponse(const std::string &address, telux::common::ErrorCode error) = 0;
};
/** @} */ /* end_addtogroup telematics_sms */
}
}

#endif // TELUX_TEL_SMSMANAGER_HPP
