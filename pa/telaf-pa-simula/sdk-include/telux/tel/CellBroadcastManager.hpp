/*
 *  Copyright (c) 2020, The Linux Foundation. All rights reserved.
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
 * @file       CellBroadcastManager.hpp
 *
 * @brief      CellBroadcastManager class is primary interface to configure,
 *             activate and receive 3GPP ETWS/CMAS cell broadcast messages as
 *             per 3GPP TS 23.041 specification. Currently 3GPP2 cell broadcast
 *             message is not supported.
 *
 */

#ifndef TELUX_TEL_CELLBROADCASTMANAGER_HPP
#define TELUX_TEL_CELLBROADCASTMANAGER_HPP

#include <vector>
#include <string>
#include <memory>
#include <future>
#include <telux/tel/CellBroadcastDefines.hpp>
#include <telux/common/CommonDefines.hpp>

namespace telux {
namespace tel {

/** @addtogroup telematics_cellbroadcast
 * @{ */

class ICellBroadcastListener;

/**
 * This function is called with the response to requestMessageIdFilters API.
 *
 * The callback can be invoked from multiple different threads.
 * The implementation should be thread safe.
 *
 * @param [in] filters         List of broadcast filters @ref CellBroadcastFilter
 * @param [in] error           Return code which indicates whether the operation
 *                             succeeded or not @ref ErrorCode
 */
using RequestFiltersResponseCallback
   = std::function<void(std::vector<CellBroadcastFilter> filters, telux::common::ErrorCode error)>;


/**
 * This function is called with the response to requestActivationStatus API.
 *
 * The callback can be invoked from multiple different threads.
 * The implementation should be thread safe.
 *
 * @param [in] isActivated     If true then filters are activated else filters are deactivated
 * @param [in] error           Return code which indicates whether the operation
 *                             succeeded or not @ref ErrorCode
 */
using RequestActivationStatusResponseCallback
   = std::function<void(bool isActivated, telux::common::ErrorCode error)>;

/**
 * Point represented by latitude and longitude.
 */
struct Point {
   double latitude;
   double longitude;
};

/**
 * This class represents a simple polygon with different points.
 */
class Polygon {
public:
   /**
    * Polygon constructor.
    * @param [in] vertices           List of @ref telux::tel::Point
    */
   Polygon(std::vector<Point> vertices);

   /**
    * Get vertices of polygon.
    *
    * @returns List of @ref telux::tel::Point.
    */
   std::vector<Point> getVertices();

private:
   std::vector<Point> vertices_;
};

/**
 * This class represents a geometry represented as simple circle.
 */
class Circle {
public:
   /**
    * Circle constructor.
    * @param [in] center          Center of circle represented by telux::tel::Point
    * @param [in] radius          Radius of circle in meters
    */
   Circle(Point center, double radius);

   /**
    * Get center point of circle.
    *
    * @returns Center of circle.
    */
   Point getCenter();

   /**
    * Get radius of circle.
    *
    * @returns Radius of circle.
    */
   double getRadius();

private:
   Point center_;
   double radius_;
};

/**
 * This class represents warning area geometry to perform geofencing on alert.
 */
class Geometry {
public:
   /**
    * Geometry constructor.
    * @param [in] polygon            @ref Polygon
    */
   Geometry(std::shared_ptr<Polygon> polygon);

   /**
    * Geometry constructor.
    * @param [in] circle             @ref Circle
    */
   Geometry(std::shared_ptr<Circle> circle);

   /**
    * Get the geometry type.
    *
    * @returns @ref GeometryType.
    *
    */
   GeometryType getType() const;

   /**
    * Get polygon goemetry as warning area to perform geofencing. This method should be called
    * only if geometry type returned by getType() API is GeometryType::POLYGON
    *
    * @return Polygon geometry object.
    *
    */
   std::shared_ptr<Polygon> getPolygon() const;

