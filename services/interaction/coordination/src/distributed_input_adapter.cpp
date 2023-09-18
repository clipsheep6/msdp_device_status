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

#include "distributed_input_adapter.h"

#include <algorithm>
#include <map>
#include <mutex>

#include "coordination_event_manager.h"
#include "devicestatus_define.h"

namespace OHOS {
namespace Msdp {
namespace DeviceStatus {
using namespace DistributedHardware::DistributedInput;
namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL { LOG_CORE, MSDP_DOMAIN_ID, "DistributedInputAdapter" };
constexpr int32_t DEFAULT_DELAY_TIME { 4000 };
constexpr int32_t RETRY_TIME { 2 };
} // namespace
DistributedInputAdapter::DistributedInputAdapter() = default;

DistributedInputAdapter::~DistributedInputAdapter()
{
    CALL_INFO_TRACE;
    std::lock_guard<std::mutex> guard(adapterLock_);
    callbackMap_.clear();
}

bool DistributedInputAdapter::IsNeedFilterOut(const std::string &networkId, const BusinessEvent &event)
{
    CALL_INFO_TRACE;
    return DistributedInputKit::IsNeedFilterOut(networkId, event);
}

int32_t DistributedInputAdapter::StartRemoteInput(const std::string &remoteNetworkId,
    const std::string &originNetworkId, const std::vector<std::string> &inputDeviceDhids, DInputCallback callback)
{
    CALL_INFO_TRACE;
    sptr<IStartStopDInputsCallback> cb = new (std::nothrow) StartDInputCallbackSink();
    CHKPR(cb, ERROR_NULL_POINTER);
    SaveCallback(CallbackType::StartDInputCallbackSink, callback);
    return DistributedInputKit::StartRemoteInput(remoteNetworkId, originNetworkId, inputDeviceDhids, cb);
}

int32_t DistributedInputAdapter::StopRemoteInput(const std::string &originNetworkId,
    const std::vector<std::string> &inputDeviceDhids, DInputCallback callback)
{
    CALL_INFO_TRACE;
    sptr<IStartStopDInputsCallback> cb = new (std::nothrow) StopDInputCallbackDHIds();
    CHKPR(cb, ERROR_NULL_POINTER);
    SaveCallback(CallbackType::StopDInputCallbackDHIds, callback);
    return DistributedInputKit::StopRemoteInput(originNetworkId, inputDeviceDhids, cb);
}

int32_t DistributedInputAdapter::StopRemoteInput(const std::string &remoteNetworkId, const std::string &originNetworkId,
    const std::vector<std::string> &inputDeviceDhids, DInputCallback callback)
{
    CALL_INFO_TRACE;
    sptr<IStartStopDInputsCallback> cb = new (std::nothrow) StopDInputCallbackSink();
    CHKPR(cb, ERROR_NULL_POINTER);
    SaveCallback(CallbackType::StopDInputCallbackSink, callback);
    return DistributedInputKit::StopRemoteInput(remoteNetworkId, originNetworkId, inputDeviceDhids, cb);
}

int32_t DistributedInputAdapter::PrepareRemoteInput(const std::string &remoteNetworkId,
                                                    const std::string &originNetworkId, DInputCallback callback)
{
    CALL_INFO_TRACE;
    sptr<IPrepareDInputCallback> cb = new (std::nothrow) PrepareStartDInputCallbackSink();
    CHKPR(cb, ERROR_NULL_POINTER);
    SaveCallback(CallbackType::PrepareStartDInputCallbackSink, callback);
    return DistributedInputKit::PrepareRemoteInput(remoteNetworkId, originNetworkId, cb);
}

int32_t DistributedInputAdapter::UnPrepareRemoteInput(const std::string &remoteNetworkId,
                                                      const std::string &originNetworkId, DInputCallback callback)
{
    CALL_INFO_TRACE;
    sptr<IUnprepareDInputCallback> cb = new (std::nothrow) UnPrepareStopDInputCallbackSink();
    CHKPR(cb, ERROR_NULL_POINTER);
    SaveCallback(CallbackType::UnPrepareStopDInputCallbackSink, callback);
    return DistributedInputKit::UnprepareRemoteInput(remoteNetworkId, originNetworkId, cb);
}

int32_t DistributedInputAdapter::PrepareRemoteInput(const std::string &networkId, DInputCallback callback)
{
    CALL_INFO_TRACE;
    sptr<IPrepareDInputCallback> cb = new (std::nothrow) PrepareStartDInputCallback();
    CHKPR(cb, ERROR_NULL_POINTER);
    SaveCallback(CallbackType::PrepareStartDInputCallback, callback);
    return DistributedInputKit::PrepareRemoteInput(networkId, cb);
}

int32_t DistributedInputAdapter::UnPrepareRemoteInput(const std::string &networkId, DInputCallback callback)
{
    CALL_INFO_TRACE;
    sptr<IUnprepareDInputCallback> cb = new (std::nothrow) UnPrepareStopDInputCallback();
    CHKPR(cb, ERROR_NULL_POINTER);
    SaveCallback(CallbackType::UnPrepareStopDInputCallback, callback);
    return DistributedInputKit::UnprepareRemoteInput(networkId, cb);
}

int32_t DistributedInputAdapter::RegisterSessionStateCb(std::function<void(uint32_t)> callback)
{
    CALL_INFO_TRACE;
    sptr<SessionStateCallback> cb = new (std::nothrow) SessionStateCallback(callback);
    CHKPR(callback, ERROR_NULL_POINTER);
    return DistributedInputKit::RegisterSessionStateCb(cb);
}

int32_t DistributedInputAdapter::UnregisterSessionStateCb()
{
    CALL_INFO_TRACE;
    return DistributedInputKit::UnregisterSessionStateCb();
}

void DistributedInputAdapter::SaveCallback(CallbackType type, DInputCallback callback)
{
    std::lock_guard<std::mutex> guard(adapterLock_);
    CHKPV(callback);
    callbackMap_[type] = callback;
    AddTimer(type);
}

void DistributedInputAdapter::AddTimer(const CallbackType &type)
{
    FI_HILOGD("AddTimer type:%{public}d", type);
    auto context = COOR_EVENT_MGR->GetIContext();
    CHKPV(context);
    int32_t timerId = context->GetTimerManager().AddTimer(DEFAULT_DELAY_TIME, RETRY_TIME, [this, type]() {
        if ((callbackMap_.find(type) == callbackMap_.end()) || (watchingMap_.find(type) == watchingMap_.end())) {
            FI_HILOGE("Callback or watching is not exist");
            return;
        }
        if (watchingMap_[type].times == 0) {
            FI_HILOGI("It will be retry to call callback next time");
            watchingMap_[type].times++;
            return;
        }
        callbackMap_[type](false);
        callbackMap_.erase(type);
    });
    if (timerId < 0) {
        FI_HILOGE("Add timer failed, timeId:%{public}d", timerId);
        return;
    }
    watchingMap_[type].timerId = timerId;
    watchingMap_[type].times = 0;
}

void DistributedInputAdapter::RemoveTimer(const CallbackType &type)
{
    FI_HILOGD("Remove timer type:%{public}d", type);
    if (watchingMap_.find(type) != watchingMap_.end()) {
        auto context = COOR_EVENT_MGR->GetIContext();
        CHKPV(context);
        context->GetTimerManager().RemoveTimer(watchingMap_[type].timerId);
        watchingMap_.erase(type);
    }
}

void DistributedInputAdapter::ProcessDInputCallback(CallbackType type, int32_t status)
{
    CALL_INFO_TRACE;
    std::lock_guard<std::mutex> guard(adapterLock_);
    RemoveTimer(type);
    auto it = callbackMap_.find(type);
    if (it == callbackMap_.end()) {
        FI_HILOGI("Dinput callback not exist");
        return;
    }
    it->second(status == RET_OK);
    callbackMap_.erase(it);
}

void DistributedInputAdapter::StartDInputCallback::OnResult(const std::string &devId, const uint32_t &inputTypes,
                                                            const int32_t &status)
{
    D_INPUT_ADAPTER->ProcessDInputCallback(CallbackType::StartDInputCallback, status);
}

void DistributedInputAdapter::StopDInputCallback::OnResult(const std::string &devId, const uint32_t &inputTypes,
                                                           const int32_t &status)
{
    D_INPUT_ADAPTER->ProcessDInputCallback(CallbackType::StopDInputCallback, status);
}

void DistributedInputAdapter::StartDInputCallbackDHIds::OnResultDhids(const std::string &devId, const int32_t &status)
{
    D_INPUT_ADAPTER->ProcessDInputCallback(CallbackType::StartDInputCallbackDHIds, status);
}

void DistributedInputAdapter::StopDInputCallbackDHIds::OnResultDhids(const std::string &devId, const int32_t &status)
{
    D_INPUT_ADAPTER->ProcessDInputCallback(CallbackType::StopDInputCallbackDHIds, status);
}

void DistributedInputAdapter::StartDInputCallbackSink::OnResultDhids(const std::string &devId, const int32_t &status)
{
    D_INPUT_ADAPTER->ProcessDInputCallback(CallbackType::StartDInputCallbackSink, status);
}

void DistributedInputAdapter::StopDInputCallbackSink::OnResultDhids(const std::string &devId, const int32_t &status)
{
    D_INPUT_ADAPTER->ProcessDInputCallback(CallbackType::StopDInputCallbackSink, status);
}

void DistributedInputAdapter::PrepareStartDInputCallback::OnResult(const std::string &devId, const int32_t &status)
{
    D_INPUT_ADAPTER->ProcessDInputCallback(CallbackType::PrepareStartDInputCallback, status);
}

void DistributedInputAdapter::UnPrepareStopDInputCallback::OnResult(const std::string &devId, const int32_t &status)
{
    D_INPUT_ADAPTER->ProcessDInputCallback(CallbackType::UnPrepareStopDInputCallback, status);
}

void DistributedInputAdapter::PrepareStartDInputCallbackSink::OnResult(const std::string &devId, const int32_t &status)
{
    D_INPUT_ADAPTER->ProcessDInputCallback(CallbackType::PrepareStartDInputCallbackSink, status);
}

void DistributedInputAdapter::UnPrepareStopDInputCallbackSink::OnResult(const std::string &devId, const int32_t &status)
{
    D_INPUT_ADAPTER->ProcessDInputCallback(CallbackType::UnPrepareStopDInputCallbackSink, status);
}

void DistributedInputAdapter::SessionStateCallback::OnResult(const std::string &devId, const uint32_t status)
{
    CHKPV(callback_);
    callback_(status);
}
} // namespace DeviceStatus
} // namespace Msdp
} // namespace OHOS
