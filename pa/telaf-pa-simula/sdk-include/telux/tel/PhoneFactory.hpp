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
 * @file       PhoneFactory.hpp
 * @brief      PhoneFactory is the central factory to create all Telephony SDK
 *             classes and services.
 */

#ifndef TELUX_TEL_PHONEFACTORY_HPP
#define TELUX_TEL_PHONEFACTORY_HPP

#include <memory>

#include <telux/tel/CallManager.hpp>
#include <telux/tel/CellBroadcastManager.hpp>
#include <telux/tel/CardManager.hpp>
#include <telux/tel/NetworkSelectionManager.hpp>
#include <telux/tel/Phone.hpp>
#include <telux/tel/PhoneManager.hpp>
#include <telux/tel/RemoteSimManager.hpp>
#include <telux/tel/SapCardManager.hpp>
#include <telux/tel/ServingSystemManager.hpp>
#include <telux/tel/SmsManager.hpp>
#include <telux/tel/SubscriptionManager.hpp>
#include <telux/tel/MultiSimManager.hpp>
#include <telux/tel/SimProfileManager.hpp>
#include <telux/tel/ImsSettingsManager.hpp>
#include <telux/tel/HttpTransactionManager.hpp>
#include <telux/tel/ImsServingSystemManager.hpp>
#include <telux/tel/SuppServicesManager.hpp>
#include <telux/tel/EcallManager.hpp>
#include <telux/tel/ApSimProfileManager.hpp>

namespace telux {

namespace tel {

/** @addtogroup telematics_phone
 * @{ */

/**
 * @brief PhoneFactory is the central factory to create all Telephony SDK Classes
 *        and services
 */
class PhoneFactory {
 public:
   /**
    * Get Phone Factory instance.
    */
   static PhoneFactory &getInstance();

   /**
    * Get Phone Manager instance. Phone Manager is the main entry point into the
    * telephony subsystem.
    * @param [in] callback  Optional callback pointer to get response of Phone Manager
    *                       initialization.
    *                       It will be invoked when initialization is either succeeded or failed.
    *                       In case of failure response, the provided Phone Manager object will no
    *                       more be a valid object.
    *
    * @returns Pointer of IPhoneManager object.
    */
   virtual std::shared_ptr<IPhoneManager> getPhoneManager(
      telux::common::InitResponseCb callback = nullptr) = 0;

   /**
    * Get SMS Manager instance for Phone ID. SMSManager used to send and receive
    * SMS messages.
    *
    * @param [in] phoneId   Unique identifier for the phone
    * @param [in] callback  Optional callback pointer to get response of SMS Manager initialization.
    *                       It will be invoked when initialization is either succeeded or failed.
    *                       In case of failure response, the provided SMS Manager object will no
    *                       more be a valid object.
    *
    * @returns Pointer of ISmsManager object or nullptr in case of failure.
    */
   virtual std::shared_ptr<ISmsManager> getSmsManager(int phoneId = DEFAULT_PHONE_ID,
      telux::common::InitResponseCb callback = nullptr) = 0;

   /**
    * Get Call Manager instance to determine state of active calls and perform
    * other functions like dial, conference, swap call.
    *
    * @param [in] callback  Optional callback pointer to get response of CallManager
    *                       initialization.
    *                       It will be invoked when initialization is either succeeded or failed.
    *                       In case of failure response, the provided Call Manager object will no
    *                       more be a valid object.
    * @returns Pointer of ICallManager object or nullptr in case of failure.
    */
   virtual std::shared_ptr<ICallManager> getCallManager(telux::common::InitResponseCb
      callback = nullptr) = 0;

   /**
    * Get Card Manager instance to handle services such as transmitting APDU,
    * SIM IO and more.
    * @param [in] callback  Optional callback pointer to get response of Card Manager
    *                       initialization.
    *                       It will be invoked when initialization is either succeeded or failed.
    *                       In case of failure response, the provided Phone Manager object will no
    *                       more be a valid object.
    *
    * @returns Pointer of ICardManager object.
    */
   virtual std::shared_ptr<ICardManager> getCardManager(
       telux::common::InitResponseCb callback = nullptr) = 0;

