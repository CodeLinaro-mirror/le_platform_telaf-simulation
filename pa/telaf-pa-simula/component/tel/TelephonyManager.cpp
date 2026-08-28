// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear
//
// libtelux_tel.so — simulated telux::tel subset.


#include "../sim/SimFactory.hpp"


#include "NetworkSelectionManager.hpp"
#include "PhoneManager.hpp"
#include "ServingSystemManager.hpp"
#include "CallManager.hpp"

#include "../common/ListenerDispatchAO.hpp"
#include "../common/Log.hpp"
#include "../common/ModemBridge.hpp"

#include <map>
#include <memory>
#include <mutex>
#include <future>
#include <chrono>
#include <telux/tel/PhoneFactory.hpp>
#include <telux/tel/PhoneManager.hpp>
#include <vector>

namespace telux {
namespace tel {

// ---------------------------------------------------------------------------
// PhoneFactory stub -- owns the shared bridge + lazily-constructed managers.
// ---------------------------------------------------------------------------
class PhoneFactoryStub : public PhoneFactory
{
public:
    PhoneFactoryStub()
        : bridge_(common::simula::ModemBridge::instance())
    {
        bridge_.start();
        common::simula::ListenerDispatchAO::instance().start();
    }

    std::shared_ptr<IPhoneManager> getPhoneManager(telux::common::InitResponseCb callback) override
    {
        std::shared_ptr<simula::SimulaPhoneManager> mgr;
        {
            std::lock_guard<std::mutex> lk(managers_mutex_);
            if (phone_manager_)
            {
                if (callback)
                    phone_manager_->setInitCallback(std::move(callback));
                return phone_manager_;
            }
            phone_manager_ = std::make_shared<simula::SimulaPhoneManager>(bridge_, nullptr);
            phone_manager_->start();
            mgr = phone_manager_;
        }
        auto promise = std::make_shared<std::promise<void>>();
        auto future  = promise->get_future();
        mgr->setInitCallback([promise](telux::common::ServiceStatus) {
            promise->set_value();
        });
        if (future.wait_for(std::chrono::seconds(30)) == std::future_status::timeout)
            LOG_WARN("[TelephonyManager] getPhoneManager() timed out waiting for Ready");
        if (callback)
            mgr->setInitCallback(std::move(callback));
        return mgr;
    }
    std::shared_ptr<ISmsManager> getSmsManager(int, telux::common::InitResponseCb) override
    {
        return nullptr;
    }
    std::shared_ptr<ICallManager> getCallManager(telux::common::InitResponseCb callback) override
    {
        std::shared_ptr<simula::SimulaCallManager> mgr;
        {
            std::lock_guard<std::mutex> lk(managers_mutex_);
            if (call_manager_)
            {
                // See getPhoneManager(): never drop the caller's InitResponseCb.
                if (callback)
                    call_manager_->setInitCallback(std::move(callback));
                return call_manager_;
            }
            call_manager_ = std::make_shared<simula::SimulaCallManager>(bridge_, nullptr);
            mgr = call_manager_;
        }
        // Timeout: 30 s matches the SML subsys_ready timeout used elsewhere.
        auto promise = std::make_shared<std::promise<void>>();
        auto future  = promise->get_future();
        mgr->setInitCallback([promise](telux::common::ServiceStatus) {
            promise->set_value();
        });
        if (future.wait_for(std::chrono::seconds(30)) == std::future_status::timeout)
            LOG_WARN("[TelephonyManager] getCallManager() timed out waiting for Ready");
        if (callback)
            mgr->setInitCallback(std::move(callback));
        return mgr;
    }
    std::shared_ptr<ICardManager> getCardManager(telux::common::InitResponseCb cb) override
    {
        return simFactory_.getCardManager(std::move(cb));
    }
    std::shared_ptr<ISapCardManager> getSapCardManager(int, telux::common::InitResponseCb) override
    {
        return nullptr;
    }
    std::shared_ptr<ISubscriptionManager> getSubscriptionManager(telux::common::InitResponseCb cb
    ) override
    {
        return simFactory_.getSubscriptionManager(std::move(cb));
    }
    std::shared_ptr<IServingSystemManager>
    getServingSystemManager(int slotId, telux::common::InitResponseCb callback) override
    {
        std::lock_guard<std::mutex> lk(managers_mutex_);
        auto it = serving_systems_.find(slotId);
        if (it != serving_systems_.end())
        {
            // See getPhoneManager(): never drop the caller's InitResponseCb.
            if (callback)
                it->second->setInitCallback(std::move(callback));
            return it->second;
        }
        auto mgr = std::make_shared<simula::SimulaTelServingSystemManager>(
          slotId, bridge_, std::move(callback)
        );
        serving_systems_.emplace(slotId, mgr);
        mgr->start();
        return mgr;
    }
    std::shared_ptr<INetworkSelectionManager>
    getNetworkSelectionManager(int slotId, telux::common::InitResponseCb callback) override
    {
        std::lock_guard<std::mutex> lk(managers_mutex_);
        auto it = network_selections_.find(slotId);
        if (it != network_selections_.end())
        {
            // See getPhoneManager(): never drop the caller's InitResponseCb.
            if (callback)
                it->second->setInitCallback(std::move(callback));
            return it->second;
        }
        auto mgr = std::make_shared<simula::SimulaNetworkSelectionManager>(
          slotId, bridge_, std::move(callback)
        );
        network_selections_.emplace(slotId, mgr);
        mgr->start();
        return mgr;
    }
    std::shared_ptr<IRemoteSimManager> getRemoteSimManager(int, telux::common::InitResponseCb)
      override
    {
        return nullptr;
    }
    std::shared_ptr<IMultiSimManager> getMultiSimManager(telux::common::InitResponseCb cb) override
    {
        return simFactory_.getMultiSimManager(std::move(cb));
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

private:
    telux::tel::simula::SimulaSimFactory simFactory_;
    common::simula::IModemBridge& bridge_;

    std::mutex managers_mutex_;
    std::shared_ptr<simula::SimulaPhoneManager> phone_manager_;
    std::map<int, std::shared_ptr<simula::SimulaTelServingSystemManager>> serving_systems_;
    std::map<int, std::shared_ptr<simula::SimulaNetworkSelectionManager>> network_selections_;
    std::shared_ptr<simula::SimulaCallManager> call_manager_;
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
