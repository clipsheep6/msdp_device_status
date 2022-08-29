/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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
#include "input_device_cooperate_sm.h"

#include <cstdio>

#include "hitrace_meter.h"

#include "bytrace_adapter.h"
#include "cooperate_messages.h"
#include "cooperate_event_manager.h"
#include "define_multimodal.h"
#include "device_manager.h"
#include "device_profile_adapter.h"
#include "input_device_cooperate_state_free.h"
#include "input_device_cooperate_state_in.h"
#include "input_device_cooperate_state_out.h"
#include "input_device_manager.h"
#include "i_pointer_drawing_manager.h"
#include "mouse_event_handler.h"
#include "multimodal_input_connect_remoter.h"
#include "timer_manager.h"
#include "util_ex.h"

namespace OHOS {
namespace MMI {
namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, MMI_LOG_DOMAIN, "InputDeviceCooperateSM"};
const std::string BUNDLE_NAME = "ohos.multimodalinput.input";
constexpr int32_t INTERVAL_MS = 2000;
constexpr int32_t MOUSE_ABS_LOCATION = 100;
constexpr int32_t MOUSE_ABS_LOCATION_X = 50;
constexpr int32_t MOUSE_ABS_LOCATION_Y = 50;
} // namespace

void InputDeviceCooperateSM::Init()
{
    preparedNetworkId_ = std::make_pair("", "");
    Reset();
    TimerMgr->AddTimer(INTERVAL_MS, 1, []() {
        InitDeviceManager();
    });
}

void InputDeviceCooperateSM::Reset()
{
    CALL_INFO_TRACE;
    startDhid_ = "";
    srcNetworkId_ = "";
    isStarting_ = false;
    isStopping_ = false;
    bool hasPointer = InputDevMgr->HasLocalPointerDevice();
    IPointerDrawingManager::GetInstance()->SetPointerVisible(getpid(), hasPointer);
    currentStateSM_ = std::make_shared<InputDeviceCooperateStateFree>();
    cooperateState_ = CooperateState::STATE_FREE;
}

void InputDeviceCooperateSM::OnCooperateChanged(const std::string &networkId, bool isOpen)
{
    CALL_DEBUG_ENTER;
    if (!isOpen) {
        CooperateEventMgr->OnCooperateMessage(CooperateMessages::MSG_COOPERATE_STATE_OFF, networkId);
        OnCloseCooperation(networkId, false);
    }
}

void InputDeviceCooperateSM::OnCloseCooperation(const std::string &networkId, bool isLocal)
{
    CALL_INFO_TRACE;
    std::lock_guard<std::mutex> guard(mutex_);
    if (!preparedNetworkId_.first.empty() && !preparedNetworkId_.second.empty()) {
        if (networkId == preparedNetworkId_.first || networkId == preparedNetworkId_.second) {
            DistributedAdapter->UnPrepareRemoteInput(preparedNetworkId_.first, preparedNetworkId_.second,
                [](bool isSuccess) {});
        }
    }
    UpdatePreparedDevices("", "");
    if (isLocal || networkId == srcNetworkId_) {
        if (InputDevMgr->HasLocalPointerDevice()) {
            MouseEventHdr->SetAbsolutionLocation(MOUSE_ABS_LOCATION_X, MOUSE_ABS_LOCATION_Y);
        }
        Reset();
        return;
    }
    std::string originNetworkId = InputDevMgr->GetOriginNetworkId(startDhid_);
    if (originNetworkId == networkId) {
        Reset();
    }
}

void InputDeviceCooperateSM::GetCooperateState(const std::string &deviceId)
{
    CALL_INFO_TRACE;
    bool state = DProfileAdapter->GetCrossingSwitchState(deviceId);
    CooperateEventMgr->OnGetState(state);
}

