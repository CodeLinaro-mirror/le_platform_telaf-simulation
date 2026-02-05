/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef SIMPROFILEMANAGERBYPASS_HPP
#define SIMPROFILEMANAGERBYPASS_HPP

#include <condition_variable>
#include <memory>
#include <map>
#include <mutex>
#include <list>
#include <string>

#include <telux/tel/SimProfileManager.hpp>
#include <telux/tel/SimProfileDefines.hpp>
#include <telux/tel/SimProfile.hpp>

#include "common/AsyncTaskQueue.hpp"
#include "common/ListenerManager.hpp"
#include "telux/common/CommonDefines.hpp"
#include "telux/common/DeviceConfig.hpp"
#include "common/Logger.hpp"

namespace telux {
namespace tel {

struct RemoteSimProfileUserData {
    int cmdCallbackId;
    int profileId;
    ProfileType profileType;
    bool isActive;
    SlotId slotId;
};

/*
 * The struct collate the response for request profile list and keep
 * the track of response for each request for profile info.
 * The response could be error or failure. If response for any request for
 * profile info is recieved as error then we send empty profile list to client
 *
 */
struct RequestProfileListData {
    std::vector<std::shared_ptr<telux::tel::SimProfile>> profiles;
    bool errorOccured;
    int totalNoOfprofiles;
    SlotId slotId;
};

/**
 * Indicates the events associated during SIM Refresh.
 */
enum class RefreshEvent {
    UNKNOWN = -1,         /**< SIM Refresh Event unknown */
    STARTED = 1,          /**< SIM Refresh started */
    COMPLETED = 2,        /**< SIM Refresh completed successfully */
    FAILED = 3,           /**< SIM Refresh failed */
};

class SimProfileManagerBypass : public ISimProfileManager,
                                public std::enable_shared_from_this<SimProfileManagerBypass> {

 public:
    SimProfileManagerBypass();

    ~SimProfileManagerBypass();

    std::future<bool> onSubsystemReady() override;

    bool isSubsystemReady() override;

    telux::common::Status init(telux::common::InitResponseCb callback);

    telux::common::ServiceStatus getServiceStatus() override;

    void cleanup();

    telux::common::Status addProfile(SlotId slotId,
        const std::string &activationCode = "", const std::string &confirmationCode = "",
        bool userConsentSupported = false, common::ResponseCallback callback = nullptr) override;

    telux::common::Status deleteProfile(SlotId slotId, int profileId,
        common::ResponseCallback callback = nullptr) override;

    telux::common::Status setProfile(SlotId slotId, int profileId,
        bool enable = false, common::ResponseCallback callback = nullptr) override;

    telux::common::Status updateNickName(SlotId slotId, int profileId,
        const std::string &nickName = "", common::ResponseCallback callback = nullptr) override;

    telux::common::Status requestProfileList(SlotId slotId,
        ProfileListResponseCb) override;

    telux::common::Status requestEid(SlotId slotId,
        EidResponseCb) override;

    telux::common::Status provideUserConsent(SlotId slotId,
        bool userConsent, UserConsentReasonType reason,
        common::ResponseCallback callback = nullptr) override;

    telux::common::Status provideConfirmationCode(SlotId slotId, std::string code,
        common::ResponseCallback callback = nullptr) override;

    telux::common::Status setServerAddress(SlotId slotId, const std::string &smdpAddress,
        common::ResponseCallback callback = nullptr) override;

    telux::common::Status requestServerAddress(SlotId slotId,
        ServerAddressResponseCb callback = nullptr) override;

    telux::common::Status memoryReset(SlotId slotId, ResetOptionMask mask,
        common::ResponseCallback callback = nullptr) override;

    telux::common::Status registerListener(std::weak_ptr<ISimProfileListener> listener);

    telux::common::Status deregisterListener(std::weak_ptr<ISimProfileListener> listener);

 private:
    std::mutex mtx_;

    std::condition_variable cv_;
    // Map with key as CallbackID and shared pointer to RequestProfileListData
    std::map<int, std::shared_ptr<RequestProfileListData>> profileListDataMap_ = {};
    telux::common::ServiceStatus subSystemStatus_
        = telux::common::ServiceStatus::SERVICE_UNAVAILABLE;
    telux::common::ServiceStatus lastReportedSvcState_
      = telux::common::ServiceStatus::SERVICE_AVAILABLE;
    // std::shared_ptr<telux::qmi::UimQmiClient> uimQmiClient_ = nullptr;
    telux::common::AsyncTaskQueue<void> taskQ_;

    // telux::common::CommandCallbackManager cmdCallbackMgr_;
    std::shared_ptr<telux::common::ListenerManager<ISimProfileListener>> listenerMgr_;

    // regRefreshIndCount_ counts the number of successful responses for
    // registerForUimRefreshIndication requests. If (regRefreshIndCount_ == noOfSlots_), then
    // subsystem is ready.
    uint8_t regRefreshIndCount_ = 0;
    uint8_t noOfSlots_ = 0;

    // This map is used to track if there is already set profile operation in progress, then
    // discard the subsequent set profile operation till current one completes applicable
    // for each slot.
    // Map stores slotId as key and cmdId corresponding to callback as value, in case of
    // nullptr callback, cmdId stored in map is INVALID_COMMAND_ID, So whenever any error occurs
    // or set profile request succeeds then cmdID can be fetched from map associated with slot
    // and then corresponding callback which was passed in set profile API can be invoked.

    std::map<SlotId, int> cachedSetProfileCmdIdMap_;

    // This map is used to track if there is already delete profile operation in progress, then
    // discard the subsequent delete profile operation till current one completes, applicable
    // for each slot.
    // Map stores slotId as key and cmdId corresponding to callback as value, in case of
    // nullptr callback, cmdId stored in map is INVALID_COMMAND_ID, So whenever any error occurs or
    // delete profile request succeeds then cmdID can be fetched from map associated with
    // slot and then corresponding callback which was passed in delete profile API can be invoked.
    std::map<SlotId, int> cachedDeleteProfileCmdIdMap_;

    //This map will store slot as key and value as true if profile was enabled before deletion
    //It is also used to track if for given slot the profile was enabled initially and disabling
    //of profile succeeded but deletion failed so need to rollback to enabled state.

    std::map<SlotId, bool> profileEnabledMap_;

    telux::common::InitResponseCb initCb_;

    void initSync();
    void updateReadyState();
    bool waitForInitialization();
    void setServiceStatus(telux::common::ServiceStatus status);
    void fakeRefrashRegResponse();

};  // end of SimProfileManagerBypass class
}
}
#endif
