/*
 *  Copyright (c) 2017-2021, The Linux Foundation. All rights reserved.
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
 * @file       LocationManager.hpp
 * @brief      Location manager provides APIs to get position reports
 *             and satellite vehicle information updates. The reports
 *             specific to particular location engine can also be obtained
 *             by choosing the required engine report.
 *
 */

#ifndef TELUX_LOC_LOCATIONMANAGER_HPP
#define TELUX_LOC_LOCATIONMANAGER_HPP

#include <future>
#include <memory>

#include "telux/common/CommonDefines.hpp"
#include "telux/loc/LocationDefines.hpp"
#include "telux/loc/LocationListener.hpp"

namespace telux {

namespace loc {

/** @addtogroup telematics_location
* @{ */

/**
 * @brief ILocationManager provides interface to register and remove listeners.
 * It also allows to set and get configuration/ criteria for position reports.
 * The new APIs(registerListenerEx, deRegisterListenerEx, startDetailedReports,
 * startBasicReports) and old/deprecated APIs(registerListener, removeListener,
 * setPositionReportTimeout, setHorizontalAccuracyLevel, setMinIntervalForReports)
 * should not be used interchangebly, either the new APIs should be used or the
 * old APIs should be used.
 *
 */
class ILocationManager {
public:

/**
 * This function is called with the response to getEnergyConsumedInfoUpdate API.
 *
 * @param[in] energyConsumed - Information regarding energy consumed by Gnss engine.
 *
 * @param[in] error - Return code which indicates whether the operation succeeded
 *                    or not.
 *
 *
 */
 using GetEnergyConsumedCallback = std::function<void(telux::loc::GnssEnergyConsumedInfo
     energyConsumed, telux::common::ErrorCode error)>;

/**
 * This function is called with the response to getYearOfHw API.
 *
 * @param[in] yearOfHw - Year of hardware information.
 *
 * @param[in] error - Return code which indicates whether the operation succeeded
 *                    or not.
 *
 */
  using GetYearOfHwCallback = std::function<void(uint16_t yearOfHw,
      telux::common::ErrorCode error)>;

/**
 * This function is called with the response to getTerrestrialPosition API.
 *
 * @param[in] terrestrialInfo - basic position related information.
 *
 */
  using GetTerrestrialInfoCallback = std::function<void(
      const std::shared_ptr<ILocationInfoBase> terrestrialInfo)>;

/**
 * Checks the status of location subsystems and returns the result.
 *
 * @returns True if location subsystem is ready for service otherwise false.
 *
 * @deprecated use getServiceStatus()
 *
 */
  virtual bool isSubsystemReady() = 0;

/**
 * This status indicates whether the object is in a usable state.
 *
 * @returns SERVICE_AVAILABLE    -  If location manager is ready for service.
 *          SERVICE_UNAVAILABLE  -  If location manager is temporarily unavailable.
 *          SERVICE_FAILED       -  If location manager encountered an irrecoverable failure.
 *
 */
  virtual telux::common::ServiceStatus getServiceStatus() = 0;

/**
 * Wait for location subsystem to be ready.
 *
 * @returns  A future that caller can wait on to be notified when location
 *           subsystem is ready.
 *
 * @deprecated The callback mechanism introduced in the
 * @ref LocationFactory::getLocationManager() API will provide the similar notification
 * mechanism as onSubsystemReady(). This API will soon be removed from further releases.
 *
 */
  virtual std::future<bool> onSubsystemReady() = 0;

/**
 * Register a listener for specific updates from location manager like
 * location, jamming info and satellite vehicle info. If enhanced position,
 * using Dead Reckoning etc., is enabled, enhanced fixes will be provided.
 * Otherwise raw GNSS fixes will be provided.
 * The position reports will start only when startDetailedReports or
 * startBasicReports is invoked.
 *
 * @param [in] listener - Pointer of ILocationListener object that processes
 *                        the notification.
 *
 * @returns Status of registerListener i.e success or suitable status code.
 *
 *
 */
  virtual telux::common::Status
      registerListenerEx(std::weak_ptr<ILocationListener> listener) = 0;

/**
 * Remove a previously registered listener.
 *
 * @param [in] listener - Previously registered ILocationListener that needs
 *                        to be removed.
 *
 * @returns Status of removeListener success or suitable status code
 *
 */
  virtual telux::common::Status
      deRegisterListenerEx(std::weak_ptr<ILocationListener> listener) = 0;

/**
 * Starts the richer location reports by configuring the time between them as
 * the interval. The reports generated after invoking this API are from the FUSED Engine type.
 * Any of the 3 APIs that is startDetailedReports or startDetailedEngineReports
 * or startBasicReports can be called one after the other irrespective of order, without
 * calling stopReports in between any of them and the API which is called last will be honored
 * for providing the callbacks. In case of multiple clients invoking this API with different
 * intervals, if the platforms is configured, then the clients will receive the reports at their
 * requested intervals. If not configured then all the clients will be serviced at the smallest
 * interval among all clients' intervals.
 * The supported periodicities are 100ms, 200ms, 500ms, 1sec, 2sec, nsec and a periodicity that a
 * caller send which is not one of these will result in the implementation picking one of these
 * periodicities.
 * Calling this Api will result in @ref ILocationListener::onDetailedLocationUpdate,
 * @ref ILocationListener::onGnssSVInfo, @ref ILocationListener::onGnssSignalInfo,
 * @ref ILocationListener::onGnssNmeaInfo, @ref ILocationListener::onGnssMeasurementsInfo,
 * @ref ILocationListener::onGnssDisasterCrisisInfo and
 * @ref ILocationListener::onGnssEphemerisInfo APIs on the listener being invoked,
 * assuming they have not been disabled using the
 * GnssReportTypeMask. If a client issues second request to this API then new request for
 * GnssReportTypeMask will over write the previous call to this API.
 *
 * The NMEA sentences received can be configured via @ref ILocationConfigurator::configureNmea.
 *
 * On platforms with Access control enabled, caller needs to have TELUX_LOC_DATA permission to
 * invoke this API successfully.
 *
 * @param [in] interval - Minimum time interval between two consecutive
 * reports in milliseconds. The interval only controls the rate at which the PVT reports are
 * delivered to clients via @ref ILocationListener::onDetailedLocationUpdate.
 * The rate at which other report types are delivered are based on a platform configuration.
 *
 * E.g. If minInterval is 1000 milliseconds, reports will be provided with a
 * periodicity of 1 second or more depending on the number of applications
 * listening to location updates.
 *
 * @param [in] callback - Optional callback to get the response of set
 *             minimum interval for reports.
 *
 * @param [in] reportMask - Optional field to specify which reports a client is interested in.
 *                          By default all the reports will be enabled.
 *
 * @returns Status of startDetailedReports i.e. success or suitable status
 * code.
 *
 */
  virtual telux::common::Status startDetailedReports(uint32_t interval,
      telux::common::ResponseCallback callback = nullptr,
          GnssReportTypeMask reportMask = DEFAULT_GNSS_REPORT) = 0;

/**
 * Starts a session which may provide richer default combined position reports
 * and position reports from specific engines. The fused position report type will
 * always be supported if at least one engine in the system is producing valid report.
 * Any of the 3 APIs that is startDetailedReports or startDetailedEngineReports
 * or startBasicReports can be called one after the other irrespective of order, without
 * calling stopReports in between any of them and the API which is called last will be
 * honored for providing the callbacks. In case of multiple clients invoking this API with
 * different intervals, if the platforms is configured, then the clients will receive the reports
 * at their requested intervals. If not configured then all the clients will be serviced at the
 * smallest interval among all clients' intervals.
 * The supported periodicities are 100ms, 200ms, 500ms, 1sec, 2sec, nsec and a periodicity that a
 * caller send which is not one of these will result in the implementation picking one of these
 * periodicities.
 * Calling this Api will result in @ref ILocationListener::onDetailedEngineLocationUpdate,
 * @ref ILocationListener::onGnssSVInfo, @ref ILocationListener::onGnssSignalInfo,
 * @ref ILocationListener::onGnssNmeaInfo, @ref ILocationListener::onEngineNmeaInfo,
 * @ref ILocationListener::onGnssMeasurementsInfo,
 * @ref ILocationListener::onGnssDisasterCrisisInfo,
 * @ref ILocationListener::onGnssEphemerisInfo and
 * @ref ILocationListener::onGnssExtendedDataInfo APIs
 * on the listener being invoked, assuming they have not been disabled using the
 * GnssReportTypeMask. If a client issues second request to this API then new request for
 * GnssReportTypeMask will over write the previous call to this API.
 *
 * Usage of NMEA Sentences:
 *
 * NMEA updates can be received by either:
 * a) Setting the
 * @ref telux::loc::GnssReportType::NMEA bit in the reportMask passed as a paramter to
 * this API and receive the sentences via @ref ILocationListener::onGnssNmeaInfo.
 * b) Setting the
 * @ref telux::loc::GnssReportType::ENGINE_NMEA bit in the reportMask passed as a paramter to
 * this API and receive the sentences via @ref ILocationListener::onEngineNmeaInfo.
 *
 * When clients set the GnssReportType::NMEA bit in the reportMask passed as a parameter
 * to @ref ILocationManager::startDetailedEngineReports, the NMEA sentences are always
 * generated by FUSED engine and are received via @ref ILocationListener::onGnssNmeaInfo.
 * Clients can also configure the NMEA sentences to be received via
 * @ref ILocationConfigurator::configureNmea.
 *
 * Additionally, clients have the ability to receive NMEA sentences for a specific
 * engineType mask via @ref ILocationListener::onEngineNmeaInfo.
 * To receive the NMEA updates for a specific engine type, clients should configure the
 * NMEA sentences for the specific engineType mask via @ref ILocationConfigurator::configureNmea.
 * Next, while invoking @ref ILocationManager::startDetailedEngineReports, clients need to set the
 * @ref telux::loc::GnssReportType::ENGINE_NMEA bit in the reportMask.
 * The engineType mask passed as a parameter to this API should at least contain the
 * engines configured via the @ref ILocationConfigurator::configureNmea API.
 * The intended NMEA updates will be received via @ref ILocationListener::onEngineNmeaInfo.
 *
 * Understanding with an example -
 * Client configures a few NMEA sentences via @ref ILocationConfigurator::configureNmea
 * with the engineType mask set as PPE and FUSED. [Fused engine is always considered as set
 * even if the client does not explicitly specify it.]
 * Client invokes detailedEngineReports via @ref ILocationManager::startDetailedEngineReports
 * by setting the @ref telux::loc::GnssReportType::NMEA bit in the reportMask and
 * setting the engineType mask as just PPE.
 * Since both the engineType masks contain PPE, the NMEA updates generated by PPE will be received
 * via @ref ILocationListener::onEngineNmeaInfo. However, NMEA updates from FUSED engine will not be
 * received since the engine mask for this API is set to only PPE.
 *
 * If client sets the engineType mask as SPE while trying to configure NMEA sentences and sets
 * engineType mask as PPE while trying to invoke the detailed engine reports, no NMEA updates will
 * be received via @ref ILocationListener::onEngineNmeaInfo since there is NO intersection between
 * the 2 masks.
 *
 * On platforms with Access control enabled, caller needs to have TELUX_LOC_DATA permission to
 * invoke this API successfully.
 *
 * @param [in] interval - Minimum time interval between two consecutive
 * reports in milliseconds. The interval only controls the rate at which the PVT reports are
 * delivered to clients via @ref ILocationListener::onDetailedEngineLocationUpdate.
 * The rate at which other report types are delivered are based on a platform configuration.
 *
 * E.g. If minInterval is 1000 milliseconds, reports will be provided with a
 * periodicity of 1 second or more depending on the number of applications
 * listening to location updates.
 *
 * @param [in] engineType - The type of engine requested for fixes such as
 * SPE or PPE or FUSED. The FUSED includes all the engines that are running to
 * generate the fixes such as reports from SPE, PPE and DRE.
 *
 * @param [in] callback - Optional callback to get the response of set
 *             minimum interval for reports.
 *
 * @param [in] reportMask - Optional field to specify which reports a client is interested in.
 *                          By default all the reports will be enabled.
 *
 * @returns Status of startDetailedEngineReports i.e. success or suitable status
 * code.
 *
 */
  virtual telux::common::Status startDetailedEngineReports(uint32_t interval,
      LocReqEngine engineType, telux::common::ResponseCallback callback = nullptr,
          GnssReportTypeMask reportMask = DEFAULT_GNSS_REPORT) = 0;

/**
 * Starts the Location report by configuring the time between
 * the consecutive reports. Any of the 3 APIs that is startDetailedReports or
 * startDetailedEngineReports or startBasicReports can be called one after the other
 * irrespective of order, without calling stopReports in between any of them and the
 * API which is called last will be honored for providing the callbacks. In case of multiple
 * clients invoking this API with different intervals, if the platforms is configured, then the
 * clients will receive the reports at their requested intervals. If not configured then all the
 * clients will be serviced at the smallest interval among all clients' intervals.
 * The supported periodicities are 100ms, 200ms, 500ms, 1sec, 2sec, nsec and a periodicity that a
 * caller send which is not one of these will result in the implementation picking one of these
 * periodicities.
 * This Api enables the onBasicLocationUpdate Api on the listener. Please note that
 * these reports are generated by FUSED Engine type.
 *
 * On platforms with Access control enabled, caller needs to have TELUX_LOC_DATA permission to
 * invoke this API successfully.
 *
 * @param [in] intervalInMs - Minimum time interval between two consecutive reports in
 *                            milliseconds. The interval controls the rate at which the
 *                            PVT reports are delivered to clients via
 *                            @ref ILocationListener::onBasicLocationUpdate.
 *
 * @param [in] callback - Optional callback to get the response of set
 *                        minimum distance for reports.
 *
 * @returns Status of startBasicReports i.e. success or suitable status code.
 *
 */
  virtual telux::common::Status
      startBasicReports(uint32_t intervalInMs,
                        telux::common::ResponseCallback callback = nullptr) = 0;
/**
 * This API registers a ILocationSystemInfoListener listener and will receive information related
 * to location system that are not tied with location fix session, e.g.: next leap second event.
 * The startBasicReports, startDetailedReports, startDetailedEngineReports does not need to be
 * called before calling this API, in order to receive updates.
 *
 * @param [in] listener - Pointer of ILocationSystemInfoListener object.
 *
 * @param [in] callback - Optional callback to get the response of location
 *                        system info.
 *
 * @returns Status of getLocationSystemInfo i.e success or suitable status code.
 *
 *
 */
  virtual telux::common::Status
      registerForSystemInfoUpdates(std::weak_ptr<ILocationSystemInfoListener> listener,
          telux::common::ResponseCallback callback = nullptr) = 0;

/**
 * This API removes a previously registered listener and will also stop receiving informations
 * related to location system for that particular listener.
 *
 * @param [in] listener - Previously registered ILocationSystemInfoListener that needs to be
 *                        removed.
 *
 * @param [in] callback - Optional callback to get the response of location
 *                        system info.
 *
 * @returns Status of deRegisterForSystemInfoUpdates success or suitable status code.
 *
 *
 */
  virtual telux::common::Status
      deRegisterForSystemInfoUpdates(std::weak_ptr<ILocationSystemInfoListener> listener,
      telux::common::ResponseCallback callback = nullptr) = 0;

/**
 * This API receives information on energy consumed by modem GNSS engine. If this API
 * is called on this object while this is already a pending request, then it will overwrite
 * the callback to be invoked and the callback from the previous invocation will not be
 * called.
 *
 * @param [in] cb - callback to get the information of Gnss energy consumed.
 *
 * @returns Status of requestEnergyConsumedInfo i.e success or suitable status code.
 *
 */
  virtual telux::common::Status requestEnergyConsumedInfo(GetEnergyConsumedCallback cb) = 0;

/**
 * This API will stop reports started using startDetailedReports or startBasicReports
 * or registerListener or setMinIntervalForReports.
 *
 * On platforms with Access control enabled, caller needs to have TELUX_LOC_DATA permission to
 * invoke this API successfully.
 *
 * @param [in] callback - Optional callback to get the response of stop reports.
 *
 * @returns Status of stopReports i.e. success or suitable status code.
 *
 */
  virtual telux::common::Status
      stopReports(telux::common::ResponseCallback callback = nullptr) = 0;

/**
 * This API retrieves the year of hardware information.
 *
 * @param[in] cb - callback to get information of year of hardware.
 *
 * @returns Status of getYearOfHw i.e success or suitable status code.
 *
 */
  virtual telux::common::Status getYearOfHw(GetYearOfHwCallback cb) = 0;

/**
 * This API retrieves single-shot terrestrial position using the set of specified terrestrial
 * technologies.
 * This API can be invoked even while there is an on-going tracking session that was started using
 * startBasicReports/startDetailedReports/startDetailedEngineReports.
 * If this API is invoked while there is already a pending request for terrestrial position, the
 * request will fail and @ref telux::common::ResponseCallback will get invoked with @ref
 * telux::common::ErrorCode::OP_IN_PROGRESS.
 * To cancel a pending request, use @ref ILocationManager::cancelTerrestrialPositionRequest.
 * Before using this API, user consent needs to be set true via
 * @ref ILocationConfigurator::provideConsentForTerrestrialPositioning.
 *
 * On platforms with Access control enabled, caller needs to have TELUX_LOC_DATA permission to
 * invoke this API successfully.
 *
 * @param[in] timeoutMsec - the time in milliseconds within which the client is expecting a
 *                          response. If the system is unable to provide a report within this
 *                          time, the @ref telux::common::ResponseCallback will be invoked with
 *                          @ref telux::common::ErrorCode::OPERATION_TIMEOUT.
 *
 * @param[in] techMask - the set of terrestrial technologies that are allowed to be used for
 *                       producing the position.
 *
 * @param[in] cb - callback to receive terrestrial position. This callback will only
 *                 be invoked when ResponseCallback is invoked with SUCCESS.
 *
 * @param [in] callback - Optional callback to get the response of getTerrestrialPosition.
 *
 * @returns Status of getTerrestrialPosition i.e success or suitable status code.
 *
 */
  virtual telux::common::Status getTerrestrialPosition(uint32_t timeoutMsec,
      TerrestrialTechnology techMask, GetTerrestrialInfoCallback cb, telux::common::
          ResponseCallback callback = nullptr) = 0;

/**
 * This API cancels the pending request invoked by @ref ILocationManager::getTerrestrialPosition.
 * If this API is invoked while there is no pending request for terrestrial position from
 * @ref ILocationManager::getTerrestrialPosition, then @ref telux::common::ResponseCallback will be invoked
 * with @ref telux::common::ErrorCode::INVALID_ARGUMENTS.
 *
 * On platforms with Access control enabled, caller needs to have TELUX_LOC_DATA permission to
 * invoke this API successfully.
 *
 * @param [in] callback - Optional callback to get the response of
 *                        cancelTerrestrialPositionRequest.
 *
 * @returns Status of cancelTerrestrialPositionRequest i.e success or suitable status code.
 *
 */
  virtual telux::common::Status cancelTerrestrialPositionRequest(telux::common::
      ResponseCallback callback = nullptr) = 0;

/**
 * This API retrieves capability information.
 *
 * @returns Status of getCapabilities i.e success or suitable status code.
 *
 *
 */
  virtual telux::loc::LocCapability getCapabilities() = 0;


/**
 * Destructor of ILocationManager
 */
  virtual ~ILocationManager() {}
  ;

/**
 * Starts the Location report by configuring the time and distance between
 * the consecutive reports. Any of the 3 APIs that is startDetailedReports or
 * startDetailedEngineReports or startBasicReports can be called one after the other
 * irrespective of order, without calling stopReports in between any of them and the
 * API which is called last will be honored for providing the callbacks. In case of multiple
 * clients invoking this API with different intervals, if the platforms is configured, then the
 * clients will receive the reports at their requested intervals. If not configured then all the
 * clients will be serviced at the smallest interval among all clients' intervals.
 * The supported periodicities are 100ms, 200ms, 500ms, 1sec, 2sec, nsec and a periodicity that a
 * caller send which is not one of these will result in the implementation picking one of these
 * periodicities.
 * This Api enables the onBasicLocationUpdate Api on the listener. Please note that
 * these reports are generated by FUSED Engine type.
 *
 *
 * On platforms with Access control enabled, caller needs to have TELUX_LOC_DATA permission to
 * invoke this API successfully.
 *
 * E.g. If intervalInMs is 1000 milliseconds and distanceInMeters is 100m,
 * reports will be provided according to the condition that happens first. So we need to
 * provide both the parameters for evaluating the report.
 *
 * The underlying system may have a minimum distance threshold(e.g. 1 meter).
 * Effective distance will not be smaller than this lower bound.
 *
 * The effective distance may have a granularity level higher than 1 m, e.g.
 * 5 m. So distanceInMeters being 59 may be honored at 60 m, depending on the system.
 *
 * Where there is another application in the system having a session with
 * shorter distance, this client may benefit and receive reports at that distance.
 *
 * @param [in] distanceInMeters - DistanceInMeters between two consecutive reports in meters.
 *                                This parameter is not used.
 *
 * @param [in] intervalInMs - Minimum time interval between two consecutive reports in
 *                            milliseconds. The interval controls the rate at which the
 *                            PVT reports are delivered to clients via
 *                            @ref ILocationListener::onBasicLocationUpdate.
 *
 * @param [in] callback - Optional callback to get the response of set
 *                        minimum distance for reports.
 *
 * @returns Status of startBasicReports i.e. success or suitable status code.
 *
 * @deprecated This API which takes distance as an argument is not supported anymore.
 * Use @ref ILocationManager::startBasicReports(uint32_t intervalInMs,
 *  telux::common::ResponseCallback callback = nullptr) = 0;
 *
 */
  virtual telux::common::Status
      startBasicReports(uint32_t distanceInMeters, uint32_t intervalInMs,
                        telux::common::ResponseCallback callback = nullptr) = 0;

};
/** @} */ /* end_addtogroup telematics_location */

} // end of namespace loc

} // end of namespace telux

#endif // TELUX_LOC_LOCATIONMANAGER_HPP