void InputDeviceCooperateSM::EnableInputDeviceCooperate(bool enabled)
{
    CALL_INFO_TRACE;
    if (enabled) {
        BytraceAdapter::StartBytrace(BytraceAdapter::TRACE_START, BytraceAdapter::START_EVENT);
        DProfileAdapter->UpdateCrossingSwitchState(enabled, onlineDevice_);
        BytraceAdapter::StartBytrace(BytraceAdapter::TRACE_STOP, BytraceAdapter::START_EVENT);
    } else {
        DProfileAdapter->UpdateCrossingSwitchState(enabled, onlineDevice_);
        std::string localNetworkId;
        InputDevMgr->GetLocalDeviceId(localNetworkId);
        OnCloseCooperation(localNetworkId, true);
    }
}

int32_t InputDeviceCooperateSM::StartInputDeviceCooperate(
    const std::string &remoteNetworkId, int32_t startInputDeviceId)
{
    CALL_INFO_TRACE;
    std::lock_guard<std::mutex> guard(mutex_);
    if (isStarting_) {
        MMI_HILOGE("In transition state, not process");
        return RET_ERR;
    }
    CHKPR(currentStateSM_, ERROR_NULL_POINTER);
    BytraceAdapter::StartBytrace(BytraceAdapter::TRACE_START, BytraceAdapter::LAUNCH_EVENT);
    isStarting_ = true;
    int32_t ret = currentStateSM_->StartInputDeviceCooperate(remoteNetworkId, startInputDeviceId);
    if (ret != RET_OK) {
        MMI_HILOGE("Start remote input fail");
        BytraceAdapter::StartBytrace(BytraceAdapter::TRACE_STOP, BytraceAdapter::LAUNCH_EVENT);
        isStarting_ = false;
        return RET_ERR;
    }
    UpdateMouseLocation();
    if (cooperateState_ == CooperateState::STATE_FREE) {
        srcNetworkId_ = remoteNetworkId;
    }
    return ret;
}

int32_t InputDeviceCooperateSM::StopInputDeviceCooperate()
{
    CALL_INFO_TRACE;
    std::lock_guard<std::mutex> guard(mutex_);
    if (isStopping_) {
        MMI_HILOGE("In transition state, not process");
        return RET_ERR;
    }
    CHKPR(currentStateSM_, ERROR_NULL_POINTER);
    BytraceAdapter::StartBytrace(BytraceAdapter::TRACE_START, BytraceAdapter::STOP_EVENT);
    isStopping_ = true;
    int32_t ret = currentStateSM_->StopInputDeviceCooperate();
    if (ret != RET_OK) {
        MMI_HILOGE("Stop input device cooperate fail");
        BytraceAdapter::StartBytrace(BytraceAdapter::TRACE_STOP, BytraceAdapter::STOP_EVENT);
        isStopping_ = false;
    }
    return ret;
}

void InputDeviceCooperateSM::StartRemoteCooperate(const std::string &remoteNetworkId)
{
    CALL_INFO_TRACE;
    CooperateEventMgr->OnCooperateMessage(CooperateMessages::MSG_COOPERATE_INFO_START, remoteNetworkId);
    std::lock_guard<std::mutex> guard(mutex_);
    isStarting_ = true;
}

void InputDeviceCooperateSM::StartRemoteCooperateResult(bool isSuccess,
    const std::string& startDhid, int32_t xPercent, int32_t yPercent)
{
    CALL_INFO_TRACE;
    std::lock_guard<std::mutex> guard(mutex_);
    if (!isStarting_) {
        MMI_HILOGI("Not in starting");
        return;
    }
    startDhid_ = startDhid;
    CooperateMessages msg =
            isSuccess ? CooperateMessages::MSG_COOPERATE_INFO_SUCCESS : CooperateMessages::MSG_COOPERATE_INFO_FAIL;
        CooperateEventMgr->OnCooperateMessage(msg);

    if (cooperateState_ == CooperateState::STATE_FREE && isSuccess) {
        MouseEventHdr->SetAbsolutionLocation(MOUSE_ABS_LOCATION - xPercent, yPercent);
        UpdateState(CooperateState::STATE_IN);
    } else if (cooperateState_ == CooperateState::STATE_OUT && isSuccess) {
        MouseEventHdr->SetAbsolutionLocation(MOUSE_ABS_LOCATION - xPercent, yPercent);
        UpdateState(CooperateState::STATE_FREE);
    } else {
        MMI_HILOGI("Current state is in");
    }
    isStarting_ = false;
}

