/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef MULTISIMMANAGERIMPLBYPASS_HPP
#define MULTISIMMANAGERIMPLBYPASS_HPP

#include <future>

#include <telux/tel/MultiSimManager.hpp>
#include <telux/common/CommonDefines.hpp>

#include "common/AsyncTaskQueue.hpp"
#include "common/ListenerManager.hpp"

#define INVALID_SLOT_COUNT -1
namespace telux {
namespace tel {

struct MultiSimUserData {
   int cmdCallbackId;
};

class MultiSimManagerBypass : public IMultiSimManager,
                              public std::enable_shared_from_this<MultiSimManagerBypass> {

public:
   MultiSimManagerBypass();

   ~MultiSimManagerBypass();

   bool isSubsystemReady() override;

   std::future<bool> onSubsystemReady() override;

   telux::common::Status init(telux::common::InitResponseCb callback);

   telux::common::ServiceStatus getServiceStatus() override;

   void cleanup();

   void updateReady();

   telux::common::Status getSlotCount(int &count) override;

   telux::common::Status requestHighCapability(HighCapabilityCallback callback) override;

   telux::common::Status setHighCapability(int slotId,
      common::ResponseCallback callback) override;

   telux::common::Status switchActiveSlot(SlotId slotId,
      common::ResponseCallback callback = nullptr) override;

   telux::common::Status requestSlotStatus(SlotStatusCallback callback) override;

   telux::common::Status registerListener(std::weak_ptr<IMultiSimListener> listener) override;

   telux::common::Status deregisterListener(std::weak_ptr<IMultiSimListener> listener) override;

private:
   std::shared_ptr<telux::common::ListenerManager<IMultiSimListener>> listenerMgr_;
   std::mutex mtx_;
   std::condition_variable cv_;
   telux::common::ServiceStatus lastReportedSvcState_
      = telux::common::ServiceStatus::SERVICE_AVAILABLE;
   telux::common::ServiceStatus subSystemStatus_
      = telux::common::ServiceStatus::SERVICE_UNAVAILABLE;

   int slotCount_ = INVALID_SLOT_COUNT;
   telux::common::AsyncTaskQueue<void> taskQ_;
   telux::common::InitResponseCb initCb_;

   // Singleton instance. MultiSimManagerBypass instance will be returned by PhoneFactory
   MultiSimManagerBypass(const MultiSimManagerBypass &) = delete;
   MultiSimManagerBypass &operator=(const MultiSimManagerBypass &) = delete;

   bool waitForInitialization();

   void initSync();

   void setServiceStatus(telux::common::ServiceStatus status);

};

}  // end of namespace tel
}
#endif
