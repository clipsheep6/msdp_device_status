/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

#include "mouse_event_handler.h"

#include <cinttypes>

#include "input-event-codes.h"

#include "define_multimodal.h"
#include "input_device_manager.h"
#include "input_event_handler.h"
#include "input_windows_manager.h"
#include "mouse_device_state.h"
#include "timer_manager.h"
#include "util.h"
#include "util_ex.h"

namespace OHOS {
namespace MMI {
namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, MMI_LOG_DOMAIN, "MouseEventHandler"};
const std::array<int32_t, 6> SPEED_NUMS { 5, 16, 23, 32, 41, 128 };
const std::array<double, 6> SPEED_GAINS { 0.6, 1.0, 1.2, 1.8, 2.1, 2.8 };
constexpr double DOUBLE_ZERO = 1e-15;

void AdjustMotionNumble(double &val)
{
    if (val > DOUBLE_ZERO) {
        val = ceil(val);
    } else if (fabs(val) <= DOUBLE_ZERO) {
        val = 0.0;
    } else {
        val = floor(val);
    }
}
} // namespace
MouseEventHandler::MouseEventHandler()
{
    pointerEvent_ = PointerEvent::Create();
    CHKPL(pointerEvent_);
}

std::shared_ptr<PointerEvent> MouseEventHandler::GetPointerEvent() const
{
    return pointerEvent_;
}

double MouseEventHandler::GetSpeedGain(const double &speed) const
{
    int32_t num = static_cast<int32_t>(ceil(abs(speed)));
    for (size_t i = 0; i < SPEED_NUMS.size(); ++i) {
        if (num <= SPEED_NUMS[i]) {
            return SPEED_GAINS[i];
        }
    }
    return SPEED_GAINS.back();
}

int32_t MouseEventHandler::HandleMotionInner(libinput_event_pointer* data)
{
    CALL_DEBUG_ENTER;
    CHKPR(data, ERROR_NULL_POINTER);
    CHKPR(pointerEvent_, ERROR_NULL_POINTER);
    pointerEvent_->SetPointerAction(PointerEvent::POINTER_ACTION_MOVE);
    pointerEvent_->SetButtonId(buttonId_);

    InitAbsolution();
    if (currentDisplayId_ == -1) {
        absolutionX_ = -1;
        absolutionY_ = -1;
        MMI_HILOGI("The currentDisplayId_ is -1");
        return RET_ERR;
    }

    int32_t ret = HandleMotionCorrection(data);
    if (ret != RET_OK) {
        MMI_HILOGE("Failed to handle motion correction");
        return ret;
    }

    WinMgr->UpdateAndAdjustMouseLocation(currentDisplayId_, absolutionX_, absolutionY_);
    pointerEvent_->SetTargetDisplayId(currentDisplayId_);
    MMI_HILOGD("Change Coordinate : x:%{public}lf,y:%{public}lf", absolutionX_, absolutionY_);
    return RET_OK;
}

int32_t MouseEventHandler::HandleMotionCorrection(libinput_event_pointer* data)
{
    CALL_DEBUG_ENTER;
    CHKPR(data, ERROR_NULL_POINTER);

    uint64_t usec = libinput_event_pointer_get_time_usec(data);
    if (usec <= lastEventTime_) {
        MMI_HILOGE("The new time less than or equal to the last time");
        return RET_ERR;
    }
    uint64_t timeDiff = usec - lastEventTime_;

    double dx = libinput_event_pointer_get_dx(data);
    double dy = libinput_event_pointer_get_dy(data);
    DisplayGroupInfo displayGroupInfo = WinMgr->GetDisplayGroupInfo();
    double correctionX = (dx / static_cast<double>(timeDiff)) * static_cast<double>(speed_) *
                         GetSpeedGain(dx) * static_cast<double>(displayGroupInfo.displaysInfo[0].width);
    double correctionY = (dy / static_cast<double>(timeDiff)) * static_cast<double>(speed_) *
                         GetSpeedGain(dy) * static_cast<double>(displayGroupInfo.displaysInfo[0].height);
    AdjustMotionNumble(correctionX);
    AdjustMotionNumble(correctionY);
    MMI_HILOGD("dx:%{public}lf, dy:%{public}lf, correctionX:%{public}lf, correctionY:%{public}lf,"
               "timeDiff:%{public}ju, width:%{public}d, height:%{public}d",
               dx, dy, correctionX, correctionY,
               timeDiff, displayGroupInfo.displaysInfo[0].width, displayGroupInfo.displaysInfo[0].height);
    absolutionX_ += correctionX;
    absolutionY_ += correctionY;
    lastEventTime_ = usec;
    return RET_OK;
}