   /**
    * Get circle goemetry as warning area to perform geofencing. This method should be called
    * only if geometry type returned by getType() API is GeometryType::CIRCLE
    *
    * @return Circle geometry object.
    *
    */
   std::shared_ptr<Circle> getCircle() const;

private:
   GeometryType type_;
   std::shared_ptr<Polygon> polygon_;
   std::shared_ptr<Circle> circle_;

};

/**
 * This class represents warning area information for alert.
 */
class WarningAreaInfo {
public:
   /**
    * Warning Area Information constructor.
    * @param [in] maxWaitTime         Maximum wait time allowed to determine position for alert
    *                                 Range is  0 to 255 where 0 means Zero wait time,
    *                                 1 - 254 is Geo-Fencing Maximum Wait Time in seconds and
    *                                 255 means use device default wait time.
    * @param [in] geometries          Geometries to perform geofencing on alert
    */
   WarningAreaInfo(int maxWaitTime, std::vector<Geometry> geometries);

   /**
    * Get maximum wait time allowed to determine position for alert.
    *
    * @returns Maximum wait time for alert in seconds.
    */
   int getGeoFenceMaxWaitTime();

   /**
    * Get geometries to perform geofencing on alert.
    *
    * @returns List of @ref telux::tel::Geometry.
    */
   std::vector<Geometry> getGeometries();


private:
   int maxWaitTime_;
   std::vector<Geometry> geometries_;
};

/**
 * Contains information elements for a GSM/UMTS/E-UTRAN/NG-RAN ETWS warning notification.
 * Supported values for each element are defined in 3GPP TS 23.041.
 */
class EtwsInfo {
public:

   /**
    * EtwsInfo constructor.
    * @param [in] geographicalScope           @ref GeographicalScope
    * @param [in] msgId                       Unique message identifier
    * @param [in] serialNumber                Serial number for message
    * @param [in] languageCode                ISO-639-1 language code for message
    * @param [in] messageText                 Message text
    * @param [in] priority                    @ref MessagePriority
    * @param [in] warningType                 @ref EtwsWarningType
    * @param [in] emergencyUserAlert          If true message is emergency user alert
    *                                         otherwise not
    * @param [in] activatePopup               If true message message activate popup flag
    *                                         is set, otherwise popup flag is false.
    * @param [in] primary                     If true ETWS message is primary notification
    *                                         otherwise not
    * @param [in] warningSecurityInformation  Buffer containing security information about ETWS
    *                                         primary notification such as timestamp and digital
    *                                         signature
    */
   EtwsInfo(GeographicalScope geographicalScope, int msgId, int serialNumber,
      std::string languageCode, std::string messageText, MessagePriority priority,
      EtwsWarningType warningType, bool emergencyUserAlert, bool activatePopup,
      bool primary, std::vector<uint8_t> warningSecurityInformation);

   /**
    * Get the geographicalScope of cellbroadcast message.
    *
    * @returns @ref GeographicalScope.
    *
    */
   GeographicalScope getGeographicalScope() const;

   /**
    * Get cellbroadcast message identifier. The message identifier identifies the type of the
    * cell broadcast message defined in spec 3GPP TS 23.041 9.4.1.2.2
    *
    * @returns Message identifier.
    *
    */
   int getMessageId() const;

   /**
    * Get the serial number of broadcast (geographical scope + message code + update number
    * for GSM/UMTS).
    *
    * @returns int containing cellbroadcast serial number.
    *
    */
   int getSerialNumber() const;

   /**
    * Get the ISO-639-1 language code for cell broadcast message, or empty string if unspecified.
    * This is not applicable for ETWS primary notification.
    *
    * @return Language code
    *
    */
   std::string getLanguageCode() const;

   /**
    * Get the body of cell broadcast message, or empty string if no body available.
    * For ETWS primary notification based on message identifier pre cannned message will be
    * sent.
    *
    * @return body or empty string
    *
    */
   std::string getMessageBody() const;

   /**
    * Get the priority for the cell broadcast message.
    *
    * @return @ref MessagePriority.
    *
    */
   MessagePriority getPriority() const;

   /**
    * Get the cellbroadcast message code.
    *
    * @returns int containing cellbroadcast message code.
    *
    */
   int getMessageCode() const;

   /**
    * Get the cellbroadcast message update number.
    *
    * @returns int containing cellbroadcast message's update number.
    *
    */
   int getUpdateNumber() const;

   /**
    * Get ETWS warning type.
    *
    * @returns @ref EtwsWarningType.
    *
    */
   EtwsWarningType getEtwsWarningType();

   /**
    * Returns the ETWS emergency user alert flag.
    *
    * @return true to notify terminal to activate emergency user alert or false otherwise
    *
    */
   bool isEmergencyUserAlert();

