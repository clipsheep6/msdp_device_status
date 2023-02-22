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

#ifndef DRAG_MANAGER_H
#define DRAG_MANAGER_H

#include <string>

#include "extra_data.h"
#include "i_input_event_consumer.h"
#include "input_manager.h"
#include "pixel_map.h"

#include "devicestatus_define.h"
#include "drag_data.h"
#include "state_change_notify.h"
#include "stream_session.h"

namespace OHOS {
namespace Msdp {
namespace DeviceStatus {
class DragManager {
public:
    DragManager() : monitorConsumer_(std::make_shared<MonitorConsumer>(nullptr))
    {}
    ~DragManager() = default;

    void OnSessionLost(SessionPtr session);
    int32_t AddListener(SessionPtr session);
    int32_t RemoveListener(SessionPtr session);
    int32_t StartDrag(const DragData &dragData, SessionPtr sess);
    int32_t StopDrag(int32_t result);
    int32_t GetDragTargetPid() const;
    void DragCallback(std::shared_ptr<MMI::PointerEvent> pointerEvent);
    void OnDragUp(std::shared_ptr<MMI::PointerEvent> pointerEvent);
    void OnDragMove(std::shared_ptr<MMI::PointerEvent> pointerEvent);
    int32_t OnRegisterThumbnailDraw(SessionPtr sess);
    int32_t OnUnregisterThumbnailDraw(SessionPtr sess);
    class MonitorConsumer : public MMI::IInputEventConsumer {
    public:
        explicit MonitorConsumer(std::function<void (std::shared_ptr<MMI::PointerEvent>)> cb) : callback_(cb)
        {}
        void OnInputEvent(std::shared_ptr<MMI::KeyEvent> keyEvent) const override;
        void OnInputEvent(std::shared_ptr<MMI::PointerEvent> pointerEvent) const override;
        void OnInputEvent(std::shared_ptr<MMI::AxisEvent> axisEvent) const override;
    private:
        std::function<void (std::shared_ptr<MMI::PointerEvent>)> callback_;
    };
private:
    int32_t NotifyDragResult(int32_t result);
    MMI::ExtraData CreateExtraData(bool appended) const;
    int32_t InitDragData(const DragData &dragData) const;
    int32_t LaunchDrag();
    void RestoreDrag();
    void FetchDragTargetPid(std::shared_ptr<MMI::PointerEvent> pointerEvent);
private:
    StateChangeNotify stateNotify_;
    DragState dragState_ { DragState::FREE };
    int32_t monitorId_ { -1 };
    int32_t dragTargetPid_ { -1 };
    SessionPtr dragOutSession_ { nullptr };
    std::shared_ptr<MonitorConsumer> monitorConsumer_ { nullptr };
};
#define INPUT_MANAGER  MMI::InputManager::GetInstance()
} // namespace DeviceStatus
} // namespace Msdp
} // namespace OHOS
#endif // DRAG_MANAGER_H
