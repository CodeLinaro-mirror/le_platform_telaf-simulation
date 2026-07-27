// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear
// Thread-safe simulated subscription value object.

#ifndef TELUX_TEL_SIMULA_SUBSCRIPTION_HPP
#define TELUX_TEL_SIMULA_SUBSCRIPTION_HPP

#include <mutex>
#include <string>
#include <telux/tel/Subscription.hpp>

namespace telux::tel::simula {

class SimulaSubscription final : public telux::tel::ISubscription
{
public:
    SimulaSubscription(int slotId, std::string iccid, std::string imsi);

    // telux::tel::ISubscription
    std::string getCarrierName() override;
    std::string getIccId() override;
    int getMcc() override;
    int getMnc() override;
    std::string getMobileCountryCode() override;
    std::string getMobileNetworkCode() override;
    std::string getPhoneNumber() override;
    int getSlotId() override;
    std::string getImsi() override;
    std::string getGID1() override;
    std::string getGID2() override;

    // Called only from the owning manager's AO thread.
    void setIds(std::string iccid, std::string imsi);

private:
    mutable std::mutex m_;
    int slotId_;
    std::string iccid_;
    std::string imsi_;
};

}  // namespace telux::tel::simula

#endif  // TELUX_TEL_SIMULA_SUBSCRIPTION_HPP