void MouseEventHandler::InitAbsolution()
{
    if (absolutionX_ != -1 || absolutionY_ != -1 || currentDisplayId_ != -1) {
        return;
    }
    MMI_HILOGD("Init absolution");
    auto dispalyGroupInfo = WinMgr->GetDisplayGroupInfo();
    if (dispalyGroupInfo.displaysInfo.empty()) {
        MMI_HILOGI("The displayInfo is empty");
        return;
    }
    currentDisplayId_ = dispalyGroupInfo.displaysInfo[0].id;
    absolutionX_ = dispalyGroupInfo.displaysInfo[0].width * 1.0 / 2;
    absolutionY_ = dispalyGroupInfo.displaysInfo[0].height * 1.0 / 2;
}

int32_t MouseEventHandler::HandleButtonInner(libinput_event_pointer* data)
{
    CALL_DEBUG_ENTER;
    CHKPR(data, ERROR_NULL_POINTER);
    MMI_HILOGD("Current action:%{public}d", pointerEvent_->GetPointerAction());

    auto ret = HandleButtonValueInner(data);
    if (ret != RET_OK) {
        MMI_HILOGE("The button value does not exist");
        return RET_ERR;
    }
    uint32_t button = libinput_event_pointer_get_button(data);
    auto state = libinput_event_pointer_get_button_state(data);
    if (state == LIBINPUT_BUTTON_STATE_RELEASED) {
        MouseState->MouseBtnStateCounts(button, BUTTON_STATE_RELEASED);
        pointerEvent_->SetPointerAction(PointerEvent::POINTER_ACTION_BUTTON_UP);
        pointerEvent_->DeleteReleaseButton(button);
        isPressed_ = false;
        buttonId_ = PointerEvent::BUTTON_NONE;
    } else if (state == LIBINPUT_BUTTON_STATE_PRESSED) {
        MouseState->MouseBtnStateCounts(button, BUTTON_STATE_PRESSED);
        pointerEvent_->SetPointerAction(PointerEvent::POINTER_ACTION_BUTTON_DOWN);
        pointerEvent_->SetButtonPressed(button);
        isPressed_ = true;
        buttonId_ = pointerEvent_->GetButtonId();
    } else {
        MMI_HILOGE("Unknown state, state:%{public}u", state);
        return RET_ERR;
    }
    return RET_OK;
}

int32_t MouseEventHandler::HandleButtonValueInner(libinput_event_pointer* data)
{
    CALL_DEBUG_ENTER;
    CHKPR(data, ERROR_NULL_POINTER);

    uint32_t button = libinput_event_pointer_get_button(data);
    switch (button) {
        case BTN_LEFT: {
            pointerEvent_->SetButtonId(PointerEvent::MOUSE_BUTTON_LEFT);
            break;
        }
        case BTN_RIGHT: {
            pointerEvent_->SetButtonId(PointerEvent::MOUSE_BUTTON_RIGHT);
            break;
        }
        case BTN_MIDDLE: {
            pointerEvent_->SetButtonId(PointerEvent::MOUSE_BUTTON_MIDDLE);
            break;
        }
        case BTN_SIDE: {
            pointerEvent_->SetButtonId(PointerEvent::MOUSE_BUTTON_SIDE);
            break;
        }
        case BTN_EXTRA: {
            pointerEvent_->SetButtonId(PointerEvent::MOUSE_BUTTON_EXTRA);
            break;
        }
        case BTN_FORWARD: {
            pointerEvent_->SetButtonId(PointerEvent::MOUSE_BUTTON_FORWARD);
            break;
        }
        case BTN_BACK: {
            pointerEvent_->SetButtonId(PointerEvent::MOUSE_BUTTON_BACK);
            break;
        }
        case BTN_TASK: {
            pointerEvent_->SetButtonId(PointerEvent::MOUSE_BUTTON_TASK);
            break;
        }
        default: {
            MMI_HILOGE("Unknown btn, btn:%{public}u", button);
            return RET_ERR;
        }
    }
    return RET_OK;
}