void InputDeviceCooperateSM::StopRemoteCooperate()
{
    CALL_INFO_TRACE;
    CooperateEventMgr->OnCooperateMessage(CooperateMessages::MSG_COOPERATE_STOP);
    std::lock_guard<std::mutex> guard(mutex_);
    isStopping_ = true;
}

void InputDeviceCooperateSM::StopRemoteCooperateResult(bool isSuccess)
{
    CALL_INFO_TRACE;
    std::lock_guard<std::mutex> guard(mutex_);
    if (!isStopping_) {
        MMI_HILOGI("Not in stopping");
        return;
    }
    CooperateMessages msg =
        isSuccess ? CooperateMessages::MSG_COOPERATE_STOP_SUCCESS : CooperateMessages::MSG_COOPERATE_STOP_FAIL;
    CooperateEventMgr->OnCooperateMessage(msg);
    if (isSuccess) {
        if (InputDevMgr->HasLocalPointerDevice()) {
            MouseEventHdr->SetAbsolutionLocation(MOUSE_ABS_LOCATION_X, MOUSE_ABS_LOCATION_Y);
        }
        UpdateState(CooperateState::STATE_FREE);
    }
    isStopping_ = false;
}

void InputDeviceCooperateSM::StartCooperateOtherResult(const std::string& srcNetworkId)
{
    CALL_INFO_TRACE;
    std::lock_guard<std::mutex> guard(mutex_);
    srcNetworkId_ = srcNetworkId;
}

void InputDeviceCooperateSM::OnStartFinish(bool isSuccess,
    const std::string &remoteNetworkId, int32_t startInputDeviceId)
{
    CALL_INFO_TRACE;
    std::lock_guard<std::mutex> guard(mutex_);
    if (!isStarting_) {
        MMI_HILOGE("Not in starting");
        return;
    }
    
    BytraceAdapter::StartBytrace(BytraceAdapter::TRACE_STOP, BytraceAdapter::LAUNCH_EVENT);
    if (!isSuccess) {
        MMI_HILOGE("Start distributed fail, startInputDevice: %{public}d", startInputDeviceId);
        NotifyRemoteStartFail(remoteNetworkId);
    } else {
        startDhid_ = InputDevMgr->GetDhid(startInputDeviceId);
        NotifyRemoteStartSucess(remoteNetworkId, startDhid_);
        if (cooperateState_ == CooperateState::STATE_FREE) {
            UpdateState(CooperateState::STATE_OUT);
        } else if (cooperateState_ == CooperateState::STATE_IN) {
            std::string sink = InputDevMgr->GetOriginNetworkId(startInputDeviceId);
            if (!sink.empty() && remoteNetworkId != sink) {
                RemoteMgr->StartCooperateOtherResult(sink, remoteNetworkId);
            }
            UpdateState(CooperateState::STATE_FREE);
        } else {
            MMI_HILOGI("Current state is out");
        }
    }
    isStarting_ = false;
}

void InputDeviceCooperateSM::OnStopFinish(bool isSuccess, const std::string &remoteNetworkId)
{
    CALL_INFO_TRACE;
    std::lock_guard<std::mutex> guard(mutex_);
    if (!isStopping_) {
        MMI_HILOGE("Not in stopping");
        return;
    }
    BytraceAdapter::StartBytrace(BytraceAdapter::TRACE_STOP, BytraceAdapter::STOP_EVENT);
    NotifyRemoteStopFinish(isSuccess, remoteNetworkId);
    if (isSuccess) {
        if (InputDevMgr->HasLocalPointerDevice()) {
            MouseEventHdr->SetAbsolutionLocation(MOUSE_ABS_LOCATION_X, MOUSE_ABS_LOCATION_Y);
        }
        if (cooperateState_ == CooperateState::STATE_IN || cooperateState_ == CooperateState::STATE_OUT) {
            UpdateState(CooperateState::STATE_FREE);
        } else {
            MMI_HILOGI("Current state is free");
        }
    }
    isStopping_ = false;
}

