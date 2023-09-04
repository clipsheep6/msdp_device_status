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

#include "coordination_manager_impl.h"

#include "devicestatus_client.h"
#include "devicestatus_define.h"
#include "util.h"

namespace OHOS {
namespace Msdp {
namespace DeviceStatus {
namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL { LOG_CORE, MSDP_DOMAIN_ID, "CoordinationManagerImpl" };
} // namespace

int32_t CoordinationManagerImpl::RegisterCoordinationListener(CoordinationListenerPtr listener)
{
    CALL_DEBUG_ENTER;
    CHKPR(listener, RET_ERR);
    std::lock_guard<std::mutex> guard(mtx_);
    for (const auto &item : devcoordinationListeners_) {
        if (item == listener) {
            FI_HILOGW("The listener already exists");
            return RET_ERR;
        }
    }
    if (!isListeningProcess_) {
        FI_HILOGI("Start monitoring");
        int32_t ret = DeviceStatusClient::GetInstance().RegisterCoordinationListener();
        if (ret != RET_OK) {
            FI_HILOGE("Failed to register, ret:%{public}d", ret);
            return ret;
        }
        isListeningProcess_ = true;
    }
    devcoordinationListeners_.push_back(listener);
    return RET_OK;
}

int32_t CoordinationManagerImpl::UnregisterCoordinationListener(CoordinationListenerPtr listener)
{
    CALL_DEBUG_ENTER;
    std::lock_guard<std::mutex> guard(mtx_);
    if (listener == nullptr) {
        devcoordinationListeners_.clear();
        goto listenerLabel;
    }
    for (auto it = devcoordinationListeners_.begin(); it != devcoordinationListeners_.end(); ++it) {
        if (*it == listener) {
            devcoordinationListeners_.erase(it);
            goto listenerLabel;
        }
    }

listenerLabel:
    if (isListeningProcess_ && devcoordinationListeners_.empty()) {
        isListeningProcess_ = false;
        return DeviceStatusClient::GetInstance().UnregisterCoordinationListener();
    }
    return RET_OK;
}

int32_t CoordinationManagerImpl::PrepareCoordination(FuncCoordinationMessage callback)
{
    CALL_DEBUG_ENTER;
    std::lock_guard<std::mutex> guard(mtx_);
    CoordinationEvent event;
    event.msg = callback;
    if (userData_ == std::numeric_limits<int32_t>::max()) {
        FI_HILOGE("userData exceeds the maximum");
        return RET_ERR;
    }
    int32_t ret = DeviceStatusClient::GetInstance().PrepareCoordination(userData_);
    if (ret != RET_OK) {
        FI_HILOGE("Prepare coordination failed");
        return ret;
    }
    devCoordinationEvents_[userData_] = event;
    userData_++;
    return RET_OK;
}

int32_t CoordinationManagerImpl::UnprepareCoordination(FuncCoordinationMessage callback)
{
    CALL_DEBUG_ENTER;
    CoordinationEvent event;
    event.msg = callback;
    std::lock_guard<std::mutex> guard(mtx_);
    if (userData_ == std::numeric_limits<int32_t>::max()) {
        FI_HILOGE("userData exceeds the maximum");
        return RET_ERR;
    }
    int32_t ret = DeviceStatusClient::GetInstance().UnprepareCoordination(userData_);
    if (ret != RET_OK) {
        FI_HILOGE("Unprepare coordination failed");
        return ret;
    }
    devCoordinationEvents_[userData_] = event;
    userData_++;
    return RET_OK;
}

int32_t CoordinationManagerImpl::ActivateCoordination(const std::string &remoteNetworkId,
    int32_t startDeviceId, FuncCoordinationMessage callback)
{
    CALL_DEBUG_ENTER;
    std::lock_guard<std::mutex> guard(mtx_);
    CoordinationEvent event;
    event.msg = callback;
    if (userData_ == std::numeric_limits<int32_t>::max()) {
        FI_HILOGE("userData exceeds the maximum");
        return RET_ERR;
    }
    int32_t ret = DeviceStatusClient::GetInstance().ActivateCoordination(
        userData_, remoteNetworkId, startDeviceId);
    if (ret != RET_OK) {
        FI_HILOGE("Activate coordination failed");
        return ret;
    }
    devCoordinationEvents_[userData_] = event;
    userData_++;
    return RET_OK;
}

int32_t CoordinationManagerImpl::DeactivateCoordination(bool isUnchained, FuncCoordinationMessage callback)
{
    CALL_DEBUG_ENTER;
    std::lock_guard<std::mutex> guard(mtx_);
    CoordinationEvent event;
    event.msg = callback;
    if (userData_ == std::numeric_limits<int32_t>::max()) {
        FI_HILOGE("userData exceeds the maximum");
        return RET_ERR;
    }
    int32_t ret = DeviceStatusClient::GetInstance().DeactivateCoordination(userData_, isUnchained);
    if (ret != RET_OK) {
        FI_HILOGE("Deactivate coordination failed");
        return ret;
    }
    devCoordinationEvents_[userData_] = event;
    userData_++;
    return RET_OK;
}

int32_t CoordinationManagerImpl::GetCoordinationState(
    const std::string &deviceId, FuncCoordinationState callback)
{
    CALL_DEBUG_ENTER;
    std::lock_guard<std::mutex> guard(mtx_);
    CoordinationEvent event;
    event.state = callback;
    if (userData_ == std::numeric_limits<int32_t>::max()) {
        FI_HILOGE("userData exceeds the maximum");
        return RET_ERR;
    }
    int32_t ret = DeviceStatusClient::GetInstance().GetCoordinationState(userData_, deviceId);
    if (ret != RET_OK) {
        FI_HILOGE("Get coordination state failed");
        return ret;
    }
    devCoordinationEvents_[userData_] = event;
    userData_++;
    return RET_OK;
}

void CoordinationManagerImpl::OnDevCoordinationListener(const std::string deviceId, CoordinationMessage msg)
{
    CALL_DEBUG_ENTER;
    std::lock_guard<std::mutex> guard(mtx_);
    for (const auto &item : devcoordinationListeners_) {
        item->OnCoordinationMessage(deviceId, msg);
    }
}

void CoordinationManagerImpl::OnCoordinationMessageEvent(int32_t userData,
    const std::string deviceId, CoordinationMessage msg)
{
    CALL_DEBUG_ENTER;
    CHK_PID_AND_TID();
    std::lock_guard<std::mutex> guard(mtx_);
    auto iter = devCoordinationEvents_.find(userData);
    if (iter == devCoordinationEvents_.end()) {
        return;
    }
    CoordinationMsg event = iter->second.msg;
    CHKPV(event);
    event(deviceId, msg);
    devCoordinationEvents_.erase(iter);
}

void CoordinationManagerImpl::OnCoordinationStateEvent(int32_t userData, bool state)
{
    CALL_DEBUG_ENTER;
    CHK_PID_AND_TID();
    std::lock_guard<std::mutex> guard(mtx_);
    auto iter = devCoordinationEvents_.find(userData);
    if (iter == devCoordinationEvents_.end()) {
        return;
    }
    CoordinationState event = iter->second.state;
    CHKPV(event);
    event(state);
    devCoordinationEvents_.erase(iter);
    FI_HILOGD("Coordination state event callback, userData:%{public}d, state:(%{public}d)", userData, state);
}

int32_t CoordinationManagerImpl::GetUserData() const
{
    std::lock_guard<std::mutex> guard(mtx_);
    return userData_;
}

const CoordinationManagerImpl::CoordinationMsg *CoordinationManagerImpl::GetCoordinationMessageEvent(
    int32_t userData) const
{
    auto iter = devCoordinationEvents_.find(userData);
    return iter == devCoordinationEvents_.end() ? nullptr : &iter->second.msg;
}

const CoordinationManagerImpl::CoordinationState *CoordinationManagerImpl::GetCoordinationStateEvent(
    int32_t userData) const
{
    auto iter = devCoordinationEvents_.find(userData);
    return iter == devCoordinationEvents_.end() ? nullptr : &iter->second.state;
}

int32_t CoordinationManagerImpl::OnCoordinationListener(const StreamClient& client, NetPacket& pkt)
{
    CALL_DEBUG_ENTER;
    int32_t userData;
    std::string deviceId;
    int32_t nType;
    pkt >> userData >> deviceId >> nType;
    if (pkt.ChkRWError()) {
        FI_HILOGE("Packet read type failed");
        return RET_ERR;
    }
    OnDevCoordinationListener(deviceId, CoordinationMessage(nType));
    return RET_OK;
}

int32_t CoordinationManagerImpl::OnCoordinationMessage(const StreamClient& client, NetPacket& pkt)
{
    CALL_DEBUG_ENTER;
    int32_t userData;
    std::string deviceId;
    int32_t nType;
    pkt >> userData >> deviceId >> nType;
    if (pkt.ChkRWError()) {
        FI_HILOGE("Packet read coordination msg failed");
        return RET_ERR;
    }
    OnCoordinationMessageEvent(userData, deviceId, CoordinationMessage(nType));
    return RET_OK;
}

int32_t CoordinationManagerImpl::OnCoordinationState(const StreamClient& client, NetPacket& pkt)
{
    CALL_DEBUG_ENTER;
    int32_t userData;
    bool state;
    pkt >> userData >> state;
    if (pkt.ChkRWError()) {
        FI_HILOGE("Packet read coordination msg failed");
        return RET_ERR;
    }
    OnCoordinationStateEvent(userData, state);
    return RET_OK;
}
} // namespace DeviceStatus
} // namespace Msdp
} // namespace OHOS