/*
 *  Copyright (c) 2017-2020, The Linux Foundation. All rights reserved.
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
 * @file       LocationListener.hpp
 *
 * @brief      Interface for location service listener object. Client needs to
 * implement this
 *             interface to get location service notifications like location
 * update,
 *             satellite vehicle info etc.
 *             The methods in listener can be invoked from multiple threads.
 *             Client needs to make sure that implementation is thread-safe.
 *
 */

#ifndef TELUX_LOC_LOCATIONLISTENER_HPP
#define TELUX_LOC_LOCATIONLISTENER_HPP

#include <telux/common/SDKListener.hpp>
#include "telux/loc/LocationDefines.hpp"
#include <memory>

namespace telux {

namespace loc {
// forward declarations
class ILocationInfoBase;
class ILocationInfoEx;
class IGnssSVInfo;

/** @addtogroup telematics_location
* @{ */

/**
 * @brief Listener class for getting location updates and satellite vehicle
 * information.
 *
 * The methods in listener can be invoked from multiple different
 * threads. Client needs to make sure that implementation is thread-safe.
 *
 */
class ILocationListener : public telux::common::ISDKListener {
public:

/**
 * This function is called when device receives location update. When there are
 * multiple engines running on the system, the received location information is
 * fused report from all engines.
 *
 * On platforms with Access control enabled, the client needs to have TELUX_LOC_DATA permission
 * for this listener API to be invoked.
 *
 * @param [in] locationInfo - Location information  like latitude, longitude,
 * timeInfo other information such as heading, altitude and velocity etc.
 *
 */
  virtual void onBasicLocationUpdate(
      const std::shared_ptr<ILocationInfoBase> &locationInfo) {}

/**
 * This function is called when device receives Gnss location update. When there
 * are multiple engines running on the system, the received location information
 * is fused report from all engines.
 *
 * On platforms with Access control enabled, the client needs to have TELUX_LOC_DATA permission
 * for this listener API to be invoked.
 *
 * @param [in] locationInfo - Contains richer set of location information
 * like latitude, longitude, timeInfo, heading, altitude, velocity and other
 * information such as deviations, elliptical accuracies etc.
 *
 */
  virtual void onDetailedLocationUpdate(
      const std::shared_ptr<ILocationInfoEx> &locationInfo) {}

/**
 * This function is called when device receives multiple Gnss location update from
 * the different engine types requested, which are SPE/PPE/FUSED.
 * This API will be called ONLY if we use startDetailedEngineReports.
 *
 * On platforms with Access control enabled, the client needs to have TELUX_LOC_DATA permission
 * for this listener API to be invoked.
 *
 * @param [in] locationEngineInfo - Contains a list of location infos. Each element in
 * the list corresponds to one of SPE/PPE/FUSED.
 *
 */
  virtual void onDetailedEngineLocationUpdate(
      const std::vector<std::shared_ptr<ILocationInfoEx> > &locationEngineInfo) {}

/**
 * This function is called when device receives GNSS satellite information.
 *
 * On platforms with Access control enabled, the client needs to have TELUX_LOC_DATA permission
 * for this listener API to be invoked.
 *
 * @param [in] gnssSVInfo - GNSS satellite information
 *
 */
  virtual void onGnssSVInfo(const std::shared_ptr<IGnssSVInfo> &gnssSVInfo) {}

/**
 * This function is called when device receives GNSS data information
 * like jammer metrics and automatic gain control for satellite signal type.
 *
 * On platforms with Access control enabled, the client needs to have TELUX_LOC_DATA permission
 * for this listener API to be invoked.
 *
 * @param [in] info - GNSS signal info
 *
 */
  virtual void onGnssSignalInfo(const std::shared_ptr<IGnssSignalInfo> &info) {}

/**
 * This function is called when device receives GNSS NMEA sentences from FUSED engine.
 *
 * To receive these updates, clients need to set the @ref telux::loc::GnssReportType::NMEA bit
 * in the reportMask passed as a paramter to @ref ILocationManager::startDetailedReports or
 * @ref ILocationManager::startDetailedEngineReports.
 *
 * Also refer to @ref ILocationManager::startDetailedEngineReports to understand the usage further.
 *
 * On platforms with Access control enabled, the client needs to have TELUX_LOC_DATA permission
 * for this listener API to be invoked.
 *
 * @param [in] timestamp - Timestamp
 * @param [in] nmea - Nmea sentence
 */
  virtual void onGnssNmeaInfo(uint64_t timestamp, const std::string &nmea) {}

/**
 * This function is called when device receives NMEA sentences from a specific engine.
 *
 * To receive these updates, clients need to set the @ref telux::loc::GnssReportType::ENGINE_NMEA
 * bit in the reportMask passed as a paramter to @ref ILocationManager::startDetailedEngineReports.
 *
 * Also refer to @ref ILocationManager::startDetailedEngineReports to understand the usage further.
 *
 * On platforms with Access control enabled, the client needs to have TELUX_LOC_DATA permission
 * for this listener API to be invoked.
 *
 * @param [in] engineType - Engine Type used in generating the NMEA sentence.
 * @param [in] timestamp - Timestamp
 * @param [in] nmea - Nmea sentence
 */
  virtual void onEngineNmeaInfo(LocationAggregationType engineType, uint64_t timestamp,
    const std::string &nmea) {}

/**
 * This function is called when device receives signal measurement information
 * such as satellite vehicle pseudo range, satellite vehicle clock time, carrier phase
 * measurement etc.
 * The frequency at which this API is called is determined by what was requested
 * @ref telux::loc::GnssReportType::MEASUREMENT or @ref telux::loc::GnssReportType::HIGH_RATE_MEASUREMENT in
 * @ref ILocationManager::startDetailedReports and
 * @ref ILocationManager::startDetailedEngineReports.
 *
 * On platforms with Access control enabled, the client needs to have TELUX_LOC_DATA permission
 * for this listener API to be invoked.
 *
 * @param [in] measurementInfo - GNSS measurement information
 *
 */
  virtual void onGnssMeasurementsInfo(const telux::loc::GnssMeasurements &measurementInfo) {}

/**
 * This function is called during a disaster/crisis to update the
 * disaster/crisis reports.
 *
 * @param [in] dcReportInfo - GNSS disaster/crisis report information.
 *             This includes the report type and data payload received from the GNSS engine.
 */
  virtual void onGnssDisasterCrisisInfo(const telux::loc::GnssDisasterCrisisReport &dcReportInfo){}

/**
 * This function is called when there is an update in the ephemeris information for a constellation.
 *
 * To receive these updates, clients need to set the @ref telux::loc::GnssReportType::EPHEMERIS bit
 * in the reportMask passed as a paramter to @ref ILocationManager::startDetailedReports or
 * @ref ILocationManager::startDetailedEngineReports.
 *
 * On platforms with Access control enabled, the client needs to have TELUX_LOC_DATA permission
 * for this listener API to be invoked.
 *
 * @param [in] ephemerisInfo - GNSS ephemeris information for a constellation.
 *
 */
  virtual void onGnssEphemerisInfo(const telux::loc::GnssEphemeris &ephemerisInfo){}

/**
 * This function is called when the capabilities of the location stack gets updated.
 *
 * On platforms with Access control enabled, the client needs to have TELUX_LOC_DATA permission
 * for this listener API to be invoked.
 *
 * @param [in] capabilityInfo - @ref telux::loc::LocCapability, capability information
 *
 */
  virtual void onCapabilitiesInfo(const telux::loc::LocCapability capabilityInfo) {}

/**
 * This function is called to provide the GNSS extended data
 * on products having applicable license enabled.
 * This data is generated only via the GNSS SPE engine. To receive these updates,
 * clients need to set the @ref telux::loc::GnssReportType::EXTENDED_DATA
 * bit in the reportMask passed as a parameter to @ref ILocationManager::startDetailedEngineReports.
 *
 * On platforms with Access control enabled, the client needs to have TELUX_LOC_DATA permission
 * for this listener API to be invoked.
 *
 * @param [in] payload - A blob of the GNSS extended data.
 *
 * @note  Eval: This is a new API and is being evaluated. It is subject to change
 *              and could break backwards compatibility.
 */
  virtual void onGnssExtendedDataInfo(const std::vector<uint8_t>& payload) {}

/**
 * Destructor of ILocationListener
 */
  virtual ~ILocationListener() {}
};

class ILocationSystemInfoListener : public telux::common::ISDKListener {
public:
/**
 * This function is called when device receives location related system information
 * such as leap second change.
 *
 * On platforms with Access control enabled, the client needs to have TELUX_LOC_DATA permission
 * for this listener API to be invoked.
 *
 * @param [in] locationSystemInfo - contains location system information such as
 *                                  current leap seconds change
 */
  virtual void onLocationSystemInfo(const LocationSystemInfo &locationSystemInfo) {}

/**
 * Destructor of ILocationSystemInfoListener
 */
  virtual ~ILocationSystemInfoListener() {}

};

/**
 * @brief ILocationConfigListener interface is used to receive notifications related to
 * configuration events.
 *
 * Clients can register for updates via @ref ILocationConfigurator::registerListener
 * by passing the list of indications present under @ref telux::loc::LocConfigIndicationsType.
 *
 * The listener method can be invoked from multiple different threads.
 * Client needs to make sure that implementation is thread-safe.
 */
class ILocationConfigListener : public telux::common::ISDKListener {
  public:
    /**
     * The API is invoked when there is any update in the Xtra assistance data.
     *
     * Clients need to register for this indication
     * via @ref LocConfigIndicationsType::LOC_CONF_IND_XTRA_STATUS.
     *
     * @param [in] xtraStatus - Xtra assistant data's current status, validity
     *                          and whether it is enabled.
     */
    virtual void onXtraStatusUpdate(const XtraStatus xtraStatus) {}

    /**
     * The API is invoked when there is any update in the Gnss Signal types supported by the modem.
     *
     * When @ref ILocationConfigurator::configureSecondaryBand is invoked,
     * the supported signals would be updated and notified via this listener API.
     *
     * Clients need to register for this indication
     * via @ref LocConfigIndicationsType::LOC_CONF_IND_SIGNAL_UPDATE.
     *
     * @param [in] gnssSignalMask - Bitset to represent the Gnss signal types supported
     *                              by the modem.
     *
     */
    virtual void onGnssSignalUpdate(const GnssSignal gnssSignalMask) {}

    virtual ~ILocationConfigListener() {}
};

/** @} */ /* end_addtogroup telematics_location */
}         // end of namespace loc

} // end of namespace telux

#endif // TELUX_LOC_LOCATIONLISTENER_HPP