void InputDeviceCooperateSM::NotifyRemoteStartFail(const std::string &remoteNetworkId)
{
    CALL_DEBUG_ENTER;
    RemoteMgr->StartRemoteCooperateResult(remoteNetworkId, false, "",  0, 0);
    CooperateEventMgr->OnStart(CooperateMessages::MSG_COOPERATE_INFO_FAIL);
}

void InputDeviceCooperateSM::NotifyRemoteStartSucess(const std::string &remoteNetworkId, const std::string& startDhid)
{
    CALL_DEBUG_ENTER;
    RemoteMgr->StartRemoteCooperateResult(remoteNetworkId,
        true, startDhid, mouseLocation_.first, mouseLocation_.second);
    CooperateEventMgr->OnStart(CooperateMessages::MSG_COOPERATE_INFO_SUCCESS);
}

void InputDeviceCooperateSM::NotifyRemoteStopFinish(bool isSuccess, const std::string &remoteNetworkId)
{
    CALL_DEBUG_ENTER;
    RemoteMgr->StopRemoteCooperateResult(remoteNetworkId, isSuccess);
    if (!isSuccess) {
        CooperateEventMgr->OnStop(CooperateMessages::MSG_COOPERATE_STOP_FAIL);
    } else {
        CooperateEventMgr->OnStop(CooperateMessages::MSG_COOPERATE_STOP_SUCCESS);
    }
}

bool InputDeviceCooperateSM::UpdateMouseLocation()
{
    CALL_DEBUG_ENTER;
    auto pointerEvent = MouseEventHdr->GetPointerEvent();
    CHKPR(pointerEvent, false);
    int32_t displayId = pointerEvent->GetTargetDisplayId();
    auto physicalDisplayInfo = WinMgr->GetPhysicalDisplay(displayId);
    CHKPR(physicalDisplayInfo, false);
    int32_t displayWidth = physicalDisplayInfo->width;
    int32_t displayHeight = physicalDisplayInfo->height;
    if (displayWidth == 0 || displayHeight == 0) {
        MMI_HILOGE("diaplay width or height is 0");
        return false;
    }
    auto mouseInfo = WinMgr->GetMouseInfo();
    int32_t xPercent = mouseInfo.physicalX * MOUSE_ABS_LOCATION / displayWidth;
    int32_t yPercent = mouseInfo.physicalY * MOUSE_ABS_LOCATION / displayHeight;
    MMI_HILOGI("displayWidth: %{public}d, displayHeight: %{public}d, physicalX: %{public}d, physicalY: %{public}d,",
        displayWidth, displayHeight, mouseInfo.physicalX, mouseInfo.physicalY);
    mouseLocation_ = std::make_pair(xPercent, yPercent);
    return true;
}

void InputDeviceCooperateSM::UpdateState(CooperateState state)
{
    MMI_HILOGI("state: %{public}d", state);
    switch (state) {
        case CooperateState::STATE_FREE: {
            Reset();
            break;
        }
        case CooperateState::STATE_IN: {
            currentStateSM_ = std::make_shared<InputDeviceCooperateStateIn>(startDhid_);
            break;
        }
        case CooperateState::STATE_OUT: {
            IPointerDrawingManager::GetInstance()->SetPointerVisible(getpid(), false);
            currentStateSM_ = std::make_shared<InputDeviceCooperateStateOut>(startDhid_);
            break;
        }
        default:
            break;
    }
    cooperateState_ = state;
}

std::shared_ptr<IInputDeviceCooperateState> InputDeviceCooperateSM::GetCurrentState() const
{
    CALL_DEBUG_ENTER;
    std::lock_guard<std::mutex> guard(mutex_);
    return currentStateSM_;
}

CooperateState InputDeviceCooperateSM::GetCurrentCooperateState() const
{
    CALL_DEBUG_ENTER;
    std::lock_guard<std::mutex> guard(mutex_);
    return cooperateState_;
}

