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

#include "coordination_sm.h"

#include <cstdio>
#include <unistd.h>

#include "device_manager.h"
#include "display_info.h"
#include "display_manager.h"
#include "hitrace_meter.h"
#include "input_manager.h"

#include "coordination_device_manager.h"
#include "coordination_event_manager.h"
#include "coordination_message.h"
#include "coordination_softbus_adapter.h"
#include "coordination_state_free.h"
#include "coordination_state_in.h"
#include "coordination_state_out.h"
#include "coordination_util.h"
#include "device_profile_adapter.h"

namespace OHOS {
namespace Msdp {
namespace DeviceStatus {
namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL { LOG_CORE, MSDP_DOMAIN_ID, "CoordinationSM" };
constexpr int32_t INTERVAL_MS { 2000 };
constexpr double PERCENT_CONST { 100.0 };
constexpr float MOUSE_ABS_LOCATION { 100 };
constexpr int32_t MOUSE_ABS_LOCATION_X { 50 };
constexpr int32_t MOUSE_ABS_LOCATION_Y { 50 };
constexpr int32_t COORDINATION_PRIORITY { 499 };
constexpr int32_t MIN_HANDLER_ID { 1 };
} // namespace

CoordinationSM::CoordinationSM() {}
CoordinationSM::~CoordinationSM()
{
    RemoveMonitor();
    RemoveInterceptor();
}

void CoordinationSM::Init()
{
    CALL_INFO_TRACE;
    preparedNetworkId_ = std::make_pair("", "");
    coordinationStates_.emplace(CoordinationState::STATE_FREE, std::make_shared<CoordinationStateFree>());
    coordinationStates_.emplace(CoordinationState::STATE_IN, std::make_shared<CoordinationStateIn>());
    coordinationStates_.emplace(CoordinationState::STATE_OUT, std::make_shared<CoordinationStateOut>());
    auto *context = COOR_EVENT_MGR->GetIContext();
    CHKPV(context);
    context->GetTimerManager().AddTimer(INTERVAL_MS, 1, [this]() {
        this->InitDeviceManager();
        COOR_SOFTBUS_ADAPTER->Init();
    });
    COOR_DEV_MGR->Init();
    runner_ = AppExecFwk::EventRunner::Create(true);
    CHKPL(runner_);
    eventHandler_ = std::make_shared<CoordinationEventHandler>(runner_);
}
void CoordinationSM::OnSoftbusSessionClosed(const std::string &NetworkId)
{
    CALL_INFO_TRACE;
    CHKPV(eventHandler_);
    std::string taskName = "process_coordinition_reset";
    std::function<void()> handleFunc =
        std::bind(&CoordinationSM::OnReset, this, NetworkId);
    eventHandler_->ProxyPostTask(handleFunc, taskName, 0);
}

void CoordinationSM::OnReset(const std::string &NetworkId)
{
    CALL_INFO_TRACE;
    Reset(NetworkId);
}

void CoordinationSM::OnSessionLost(SessionPtr session)
{
    CALL_DEBUG_ENTER;
    CHKPV(session);
    sptr<CoordinationEventManager::EventInfo> event = new (std::nothrow) CoordinationEventManager::EventInfo();
    CHKPV(event);
    event->type = CoordinationEventManager::EventType::LISTENER;
    event->sess = session;
    COOR_EVENT_MGR->RemoveCoordinationEvent(event);
    RemoveMonitor();
    RemoveInterceptor();
    if (currentState_ != CoordinationState::STATE_FREE) {
        DeactivateCoordination(COOR_SM->isUnchained_);
    }
}

void CoordinationSM::Reset(const std::string &networkId)
{
    CALL_INFO_TRACE;
    std::lock_guard<std::mutex> guard(mutex_);
    bool needReset = true;
    if (currentState_ == CoordinationState::STATE_OUT) {
        if (networkId != remoteNetworkId_) {
            needReset = false;
        }
    }
    if (currentState_ == CoordinationState::STATE_IN) {
        std::string originNetworkId = COOR_DEV_MGR->GetOriginNetworkId(startDeviceDhid_);
        if (networkId != originNetworkId) {
            needReset = false;
        }
    }
    if (needReset) {
        Reset(true);
        SetPointerVisible();
    }
}

void CoordinationSM::Reset(bool adjustAbsolutionLocation)
{
    CALL_INFO_TRACE;
    startDeviceDhid_ = "";
    remoteNetworkId_ = "";
    currentState_ = CoordinationState::STATE_FREE;
    if (adjustAbsolutionLocation) {
        SetAbsolutionLocation(MOUSE_ABS_LOCATION_X, MOUSE_ABS_LOCATION_Y);
    }
    isStarting_ = false;
    isStopping_ = false;
    RemoveInterceptor();
}

void CoordinationSM::OnCoordinationChanged(const std::string &networkId, bool isOpen)
{
    CALL_DEBUG_ENTER;
    CoordinationMessage msg = isOpen ? CoordinationMessage::PREPARE : CoordinationMessage::UNPREPARE;
    auto *context = COOR_EVENT_MGR->GetIContext();
    CHKPV(context);
    int32_t ret = context->GetDelegateTasks().PostAsyncTask(
        std::bind(&CoordinationEventManager::OnCoordinationMessage, COOR_EVENT_MGR, msg, networkId));
    if (ret != RET_OK) {
        FI_HILOGE("Posting async task failed");
    }
    if (!isOpen) {
        OnCloseCoordination(networkId, false);
    }
}

void CoordinationSM::OnCloseCoordination(const std::string &networkId, bool isLocal)
{
    CALL_INFO_TRACE;
    std::lock_guard<std::mutex> guard(mutex_);
    if (!preparedNetworkId_.first.empty() && !preparedNetworkId_.second.empty()) {
        if (networkId == preparedNetworkId_.first || networkId == preparedNetworkId_.second) {
            if (currentState_ != CoordinationState::STATE_FREE) {
                D_INPUT_ADAPTER->StopRemoteInput(preparedNetworkId_.first, preparedNetworkId_.second,
                    COOR_DEV_MGR->GetCoordinationDhids(startDeviceDhid_), [](bool isSuccess) {
                    FI_HILOGI("Failed to stop remote");
                });
            }
            D_INPUT_ADAPTER->UnPrepareRemoteInput(preparedNetworkId_.first, preparedNetworkId_.second,
                [](bool isSuccess) {});
        }
    }
    preparedNetworkId_ = std::make_pair("", "");
    if (currentState_ == CoordinationState::STATE_FREE) {
        return;
    }
    if (isLocal || networkId == remoteNetworkId_) {
        Reset(true);
        SetPointerVisible();
        return;
    }
    if (COOR_DEV_MGR->GetOriginNetworkId(startDeviceDhid_) == networkId) {
        Reset();
        SetPointerVisible();
    }
}

int32_t CoordinationSM::GetCoordinationState(const std::string &deviceId)
{
    CALL_INFO_TRACE;
    if (deviceId.empty()) {
        FI_HILOGE("DeviceId is empty");
        return static_cast<int32_t>(CoordinationMessage::PARAMETER_ERROR);
    }
    bool state = DP_ADAPTER->GetCrossingSwitchState(deviceId);
    COOR_EVENT_MGR->OnGetCrossingSwitchState(state);
    return RET_OK;
}

void CoordinationSM::PrepareCoordination()
{
    CALL_INFO_TRACE;
    if (monitorId_ <= 0) {
        auto monitor = std::make_shared<MonitorConsumer>(
            std::bind(&CoordinationSM::UpdateLastPointerEventCallback, this, std::placeholders::_1));
        monitorId_ = MMI::InputManager::GetInstance()->AddMonitor(monitor);
        if (monitorId_ <= 0) {
            FI_HILOGE("Failed to add monitor, error code:%{public}d", monitorId_);
            monitorId_ = -1;
            return;
        }
    }
    DP_ADAPTER->UpdateCrossingSwitchState(true, onlineDevice_);
}

void CoordinationSM::UnprepareCoordination()
{
    CALL_INFO_TRACE;
    DP_ADAPTER->UpdateCrossingSwitchState(false, onlineDevice_);
    std::string localNetworkId = COORDINATION::GetLocalNetworkId();
    OnCloseCoordination(localNetworkId, true);
    RemoveMonitor();
}

int32_t CoordinationSM::ActivateCoordination(const std::string &remoteNetworkId, int32_t startDeviceId)
{
    CALL_INFO_TRACE;
    std::lock_guard<std::mutex> guard(mutex_);
    if (isStarting_) {
        FI_HILOGE("In transition state, not process");
        return static_cast<int32_t>(CoordinationMessage::COORDINATION_FAIL);
    }
    if (COOR_SOFTBUS_ADAPTER->OpenInputSoftbus(remoteNetworkId) != RET_OK) {
        FI_HILOGE("Open input softbus fail");
        return static_cast<int32_t>(CoordinationMessage::COORDINATION_FAIL);
    }
    isStarting_ = true;
    SetSinkNetworkId(remoteNetworkId);
    auto state = GetCurrentState();
    CHKPR(state, ERROR_NULL_POINTER);
    int32_t ret = state->ActivateCoordination(remoteNetworkId, startDeviceId);
    if (ret != RET_OK) {
        FI_HILOGE("Start remote input fail");
        isStarting_ = false;
        return ret;
    }
    UpdateMouseLocation();
    if (currentState_ == CoordinationState::STATE_FREE) {
        remoteNetworkId_ = remoteNetworkId;
    }
    return ret;
}

int32_t CoordinationSM::DeactivateCoordination(bool isUnchained)
{
    CALL_INFO_TRACE;
    std::lock_guard<std::mutex> guard(mutex_);
    if (isStopping_) {
        FI_HILOGE("In transition state, not process");
        return RET_ERR;
    }

    isStopping_ = true;
    std::string stopNetworkId;
    if (currentState_ == CoordinationState::STATE_IN) {
        stopNetworkId = COOR_DEV_MGR->GetOriginNetworkId(startDeviceDhid_);
    } else if (currentState_ == CoordinationState::STATE_OUT) {
        stopNetworkId = remoteNetworkId_;
    } else {
        stopNetworkId = sinkNetworkId_;
    }
    isUnchained_ = isUnchained;
    FI_HILOGD("isUnchained_:%{public}d, stopNetworkId:%{public}s",
        isUnchained_, stopNetworkId.substr(0, SUBSTR_NETWORKID_LEN).c_str());
    auto state = GetCurrentState();
    CHKPR(state, ERROR_NULL_POINTER);
    int32_t ret = state->DeactivateCoordination(stopNetworkId, isUnchained, preparedNetworkId_);
    if (ret != RET_OK) {
        FI_HILOGE("Stop input device coordination fail");
        isStopping_ = false;
    }
    CHKPR(notifyDragCancelCallback_, ERROR_NULL_POINTER);
    notifyDragCancelCallback_();
    return ret;
}

void CoordinationSM::RegisterNotifyDragCancel(std::function<void(void)> callback)
{
    CALL_DEBUG_ENTER;
    CHKPV(callback);
    notifyDragCancelCallback_ = callback;
}

void CoordinationSM::StartRemoteCoordination(const std::string &remoteNetworkId, bool buttonIsPressed)
{
    CALL_INFO_TRACE;
    std::lock_guard<std::mutex> guard(mutex_);
    auto *context = COOR_EVENT_MGR->GetIContext();
    CHKPV(context);
    COOR_SM->SetSinkNetworkId(remoteNetworkId);
    FI_HILOGD("The remoteNetworkId:%{public}s", remoteNetworkId.substr(0, SUBSTR_NETWORKID_LEN).c_str());
    int32_t ret = context->GetDelegateTasks().PostAsyncTask(std::bind(&CoordinationEventManager::OnCoordinationMessage,
        COOR_EVENT_MGR, CoordinationMessage::ACTIVATE, remoteNetworkId));
    if (ret != RET_OK) {
        FI_HILOGE("Posting async task failed");
    }
    isStarting_ = true;
    if (buttonIsPressed) {
        MMI::InputManager::GetInstance()->EnableInputDevice(true);
        StartPointerEventFilter();
        COOR_SOFTBUS_ADAPTER->NotifyFilterAdded(sinkNetworkId_);
    }
    NotifyRemoteNetworkId(remoteNetworkId);
}

void CoordinationSM::StartPointerEventFilter()
{
    CALL_INFO_TRACE;
    int32_t POINTER_DEFAULT_PRIORITY = 220;
    auto filter = std::make_shared<PointerFilter>();
    uint32_t touchTags = CapabilityToTags(MMI::INPUT_DEV_CAP_POINTER);
    FI_HILOGE("Touchtags:%{public}d", static_cast<int32_t>(touchTags));
    if (filterId_ >= 0) {
        MMI::InputManager::GetInstance()->RemoveInputEventFilter(filterId_);
    }
    filterId_ =
        MMI::InputManager::GetInstance()->AddInputEventFilter(filter, POINTER_DEFAULT_PRIORITY, touchTags);
    if (0 > filterId_) {
        FI_HILOGE("Add Event Filter Failed");
    }
    filter->UpdateCurrentFilterId(filterId_);
}

void CoordinationSM::StartRemoteCoordinationResult(bool isSuccess, const std::string &startDeviceDhid, int32_t xPercent,
    int32_t yPercent)
{
    CALL_INFO_TRACE;
    std::lock_guard<std::mutex> guard(mutex_);
    if (!isStarting_) {
        FI_HILOGI("Not in starting");
        return;
    }
    startDeviceDhid_ = startDeviceDhid;
    CoordinationMessage msg = isSuccess ? CoordinationMessage::ACTIVATE_SUCCESS : CoordinationMessage::ACTIVATE_FAIL;
    auto *context = COOR_EVENT_MGR->GetIContext();
    CHKPV(context);
    int32_t ret = context->GetDelegateTasks().PostAsyncTask(
        std::bind(&CoordinationEventManager::OnCoordinationMessage, COOR_EVENT_MGR, msg, ""));
    if (ret != RET_OK) {
        FI_HILOGE("Posting async task failed");
    }

    if (!isSuccess || currentState_ == CoordinationState::STATE_IN) {
        isStarting_ = false;
        return;
    }
    if (currentState_ == CoordinationState::STATE_FREE) {
        NotifyMouseLocation(xPercent, yPercent);
        UpdateState(CoordinationState::STATE_IN);
#ifdef OHOS_BUILD_ENABLE_MOTION_DRAG
        NotifyRemoteNetworkId(COOR_DEV_MGR->GetOriginNetworkId(startDeviceDhid_));
        StateChangedNotify(CoordinationState::STATE_FREE, CoordinationState::STATE_IN);
#else
        SetAbsolutionLocation(MOUSE_ABS_LOCATION - xPercent, yPercent);
        MMI::InputManager::GetInstance()->SetPointerVisible(true);
#endif // OHOS_BUILD_ENABLE_MOTION_DRAG
    }
    if (currentState_ == CoordinationState::STATE_OUT) {
        NotifyMouseLocation(xPercent, yPercent);
        UpdateState(CoordinationState::STATE_FREE);
#ifdef OHOS_BUILD_ENABLE_MOTION_DRAG
        NotifyRemoteNetworkId(remoteNetworkId_);
        StateChangedNotify(CoordinationState::STATE_OUT, CoordinationState::STATE_FREE);
#else
        SetAbsolutionLocation(MOUSE_ABS_LOCATION - xPercent, yPercent);
        MMI::InputManager::GetInstance()->SetPointerVisible(true);
#endif // OHOS_BUILD_ENABLE_MOTION_DRAG
    }
    isStarting_ = false;
}

void CoordinationSM::StopRemoteCoordination(bool isUnchained)
{
    CALL_INFO_TRACE;
    std::lock_guard<std::mutex> guard(mutex_);
    isStopping_ = true;
    isUnchained_ = isUnchained;
}

void CoordinationSM::StopRemoteCoordinationResult(bool isSuccess)
{
    CALL_INFO_TRACE;
    std::lock_guard<std::mutex> guard(mutex_);
    if (!isStopping_) {
        FI_HILOGI("Not in stopping");
        return;
    }
    if (isSuccess) {
        Reset(true);
        SetPointerVisible();
    }
    if (!preparedNetworkId_.first.empty() && !preparedNetworkId_.second.empty() && isUnchained_) {
        FI_HILOGI("The sink preparedNetworkId isn't empty, first:%{public}s, second:%{public}s",
            preparedNetworkId_.first.c_str(), preparedNetworkId_.second.c_str());
        bool ret = UnchainCoordination(preparedNetworkId_.first, preparedNetworkId_.second);
        if (ret) {
            COOR_SM->NotifyChainRemoved();
            std::string localNetworkId = COORDINATION::GetLocalNetworkId();
            FI_HILOGD("localNetworkId:%{public}s", localNetworkId.substr(0, SUBSTR_NETWORKID_LEN).c_str());
            COOR_SOFTBUS_ADAPTER->NotifyUnchainedResult(localNetworkId, sinkNetworkId_, ret);
        } else {
            FI_HILOGE("Failed to unchain coordination");
        }
        isUnchained_ = false;
    }
    isStopping_ = false;
}

void CoordinationSM::StartCoordinationOtherResult(const std::string &remoteNetworkId)
{
    CALL_INFO_TRACE;
    std::lock_guard<std::mutex> guard(mutex_);
    remoteNetworkId_ = remoteNetworkId;
}

void CoordinationSM::OnStartFinish(bool isSuccess, const std::string &remoteNetworkId, int32_t startDeviceId)
{
    CALL_INFO_TRACE;
    std::lock_guard<std::mutex> guard(mutex_);
    if (!isStarting_) {
        FI_HILOGE("Not in starting");
        return;
    }

    if (!isSuccess) {
        FI_HILOGE("Start distributed fail, startDevice:%{public}d", startDeviceId);
        NotifyRemoteStartFail(remoteNetworkId);
    } else {
        startDeviceDhid_ = COOR_DEV_MGR->GetDhid(startDeviceId);
        if (currentState_ == CoordinationState::STATE_FREE) {
#ifdef OHOS_BUILD_ENABLE_MOTION_DRAG
            NotifyRemoteNetworkId(remoteNetworkId);
            NotifyMouseLocation(mouseLocation_.first, mouseLocation_.second);
            StateChangedNotify(CoordinationState::STATE_FREE, CoordinationState::STATE_OUT);
#endif // OHOS_BUILD_ENABLE_MOTION_DRAG
        } else if (currentState_ == CoordinationState::STATE_IN) {
            std::string originNetworkId = COOR_DEV_MGR->GetOriginNetworkId(startDeviceId);
            if (!originNetworkId.empty() && remoteNetworkId != originNetworkId) {
                COOR_SOFTBUS_ADAPTER->StartCoordinationOtherResult(originNetworkId, remoteNetworkId);
            }
#ifdef OHOS_BUILD_ENABLE_MOTION_DRAG
            NotifyRemoteNetworkId(originNetworkId);
            NotifyMouseLocation(mouseLocation_.first, mouseLocation_.second);
            StateChangedNotify(CoordinationState::STATE_IN, CoordinationState::STATE_FREE);
#endif // OHOS_BUILD_ENABLE_MOTION_DRAG
            SetPointerVisible();
        }
        NotifyRemoteStartSuccess(remoteNetworkId, startDeviceDhid_);
        if (currentState_ == CoordinationState::STATE_FREE) {
            UpdateState(CoordinationState::STATE_OUT);
        } else if (currentState_ == CoordinationState::STATE_IN) {
            UpdateState(CoordinationState::STATE_FREE);
        } else {
            FI_HILOGI("Current state is out");
        }
    }
    isStarting_ = false;
}

void CoordinationSM::OnStopFinish(bool isSuccess, const std::string &remoteNetworkId)
{
    CALL_INFO_TRACE;
    std::lock_guard<std::mutex> guard(mutex_);
    if (!isStopping_) {
        FI_HILOGE("Not in stopping");
        return;
    }
    NotifyRemoteStopFinish(isSuccess, remoteNetworkId);
    if (isSuccess) {
        if (COOR_DEV_MGR->HasLocalPointerDevice()) {
            MMI::InputManager::GetInstance()->SetPointerVisible(true);
            SetAbsolutionLocation(MOUSE_ABS_LOCATION_X, MOUSE_ABS_LOCATION_Y);
        }
        if (currentState_ == CoordinationState::STATE_IN || currentState_ == CoordinationState::STATE_OUT) {
            UpdateState(CoordinationState::STATE_FREE);
#ifdef OHOS_BUILD_ENABLE_MOTION_DRAG
            NotifyRemoteNetworkId(remoteNetworkId);
            StateChangedNotify(currentState_, CoordinationState::STATE_FREE);
#endif // OHOS_BUILD_ENABLE_MOTION_DRAG
        } else {
            FI_HILOGI("Current state is free");
        }
    }
    if (!preparedNetworkId_.first.empty() && !preparedNetworkId_.second.empty() && isUnchained_) {
        FI_HILOGI("The local preparedNetworkId isn't empty, first:%{public}s, second:%{public}s",
            preparedNetworkId_.first.c_str(), preparedNetworkId_.second.c_str());
        bool ret = UnchainCoordination(preparedNetworkId_.first, preparedNetworkId_.second);
        if (ret) {
            COOR_SM->NotifyChainRemoved();
            std::string localNetworkId = COORDINATION::GetLocalNetworkId();
            FI_HILOGD("localNetworkId:%{public}s", localNetworkId.substr(0, SUBSTR_NETWORKID_LEN).c_str());
            COOR_SOFTBUS_ADAPTER->NotifyUnchainedResult(localNetworkId, remoteNetworkId, ret);
        } else {
            FI_HILOGE("Failed to unchain coordination");
        }
    }
    if (!isUnchained_) {
        COOR_SOFTBUS_ADAPTER->CloseInputSoftbus(remoteNetworkId);
    }
    COOR_SM->SetPointerVisible();
    isUnchained_ = false;
    isStopping_ = false;
}

void CoordinationSM::NotifyRemoteStartFail(const std::string &remoteNetworkId)
{
    CALL_DEBUG_ENTER;
    COOR_SOFTBUS_ADAPTER->StartRemoteCoordinationResult(remoteNetworkId, false, "", 0, 0);
    COOR_EVENT_MGR->OnStart(CoordinationMessage::ACTIVATE_FAIL);
}

void CoordinationSM::NotifyRemoteStartSuccess(const std::string &remoteNetworkId, const std::string &startDeviceDhid)
{
    CALL_DEBUG_ENTER;
    COOR_SOFTBUS_ADAPTER->StartRemoteCoordinationResult(remoteNetworkId, true, startDeviceDhid, mouseLocation_.first,
        mouseLocation_.second);
    COOR_EVENT_MGR->OnStart(CoordinationMessage::ACTIVATE_SUCCESS);
}

void CoordinationSM::NotifyRemoteStopFinish(bool isSuccess, const std::string &remoteNetworkId)
{
    CALL_DEBUG_ENTER;
    COOR_SOFTBUS_ADAPTER->StopRemoteCoordinationResult(remoteNetworkId, isSuccess);
    if (!isSuccess) {
        COOR_EVENT_MGR->OnStop(CoordinationMessage::COORDINATION_FAIL);
    } else {
        COOR_EVENT_MGR->OnStop(CoordinationMessage::DEACTIVATE_SUCCESS);
    }
}

bool CoordinationSM::UpdateMouseLocation()
{
    CALL_DEBUG_ENTER;
    auto display = Rosen::DisplayManager::GetInstance().GetDefaultDisplay();
    CHKPF(display);
    int32_t width = display->GetWidth();
    int32_t height = display->GetHeight();
    if (width == 0 || height == 0) {
        FI_HILOGE("display width or height is 0");
        return false;
    }
    int32_t xPercent = displayX_ * MOUSE_ABS_LOCATION / width;
    int32_t yPercent = displayY_ * MOUSE_ABS_LOCATION / height;
    FI_HILOGI("displayWidth:%{public}d, displayHeight:%{public}d, "
        "physicalX:%{public}d, physicalY:%{public}d,",
        width, height, displayX_, displayY_);
    mouseLocation_ = std::make_pair(xPercent, yPercent);
    return true;
}

bool CoordinationSM::UnchainCoordination(const std::string &localNetworkId, const std::string &remoteNetworkId)
{
    CALL_DEBUG_ENTER;
    int32_t ret = D_INPUT_ADAPTER->UnPrepareRemoteInput(localNetworkId, remoteNetworkId, [](bool isSuccess) {});
    if (ret != RET_OK) {
        FI_HILOGE("Failed to call distributed UnprepareRemoteInput");
        return false;
    }
    preparedNetworkId_ = std::make_pair("", "");
    return true;
}

void CoordinationSM::UpdateState(CoordinationState state)
{
    FI_HILOGI("state:%{public}d", state);
    currentState_ = state;
    switch (state) {
        case CoordinationState::STATE_FREE: {
            Reset();
            MMI::InputManager::GetInstance()->EnableInputDevice(false);
            break;
        }
        case CoordinationState::STATE_IN: {
            MMI::InputManager::GetInstance()->SetPointerVisible(false);
            auto interceptor = std::make_shared<InterceptorConsumer>();
            MMI::InputManager::GetInstance()->EnableInputDevice(true);
            auto state = GetCurrentState();
            CHKPV(state);
            state->SetStartDeviceDhid(startDeviceDhid_);
            interceptorId_ = MMI::InputManager::GetInstance()->AddInterceptor(interceptor, COORDINATION_PRIORITY,
                CapabilityToTags(MMI::INPUT_DEV_CAP_KEYBOARD));
            if (interceptorId_ <= 0) {
                FI_HILOGE("Failed to add interceptor, Error code:%{public}d", interceptorId_);
                DeactivateCoordination(isUnchained_);
                return;
            }
            COOR_SOFTBUS_ADAPTER->ConfigTcpAlive();
            break;
        }
        case CoordinationState::STATE_OUT: {
            MMI::InputManager::GetInstance()->SetPointerVisible(false);
            auto interceptor = std::make_shared<InterceptorConsumer>();
            interceptorId_ = MMI::InputManager::GetInstance()->AddInterceptor(interceptor, COORDINATION_PRIORITY,
                CapabilityToTags(MMI::INPUT_DEV_CAP_KEYBOARD) | CapabilityToTags(MMI::INPUT_DEV_CAP_POINTER));
            auto state = GetCurrentState();
            CHKPV(state);
            state->SetStartDeviceDhid(startDeviceDhid_);
            if (interceptorId_ <= 0) {
                FI_HILOGE("Failed to add interceptor, Error code:%{public}d", interceptorId_);
                DeactivateCoordination(isUnchained_);
                return;
            }
            COOR_SOFTBUS_ADAPTER->ConfigTcpAlive();
            break;
        }
        default:
            break;
    }
}

CoordinationState CoordinationSM::GetCurrentCoordinationState() const
{
    CALL_DEBUG_ENTER;
    std::lock_guard<std::mutex> guard(mutex_);
    return currentState_;
}

void CoordinationSM::UpdatePreparedDevices(const std::string &remoteNetworkId, const std::string &originNetworkId)
{
    CALL_DEBUG_ENTER;
    std::lock_guard<std::mutex> guard(mutex_);
    preparedNetworkId_ = std::make_pair(remoteNetworkId, originNetworkId);
}

std::pair<std::string, std::string> CoordinationSM::GetPreparedDevices() const
{
    CALL_DEBUG_ENTER;
    std::lock_guard<std::mutex> guard(mutex_);
    return preparedNetworkId_;
}

bool CoordinationSM::IsStarting() const
{
    std::lock_guard<std::mutex> guard(mutex_);
    return isStarting_;
}

bool CoordinationSM::IsStopping() const
{
    std::lock_guard<std::mutex> guard(mutex_);
    return isStopping_;
}

void CoordinationSM::OnKeyboardOnline(const std::string &dhid)
{
    CALL_INFO_TRACE;
    std::lock_guard<std::mutex> guard(mutex_);
    auto state = GetCurrentState();
    CHKPV(state);
    state->OnKeyboardOnline(dhid, preparedNetworkId_);
}

void CoordinationSM::OnPointerOffline(const std::string &dhid, const std::vector<std::string> &keyboards)
{
    CALL_INFO_TRACE;
    std::lock_guard<std::mutex> guard(mutex_);
    if (currentState_ == CoordinationState::STATE_FREE) {
        FI_HILOGI("Current state: free");
        return;
    }
    if ((currentState_ == CoordinationState::STATE_IN) && (startDeviceDhid_ == dhid)) {
        Reset(true);
        SetPointerVisible();
        return;
    }
    if ((currentState_ == CoordinationState::STATE_OUT) && (startDeviceDhid_ == dhid)) {
        std::string remoteNetworkId = remoteNetworkId_;
        if (remoteNetworkId.empty()) {
            remoteNetworkId = preparedNetworkId_.first;
        }
        std::string localNetworkId = COORDINATION::GetLocalNetworkId();
        D_INPUT_ADAPTER->StopRemoteInput(remoteNetworkId, localNetworkId, keyboards,
            [this, remoteNetworkId](bool isSuccess) {});
        Reset(true);
        SetPointerVisible();
    }
}

void CoordinationSM::OnKeyboardOffline(const std::string &dhid)
{
    CALL_INFO_TRACE;
    if (currentState_ == CoordinationState::STATE_OUT) {
        std::string remoteNetworkId = remoteNetworkId_;
        if (remoteNetworkId.empty()) {
            remoteNetworkId = preparedNetworkId_.first;
        }
        std::string localNetworkId = COORDINATION::GetLocalNetworkId();
        std::vector<std::string> inputDeviceDhids;
        inputDeviceDhids.push_back(dhid);
        D_INPUT_ADAPTER->StopRemoteInput(remoteNetworkId, localNetworkId, inputDeviceDhids,
            [this, remoteNetworkId](bool isSuccess) {});
    }
}

bool CoordinationSM::InitDeviceManager()
{
    CALL_DEBUG_ENTER;
    initCallback_ = std::make_shared<DeviceInitCallBack>();
    int32_t ret = DIS_HARDWARE.InitDeviceManager(FI_PKG_NAME, initCallback_);
    if (ret != 0) {
        FI_HILOGE("Init device manager failed, ret:%{public}d", ret);
        return false;
    }
    stateCallback_ = std::make_shared<DmDeviceStateCallback>();
    ret = DIS_HARDWARE.RegisterDevStateCallback(FI_PKG_NAME, "", stateCallback_);
    if (ret != 0) {
        FI_HILOGE("Register devStateCallback failed, ret:%{public}d", ret);
        return false;
    }
    return true;
}

void CoordinationSM::OnDeviceOnline(const std::string &networkId)
{
    CALL_INFO_TRACE;
    std::lock_guard<std::mutex> guard(mutex_);
    onlineDevice_.push_back(networkId);
    DP_ADAPTER->RegisterCrossingStateListener(networkId,
        std::bind(&CoordinationSM::OnCoordinationChanged, COOR_SM, std::placeholders::_1, std::placeholders::_2));
    COOR_SOFTBUS_ADAPTER->Init();
}

void CoordinationSM::OnDeviceOffline(const std::string &networkId)
{
    CALL_INFO_TRACE;
    DP_ADAPTER->UnregisterCrossingStateListener(networkId);
    Reset(networkId);
    preparedNetworkId_ = std::make_pair("", "");
    std::lock_guard<std::mutex> guard(mutex_);
    if (!onlineDevice_.empty()) {
        auto it = std::find(onlineDevice_.begin(), onlineDevice_.end(), networkId);
        if (it != onlineDevice_.end()) {
            onlineDevice_.erase(it);
        }
    }
    if (currentState_ == CoordinationState::STATE_IN && sinkNetworkId_ == networkId) {
        COOR_EVENT_MGR->OnCoordinationMessage(CoordinationMessage::SESSION_CLOSED);
    }
    if (currentState_ == CoordinationState::STATE_OUT && remoteNetworkId_ == networkId) {
        COOR_EVENT_MGR->OnCoordinationMessage(CoordinationMessage::SESSION_CLOSED);
    }
}

std::string CoordinationSM::GetDeviceCoordinationState(CoordinationState value) const
{
    std::string state;
    switch (value) {
        case CoordinationState::STATE_FREE: {
            state = "free";
            break;
        }
        case CoordinationState::STATE_IN: {
            state = "in";
            break;
        }
        case CoordinationState::STATE_OUT: {
            state = "out";
            break;
        }
        default: {
            state = "unknown";
            FI_HILOGW("Coordination status unknown");
            break;
        }
    }
    return state;
}

void CoordinationSM::Dump(int32_t fd)
{
    CALL_DEBUG_ENTER;
    std::lock_guard<std::mutex> guard(mutex_);
    dprintf(fd, "Coordination information:\n");
    dprintf(fd,
        "coordinationState:%s | startDeviceDhid:%s | remoteNetworkId:%s | isStarting:%s | isStopping:%s\n"
        "physicalX:%d | physicalY:%d | displayX:%d | displayY:%d | interceptorId:%d | monitorId:%d | filterId:%d\n",
        GetDeviceCoordinationState(currentState_).c_str(), startDeviceDhid_.c_str(),
        remoteNetworkId_.substr(0, SUBSTR_NETWORKID_LEN).c_str(), isStarting_ ? "true" : "false", isStopping_ ? "true" : "false",
        mouseLocation_.first, mouseLocation_.second, displayX_, displayY_, interceptorId_, monitorId_, filterId_);
    if (onlineDevice_.empty()) {
        dprintf(fd, "onlineDevice:%s\n", "None");
        return;
    }
    for (const auto &item : onlineDevice_) {
        dprintf(fd, "onlineDevice:%s\n", item.c_str());
    }
}

void CoordinationSM::UpdateLastPointerEventCallback(std::shared_ptr<MMI::PointerEvent> pointerEvent)
{
    lastPointerEvent_ = pointerEvent;
}

std::shared_ptr<MMI::PointerEvent> CoordinationSM::GetLastPointerEvent() const
{
    return lastPointerEvent_;
}

void CoordinationSM::RemoveMonitor()
{
    if ((monitorId_ >= MIN_HANDLER_ID) && (monitorId_ < std::numeric_limits<int32_t>::max())) {
        MMI::InputManager::GetInstance()->RemoveMonitor(monitorId_);
        monitorId_ = -1;
    }
}

void CoordinationSM::RemoveInterceptor()
{
    if ((interceptorId_ >= MIN_HANDLER_ID) && (interceptorId_ < std::numeric_limits<int32_t>::max())) {
        MMI::InputManager::GetInstance()->RemoveInterceptor(interceptorId_);
        interceptorId_ = -1;
    }
}

bool CoordinationSM::IsNeedFilterOut(const std::string &deviceId, const std::shared_ptr<MMI::KeyEvent> keyEvent)
{
    CALL_DEBUG_ENTER;
    std::vector<OHOS::MMI::KeyEvent::KeyItem> KeyItems = keyEvent->GetKeyItems();
    std::vector<int32_t> KeyItemsForDInput;
    KeyItemsForDInput.reserve(KeyItems.size());
    for (const auto &item : KeyItems) {
        KeyItemsForDInput.push_back(item.GetKeyCode());
    }
    DistributedHardware::DistributedInput::BusinessEvent businessEvent;
    businessEvent.keyCode = keyEvent->GetKeyCode();
    businessEvent.keyAction = keyEvent->GetKeyAction();
    businessEvent.pressedKeys = KeyItemsForDInput;
    FI_HILOGI("businessEvent.keyCode:%{public}d, keyAction:%{public}d",
        businessEvent.keyCode, businessEvent.keyAction);
    for (const auto &item : businessEvent.pressedKeys) {
        FI_HILOGI("pressedKeys:%{public}d", item);
    }
    return D_INPUT_ADAPTER->IsNeedFilterOut(deviceId, businessEvent);
}

void CoordinationSM::DeviceInitCallBack::OnRemoteDied()
{
    CALL_INFO_TRACE;
}

void CoordinationSM::DmDeviceStateCallback::OnDeviceOnline(const DistributedHardware::DmDeviceInfo &deviceInfo)
{
    CALL_DEBUG_ENTER;
    COOR_SM->OnDeviceOnline(deviceInfo.networkId);
}

void CoordinationSM::DmDeviceStateCallback::OnDeviceOffline(const DistributedHardware::DmDeviceInfo &deviceInfo)
{
    CALL_INFO_TRACE;
    COOR_SM->OnDeviceOffline(deviceInfo.networkId);
}

void CoordinationSM::DmDeviceStateCallback::OnDeviceChanged(const DistributedHardware::DmDeviceInfo &deviceInfo)
{
    CALL_INFO_TRACE;
}

void CoordinationSM::DmDeviceStateCallback::OnDeviceReady(const DistributedHardware::DmDeviceInfo &deviceInfo)
{
    CALL_INFO_TRACE;
}

void CoordinationSM::SetAbsolutionLocation(double xPercent, double yPercent)
{
    CALL_INFO_TRACE;
    auto display = Rosen::DisplayManager::GetInstance().GetDefaultDisplay();
    CHKPV(display);
    int32_t width = display->GetWidth();
    int32_t height = display->GetHeight();
    int32_t physicalX = static_cast<int32_t>(width * xPercent / PERCENT_CONST);
    int32_t physicalY = static_cast<int32_t>(height * yPercent / PERCENT_CONST);
    FI_HILOGD("width:%{public}d, height:%{public}d, physicalX:%{public}d, physicalY:%{public}d", width, height,
        physicalX, physicalY);
    MMI::InputManager::GetInstance()->SetPointerLocation(physicalX, physicalY);
}


void CoordinationSM::OnInterceptorInputEvent(std::shared_ptr<MMI::KeyEvent> keyEvent)
{
    CHKPV(eventHandler_);
    CHKPV(keyEvent);
    std::string taskName = "process_interceptor_keyevent";
    std::function<void()> handleFunc =
        std::bind(&CoordinationSM::OnPostInterceptorKeyEvent, this, keyEvent);
    eventHandler_->ProxyPostTask(handleFunc, taskName, 0);
}

void CoordinationSM::OnInterceptorInputEvent(std::shared_ptr<MMI::PointerEvent> pointerEvent)
{
    CHKPV(eventHandler_);
    CHKPV(pointerEvent);
    std::string taskName = "process_interceptor_pointerevent";
    std::function<void()> handleFunc =
        std::bind(&CoordinationSM::OnPostInterceptorPointerEvent, this, pointerEvent);
    eventHandler_->ProxyPostTask(handleFunc, taskName, 0);
}

void CoordinationSM::OnMonitorInputEvent(std::shared_ptr<MMI::PointerEvent> pointerEvent)
{
    CHKPV(eventHandler_);
    CHKPV(pointerEvent);
    std::string taskName = "process_monitor_pointerevent";
    std::function<void()> handleFunc =
        std::bind(&CoordinationSM::OnPostMonitorInputEvent, this, pointerEvent);
    eventHandler_->ProxyPostTask(handleFunc, taskName, 0);
}

void CoordinationSM::OnPostInterceptorKeyEvent(std::shared_ptr<MMI::KeyEvent> keyEvent)
{
    CALL_DEBUG_ENTER;
    CHKPV(keyEvent);
    int32_t keyCode = keyEvent->GetKeyCode();
    CoordinationState state = GetCurrentCoordinationState();
    int32_t deviceId = keyEvent->GetDeviceId();
    if (keyCode == MMI::KeyEvent::KEYCODE_BACK || keyCode == MMI::KeyEvent::KEYCODE_VOLUME_UP ||
        keyCode == MMI::KeyEvent::KEYCODE_VOLUME_DOWN || keyCode == MMI::KeyEvent::KEYCODE_POWER) {
        if ((state == CoordinationState::STATE_OUT) || (!COOR_DEV_MGR->IsRemote(deviceId))) {
            keyEvent->AddFlag(MMI::AxisEvent::EVENT_FLAG_NO_INTERCEPT);
            MMI::InputManager::GetInstance()->SimulateInputEvent(keyEvent);
        }
        return;
    }
    if (state == CoordinationState::STATE_IN) {
        if (COOR_DEV_MGR->IsRemote(deviceId)) {
            auto networkId = COOR_DEV_MGR->GetOriginNetworkId(deviceId);
            if (!IsNeedFilterOut(networkId, keyEvent)) {
                keyEvent->AddFlag(MMI::AxisEvent::EVENT_FLAG_NO_INTERCEPT);
                MMI::InputManager::GetInstance()->SimulateInputEvent(keyEvent);
            }
        } else {
            keyEvent->AddFlag(MMI::AxisEvent::EVENT_FLAG_NO_INTERCEPT);
            MMI::InputManager::GetInstance()->SimulateInputEvent(keyEvent);
        }
    } else if (state == CoordinationState::STATE_OUT) {
        std::string networkId = COORDINATION::GetLocalNetworkId();
        if (IsNeedFilterOut(networkId, keyEvent)) {
            keyEvent->AddFlag(MMI::AxisEvent::EVENT_FLAG_NO_INTERCEPT);
            MMI::InputManager::GetInstance()->SimulateInputEvent(keyEvent);
        }
    }
}

void CoordinationSM::OnPostInterceptorPointerEvent(std::shared_ptr<MMI::PointerEvent> pointerEvent)
{
    CALL_DEBUG_ENTER;
    CHKPV(pointerEvent);
    if (pointerEvent->GetSourceType() != MMI::PointerEvent::SOURCE_TYPE_MOUSE) {
        FI_HILOGD("Not mouse event, skip");
        return;
    }
    CoordinationState state = GetCurrentCoordinationState();
    if (state == CoordinationState::STATE_OUT) {
        int32_t deviceId = pointerEvent->GetDeviceId();
        std::string dhid = COOR_DEV_MGR->GetDhid(deviceId);
        if (startDeviceDhid_ != dhid) {
            FI_HILOGI("Move other mouse, stop input device coordination");
            DeactivateCoordination(isUnchained_);
        }
    }
}

void CoordinationSM::OnPostMonitorInputEvent(std::shared_ptr<MMI::PointerEvent> pointerEvent)
{
    CALL_DEBUG_ENTER;
    CHKPV(pointerEvent);
    MMI::PointerEvent::PointerItem pointerItem;
    pointerEvent->GetPointerItem(pointerEvent->GetPointerId(), pointerItem);
    displayX_ = pointerItem.GetDisplayX();
    displayY_ = pointerItem.GetDisplayY();
    CoordinationState state = GetCurrentCoordinationState();
    if (state == CoordinationState::STATE_IN) {
        int32_t deviceId = pointerEvent->GetDeviceId();
        if (!COOR_DEV_MGR->IsRemote(deviceId)) {
            DeactivateCoordination(isUnchained_);
        }
    }
}

void CoordinationSM::InterceptorConsumer::OnInputEvent(std::shared_ptr<MMI::KeyEvent> keyEvent) const
{
    COOR_SM->OnInterceptorInputEvent(keyEvent);
}

void CoordinationSM::InterceptorConsumer::OnInputEvent(std::shared_ptr<MMI::PointerEvent> pointerEvent) const
{
    COOR_SM->OnInterceptorInputEvent(pointerEvent);
}

void CoordinationSM::InterceptorConsumer::OnInputEvent(std::shared_ptr<MMI::AxisEvent> axisEvent) const {}

void CoordinationSM::MonitorConsumer::OnInputEvent(std::shared_ptr<MMI::KeyEvent> keyEvent) const {}

void CoordinationSM::MonitorConsumer::OnInputEvent(std::shared_ptr<MMI::PointerEvent> pointerEvent) const
{
    CHKPV(pointerEvent);
    if (pointerEvent->GetSourceType() != MMI::PointerEvent::SOURCE_TYPE_MOUSE) {
        FI_HILOGD("Not mouse event, skip");
        return;
    }
    if (callback_) {
        callback_(pointerEvent);
    }
    COOR_SM->OnMonitorInputEvent(pointerEvent);
}

void CoordinationSM::MonitorConsumer::OnInputEvent(std::shared_ptr<MMI::AxisEvent> axisEvent) const {}

void CoordinationSM::RegisterStateChange(CooStateChangeType type,
    std::function<void(CoordinationState, CoordinationState)> callback)
{
    CALL_DEBUG_ENTER;
    CHKPV(callback);
    stateChangedCallbacks_[type] = callback;
}

void CoordinationSM::RegisterRemoteNetworkId(std::function<void(std::string)> callback)
{
    CALL_DEBUG_ENTER;
    CHKPV(callback);
    remoteNetworkIdCallback_ = callback;
}

void CoordinationSM::RegisterMouseLocation(std::function<void(int32_t, int32_t)> callback)
{
    CALL_DEBUG_ENTER;
    CHKPV(callback);
    mouseLocationCallback_ = callback;
}

void CoordinationSM::StateChangedNotify(CoordinationState oldState, CoordinationState newState)
{
    CALL_DEBUG_ENTER;
    if (oldState == CoordinationState::STATE_FREE && newState == CoordinationState::STATE_IN) {
        ChangeNotify(CooStateChangeType::STATE_FREE_TO_IN, oldState, newState);
        return;
    }
    if (oldState == CoordinationState::STATE_FREE && newState == CoordinationState::STATE_OUT) {
        ChangeNotify(CooStateChangeType::STATE_FREE_TO_OUT, oldState, newState);
        return;
    }
    if (oldState == CoordinationState::STATE_IN && newState == CoordinationState::STATE_FREE) {
        ChangeNotify(CooStateChangeType::STATE_IN_TO_FREE, oldState, newState);
        return;
    }
    if (oldState == CoordinationState::STATE_OUT && newState == CoordinationState::STATE_FREE) {
        ChangeNotify(CooStateChangeType::STATE_OUT_TO_FREE, oldState, newState);
    }
}

void CoordinationSM::ChangeNotify(CooStateChangeType type, CoordinationState oldState, CoordinationState newState)
{
    auto item = stateChangedCallbacks_[type];
    if (item != nullptr) {
        item(oldState, newState);
    }
}

void CoordinationSM::NotifyRemoteNetworkId(const std::string &remoteNetworkId)
{
    if (remoteNetworkIdCallback_ != nullptr) {
        remoteNetworkIdCallback_(remoteNetworkId);
    }
}

void CoordinationSM::NotifyMouseLocation(int32_t x, int32_t y)
{
    if (mouseLocationCallback_ != nullptr) {
        mouseLocationCallback_(x, y);
    }
}

void CoordinationSM::SetUnchainStatus(bool isUnchained)
{
    CALL_DEBUG_ENTER;
    isUnchained_ = isUnchained;
}

void CoordinationSM::NotifyChainRemoved()
{
    CALL_DEBUG_ENTER;
    CoordinationMessage msg = CoordinationMessage::SESSION_CLOSED;
    auto *context = COOR_EVENT_MGR->GetIContext();
    CHKPV(context);
    int32_t ret = context->GetDelegateTasks().PostAsyncTask(
        std::bind(&CoordinationEventManager::OnCoordinationMessage, COOR_EVENT_MGR, msg, ""));
    if (ret != RET_OK) {
        FI_HILOGE("Posting async task failed");
    }
}

void CoordinationSM::NotifyUnchainedResult(const std::string &remoteNetworkId, bool isSuccess)
{
    CALL_DEBUG_ENTER;
    FI_HILOGD("Notify unchained result, isSuccess:%{public}d", isSuccess);
    if (isSuccess) {
        COOR_SM->NotifyChainRemoved();
    }
    isUnchained_ = false;
    isStopping_ = false;
    preparedNetworkId_ = std::make_pair("", "");
    COOR_SOFTBUS_ADAPTER->CloseInputSoftbus(remoteNetworkId);
}

void CoordinationSM::SetSinkNetworkId(const std::string &sinkNetworkId)
{
    CALL_DEBUG_ENTER;
    sinkNetworkId_ = sinkNetworkId;
}

void CoordinationSM::SetPointerVisible()
{
    bool hasPointer = COOR_DEV_MGR->HasLocalPointerDevice();
    FI_HILOGD("hasPointer:%{public}s", hasPointer ? "true" : "false");
    MMI::InputManager::GetInstance()->SetPointerVisible(hasPointer);
}

std::shared_ptr<ICoordinationState> CoordinationSM::GetCurrentState()
{
    auto it = coordinationStates_.find(currentState_);
    if (it == coordinationStates_.end()) {
        FI_HILOGE("currentState_ not found");
        return nullptr;
    }
    return it->second;
}

bool PointerFilter::OnInputEvent(std::shared_ptr<MMI::PointerEvent> pointerEvent) const
{
    FI_HILOGD("PointerFilter OnInputEvent enter");
    CHKPF(pointerEvent);
    if (pointerEvent->GetPointerAction() == MMI::PointerEvent::POINTER_ACTION_BUTTON_DOWN) {
        FI_HILOGI("Current event is down");
        auto *context = COOR_EVENT_MGR->GetIContext();
        CHKPF(context);
        int32_t ret = context->GetDelegateTasks().PostAsyncTask(
            std::bind(&MMI::InputManager::RemoveInputEventFilter, MMI::InputManager::GetInstance(), filterId_));
        if (ret != RET_OK) {
            FI_HILOGE("Posting async task failed");
        }
        filterId_ = -1;
        return true;
    }
    return false;
}
} // namespace DeviceStatus
} // namespace Msdp
} // namespace OHOS
