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

#include "drag_manager.h"

#include <atomic>

#include "display_manager.h"
#include "extra_data.h"
#include "hitrace_meter.h"
#include "pixel_map.h"
#include "udmf_client.h"
#include "unified_types.h"
#include "window_manager.h"

#include "devicestatus_define.h"
#include "drag_data.h"
#include "drag_data_manager.h"
#include "drag_hisysevent.h"
#include "fi_log.h"
#include "proto.h"

namespace OHOS {
namespace Msdp {
namespace DeviceStatus {
namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL { LOG_CORE, MSDP_DOMAIN_ID, "DragManager" };
constexpr int32_t TIMEOUT_MS { 2000 };
constexpr int32_t INTERVAL_MS { 500 };
constexpr uint64_t FOLD_SCREEN_ID { 5 };
std::atomic<int64_t> g_startFilterTime { -1 };
#ifdef OHOS_DRAG_ENABLE_INTERCEPTOR
constexpr int32_t DRAG_PRIORITY { 500 };
#endif // OHOS_DRAG_ENABLE_INTERCEPTOR
} // namespace

DragManager::~DragManager()
{
    EventHub::UnRegisterEvent(eventHub_);
}

int32_t DragManager::Init(IContext* context)
{
    CALL_INFO_TRACE;
    CHKPR(context, RET_ERR);
    context_ = context;
    int32_t repeatCount = 1;
    context_->GetTimerManager().AddTimer(INTERVAL_MS, repeatCount, [this]() {
        if (eventHub_ == nullptr) {
            eventHub_ = EventHub::GetEventHub(context_);
        }
        auto samgrProxy = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
        if (samgrProxy == nullptr) {
            FI_HILOGE("samgrProxy is nullptr");
            return;
        }
        statusListener_ = new (std::nothrow) DragAbilityStatusChange(eventHub_);
        if (statusListener_ == nullptr) {
            FI_HILOGE("statusListener_ is nullptr");
            return;
        }
        int32_t ret = samgrProxy->SubscribeSystemAbility(COMMON_EVENT_SERVICE_ID, statusListener_);
        FI_HILOGI("SubscribeSystemAbility result:%{public}d", ret);
    });
    return RET_OK;
}

void DragManager::OnSessionLost(SessionPtr session)
{
    CALL_INFO_TRACE;
    if (RemoveListener(session) != RET_OK) {
        FI_HILOGE("Failed to clear client listener");
    }
}

int32_t DragManager::AddListener(SessionPtr session)
{
    CALL_INFO_TRACE;
    CHKPR(session, RET_ERR);
    auto info = std::make_shared<StateChangeNotify::MessageInfo>();
    info->session = session;
    info->msgId = MessageId::DRAG_STATE_LISTENER;
    info->msgType = MessageType::NOTIFY_STATE;
    stateNotify_.AddNotifyMsg(info);
    return RET_OK;
}

int32_t DragManager::RemoveListener(SessionPtr session)
{
    CALL_INFO_TRACE;
    CHKPR(session, RET_ERR);
    auto info = std::make_shared<StateChangeNotify::MessageInfo>();
    info->session = session;
    info->msgType = MessageType::NOTIFY_STATE;
    stateNotify_.RemoveNotifyMsg(info);
    return RET_OK;
}

int32_t DragManager::AddSubscriptListener(SessionPtr session)
{
    CALL_INFO_TRACE;
    CHKPR(session, RET_ERR);
    auto info = std::make_shared<StateChangeNotify::MessageInfo>();
    info->session = session;
    info->msgId = MessageId::DRAG_STYLE_LISTENER;
    info->msgType = MessageType::NOTIFY_STYLE;
    stateNotify_.AddNotifyMsg(info);
    return RET_OK;
}

int32_t DragManager::RemoveSubscriptListener(SessionPtr session)
{
    CALL_INFO_TRACE;
    CHKPR(session, RET_ERR);
    auto info = std::make_shared<StateChangeNotify::MessageInfo>();
    info->msgType = MessageType::NOTIFY_STYLE;
    info->session = session;
    stateNotify_.RemoveNotifyMsg(info);

    return RET_OK;
}

void DragManager::PrintDragData(const DragData &dragData)
{
    CALL_INFO_TRACE;
    for (const auto& shadowInfo : dragData.shadowInfos) {
        CHKPV(shadowInfo.pixelMap);
        FI_HILOGI("PixelFormat:%{public}d, PixelAlphaType:%{public}d, PixelAllocatorType:%{public}d,"
            " PixelWidth:%{public}d, PixelHeight:%{public}d, shadowX:%{public}d, shadowY:%{public}d",
            static_cast<int32_t>(shadowInfo.pixelMap->GetPixelFormat()),
            static_cast<int32_t>(shadowInfo.pixelMap->GetAlphaType()),
            static_cast<int32_t>(shadowInfo.pixelMap->GetAllocatorType()),
            shadowInfo.pixelMap->GetWidth(), shadowInfo.pixelMap->GetHeight(), shadowInfo.x, shadowInfo.y);
    }
    std::string summarys;
    for (const auto &[udKey, recordSize] : dragData.summarys) {
        std::string str = udKey + "-" + std::to_string(recordSize) + ";";
        summarys += str;
    }
    FI_HILOGI("SourceType:%{public}d, pointerId:%{public}d, displayId:%{public}d,"
        " displayX:%{public}d, displayY:%{public}d, dragNum:%{public}d,"
        " hasCanceledAnimation:%{public}d, udKey:%{public}s, hasCoordinateCorrected:%{public}d, summarys:%{public}s",
        dragData.sourceType, dragData.pointerId, dragData.displayId, dragData.displayX,
        dragData.displayY, dragData.dragNum, dragData.hasCanceledAnimation,
        GetAnonyString(dragData.udKey).c_str(), dragData.hasCoordinateCorrected, summarys.c_str());
}

int32_t DragManager::StartDrag(const DragData &dragData, SessionPtr sess)
{
    CALL_INFO_TRACE;
    PrintDragData(dragData);
    if (dragState_ == DragState::START) {
        FI_HILOGE("Drag instance already exists, no need to start drag again");
        return RET_ERR;
    }
    dragOutSession_ = sess;
    if (InitDataManager(dragData) != RET_OK) {
        FI_HILOGE("Failed to init data manager");
        return RET_ERR;
    }
    if (OnStartDrag() != RET_OK) {
        DragDFX::WriteStartDrag(dragState_, OHOS::HiviewDFX::HiSysEvent::EventType::FAULT);
        FI_HILOGE("Failed to execute OnStartDrag");
        return RET_ERR;
    }
    dragState_ = DragState::START;
    stateNotify_.StateChangedNotify(DragState::START);
    StateChangedNotify(DragState::START);
    return RET_OK;
}

int32_t DragManager::StopDrag(const DragDropResult &dropResult)
{
    CALL_INFO_TRACE;
    FI_HILOGI("windowId:%{public}d", dropResult.windowId);
    if (dragState_ == DragState::STOP) {
        FI_HILOGE("No drag instance running, can not stop drag");
        return RET_ERR;
    }
    if ((dropResult.result != DragResult::DRAG_EXCEPTION) && (context_ != nullptr) && (timerId_ >= 0)) {
        context_->GetTimerManager().RemoveTimer(timerId_);
        timerId_ = -1;
    }
    int32_t ret = RET_OK;
    if (OnStopDrag(dropResult.result, dropResult.hasCustomAnimation) != RET_OK) {
        DragDFX::WriteStopDrag(dragState_, dropResult, OHOS::HiviewDFX::HiSysEvent::EventType::FAULT);
        FI_HILOGE("On stop drag failed");
        ret = RET_ERR;
    }
    dragState_ = DragState::STOP;
    if (dropResult.result == DragResult::DRAG_SUCCESS && dropResult.windowId > 0) {
        Rosen::WMError result = Rosen::WindowManager::GetInstance().RaiseWindowToTop(dropResult.windowId);
        if (result != Rosen::WMError::WM_OK) {
            FI_HILOGE("Raise window to top failed, windowId: %{public}d", dropResult.windowId);
        }
    }
    stateNotify_.StateChangedNotify(DragState::STOP);
    if (NotifyDragResult(dropResult.result) != RET_OK) {
        FI_HILOGE("Notify drag result failed");
    }
    DRAG_DATA_MGR.ResetDragData();
    dragResult_ = static_cast<DragResult>(dropResult.result);
    StateChangedNotify(DragState::STOP);
    dragDrawing_.SetTextEditorAreaFlag(false);
    return ret;
}

int32_t DragManager::GetDragTargetPid() const
{
    CALL_INFO_TRACE;
    return DRAG_DATA_MGR.GetTargetPid();
}

int32_t DragManager::GetUdKey(std::string &udKey) const
{
    CALL_INFO_TRACE;
    DragData dragData = DRAG_DATA_MGR.GetDragData();
    if (dragData.udKey.empty()) {
        FI_HILOGE("Target udKey is empty");
        return RET_ERR;
    }
    udKey = dragData.udKey;
    return RET_OK;
}

int32_t DragManager::UpdateDragStyle(DragCursorStyle style, int32_t targetPid, int32_t targetTid)
{
    if (dragState_ != DragState::START) {
        FI_HILOGE("No drag instance running, can not update drag style");
        return RET_ERR;
    }
    if ((style < DragCursorStyle::DEFAULT) || (style > DragCursorStyle::MOVE)) {
        DragDFX::WriteUpdateDragStyle(style, OHOS::HiviewDFX::HiSysEvent::EventType::FAULT);
        FI_HILOGE("Invalid style:%{public}d", style);
        return RET_ERR;
    }
    DRAG_DATA_MGR.SetTargetPid(targetPid);
    DRAG_DATA_MGR.SetTargetTid(targetTid);
    if (style == DRAG_DATA_MGR.GetDragStyle()) {
        FI_HILOGD("Not need update drag style");
        return RET_OK;
    }
    FI_HILOGI("Update drag style successfully");
    DRAG_DATA_MGR.SetDragStyle(style);
    DragCursorStyle updateStyle = DragCursorStyle::DEFAULT;
    if ((dragAction_ == DragAction::COPY) && (style == DragCursorStyle::MOVE)) {
        updateStyle = DragCursorStyle::COPY;
    } else {
        updateStyle = style;
    }
    stateNotify_.StyleChangedNotify(updateStyle);
    int32_t ret = dragDrawing_.UpdateDragStyle(updateStyle);
    if (ret != RET_OK) {
        DragDFX::WriteUpdateDragStyle(style, OHOS::HiviewDFX::HiSysEvent::EventType::FAULT);
    }
    return ret;
}

int32_t DragManager::UpdateShadowPic(const ShadowInfo &shadowInfo)
{
    CALL_INFO_TRACE;
    if (dragState_ != DragState::START) {
        FI_HILOGE("No drag instance running, can not update shadow picture");
        return RET_ERR;
    }
    DRAG_DATA_MGR.SetShadowInfos({ shadowInfo });
    return dragDrawing_.UpdateShadowPic(shadowInfo);
}

int32_t DragManager::GetDragData(DragData &dragData)
{
    CALL_INFO_TRACE;
    if (dragState_ != DragState::START) {
        FI_HILOGE("No drag instance running, can not get dragData");
        return RET_ERR;
    }
    dragData = DRAG_DATA_MGR.GetDragData();
    return RET_OK;
}

int32_t DragManager::GetDragState(DragState &dragState)
{
    CALL_INFO_TRACE;
    dragState = GetDragState();
    if (dragState == DragState::ERROR) {
        FI_HILOGE("dragState_ is error");
        return RET_ERR;
    }
    return RET_OK;
}

int32_t DragManager::NotifyDragResult(DragResult result)
{
    CALL_INFO_TRACE;
    DragData dragData = DRAG_DATA_MGR.GetDragData();
    int32_t targetPid = GetDragTargetPid();
    NetPacket pkt(MessageId::DRAG_NOTIFY_RESULT);
    if ((result < DragResult::DRAG_SUCCESS) || (result > DragResult::DRAG_EXCEPTION)) {
        DragDFX::WriteNotifyDragResult(result, OHOS::HiviewDFX::HiSysEvent::EventType::FAULT);
        FI_HILOGE("The invalid result:%{public}d", static_cast<int32_t>(result));
        return RET_ERR;
    }
    pkt << dragData.displayX << dragData.displayY << static_cast<int32_t>(result) << targetPid;
    if (pkt.ChkRWError()) {
        FI_HILOGE("Failed to packet write data");
        return RET_ERR;
    }
    CHKPR(dragOutSession_, RET_ERR);
    if (!dragOutSession_->SendMsg(pkt)) {
        FI_HILOGE("Failed to send message");
        return MSG_SEND_FAIL;
    }
    DragDFX::WriteNotifyDragResult(result, OHOS::HiviewDFX::HiSysEvent::EventType::BEHAVIOR);
    return RET_OK;
}

int32_t DragManager::NotifyHideIcon()
{
    CALL_DEBUG_ENTER;
    NetPacket pkt(MessageId::DRAG_NOTIFY_HIDE_ICON);
    if (pkt.ChkRWError()) {
        FI_HILOGE("Packet write data failed");
        return RET_ERR;
    }
    CHKPR(dragOutSession_, RET_ERR);
    if (!dragOutSession_->SendMsg(pkt)) {
        FI_HILOGE("Send message failed");
        return MSG_SEND_FAIL;
    }
    return RET_OK;
}

void DragManager::DragCallback(std::shared_ptr<MMI::PointerEvent> pointerEvent)
{
    CHKPV(pointerEvent);
    int32_t pointerAction = pointerEvent->GetPointerAction();
    if (pointerAction == MMI::PointerEvent::POINTER_ACTION_PULL_MOVE) {
        OnDragMove(pointerEvent);
        return;
    }
    if (pointerAction == MMI::PointerEvent::POINTER_ACTION_PULL_UP) {
        OnDragUp(pointerEvent);
        return;
    }
    FI_HILOGW("Unknow action, sourceType:%{public}d, pointerId:%{public}d, pointerAction:%{public}d",
        pointerEvent->GetSourceType(), pointerEvent->GetPointerId(), pointerAction);
}

void DragManager::OnDragMove(std::shared_ptr<MMI::PointerEvent> pointerEvent)
{
    CHKPV(pointerEvent);
    MMI::PointerEvent::PointerItem pointerItem;
    pointerEvent->GetPointerItem(pointerEvent->GetPointerId(), pointerItem);
    FI_HILOGD("SourceType:%{public}d, pointerId:%{public}d, displayX:%{public}d, displayY:%{public}d",
        pointerEvent->GetSourceType(), pointerEvent->GetPointerId(),
        pointerItem.GetDisplayX(), pointerItem.GetDisplayY());
    dragDrawing_.Draw(pointerEvent->GetTargetDisplayId(), pointerItem.GetDisplayX(), pointerItem.GetDisplayY());
}

void DragManager::SendDragData(int32_t targetTid, const std::string &udKey)
{
    CALL_INFO_TRACE;
    UDMF::QueryOption option;
    option.key = udKey;
    UDMF::Privilege privilege;
    privilege.tokenId = static_cast<uint32_t>(targetTid);
    int32_t ret = UDMF::UdmfClient::GetInstance().AddPrivilege(option, privilege);
    if (ret != RET_OK) {
        FI_HILOGE("Failed to send pid to Udmf client");
    }
}

void DragManager::OnDragUp(std::shared_ptr<MMI::PointerEvent> pointerEvent)
{
    CALL_INFO_TRACE;
    CHKPV(pointerEvent);
    DragData dragData = DRAG_DATA_MGR.GetDragData();
    if (dragData.sourceType == MMI::PointerEvent::SOURCE_TYPE_MOUSE) {
        dragDrawing_.EraseMouseIcon();
        FI_HILOGI("Set the pointer cursor visible");
        MMI::InputManager::GetInstance()->SetPointerVisible(true);
    }
    CHKPV(context_);
    int32_t repeatCount = 1;
    timerId_ = context_->GetTimerManager().AddTimer(TIMEOUT_MS, repeatCount, [this]() {
        DragDropResult dropResult { DragResult::DRAG_EXCEPTION, false, -1 };
        FI_HILOGW("Timeout, automatically stop dragging");
        this->StopDrag(dropResult);
    });
    CHKPV(notifyPUllUpCallback_);
    notifyPUllUpCallback_();
}

#ifdef OHOS_DRAG_ENABLE_INTERCEPTOR
void DragManager::InterceptorConsumer::OnInputEvent(std::shared_ptr<MMI::KeyEvent> keyEvent) const
{
}

void DragManager::InterceptorConsumer::OnInputEvent(std::shared_ptr<MMI::PointerEvent> pointerEvent) const
{
    CHKPV(pointerEvent);
    if (g_startFilterTime > 0) {
        auto actionTime = pointerEvent->GetActionTime();
        if (g_startFilterTime >= actionTime
            && pointerEvent->GetPointerAction() == MMI::PointerEvent::POINTER_ACTION_PULL_MOVE) {
            FI_HILOGW("Invalid event");
            return;
        } else {
            g_startFilterTime = -1;
        }
    }
    CHKPV(pointerEventCallback_);
    CHKPV(context_);
    pointerEventCallback_(pointerEvent);
    pointerEvent->AddFlag(MMI::InputEvent::EVENT_FLAG_NO_INTERCEPT);
    MMI::InputManager::GetInstance()->SimulateInputEvent(pointerEvent);
    if (pointerEvent->GetPointerAction() == MMI::PointerEvent::POINTER_ACTION_PULL_UP) {
        FI_HILOGI("Pointer button is released, appened extra data");
        MMI::InputManager::GetInstance()->AppendExtraData(DragManager::CreateExtraData(false));
    }
}

void DragManager::InterceptorConsumer::OnInputEvent(std::shared_ptr<MMI::AxisEvent> axisEvent) const
{
}
#endif // OHOS_DRAG_ENABLE_INTERCEPTOR

#ifdef OHOS_DRAG_ENABLE_MONITOR
void DragManager::MonitorConsumer::OnInputEvent(std::shared_ptr<MMI::KeyEvent> keyEvent) const
{
    CALL_DEBUG_ENTER;
}

void DragManager::MonitorConsumer::OnInputEvent(std::shared_ptr<MMI::PointerEvent> pointerEvent) const
{
    CALL_DEBUG_ENTER;
    CHKPV(pointerEvent);
    CHKPV(pointerEventCallback_);
    pointerEventCallback_(pointerEvent);
    if (pointerEvent->GetPointerAction() == MMI::PointerEvent::POINTER_ACTION_PULL_UP) {
        FI_HILOGI("Pointer button is released, appened extra data");
        MMI::InputManager::GetInstance()->AppendExtraData(DragManager::CreateExtraData(false));
    }
}

void DragManager::MonitorConsumer::OnInputEvent(std::shared_ptr<MMI::AxisEvent> axisEvent) const
{
    CALL_DEBUG_ENTER;
}
#endif // OHOS_DRAG_ENABLE_MONITOR

void DragManager::Dump(int32_t fd) const
{
    DragCursorStyle style = DRAG_DATA_MGR.GetDragStyle();
    int32_t targetTid = DRAG_DATA_MGR.GetTargetTid();
    dprintf(fd, "Drag information:\n");
#ifdef OHOS_DRAG_ENABLE_INTERCEPTOR
    dprintf(fd,
            "dragState:%s | dragResult:%s | interceptorId:%d | dragTargetPid:%d | dragTargetTid:%d | "
            "cursorStyle:%s | isWindowVisble:%s\n", GetDragState(dragState_).c_str(),
            GetDragResult(dragResult_).c_str(), pointerEventInterceptorId_, GetDragTargetPid(), targetTid,
            GetDragCursorStyle(style).c_str(), DRAG_DATA_MGR.GetDragWindowVisible() ? "true" : "false");
#endif // OHOS_DRAG_ENABLE_INTERCEPTOR
#ifdef OHOS_DRAG_ENABLE_MONITOR
    dprintf(fd,
            "dragState:%s | dragResult:%s | monitorId:%d | dragTargetPid:%d | dragTargetTid:%d | "
            "cursorStyle:%s | isWindowVisble:%s\n", GetDragState(dragState_).c_str(),
            GetDragResult(dragResult_).c_str(), pointerEventMonitorId_, GetDragTargetPid(), targetTid,
            GetDragCursorStyle(style).c_str(), DRAG_DATA_MGR.GetDragWindowVisible() ? "true" : "false");
#endif // OHOS_DRAG_ENABLE_MONITOR
    DragData dragData = DRAG_DATA_MGR.GetDragData();
    std::string udKey;
    if (RET_ERR == GetUdKey(udKey)) {
        FI_HILOGE("Target udKey is empty");
        udKey = "";
    }
    for (const auto& shadowInfo : dragData.shadowInfos) {
        dprintf(fd, "dragData = {\n""\tshadowInfoX:%d\n\tshadowInfoY\n", shadowInfo.x, shadowInfo.y);
    }
    dprintf(fd, "dragData = {\n"
            "\tudKey:%s\n\tfilterInfo:%s\n\textraInfo:%s\n\tsourceType:%d"
            "\tdragNum:%d\n\tpointerId:%d\n\tdisplayX:%d\n\tdisplayY:%d\n""\tdisplayId:%d\n\thasCanceledAnimation:%s\n",
            GetAnonyString(dragData.udKey).c_str(), dragData.filterInfo.c_str(), dragData.extraInfo.c_str(), dragData.sourceType,
            dragData.dragNum, dragData.pointerId, dragData.displayX, dragData.displayY, dragData.displayId,
            dragData.hasCanceledAnimation ? "true" : "false");
    if (dragState_ != DragState::STOP) {
        for (const auto& shadowInfo : dragData.shadowInfos) {
            CHKPV(shadowInfo.pixelMap);
            dprintf(fd, "\tpixelMapWidth:%d\n\tpixelMapHeight:%d\n", shadowInfo.pixelMap->GetWidth(),
                shadowInfo.pixelMap->GetHeight());
        }
    }
    dprintf(fd, "}\n");
}

std::string DragManager::GetDragState(DragState value) const
{
    std::string state = "unknown";
    const std::map<DragState, std::string> dragStates = {
        { DragState::START, "start" },
        { DragState::STOP, "stop" },
        { DragState::CANCEL, "cancel" },
        { DragState::ERROR, "error" }
    };
    auto iter = dragStates.find(value);
    if (iter != dragStates.end()) {
        state = iter->second;
    }
    return state;
}

std::string DragManager::GetDragResult(DragResult value) const
{
    std::string result = "unknown";
    const std::map<DragResult, std::string> dragResults = {
        { DragResult::DRAG_SUCCESS, "success" },
        { DragResult::DRAG_FAIL, "fail" },
        { DragResult::DRAG_CANCEL, "cancel" },
        { DragResult::DRAG_EXCEPTION, "abnormal" }
    };
    auto iter = dragResults.find(value);
    if (iter != dragResults.end()) {
        result = iter->second;
    }
    return result;
}

std::string DragManager::GetDragCursorStyle(DragCursorStyle value) const
{
    std::string style = "unknown";
    const std::map<DragCursorStyle, std::string> cursorStyles = {
        { DragCursorStyle::COPY, "copy" },
        { DragCursorStyle::DEFAULT, "default" },
        { DragCursorStyle::FORBIDDEN, "forbidden" },
        { DragCursorStyle::MOVE, "move" }
    };
    auto iter = cursorStyles.find(value);
    if (iter != cursorStyles.end()) {
        style = iter->second;
    }
    return style;
}

MMI::ExtraData DragManager::CreateExtraData(bool appended)
{
    DragData dragData = DRAG_DATA_MGR.GetDragData();
    MMI::ExtraData extraData;
    extraData.buffer = dragData.buffer;
    extraData.sourceType = dragData.sourceType;
    extraData.pointerId = dragData.pointerId;
    extraData.appended = appended;
    FI_HILOGD("sourceType:%{public}d, pointerId:%{public}d", extraData.sourceType, extraData.pointerId);
    return extraData;
}

int32_t DragManager::InitDataManager(const DragData &dragData) const
{
    CALL_INFO_TRACE;
    DRAG_DATA_MGR.Init(dragData);
    return RET_OK;
}

int32_t DragManager::AddDragEventHandler(int32_t sourceType)
{
    CALL_INFO_TRACE;
    uint32_t deviceTags = 0;
#ifdef OHOS_DRAG_ENABLE_INTERCEPTOR
    if (sourceType == MMI::PointerEvent::SOURCE_TYPE_MOUSE) {
        deviceTags = MMI::CapabilityToTags(MMI::INPUT_DEV_CAP_POINTER);
    } else if (sourceType == MMI::PointerEvent::SOURCE_TYPE_TOUCHSCREEN) {
        deviceTags = MMI::CapabilityToTags(MMI::INPUT_DEV_CAP_TOUCH) |
            MMI::CapabilityToTags(MMI::INPUT_DEV_CAP_TABLET_TOOL);
    } else {
        FI_HILOGW("Drag is not supported for this device type:%{public}d", sourceType);
        return RET_ERR;
    }
#endif // OHOS_DRAG_ENABLE_INTERCEPTOR
    if (AddPointerEventHandler(deviceTags) != RET_OK) {
        FI_HILOGE("Failed to add pointer event handler");
        return RET_ERR;
    }
    if (AddKeyEventMointor() != RET_OK) {
        FI_HILOGE("Failed to add key event handler");
        return RET_ERR;
    }
    return RET_OK;
}

int32_t DragManager::AddPointerEventHandler(uint32_t deviceTags)
{
    CALL_INFO_TRACE;
#ifdef OHOS_DRAG_ENABLE_MONITOR
    auto monitor = std::make_shared<MonitorConsumer>(std::bind(&DragManager::DragCallback, this,
        std::placeholders::_1));
    pointerEventMonitorId_ = MMI::InputManager::GetInstance()->AddMonitor(monitor);
    if (pointerEventMonitorId_ <= 0) {
        FI_HILOGE("Failed to add pointer event monitor");
        return RET_ERR;
    }
#else
    auto callback = std::bind(&DragManager::DragCallback, this, std::placeholders::_1);
    auto interceptor = std::make_shared<InterceptorConsumer>(context_, callback);
    pointerEventInterceptorId_ = MMI::InputManager::GetInstance()->AddInterceptor(
        interceptor, DRAG_PRIORITY, deviceTags);
    if (pointerEventInterceptorId_ <= 0) {
        FI_HILOGE("Failed to add pointer event interceptor");
        return RET_ERR;
    }
#endif // OHOS_DRAG_ENABLE_MONITOR
    FI_HILOGI("Add drag poniter event handle successfully");
    return RET_OK;
}

int32_t DragManager::AddKeyEventMointor()
{
    CALL_INFO_TRACE;
    keyEventMonitorId_ = MMI::InputManager::GetInstance()->AddMonitor(
        std::bind(&DragManager::DragKeyEventCallback, this, std::placeholders::_1));
    if (keyEventMonitorId_ <= 0) {
        FI_HILOGE("Failed to add key event monitor");
        return RET_ERR;
    }
    FI_HILOGI("Add drag key event monitor successfully");
    return RET_OK;
}

int32_t DragManager::RemovePointerEventHandler()
{
    CALL_INFO_TRACE;
#ifdef OHOS_DRAG_ENABLE_MONITOR
    if (pointerEventMonitorId_ <= 0) {
        FI_HILOGE("Invalid pointer event monitor id:%{public}d", pointerEventMonitorId_);
        return RET_ERR;
    }
    MMI::InputManager::GetInstance()->RemoveMonitor(pointerEventMonitorId_);
    pointerEventMonitorId_ = -1;
#else
    if (pointerEventInterceptorId_ <= 0) {
        FI_HILOGE("Invalid pointer event interceptor id:%{public}d", pointerEventInterceptorId_);
    }
    MMI::InputManager::GetInstance()->RemoveInterceptor(pointerEventInterceptorId_);
    pointerEventInterceptorId_ = -1;
#endif // OHOS_DRAG_ENABLE_MONITOR
    FI_HILOGI("Remove drag pointer event handler successfully");
    return RET_OK;
}

int32_t DragManager::RemoveKeyEventMointor()
{
    CALL_INFO_TRACE;
    if (keyEventMonitorId_ <= 0) {
        FI_HILOGE("Invalid key event monitor id:%{public}d", keyEventMonitorId_);
        return RET_ERR;
    }
    MMI::InputManager::GetInstance()->RemoveMonitor(keyEventMonitorId_);
    keyEventMonitorId_ = -1;
    FI_HILOGI("Remove drag key event handle successfully");
    return RET_OK;
}

int32_t DragManager::OnStartDrag()
{
    CALL_INFO_TRACE;
    auto extraData = CreateExtraData(true);
    DragData dragData = DRAG_DATA_MGR.GetDragData();
    if (Rosen::DisplayManager::GetInstance().IsFoldable()) {
        Rosen::FoldDisplayMode foldMode = Rosen::DisplayManager::GetInstance().GetFoldDisplayMode();
        if (foldMode == Rosen::FoldDisplayMode::MAIN) {
            dragDrawing_.SetScreenId(FOLD_SCREEN_ID);
        }
    }
    int32_t ret = dragDrawing_.Init(dragData);
    if (ret == INIT_FAIL) {
        FI_HILOGE("Init drag drawing failed");
        dragDrawing_.DestroyDragWindow();
        return RET_ERR;
    }
    if (ret == INIT_CANCEL) {
        FI_HILOGE("Init drag drawing cancel, drag animation is running");
        return RET_ERR;
    }
    dragDrawing_.Draw(dragData.displayId, dragData.displayX, dragData.displayY);
    FI_HILOGI("Start drag, appened extra data");
    MMI::InputManager::GetInstance()->AppendExtraData(extraData);
    ret = AddDragEventHandler(dragData.sourceType);
    if (ret != RET_OK) {
        FI_HILOGE("Failed to add drag event handler");
        dragDrawing_.DestroyDragWindow();
        return RET_ERR;
    }
    dragAction_.store(DragAction::MOVE);
    return RET_OK;
}

int32_t DragManager::OnStopDrag(DragResult result, bool hasCustomAnimation)
{
    FI_HILOGI("Add custom animation:%{public}s", hasCustomAnimation ? "true" : "false");
    if (RemovePointerEventHandler() != RET_OK) {
        FI_HILOGE("Failed to remove pointer event handler");
        return RET_ERR;
    }
    if (RemoveKeyEventMointor() != RET_OK) {
        FI_HILOGE("Failed to remove key event handler");
        return RET_ERR;
    }
    dragAction_.store(DragAction::MOVE);
    DragData dragData = DRAG_DATA_MGR.GetDragData();
    if ((dragData.sourceType == MMI::PointerEvent::SOURCE_TYPE_MOUSE) && !DRAG_DATA_MGR.IsMotionDrag()) {
        dragDrawing_.EraseMouseIcon();
        FI_HILOGI("Set the pointer cursor visible");
        MMI::InputManager::GetInstance()->SetPointerVisible(true);
    }
    FI_HILOGI("Stop drag, appened extra data");
    MMI::InputManager::GetInstance()->AppendExtraData(DragManager::CreateExtraData(false));
    return HandleDragResult(result, hasCustomAnimation);
}

int32_t DragManager::OnSetDragWindowVisible(bool visible)
{
    FI_HILOGI("Set drag window visibleion:%{public}s", visible ? "true" : "false");
    if (dragState_ == DragState::MOTION_DRAGGING) {
        FI_HILOGW("Currently in motion dragging");
        return RET_OK;
    }
    if (dragState_ == DragState::STOP) {
        FI_HILOGW("No drag instance running, can not set drag window visible");
        return RET_ERR;
    }
    DRAG_DATA_MGR.SetDragWindowVisible(visible);
    dragDrawing_.UpdateDragWindowState(visible);
    DragData dragData = DRAG_DATA_MGR.GetDragData();
    if (dragData.sourceType == MMI::PointerEvent::SOURCE_TYPE_MOUSE && visible) {
        FI_HILOGI("Set the pointer cursor invisible");
        MMI::InputManager::GetInstance()->SetPointerVisible(false);
    }
    DragDFX::WriteDragWindowVisible(dragState_, visible, OHOS::HiviewDFX::HiSysEvent::EventType::BEHAVIOR);
    return RET_OK;
}

int32_t DragManager::OnGetShadowOffset(int32_t& offsetX, int32_t& offsetY, int32_t& width, int32_t& height)
{
    return DRAG_DATA_MGR.GetShadowOffset(offsetX, offsetY, width, height);
}

void DragManager::RegisterStateChange(std::function<void(DragState)> callback)
{
    CALL_INFO_TRACE;
    CHKPV(callback);
    stateChangedCallback_ = callback;
}

void DragManager::RegisterNotifyPullUp(std::function<void(void)> callback)
{
    CALL_INFO_TRACE;
    CHKPV(callback);
    notifyPUllUpCallback_ = callback;
}

void DragManager::StateChangedNotify(DragState state)
{
    CALL_INFO_TRACE;
    if ((stateChangedCallback_ != nullptr) && (!DRAG_DATA_MGR.IsMotionDrag())) {
        stateChangedCallback_(state);
    }
}

MMI::ExtraData DragManager::GetExtraData(bool appended) const
{
    return CreateExtraData(appended);
}

DragState DragManager::GetDragState() const
{
    return dragState_;
}

void DragManager::GetAllowDragState(bool &isAllowDrag)
{
    CALL_DEBUG_ENTER;
    if (dragState_ != DragState::START) {
        FI_HILOGW("Currently state is \'%{public}d\' not in allowed dragState", static_cast<int32_t>(dragState_));
        return;
    }
    isAllowDrag = dragDrawing_.GetAllowDragState();
}

void DragManager::SetDragState(DragState state)
{
    dragState_ = state;
}

DragResult DragManager::GetDragResult() const
{
    return dragResult_;
}

int32_t DragManager::GetDragSummary(std::map<std::string, int64_t> &summarys)
{
    if (GetDragState() != DragState::START) {
        FI_HILOGE("No drag instance running");
        return RET_ERR;
    }
    DragData dragData;
    if (GetDragData(dragData) != RET_OK) {
        FI_HILOGE("GetDragData failed");
        return RET_ERR;
    }
    summarys = dragData.summarys;
    if (summarys.empty()) {
        FI_HILOGE("Summarys is empty");
        return RET_ERR;
    }
    return RET_OK;
}

int32_t DragManager::HandleDragResult(DragResult result, bool hasCustomAnimation)
{
    CALL_INFO_TRACE;
    switch (result) {
        case DragResult::DRAG_SUCCESS: {
            if (!hasCustomAnimation) {
                dragDrawing_.OnDragSuccess();
            } else {
                dragDrawing_.DestroyDragWindow();
                dragDrawing_.UpdateDrawingState();
            }
            break;
        }
        case DragResult::DRAG_FAIL:
        case DragResult::DRAG_CANCEL: {
            if (!hasCustomAnimation) {
                dragDrawing_.OnDragFail();
            } else {
                dragDrawing_.DestroyDragWindow();
                dragDrawing_.UpdateDrawingState();
            }
            break;
        }
        case DragResult::DRAG_EXCEPTION: {
            dragDrawing_.DestroyDragWindow();
            dragDrawing_.UpdateDrawingState();
            break;
        }
        default: {
            FI_HILOGW("Unsupported DragResult type, DragResult:%{public}d", result);
            break;
        }
    }
    return RET_OK;
}

void DragManager::SetPointerEventFilterTime(int64_t filterTime)
{
    CALL_DEBUG_ENTER;
    g_startFilterTime = filterTime;
}

void DragManager::MoveTo(int32_t x, int32_t y)
{
    if (dragState_ != DragState::START && dragState_ != DragState::MOTION_DRAGGING) {
        FI_HILOGE("Drag instance not running");
        return;
    }
    DragData dragData = DRAG_DATA_MGR.GetDragData();
    FI_HILOGI("displayId:%{public}d, x:%{public}d, y:%{public}d", dragData.displayId, x, y);
    dragDrawing_.Draw(dragData.displayId, x, y);
}

int32_t DragManager::UpdatePreviewStyle(const PreviewStyle &previewStyle)
{
    if (dragState_ != DragState::START && dragState_ != DragState::MOTION_DRAGGING) {
        FI_HILOGE("Drag instance not running");
        return RET_ERR;
    }
    if (previewStyle == DRAG_DATA_MGR.GetPreviewStyle()) {
        FI_HILOGD("Not need to update previewStyle");
        return RET_OK;
    }
    DRAG_DATA_MGR.SetPreviewStyle(previewStyle);
    FI_HILOGI("Update previewStyle successfully");
    return dragDrawing_.UpdatePreviewStyle(previewStyle);
}

int32_t DragManager::UpdatePreviewStyleWithAnimation(const PreviewStyle &previewStyle,
    const PreviewAnimation &animation)
{
    if (dragState_ != DragState::START && dragState_ != DragState::MOTION_DRAGGING) {
        FI_HILOGE("Drag instance not running");
        return RET_ERR;
    }
    if (previewStyle == DRAG_DATA_MGR.GetPreviewStyle()) {
        FI_HILOGD("Not need to update previewStyle");
        return RET_OK;
    }
    DRAG_DATA_MGR.SetPreviewStyle(previewStyle);
    FI_HILOGI("Update previewStyle successfully");
    return dragDrawing_.UpdatePreviewStyleWithAnimation(previewStyle, animation);
}

void DragManager::DragKeyEventCallback(std::shared_ptr<MMI::KeyEvent> keyEvent)
{
    CHKPV(keyEvent);
    auto keyItems = keyEvent->GetKeyItems();
    auto iter = std::find_if(keyItems.begin(), keyItems.end(),
        [] (std::optional<MMI::KeyEvent::KeyItem> keyItem) {
            return ((keyItem->GetKeyCode() == MMI::KeyEvent::KEYCODE_CTRL_LEFT) ||
                    (keyItem->GetKeyCode() == MMI::KeyEvent::KEYCODE_CTRL_RIGHT));
        });
    if (iter == keyItems.end()) {
        dragAction_.store(DragAction::MOVE);
        return;
    }
    if ((DRAG_DATA_MGR.GetDragStyle() == DragCursorStyle::DEFAULT) ||
        (DRAG_DATA_MGR.GetDragStyle() == DragCursorStyle::FORBIDDEN)) {
        dragAction_.store(DragAction::MOVE);
        return;
    }
    if (!iter->IsPressed()) {
        CtrlKeyStyleChangedNotify(DRAG_DATA_MGR.GetDragStyle(), DragAction::MOVE);
        HandleCtrlKeyEvent(DRAG_DATA_MGR.GetDragStyle(), DragAction::MOVE);
        dragAction_.store(DragAction::MOVE);
        return;
    }
    if (iter->IsPressed()) {
        if (DRAG_DATA_MGR.GetDragStyle() == DragCursorStyle::COPY) {
            FI_HILOGD("Not need update drag style");
            return;
        }
        CtrlKeyStyleChangedNotify(DragCursorStyle::COPY, DragAction::COPY);
        HandleCtrlKeyEvent(DragCursorStyle::COPY, DragAction::COPY);
        dragAction_.store(DragAction::COPY);
    }
}

void DragManager::HandleCtrlKeyEvent(DragCursorStyle style, DragAction action)
{
    CALL_DEBUG_ENTER;
    if (action == dragAction_.load()) {
        FI_HILOGD("Not need update drag style");
        return;
    }
    CHKPV(context_);
    int32_t ret = context_->GetDelegateTasks().PostAsyncTask(
        std::bind(&DragDrawing::UpdateDragStyle, &dragDrawing_, style));
    if (ret != RET_OK) {
        FI_HILOGE("Post async task failed");
    }
}

void DragManager::CtrlKeyStyleChangedNotify(DragCursorStyle style, DragAction action)
{
    CALL_DEBUG_ENTER;
    if (action == dragAction_.load()) {
        FI_HILOGD("Has notified");
        return;
    }
    CHKPV(context_);
    int32_t ret = context_->GetDelegateTasks().PostAsyncTask(
        std::bind(&StateChangeNotify::StyleChangedNotify, &stateNotify_, style));
    if (ret != RET_OK) {
        FI_HILOGE("Post async task failed");
    }
}

int32_t DragManager::GetDragAction(DragAction& dragAction) const
{
    CALL_DEBUG_ENTER;
    if (dragState_ != DragState::START) {
        FI_HILOGE("No drag instance running, can not get drag action");
        return RET_ERR;
    }
    dragAction = dragAction_.load();
    return RET_OK;
}

int32_t DragManager::EnterTextEditorArea(bool enable)
{
    CALL_DEBUG_ENTER;
    return dragDrawing_.EnterTextEditorArea(enable);
}

int32_t DragManager::GetExtraInfo(std::string &extraInfo) const
{
    CALL_DEBUG_ENTER;
    DragData dragData = DRAG_DATA_MGR.GetDragData();
    if (dragData.extraInfo.empty()) {
        FI_HILOGE("The extraInfo is empty");
        return RET_ERR;
    }
    extraInfo = dragData.extraInfo;
    return RET_OK;
}

int32_t DragManager::AddPrivilege(int32_t tokenId)
{
    CALL_DEBUG_ENTER;
    if (dragState_ != DragState::START && dragState_ != DragState::MOTION_DRAGGING) {
        FI_HILOGE("Drag instance not running");
        return RET_ERR;
    }
    DragData dragData = DRAG_DATA_MGR.GetDragData();
    FI_HILOGD("Target window drag tid:%{public}d", tokenId);
    SendDragData(tokenId, dragData.udKey);
    return RET_OK;
}
} // namespace DeviceStatus
} // namespace Msdp
} // namespace OHOS
