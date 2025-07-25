/*
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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

#include "cooperate_client.h"
#include "cooperate_hisysevent.h"

#ifdef ENABLE_PERFORMANCE_CHECK
#include <algorithm>
#include <numeric>
#endif // ENABLE_PERFORMANCE_CHECK

#include "devicestatus_define.h"
#include "intention_client.h"
#include "utility.h"

#undef LOG_TAG
#define LOG_TAG "CooperateClient"

namespace OHOS {
namespace Msdp {
namespace DeviceStatus {
namespace {
#ifdef ENABLE_PERFORMANCE_CHECK
constexpr int32_t PERCENTAGE { 100 };
constexpr int32_t FAILURE_DURATION { -100 };
constexpr int32_t INVALID_INDEX { -1 };
#endif // ENABLE_PERFORMANCE_CHECK
} // namespace

int32_t CooperateClient::RegisterListener(CooperateListenerPtr listener, bool isCheckPermission)
{
    CALL_DEBUG_ENTER;
    CHKPR(listener, RET_ERR);
    std::lock_guard<std::mutex> guard(mtx_);
    for (const auto &item : devCooperateListener_) {
        if (item == listener) {
            FI_HILOGE("The listener already exists");
            return RET_ERR;
        }
    }
    if (!isListeningProcess_) {
        FI_HILOGI("Start monitoring");
        if (int32_t ret = INTENTION_CLIENT->RegisterCooperateListener(); ret != RET_OK) {
            FI_HILOGE("Failed to register, ret:%{public}d", ret);
            return ret;
        }
        isListeningProcess_ = true;
    }
    devCooperateListener_.push_back(listener);
    return RET_OK;
}

int32_t CooperateClient::UnregisterListener(CooperateListenerPtr listener, bool isCheckPermission)
{
    CALL_DEBUG_ENTER;
    std::lock_guard<std::mutex> guard(mtx_);
    if (listener == nullptr) {
        devCooperateListener_.clear();
        goto listenerLabel;
    }
    for (auto it = devCooperateListener_.begin(); it != devCooperateListener_.end(); ++it) {
        if (*it == listener) {
            devCooperateListener_.erase(it);
            goto listenerLabel;
        }
    }

listenerLabel:
    if (isListeningProcess_ && devCooperateListener_.empty()) {
        isListeningProcess_ = false;
        return INTENTION_CLIENT->UnregisterCooperateListener();
    }
    return RET_OK;
}

int32_t CooperateClient::Enable(CooperateMessageCallback callback, bool isCheckPermission)
{
    CALL_DEBUG_ENTER;
    std::lock_guard<std::mutex> guard(mtx_);
    CooperateEvent event { callback };
    auto userId = GenerateRequestID();
    int32_t ret = INTENTION_CLIENT->EnableCooperate(userId);
    if (ret != RET_OK) {
        FI_HILOGE("Prepare cooperate failed");
        return ret;
    }
    devCooperateEvent_.insert_or_assign(userId, event);
    return RET_OK;
}

int32_t CooperateClient::Disable(CooperateMessageCallback callback, bool isCheckPermission)
{
    CALL_DEBUG_ENTER;
    std::lock_guard<std::mutex> guard(mtx_);
    CooperateEvent event { callback };
    auto userId = GenerateRequestID();
    int32_t ret = INTENTION_CLIENT->DisableCooperate(userId);
    if (ret != RET_OK) {
        FI_HILOGE("Unprepare cooperate failed");
        return ret;
    }
    devCooperateEvent_.insert_or_assign(userId, event);
    return RET_OK;
}

int32_t CooperateClient::Start(const std::string &remoteNetworkId,
    int32_t startDeviceId, CooperateMessageCallback callback, bool isCheckPermission)
{
    CALL_DEBUG_ENTER;
    std::lock_guard<std::mutex> guard(mtx_);
    CooperateEvent event { callback };
    auto userData = GenerateRequestID();
#ifdef ENABLE_PERFORMANCE_CHECK
    StartTrace(userData);
#endif // ENABLE_PERFORMANCE_CHECK
    int32_t ret = INTENTION_CLIENT->StartCooperate(remoteNetworkId, userData, startDeviceId, isCheckPermission);
    if (ret != RET_OK) {
        FI_HILOGE("Activate cooperate failed");
        return ret;
    }
    devCooperateEvent_.insert_or_assign(userData, event);
    return RET_OK;
}

int32_t CooperateClient::Stop(bool isUnchained, CooperateMessageCallback callback, bool isCheckPermission)
{
    CALL_DEBUG_ENTER;
    std::lock_guard<std::mutex> guard(mtx_);
    CooperateEvent event { callback };
    auto userData = GenerateRequestID();
#ifdef ENABLE_PERFORMANCE_CHECK
    StartTrace(userData);
#endif // ENABLE_PERFORMANCE_CHECK
    int32_t ret = INTENTION_CLIENT->StopCooperate(userData, isUnchained, isCheckPermission);
    if (ret != RET_OK) {
        FI_HILOGE("Deactivate cooperate failed");
        return ret;
    }
    devCooperateEvent_.insert_or_assign(userData, event);
    return RET_OK;
}

int32_t CooperateClient::GetCooperateState(const std::string &networkId, CooperateStateCallback callback,
    bool isCheckPermission)
{
    CALL_DEBUG_ENTER;
    std::lock_guard<std::mutex> guard(mtx_);
    CooperateEvent event { callback };
    auto userData = GenerateRequestID();
    int32_t ret = INTENTION_CLIENT->GetCooperateStateAsync(networkId, userData, isCheckPermission);
    if (ret != RET_OK) {
        FI_HILOGE("Get cooperate state failed");
        return ret;
    }
    devCooperateEvent_.insert_or_assign(userData, event);
    return RET_OK;
}

int32_t CooperateClient::GetCooperateState(const std::string &udId, bool &state)
{
    CALL_DEBUG_ENTER;
    std::lock_guard<std::mutex> guard(mtx_);
    if (INTENTION_CLIENT->GetCooperateStateSync(udId, state) != RET_OK) {
        FI_HILOGE("Get cooperate state failed udId: %{public}s", Utility::Anonymize(udId).c_str());
        return RET_ERR;
    }
    FI_HILOGI("GetCooperateState for udId: %{public}s successfully, state: %{public}s",
        Utility::Anonymize(udId).c_str(), state ? "true" : "false");
    return RET_OK;
}

int32_t CooperateClient::RegisterEventListener(const std::string &networkId, MouseLocationListenerPtr listener)
{
    CALL_DEBUG_ENTER;
    CHKPR(listener, COMMON_PARAMETER_ERROR);
    std::lock_guard<std::mutex> guard(mtx_);
    if (eventListener_.find(networkId) != eventListener_.end() &&
        eventListener_[networkId].find(listener) != eventListener_[networkId].end()) {
        FI_HILOGE("This listener for networkId:%{public}s already exists", Utility::Anonymize(networkId).c_str());
        return RET_ERR;
    }
    if (int32_t ret = INTENTION_CLIENT->RegisterMouseEventListener(networkId); ret != RET_OK) {
        FI_HILOGE("RegisterEventListener failed, ret:%{public}d", ret);
        return ret;
    }
    eventListener_[networkId].insert(listener);
    FI_HILOGI("Add listener for networkId:%{public}s successfully", Utility::Anonymize(networkId).c_str());
    return RET_OK;
}

int32_t CooperateClient::UnregisterEventListener(const std::string &networkId, MouseLocationListenerPtr listener)
{
    CALL_DEBUG_ENTER;
    std::lock_guard<std::mutex> guard(mtx_);
    if (eventListener_.find(networkId) == eventListener_.end()) {
        FI_HILOGE("No listener for networkId:%{public}s is registered", Utility::Anonymize(networkId).c_str());
        return RET_ERR;
    }
    if (eventListener_.find(networkId) != eventListener_.end() && listener != nullptr &&
        eventListener_[networkId].find(listener) == eventListener_[networkId].end()) {
        FI_HILOGE("Current listener for networkId:%{public}s is not registered", Utility::Anonymize(networkId).c_str());
        return RET_ERR;
    }
    if (listener == nullptr) {
        eventListener_.erase(networkId);
        FI_HILOGI("Remove all listener for networkId:%{public}s", Utility::Anonymize(networkId).c_str());
    } else {
        eventListener_[networkId].erase(listener);
        FI_HILOGI("Remove listener for networkId:%{public}s", Utility::Anonymize(networkId).c_str());
        if (eventListener_[networkId].empty()) {
            eventListener_.erase(networkId);
            FI_HILOGD("No listener for networkId:%{public}s, clean current networkId",
                Utility::Anonymize(networkId).c_str());
        }
    }
    if (eventListener_.find(networkId) != eventListener_.end()) {
        FI_HILOGD("UnregisterEventListener for networkId:%{public}s successfully",
            Utility::Anonymize(networkId).c_str());
        return RET_OK;
    }
    if (int32_t ret = INTENTION_CLIENT->UnregisterMouseEventListener(networkId); ret != RET_OK) {
        FI_HILOGE("UnregisterEventListener failed, ret:%{public}d", ret);
        return ret;
    }
    FI_HILOGD("Unregister all Listener for networkId:%{public}s successfully", Utility::Anonymize(networkId).c_str());
    return RET_OK;
}

int32_t CooperateClient::StartWithOptions(const std::string &remoteNetworkId,
    int32_t startDeviceId, CooperateMessageCallback callback, const CooperateOptions &options)
{
    CALL_DEBUG_ENTER;
    std::lock_guard<std::mutex> guard(mtx_);
    CooperateEvent event { callback };
    auto userData = GenerateRequestID();
#ifdef ENABLE_PERFORMANCE_CHECK
    StartTrace(userData);
#endif // ENABLE_PERFORMANCE_CHECK
    bool isCheckPermission = false;
    int32_t ret = INTENTION_CLIENT->StartCooperateWithOptions(remoteNetworkId, userData,
        startDeviceId, isCheckPermission, options);
    if (ret != RET_OK) {
        FI_HILOGE("Activate cooperate failed");
        return ret;
    }
    devCooperateEvent_.insert_or_assign(userData, event);
    return RET_OK;
}

int32_t CooperateClient::SetDamplingCoefficient(uint32_t direction, double coefficient)
{
    FI_HILOGI("SetDamplingCoefficient(0x%{public}x, %{public}.3f)", direction, coefficient);
    if (auto ret = INTENTION_CLIENT->SetDamplingCoefficient(direction, coefficient); ret != RET_OK) {
        FI_HILOGE("SetDamplingCoefficient failed, error:%{public}d", ret);
        return ret;
    }
    return RET_OK;
}

int32_t CooperateClient::AddHotAreaListener(HotAreaListenerPtr listener)
{
    CALL_DEBUG_ENTER;
    CHKPR(listener, RET_ERR);
    std::lock_guard<std::mutex> guard(mtx_);
    if (std::find(devHotAreaListener_.begin(), devHotAreaListener_.end(), listener) != devHotAreaListener_.end()) {
        FI_HILOGD("Current listener is registered already");
        return RET_ERR;
    }
    if (int32_t ret = INTENTION_CLIENT->RegisterHotAreaListener(GenerateRequestID(), false); ret != RET_OK) {
        FI_HILOGE("AddHotAreaListener failed, ret:%{public}d", ret);
        return ret;
    }
    devHotAreaListener_.push_back(listener);
    return RET_OK;
}

int32_t CooperateClient::RemoveHotAreaListener(HotAreaListenerPtr listener)
{
    CALL_DEBUG_ENTER;
    {
        std::lock_guard<std::mutex> guard(mtx_);
        if (listener != nullptr &&
            std::find(devHotAreaListener_.begin(), devHotAreaListener_.end(), listener) == devHotAreaListener_.end()) {
            FI_HILOGD("Current listener is not registered");
            return RET_ERR;
        }
        if (listener == nullptr) {
            devHotAreaListener_.clear();
        } else {
            for (auto it = devHotAreaListener_.begin(); it != devHotAreaListener_.end(); ++it) {
                if (*it == listener) {
                    devHotAreaListener_.erase(it);
                    break;
                }
            }
        }
        if (!devHotAreaListener_.empty()) {
            FI_HILOGI("RemoveHotAreaListener successfully");
            return RET_OK;
        }
    }
    if (int32_t ret = INTENTION_CLIENT->UnregisterHotAreaListener(GenerateRequestID(), false); ret != RET_OK) {
        FI_HILOGE("RemoveHotAreaListener failed, ret:%{public}d", ret);
        return ret;
    }
    FI_HILOGI("Remove all hot area listener successfully");
    return RET_OK;
}

int32_t CooperateClient::GenerateRequestID()
{
    static int32_t requestId { 0 };

    if (requestId == std::numeric_limits<int32_t>::max()) {
        FI_HILOGE("Request ID exceeds the maximum");
        requestId = 0;
    }
    return requestId++;
}

int32_t CooperateClient::OnCoordinationListener(const StreamClient &client, NetPacket &pkt)
{
    CALL_INFO_TRACE;
    int32_t userData = 0;
    std::string networkId;
    int32_t nType = 0;
    pkt >> userData >> networkId >> nType;
    if (pkt.ChkRWError()) {
        FI_HILOGE("Packet read type failed");
        return RET_ERR;
    }
    FI_HILOGI("NetworkId:%{public}s, nType:%{public}d", Utility::Anonymize(networkId).c_str(), nType);
    OnDevCooperateListener(networkId, CoordinationMessage(nType));
    return RET_OK;
}

void CooperateClient::OnDevCooperateListener(const std::string &networkId, CoordinationMessage msg)
{
    CALL_INFO_TRACE;
    std::lock_guard<std::mutex> guard(mtx_);
    for (const auto &item : devCooperateListener_) {
        item->OnCoordinationMessage(networkId, msg);
    }
}

int32_t CooperateClient::OnCoordinationMessage(const StreamClient &client, NetPacket &pkt)
{
    CALL_INFO_TRACE;
    int32_t userData = 0;
    std::string networkId;
    int32_t nType = 0;
    int32_t errCode = -1;
    pkt >> userData >> networkId >> nType >> errCode;
    if (pkt.ChkRWError()) {
        FI_HILOGE("Packet read coordination msg failed");
        return RET_ERR;
    }
#ifdef ENABLE_PERFORMANCE_CHECK
    FinishTrace(userData, CoordinationMessage(nType));
#endif // ENABLE_PERFORMANCE_CHECK
    FI_HILOGI("NetworkId:%{public}s, nType:%{public}d", Utility::Anonymize(networkId).c_str(), nType);
    CoordinationMsgInfo msgInfo {
        .msg = static_cast<CoordinationMessage> (nType),
        .errCode = errCode
    };
    OnCooperateMessageEvent(userData, networkId, msgInfo);
    CooperateRadarInfo radarInfo {
        .funcName = __FUNCTION__,
        .bizState = static_cast<int32_t> (BizState::STATE_END),
        .bizStage = static_cast<int32_t> (BizCooperateStage::STAGE_CLIENT_ON_MESSAGE_RCVD),
        .stageRes = static_cast<int32_t> (BizCooperateStageRes::RES_IDLE),
        .bizScene = static_cast<int32_t> (BizCooperateScene::SCENE_ACTIVE),
        .errCode = static_cast<int32_t> (msgInfo.errCode),
        .hostName = "",
        .localNetId = "",
        .peerNetId = Utility::DFXRadarAnonymize(networkId.c_str())
    };
    if (CoordinationMessage(nType) == CoordinationMessage::ACTIVATE_SUCCESS) {
        radarInfo.stageRes = static_cast<int32_t> (BizCooperateStageRes::RES_SUCCESS);
        CooperateRadar::ReportCooperateRadarInfo(radarInfo);
    } else if (CoordinationMessage(nType) == CoordinationMessage::ACTIVATE_FAIL) {
        radarInfo.stageRes = static_cast<int32_t> (BizCooperateStageRes::RES_FAIL);
        CooperateRadar::ReportCooperateRadarInfo(radarInfo);
    }
    return RET_OK;
}

void CooperateClient::OnCooperateMessageEvent(int32_t userData,
    const std::string &networkId, const CoordinationMsgInfo &msgInfo)
{
    CALL_INFO_TRACE;
    CHK_PID_AND_TID();
    std::lock_guard<std::mutex> guard(mtx_);
    auto iter = devCooperateEvent_.find(userData);
    if (iter == devCooperateEvent_.end()) {
        return;
    }
    CooperateMessageCallback callback = iter->second.msgCb;
    CHKPV(callback);
    callback(networkId, msgInfo);
    devCooperateEvent_.erase(iter);
}

int32_t CooperateClient::OnCoordinationState(const StreamClient &client, NetPacket &pkt)
{
    CALL_INFO_TRACE;
    int32_t userData = 0;
    bool state = false;
    int32_t errCode = -1;
    pkt >> userData >> state >> errCode;
    if (pkt.ChkRWError()) {
        FI_HILOGE("Packet read coordination msg failed");
        return RET_ERR;
    }
    FI_HILOGI("State%{public}s", state ? "true" : "false");
    OnCooperateStateEvent(userData, state);
    return RET_OK;
}

void CooperateClient::OnCooperateStateEvent(int32_t userData, bool state)
{
    CALL_INFO_TRACE;
    CHK_PID_AND_TID();
    std::lock_guard<std::mutex> guard(mtx_);
    auto iter = devCooperateEvent_.find(userData);
    if (iter == devCooperateEvent_.end()) {
        return;
    }
    CooperateStateCallback event = iter->second.stateCb;
    CHKPV(event);
    event(state);
    devCooperateEvent_.erase(iter);
    FI_HILOGD("Coordination state event callback, userData:%{public}d, state:(%{public}d)", userData, state);
}

int32_t CooperateClient::OnHotAreaListener(const StreamClient &client, NetPacket &pkt)
{
    CALL_DEBUG_ENTER;
    int32_t positionX = 0;
    int32_t positionY = 0;
    int32_t type = 0;
    bool isEdge = false;
    pkt >> positionX >> positionY >> type >> isEdge;
    if (pkt.ChkRWError()) {
        FI_HILOGE("Packet read type failed");
        return RET_ERR;
    }
    OnDevHotAreaListener(positionX, positionY, HotAreaType(type), isEdge);
    return RET_OK;
}

int32_t CooperateClient::OnMouseLocationListener(const StreamClient &client, NetPacket &pkt)
{
    CALL_DEBUG_ENTER;
    std::string networkId;
    Event event;
    pkt >> networkId >> event.displayX >> event.displayY >> event.displayWidth >> event.displayHeight;
    if (pkt.ChkRWError()) {
        FI_HILOGE("Packet read type failed");
        return RET_ERR;
    }
    OnDevMouseLocationListener(networkId, event);
    return RET_OK;
}

void CooperateClient::OnDevHotAreaListener(int32_t displayX,
    int32_t displayY, HotAreaType type, bool isEdge)
{
    CALL_DEBUG_ENTER;
    std::lock_guard<std::mutex> guard(mtx_);
    for (const auto &item : devHotAreaListener_) {
        item->OnHotAreaMessage(displayX, displayY, type, isEdge);
    }
}

void CooperateClient::OnDevMouseLocationListener(const std::string &networkId, const Event &event)
{
    CALL_DEBUG_ENTER;
    std::lock_guard<std::mutex> guard(mtx_);
    if (eventListener_.find(networkId) == eventListener_.end()) {
        FI_HILOGI("No listener for networkId:%{public}s is registered", Utility::Anonymize(networkId).c_str());
        return;
    }
    for (const auto &listener : eventListener_[networkId]) {
            CHKPC(listener);
            listener->OnMouseLocationEvent(networkId, event);
            FI_HILOGD("Trigger listener for networkId:%{public}s,"
            "displayX:%{private}d, displayY:%{private}d, displayWidth:%{public}d, displayHeight:%{public}d",
                Utility::Anonymize(networkId).c_str(), event.displayX, event.displayY,
                event.displayWidth, event.displayHeight);
    }
}

void CooperateClient::OnConnected()
{
    CALL_INFO_TRACE;
    if (connectedCooperateListeners_.empty()) {
        FI_HILOGE("The connect cooperate listener list is empty");
        return;
    }
    for (const auto &listener : connectedCooperateListeners_) {
        if (RegisterListener(listener) != RET_OK) {
            FI_HILOGW("AddCooperatelistener failed");
        }
    }
}

void CooperateClient::OnDisconnected()
{
    CALL_INFO_TRACE;
    if (isListeningProcess_) {
        std::lock_guard<std::mutex> guard(mtx_);
        if (devCooperateListener_.empty()) {
            FI_HILOGE("The cooperate listener list is empty");
            return;
        }
        connectedCooperateListeners_ = devCooperateListener_;
        devCooperateListener_.clear();
        isListeningProcess_ = false;
    }
}

#ifdef ENABLE_PERFORMANCE_CHECK
int32_t CooperateClient::GetFirstSuccessIndex()
{
    CALL_DEBUG_ENTER;
    size_t durationLen =  performanceInfo_.durationList.size();
    for (size_t i = 0; i < durationLen; ++i) {
        if (performanceInfo_.durationList[i] != FAILURE_DURATION) {
            performanceInfo_.successNum = 1;
            FI_HILOGI("[PERF] First success index:%{public}zu", i);
            return static_cast<int32_t>(i);
        }
    }
    return INVALID_INDEX;
}
void CooperateClient::StartTrace(int32_t userData)
{
    CALL_DEBUG_ENTER;
    std::lock_guard guard { performanceLock_ };
    performanceInfo_.traces_.emplace(userData, std::chrono::steady_clock::now());
    performanceInfo_.activateNum += 1;
    FI_HILOGI("[PERF] Start tracing \'%{public}d\'", userData);
}

void CooperateClient::FinishTrace(int32_t userData, CoordinationMessage msg)
{
    CALL_DEBUG_ENTER;
    std::lock_guard guard { performanceLock_ };
    if (msg == CoordinationMessage::ACTIVATE_SUCCESS) {
        if (auto iter = performanceInfo_.traces_.find(userData); iter != performanceInfo_.traces_.end()) {
            auto curDuration = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - iter->second).count();
            FI_HILOGI("[PERF] Finish tracing \'%{public}d\', elapsed: %{public}lld ms", userData, curDuration);
            performanceInfo_.traces_.erase(iter);
            performanceInfo_.durationList.push_back(curDuration);
        } else {
            FI_HILOGW("[PERF] FinishTrace with something wrong");
        }
    } else if (msg == CoordinationMessage::ACTIVATE_FAIL) {
        FI_HILOGW("[PERF] Activate coordination failed");
        performanceInfo_.traces_.erase(userData);
        performanceInfo_.durationList.push_back(FAILURE_DURATION);
    }
}

void CooperateClient::DumpPerformanceInfo()
{
    CALL_DEBUG_ENTER;
    std::lock_guard guard { performanceLock_ };
    int32_t firstSuccessIndex = GetFirstSuccessIndex();
    int32_t durationLen = static_cast<int32_t>(performanceInfo_.durationList.size());
    if (firstSuccessIndex < 0 || firstSuccessIndex >= durationLen) {
        FI_HILOGE("[PERF] DumpPerformanceInfo failed, invalid first success index");
        return;
    }
    performanceInfo_.failNum = firstSuccessIndex;
    performanceInfo_.failBeforeSuccess = firstSuccessIndex;
    performanceInfo_.firstSuccessDuration = performanceInfo_.durationList[firstSuccessIndex];
    int32_t successDurationSumWithoutFirst { 0 };
    for (int32_t i = firstSuccessIndex + 1; i < durationLen; i++) {
        if (performanceInfo_.durationList[i] != FAILURE_DURATION) {
            successDurationSumWithoutFirst += performanceInfo_.durationList[i];
            performanceInfo_.minDuration = std::min(performanceInfo_.durationList[i], performanceInfo_.minDuration);
            performanceInfo_.maxDuration = std::max(performanceInfo_.durationList[i], performanceInfo_.maxDuration);
            performanceInfo_.successNum += 1;
        } else {
            performanceInfo_.failNum += 1;
        }
    }
    int32_t validActivateNum = performanceInfo_.activateNum - performanceInfo_.failBeforeSuccess;
    if (validActivateNum > 0) {
        performanceInfo_.successRate = (static_cast<float>(performanceInfo_.successNum) * PERCENTAGE) /
            validActivateNum;
    }
    if (int32_t successNumWithoutFirst = performanceInfo_.successNum - 1; successNumWithoutFirst > 0) {
        performanceInfo_.averageDuration = successDurationSumWithoutFirst / successNumWithoutFirst;
    }
    FI_HILOGI("[PERF] performanceInfo:"
        "activateNum:%{public}d successNum:%{public}d failNum:%{public}d successRate:%{public}.2f "
        "averageDuration:%{public}d ms maxDuration:%{public}d ms minDuration:%{public}d ms failBeforeSucc:%{public}d "
        "firstSuccessDuration:%{public}d ms",
        performanceInfo_.activateNum, performanceInfo_.successNum, performanceInfo_.failNum,
        performanceInfo_.successRate, performanceInfo_.averageDuration, performanceInfo_.maxDuration,
        performanceInfo_.minDuration, performanceInfo_.failBeforeSuccess, performanceInfo_.firstSuccessDuration);
    std::string durationStr;
    for (auto duration : performanceInfo_.durationList) {
        durationStr += std::to_string(duration) + ", ";
    }
    FI_HILOGI("[PERF] Duration: %{public}s", durationStr.c_str());
    performanceInfo_ = PerformanceInfo();
}
#endif // ENABLE_PERFORMANCE_CHECK
} // namespace DeviceStatus
} // namespace Msdp
} // namespace OHOS