   /**
    * Returns the ETWS activate popup flag.
    *
    * @return true to notify terminal to activate display popup or false otherwise
    *
    */
   bool isPopupAlert();

   /**
     * Returns the ETWS format flag. This flag determine whether ETWS message is primary
     * notification or not.
     *
     * @return true if the message is primary message, otherwise secondary message
     *
     */
   bool isPrimary();

   /**
     * Returns security information about ETWS primary notification such as
     * timestamp and digital signature(applicable only for GSM).
     *
     * @return byte buffer
     *
     */
   std::vector<uint8_t> getWarningSecurityInformation();

private:
   GeographicalScope scope_;
   int messageId_;
   int serialNum_;
   int messageCode_;
   int updateNumber_;
   std::string languageCode_ = "";
   std::string body_ = "";
   MessagePriority priority_;
   EtwsWarningType warningType_;
   bool emergencyUserAlert_;
   bool activatePopup_;
   bool isPrimary_;
   std::vector<uint8_t> warningInfo_;

};

/**
 * Contains information elements for a GSM/UMTS/E-UTRAN/NG-RAN CMAS warning notification.
 * Supported values for each element are defined in 3GPP TS 23.041.
 */
class CmasInfo {
public:

   /**
    * CmasInfo constructor.
    * @param [in] geographicalScope       @ref GeographicalScope
    * @param [in] msgId                   Unique message identifier
    * @param [in] serialNumber            Serial number for message
    * @param [in] languageCode            ISO-639-1 language code for message
    * @param [in] messageText             Message text
    * @param [in] priority                @ref MessagePriority
    * @param [in] messageClass            @ref CmasMessageClass
    * @param [in] severity                @ref CmasSeverity
    * @param [in] urgency                 @ref CmasUrgency
    * @param [in] certainty               @ref CmasCertainty
    * @param [in] warningAreaInfo         @ref WarningAreaInfo
    */
   CmasInfo(GeographicalScope geographicalScope, int msgId, int serialNumber,
      std::string languageCode, std::string messageText, MessagePriority priority,
      CmasMessageClass messageClass, CmasSeverity severity, CmasUrgency urgency,
      CmasCertainty certainty, std::shared_ptr<WarningAreaInfo> warningAreaInfo);

   /**
    * Get the geographicalScope of cellbroadcast message.
    *
    * @returns @ref GeographicalScope.
    *
    */
   GeographicalScope getGeographicalScope() const;

   /**
    * Get cellbroadcast message identifier. The message identifier identifies the type of the
    * cell broadcast message defined in spec 3GPP TS 23.041 9.4.1.2.2
    *
    * @returns Message identifier.
    *
    */
   int getMessageId() const;

   /**
    * Get the serial number of broadcast (geographical scope + message code + update number
    * for GSM/UMTS).
    *
    * @returns int containing cellbroadcast serial number.
    *
    */
   int getSerialNumber() const;

   /**
    * Get the ISO-639-1 language code for cell broadcast message, or empty string if unspecified.
    * This is not applicable for ETWS primary notification.
    *
    * @return Language code
    *
    */
   std::string getLanguageCode() const;

   /**
    * Get the body of cell broadcast message, or empty string if no body available.
    * For ETWS primary notification based on message identifier pre cannned message will be
    * sent.
    *
    * @return body or empty string
    *
    */
   std::string getMessageBody() const;

   /**
    * Get the priority for the cell broadcast message.
    *
    * @return @ref MessagePriority.
    *
    */
   MessagePriority getPriority() const;

   /**
    * Get the cellbroadcast message code.
    *
    * @returns int containing cellbroadcast message code.
    *
    */
   int getMessageCode() const;

   /**
    * Get the cellbroadcast message update number.
    *
    * @returns int containing cellbroadcast message's update number.
    *
    */
   int getUpdateNumber() const;

   /**
    * Get CMAS message class.
    *
    * @returns @ref CmasMessageClass.
    *
    */
   CmasMessageClass getMessageClass();

   /**
    * Get CMAS message severity.
    *
    * @returns @ref CmasSeverity.
    *
    */
   CmasSeverity getSeverity();

   /**
    * Get CMAS message urgency.
    *
    * @returns @ref CmasUrgency.
    *
    */
   CmasUrgency getUrgency();

