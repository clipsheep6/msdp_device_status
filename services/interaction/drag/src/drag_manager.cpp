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

#include "extra_data.h"
#include "hitrace_meter.h"
#include "input_manager.h"
#include "pixel_map.h"
#include "pointer_style.h"

#include "devicestatus_define.h"
#include "drag_data.h"
#include "drag_data_adapter.h"
#include "fi_log.h"
#include "proto.h"

namespace OHOS {
namespace Msdp {
namespace DeviceStatus {
namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, MSDP_DOMAIN_ID, "DragManager" };
} // namespace

void DragManager::OnSessionLost(SessionPtr session)
{
    CALL_DEBUG_ENTER;
    if (RemoveListener(session) != RET_OK) {
        FI_HILOGE("Failed to clear client listener");
    }
}

int32_t DragManager::AddListener(SessionPtr session)
{
    CALL_DEBUG_ENTER;
    CHKPR(session, RET_ERR);
    auto info = std::make_shared<StateChangeNotify::MessageInfo>();
    info->session = session;
    info->msgId = MessageId::DRAG_STATE_LISTENER;
    stateNotify_.AddNotifyMsg(info);
    return RET_OK;
}

int32_t DragManager::RemoveListener(SessionPtr session)
{
    CALL_DEBUG_ENTER;
    CHKPR(session, RET_ERR);
    auto info = std::make_shared<StateChangeNotify::MessageInfo>();
    info->session = session;
    stateNotify_.RemoveNotifyMsg(info);
    return RET_OK;
}

int32_t DragManager::StartDrag(const DragData &dragData, SessionPtr sess)
{
    CALL_DEBUG_ENTER;
    if (dragState_ == DragMessage::MSG_DRAG_STATE_START) {
        FI_HILOGE("Drag instance is running, can not start drag again");
        return RET_ERR;
    }
    CHKPR(sess, RET_ERR);
    dragOutSession_ = sess;
    if (InitDataAdapter(dragData) < 0) {
        FI_HILOGE("InitDataAdapter failed");
        return RET_ERR;
    }
    if (LaunchDrag() < 0) {
        FI_HILOGE("LaunchDrag failed");
        return RET_ERR;
    }
    INPUT_MANAGER->SetPointerVisible(false);
    dragState_ = DragMessage::MSG_DRAG_STATE_START;
    stateNotify_.StateChangedNotify(DragMessage::MSG_DRAG_STATE_START);
    return RET_OK;
}

int32_t DragManager::StopDrag(int32_t result)
{
    CALL_DEBUG_ENTER;
    if (dragState_ == DragMessage::MSG_DRAG_STATE_STOP) {
        FI_HILOGE("No drag instance running, can not stop drag");
        return RET_ERR;
    }
    INPUT_MANAGER->SetPointerVisible(true);
    if (RestoreDrag() < 0) {
        FI_HILOGE("RestoreDrag failed");
        return RET_ERR;
    }
    dragState_ = DragMessage::MSG_DRAG_STATE_STOP;
    stateNotify_.StateChangedNotify(DragMessage::MSG_DRAG_STATE_STOP);
    NotifyDragResult(result);
    return RET_OK;
}

int32_t DragManager::GetDragTargetPid() const
{
    return dragTargetPid_;
}

int32_t DragManager::NotifyDragResult(int32_t result)
{
    CALL_DEBUG_ENTER;
    NetPacket pkt(MessageId::DRAG_NOTIFY_RESULT);
    pkt << result;
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
    CALL_DEBUG_ENTER;
    CHKPV(pointerEvent);
    MMI::PointerEvent::PointerItem pointerItem;
    pointerEvent->GetPointerItem(pointerEvent->GetPointerId(), pointerItem);
    auto pointerAction = pointerEvent->GetPointerAction();
    if (pointerAction == MMI::PointerEvent::POINTER_ACTION_PULL_MOVE) {
        OnDragMove(pointerEvent);
    } else if (pointerAction == MMI::PointerEvent::POINTER_ACTION_PULL_UP) {
        OnDragUp(pointerEvent);
    } else {
        FI_HILOGW("Unknow pointerAction:%{public}d", pointerEvent->GetPointerAction());
    }
}

void DragManager::OnDragMove(std::shared_ptr<MMI::PointerEvent> pointerEvent)
{
    CALL_DEBUG_ENTER;
    CHKPV(pointerEvent);
    MMI::PointerEvent::PointerItem pointerItem;
    pointerEvent->GetPointerItem(pointerEvent->GetPointerId(), pointerItem);
}

void DragManager::OnDragUp(std::shared_ptr<MMI::PointerEvent> pointerEvent)
{
    CALL_DEBUG_ENTER;
    CHKPV(pointerEvent);
    FetchDragTargetPid(pointerEvent);
}

void DragManager::MonitorConsumer::OnInputEvent(std::shared_ptr<MMI::AxisEvent> axisEvent) const
{}
void DragManager::MonitorConsumer::OnInputEvent(std::shared_ptr<MMI::KeyEvent> keyEvent) const
{}
void DragManager::MonitorConsumer::OnInputEvent(std::shared_ptr<MMI::PointerEvent> pointerEvent) const
{
    CALL_DEBUG_ENTER;
    CHKPV(pointerEvent);
    CHKPV(callback_);
    callback_(pointerEvent);
}

OHOS::MMI::ExtraData DragManager::CreateExtraData(bool appended) const
{
    DragData dragData = DataAdapter.GetDragData();
    MMI::ExtraData extraData;
    extraData.buffer = dragData.buffer;
    extraData.sourceType = dragData.sourceType;
    extraData.appended = appended;
    return extraData;
}

int32_t DragManager::InitDataAdapter(const DragData &dragData) const
{
    MMI::PointerStyle pointerStyle;
    if (INPUT_MANAGER->GetPointerStyle(MMI::GLOBAL_WINDOW_ID, pointerStyle) != RET_OK) {
        FI_HILOGE("GetPointerStyle failed");
        return RET_ERR;
    }
    DataAdapter.Init(dragData, pointerStyle);
    return RET_OK;
}

int32_t DragManager::LaunchDrag()
{
    auto extraData = CreateExtraData(true);
    INPUT_MANAGER->AppendExtraData(extraData);
    auto callback = std::bind(&DragManager::DragCallback, this, std::placeholders::_1);
    monitorConsumer_ = std::make_shared<MonitorConsumer>(MonitorConsumer(callback));
    monitorId_ = INPUT_MANAGER->AddMonitor(monitorConsumer_);
    if (monitorId_ < 0) {
        FI_HILOGE("AddMonitor failed, monitorId_:%{public}d", monitorId_);
        return RET_ERR;
    }
    return RET_OK;
}

int32_t DragManager::RestoreDrag()
{
    auto extraData = CreateExtraData(false);
    INPUT_MANAGER->AppendExtraData(extraData);
    if ((monitorId_ > 0) && (monitorId_ < std::numeric_limits<int32_t>::max())) {
        INPUT_MANAGER->RemoveMonitor(monitorId_);
        monitorId_ = -1;
        monitorConsumer_ = nullptr;
        return RET_OK;
    }
    return RET_ERR;
}

void DragManager::FetchDragTargetPid(std::shared_ptr<MMI::PointerEvent> pointerEvent)
{
    MMI::PointerEvent::PointerItem pointerItem;
    pointerEvent->GetPointerItem(pointerEvent->GetPointerId(), pointerItem);
    dragTargetPid_ = INPUT_MANAGER->GetWindowPid(pointerItem.GetTargetWindowId());
}

} // namespace DeviceStatus
} // namespace Msdp
} // namespace OHOS
