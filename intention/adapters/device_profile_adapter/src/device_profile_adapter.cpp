/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
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

#include <optional>

#include "cJSON.h"
#include "devicestatus_define.h"
#include "fi_log.h"
#include "json_parser.h"
#include "sync_options.h"

namespace OHOS {
namespace Msdp {
namespace DeviceStatus {
namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL { LOG_CORE, MSDP_DOMAIN_ID, "DeviceProfileAdapter" };
} // namespace

DeviceProfileAdapter::DeviceProfileAdapter() {}

DeviceProfileAdapter::~DeviceProfileAdapter() {}

void DeviceProfileAdapter::AddProfileCallback(const std::string &pluginName, ProfileChangedCallback callback)
{
    CALL_INFO_TRACE;
    CHKPV(callback);
    std::lock_guard<std::mutex> guard(mutex_);
    auto it = profileChangedCallbacks_.find(pluginName);
    if (it != profileChangedCallbacks_.end()) {
        UpdateProfileCallback(pluginName, callback);
    } else {
        profileChangedCallbacks_[pluginName] = callback;
    }
}

void DeviceProfileAdapter::UpdateProfileCallback(const std::string &pluginName, ProfileChangedCallback callback)
{
    CALL_INFO_TRACE;
    CHKPV(callback);
    std::lock_guard<std::mutex> guard(mutex_);
    profileChangedCallbacks_[pluginName] = callback;
}

void DeviceProfileAdapter::RemoveProfileCallback(const std::string &pluginName)
{
    CALL_INFO_TRACE;
    std::lock_guard<std::mutex> guard(mutex_);
    auto it = profileChangedCallbacks_.find(pluginName);
    if (it == profileChangedCallbacks_.end()) {
        FI_HILOGE("The plugin not has callback, pluginName:%{public}s", pluginName.c_str());
        return;
    }
    profileChangedCallbacks_.erase(it);
}

int32_t DeviceProfileAdapter::RegisterProfileListener(const std::string &deviceId)
{
    CALL_INFO_TRACE;
    if (deviceId.empty()) {
        FI_HILOGE("deviceId is empty");
        return RET_ERR;
    }
    
    std::lock_guard<std::mutex> guard(mutex_);
    auto it = profileEventCallbacks_.find(deviceId);
    if (it == profileEventCallbacks_.end() || it->second == nullptr) {
        profileEventCallbacks_[deviceId] = std::make_shared<DeviceProfileAdapter::ProfileEventCallbackImpl>();
    }

    std::list<SubscribeInfo> subscribeInfos;
    PackSubscribeInfos(subscribeInfos, deviceId);
    std::list<ProfileEvent> failedEvents;
    int32_t ret = DistributedDeviceProfileClient::GetInstance().SubscribeProfileEvents(subscribeInfos,
        profileEventCallbacks_[deviceId], failedEvents);
    if (ret != RET_OK) {
        profileEventCallbacks_.erase(deviceId);
        FI_HILOGE("SubscribeProfileEvents failed");
        return RET_ERR;
    }
    return RET_OK;
}

int32_t DeviceProfileAdapter::UnRegisterProfileListener(const std::string &deviceId)
{
    CALL_INFO_TRACE;
    if (deviceId.empty()) {
        FI_HILOGE("deviceId is empty");
        return RET_ERR;
    }
    std::lock_guard<std::mutex> guard(mutex_);
    auto it = profileEventCallbacks_.find(deviceId);
    if (it == profileEventCallbacks_.end()) {
        FI_HILOGE("This device has no profileEventCallback");
        return RET_ERR;
    }
    std::list<ProfileEvent> profileEvents;
    profileEvents.emplace_back(ProfileEvent::EVENT_PROFILE_CHANGED);
    std::list<ProfileEvent> failedEvents;
    int32_t ret =  DistributedDeviceProfileClient::GetInstance().UnsubscribeProfileEvents(profileEvents, it->second,
        failedEvents);
    if (ret != RET_OK) {
        FI_HILOGE("UnsubscribeProfileEvents failed");
        return RET_ERR;
    }
    profileEventCallbacks_.erase(it);
    return RET_OK;
}

void DeviceProfileAdapter::PackSubscribeInfos(std::list<SubscribeInfo> &subscribeInfos, const std::string &deviceId)
{
    CALL_INFO_TRACE;
    std::list<std::string> serviceIdList;
    serviceIdList.emplace_back(SERVICE_ID);
    ExtraInfo extraInfo;
    extraInfo["deviceId"] = deviceId;
    extraInfo["serviceIds"] = serviceIdList;

    SubscribeInfo changeEventInfo;
    changeEventInfo.profileEvent = ProfileEvent::EVENT_PROFILE_CHANGED;
    changeEventInfo.extraInfo = std::move(extraInfo);
    SubscribeInfo syncEventInfo;
    syncEventInfo.profileEvent = ProfileEvent::EVENT_SYNC_COMPLETED;
    
    subscribeInfos.emplace_back(changeEventInfo);
    subscribeInfos.emplace_back(syncEventInfo);
}

void DeviceProfileAdapter::ProfileEventCallbackImpl::OnProfileChanged(
    const ProfileChangeNotification &changeNotification)
{
    CALL_INFO_TRACE;
    std::string deviceId = changeNotification.GetDeviceId();
    std::lock_guard<std::mutex> guard(DP_ADAPTER->mutex_);
    for (const auto &callback : DP_ADAPTER->profileChangedCallbacks_) {
        if (callback.second != nullptr) {
            callback.second(deviceId);
        } else {
            FI_HILOGE("callback is nullptr, plugin game:%{public}s", callback.first.c_str());
        }
    }
}

int32_t DeviceProfileAdapter::GetDPValue(ValueType valueType, cJSON* jsonValue, DP_VALUE &dpValue)
{
    switch (valueType) {
        case ValueType::INT_TYPE: {
            if (!cJSON_IsNumber(jsonValue)) {
                FI_HILOGE("dpValue is not number type");
                return RET_ERR;
            }
            DP_VALUE valueTemp { std::in_place_type<int32_t>, jsonValue->valueint };
            dpValue = valueTemp;
            break;
        }
        case ValueType::BOOL_TYPE: {
            if (!cJSON_IsNumber(jsonValue)) {
                FI_HILOGE("dpValue is not bool type");
                return RET_ERR;
            }
            DP_VALUE valueTemp { std::in_place_type<bool>, static_cast<bool>(jsonValue->valueint) };
            dpValue = valueTemp;
            break;
        }
        case ValueType::STRING_TYPE: {
            if (!cJSON_IsString(jsonValue)) {
                FI_HILOGE("dpValue is not string type");
                return RET_ERR;
            }
            DP_VALUE valueTemp { std::in_place_type<std::string>, jsonValue->valuestring };
            dpValue = valueTemp;
            break;
        }
        default: {
            FI_HILOGE("dpValue is neither an int type, nor a bool type, nor a string type, type:%{public}d",
                valueType);
            return RET_ERR;
        }
    }
    return RET_OK;
}

int32_t DeviceProfileAdapter::GetProperty(const std::string &deviceId, const std::string &characteristicsName,
    ValueType valueType, DP_VALUE &dpValue)
{
    CALL_INFO_TRACE;
    ServiceCharacteristicProfile profile;
    if (RET_OK != DistributedDeviceProfileClient::GetInstance().GetDeviceProfile(deviceId, SERVICE_ID, profile)) {
        FI_HILOGE("GetDeviceProfile failed");
        return RET_ERR;
    }
    std::string jsonData = profile.GetCharacteristicProfileJson();
    JsonParser parser;
    parser.json = cJSON_Parse(jsonData.c_str());
    if (!cJSON_IsObject(parser.json)) {
        FI_HILOGE("Parser json is not object");
        return RET_ERR;
    }
    cJSON* jsonValue = cJSON_GetObjectItemCaseSensitive(parser.json, characteristicsName.c_str());
    if (jsonValue != nullptr) {
        if (RET_OK != GetDPValue(valueType, jsonValue, dpValue)) {
            FI_HILOGE("GetDPValue failed");
            return RET_ERR;
        }
    }
    return RET_OK;
}

int32_t DeviceProfileAdapter::SetProperty(const DP_VALUE &dpValue, ValueType valueType,
    const std::string &characteristicsName, const std::vector<std::string> &deviceIds)
{
    CALL_INFO_TRACE;
    ServiceCharacteristicProfile profile;
    if (!SetProfile(profile, characteristicsName, dpValue, valueType)) {
        FI_HILOGE("SetProfile failed");
        return RET_ERR;
    }

    int32_t ret = DistributedDeviceProfileClient::GetInstance().PutDeviceProfile(profile);
    if (ret != RET_OK) {
        FI_HILOGE("Put device profile failed, ret:%{public}d", ret);
        return ret;
    }

    ret = SyncDeviceFile(deviceIds);
    if (ret != RET_OK) {
        FI_HILOGE("Sync device profile failed, ret:%{public}d", ret);
        return ret;
    }
    return ret;
}

int32_t DeviceProfileAdapter::SyncDeviceFile(const std::vector<std::string> &deviceIds)
{
    SyncOptions syncOptions;
    std::for_each(deviceIds.begin(), deviceIds.end(),
                  [&syncOptions](auto &deviceId) {
                      syncOptions.AddDevice(deviceId);
                      FI_HILOGD("Add device success");
                  });
    auto syncCallback = std::make_shared<DeviceProfileAdapter::ProfileEventCallbackImpl>();
    int32_t ret = DistributedDeviceProfileClient::GetInstance().SyncDeviceProfile(syncOptions, syncCallback);
    if (ret != RET_OK) {
        FI_HILOGE("Sync device profile failed");
    }
    return ret;
}

int32_t DeviceProfileAdapter::SetProfile(ServiceCharacteristicProfile &profile, const std::string &characteristicsName,
    const DP_VALUE &dpValue, ValueType valueType)
{
    profile.SetServiceId(SERVICE_ID);
    profile.SetServiceType(SERVICE_TYPE);
    char* smsg = nullptr;
    if (RET_OK != DistributedDeviceProfileClient::GetInstance().GetDeviceProfile("", SERVICE_ID, profile)) {
        FI_HILOGE("GetDeviceProfile failed");
        return RET_ERR;
    }
    std::string jsonData = profile.GetCharacteristicProfileJson();
    if (jsonData.empty() || jsonData == JSON_NULL) {
        cJSON* data = cJSON_CreateObject();
        cJSON* item = cJSON_CreateNull();
        if (RET_OK != CreatJsonItem(item, dpValue, valueType)) {
            FI_HILOGE("CreatJsonItem failed");
            return RET_ERR;
        }
        cJSON_AddItemToObject(data, characteristicsName.c_str(), item);
        smsg = cJSON_Print(data);
        cJSON_Delete(data);
        CHKPR(smsg, RET_ERR);
    } else {
        JsonParser parser;
        parser.json = cJSON_Parse(jsonData.c_str());
        if (!cJSON_IsObject(parser.json)) {
            FI_HILOGE("Parser json is not object");
            return RET_ERR;
        }

        cJSON* data = cJSON_GetObjectItemCaseSensitive(parser.json, characteristicsName.c_str());
        if (data != nullptr) {
            if (RET_OK != ModifyJsonItem(data, dpValue, valueType, characteristicsName)) {
                FI_HILOGE("Modify property failed");
                return RET_ERR;
            }
        } else {
            cJSON* item = cJSON_CreateNull();
            if (RET_OK != CreatJsonItem(item, dpValue, valueType)) {
                FI_HILOGE("CreatJsonItem failed");
                return RET_ERR;
            }
            cJSON_AddItemToObject(parser.json, characteristicsName.c_str(), item);
        }
        smsg = cJSON_Print(parser.json);
        CHKPR(smsg, RET_ERR);
    }
    profile.SetCharacteristicProfileJson(smsg);
    cJSON_free(smsg);
    return RET_OK;
}

int32_t DeviceProfileAdapter::CreatJsonItem(cJSON* item, const DP_VALUE &dpValue, ValueType valueType)
{
    switch (valueType) {
        case ValueType::INT_TYPE: {
            item = cJSON_CreateNumber(std::get<int32_t>(dpValue));
            break;
        }
        case ValueType::BOOL_TYPE: {
            item = cJSON_CreateNumber(std::get<bool>(dpValue));
            break;
        }
        case ValueType::STRING_TYPE: {
            item = cJSON_CreateString(std::get<std::string>(dpValue).c_str());
            break;
        }
        default: {
            FI_HILOGE("valueType is neither an int type, nor a bool type, nor a string type, type:%{public}d",
                valueType);
            return RET_ERR;
        }
    }
    return RET_OK;
}

int32_t DeviceProfileAdapter::ModifyJsonItem(cJSON* data, const DP_VALUE &dpValue, ValueType valueType,
    const std::string &characteristicsName)
{
    CHKPR(data, RET_ERR);
    switch (valueType) {
        case ValueType::INT_TYPE: {
            if (!cJSON_IsNumber(data)) {
                return RET_ERR;
            }
            cJSON_ReplaceItemInObject(data, characteristicsName.c_str(),
                cJSON_CreateNumber(std::get<int32_t>(dpValue)));
            break;
        }
        case ValueType::BOOL_TYPE: {
            if (!cJSON_IsNumber(data)) {
                return RET_ERR;
            }
            cJSON_ReplaceItemInObject(data, characteristicsName.c_str(),
                cJSON_CreateNumber(static_cast<int32_t>(std::get<bool>(dpValue))));
            break;
        }
        case ValueType::STRING_TYPE: {
            if (!cJSON_IsString(data)) {
                return RET_ERR;
            }
            cJSON_ReplaceItemInObject(data, characteristicsName.c_str(),
                cJSON_CreateString(std::get<std::string>(dpValue).c_str()));
            break;
        }
        default: {
            FI_HILOGE("dpValue is neither an int type, nor a bool type, nor a string type, type:%{public}d", valueType);
            return RET_ERR;
        }
    }
    return RET_OK;
}
} // namespace DeviceStatus
} // namespace Msdp
} // namespace OHOS