void InputDeviceCooperateSM::UpdatePreparedDevices(const std::string &srcNetworkId, const std::string &sinkNetworkId)
{
    CALL_DEBUG_ENTER;
    std::lock_guard<std::mutex> guard(mutex_);
    preparedNetworkId_ = std::make_pair(srcNetworkId, sinkNetworkId);
}

std::pair<std::string, std::string> InputDeviceCooperateSM::GetPreparedDevices() const
{
    CALL_DEBUG_ENTER;
    std::lock_guard<std::mutex> guard(mutex_);
    return preparedNetworkId_;
}

const std::string& InputDeviceCooperateSM::GetSrcNetworkId() const
{
    CALL_DEBUG_ENTER;
    std::lock_guard<std::mutex> guard(mutex_);
    return srcNetworkId_;
}

void InputDeviceCooperateSM::OnKeyboardOnline(const std::string &dhid)
{
    CALL_INFO_TRACE;
    std::lock_guard<std::mutex> guard(mutex_);
    CHKPV(currentStateSM_);
    currentStateSM_->OnKeyboardOnline(dhid);
}

void InputDeviceCooperateSM::OnPointerOffline(const std::string &dhid, const std::string &sinkNetworkId,
    const std::vector<std::string> &keyboards)
{
    CALL_INFO_TRACE;
    std::lock_guard<std::mutex> guard(mutex_);
    if (cooperateState_ == CooperateState::STATE_FREE || startDhid_ != dhid) {
        return;
    }
    if (cooperateState_ == CooperateState::STATE_OUT) {
        std::string src = srcNetworkId_;
        if (src.empty()) {
            src = preparedNetworkId_.first;
        }
        DistributedAdapter->StopRemoteInput(src, sinkNetworkId, keyboards, [this, src](bool isSuccess) {});
    }
    Reset();
}

void InputDeviceCooperateSM::HandleLibinputEvent(libinput_event *event)
{
    MMI_HILOGI("current state :%{public}d", cooperateState_);
    CHKPV(event);
    auto type = libinput_event_get_type(event);
    switch (type) {
        case LIBINPUT_EVENT_POINTER_MOTION:
        case LIBINPUT_EVENT_POINTER_MOTION_ABSOLUTE:
        case LIBINPUT_EVENT_POINTER_BUTTON:
        case LIBINPUT_EVENT_POINTER_AXIS: {
            CheckPointerEvent(event);
            break;
        }
        default: {
            MMI_HILOGI("This device does not support");
            CHKPV(nextHandler_);
            nextHandler_->HandleLibinputEvent(event);
            break;
        }
    }
}

void InputDeviceCooperateSM::CheckPointerEvent(struct libinput_event *event)
{
    std::lock_guard<std::mutex> guard(mutex_);
    if (isStopping_ || isStarting_) {
        MMI_HILOGE("In transition state, not process");
        return;
    }
    auto inputDevice = libinput_event_get_device(event);
    if (cooperateState_ == CooperateState::STATE_IN) {
        if (!InputDevMgr->IsRemote(inputDevice)) {
            StopInputDeviceCooperate();
            return;
        }
    } else if (cooperateState_ == CooperateState::STATE_OUT) {
        int32_t deviceId = InputDevMgr->FindInputDeviceId(inputDevice);
        std::string dhid = InputDevMgr->GetDhid(deviceId);
        if (startDhid_ != dhid) {
            MMI_HILOGI("Move other mouse, stop input device cooperate");
            StopInputDeviceCooperate();
        }
        return;
    } else {
        if (InputDevMgr->IsRemote(inputDevice)) {
            return;
        }
    }
    CHKPV(nextHandler_);
    nextHandler_->HandleLibinputEvent(event);
}

bool InputDeviceCooperateSM::InitDeviceManager()
{
    CALL_DEBUG_ENTER;
    initCallback_ = std::make_shared<DeviceInitCallBack>();
    CHKPR(initCallback_, false);
    int32_t ret =
        DistributedHardware::DeviceManager::GetInstance().InitDeviceManager(BUNDLE_NAME, initCallback_);
    if (ret != 0) {
        MMI_HILOGE("Init device manager failed, ret:%{public}d", ret);
        return false;
    }
    stateCallback_ = std::make_shared<MmiDeviceStateCallback>();
    ret =
        DistributedHardware::DeviceManager::GetInstance().RegisterDevStateCallback(BUNDLE_NAME, "", stateCallback_);
    if (ret != 0) {
        MMI_HILOGE("Register devStateCallback failed, ret:%{public}d", ret);
        return false;
    }
    return true;
}

