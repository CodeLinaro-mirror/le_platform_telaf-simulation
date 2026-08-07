// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear
//
// PhoneManager.hpp - SimulaPhoneManager, the MQTT-driven
// telux::tel::IPhoneManager implementation.


#ifndef TELUX_TEL_SIMULA_PHONE_MANAGER_HPP
#define TELUX_TEL_SIMULA_PHONE_MANAGER_HPP

#include "../common/IModemBridge.hpp"

#include <chart/active_object.hpp>
#include <map>
#include <memory>
#include <mutex>
#include <telux/tel/Phone.hpp>
#include <telux/tel/PhoneManager.hpp>
#include <vector>

namespace telux::tel::simula {

class SimulaPhone;

class SimulaPhoneManager final
    : public telux::tel::IPhoneManager
    , private chart::ActiveObject
{
public:
    explicit SimulaPhoneManager(
      common::simula::IModemBridge& bridge,
      telux::common::InitResponseCb initCb = nullptr
    );
    ~SimulaPhoneManager() override;

    SimulaPhoneManager(const SimulaPhoneManager&) = delete;
    SimulaPhoneManager& operator=(const SimulaPhoneManager&) = delete;

    void start();

    void setInitCallback(telux::common::InitResponseCb cb);

    // telux::tel::IPhoneManager
    bool isSubsystemReady() override;
    std::future<bool> onSubsystemReady() override;
    telux::common::ServiceStatus getServiceStatus() override;
    telux::common::Status getPhoneIds(std::vector<int>& phoneIds) override;
    int getPhoneIdFromSlotId(int slotId) override;
    int getSlotIdFromPhoneId(int phoneId) override;
    std::shared_ptr<telux::tel::IPhone> getPhone(int phoneId = DEFAULT_PHONE_ID) override;
    telux::common::Status requestCellularCapabilityInfo(
      std::shared_ptr<telux::tel::ICellularCapabilityCallback> callback = nullptr
    ) override;
    telux::common::Status requestOperatingMode(
      std::shared_ptr<telux::tel::IOperatingModeCallback> callback = nullptr
    ) override;
    telux::common::Status setOperatingMode(
      telux::tel::OperatingMode operatingMode,
      telux::common::ResponseCallback callback = nullptr
    ) override;
    telux::common::Status resetWwan(telux::common::ResponseCallback callback = nullptr) override;
    telux::common::Status registerListener(std::weak_ptr<telux::tel::IPhoneListener> listener) override;
    telux::common::Status removeListener(std::weak_ptr<telux::tel::IPhoneListener> listener) override;

private:
    friend chart::Status PhoneMgrNotReady_St(chart::Hsm*, chart::Event const*);
    friend chart::Status PhoneMgrReady_St(chart::Hsm*, chart::Event const*);

    void handleReadinessInd_(std::string_view topic, const common::simula::Envelope& env);
    void handleOpModeInd_(std::string_view topic, const common::simula::Envelope& env);
    void handleSignalStrengthInd_(std::string_view topic, const common::simula::Envelope& env);
    void handleCellInfoInd_(std::string_view topic, const common::simula::Envelope& env);
    void broadcastToListeners_(
      std::function<void(const std::shared_ptr<telux::tel::IPhoneListener>&)> invoke
    );
    bool isReadyDerived_() const;
    void publishStatus_(telux::common::ServiceStatus s);
    // Mirror of start()'s bridge registrations, ending in a drain() fence.
    // Called from the dtor before any member teardown -- see the dtor.
    void unsubscribeFromBridge_();

    common::simula::IModemBridge& bridge_;
    // Handle for the connectivity observer registered in start(), withdrawn
    // in the dtor. 0 until start() runs, which unsubscribe_connectivity
    // treats as a no-op.
    common::simula::IModemBridge::ConnectivityToken conn_token_{ 0 };
    // Every not-yet-notified init callback. A NotReady-period caller appends
    // here (SetInitCb_Signal, tel/PhoneManager.cpp); PhoneMgrReady_St's Entry
    // drains and fires all of them. AO-thread-only after construction, so no
    // separate mutex is needed (see setInitCallback()'s comment).
    std::vector<telux::common::InitResponseCb> init_cbs_;
    std::atomic<telux::common::ServiceStatus> last_status_{
        telux::common::ServiceStatus::SERVICE_UNAVAILABLE
    };

    std::mutex phones_mutex_;
    std::map<int, std::shared_ptr<SimulaPhone>> phones_;

    std::mutex listeners_mutex_;
    std::vector<std::weak_ptr<telux::tel::IPhoneListener>> listeners_;
};

}  // namespace telux::tel::simula

#endif  // TELUX_TEL_SIMULA_PHONE_MANAGER_HPP
