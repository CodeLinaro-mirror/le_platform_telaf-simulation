/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "DeviceInfoManager.hpp"
#include "telux/platform/PlatformFactory.hpp"
#include "telux/platform/FsManager.hpp"
#include "telux/platform/TimeManager.hpp"
#include "telux/platform/hardware/AntennaManager.hpp"
#include "Log.hpp"

#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>

#include <cstring>
#include <string>
#include <fstream>
#include <cerrno>
#include <algorithm>
#include <sys/stat.h>

#define DEVICEINFO_JSON_FILE "/tmp/IDeviceInfoManager.json"
#define DEFAULT_IMEI "000000000000000"
#define IMEI_LENGTH 15

namespace telux
{
namespace platform
{

class SimulaPlatformFactory : public PlatformFactory
{
public:
    static PlatformFactory& getInstance()
    {
        LOG_DEBUG("[SimulaPlatformFactory] getInstance() called");
        static SimulaPlatformFactory instance;
        return instance;
    }

    std::shared_ptr<IDeviceInfoManager> getDeviceInfoManager(
        telux::common::InitResponseCb callback = nullptr) override
    {
        LOG_INFO("[SimulaPlatformFactory] getDeviceInfoManager() called");
        auto mgr = std::make_shared<DeviceInfoManager>(callback);
        LOG_DEBUG("[SimulaPlatformFactory] DeviceInfoManager instance created: %p",
                  static_cast<void*>(mgr.get()));
        return mgr;
    }

    std::shared_ptr<IFsManager> getFsManager(
        telux::common::InitResponseCb callback = nullptr) override
    {
        LOG_WARN("[SimulaPlatformFactory] getFsManager() not supported, returning nullptr");
        (void)callback;
        return nullptr;
    }

    std::shared_ptr<ITimeManager> getTimeManager(
        telux::common::InitResponseCb callback = nullptr) override
    {
        LOG_WARN("[SimulaPlatformFactory] getTimeManager() not supported, returning nullptr");
        (void)callback;
        return nullptr;
    }

    std::shared_ptr<hardware::IAntennaManager> getAntennaManager(
        telux::common::InitResponseCb callback = nullptr) override
    {
        LOG_WARN("[SimulaPlatformFactory] getAntennaManager() not supported, returning nullptr");
        (void)callback;
        return nullptr;
    }

private:
    SimulaPlatformFactory() = default;
    ~SimulaPlatformFactory() = default;
};


PlatformFactory& PlatformFactory::getInstance()
{
    LOG_DEBUG("[PlatformFactory] getInstance() called - returning SimulaPlatformFactory");
    return SimulaPlatformFactory::getInstance();
}

PlatformFactory::PlatformFactory()  = default;
PlatformFactory::~PlatformFactory() = default;

using namespace telux::common;

DeviceInfoManager::DeviceInfoManager(InitResponseCb cb)
    : serviceStatus_(ServiceStatus::SERVICE_AVAILABLE), callback_(cb)
{
    LOG_INFO("[DeviceInfoManager] Constructor called");
    if (callback_)
    {
        LOG_DEBUG("[DeviceInfoManager] Invoking init callback with SERVICE_AVAILABLE");
        callback_(ServiceStatus::SERVICE_AVAILABLE);
    }
}

DeviceInfoManager::~DeviceInfoManager()
{
    LOG_DEBUG("[DeviceInfoManager] Destructor called");
}

ServiceStatus DeviceInfoManager::getServiceStatus()
{
    LOG_DEBUG("[DeviceInfoManager] getServiceStatus() = %d",
              static_cast<int>(serviceStatus_));
    return serviceStatus_;
}

Status DeviceInfoManager::registerListener(std::weak_ptr<IDeviceInfoListener> listener)
{
    LOG_DEBUG("[DeviceInfoManager] registerListener() called - not supported");
    (void)listener;
    return Status::SUCCESS;
}

Status DeviceInfoManager::deregisterListener(std::weak_ptr<IDeviceInfoListener> listener)
{
    LOG_DEBUG("[DeviceInfoManager] deregisterListener() called - not supported");
    (void)listener;
    return Status::SUCCESS;
}

Status DeviceInfoManager::getPlatformVersion(PlatformVersion& pv)
{
    LOG_WARN("[DeviceInfoManager] getPlatformVersion() not supported");
    (void)pv;
    return Status::NOTSUPPORTED;
}

static bool isValidImei(const std::string& imei)
{
    if (imei.length() != IMEI_LENGTH)
    {
        return false;
    }
    return std::all_of(imei.begin(), imei.end(), ::isdigit);
}

Status DeviceInfoManager::getIMEI(std::string& imei)
{
    LOG_INFO("[DeviceInfoManager] getIMEI() called");

    struct stat fileStat;
    if (stat(DEVICEINFO_JSON_FILE, &fileStat) == 0)
    {
        LOG_DEBUG("[DeviceInfoManager] Found '%s', attempting to parse", DEVICEINFO_JSON_FILE);
        try
        {
            boost::property_tree::ptree pt;
            boost::property_tree::read_json(DEVICEINFO_JSON_FILE, pt);
            std::string parsedImei = pt.get<std::string>("IDeviceInfoManager.GetIMEI.imei");
            if (!isValidImei(parsedImei))
            {
                LOG_WARN("[DeviceInfoManager] IMEI '%s' from '%s' is invalid "
                         "(must be exactly %d digits) - using default IMEI",
                         parsedImei.c_str(), DEVICEINFO_JSON_FILE, IMEI_LENGTH);
            }
            else
            {
                imei = parsedImei;
                LOG_INFO("[DeviceInfoManager] getIMEI() from file - IMEI: '%s'", imei.c_str());
                return Status::SUCCESS;
            }
        }
        catch (const std::exception& e)
        {
            LOG_WARN("[DeviceInfoManager] Failed to parse '%s': %s - using default IMEI",
                     DEVICEINFO_JSON_FILE, e.what());
        }
    }
    else
    {
        LOG_DEBUG("[DeviceInfoManager] '%s' not found - using default IMEI", DEVICEINFO_JSON_FILE);
    }

    imei = DEFAULT_IMEI;
    LOG_INFO("[DeviceInfoManager] getIMEI() using default - IMEI: '%s'", imei.c_str());
    return Status::SUCCESS;
}

} // namespace platform
} // namespace telux

