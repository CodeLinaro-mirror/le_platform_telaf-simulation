// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear
//
// libtelux_tel.so — simulated telux::tel subset.
//
// PhoneManager is declared immediately available so tafDataCallSvc's
// initPhoneManager() does not waste 90 seconds waiting. Full MQTT-driven
// readiness can be added once the Legato-process ↔ mosquitto connectivity
// issue is resolved (see ModemBridge / initDataServingSystemManagers timeout).
//
// TODO: replace with MQTT-driven implementation once ModemBridge connects.

#include <future>
#include <memory>
#include <telux/tel/PhoneFactory.hpp>
#include <telux/tel/PhoneManager.hpp>
#include <vector>

namespace telux {
namespace tel {

// ---------------------------------------------------------------------------
// PhoneManager stub
// ---------------------------------------------------------------------------
class PhoneManagerStub : public IPhoneManager
{
public:
    telux::common::ServiceStatus getServiceStatus() override
    {
        return telux::common::ServiceStatus::SERVICE_AVAILABLE;
    }
    bool isSubsystemReady() override
    {
        return true;
    }
    std::future<bool> onSubsystemReady() override
    {
        std::promise<bool> p;
        p.set_value(true);
        return p.get_future();
    }
    telux::common::Status getPhoneIds(std::vector<int>& phoneIds) override
    {
        phoneIds = { 1 };
        return telux::common::Status::SUCCESS;
    }
    int getPhoneIdFromSlotId(int slotId) override
    {
        return slotId;
    }
    int getSlotIdFromPhoneId(int phoneId) override
    {
        return phoneId;
    }
    std::shared_ptr<IPhone> getPhone(int) override
    {
        return nullptr;
    }
    telux::common::Status
    requestCellularCapabilityInfo(std::shared_ptr<ICellularCapabilityCallback>) override
    {
        return telux::common::Status::FAILED;
    }
    telux::common::Status requestOperatingMode(std::shared_ptr<IOperatingModeCallback>) override
    {
        return telux::common::Status::FAILED;
    }
    telux::common::Status setOperatingMode(OperatingMode, telux::common::ResponseCallback) override
    {
        return telux::common::Status::FAILED;
    }
    telux::common::Status resetWwan(telux::common::ResponseCallback) override
    {
        return telux::common::Status::FAILED;
    }
    telux::common::Status registerListener(std::weak_ptr<IPhoneListener>) override
    {
        return telux::common::Status::FAILED;
    }
    telux::common::Status removeListener(std::weak_ptr<IPhoneListener>) override
    {
        return telux::common::Status::FAILED;
    }
};

// ---------------------------------------------------------------------------
// PhoneFactory stub
// ---------------------------------------------------------------------------
class PhoneFactoryStub : public PhoneFactory
{
public:
    std::shared_ptr<IPhoneManager> getPhoneManager(telux::common::InitResponseCb) override
    {
        static auto mgr = std::make_shared<PhoneManagerStub>();
        return mgr;
    }
    std::shared_ptr<ISmsManager> getSmsManager(int, telux::common::InitResponseCb) override
    {
        return nullptr;
    }
    std::shared_ptr<ICallManager> getCallManager(telux::common::InitResponseCb) override
    {
        return nullptr;
    }
    std::shared_ptr<ICardManager> getCardManager(telux::common::InitResponseCb) override
    {
        return nullptr;
    }
    std::shared_ptr<ISapCardManager> getSapCardManager(int, telux::common::InitResponseCb) override
    {
        return nullptr;
    }
    std::shared_ptr<ISubscriptionManager> getSubscriptionManager(telux::common::InitResponseCb
    ) override
    {
        return nullptr;
    }
    std::shared_ptr<IServingSystemManager>
    getServingSystemManager(int, telux::common::InitResponseCb) override
    {
        return nullptr;
    }
    std::shared_ptr<INetworkSelectionManager>
    getNetworkSelectionManager(int, telux::common::InitResponseCb) override
    {
        return nullptr;
    }
    std::shared_ptr<IRemoteSimManager> getRemoteSimManager(int, telux::common::InitResponseCb)
      override
    {
        return nullptr;
    }
    std::shared_ptr<IMultiSimManager> getMultiSimManager(telux::common::InitResponseCb) override
    {
        return nullptr;
    }
    std::shared_ptr<ICellBroadcastManager>
    getCellBroadcastManager(SlotId, telux::common::InitResponseCb) override
    {
        return nullptr;
    }
    std::shared_ptr<ISimProfileManager> getSimProfileManager(telux::common::InitResponseCb) override
    {
        return nullptr;
    }
    std::shared_ptr<IImsSettingsManager> getImsSettingsManager(telux::common::InitResponseCb
    ) override
    {
        return nullptr;
    }
    std::shared_ptr<IEcallManager> getEcallManager(telux::common::InitResponseCb) override
    {
        return nullptr;
    }
    std::shared_ptr<IHttpTransactionManager> getHttpTransactionManager(telux::common::InitResponseCb
    ) override
    {
        return nullptr;
    }
    std::shared_ptr<IImsServingSystemManager>
    getImsServingSystemManager(SlotId, telux::common::InitResponseCb) override
    {
        return nullptr;
    }
    std::shared_ptr<ISuppServicesManager>
    getSuppServicesManager(SlotId, telux::common::InitResponseCb) override
    {
        return nullptr;
    }
    std::shared_ptr<IApSimProfileManager> getApSimProfileManager(telux::common::InitResponseCb
    ) override
    {
        return nullptr;
    }
};

PhoneFactory::PhoneFactory() = default;
PhoneFactory::~PhoneFactory() = default;

PhoneFactory&
PhoneFactory::getInstance()
{
    static PhoneFactoryStub instance;
    return instance;
}

}  // namespace tel
}  // namespace telux