   /**
    * Get Sap Card Manager instance associated with the provided slot id. This
    * object will handle services in SAP mode such as APDU, SIM Power On/Off
    * and SIM reset.
    *
    * On platforms with access control enabled, caller needs to have TELUX_TEL_SAP permission to
    * invoke this API successfully.
    *
    * @param [in] slotId    Unique identifier for the SIM slot
    * @param[in] callback   Optional callback pointer to get the response of the manager
    *                       initialisation.
    *
    * @returns Pointer of ISapCardManager object.
    */
   virtual std::shared_ptr<ISapCardManager> getSapCardManager(
      int slotId = DEFAULT_SLOT_ID, telux::common::InitResponseCb callback = nullptr) = 0;

   /**
    * Get Subscription Manager instance to get device subscription details
    *
    * @param [in] callback  Optional callback pointer to get response of Phone Manager
    *                       initialization.
    *                       It will be invoked when initialization is either succeeded or failed.
    *                       In case of failure response, the provided SubscriptionManager object
    *                       will no more be a valid object.
    *
    * @returns Pointer of ISubscriptionManager object.
    *
    */
   virtual std::shared_ptr<ISubscriptionManager> getSubscriptionManager(
      telux::common::InitResponseCb callback = nullptr) = 0;

   /**
    * Get Serving System Manager instance to get and set preferred network type.
    *
    * @param [in] slotId    Unique identifier for the SIM slot
    * @param[in] callback   Optional callback pointer to get the response of the manager
    *                       initialisation.
    *
    * @returns Pointer of IServingSystemManager object.
    */
   virtual std::shared_ptr<IServingSystemManager> getServingSystemManager(
      int slotId = DEFAULT_SLOT_ID, telux::common::InitResponseCb callback = nullptr) = 0;

   /**
    * Get Network Selection Manager instance to get and set selection mode, get
    * and set preferred networks and scan available networks.
    *
    * @param [in] slotId    Unique identifier for the SIM slot
    * @param[in] callback   Optional callback pointer to get the response of the manager
    *                       initialisation.
    *
    * @returns Pointer of INetworkSelectionManager object.
    */
   virtual std::shared_ptr<INetworkSelectionManager> getNetworkSelectionManager(
      int slotId = DEFAULT_SLOT_ID, telux::common::InitResponseCb  callback = nullptr) = 0;

   /**
    * Get Remote SIM Manager instance to handle services like exchanging APDU,
    * SIM Power On/Off, etc.
    *
    * On platforms with access control enabled, caller needs to have TELUX_TEL_REMOTE_SIM permission
    * to invoke this API successfully.
    *
    * @param [in] slotId    Unique identifier for the SIM slot
    * @param [in] callback  Optional callback pointer to get response of RemoteSim Manager
    *                       initialization.
    *                       It will be invoked when initialization is either succeeded or failed.
    *                       In case of failure response, the provided Phone Manager object will no
    *                       more be a valid object.
    *
    * @returns Pointer of IRemoteSimManager object.
    */
   virtual std::shared_ptr<IRemoteSimManager> getRemoteSimManager(int slotId = DEFAULT_SLOT_ID,
       telux::common::InitResponseCb  callback = nullptr ) = 0;

   /**
    * Get Multi SIM Manager instance to handle operations like high capabilty
    * switch.
    * @param [in] callback  Optional callback pointer to get response of MultiSimManager
    *                       initialization.
    *                       It will be invoked when initialization is either succeeded or failed.
    *                       In case of failure response, the provided MultiSimManager object will no
    *                       more be a valid object.
    * @returns Pointer of IMultiSimManager object.
    *
    */
   virtual std::shared_ptr<IMultiSimManager> getMultiSimManager(
      telux::common::InitResponseCb callback = nullptr) = 0;

   /**
    * Get CellBroadcast Manager instance for Slot ID. CellBroadcast manager used to receive
    * broacast messages and configure broadcast messages.
    *
    * @param [in] slotId   @ref telux::common::SlotId
    * @param [in] callback  Optional callback pointer to get response of CellBroadcast Manager
    *                       initialization.
    *                       It will be invoked when initialization is either succeeded or failed.
    *                       In case of failure response, the provided Phone Manager object will no
    *                       more be a valid object.
    *
    * @returns Pointer of ICellBroadcastManager object or nullptr in case of failure.
    */
   virtual std::shared_ptr<ICellBroadcastManager> getCellBroadcastManager(
      SlotId slotId = DEFAULT_SLOT_ID, telux::common::InitResponseCb  callback = nullptr) = 0;