   /**
    * Get CMAS message certainty.
    *
    * @returns @ref CmasCertainty.
    *
    */
   CmasCertainty getCertainty();

   /**
    * Returns warning area information for alert. This is applicable for LTE and NR5G
    *
    * @return  pointer to WarningAreaInfo or null if there is no warning area information available.
    *
    */

   std::shared_ptr<WarningAreaInfo> getWarningAreaInfo();

private:
   GeographicalScope scope_;
   int messageId_;
   int serialNum_;
   int messageCode_;
   int updateNumber_;
   std::string languageCode_ = "";
   std::string body_ = "";
   MessagePriority priority_;
   CmasMessageClass messageClass_;
   CmasSeverity severity_;
   CmasUrgency urgency_;
   CmasCertainty certainity_;
   std::shared_ptr<WarningAreaInfo> warningAreaInfo_;

};

/**
 * @brief Cell Broadcast message.
 */
class CellBroadcastMessage {
public:

   /**
    * CellBroadcastMessage constructor.
    * @param [in] etwsInfo    @ref EtwsInfo
    */
   CellBroadcastMessage(std::shared_ptr<EtwsInfo> etwsInfo);

   /**
    * CellBroadcastMessage constructor.
    * @param [in] cmasInfo    @ref CmasInfo
    */
   CellBroadcastMessage(std::shared_ptr<CmasInfo> cmasInfo);

   /**
    * Get the cellbroadcast message type.
    *
    * @returns @ref MessageType.
    *
    */
   MessageType getMessageType() const;

   /**
    * Get ETWS warning notification containing information about the ETWS warning type,
    * the emergency user alert flag and the popup flag. This method should be called only if
    * message type returned by getMessageType() API is MessageType::ETWS
    *
    * @return  pointer to EtwsInfo or null if this is not an ETWS warning notification
    *
    */
   std::shared_ptr<EtwsInfo> getEtwsInfo() const;

   /**
    * Get CMAS warning notification containing information about the CMAS message class, severity,
    * urgency and certainty.This method should be called only if message type returned by
    * getMessageType() API is MessageType::CMAS
    *
    * @return pointer to CmasInfo or null if this is not a CMAS warning notification
    *
    */
   std::shared_ptr<CmasInfo> getCmasInfo() const;

private:
   MessageType messageType_;
   std::shared_ptr<EtwsInfo> etwsInfo_;
   std::shared_ptr<CmasInfo> cmasInfo_;
};

/**
 * @brief CellBroadcastManager class is primary interface to configure and
 *        activate emergency broadcast messages and receive broadcast messages.
 */
class ICellBroadcastManager {
public:
   /**
    * Checks the status of network subsystem and returns the result.
    *
    * @deprecated Use ICellBroadcastManager::getServiceStatus() API.
    *
    * @returns True if network subsystem is ready for service otherwise false.
    *
    */
   virtual bool isSubsystemReady() = 0;

   /**
    * Wait for network subsystem to be ready.
    *
    * @deprecated Use InitResponseCb in PhoneFactory::getCellBroadcastManager instead,
    *             to get notified about subsystem readiness.
    *
    * @returns  A future that caller can wait on to be notified when network
    *           subsystem is ready.
    *
    */
   virtual std::future<bool> onSubsystemReady() = 0;

   /**
    * This status indicates whether the ICellBroadcastManager object is in a usable state.
    *
    * @returns SERVICE_AVAILABLE    - If CellBroadcast Manager is ready for service.
    *          SERVICE_UNAVAILABLE  - If CellBroadcast Manager is temporarily unavailable.
    *          SERVICE_FAILED       - If CellBroadcast Manager encountered an irrecoverable
    *                                 failure.
    *
    */
    virtual telux::common::ServiceStatus getServiceStatus() = 0;

   /**
    * Get associated slot for this CellBroadcastManager.
    *
    * @returns SlotId
    *
    */
   virtual SlotId getSlotId() = 0;

   /**
    * Configures the broadcast messages to be received.
    *
    * On platforms with access control enabled, caller needs to have TELUX_TEL_CELL_BROADCAST_CONFIG
    * permission to invoke this API successfully.
    *
    * @param [in] filters        List of filtered broadcast message identifiers.
    * @param [in] callback       Optional callback to get the response
    *                            of set cell broadcast filters.
    *
    * @returns Status of updateMessageIdFilters i.e. success or suitable error code.
    *
    */
   virtual telux::common::Status updateMessageFilters(std::vector<CellBroadcastFilter> filters,
      telux::common::ResponseCallback callback = nullptr)
      = 0;