void InputDeviceCooperateSM::OnDeviceOnline(const std::string& networkId)
{
    CALL_INFO_TRACE;
    std::lock_guard<std::mutex> guard(mutex_);
    onlineDevice_.push_back(networkId);
    DProfileAdapter->RegisterCrossingStateListener(networkId,
        std::bind(&InputDeviceCooperateSM::OnCooperateChanged,
        InputDevCooSM, std::placeholders::_1, std::placeholders::_2));
}

void InputDeviceCooperateSM::OnDeviceOffline(const std::string& networkId)
{
    CALL_INFO_TRACE;
    std::lock_guard<std::mutex> guard(mutex_);
    DProfileAdapter->UnregisterCrossingStateListener(networkId);
    bool needReset = true;
    if (cooperateState_ == CooperateState::STATE_OUT) {
        if (networkId != srcNetworkId_) {
            needReset = false;
            MMI_HILOGE("OnDeviceOffline: needReset false");
        }
    }
    if (cooperateState_ == CooperateState::STATE_IN) {
        std::string sinkNetwoekId = InputDevMgr->GetOriginNetworkId(startDhid_);
        if (networkId != sinkNetwoekId) {
            needReset = false;
            MMI_HILOGE("OnDeviceOffline: needReset false");
        }
    }
    if (needReset) {
        MMI_HILOGE("OnDeviceOffline: needReset true");
        UpdatePreparedDevices("", "");
        if (InputDevMgr->HasLocalPointerDevice()) {
            MouseEventHdr->SetAbsolutionLocation(MOUSE_ABS_LOCATION_X, MOUSE_ABS_LOCATION_Y);
        }
        Reset();
    }
    if (!onlineDevice_.empty()) {
        auto it = std::find(onlineDevice_.begin(), onlineDevice_.end(), networkId);
        if (it != onlineDevice_.end()) {
            onlineDevice_.erase(it);
        }
    }
}

void InputDeviceCooperateSM::Dump(int32_t fd, const std::vector<std::string> &args)
{
    CALL_DEBUG_ENTER;
    std::lock_guard<std::mutex> guard(mutex_);
    mprintf(fd, "Keyboard and mouse crossing information:");
    mprintf(fd, "State machine status: %d\t", cooperateState_);
    mprintf(fd, "Peripheral keyboard and mouse information: startDhid_  srcNetworkId_:\t");
    mprintf(fd, "%s", startDhid_.c_str());
    mprintf(fd, "%s", srcNetworkId_.c_str());
    mprintf(fd, "Run successfully");
}

void InputDeviceCooperateSM::DeviceInitCallBack::OnRemoteDied()
{
    CALL_INFO_TRACE;
}

void InputDeviceCooperateSM::MmiDeviceStateCallback::OnDeviceOnline(
    const DistributedHardware::DmDeviceInfo &deviceInfo)
{
    CALL_DEBUG_ENTER;
    InputDevCooSM->OnDeviceOnline(deviceInfo.deviceId);
}

void InputDeviceCooperateSM::MmiDeviceStateCallback::OnDeviceOffline(
    const DistributedHardware::DmDeviceInfo &deviceInfo)
{
    CALL_DEBUG_ENTER;
    InputDevCooSM->OnDeviceOffline(deviceInfo.deviceId);
}

void InputDeviceCooperateSM::MmiDeviceStateCallback::OnDeviceChanged(
    const DistributedHardware::DmDeviceInfo &deviceInfo)
{
    CALL_INFO_TRACE;
}

void InputDeviceCooperateSM::MmiDeviceStateCallback::OnDeviceReady(
    const DistributedHardware::DmDeviceInfo &deviceInfo)
{
    CALL_INFO_TRACE;
}
} // namespace MMI
} // namespace OHOS