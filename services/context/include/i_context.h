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

#ifndef OHOS_MSDP_DEVICE_STATUS_I_CONTEXT_H
#define OHOS_MSDP_DEVICE_STATUS_I_CONTEXT_H

#include "input_manager.h"

#include "i_delegate_tasks.h"
#include "i_device_manager.h"
#include "i_timer_manager.h"

struct libinput_event;
struct libinput_device;

namespace OHOS {
namespace Msdp {
namespace DeviceStatus {
struct MouseLocation {
    int32_t physicalX { 0 };
    int32_t physicalY { 0 };
};

class IContext {
public:
    IContext() = default;
    virtual ~IContext() = default;

    virtual IDelegateTasks& GetDelegateTasks() = 0;
    virtual IDeviceManager& GetDeviceManager() = 0;
    virtual ITimerManager& GetTimerManager() = 0;

    virtual int32_t FindInputDeviceId(struct libinput_device* inputDevice) = 0;
    virtual std::shared_ptr<::OHOS::MMI::PointerEvent> GetPointerEvent() = 0;
    virtual MouseLocation GetMouseInfo() = 0;
    virtual int32_t SetPointerVisible(int32_t pid, bool visible) = 0;
    virtual void SelectAutoRepeat(std::shared_ptr<MMI::KeyEvent>& keyEvent) = 0;
    virtual void SetJumpInterceptState(bool isJump) = 0;
    virtual bool IsRemote(struct libinput_device *inputDevice) = 0;
    virtual const ::OHOS::MMI::DisplayGroupInfo& GetDisplayGroupInfo() = 0;
    virtual void SetAbsolutionLocation(double xPercent, double yPercent) = 0;
};
} // namespace DeviceStatus
} // namespace Msdp
} // namespace OHOS
#endif // OHOS_MSDP_DEVICE_STATUS_I_CONTEXT_H