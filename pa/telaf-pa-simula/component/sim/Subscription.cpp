// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "Subscription.hpp"

namespace telux::tel::simula {

SimulaSubscription::SimulaSubscription(int slotId, std::string iccid, std::string imsi)
    : slotId_(slotId)
    , iccid_(std::move(iccid))
    , imsi_(std::move(imsi))
{}

std::string
SimulaSubscription::getCarrierName()
{
    return "";
}

std::string
SimulaSubscription::getIccId()
{
    std::lock_guard<std::mutex> lk(m_);
    return iccid_;
}

int
SimulaSubscription::getMcc()
{
    return 0;
}

int
SimulaSubscription::getMnc()
{
    return 0;
}

std::string
SimulaSubscription::getMobileCountryCode()
{
    return "";
}

std::string
SimulaSubscription::getMobileNetworkCode()
{
    return "";
}

std::string
SimulaSubscription::getPhoneNumber()
{
    return "";
}

int
SimulaSubscription::getSlotId()
{
    return slotId_;
}

std::string
SimulaSubscription::getImsi()
{
    std::lock_guard<std::mutex> lk(m_);
    return imsi_;
}

std::string
SimulaSubscription::getGID1()
{
    return "";
}

std::string
SimulaSubscription::getGID2()
{
    return "";
}

void
SimulaSubscription::setIds(std::string iccid, std::string imsi)
{
    std::lock_guard<std::mutex> lk(m_);
    iccid_ = std::move(iccid);
    imsi_ = std::move(imsi);
}

}  // namespace telux::tel::simula
