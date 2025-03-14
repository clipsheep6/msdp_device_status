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

#ifndef I_MOTION_DRAG_H
#define I_MOTION_DRAG_H

#include <event_handler.h>

namespace OHOS {
namespace Msdp {
namespace DeviceStatus {

class IMotionDrag {
public:
    IMotionDrag() = default;
    virtual ~IMotionDrag() = default;

    virtual void Enable(std::shared_ptr<AppExecFwk::EventHandler>) = 0;
    virtual void Disable() = 0;
    virtual void OnRemoteStartCooperateSetPointerButtonDown() = 0;
};
} // namespace DeviceStatus
} // namespace Msdp
} // namespace OHOS
#endif // I_MOTION_DRAG_H