int32_t MouseEventHandler::HandleAxisInner(libinput_event_pointer* data)
{
    CALL_DEBUG_ENTER;
    CHKPR(data, ERROR_NULL_POINTER);
    if (buttonId_ == PointerEvent::BUTTON_NONE && pointerEvent_->GetButtonId() != PointerEvent::BUTTON_NONE) {
        pointerEvent_->SetButtonId(PointerEvent::BUTTON_NONE);
    }
    if (TimerMgr->IsExist(timerId_)) {
        pointerEvent_->SetPointerAction(PointerEvent::POINTER_ACTION_AXIS_UPDATE);
        TimerMgr->ResetTimer(timerId_);
        MMI_HILOGD("Axis update");
    } else {
        static constexpr int32_t timeout = 100;
        std::weak_ptr<MouseEventHandler> weakPtr = shared_from_this();
        timerId_ = TimerMgr->AddTimer(timeout, 1, [weakPtr]() {
            CALL_DEBUG_ENTER;
            auto sharedPtr = weakPtr.lock();
            CHKPV(sharedPtr);
            MMI_HILOGD("timer:%{public}d", sharedPtr->timerId_);
            sharedPtr->timerId_ = -1;
            auto pointerEvent = sharedPtr->GetPointerEvent();
            CHKPV(pointerEvent);
            pointerEvent->SetPointerAction(PointerEvent::POINTER_ACTION_AXIS_END);
            auto inputEventNormalizeHandler = InputHandler->GetInputEventNormalizeHandler();
            CHKPV(inputEventNormalizeHandler);
            inputEventNormalizeHandler->HandlePointerEvent(pointerEvent);
        });

        pointerEvent_->SetPointerAction(PointerEvent::POINTER_ACTION_AXIS_BEGIN);
        MMI_HILOGD("Axis begin");
    }

    if (libinput_event_pointer_has_axis(data, LIBINPUT_POINTER_AXIS_SCROLL_VERTICAL)) {
        double axisValue = libinput_event_pointer_get_axis_value(data, LIBINPUT_POINTER_AXIS_SCROLL_VERTICAL);
        pointerEvent_->SetAxisValue(PointerEvent::AXIS_TYPE_SCROLL_VERTICAL, axisValue);
    }
    if (libinput_event_pointer_has_axis(data, LIBINPUT_POINTER_AXIS_SCROLL_HORIZONTAL)) {
        double axisValue = libinput_event_pointer_get_axis_value(data, LIBINPUT_POINTER_AXIS_SCROLL_HORIZONTAL);
        pointerEvent_->SetAxisValue(PointerEvent::AXIS_TYPE_SCROLL_HORIZONTAL, axisValue);
    }
    return RET_OK;
}

void MouseEventHandler::HandlePostInner(libinput_event_pointer* data, int32_t deviceId,
                                        PointerEvent::PointerItem& pointerItem)
{
    CALL_DEBUG_ENTER;
    CHKPV(data);
    auto mouseInfo = WinMgr->GetMouseInfo();
    MouseState->SetMouseCoords(mouseInfo.physicalX, mouseInfo.physicalY);
    pointerItem.SetDisplayX(mouseInfo.physicalX);
    pointerItem.SetDisplayY(mouseInfo.physicalY);
    pointerItem.SetWindowX(0);
    pointerItem.SetWindowY(0);
    pointerItem.SetPointerId(0);
    pointerItem.SetPressed(isPressed_);

    int64_t time = GetSysClockTime();
    pointerItem.SetDownTime(time);
    pointerItem.SetWidth(0);
    pointerItem.SetHeight(0);
    pointerItem.SetPressure(0);
    pointerItem.SetToolType(PointerEvent::TOOL_TYPE_FINGER);
    pointerItem.SetDeviceId(deviceId);

    pointerEvent_->UpdateId();
    pointerEvent_->UpdatePointerItem(pointerEvent_->GetPointerId(), pointerItem);
    pointerEvent_->SetSourceType(PointerEvent::SOURCE_TYPE_MOUSE);
    pointerEvent_->SetActionTime(time);
    pointerEvent_->SetActionStartTime(time);
    pointerEvent_->SetDeviceId(deviceId);
    pointerEvent_->SetPointerId(0);
    pointerEvent_->SetTargetDisplayId(currentDisplayId_);
    pointerEvent_->SetTargetWindowId(-1);
    pointerEvent_->SetAgentWindowId(-1);
}

