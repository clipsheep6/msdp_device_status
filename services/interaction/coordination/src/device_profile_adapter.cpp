/*
 * Copyright (c) 2022-2023 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "device_profile_adapter.h"

#include <algorithm>
#include <mutex>

#include "distributed_device_profile_client.h"
#include "service_characteristic_profile.h"

#include "coordination_util.h"
#include "devicestatus_define.h"
#include "json_parser.h"

#undef LOG_TAG
#define LOG_TAG "DeviceProfileAdapter"

namespace OHOS {
namespace Msdp {
namespace DeviceStatus {
using namespace OHOS::DeviceProfile;
namespace {
const std::string SERVICE_ID { "deviceStatus" };
} // namespace

DeviceProfileAdapter::DeviceProfileAdapter() {}

DeviceProfileAdapter::~DeviceProfileAdapter()
{
    std::lock_guard<std::mutex> guard(adapterLock_);
    profileEventCallbacks_.clear();
    callbacks_.clear();
}

void DeviceProfileAdapter::ProfileEventCallbackImpl::OnProfileChanged(
    const ProfileChangeNotification &changeNotification)
{
    CALL_INFO_TRACE;
    std::string networkId = changeNotification.GetDeviceId();
    DP_ADAPTER->OnProfileChanged(networkId);
}

int32_t DeviceProfileAdapter::UpdateCrossingSwitchState(bool state, const std::vector<std::string> &deviceIds)
{
    std::string stateStr = state ? "true" : "false";
    FI_HILOGI("Crossing switch state:%{public}s, device size:%{public}d", stateStr.c_str(),
        static_cast<int32_t>(deviceIds.size()));
    const std::string SERVICE_TYPE = "deviceStatus";
    ServiceCharacteristicProfile profile;
    profile.SetServiceType(SERVICE_TYPE);
    profile.SetServiceId(SERVICE_ID);
    cJSON *data = cJSON_CreateObject();
    CHKPR(data, RET_ERR);
    cJSON_AddItemToObject(data, characteristicsName_.c_str(), cJSON_CreateNumber(state));
    char *smsg = cJSON_Print(data);
    cJSON_Delete(data);
    profile.SetCharacteristicProfileJson(smsg);
    cJSON_free(smsg);

    int32_t ret = DistributedDeviceProfileClient::GetInstance().PutDeviceProfile(profile);
    if (ret != 0) {
        FI_HILOGE("Failed to put the device profile, ret:%{public}d", ret);
        return ret;
    }
    return ret;
}

int32_t DeviceProfileAdapter::UpdateCrossingSwitchState(bool state)
{
    std::string stateStr = state ? "true" : "false";
    FI_HILOGI("Crossing switch state:%{public}s", stateStr.c_str());
    const std::string SERVICE_TYPE = "deviceStatus";
    ServiceCharacteristicProfile profile;
    profile.SetServiceId(SERVICE_ID);
    profile.SetServiceType(SERVICE_TYPE);
    cJSON *data = cJSON_CreateObject();
    CHKPR(data, RET_ERR);
    cJSON_AddItemToObject(data, characteristicsName_.c_str(), cJSON_CreateNumber(state));
    char *smsg = cJSON_Print(data);
    cJSON_Delete(data);
    profile.SetCharacteristicProfileJson(smsg);
    cJSON_free(smsg);

    return DistributedDeviceProfileClient::GetInstance().PutDeviceProfile(profile);
}

bool DeviceProfileAdapter::GetCrossingSwitchState(const std::string &networkIdOrUdId)
{
    CALL_INFO_TRACE;
    auto transfer = [] (const std::string &networkIdOrUdId) -> std::string {
        if (networkIdOrUdId == COORDINATION::GetLocalNetworkId() || networkIdOrUdId == COORDINATION::GetLocalUdId()) {
            FI_HILOGD("NetworkIdOrUdId:%{public}s", GetAnonyString(networkIdOrUdId).c_str());
            return "";
        }
        return networkIdOrUdId;
    };
    ServiceCharacteristicProfile profile;
    DistributedDeviceProfileClient::GetInstance().GetDeviceProfile(transfer(networkIdOrUdId), SERVICE_ID, profile);
    std::string jsonData = profile.GetCharacteristicProfileJson();
    JsonParser parser;
    parser.json = cJSON_Parse(jsonData.c_str());
    if (!cJSON_IsObject(parser.json)) {
        FI_HILOGE("The parser json is not an object, networkIdOrUdId:%{public}s",
            GetAnonyString(networkIdOrUdId).c_str());
        return false;
    }
    cJSON* state = cJSON_GetObjectItemCaseSensitive(parser.json, characteristicsName_.c_str());
    if (!cJSON_IsNumber(state)) {
        FI_HILOGE("The state is not a number type");
        return false;
    }
    return (static_cast<bool>(state->valueint));
}

int32_t DeviceProfileAdapter::RegisterCrossingStateListener(const std::string &networkId, DPCallback callback)
{
    CHKPR(callback, RET_ERR);
    if (networkId.empty()) {
        FI_HILOGE("DeviceId is nullptr");
        return RET_ERR;
    }
    std::lock_guard<std::mutex> guard(adapterLock_);
    auto callbackIter = callbacks_.find(networkId);
    if (callbackIter != callbacks_.end()) {
        callbackIter->second = callback;
        FI_HILOGW("Callback is updated");
        return RET_OK;
    }
    callbacks_[networkId] = callback;
    FI_HILOGI("Register crossing state listener success, networkId:%{public}s", GetAnonyString(networkId).c_str());
    int32_t ret = RegisterProfileListener(networkId);
    if (ret != RET_OK) {
        FI_HILOGE("Register profile listener failed");
    }
    return ret;
}

int32_t DeviceProfileAdapter::UnregisterCrossingStateListener(const std::string &networkId)
{
    if (networkId.empty()) {
        FI_HILOGE("DeviceId is empty");
        return RET_ERR;
    }
    FI_HILOGI("Unregister crossing state listener, networkId:%{public}s", GetAnonyString(networkId).c_str());
    std::lock_guard<std::mutex> guard(adapterLock_);
    auto it = profileEventCallbacks_.find(networkId);
    if (it != profileEventCallbacks_.end()) {
        std::list<ProfileEvent> profileEvents;
        profileEvents.emplace_back(ProfileEvent::EVENT_PROFILE_CHANGED);
        std::list<ProfileEvent> failedEvents;
        DistributedDeviceProfileClient::GetInstance().UnsubscribeProfileEvents(profileEvents,
            it->second, failedEvents);
        profileEventCallbacks_.erase(it);
    }
    auto callbackIter = callbacks_.find(networkId);
    if (callbackIter == callbacks_.end()) {
        FI_HILOGW("This device has no callback");
        return RET_OK;
    }
    callbacks_.erase(callbackIter);
    return RET_OK;
}

int32_t DeviceProfileAdapter::RegisterProfileListener(const std::string &networkId)
{
    CALL_INFO_TRACE;
    std::list<std::string> serviceIdList;
    serviceIdList.emplace_back(SERVICE_ID);
    ExtraInfo extraInfo;
    extraInfo["deviceId"] = networkId;
    extraInfo["serviceIds"] = serviceIdList;
    SubscribeInfo changeEventInfo;
    changeEventInfo.profileEvent = ProfileEvent::EVENT_PROFILE_CHANGED;
    changeEventInfo.extraInfo = std::move(extraInfo);
    std::list<SubscribeInfo> subscribeInfos;
    subscribeInfos.emplace_back(changeEventInfo);
    SubscribeInfo syncEventInfo;
    syncEventInfo.profileEvent = ProfileEvent::EVENT_SYNC_COMPLETED;
    subscribeInfos.emplace_back(syncEventInfo);
    std::list<ProfileEvent> failedEvents;
    auto it = profileEventCallbacks_.find(networkId);
    if (it == profileEventCallbacks_.end() || it->second == nullptr) {
        profileEventCallbacks_[networkId] = std::make_shared<DeviceProfileAdapter::ProfileEventCallbackImpl>();
    }
    return DistributedDeviceProfileClient::GetInstance().SubscribeProfileEvents(
        subscribeInfos, profileEventCallbacks_[networkId], failedEvents);
}

void DeviceProfileAdapter::OnProfileChanged(const std::string &networkId)
{
    std::lock_guard<std::mutex> guard(adapterLock_);
    auto it = callbacks_.find(networkId);
    if (it == callbacks_.end()) {
        FI_HILOGW("The device has no callback");
        return;
    }
    if (it->second != nullptr) {
        auto state = GetCrossingSwitchState(networkId);
        it->second(networkId, state);
        std::string stateStr = state ? "true" : "false";
        FI_HILOGI("Crossing switch state:%{public}s", stateStr.c_str());
    } else {
        callbacks_.erase(it);
        FI_HILOGW("Remove networkId:%{public}s profile changed callback", GetAnonyString(networkId).c_str());
    }
}
} // namespace DeviceStatus
} // namespace Msdp
} // namespace OHOS
