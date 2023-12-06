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

#include <atomic>
#include <string>

#include "extra_data.h"
#include "i_input_event_consumer.h"
#include "input_manager.h"
#include "pixel_map.h"

#include "devicestatus_define.h"
#include "drag_data.h"
#include "drag_drawing.h"
#include "event_hub.h"
#include "i_context.h"
#include "state_change_notify.h"
#include "stream_session.h"

namespace OHOS {
namespace Msdp {
namespace DeviceStatus {
class DragManager : public IDragManager {
public:
    DragManager() {}
    DISALLOW_COPY_AND_MOVE(DragManager);
    ~DragManager();

    int32_t Init(IContext* context);
    void OnSessionLost(SessionPtr session);
    int32_t AddListener(SessionPtr session);
    int32_t RemoveListener(SessionPtr session);
    int32_t AddSubscriptListener(SessionPtr session);
    int32_t RemoveSubscriptListener(SessionPtr session);
    int32_t StartDrag(const DragData &dragData, SessionPtr sess) override;
    int32_t StopDrag(const DragDropResult &dropResult) override;
    int32_t GetDragTargetPid() const;
    int32_t GetUdKey(std::string &udKey) const;
    void SendDragData(int32_t targetTid, const std::string &udKey);
    int32_t UpdateDragStyle(DragCursorStyle style, int32_t targetPid, int32_t targetTid);
    int32_t UpdateShadowPic(const ShadowInfo &shadowInfo);
    int32_t GetDragData(DragData &dragData);
    int32_t GetDragState(DragState &dragState);
    void GetAllowDragState(bool &isAllowDrag) override;
    void DragCallback(std::shared_ptr<MMI::PointerEvent> pointerEvent);
    void OnDragUp(std::shared_ptr<MMI::PointerEvent> pointerEvent);
    void OnDragMove(std::shared_ptr<MMI::PointerEvent> pointerEvent);
    int32_t OnSetDragWindowVisible(bool visible) override;
    MMI::ExtraData GetExtraData(bool appended) const override;
    int32_t OnGetShadowOffset(int32_t &offsetX, int32_t &offsetY, int32_t &width, int32_t &height);
    void Dump(int32_t fd) const override;
    void RegisterStateChange(std::function<void(DragState)> callback) override;
    void RegisterNotifyPullUp(std::function<void(void)> callback) override;
    void SetPointerEventFilterTime(int64_t filterTime) override;
    void MoveTo(int32_t x, int32_t y) override;
    DragResult GetDragResult() const override;
    DragState GetDragState() const override;
    void SetDragState(DragState state) override;
    int32_t UpdatePreviewStyle(const PreviewStyle &previewStyle) override;
    int32_t UpdatePreviewStyleWithAnimation(const PreviewStyle &previewStyle,
        const PreviewAnimation &animation) override;
    int32_t GetDragSummary(std::map<std::string, int64_t> &summarys);
    void DragKeyEventCallback(std::shared_ptr<MMI::KeyEvent> keyEvent);
    int32_t EnterTextEditorArea(bool enable);
    int32_t GetDragAction(DragAction &dragAction) const;
    int32_t GetExtraInfo(std::string &extraInfo) const;
#ifdef OHOS_DRAG_ENABLE_INTERCEPTOR
    class InterceptorConsumer : public MMI::IInputEventConsumer {
    public:
        InterceptorConsumer(IContext *context,
            std::function<void (std::shared_ptr<MMI::PointerEvent>)> cb)
            : context_(context),
            pointerEventCallback_(cb) {}
        void OnInputEvent(std::shared_ptr<MMI::KeyEvent> keyEvent) const override;
        void OnInputEvent(std::shared_ptr<MMI::PointerEvent> pointerEvent) const override;
        void OnInputEvent(std::shared_ptr<MMI::AxisEvent> axisEvent) const override;
    private:
        IContext* context_ { nullptr };
        std::function<void (std::shared_ptr<MMI::PointerEvent>)> pointerEventCallback_ { nullptr };
    };
#endif // OHOS_DRAG_ENABLE_INTERCEPTOR

#ifdef OHOS_DRAG_ENABLE_MONITOR
    class MonitorConsumer : public MMI::IInputEventConsumer {
    public:
        explicit MonitorConsumer(
            std::function<void (std::shared_ptr<MMI::PointerEvent>)> cb) : pointerEventCallback_(cb) {}
        void OnInputEvent(std::shared_ptr<MMI::KeyEvent> keyEvent) const override;
        void OnInputEvent(std::shared_ptr<MMI::PointerEvent> pointerEvent) const override;
        void OnInputEvent(std::shared_ptr<MMI::AxisEvent> axisEvent) const override;
    private:
        std::function<void (std::shared_ptr<MMI::PointerEvent>)> pointerEventCallback_;
    };
#endif //OHOS_DRAG_ENABLE_MONITOR
private:
    void PrintDragData(const DragData &dragData);
    int32_t AddDragEventHandler(int32_t sourceType);
    int32_t AddPointerEventHandler(uint32_t deviceTags);
    int32_t AddKeyEventMointor();
    int32_t RemoveKeyEventMointor();
    int32_t RemovePointerEventHandler();
    int32_t NotifyDragResult(DragResult result);
    int32_t NotifyHideIcon();
    int32_t InitDataManager(const DragData &dragData) const;
    int32_t OnStartDrag();
    int32_t OnStopDrag(DragResult result, bool hasCustomAnimation);
    std::string GetDragState(DragState value) const;
    std::string GetDragResult(DragResult value) const;
    std::string GetDragCursorStyle(DragCursorStyle value) const;
    static MMI::ExtraData CreateExtraData(bool appended);
    void StateChangedNotify(DragState state);
    int32_t HandleDragResult(DragResult result, bool hasCustomAnimation);
    void HandleCtrlKeyDown();
    void HandleCtrlKeyUp();
private:
    int32_t timerId_ { -1 };
    StateChangeNotify stateNotify_;
    DragState dragState_ { DragState::STOP };
    DragResult dragResult_ { DragResult::DRAG_FAIL };
    int32_t keyEventMonitorId_ { -1 };
    std::atomic<DragAction> dragAction_ { DragAction::MOVE };
#ifdef OHOS_DRAG_ENABLE_INTERCEPTOR
    int32_t pointerEventInterceptorId_ { -1 };
#endif // OHOS_DRAG_ENABLE_INTERCEPTOR
#ifdef OHOS_DRAG_ENABLE_MONITOR
    int32_t pointerEventMonitorId_ { -1 };
#endif //OHOS_DRAG_ENABLE_MONITOR
    SessionPtr dragOutSession_ { nullptr };
    DragDrawing dragDrawing_;
    IContext* context_ { nullptr };
    std::function<void(DragState)> stateChangedCallback_ { nullptr };
    std::function<void(void)> notifyPUllUpCallback_ { nullptr };
    std::shared_ptr<EventHub> eventHub_ { nullptr };
    sptr<ISystemAbilityStatusChange> statusListener_ = nullptr;
};
} // namespace DeviceStatus
} // namespace Msdp
} // namespace OHOS
#endif // DRAG_MANAGER_H