int32_t MouseEventHandler::Normalize(struct libinput_event *event)
{
    CALL_DEBUG_ENTER;
    CHKPR(event, ERROR_NULL_POINTER);
    auto data = libinput_event_get_pointer_event(event);
    CHKPR(data, ERROR_NULL_POINTER);
    CHKPR(pointerEvent_, ERROR_NULL_POINTER);
    pointerEvent_->ClearAxisValue();
    int32_t result;
    const int32_t type = libinput_event_get_type(event);
    switch (type) {
        case LIBINPUT_EVENT_POINTER_MOTION:
        case LIBINPUT_EVENT_POINTER_MOTION_ABSOLUTE: {
            result = HandleMotionInner(data);
            break;
        }
        case LIBINPUT_EVENT_POINTER_BUTTON: {
            result = HandleButtonInner(data);
            break;
        }
        case LIBINPUT_EVENT_POINTER_AXIS: {
            result = HandleAxisInner(data);
            break;
        }
        default: {
            MMI_HILOGE("Unknow type:%{public}d", type);
            return RET_ERR;
        }
    }
    int32_t deviceId = InputDevMgr->FindInputDeviceId(libinput_event_get_device(event));
    PointerEvent::PointerItem pointerItem;
    HandlePostInner(data, deviceId, pointerItem);
    DumpInner();
    return result;
}
#ifdef OHOS_BUILD_ENABLE_POINTER_DRAWING
void MouseEventHandler::HandleMotionMoveMouse(int32_t offsetX, int32_t offsetY)
{
    CALL_DEBUG_ENTER;
    CHKPV(pointerEvent_);
    pointerEvent_->SetPointerAction(PointerEvent::POINTER_ACTION_MOVE);
    InitAbsolution();
    absolutionX_ += offsetX;
    absolutionY_ += offsetY;
    WinMgr->UpdateAndAdjustMouseLocation(currentDisplayId_, absolutionX_, absolutionY_);
}

void MouseEventHandler::HandlePostMoveMouse(PointerEvent::PointerItem& pointerItem)
{
    CALL_DEBUG_ENTER;
    auto mouseInfo = WinMgr->GetMouseInfo();
    CHKPV(pointerEvent_);
    MouseState->SetMouseCoords(mouseInfo.physicalX, mouseInfo.physicalY);
    pointerItem.SetDisplayX(mouseInfo.physicalX);
    pointerItem.SetDisplayY(mouseInfo.physicalY);
    pointerItem.SetWindowX(0);
    pointerItem.SetWindowY(0);
    pointerItem.SetPointerId(0);
    pointerItem.SetPressed(isPressed_);

    int64_t time = GetSysClockTime();
    pointerItem.SetDownTime(time);
    pointerItem.SetWidth(0);
    pointerItem.SetHeight(0);
    pointerItem.SetPressure(0);

    pointerEvent_->UpdateId();
    pointerEvent_->UpdatePointerItem(pointerEvent_->GetPointerId(), pointerItem);
    pointerEvent_->SetSourceType(PointerEvent::SOURCE_TYPE_MOUSE);
    pointerEvent_->SetActionTime(time);
    pointerEvent_->SetActionStartTime(time);

    pointerEvent_->SetPointerId(0);
    pointerEvent_->SetTargetDisplayId(-1);
    pointerEvent_->SetTargetWindowId(-1);
    pointerEvent_->SetAgentWindowId(-1);
}

bool MouseEventHandler::NormalizeMoveMouse(int32_t offsetX, int32_t offsetY)
{
    CALL_DEBUG_ENTER;
    CHKPF(pointerEvent_);
    bool bHasPoinerDevice = InputDevMgr->HasPointerDevice();
    if (!bHasPoinerDevice) {
        MMI_HILOGE("There hasn't any pointer device");
        return false;
    }
    
    PointerEvent::PointerItem pointerItem;
    HandleMotionMoveMouse(offsetX, offsetY);
    HandlePostMoveMouse(pointerItem);
    DumpInner();
    return bHasPoinerDevice;
}
#endif // OHOS_BUILD_ENABLE_POINTER_DRAWING

void MouseEventHandler::DumpInner()
{
    PrintEventData(pointerEvent_);
}

void MouseEventHandler::Dump(int32_t fd, const std::vector<std::string> &args)
{
    CALL_DEBUG_ENTER;
    PointerEvent::PointerItem item;
    CHKPV(pointerEvent_);
    pointerEvent_->GetPointerItem(pointerEvent_->GetPointerId(), item);
    mprintf(fd, "Mouse device state information:\t");
    mprintf(fd,
            "PointerId:%d | SourceType:%s | PointerAction:%s | WindowX:%d | WindowY:%d | ButtonId:%d "
            "| AgentWindowId:%d | TargetWindowId:%d | DownTime:%" PRId64 " | IsPressed:%s \t",
            pointerEvent_->GetPointerId(), pointerEvent_->DumpSourceType(), pointerEvent_->DumpPointerAction(),
            item.GetWindowX(), item.GetWindowY(), pointerEvent_->GetButtonId(), pointerEvent_->GetAgentWindowId(),
            pointerEvent_->GetTargetWindowId(), item.GetDownTime(), item.IsPressed() ? "true" : "false");
}
} // namespace MMI
} // namespace OHOS
