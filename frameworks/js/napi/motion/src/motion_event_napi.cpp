/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "motion_event_napi.h"

#include "fi_log.h"

#undef LOG_TAG
#define LOG_TAG "DeviceMotionEvent"

namespace OHOS {
namespace Msdp {
MotionEventNapi::MotionEventNapi(napi_env env, napi_value thisVar)
{
    env_ = env;
    thisVarRef_ = nullptr;
    napi_create_reference(env, thisVar, 1, &thisVarRef_);
}

MotionEventNapi::~MotionEventNapi()
{
    events_.clear();
    if (env_ != nullptr && thisVarRef_ != nullptr) {
        napi_delete_reference(env_, thisVarRef_);
    }
}

#ifdef MOTION_ENABLE
bool MotionEventNapi::AddCallback(int32_t eventType, napi_value handler)
{
    FI_HILOGD("Enter");
    auto iter = events_.find(eventType);
    if (iter == events_.end()) {
        FI_HILOGD("found event:%{public}d", eventType);
        std::shared_ptr<MotionEventListener> listener = std::make_shared<MotionEventListener>();
        std::set<napi_ref> onRefSets;
        listener->onRefSets = onRefSets;
        napi_ref onHandlerRef = nullptr;
        napi_status status = napi_create_reference(env_, handler, 1, &onHandlerRef);
        if (status != napi_ok) {
            FI_HILOGE("napi_create_reference failed");
            return false;
        }

        auto ret = listener->onRefSets.insert(onHandlerRef);
        if (!ret.second) {
            FI_HILOGE("Failed to insert refs");
            return false;
        }

        events_.insert(std::make_pair(eventType, listener));
        FI_HILOGD("Insert finish");
        return true;
    }
    FI_HILOGD("found event: %{public}d", eventType);
    if (iter->second == nullptr) {
        FI_HILOGE("listener is nullptr");
        return false;
    }
    if (iter->second->onRefSets.empty()) {
        FI_HILOGE("Refs is empty()");
        return false;
    }

    FI_HILOGD("Check type: %{public}d same handle", eventType);
    if (!InsertRef(iter->second, handler)) {
        FI_HILOGE("Failed to insert ref");
        return false;
    }
    return true;
}

bool MotionEventNapi::RemoveCallback(int32_t eventType)
{
    FI_HILOGD("Enter, event:%{public}d", eventType);
    auto iter = events_.find(eventType);
    if (iter == events_.end()) {
        FI_HILOGE("EventType %{public}d not found", eventType);
        return false;
    }

    if (iter->second == nullptr) {
        FI_HILOGE("listener is nullptr");
        return false;
    }

    if (iter->second->onRefSets.empty()) {
        FI_HILOGE("Refs is empty");
        return false;
    }

    for (auto it = iter->second->onRefSets.begin(); it != iter->second->onRefSets.end();) {
        if (*it == nullptr) {
            ++it;
            continue;
        }
        napi_status status = napi_delete_reference(env_, *it);
        if (status != napi_ok) {
            FI_HILOGE("napi_delete_reference failed");
            ++it;
            continue;
        }
        it = iter->second->onRefSets.erase(it);
    }
    if (iter->second->onRefSets.empty()) {
        FI_HILOGE("onRefSets is empty");
        events_.erase(iter);
    }
    return true;
}

bool MotionEventNapi::InsertRef(std::shared_ptr<MotionEventListener> listener, const napi_value &handler)
{
    if (listener == nullptr) {
        FI_HILOGE("listener is nullptr");
        return false;
    }
    for (auto item : listener->onRefSets) {
        napi_value onHandler = nullptr;
        napi_status status = napi_get_reference_value(env_, item, &onHandler);
        if (status != napi_ok) {
            FI_HILOGE("napi_get_reference_value failed");
            status = napi_delete_reference(env_, item);
            if (status != napi_ok) {
                FI_HILOGE("napi_delete_reference failed");
                continue;
            }
            continue;
        }

        if (IsSameValue(env_, handler, onHandler)) {
            continue;
        }

        napi_ref onHandlerRef = nullptr;
        status = napi_create_reference(env_, handler, 1, &(onHandlerRef));
        if (status != napi_ok) {
            FI_HILOGE("napi_create_reference failed");
            return false;
        }

        FI_HILOGD("Insert new ref");
        auto ret = listener->onRefSets.insert(onHandlerRef);
        if (!ret.second) {
            FI_HILOGE("Failed to insert");
            return false;
        }
    }
    FI_HILOGD("ref size %{public}zu", listener->onRefSets.size());
    return true;
}

void MotionEventNapi::OnEventOperatingHand(int32_t eventType, size_t argc, const MotionEvent &event)
{
    FI_HILOGD("eventType: %{public}d", eventType);
    auto typeIter = events_.find(eventType);
    if (typeIter == events_.end()) {
        FI_HILOGE("eventType: %{public}d not found", eventType);
        return;
    }
    for (auto item : typeIter->second->onRefSets) {
        napi_value handler = nullptr;
        napi_status ret = napi_get_reference_value(env_, item, &handler);
        if (ret != napi_ok) {
            FI_HILOGE("napi_get_reference_value for %{public}d failed, status: %{public}d", eventType, ret);
            return;
        }
        ConvertOperatingHandData(handler, argc, event);
    }
}

void MotionEventNapi::ConvertOperatingHandData(napi_value handler, size_t argc, const MotionEvent &event)
{
    napi_value motionValue = nullptr;
    napi_value result;
    napi_status ret = napi_create_object(env_, &result);
    if (ret != napi_ok) {
        FI_HILOGE("napi_create_object failed");
        return;
    }
    CreateIntData(env_, motionValue, result, "OperatingHandStatus", event.status);
    napi_value callResult = nullptr;
    ret = napi_call_function(env_, nullptr, handler, argc, &result, &callResult);
    if (ret != napi_ok) {
        FI_HILOGE("napi_call_function ret %{public}d", ret);
        return;
    }
}

void MotionEventNapi::CreateIntData(napi_env env, napi_value motionValue, napi_value result, std::string name,
    int32_t value)
{
    napi_status ret = napi_create_int32(env, value, &motionValue);
    if (ret != napi_ok) {
        FI_HILOGE("napi_create_int32 failed");
        return;
    }

    ret = napi_set_named_property(env, result, name.c_str(), motionValue);
    if (ret != napi_ok) {
        FI_HILOGE("napi_set_named_property failed");
        return;
    }
}

bool MotionEventNapi::IsSameValue(const napi_env &env, const napi_value &lhs, const napi_value &rhs)
{
    FI_HILOGD("Enter");
    bool result = false;
    napi_status status = napi_strict_equals(env, lhs, rhs, &result);
    if (status != napi_ok) {
        FI_HILOGE("napi_strict_equals failed");
        return result;
    }
    return result;
}

bool MotionEventNapi::CheckEvents(int32_t eventType)
{
    FI_HILOGD("Enter");
    auto typeIter = events_.find(eventType);
    if (typeIter == events_.end()) {
        FI_HILOGD("eventType not find");
        return true;
    }
    if (typeIter->second->onRefSets.empty()) {
        return true;
    }
    return false;
}
#endif
} // namespace Msdp
} // namespace OHOS
