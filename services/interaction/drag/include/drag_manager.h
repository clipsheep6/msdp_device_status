/*
 * Copyright (c) 2023-2024 Huawei Device Co., Ltd.
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

#ifndef OHOS_BUILD_ENABLE_ARKUI_X
#include "extra_data.h"
#include "i_input_event_consumer.h"
#include "input_manager.h"
#endif // OHOS_BUILD_ENABLE_ARKUI_X
#include "pixel_map.h"

#ifndef OHOS_BUILD_ENABLE_ARKUI_X
#include "collaboration_service_status_change.h"
#include "display_change_event_listener.h"
#endif // OHOS_BUILD_ENABLE_ARKUI_X
#include "devicestatus_define.h"
#include "drag_data.h"
#include "drag_drawing.h"
#include "id_factory.h"
#ifndef OHOS_BUILD_ENABLE_ARKUI_X
#include "event_hub.h"
#include "i_context.h"
#include "state_change_notify.h"
#else
#include "i_drag_manager.h"
#endif // OHOS_BUILD_ENABLE_ARKUI_X

namespace OHOS {
namespace Msdp {
namespace DeviceStatus {
class DragManager : public IDragManager,
                    public IdFactory<int32_t> {
public:
#ifdef OHOS_BUILD_ENABLE_ARKUI_X
    static DragManager &GetInstance();
#endif // OHOS_BUILD_ENABLE_ARKUI_X
    DragManager() = default;
    DISALLOW_COPY_AND_MOVE(DragManager);
    ~DragManager();

#ifndef OHOS_BUILD_ENABLE_ARKUI_X
    int32_t Init(IContext* context);
    void OnSessionLost(SocketSessionPtr session);
    int32_t AddListener(int32_t pid) override;
    int32_t RemoveListener(int32_t pid) override;
    int32_t AddSubscriptListener(int32_t pid) override;
    int32_t RemoveSubscriptListener(int32_t pid) override;
    int32_t StartDrag(
        const DragData &dragData, int32_t pid, const std::string &peerNetId = "",
        bool isLongPressDrag = false) override;
#else
    int32_t StartDrag(const DragData &dragData) override;
    int32_t UpdatePointerAction(std::shared_ptr<MMI::PointerEvent> pointerEvent);
#endif // OHOS_BUILD_ENABLE_ARKUI_X
    int32_t StopDrag(const DragDropResult &dropResult, const std::string &packageName = "", int32_t pid = -1) override;
    int32_t GetDragTargetPid() const override;
    int32_t GetUdKey(std::string &udKey) const override;
    void SendDragData(int32_t targetTid, const std::string &udKey);
#ifndef OHOS_BUILD_ENABLE_ARKUI_X
    int32_t UpdateDragStyle(
        DragCursorStyle style, int32_t targetPid, int32_t targetTid, int32_t eventId = -1) override;
#else
    int32_t UpdateDragStyle(DragCursorStyle style) override;
#endif // OHOS_BUILD_ENABLE_ARKUI_X
    int32_t UpdateShadowPic(const ShadowInfo &shadowInfo) override;
    int32_t GetDragData(DragData &dragData) override;
    int32_t GetDragState(DragState &dragState) override;
    DragCursorStyle GetDragStyle() const override;
    void GetAllowDragState(bool &isAllowDrag) override;
    void DragCallback(std::shared_ptr<MMI::PointerEvent> pointerEvent);
    int32_t OnDragUp(std::shared_ptr<MMI::PointerEvent> pointerEvent);
    void OnDragCancel(std::shared_ptr<MMI::PointerEvent> pointerEvent);
#ifndef OHOS_BUILD_ENABLE_ARKUI_X
    void OnDragMove(std::shared_ptr<MMI::PointerEvent> pointerEvent);
#else
    int32_t OnDragMove(std::shared_ptr<MMI::PointerEvent> pointerEvent);
#endif // OHOS_BUILD_ENABLE_ARKUI_X
    int32_t OnSetDragWindowVisible(bool visible, bool isForce = false, bool isZoomInAndAlphaChanged = false) override;
    MMI::ExtraData GetExtraData(bool appended) const override;
    int32_t OnGetShadowOffset(ShadowOffset &shadowOffset) override;
    bool GetControlCollaborationVisible() const override;
    void SetControlCollaborationVisible(bool visible) override;
#ifndef OHOS_BUILD_ENABLE_ARKUI_X
    void Dump(int32_t fd) const override;
    void RegisterStateChange(std::function<void(DragState)> callback) override;
    void UnregisterStateChange() override;
    void RegisterNotifyPullUp(std::function<void(bool)> callback) override;
    void UnregisterNotifyPullUp() override;
    void RegisterCrossDrag(std::function<void(bool)> callback) override;
    void UnregisterCrossDrag() override;
    void NotifyCrossDrag(bool isButtonDown) override;
#endif // OHOS_BUILD_ENABLE_ARKUI_X
    void SetPointerEventFilterTime(int64_t filterTime) override;
    void MoveTo(int32_t x, int32_t y, bool isMultiSelectedAnimation = true) override;
    DragResult GetDragResult() const override;
    DragState GetDragState() const override;
    void SetDragState(DragState state) override;
    void SetDragOriginDpi(float dragOriginDpi) override;
    int32_t UpdatePreviewStyle(const PreviewStyle &previewStyle) override;
    int32_t UpdatePreviewStyleWithAnimation(const PreviewStyle &previewStyle,
        const PreviewAnimation &animation) override;
    int32_t RotateDragWindowSync(const std::shared_ptr<Rosen::RSTransaction>& rsTransaction = nullptr) override;
    int32_t GetDragSummary(std::map<std::string, int64_t> &summarys) override;
    void DragKeyEventCallback(std::shared_ptr<MMI::KeyEvent> keyEvent);
    int32_t EnterTextEditorArea(bool enable) override;
    int32_t GetDragAction(DragAction &dragAction) const override;
    int32_t GetExtraInfo(std::string &extraInfo) const override;
    int32_t AddPrivilege(int32_t tokenId) override;
    int32_t EraseMouseIcon() override;
#ifndef OHOS_BUILD_ENABLE_ARKUI_X
    int32_t AddSelectedPixelMap(std::shared_ptr<OHOS::Media::PixelMap> pixelMap) override;
#endif // OHOS_BUILD_ENABLE_ARKUI_X
    int32_t RotateDragWindow(Rosen::Rotation rotation) override;
    int32_t ScreenRotate(Rosen::Rotation rotation, Rosen::Rotation lastRotation) override;
    void SetDragWindowScreenId(uint64_t displayId, uint64_t screenId) override;
#ifndef OHOS_BUILD_ENABLE_ARKUI_X
    int32_t SetMouseDragMonitorState(bool state) override;
#ifdef OHOS_DRAG_ENABLE_INTERCEPTOR
    class InterceptorConsumer : public MMI::IInputEventConsumer {
    public:
        InterceptorConsumer(std::function<void (std::shared_ptr<MMI::PointerEvent>)> cb)
            : pointerEventCallback_(cb) {}
        void OnInputEvent(std::shared_ptr<MMI::KeyEvent> keyEvent) const override;
        void OnInputEvent(std::shared_ptr<MMI::PointerEvent> pointerEvent) const override;
        void OnInputEvent(std::shared_ptr<MMI::AxisEvent> axisEvent) const override;
    private:
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
#else
    void SetDragWindow(std::shared_ptr<OHOS::Rosen::Window> window) override;
    void AddDragDestroy(std::function<void()> cb) override;
    void SetSVGFilePath(const std::string &filePath) override;
#endif // OHOS_BUILD_ENABLE_ARKUI_X
private:
    void PrintDragData(const DragData &dragData, const std::string &packageName = "");
#ifndef OHOS_BUILD_ENABLE_ARKUI_X
    int32_t AddDragEventHandler(int32_t sourceType);
    int32_t AddPointerEventHandler(uint32_t deviceTags);
    int32_t AddKeyEventMonitor();
    int32_t RemoveDragEventHandler();
    int32_t RemoveKeyEventMonitor();
    int32_t RemovePointerEventHandler();
    int32_t NotifyDragResult(DragResult result, DragBehavior dragBehavior);
    int32_t NotifyHideIcon();
#endif // OHOS_BUILD_ENABLE_ARKUI_X
    int32_t InitDataManager(const DragData &dragData) const;
    int32_t OnStartDrag(const std::string &packageName = "", int32_t pid = -1);
    int32_t OnStopDrag(DragResult result, bool hasCustomAnimation, const std::string &packageName = "",
        int32_t pid = -1);
    std::string GetDragState(DragState value) const;
    std::string GetDragResult(DragResult value) const;
    std::string GetDragCursorStyle(DragCursorStyle value) const;
    static MMI::ExtraData CreateExtraData(bool appended);
#ifndef OHOS_BUILD_ENABLE_ARKUI_X
    void StateChangedNotify(DragState state);
    int32_t AddDragEvent(const DragData &dragData, const std::string &packageName);
#endif // OHOS_BUILD_ENABLE_ARKUI_X
    void CtrlKeyStyleChangedNotify(DragCursorStyle style, DragAction action);
    int32_t HandleDragResult(DragResult result, bool hasCustomAnimation);
    void HandleCtrlKeyEvent(DragCursorStyle style, DragAction action);
    int32_t OnUpdateDragStyle(DragCursorStyle style);
    void UpdateDragStyleCross();
    inline std::string GetDragStyleName(DragCursorStyle style);
    DragCursorStyle GetRealDragStyle(DragCursorStyle style);
    void GetDragBehavior(const DragDropResult &dropResult, DragBehavior &dragBehavior);
#ifndef OHOS_BUILD_ENABLE_ARKUI_X
    int32_t NotifyAddSelectedPixelMapResult(bool result);
    void ResetMouseDragMonitorInfo();
    void ResetMouseDragMonitorTimerId(const DragData &dragData);
    std::string GetPackageName(int32_t pid);
#endif // OHOS_BUILD_ENABLE_ARKUI_X
#ifndef OHOS_BUILD_ENABLE_ARKUI_X
    void ReportDragWindowVisibleRadarInfo(StageRes stageRes, DragRadarErrCode errCode, const std::string &funcName);
    void ReportDragRadarInfo(struct DragRadarInfo &dragRadarInfo);
    void ReportStartDragRadarInfo(BizState bizState, StageRes stageRes, DragRadarErrCode errCode,
        const std::string &packageName, const std::string &peerNetId);
    void ReportStopDragRadarInfo(BizState bizState, StageRes stageRes, DragRadarErrCode errCode, int32_t pid,
        const std::string &packageName);
    void ReportStartDragFailedRadarInfo(StageRes stageRes, DragRadarErrCode errCode, const std::string &funcName,
        const std::string &packageName);
    void ReportDragUEInfo(struct DragRadarInfo &dragRadarInfo, const std::string &eventDescription);
    void ReportStartDragUEInfo(const std::string &packageName);
    void ReportStopDragUEInfo(const std::string &packageName);
#endif // OHOS_BUILD_ENABLE_ARKUI_X
private:
    int32_t timerId_ { -1 };
    int32_t mouseDragMonitorTimerId_ { -1 };
    DragState dragState_ { DragState::STOP };
    DragResult dragResult_ { DragResult::DRAG_FAIL };
    std::atomic<DragAction> dragAction_ { DragAction::MOVE };
    DragDrawing dragDrawing_;
    bool isControlCollaborationVisible_ { false };
    inline static std::atomic<int32_t> pullId_ { -1 };
#ifndef OHOS_BUILD_ENABLE_ARKUI_X
    StateChangeNotify stateNotify_;
    int32_t keyEventMonitorId_ { -1 };
    IContext* context_ { nullptr };
#ifdef OHOS_DRAG_ENABLE_INTERCEPTOR
    int32_t pointerEventInterceptorId_ { -1 };
#endif // OHOS_DRAG_ENABLE_INTERCEPTOR
#ifdef OHOS_DRAG_ENABLE_MONITOR
    int32_t pointerEventMonitorId_ { -1 };
#endif //OHOS_DRAG_ENABLE_MONITOR
    SocketSessionPtr dragOutSession_ { nullptr };
    std::function<void(DragState)> stateChangedCallback_ { nullptr };
    std::function<void(bool)> notifyPUllUpCallback_ { nullptr };
    std::function<void(bool)> crossDragCallback_ { nullptr };
    std::shared_ptr<EventHub> eventHub_ { nullptr };
    sptr<ISystemAbilityStatusChange> statusListener_ { nullptr };
    sptr<ISystemAbilityStatusChange> displayAbilityStatusChange_ { nullptr };
    sptr<ISystemAbilityStatusChange> appStateObserverStatusChange_ { nullptr };
    sptr<ISystemAbilityStatusChange> CollaborationServiceStatusChange_ { nullptr };
#endif // OHOS_BUILD_ENABLE_ARKUI_X
    uint64_t displayId_ { 0 };
    uint64_t screenId_ { 0 };
    int32_t lastEventId_ { -1 };
    int64_t mouseDragMonitorDisplayX_ { -1 };
    int64_t mouseDragMonitorDisplayY_ { -1 };
    bool mouseDragMonitorState_ { false };
    bool existMouseMoveDragCallback_ { false };
    int32_t lastDisplayId_ { -1 };
    std::string peerNetId_;
    bool isLongPressDrag_ { false };
    bool needLongPressDragAnimation_ { true };
};
} // namespace DeviceStatus
} // namespace Msdp
} // namespace OHOS

#ifdef OHOS_BUILD_ENABLE_ARKUI_X
#define DRAG_MANAGER  OHOS::Msdp::DeviceStatus::DragManager::GetInstance()
#endif // OHOS_BUILD_ENABLE_ARKUI_X
#endif // DRAG_MANAGER_H
