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

#include "input_adapter.h"

#include "input_manager.h"

#include "devicestatus_define.h"

#undef LOG_TAG
#define LOG_TAG "InputAdapter"

namespace OHOS {
namespace Msdp {
namespace DeviceStatus {

int32_t InputAdapter::AddMonitor(std::function<void(std::shared_ptr<MMI::PointerEvent>)> callback)
{
    int32_t monitorId = MMI::InputManager::GetInstance()->AddMonitor(callback);
    if (monitorId < 0) {
        FI_HILOGE("AddMonitor fail");
    }
    return monitorId;
}

int32_t InputAdapter::AddMonitor(std::function<void(std::shared_ptr<MMI::KeyEvent>)> callback)
{
    int32_t monitorId = MMI::InputManager::GetInstance()->AddMonitor(callback);
    if (monitorId < 0) {
        FI_HILOGE("AddMonitor fail");
    }
    return monitorId;
}

int32_t InputAdapter::AddMonitor(std::function<void(std::shared_ptr<MMI::PointerEvent>)> pointCallback,
    std::function<void(std::shared_ptr<MMI::KeyEvent>)> keyCallback, MMI::HandleEventType eventType)
{
    auto monitor = std::make_shared<MonitorConsumer>(pointCallback, keyCallback);
    int32_t monitorId = MMI::InputManager::GetInstance()->AddMonitor(monitor, eventType);
    if (monitorId < 0) {
        FI_HILOGE("AddMonitor fail");
    }
    return monitorId;
}

void InputAdapter::RemoveMonitor(int32_t monitorId)
{
    MMI::InputManager::GetInstance()->RemoveMonitor(monitorId);
}

int32_t InputAdapter::AddInterceptor(std::function<void(std::shared_ptr<MMI::PointerEvent>)> pointCallback)
{
    return AddInterceptor(pointCallback, nullptr);
}

int32_t InputAdapter::AddInterceptor(std::function<void(std::shared_ptr<MMI::KeyEvent>)> keyCallback)
{
    return AddInterceptor(nullptr, keyCallback);
}

int32_t InputAdapter::AddInterceptor(std::function<void(std::shared_ptr<MMI::PointerEvent>)> pointCallback,
                                     std::function<void(std::shared_ptr<MMI::KeyEvent>)> keyCallback)
{
    uint32_t tags { 0u };
    if (pointCallback != nullptr) {
        tags |= MMI::CapabilityToTags(MMI::INPUT_DEV_CAP_POINTER);
    }
    if (keyCallback != nullptr) {
        tags |= MMI::CapabilityToTags(MMI::INPUT_DEV_CAP_KEYBOARD);
    }
    if (tags == 0u) {
        FI_HILOGE("Both interceptors are null");
        return -1;
    }
    auto interceptor = std::make_shared<InterceptorConsumer>(pointCallback, keyCallback);
    constexpr int32_t DEFAULT_PRIORITY { 499 };
    int32_t interceptorId = MMI::InputManager::GetInstance()->AddInterceptor(interceptor, DEFAULT_PRIORITY, tags);
    if (interceptorId < 0) {
        FI_HILOGE("AddInterceptor fail");
    }
    return interceptorId;
}

void InputAdapter::RemoveInterceptor(int32_t interceptorId)
{
    MMI::InputManager::GetInstance()->RemoveInterceptor(interceptorId);
}

int32_t InputAdapter::AddFilter(std::function<bool(std::shared_ptr<MMI::PointerEvent>)> callback)
{
    constexpr int32_t DEFAULT_PRIORITY { 220 };
    auto filter = std::make_shared<PointerFilter>(callback);
    uint32_t tags = CapabilityToTags(MMI::INPUT_DEV_CAP_POINTER);
    int32_t filterId = MMI::InputManager::GetInstance()->AddInputEventFilter(filter, DEFAULT_PRIORITY, tags);
    if (filterId < 0) {
        FI_HILOGE("AddInputEventFilter fail");
    }
    return filterId;
}

void InputAdapter::RemoveFilter(int32_t filterId)
{
    MMI::InputManager::GetInstance()->RemoveInputEventFilter(filterId);
}

int32_t InputAdapter::SetPointerVisibility(bool visible, int32_t priority)
{
    FI_HILOGI("Set pointer visibility, visible:%{public}s", visible ? "true" : "false");
    return MMI::InputManager::GetInstance()->SetPointerVisible(visible, priority);
}

int32_t InputAdapter::SetPointerLocation(int32_t x, int32_t y, int32_t displayId)
{
    return MMI::InputManager::GetInstance()->SetPointerLocation(x, y, displayId);
}

int32_t InputAdapter::EnableInputDevice(bool enable)
{
    return MMI::InputManager::GetInstance()->EnableInputDevice(enable);
}

void InputAdapter::SimulateInputEvent(std::shared_ptr<MMI::PointerEvent> pointerEvent)
{
    MMI::InputManager::GetInstance()->SimulateInputEvent(pointerEvent);
}

void InputAdapter::SimulateInputEvent(std::shared_ptr<MMI::KeyEvent> keyEvent)
{
    MMI::InputManager::GetInstance()->SimulateInputEvent(keyEvent);
}

int32_t InputAdapter::AddVirtualInputDevice(std::shared_ptr<MMI::InputDevice> device, int32_t &deviceId)
{
    return MMI::InputManager::GetInstance()->AddVirtualInputDevice(device, deviceId);
}

int32_t InputAdapter::RemoveVirtualInputDevice(int32_t deviceId)
{
    return MMI::InputManager::GetInstance()->RemoveVirtualInputDevice(deviceId);
}

int32_t InputAdapter::GetPointerSpeed(int32_t &speed)
{
    auto ret = MMI::InputManager::GetInstance()->GetPointerSpeed(speed);
    FI_HILOGI("Get pointerSpeed:%{public}d", speed);
    return ret;
}

int32_t InputAdapter::SetPointerSpeed(int32_t speed)
{
    if (speed < 0) {
        FI_HILOGW("Invalid pointerSpeed:%{public}d", speed);
        return RET_ERR;
    }
    FI_HILOGI("Set pointerSpeed:%{public}d", speed);
    return MMI::InputManager::GetInstance()->SetPointerSpeed(speed);
}

int32_t InputAdapter::GetTouchPadSpeed(int32_t &speed)
{
    auto ret = MMI::InputManager::GetInstance()->GetTouchpadPointerSpeed(speed);
    FI_HILOGI("Get TouchPad Speed:%{public}d", speed);
    return ret;
}

int32_t InputAdapter::SetTouchPadSpeed(int32_t speed)
{
    if (speed < 0) {
        FI_HILOGW("Invalid Touchpad Speed:%{public}d", speed);
        return RET_ERR;
    }
    FI_HILOGI("Set TouchPad Speed:%{public}d", speed);
    return MMI::InputManager::GetInstance()->SetTouchpadPointerSpeed(speed);
}
} // namespace DeviceStatus
} // namespace Msdp
} // namespace OHOS