   /**
    * Get SimProfileManager. SimProfileManager is a primary interface for remote
    * eUICC(eSIM) provisioning and local profile assistance.
    * @param [in] callback  Optional callback pointer to get response of SimProfile Manager
    *                       initialization.
    *                       It will be invoked when initialization is either succeeded or failed.
    *                       In case of failure response, the provided Phone Manager object will no
    *                       more be a valid object.
    *
    * @returns Pointer of ISimProfileManager object or nullptr in case of failure.
    *
    */
   virtual std::shared_ptr<ISimProfileManager> getSimProfileManager(
       telux::common::InitResponseCb  callback = nullptr) = 0;

   /**
    * Get Ims Settings Manager instance to handle IMS service enable configuation parameters like
    * enable/disable voIMS.
    *
    * @param[in] callback   Optional callback pointer to get the response of the manager
    *                       initialisation.
    *
    * @returns Pointer of IImsSettingsManager object.
    *
    */
   virtual std::shared_ptr<IImsSettingsManager> getImsSettingsManager(
       telux::common::InitResponseCb  callback = nullptr) = 0;

   /*
    * Get Ecall Manager instance to change eCall related configuration
    *
    * In a system where access control is enabled for SDK APIs, the client needs to have necessary
    * permission to successfully execute this API.
    *
    * @param [in] callback  Optional client callback to get the initialization status of
    *                       IEcallManager
    *                       @ref telux::common::InitResponseCb
    *
    * @returns Pointer of IEcallManager object or nullptr in case of failure.
    *
    * @deprecated This API is not being supported
    */
   virtual std::shared_ptr<IEcallManager> getEcallManager(
       telux::common::InitResponseCb callback = nullptr) = 0;

   /**
    * Get HttpTransactionManager instance to handle HTTP related requests
    * from the modem for SIM profile update related operations.
    *
    * @param[in] callback   Optional callback pointer to get the response of the manager
    *                       initialisation.
    *
    * @returns Pointer of IHttpTransactionManager object or nullptr in case of failure.
    *
    */
   virtual std::shared_ptr<IHttpTransactionManager> getHttpTransactionManager(
      telux::common::InitResponseCb  callback = nullptr) = 0;

   /**
    * Get IMS Serving System Manager instance to query IMS registration status
    *
    * @returns Pointer of IImsServingSystemManager object or nullptr in case of failure.
    *
    */
   virtual std::shared_ptr<IImsServingSystemManager> getImsServingSystemManager(SlotId slotId,
      telux::common::InitResponseCb callback = nullptr) = 0;

   /**
    * Get Supplementary service manager instance to set/get preference for supplementary services
    * like call waiting, call forwarding etc.
    *
    * @param [in] SlotId     @ref telux::common::SlotId
    * @param [in] callback   Optional callback pointer to get the response of the manager
    *                        initialisation.
    *
    * @returns Pointer of ISuppServicesManager object.
    *
    */
   virtual std::shared_ptr<ISuppServicesManager> getSuppServicesManager(
      SlotId slotId = DEFAULT_SLOT_ID, telux::common::InitResponseCb  callback = nullptr) = 0;

   /**
    * Gets ApSimProfileManager. ApSimProfileManager is the primary interface to allow the modem
    * software to interact with an LPA running on the Application process.
    *
    * @param [in] callback  Optional callback pointer to get response of ApSimProfile Manager
    *                       initialization.
    *                       It will be invoked when initialization is either succeeded or failed.
    *                       In case of failure response, the provided ApSimProfile Manager object
    *                       will no more be a valid object.
    *
    * @returns Pointer of IApSimProfileManager object or nullptr in case of failure.
    *
    * @note   Eval: This is a new API and is being evaluated. It is subject to change and
    *         could break backwards compatibility.
    */
   virtual std::shared_ptr<IApSimProfileManager> getApSimProfileManager(
       telux::common::InitResponseCb  callback = nullptr) = 0;

#ifndef TELUX_DOXY_SKIP
 protected:
   PhoneFactory();
   virtual ~PhoneFactory();
#endif

 private:
   PhoneFactory(const PhoneFactory &) = delete;
   PhoneFactory &operator=(const PhoneFactory &) = delete;
};

/** @} */ /* end_addtogroup telematics_phone */

}  // End of namespace tel

}  // End of namespace telux

#endif // TELUX_TEL_PHONEFACTORY_HPP
