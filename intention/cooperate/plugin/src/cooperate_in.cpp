/*
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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

#include "cooperate_in.h"
#include "cooperate_hisysevent.h"

#include "devicestatus_define.h"
#include "utility.h"

#undef LOG_TAG
#define LOG_TAG "CooperateIn"

namespace OHOS {
namespace Msdp {
namespace DeviceStatus {
namespace Cooperate {

CooperateIn::CooperateIn(IStateMachine &parent, IContext *env)
    : ICooperateState(parent), env_(env)
{
    initial_ = std::make_shared<Initial>(*this);
    Initial::BuildChains(initial_, *this);
    current_ = initial_;
}

CooperateIn::~CooperateIn()
{
    Initial::RemoveChains(initial_);
}

void CooperateIn::OnEvent(Context &context, const CooperateEvent &event)
{
    current_->OnEvent(context, event);
}

void CooperateIn::OnEnterState(Context &context)
{
    CALL_INFO_TRACE;
    int32_t ret = env_->GetInput().SetPointerVisibility(!context.NeedHideCursor(), PRIORITY);
    CooperateRadarInfo radarInfo {
        .funcName = __FUNCTION__,
        .bizState = static_cast<int32_t> (BizState::STATE_END),
        .bizScene = static_cast<int32_t> (BizCooperateScene::SCENE_PASSIVE),
        .hostName = "",
        .localNetId = "",
        .peerNetId = ""
    };
    if (ret != RET_OK) {
        radarInfo.bizStage = static_cast<int32_t> (BizCooperateStage::STAGE_PASSIVE_CURSOR_VISIBILITY);
        radarInfo.stageRes = static_cast<int32_t> (BizCooperateStageRes::RES_FAIL);
        radarInfo.errCode = static_cast<int32_t> (CooperateRadarErrCode::PASSIVE_CURSOR_VISIBILITY_FAILED);
        CooperateRadar::ReportCooperateRadarInfo(radarInfo);
    }
    radarInfo.bizStage = static_cast<int32_t> (BizCooperateStage::STAGE_PASSIVE_CURSOR_VISIBILITY);
    radarInfo.stageRes = static_cast<int32_t> (BizCooperateStageRes::RES_SUCCESS);
    radarInfo.errCode = static_cast<int32_t> (CooperateRadarErrCode::CALLING_COOPERATE_SUCCESS);
    CooperateRadar::ReportCooperateRadarInfo(radarInfo);
}

void CooperateIn::OnLeaveState(Context & context)
{
    CALL_INFO_TRACE;
    UpdateCooperateFlagEvent event {
        .mask = COOPERATE_FLAG_HIDE_CURSOR,
        .flag = COOPERATE_FLAG_HIDE_CURSOR,
    };
    context.UpdateCooperateFlag(event);
    CHKPV(env_);
    env_->GetInput().SetPointerVisibility(false);
}

std::set<int32_t> CooperateIn::Initial::filterPointerActions_ {
    MMI::PointerEvent::POINTER_ACTION_ENTER_WINDOW,
    MMI::PointerEvent::POINTER_ACTION_LEAVE_WINDOW,
    MMI::PointerEvent::POINTER_ACTION_PULL_IN_WINDOW,
    MMI::PointerEvent::POINTER_ACTION_PULL_OUT_WINDOW,
};

void CooperateIn::Initial::BuildChains(std::shared_ptr<Initial> self, CooperateIn &parent)
{
    auto s11 = std::make_shared<RelayConfirmation>(parent, self);
    self->relay_ = s11;
    s11->SetNext(self);
}

void CooperateIn::Initial::RemoveChains(std::shared_ptr<Initial> self)
{
    if (self->relay_ != nullptr) {
        self->relay_->SetNext(nullptr);
        self->relay_ = nullptr;
    }
}

CooperateIn::Initial::Initial(CooperateIn &parent)
    : ICooperateStep(parent, nullptr), parent_(parent)
{
    AddHandler(CooperateEventType::DISABLE, [this](Context &context, const CooperateEvent &event) {
        this->OnDisable(context, event);
    });
    AddHandler(CooperateEventType::START, [this](Context &context, const CooperateEvent &event) {
        this->OnStart(context, event);
    });
    AddHandler(CooperateEventType::STOP, [this](Context &context, const CooperateEvent &event) {
        this->OnStop(context, event);
    });
    AddHandler(CooperateEventType::APP_CLOSED, [this](Context &context, const CooperateEvent &event) {
        this->OnAppClosed(context, event);
    });
    AddHandler(CooperateEventType::INPUT_POINTER_EVENT, [this](Context &context, const CooperateEvent &event) {
        this->OnPointerEvent(context, event);
    });
    AddHandler(CooperateEventType::DDM_BOARD_OFFLINE, [this](Context &context, const CooperateEvent &event) {
        this->OnBoardOffline(context, event);
    });
    AddHandler(CooperateEventType::DDP_COOPERATE_SWITCH_CHANGED,
        [this](Context &context, const CooperateEvent &event) {
            this->OnSwitchChanged(context, event);
    });
    AddHandler(CooperateEventType::DSOFTBUS_SESSION_CLOSED,
        [this](Context &context, const CooperateEvent &event) {
            this->OnSoftbusSessionClosed(context, event);
    });
    AddHandler(CooperateEventType::DSOFTBUS_START_COOPERATE,
        [this](Context &context, const CooperateEvent &event) {
            this->OnRemoteStart(context, event);
    });
    AddHandler(CooperateEventType::DSOFTBUS_STOP_COOPERATE,
        [this](Context &context, const CooperateEvent &event) {
            this->OnRemoteStop(context, event);
    });
    AddHandler(CooperateEventType::UPDATE_COOPERATE_FLAG,
        [this](Context &context, const CooperateEvent &event) {
            this->OnUpdateCooperateFlag(context, event);
    });
    AddHandler(CooperateEventType::DSOFTBUS_INPUT_DEV_SYNC,
        [this](Context &context, const CooperateEvent &event) {
            this->OnRemoteInputDevice(context, event);
    });
    AddHandler(CooperateEventType::WITH_OPTIONS_START,
        [this](Context &context, const CooperateEvent &event) {
            this->OnStartWithOptions(context, event);
    });
    AddHandler(CooperateEventType::DSOFTBUS_COOPERATE_WITH_OPTIONS,
        [this](Context &context, const CooperateEvent &event) {
            this->OnRemoteStartWithOptions(context, event);
    });
}

void CooperateIn::Initial::OnDisable(Context &context, const CooperateEvent &event)
{
    FI_HILOGI("[disable cooperation] Stop cooperation");
    parent_.StopCooperate(context, event);
}

void CooperateIn::Initial::OnStart(Context &context, const CooperateEvent &event)
{
    CALL_INFO_TRACE;
    CHKPV(parent_.env_);
    StartCooperateEvent startEvent = std::get<StartCooperateEvent>(event.event);
    context.ResetPriv();

    if (parent_.env_->GetDragManager().GetDragState() == DragState::MOTION_DRAGGING) {
        FI_HILOGE("Not allow cooperate");
        NotAollowCooperateWhenMotionDragging result {
            .pid = startEvent.pid,
            .userData = startEvent.userData,
            .networkId = startEvent.remoteNetworkId,
            .success = false,
            .errCode = static_cast<int32_t>(CoordinationErrCode::NOT_AOLLOW_COOPERATE_WHEN_MOTION_DRAGGING)
        };
        context.eventMgr_.ErrorNotAollowCooperateWhenMotionDragging(result);
        return;
    }
    if (context.IsLocal(startEvent.remoteNetworkId)) {
        DSoftbusStartCooperateFinished result {
            .success = false,
            .errCode = static_cast<int32_t>(CoordinationErrCode::UNEXPECTED_START_CALL)
        };
        context.eventMgr_.StartCooperateFinish(result);
        return;
    }
    FI_HILOGI("[start] start cooperation(%{public}s, %{public}d)",
        Utility::Anonymize(startEvent.remoteNetworkId).c_str(), startEvent.startDeviceId);
    context.eventMgr_.StartCooperate(startEvent);

    if (context.IsPeer(startEvent.remoteNetworkId)) {
        OnComeBack(context, event);
    } else {
        OnRelay(context, event);
    }
}

void CooperateIn::Initial::OnComeBack(Context &context, const CooperateEvent &event)
{
    CALL_INFO_TRACE;
    context.inputEventBuilder_.Disable();
    FI_HILOGI("[come back] To \'%{public}s\'", Utility::Anonymize(context.Peer()).c_str());
    StartCooperateEvent startEvent = std::get<StartCooperateEvent>(event.event);
    DSoftbusComeBack notice {
        .originNetworkId = context.Local(),
        .success = true,
        .cursorPos = context.NormalizedCursorPosition(),
        .uid = startEvent.uid
    };
    context.OnStartCooperate(notice.extra);
    if (context.dsoftbus_.ComeBack(context.Peer(), notice) != RET_OK) {
        notice.success = false;
        notice.errCode = static_cast<int32_t>(CoordinationErrCode::SEND_PACKET_FAILED);
    }
    context.eventMgr_.StartCooperateFinish(notice);
    TransiteTo(context, CooperateState::COOPERATE_STATE_FREE);
    context.OnBack();
}

void CooperateIn::Initial::OnRelay(Context &context, const CooperateEvent &event)
{
    CALL_INFO_TRACE;
    StartCooperateEvent startEvent = std::get<StartCooperateEvent>(event.event);
    parent_.process_.StartCooperate(context, startEvent);
    FI_HILOGI("[relay cooperate] To \'%{public}s\'", Utility::Anonymize(parent_.process_.Peer()).c_str());

    if (relay_ != nullptr) {
        Switch(relay_);
        relay_->OnProgress(context, event);
    }
}

void CooperateIn::Initial::OnRelayWithOptions(Context &context, const CooperateEvent &event)
{
    CALL_INFO_TRACE;
    StartWithOptionsEvent startEvent = std::get<StartWithOptionsEvent>(event.event);
    parent_.process_.StartCooperateWithOptions(context, startEvent);
    FI_HILOGI("[relay cooperate With Options] To '%{public}s'", Utility::Anonymize(parent_.process_.Peer()).c_str());
    if (relay_ == nullptr) {
        FI_HILOGE("relay_ is nullptr");
        return;
    }
    Switch(relay_);
    relay_->OnProgressWithOptions(context, event);
}

void CooperateIn::Initial::OnStartWithOptions(Context &context, const CooperateEvent &event)
{
    CALL_INFO_TRACE;
    CHKPV(parent_.env_);
    StartWithOptionsEvent startEvent = std::get<StartWithOptionsEvent>(event.event);
    context.ResetPriv();
    context.StartCooperateWithOptions(startEvent);
    if (parent_.env_->GetDragManager().GetDragState() == DragState::MOTION_DRAGGING) {
        FI_HILOGE("Not allow cooperate");
        NotAollowCooperateWhenMotionDragging result {
            .pid = startEvent.pid,
            .userData = startEvent.userData,
            .networkId = startEvent.remoteNetworkId,
            .success = false,
            .errCode = static_cast<int32_t>(CoordinationErrCode::NOT_AOLLOW_COOPERATE_WHEN_MOTION_DRAGGING)
        };
        context.eventMgr_.ErrorNotAollowCooperateWhenMotionDragging(result);
        return;
    }
    if (context.IsLocal(startEvent.remoteNetworkId)) {
        DSoftbusCooperateWithOptionsFinished result {
            .success = false,
            .errCode = static_cast<int32_t>(CoordinationErrCode::UNEXPECTED_START_CALL)
        };
        context.eventMgr_.StartCooperateWithOptionsFinish(result);
        return;
    }
    FI_HILOGI("[start] with options start cooperation(%{public}s, %{public}d)",
        Utility::Anonymize(startEvent.remoteNetworkId).c_str(), startEvent.startDeviceId);
    context.eventMgr_.StartCooperateWithOptions(startEvent);
    if (context.IsPeer(startEvent.remoteNetworkId)) {
        OnComeBackWithOptions(context, event);
    } else {
        OnRelayWithOptions(context, event);
    }
}

void CooperateIn::Initial::OnComeBackWithOptions(Context &context, const CooperateEvent &event)
{
    CALL_INFO_TRACE;
    context.inputEventBuilder_.Disable();
    FI_HILOGI("[come back] To \'%{public}s\'", Utility::Anonymize(context.Peer()).c_str());
    StartWithOptionsEvent withOptionsNotice = std::get<StartWithOptionsEvent>(event.event);
    DSoftbusComeBackWithOptions notice {
        .originNetworkId = context.Local(),
        .success = true,
        .cooperateOptions = {withOptionsNotice.displayX, withOptionsNotice.displayY, withOptionsNotice.displayId},
    };
    context.OnStartCooperate(notice.extra);
    if (context.dsoftbus_.ComeBackWithOptions(context.Peer(), notice) != RET_OK) {
        notice.success = false;
        notice.errCode = static_cast<int32_t>(CoordinationErrCode::SEND_PACKET_FAILED);
    }
    context.eventMgr_.StartCooperateWithOptionsFinish(notice);
    context.inputDevMgr_.RemoveVirtualInputDevice(context.Peer());
    TransiteTo(context, CooperateState::COOPERATE_STATE_FREE);
    context.OnBack();
}

void CooperateIn::Initial::OnStop(Context &context, const CooperateEvent &event)
{
    CALL_INFO_TRACE;
    StopCooperateEvent param = std::get<StopCooperateEvent>(event.event);

    context.eventMgr_.StopCooperate(param);
    parent_.StopCooperate(context, event);

    param.networkId = context.Peer();
    DSoftbusStopCooperateFinished notice {
        .networkId = context.Peer(),
        .normal = true,
    };
    context.eventMgr_.StopCooperateFinish(notice);

    parent_.UnchainConnections(context, param);
}

void CooperateIn::Initial::OnRemoteStart(Context &context, const CooperateEvent &event)
{
    CALL_INFO_TRACE;
    DSoftbusStartCooperate notice = std::get<DSoftbusStartCooperate>(event.event);
    context.StorePeerPointerSpeed(notice.pointerSpeed);
    context.StorePeerTouchPadSpeed(notice.touchPadSpeed);
    if (context.IsPeer(notice.networkId) || context.IsLocal(notice.networkId)) {
        return;
    }
    context.OnResetCooperation();
    context.OnRemoteStartCooperate(notice.extra);
    context.eventMgr_.RemoteStart(notice);

    DSoftbusStopCooperate stopNotice {};
    context.dsoftbus_.StopCooperate(context.Peer(), stopNotice);

    context.RemoteStartSuccess(notice);
    context.inputEventBuilder_.Update(context);
    context.eventMgr_.RemoteStartFinish(notice);
    FI_HILOGI("[remote start] Cooperation with \'%{public}s\' established", Utility::Anonymize(context.Peer()).c_str());
    context.OnTransitionIn();
}

void CooperateIn::Initial::OnRemoteStartWithOptions(Context &context, const CooperateEvent &event)
{
    CALL_INFO_TRACE;
    DSoftbusCooperateOptions notice = std::get<DSoftbusCooperateOptions>(event.event);
    context.StorePeerPointerSpeed(notice.pointerSpeed);
    context.StorePeerTouchPadSpeed(notice.touchPadSpeed);
    if (context.IsPeer(notice.networkId) || context.IsLocal(notice.networkId)) {
        return;
    }
    context.OnResetCooperation();
    context.OnRemoteStartCooperate(notice.extra);
    context.eventMgr_.RemoteStartWithOptions(notice);

    DSoftbusStopCooperate stopNotice {};
    context.dsoftbus_.StopCooperate(context.Peer(), stopNotice);
    context.AdjustPointerPos(notice);
    context.OnRemoteStart(notice);
    context.inputEventBuilder_.Update(context);
    context.eventMgr_.RemoteStartWithOptionsFinish(notice);
    FI_HILOGI("[remote start cooperate with options] Cooperation with \'%{public}s\' established",
        Utility::Anonymize(context.Peer()).c_str());
    context.OnTransitionIn();
}

void CooperateIn::Initial::OnRemoteStop(Context &context, const CooperateEvent &event)
{
    DSoftbusStopCooperate notice = std::get<DSoftbusStopCooperate>(event.event);

    if (!context.IsPeer(notice.networkId)) {
        return;
    }
    FI_HILOGI("[remote stop] Notification from \'%{public}s\'", Utility::Anonymize(notice.networkId).c_str());
    context.eventMgr_.RemoteStop(notice);
    context.inputEventBuilder_.Disable();
    context.eventMgr_.RemoteStopFinish(notice);
    context.inputDevMgr_.RemoveVirtualInputDevice(context.Peer());
    TransiteTo(context, CooperateState::COOPERATE_STATE_FREE);
    context.OnResetCooperation();
}

void CooperateIn::Initial::OnAppClosed(Context &context, const CooperateEvent &event)
{
    FI_HILOGI("[app closed] Close all connections");
    context.dsoftbus_.CloseAllSessions();
    FI_HILOGI("[app closed] Stop cooperation");
    parent_.StopCooperate(context, event);
}

void CooperateIn::Initial::OnPointerEvent(Context &context, const CooperateEvent &event)
{
    InputPointerEvent notice = std::get<InputPointerEvent>(event.event);

    if ((notice.sourceType != MMI::PointerEvent::SOURCE_TYPE_MOUSE) ||
        (filterPointerActions_.find(notice.pointerAction) != filterPointerActions_.end()) ||
        !InputEventBuilder::IsLocalEvent(notice)) {
        return;
    }
    FI_HILOGI("Stop cooperation on operation of local pointer");
    context.OnPointerEvent(notice);
    parent_.StopCooperate(context, event);
}

void CooperateIn::Initial::OnBoardOffline(Context &context, const CooperateEvent &event)
{
    DDMBoardOfflineEvent notice = std::get<DDMBoardOfflineEvent>(event.event);

    if (!context.IsPeer(notice.networkId)) {
        return;
    }
    FI_HILOGI("[board offline] Peer(\'%{public}s\') is offline", Utility::Anonymize(notice.networkId).c_str());
    parent_.StopCooperate(context, event);
}

void CooperateIn::Initial::OnSwitchChanged(Context &context, const CooperateEvent &event)
{
    DDPCooperateSwitchChanged notice = std::get<DDPCooperateSwitchChanged>(event.event);

    if (!context.IsPeer(notice.networkId) || notice.normal) {
        return;
    }
    FI_HILOGI("[switch off] Peer(\'%{public}s\') switch off", Utility::Anonymize(notice.networkId).c_str());
    parent_.StopCooperate(context, event);
}

void CooperateIn::Initial::OnSoftbusSessionClosed(Context &context, const CooperateEvent &event)
{
    DSoftbusSessionClosed notice = std::get<DSoftbusSessionClosed>(event.event);

    if (!context.IsPeer(notice.networkId)) {
        return;
    }
    FI_HILOGI("[softbus session closed] Disconnected with \'%{public}s\'",
        Utility::Anonymize(notice.networkId).c_str());
    parent_.StopCooperate(context, event);
    context.eventMgr_.OnSoftbusSessionClosed(notice);
}

void CooperateIn::Initial::OnRemoteInputDevice(Context &context, const CooperateEvent &event)
{
    CALL_INFO_TRACE;
    DSoftbusSyncInputDevice notice = std::get<DSoftbusSyncInputDevice>(event.event);
    if (!context.IsPeer(notice.networkId)) {
        return;
    }
    FI_HILOGI("Remote input device from \'%{public}s\'", Utility::Anonymize(notice.networkId).c_str());
    context.inputDevMgr_.AddVirtualInputDevice(notice.networkId);
}

void CooperateIn::Initial::OnRemoteHotPlug(Context &context, const CooperateEvent &event)
{
    DSoftbusHotPlugEvent notice = std::get<DSoftbusHotPlugEvent>(event.event);
    if (!context.IsPeer(notice.networkId)) {
        return;
    }
    FI_HILOGI("Remote hot plug event from \'%{public}s\'", Utility::Anonymize(notice.networkId).c_str());
}

void CooperateIn::Initial::OnUpdateCooperateFlag(Context &context, const CooperateEvent &event)
{
    UpdateCooperateFlagEvent notice = std::get<UpdateCooperateFlagEvent>(event.event);
    uint32_t changed = (notice.mask & (context.CooperateFlag() ^ notice.flag));
    context.UpdateCooperateFlag(notice);

    if (changed & COOPERATE_FLAG_FREEZE_CURSOR) {
        FI_HILOGI("Toggle freezing state of cursor");
        if (notice.flag & COOPERATE_FLAG_FREEZE_CURSOR) {
            context.inputEventBuilder_.Freeze();
        } else {
            context.inputEventBuilder_.Thaw();
        }
    }
}

void CooperateIn::Initial::OnProgress(Context &context, const CooperateEvent &event)
{}

void CooperateIn::Initial::OnReset(Context &context, const CooperateEvent &event)
{}

void CooperateIn::Initial::OnProgressWithOptions(Context &context, const CooperateEvent &event)
{}

CooperateIn::RelayConfirmation::RelayConfirmation(CooperateIn &parent, std::shared_ptr<ICooperateStep> prev)
    : ICooperateStep(parent, prev), parent_(parent)
{
    AddHandler(CooperateEventType::DISABLE, [this](Context &context, const CooperateEvent &event) {
        this->OnDisable(context, event);
    });
    AddHandler(CooperateEventType::STOP, [this](Context &context, const CooperateEvent &event) {
        this->OnStop(context, event);
    });
    AddHandler(CooperateEventType::APP_CLOSED, [this](Context &context, const CooperateEvent &event) {
        this->OnAppClosed(context, event);
    });
    AddHandler(CooperateEventType::INPUT_POINTER_EVENT,
        [this](Context &context, const CooperateEvent &event) {
            this->OnPointerEvent(context, event);
    });
    AddHandler(CooperateEventType::DDM_BOARD_OFFLINE,
        [this](Context &context, const CooperateEvent &event) {
            this->OnBoardOffline(context, event);
    });
    AddHandler(CooperateEventType::DDP_COOPERATE_SWITCH_CHANGED,
        [this](Context &context, const CooperateEvent &event) {
            this->OnSwitchChanged(context, event);
    });
    AddHandler(CooperateEventType::DSOFTBUS_SESSION_CLOSED,
        [this](Context &context, const CooperateEvent &event) {
            this->OnSoftbusSessionClosed(context, event);
    });
    AddHandler(CooperateEventType::DSOFTBUS_RELAY_COOPERATE_FINISHED,
        [this](Context &context, const CooperateEvent &event) {
            this->OnResponse(context, event);
    });
    AddHandler(CooperateEventType::DSOFTBUS_START_COOPERATE,
        [this](Context &context, const CooperateEvent &event) {
            this->OnRemoteStart(context, event);
    });
    AddHandler(CooperateEventType::DSOFTBUS_STOP_COOPERATE,
        [this](Context &context, const CooperateEvent &event) {
            this->OnRemoteStop(context, event);
    });
    AddHandler(CooperateEventType::DSOFTBUS_COOPERATE_WITH_OPTIONS,
        [this](Context &context, const CooperateEvent &event) {
            this->OnRemoteStartWithOptions(context, event);
    });
    AddHandler(CooperateEventType::DSOFTBUS_RELAY_COOPERATE_WITHOPTIONS_FINISHED,
        [this](Context &context, const CooperateEvent &event) {
            this->OnResponseWithOptions(context, event);
    });
}

void CooperateIn::RelayConfirmation::OnDisable(Context &context, const CooperateEvent &event)
{
    FI_HILOGI("[relay cooperate] Disable cooperation");
    parent_.StopCooperate(context, event);
    OnReset(context, event);
}

void CooperateIn::RelayConfirmation::OnStop(Context &context, const CooperateEvent &event)
{
    FI_HILOGI("[relay cooperate] Stop cooperation");
    parent_.StopCooperate(context, event);
    OnReset(context, event);

    StopCooperateEvent param = std::get<StopCooperateEvent>(event.event);
    parent_.UnchainConnections(context, param);
}

void CooperateIn::RelayConfirmation::OnRemoteStart(Context &context, const CooperateEvent &event)
{
    CALL_INFO_TRACE;
    DSoftbusStartCooperate notice = std::get<DSoftbusStartCooperate>(event.event);
    context.StorePeerPointerSpeed(notice.pointerSpeed);
    context.StorePeerTouchPadSpeed(notice.touchPadSpeed);
    if (context.IsPeer(notice.networkId) || context.IsLocal(notice.networkId)) {
        return;
    }
    FI_HILOGI("[remote start] Notification from %{public}s", Utility::Anonymize(notice.networkId).c_str());
    if (parent_.process_.IsPeer(notice.networkId)) {
        auto ret = context.Sender().Send(event);
        if (ret != Channel<CooperateEvent>::NO_ERROR) {
            FI_HILOGE("Failed to send event via channel, error:%{public}d", ret);
        }
        OnResetWithNotifyMessage(context, event);
        return;
    }
    parent_.env_->GetTimerManager().AddTimer(DEFAULT_COOLING_TIME, REPEAT_ONCE,
        [sender = context.Sender(), event]() mutable {
            auto ret = sender.Send(event);
            if (ret != Channel<CooperateEvent>::NO_ERROR) {
                FI_HILOGE("Failed to send event via channel, error:%{public}d", ret);
            }
        });
}

void CooperateIn::RelayConfirmation::OnRemoteStartWithOptions(Context &context, const CooperateEvent &event)
{
    CALL_INFO_TRACE;
    DSoftbusCooperateOptions notice = std::get<DSoftbusCooperateOptions>(event.event);
    context.StorePeerPointerSpeed(notice.pointerSpeed);
    context.StorePeerTouchPadSpeed(notice.touchPadSpeed);
    if (context.IsPeer(notice.networkId) || context.IsLocal(notice.networkId)) {
        return;
    }
    FI_HILOGI("[remote start] Notification from %{public}s", Utility::Anonymize(notice.networkId).c_str());
    if (parent_.process_.IsPeer(notice.networkId)) {
        auto ret = context.Sender().Send(event);
        if (ret != Channel<CooperateEvent>::NO_ERROR) {
            FI_HILOGE("Failed to send with options event via channel, error:%{public}d", ret);
        }
        OnReset(context, event);
        return;
    }
    parent_.env_->GetTimerManager().AddTimer(DEFAULT_COOLING_TIME, REPEAT_ONCE,
        [sender = context.Sender(), event]() mutable {
            auto ret = sender.Send(event);
            if (ret != Channel<CooperateEvent>::NO_ERROR) {
                FI_HILOGE("Failed to send with options event via channel, error:%{public}d", ret);
            }
        });
}

void CooperateIn::RelayConfirmation::OnRemoteStop(Context &context, const CooperateEvent &event)
{
    DSoftbusStopCooperate notice = std::get<DSoftbusStopCooperate>(event.event);

    if (!context.IsPeer(notice.networkId)) {
        return;
    }
    FI_HILOGI("[remote stop] Notification from %{public}s", Utility::Anonymize(notice.networkId).c_str());
    auto ret = context.Sender().Send(event);
    if (ret != Channel<CooperateEvent>::NO_ERROR) {
        FI_HILOGE("Failed to send event via channel, error:%{public}d", ret);
    }
    OnReset(context, event);
}

void CooperateIn::RelayConfirmation::OnAppClosed(Context &context, const CooperateEvent &event)
{
    FI_HILOGI("[app closed] Close all connections");
    context.dsoftbus_.CloseAllSessions();
    FI_HILOGI("[relay cooperate] Stop cooperation on app closed");
    parent_.StopCooperate(context, event);
    OnResetWithNotifyMessage(context, event);
}

void CooperateIn::RelayConfirmation::OnPointerEvent(Context &context, const CooperateEvent &event)
{
    InputPointerEvent notice = std::get<InputPointerEvent>(event.event);

    if ((notice.sourceType != MMI::PointerEvent::SOURCE_TYPE_MOUSE) ||
        !InputEventBuilder::IsLocalEvent(notice)) {
        return;
    }
    FI_HILOGI("[relay cooperate] Stop cooperation on operation of local pointer");
    context.OnPointerEvent(notice);
    parent_.StopCooperate(context, event);
    OnReset(context, event);
}

void CooperateIn::RelayConfirmation::OnBoardOffline(Context &context, const CooperateEvent &event)
{
    DDMBoardOfflineEvent notice = std::get<DDMBoardOfflineEvent>(event.event);

    if (!context.IsPeer(notice.networkId) && !parent_.process_.IsPeer(notice.networkId)) {
        return;
    }
    FI_HILOGI("[relay cooperate] Peer(%{public}s) is offline", Utility::Anonymize(notice.networkId).c_str());
    if (context.IsPeer(notice.networkId)) {
        parent_.StopCooperate(context, event);
    }
    OnResetWithNotifyMessage(context, event);
}

void CooperateIn::RelayConfirmation::OnSwitchChanged(Context &context, const CooperateEvent &event)
{
    DDPCooperateSwitchChanged notice = std::get<DDPCooperateSwitchChanged>(event.event);

    if (notice.normal ||
        (!context.IsPeer(notice.networkId) &&
         !parent_.process_.IsPeer(notice.networkId))) {
        return;
    }
    FI_HILOGI("[relay cooperate] Peer(\'%{public}s\') switch off", Utility::Anonymize(notice.networkId).c_str());
    if (context.IsPeer(notice.networkId)) {
        parent_.StopCooperate(context, event);
    }
    OnResetWithNotifyMessage(context, event);
}

void CooperateIn::RelayConfirmation::OnSoftbusSessionClosed(Context &context, const CooperateEvent &event)
{
    DSoftbusSessionClosed notice = std::get<DSoftbusSessionClosed>(event.event);

    if (!context.IsPeer(notice.networkId) && !parent_.process_.IsPeer(notice.networkId)) {
        return;
    }
    FI_HILOGI("[relay cooperate] Disconnected with \'%{public}s\'", Utility::Anonymize(notice.networkId).c_str());
    if (context.IsPeer(notice.networkId)) {
        parent_.StopCooperate(context, event);
    }
    OnResetWithNotifyMessage(context, event);
}

void CooperateIn::RelayConfirmation::OnResponse(Context &context, const CooperateEvent &event)
{
    DSoftbusRelayCooperateFinished notice = std::get<DSoftbusRelayCooperateFinished>(event.event);

    if (!context.IsPeer(notice.networkId)) {
        return;
    }
    FI_HILOGI("[relay cooperate] \'%{public}s\' respond", Utility::Anonymize(notice.networkId).c_str());
    parent_.env_->GetTimerManager().RemoveTimer(timerId_);
    if (notice.normal) {
        OnNormal(context, event);
        Proceed(context, event);
    } else {
        OnResetWithNotifyMessage(context, event);
        parent_.StopCooperate(context, event);
    }
}

void CooperateIn::RelayConfirmation::OnResponseWithOptions(Context &context, const CooperateEvent &event)
{
    DSoftbusRelayCooperateFinished notice = std::get<DSoftbusRelayCooperateFinished>(event.event);
    if (!context.IsPeer(notice.networkId)) {
        return;
    }
    FI_HILOGI("[relay cooperate] \'%{public}s\' respond", Utility::Anonymize(notice.networkId).c_str());
    parent_.env_->GetTimerManager().RemoveTimer(timerId_);
    if (notice.normal) {
        OnNormalWithOptions(context, event);
        Proceed(context, event);
    } else {
        OnResetWithOptionsNotifyMessage(context, event);
        parent_.StopCooperate(context, event);
    }
}

void CooperateIn::RelayConfirmation::OnNormal(Context &context, const CooperateEvent &event)
{
    FI_HILOGI("[relay cooperate] Cooperation with \'%{public}s\' established",
        Utility::Anonymize(parent_.process_.Peer()).c_str());
    context.inputEventBuilder_.Disable();
    DSoftbusRelayCooperateFinished noticeFinished = std::get<DSoftbusRelayCooperateFinished>(event.event);
    DSoftbusStartCooperate notice {
        .originNetworkId = context.Peer(),
        .success = true,
        .cursorPos = context.NormalizedCursorPosition(),
        .uid = noticeFinished.uid,
    };
    context.OnStartCooperate(notice.extra);
    context.dsoftbus_.StartCooperate(parent_.process_.Peer(), notice);

    context.eventMgr_.StartCooperateFinish(notice);
    TransiteTo(context, CooperateState::COOPERATE_STATE_FREE);
    context.OnRelayCooperation(parent_.process_.Peer(), context.NormalizedCursorPosition());
}

void CooperateIn::RelayConfirmation::OnNormalWithOptions(Context &context, const CooperateEvent &event)
{
    FI_HILOGI("[relay cooperate] Cooperation with \'%{public}s\' established",
        Utility::Anonymize(parent_.process_.Peer()).c_str());
    context.inputEventBuilder_.Disable();

    DSoftbusCooperateOptions notice {
        .originNetworkId = context.Peer(),
        .success = true,
        .cooperateOptions = {startWithOptionsEvent_.displayX, startWithOptionsEvent_.displayY,
            startWithOptionsEvent_.displayId},
    };
    context.OnStartCooperate(notice.extra);
    context.dsoftbus_.StartCooperateWithOptions(parent_.process_.Peer(), notice);

    context.eventMgr_.StartCooperateWithOptionsFinish(notice);
    TransiteTo(context, CooperateState::COOPERATE_STATE_FREE);
    context.OnRelayCooperation(parent_.process_.Peer(), context.NormalizedCursorPosition());
}

void CooperateIn::RelayConfirmation::OnProgress(Context &context, const CooperateEvent &event)
{
    std::string remoteNetworkId = parent_.process_.Peer();
    FI_HILOGI("[relay cooperate] Connect \'%{public}s\'", Utility::Anonymize(remoteNetworkId).c_str());
    int32_t ret = context.dsoftbus_.OpenSession(remoteNetworkId);
    if (ret != RET_OK) {
        FI_HILOGE("[relay cooperate] Failed to connect to \'%{public}s\'", Utility::Anonymize(remoteNetworkId).c_str());
        OnResetWithNotifyMessage(context, event);
        return;
    }
    StartCooperateEvent startEvent = std::get<StartCooperateEvent>(event.event);
    FI_HILOGI("[relay cooperate] Notify origin(\'%{public}s\')", Utility::Anonymize(context.Peer()).c_str());
    DSoftbusRelayCooperate notice {
        .targetNetworkId = parent_.process_.Peer(),
        .pointerSpeed = context.GetPointerSpeed(),
        .touchPadSpeed = context.GetTouchPadSpeed(),
        .uid = startEvent.uid,
    };
    context.dsoftbus_.RelayCooperate(context.Peer(), notice);
    timerId_ = parent_.env_->GetTimerManager().AddTimer(DEFAULT_TIMEOUT, REPEAT_ONCE,
        [sender = context.Sender(), remoteNetworkId = context.Peer()]() mutable {
            auto ret = sender.Send(CooperateEvent(
                CooperateEventType::DSOFTBUS_RELAY_COOPERATE_FINISHED,
                DSoftbusRelayCooperateFinished {
                    .networkId = remoteNetworkId,
                    .normal = false,
                }));
            if (ret != Channel<CooperateEvent>::NO_ERROR) {
                FI_HILOGE("Failed to send event via channel, error:%{public}d", ret);
            }
        });
}

void CooperateIn::RelayConfirmation::OnProgressWithOptions(Context &context, const CooperateEvent &event)
{
    std::string remoteNetworkId = parent_.process_.Peer();
    FI_HILOGI("[relay cooperate] Connect \'%{public}s\'", Utility::Anonymize(remoteNetworkId).c_str());
    int32_t ret = context.dsoftbus_.OpenSession(remoteNetworkId);
    if (ret != RET_OK) {
        FI_HILOGE("[relay cooperate] Failed to connect to \'%{public}s\'", Utility::Anonymize(remoteNetworkId).c_str());
        OnResetWithOptionsNotifyMessage(context, event);
        return;
    }

    FI_HILOGI("[relay cooperate] Notify origin(\'%{public}s\')", Utility::Anonymize(context.Peer()).c_str());
    DSoftbusRelayCooperate notice {
        .targetNetworkId = parent_.process_.Peer(),
        .pointerSpeed = context.GetPointerSpeed(),
        .touchPadSpeed = context.GetTouchPadSpeed(),
    };
    context.dsoftbus_.RelayCooperateWithOptions(context.Peer(), notice);
    startWithOptionsEvent_ = std::get<StartWithOptionsEvent>(event.event);
    timerId_ = parent_.env_->GetTimerManager().AddTimer(DEFAULT_TIMEOUT, REPEAT_ONCE,
        [sender = context.Sender(), remoteNetworkId = context.Peer()]() mutable {
            auto ret = sender.Send(CooperateEvent(
                CooperateEventType::DSOFTBUS_RELAY_COOPERATE_WITHOPTIONS_FINISHED,
                DSoftbusRelayCooperateFinished {
                    .networkId = remoteNetworkId,
                    .normal = false,
                }));
            if (ret != Channel<CooperateEvent>::NO_ERROR) {
                FI_HILOGE("Failed to send event via channel, error:%{public}d", ret);
            }
        });
}

void CooperateIn::RelayConfirmation::OnReset(Context &context, const CooperateEvent &event)
{
    FI_HILOGI("[relay cooperate] reset cooperation with \'%{public}s\'",
        Utility::Anonymize(parent_.process_.Peer()).c_str());
    Reset(context, event);
}

void CooperateIn::RelayConfirmation::OnResetWithNotifyMessage(Context &context, const CooperateEvent &event)
{
    FI_HILOGI("[relay cooperate] reset cooperation with \'%{public}s\'",
        Utility::Anonymize(parent_.process_.Peer()).c_str());
    DSoftbusStartCooperateFinished result {
        .success = false
    };
    context.eventMgr_.StartCooperateFinish(result);
    Reset(context, event);
}

void CooperateIn::RelayConfirmation::OnResetWithOptionsNotifyMessage(Context &context, const CooperateEvent &event)
{
    FI_HILOGI("[relay cooperate] reset cooperation with \'%{public}s\'",
        Utility::Anonymize(parent_.process_.Peer()).c_str());
    DSoftbusCooperateWithOptionsFinished result {
        .success = false
    };
    context.eventMgr_.StartCooperateWithOptionsFinish(result);
    Reset(context, event);
}

void CooperateIn::StopCooperate(Context &context, const CooperateEvent &event)
{
    FI_HILOGI("Stop cooperation with \'%{public}s\'", Utility::Anonymize(context.Peer()).c_str());
    context.inputEventBuilder_.Disable();
    context.UpdateCursorPosition();

    DSoftbusStopCooperate notice {};
    context.dsoftbus_.StopCooperate(context.Peer(), notice);
    TransiteTo(context, CooperateState::COOPERATE_STATE_FREE);
    context.OnResetCooperation();
    SetPointerVisible(context);
}

void CooperateIn::SetPointerVisible(Context &context)
{
    CHKPV(env_);
    bool hasLocalPointerDevice =  env_->GetDeviceManager().HasLocalPointerDevice() ||
        env_->GetInput().HasLocalPointerDevice() ||
        env_->GetDeviceManager().HasPencilAirMouse();
    FI_HILOGI("HasLocalPointerDevice:%{public}s", hasLocalPointerDevice ? "true" : "false");
    env_->GetInput().SetPointerVisibility(hasLocalPointerDevice, PRIORITY);
}

void CooperateIn::UnchainConnections(Context &context, const StopCooperateEvent &event) const
{
    if (event.isUnchained) {
        FI_HILOGI("Unchain all connections");
        context.dsoftbus_.CloseAllSessions();
        context.eventMgr_.OnUnchain(event);
    }
}
} // namespace Cooperate
} // namespace DeviceStatus
} // namespace Msdp
} // namespace OHOS