   /**
    * Retrieve configured message filters for which broadcast messages will be received.
    *
    * On platforms with access control enabled, caller needs to have TELUX_TEL_CELL_BROADCAST_CONFIG
    * permission to invoke this API successfully.
    *
    * @param [in] callback       Callback to get the response of get cell broadcast
    *                            filters.
    *
    * @returns Status of requestMessageIdFilters i.e. success or suitable error code.
    *
    */
   virtual telux::common::Status requestMessageFilters(
      RequestFiltersResponseCallback callback) = 0;

   /**
    * Allows activation and deactivation of configured broadcast messages.
    *
    * On platforms with access control enabled, caller needs to have TELUX_TEL_CELL_BROADCAST_CONFIG
    * permission to invoke this API successfully.
    *
    * @param [in] activate       Activate/deactivate broadcast messages.
    * @param [in] callback       Optional callback pointer to get the response.
    *
    * @returns Status of setActivationStatus i.e. success or suitable error code.
    *
    */
   virtual telux::common::Status setActivationStatus(bool activate,
      telux::common::ResponseCallback callback = nullptr) = 0;

   /**
    * Get activation status for configured broadcast messages.
    *
    * On platforms with access control enabled, caller needs to have TELUX_TEL_CELL_BROADCAST_CONFIG
    * permission to invoke this API successfully.
    *
    * @param [in] callback       Callback pointer to get the response.
    *
    * @returns Status of requestActivationStatus i.e. success or suitable error code.
    *
    */
   virtual telux::common::Status requestActivationStatus(
      RequestActivationStatusResponseCallback callback) = 0;

   /**
    * Register a listener for cell broadcast messages.
    *
    * @param [in] listener    Pointer to ICellBroadcastListener object which receives broadcast
    * message.
    *
    * @returns Status of registerListener i.e. success or suitable error code.
    *
    */
   virtual telux::common::Status registerListener(std::weak_ptr<ICellBroadcastListener> listener)
      = 0;

   /**
    * De-register the listener.
    *
    * @param [in] listener    Listener to be de-registered
    *
    * @returns Status of deregisterListener i.e. success or suitable error code.
    *
    */
   virtual telux::common::Status deregisterListener(std::weak_ptr<ICellBroadcastListener> listener)
      = 0;

   virtual ~ICellBroadcastManager(){};
};

/**
 * @brief A listener class which monitors cell broadcast messages.
 *
 * The methods in listener can be invoked from multiple different threads. The
 * implementation should be thread safe.
 */
class ICellBroadcastListener : public common::IServiceStatusListener {
public:
   /**
    * This function is called when device receives an incoming cell broadcast message.
    *
    * On platforms with access control enabled, the client needs to have
    * TELUX_TEL_CELL_BROADCAST_LISTEN permission to invoke this API successfully.
    *
    * @param [in] slotId    - Slot Id on which broadcast message is received.
    * @param [in] cbMessage - Broadcast message with information related to ETWS/CMAS notification.
    *
    */
   virtual void onIncomingMessage(SlotId slotId,
      const std::shared_ptr<CellBroadcastMessage> cbMessage) {
   }

   /**
    * This function is called when there is change in broadcast configuration like updation
    * of message filters by the client using @ref ICellBroadcastManager::updateMessageFilters.
    *
    * On platforms with access control enabled, the client needs to have
    * TELUX_TEL_CELL_BROADCAST_CONFIG permission to invoke this API successfully.
    *
    * @param [in] slotId  - Slot Id on which change in message filters is received.
    * @param [in] filters - Complete list of configured broadcast message filters.
    *
    */
   virtual void onMessageFilterChange(SlotId slotId, std::vector<CellBroadcastFilter> filters) {
   }

   virtual ~ICellBroadcastListener() {
   }
};
/** @} */ /* end_addtogroup telematics_cellbroadcast */
}  // End of namespace tel
}  // End of namespace telux
#endif // TELUX_TEL_CELLBROADCASTMANAGER_HPP
