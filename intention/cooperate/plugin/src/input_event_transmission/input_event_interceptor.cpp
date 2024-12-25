/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "input_event_transmission/input_event_interceptor.h"

#include "cooperate_context.h"
#include "devicestatus_define.h"
#include "display_manager.h"
#include "power_mgr_client.h"
#include "input_event_transmission/input_event_serialization.h"
#include "utility.h"
#include "kits/c/wifi_hid2d.h"

#include "res_sched_client.h"
#include "res_type.h"

#undef LOG_TAG
#define LOG_TAG "InputEventInterceptor"

namespace OHOS {
namespace Msdp {
namespace DeviceStatus {
namespace Cooperate {

namespace {
const std::string WIFI_INTERFACE_NAME { "chba0" };
const int32_t RESTORE_SCENE { 0 };
const int32_t FORBIDDEN_SCENE { 1 };
const int32_t UPPER_SCENE_FPS { 0 };
const int32_t UPPER_SCENE_BW { 0 };
const int32_t INTERVAL_MS { 2000 };
const int32_t REPEAT_MAX { 10000 };
const int32_t MODE_ENABLE { 0 };
const int32_t MODE_DISABLE { 1 };
const std::string LOW_LATENCY_KEY = "identity";
}

std::set<int32_t> InputEventInterceptor::filterKeys_ {
    MMI::KeyEvent::KEYCODE_BACK,
    MMI::KeyEvent::KEYCODE_POWER,
};

std::set<int32_t> InputEventInterceptor::filterPointers_ {
    MMI::PointerEvent::POINTER_ACTION_ENTER_WINDOW,
    MMI::PointerEvent::POINTER_ACTION_LEAVE_WINDOW,
    MMI::PointerEvent::POINTER_ACTION_PULL_IN_WINDOW,
    MMI::PointerEvent::POINTER_ACTION_PULL_OUT_WINDOW,
};

InputEventInterceptor::~InputEventInterceptor()
{
    Disable();
}

void InputEventInterceptor::Enable(Context &context)
{
    CALL_INFO_TRACE;
    if (interceptorId_ > 0) {
        return;
    }
    auto cursorPos = context.CursorPosition();
    FI_HILOGI("Cursor transite out at (%{private}d, %{private}d)", cursorPos.x, cursorPos.y);
    remoteNetworkId_ = context.Peer();
    sender_ = context.Sender();
    inputEventSampler_.SetPointerEventHandler(
        [this](std::shared_ptr<MMI::PointerEvent> pointerEvent) {
            this->OnPointerEvent(pointerEvent);
        }
    );
    interceptorId_ = env_->GetInput().AddInterceptor(
        [this](std::shared_ptr<MMI::PointerEvent> pointerEvent) { inputEventSampler_.OnPointerEvent(pointerEvent); },
        [this](std::shared_ptr<MMI::KeyEvent> keyEvent) { this->OnKeyEvent(keyEvent); });
    if (interceptorId_ < 0) {
        FI_HILOGE("Input::AddInterceptor fail");
    }
    TurnOffChannelScan();
    HeartBeatSend();
    ExecuteInner();
}

void InputEventInterceptor::HeartBeatSend()
{
    CALL_DEBUG_ENTER;
    CHKPV(env_);
    heartTimer_ = env_->GetTimerManager().AddTimer(INTERVAL_MS, REPEAT_MAX, [this]() {
        NetPacket packet(MessageId::DSOFTBUS_HEART_BEAT_PACKET);
        if (InputEventSerialization::HeartBeatMarshalling(packet) != RET_OK) {
            FI_HILOGE("Failed to serialize packet");
            return;
        }
        env_->GetDSoftbus().SendPacket(remoteNetworkId_, packet);
    });
}

void InputEventInterceptor::Disable()
{
    CALL_INFO_TRACE;
    TurnOnChannelScan();
    if (interceptorId_ > 0) {
        env_->GetInput().RemoveInterceptor(interceptorId_);
        interceptorId_ = -1;
    }
    if ((pointerEventTimer_ >= 0) && (env_->GetTimerManager().IsExist(pointerEventTimer_))) {
        env_->GetTimerManager().RemoveTimer(pointerEventTimer_);
        pointerEventTimer_ = -1;
    }
    if (heartTimer_ < 0) {
        FI_HILOGE("Invalid heartTimer_");
        return;
    }
    if (env_->GetTimerManager().RemoveTimer(heartTimer_) != RET_OK) {
        FI_HILOGE("Failed to RemoveTimer");
    }
    heartTimer_ = -1;
}

void InputEventInterceptor::Update(Context &context)
{
    remoteNetworkId_ = context.Peer();
    FI_HILOGI("Update peer to \'%{public}s\'", Utility::Anonymize(remoteNetworkId_).c_str());
}

void InputEventInterceptor::OnPointerEvent(std::shared_ptr<MMI::PointerEvent> pointerEvent)
{
    CHKPV(pointerEvent);
    if (scanState_) {
        TurnOffChannelScan();
    }
    RefreshActivity();
    if ((pointerEventTimer_ >= 0) && (env_->GetTimerManager().IsExist(pointerEventTimer_))) {
        env_->GetTimerManager().RemoveTimer(pointerEventTimer_);
        pointerEventTimer_ = -1;
    }
    if (auto pointerAction = pointerEvent->GetPointerAction();
        filterPointers_.find(pointerAction) != filterPointers_.end()) {
        FI_HILOGI("Current pointerAction:%{public}d, skip", static_cast<int32_t>(pointerAction));
        return;
    }
    if (auto pointerAction = pointerEvent->GetPointerAction();
        pointerAction == MMI::PointerEvent::POINTER_ACTION_CANCEL) {
        auto originAction = pointerEvent->GetOriginPointerAction();
        FI_HILOGI("Reset to origin action:%{public}d", static_cast<int32_t>(originAction));
        pointerEvent->SetPointerAction(originAction);
    }
    OnNotifyCrossDrag(pointerEvent);
    NetPacket packet(MessageId::DSOFTBUS_INPUT_POINTER_EVENT);

    int32_t ret = InputEventSerialization::Marshalling(pointerEvent, packet);
    if (ret != RET_OK) {
        FI_HILOGE("Failed to serialize pointer event");
        return;
    }
    FI_HILOGD("PointerEvent(No:%{public}d,Source:%{public}s,Action:%{public}s)",
        pointerEvent->GetId(), pointerEvent->DumpSourceType(), pointerEvent->DumpPointerAction());
    env_->GetDSoftbus().SendPacket(remoteNetworkId_, packet);
    pointerEventTimer_ = env_->GetTimerManager().AddTimer(POINTER_EVENT_TIMEOUT, REPEAT_ONCE, [this]() {
        TurnOnChannelScan();
        pointerEventTimer_ = -1;
    });
}

void InputEventInterceptor::OnNotifyCrossDrag(std::shared_ptr<MMI::PointerEvent> pointerEvent)
{
    CHKPV(pointerEvent);
    auto pointerAction = pointerEvent->GetPointerAction();
    if (pointerAction == MMI::PointerEvent::POINTER_ACTION_PULL_IN_WINDOW ||
        pointerAction == MMI::PointerEvent::POINTER_ACTION_PULL_OUT_WINDOW) {
        FI_HILOGD("PointerAction:%{public}d, it's pressedButtons is empty, skip", pointerAction);
        return;
    }
    auto pressedButtons = pointerEvent->GetPressedButtons();
    bool isButtonDown = (pressedButtons.find(MMI::PointerEvent::MOUSE_BUTTON_LEFT) != pressedButtons.end());
    FI_HILOGD("PointerAction:%{public}d, isPressed:%{public}s", pointerAction, isButtonDown ? "true" : "false");
    CHKPV(env_);
    env_->GetDragManager().NotifyCrossDrag(isButtonDown);
}

void InputEventInterceptor::OnKeyEvent(std::shared_ptr<MMI::KeyEvent> keyEvent)
{
    CHKPV(keyEvent);
    RefreshActivity();
    if (filterKeys_.find(keyEvent->GetKeyCode()) != filterKeys_.end()) {
        keyEvent->AddFlag(MMI::AxisEvent::EVENT_FLAG_NO_INTERCEPT);
        env_->GetInput().SimulateInputEvent(keyEvent);
        return;
    }
    NetPacket packet(MessageId::DSOFTBUS_INPUT_KEY_EVENT);

    int32_t ret = InputEventSerialization::KeyEventToNetPacket(keyEvent, packet);
    if (ret != RET_OK) {
        FI_HILOGE("Failed to serialize key event");
        return;
    }
    FI_HILOGD("KeyEvent(No:%{public}d,Key:%{private}d,Action:%{public}d)",
        keyEvent->GetId(), keyEvent->GetKeyCode(), keyEvent->GetKeyAction());
    env_->GetDSoftbus().SendPacket(remoteNetworkId_, packet);
}

void InputEventInterceptor::TurnOffChannelScan()
{
    scanState_ = false;
    if (SetWifiScene(FORBIDDEN_SCENE) != RET_OK) {
        scanState_ = true;
        FI_HILOGE("Forbidden scene failed");
    }
}

void InputEventInterceptor::ExecuteInner()
{
    CALL_DEBUG_ENTER;
    // to enable low latency mode: value = 0
    OHOS::ResourceSchedule::ResSchedClient::GetInstance().ReportData(
        OHOS::ResourceSchedule::ResType::RES_TYPE_NETWORK_LATENCY_REQUEST, MODE_ENABLE,
        {{LOW_LATENCY_KEY, FI_PKG_NAME}});
}

void InputEventInterceptor::HandleStopTimer()
{
    CALL_DEBUG_ENTER;
    OHOS::ResourceSchedule::ResSchedClient::GetInstance().ReportData(
        OHOS::ResourceSchedule::ResType::RES_TYPE_NETWORK_LATENCY_REQUEST, MODE_DISABLE,
        {{LOW_LATENCY_KEY, FI_PKG_NAME}});
}

void InputEventInterceptor::TurnOnChannelScan()
{
    scanState_ = true;
    if (SetWifiScene(RESTORE_SCENE) != RET_OK) {
        scanState_ = false;
        FI_HILOGE("Restore scene failed");
    }
}

int32_t InputEventInterceptor::SetWifiScene(unsigned int scene)
{
    CALL_DEBUG_ENTER;
    Hid2dUpperScene upperScene;
    upperScene.scene = scene;
    upperScene.fps = UPPER_SCENE_FPS;
    upperScene.bw = UPPER_SCENE_BW;
    if (Hid2dSetUpperScene(WIFI_INTERFACE_NAME.c_str(), &upperScene) != RET_OK) {
        FI_HILOGE("Set wifi scene failed");
        return RET_ERR;
    }
    return RET_OK;
}

void InputEventInterceptor::ReportPointerEvent(std::shared_ptr<MMI::PointerEvent> pointerEvent)
{
    MMI::PointerEvent::PointerItem pointerItem;

    if (!pointerEvent->GetPointerItem(pointerEvent->GetPointerId(), pointerItem)) {
        FI_HILOGE("Corrupted pointer event");
        return;
    }
    auto ret = sender_.Send(CooperateEvent(
        CooperateEventType::INPUT_POINTER_EVENT,
        InputPointerEvent {
            .deviceId = pointerEvent->GetDeviceId(),
            .pointerAction = pointerEvent->GetPointerAction(),
            .sourceType = pointerEvent->GetSourceType(),
            .position = Coordinate {
                .x = pointerItem.GetDisplayX(),
                .y = pointerItem.GetDisplayY(),
            }
        }));
    if (ret != Channel<CooperateEvent>::NO_ERROR) {
        FI_HILOGE("Failed to send event via channel, error:%{public}d", ret);
    }
}

void InputEventInterceptor::RefreshActivity()
{
    if (!PowerMgr::PowerMgrClient::GetInstance().RefreshActivity(
        PowerMgr::UserActivityType::USER_ACTIVITY_TYPE_TOUCH)) {
        FI_HILOGE("RefreshActivity Failed");
    }
    return;
}
} // namespace Cooperate
} // namespace DeviceStatus
} // namespace Msdp
} // namespace OHOS
