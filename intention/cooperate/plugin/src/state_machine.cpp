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

#include "state_machine.h"

#include "application_state_observer_stub.h"
#include "iservice_registry.h"
#include "system_ability_definition.h"

#include "common_event_observer.h"
#include "cooperate_events.h"
#include "cooperate_free.h"
#include "cooperate_hisysevent.h"
#include "cooperate_in.h"
#include "cooperate_out.h"
#include "devicestatus_define.h"
#include "devicestatus_errors.h"
#include "event_manager.h"
#include "utility.h"

#undef LOG_TAG
#define LOG_TAG "StateMachine"

namespace OHOS {
namespace Msdp {
namespace DeviceStatus {

namespace {
const std::string VIRTUAL_TRACK_PAD_NAME { "VirtualTrackpad" };
}
namespace Cooperate {

StateMachine::AppStateObserver::AppStateObserver(Channel<CooperateEvent>::Sender sender, int32_t clientPid)
    : sender_(sender), clientPid_(clientPid) {}

void StateMachine::AppStateObserver::OnProcessDied(const AppExecFwk::ProcessData &processData)
{
    FI_HILOGI("\'%{public}s\' died, pid:%{public}d", processData.bundleName.c_str(), processData.pid);
    if (processData.pid == clientPid_) {
        auto ret = sender_.Send(CooperateEvent(
            CooperateEventType::APP_CLOSED,
            ClientDiedEvent {
                .pid = clientPid_,
            }));
        if (ret != Channel<CooperateEvent>::NO_ERROR) {
            FI_HILOGE("Failed to send event via channel, error:%{public}d", ret);
        }
        FI_HILOGI("\'%{public}s\' died, report to handler", processData.bundleName.c_str());
    }
}

void StateMachine::AppStateObserver::UpdateClientPid(int32_t clientPid)
{
    clientPid_ = clientPid;
}

StateMachine::StateMachine(IContext *env)
    : env_(env)
{
    states_[COOPERATE_STATE_FREE] = std::make_shared<CooperateFree>(*this, env);
    states_[COOPERATE_STATE_OUT] = std::make_shared<CooperateOut>(*this, env);
    states_[COOPERATE_STATE_IN] = std::make_shared<CooperateIn>(*this, env);

    AddHandler(CooperateEventType::ADD_OBSERVER, [this](Context &context, const CooperateEvent &event) {
        this->AddObserver(context, event);
    });
    AddHandler(CooperateEventType::REMOVE_OBSERVER, [this](Context &context, const CooperateEvent &event) {
        this->RemoveObserver(context, event);
    });
    AddHandler(CooperateEventType::REGISTER_LISTENER, [this](Context &context, const CooperateEvent &event) {
        this->RegisterListener(context, event);
    });
    AddHandler(CooperateEventType::UNREGISTER_LISTENER, [this](Context &context, const CooperateEvent &event) {
        this->UnregisterListener(context, event);
    });
    AddHandler(CooperateEventType::REGISTER_HOTAREA_LISTENER, [this](Context &context, const CooperateEvent &event) {
        this->RegisterHotAreaListener(context, event);
    });
    AddHandler(CooperateEventType::UNREGISTER_HOTAREA_LISTENER,
        [this](Context &context, const CooperateEvent &event) {
            this->UnregisterHotAreaListener(context, event);
    });
    AddHandler(CooperateEventType::ENABLE, [this](Context &context, const CooperateEvent &event) {
        this->EnableCooperate(context, event);
    });
    AddHandler(CooperateEventType::DISABLE, [this](Context &context, const CooperateEvent &event) {
        this->DisableCooperate(context, event);
    });
    AddHandler(CooperateEventType::START, [this](Context &context, const CooperateEvent &event) {
        this->StartCooperate(context, event);
    });
    AddHandler(CooperateEventType::GET_COOPERATE_STATE, [this](Context &context, const CooperateEvent &event) {
        this->GetCooperateState(context, event);
    });
    AddHandler(CooperateEventType::WITH_OPTIONS_START, [this](Context &context, const CooperateEvent &event) {
        this->StartCooperateWithOptions(context, event);
    });
    AddHandler(CooperateEventType::REGISTER_EVENT_LISTENER,
        [this](Context &context, const CooperateEvent &event) {
            this->RegisterEventListener(context, event);
    });
    AddHandler(CooperateEventType::UNREGISTER_EVENT_LISTENER,
        [this](Context &context, const CooperateEvent &event) {
            this->UnregisterEventListener(context, event);
    });
    AddHandler(CooperateEventType::DDM_BOARD_ONLINE,
        [this](Context &context, const CooperateEvent &event) {
            this->OnBoardOnline(context, event);
    });
    AddHandler(CooperateEventType::DDM_BOARD_OFFLINE,
        [this](Context &context, const CooperateEvent &event) {
            this->OnBoardOffline(context, event);
    });
    AddHandler(CooperateEventType::DDP_COOPERATE_SWITCH_CHANGED,
        [this](Context &context, const CooperateEvent &event) {
            this->OnProfileChanged(context, event);
    });
    AddHandler(CooperateEventType::INPUT_POINTER_EVENT,
        [this](Context &context, const CooperateEvent &event) {
            this->OnPointerEvent(context, event);
    });
    AddHandler(CooperateEventType::APP_CLOSED, [this](Context &context, const CooperateEvent &event) {
        this->OnProcessClientDied(context, event);
    });
    AddHandler(CooperateEventType::DSOFTBUS_SESSION_OPENED,
        [this](Context &context, const CooperateEvent &event) {
            this->OnSoftbusSessionOpened(context, event);
    });
    AddHandler(CooperateEventType::DSOFTBUS_SESSION_CLOSED,
        [this](Context &context, const CooperateEvent &event) {
            this->OnSoftbusSessionClosed(context, event);
    });
    AddHandler(CooperateEventType::DSOFTBUS_SUBSCRIBE_MOUSE_LOCATION,
        [this](Context &context, const CooperateEvent &event) {
            this->OnSoftbusSubscribeMouseLocation(context, event);
    });
    AddHandler(CooperateEventType::DSOFTBUS_UNSUBSCRIBE_MOUSE_LOCATION,
        [this](Context &context, const CooperateEvent &event) {
            this->OnSoftbusUnSubscribeMouseLocation(context, event);
    });
    AddHandler(CooperateEventType::DSOFTBUS_REPLY_SUBSCRIBE_MOUSE_LOCATION,
        [this](Context &context, const CooperateEvent &event) {
            this->OnSoftbusReplySubscribeMouseLocation(context, event);
    });
    AddHandler(CooperateEventType::DSOFTBUS_REPLY_UNSUBSCRIBE_MOUSE_LOCATION,
        [this](Context &context, const CooperateEvent &event) {
            this->OnSoftbusReplyUnSubscribeMouseLocation(context, event);
    });
    AddHandler(CooperateEventType::DSOFTBUS_MOUSE_LOCATION,
        [this](Context &context, const CooperateEvent &event) {
            this->OnSoftbusMouseLocation(context, event);
    });
    AddHandler(CooperateEventType::DSOFTBUS_START_COOPERATE,
        [this](Context &context, const CooperateEvent &event) {
            this->OnRemoteStart(context, event);
    });
    AddHandler(CooperateEventType::INPUT_HOTPLUG_EVENT,
        [this](Context &context, const CooperateEvent &event) {
            this->OnHotPlugEvent(context, event);
    });
    AddHandler(CooperateEventType::DSOFTBUS_INPUT_DEV_HOT_PLUG,
        [this](Context &context, const CooperateEvent &event) {
            this->OnRemoteHotPlug(context, event);
    });
    AddHandler(CooperateEventType::DSOFTBUS_INPUT_DEV_SYNC,
        [this](Context &context, const CooperateEvent &event) {
            this->OnRemoteInputDevice(context, event);
    });
    AddHandler(CooperateEventType::STOP, [this](Context &context, const CooperateEvent &event) {
        this->StopCooperate(context, event);
    });
    AddHandler(CooperateEventType::UPDATE_VIRTUAL_DEV_ID_MAP, [this](Context &context, const CooperateEvent &event) {
        this->UpdateVirtualDeviceIdMap(context, event);
    });
    AddHandler(CooperateEventType::DSOFTBUS_COOPERATE_WITH_OPTIONS,
        [this](Context &context, const CooperateEvent &event) {
            this->OnRemoteStartWithOptions(context, event);
    });
}

void StateMachine::OnEvent(Context &context, const CooperateEvent &event)
{
    if (auto iter = handlers_.find(event.type); iter != handlers_.end()) {
        iter->second(context, event);
    } else {
        Transfer(context, event);
    }
}

void StateMachine::TransiteTo(Context &context, CooperateState state)
{
    if ((state >= COOPERATE_STATE_FREE) &&
        (state < N_COOPERATE_STATES) &&
        (state != current_)) {
        states_[current_]->OnLeaveState(context);
        current_ = state;
        states_[current_]->OnEnterState(context);
        StatusChangeEvent event = {
            .networkId = IDSoftbusAdapter::GetLocalNetworkId(),
            .msg = CoordinationMessage::COORDINATION_STATUS_FREE,
        };
        if (state == COOPERATE_STATE_OUT) {
            event.msg = CoordinationMessage::COORDINATION_STATUS_OUT;
        }
        if (state == COOPERATE_STATE_IN) {
            event.msg = CoordinationMessage::COORDINATION_STATUS_IN;
        }
        context.eventMgr_.OnStatusChanged(event);
    }
}

void StateMachine::AddHandler(CooperateEventType event, std::function<void(Context&, const CooperateEvent&)> handler)
{
    handlers_.emplace(event, handler);
}

void StateMachine::OnQuit(Context &context)
{
    CALL_DEBUG_ENTER;
    RemoveWatches(context);
    RemoveMonitor(context);
}

void StateMachine::AddObserver(Context &context, const CooperateEvent &event)
{
    AddObserverEvent notice = std::get<AddObserverEvent>(event.event);
    context.AddObserver(notice.observer);
}

void StateMachine::RemoveObserver(Context &context, const CooperateEvent &event)
{
    RemoveObserverEvent notice = std::get<RemoveObserverEvent>(event.event);
    context.RemoveObserver(notice.observer);
}

void StateMachine::RegisterListener(Context &context, const CooperateEvent &event)
{
    RegisterListenerEvent notice = std::get<RegisterListenerEvent>(event.event);
    context.eventMgr_.RegisterListener(notice);
}

void StateMachine::UnregisterListener(Context &context, const CooperateEvent &event)
{
    UnregisterListenerEvent notice = std::get<UnregisterListenerEvent>(event.event);
    context.eventMgr_.UnregisterListener(notice);
}

void StateMachine::RegisterHotAreaListener(Context &context, const CooperateEvent &event)
{
    RegisterHotareaListenerEvent notice = std::get<RegisterHotareaListenerEvent>(event.event);
    context.hotArea_.AddListener(notice);
}

void StateMachine::UnregisterHotAreaListener(Context &context, const CooperateEvent &event)
{
    UnregisterHotareaListenerEvent notice = std::get<UnregisterHotareaListenerEvent>(event.event);
    context.hotArea_.RemoveListener(notice);
}

void StateMachine::EnableCooperate(Context &context, const CooperateEvent &event)
{
    CALL_INFO_TRACE;
    EnableCooperateEvent enableEvent = std::get<EnableCooperateEvent>(event.event);
    context.EnableCooperate(enableEvent);
    context.eventMgr_.EnableCooperate(enableEvent);
    context.hotArea_.EnableCooperate(enableEvent);
    observer_ = CommonEventObserver::CreateCommonEventObserver(
        [&context, this] (const std::string &commonEvent) {
            OnCommonEvent(context, commonEvent);
        }
    );
    context.commonEvent_.AddObserver(observer_);
    AddSessionObserver(context, enableEvent);
    AddMonitor(context);
    AddPreMonitor(context);
    auto devAddedCallback = [this, &context](int32_t deviceId, const std::string &type) {
        FI_HILOGI("Device added");
        bool isVirtualtrackpad = this->CheckIsVirtualTrackpad(deviceId);
        if (isVirtualtrackpad) {
            context.SetVirtualTrackpadDeviceId(deviceId);
        }
    };
    auto devRemovedCallback = [this, &context](int32_t deviceId, const std::string &type) {
        FI_HILOGI("Device removed, deviceId %{public}d", deviceId);
        if (deviceId == context.GetVirtualTrackpadDeviceId()) {
            ResetCooperate(context);
            context.ResetVirtualTrackpadDeviceId();
        }
    };
    CHKPV(env_);
    env_->GetInput().RegisterDevListener(devAddedCallback, devRemovedCallback);
    isCooperateEnable_ = true;
    Transfer(context, event);
}

void StateMachine::DisableCooperate(Context &context, const CooperateEvent &event)
{
    CALL_INFO_TRACE;
    CHKPV(env_);
    DisableCooperateEvent disableEvent = std::get<DisableCooperateEvent>(event.event);
    context.DisableCooperate(disableEvent);
    context.eventMgr_.DisableCooperate(disableEvent);
    context.commonEvent_.RemoveObserver(observer_);
    context.inputDevMgr_.RemoveAllVirtualInputDevice();
    RemoveSessionObserver(context, disableEvent);
    RemoveMonitor(context);
    RemovePreMonitor(context);
    env_->GetInput().UnregisterDevListener();
    context.ResetVirtualTrackpadDeviceId();
    isCooperateEnable_ = false;
    Transfer(context, event);
}

void StateMachine::StartCooperate(Context &context, const CooperateEvent &event)
{
    CALL_INFO_TRACE;
    CHKPV(env_);
    StartCooperateEvent startEvent = std::get<StartCooperateEvent>(event.event);
    bool checkSameAccount = false;
    if (startEvent.uid > 0) {
        checkSameAccount = env_->GetDDM().CheckSameAccountToLocalWithUid(startEvent.remoteNetworkId, startEvent.uid);
    } else {
        checkSameAccount = env_->GetDDM().CheckSameAccountToLocal(startEvent.remoteNetworkId);
    }
    if (!checkSameAccount) {
        FI_HILOGE("CheckSameAccountToLocal failed");
        startEvent.errCode->set_value(COMMON_PERMISSION_CHECK_ERROR);
        CooperateRadarInfo radarInfo {
            .funcName =  __FUNCTION__,
            .bizState = static_cast<int32_t> (BizState::STATE_END),
            .bizStage = static_cast<int32_t> (BizCooperateStage::STAGE_CHECK_SAME_ACCOUNT),
            .stageRes = static_cast<int32_t> (BizCooperateStageRes::RES_FAIL),
            .bizScene = static_cast<int32_t> (BizCooperateScene::SCENE_ACTIVE),
            .errCode = static_cast<int32_t> (CooperateRadarErrCode::CHECK_SAME_ACCOUNT_FAILED),
            .hostName = "",
            .localNetId = Utility::DFXRadarAnonymize(context.Local().c_str()),
            .peerNetId = Utility::DFXRadarAnonymize(startEvent.remoteNetworkId.c_str())
        };
        CooperateRadar::ReportCooperateRadarInfo(radarInfo);
        return;
    }
    UpdateApplicationStateObserver(startEvent.pid);
    if (!context.IsAllowCooperate()) {
        FI_HILOGI("Not allow cooperate");
        startEvent.errCode->set_value(COMMON_NOT_ALLOWED_DISTRIBUTED);
        CooperateRadarInfo radarInfo {
            .funcName = __FUNCTION__,
            .bizState = static_cast<int32_t> (BizState::STATE_END),
            .bizStage = static_cast<int32_t> (BizCooperateStage::STAGE_CHECK_ALLOW_COOPERATE),
            .stageRes = static_cast<int32_t> (BizCooperateStageRes::RES_FAIL),
            .bizScene = static_cast<int32_t> (BizCooperateScene::SCENE_ACTIVE),
            .errCode = static_cast<int32_t> (CooperateRadarErrCode::CHECK_ALLOW_COOPERATE_FAILED),
            .hostName = "",
            .localNetId = Utility::DFXRadarAnonymize(context.Local().c_str()),
            .peerNetId = Utility::DFXRadarAnonymize(startEvent.remoteNetworkId.c_str())
        };
        CooperateRadar::ReportCooperateRadarInfo(radarInfo);
        return;
    }
    startEvent.errCode->set_value(RET_OK);
    Transfer(context, event);
}

void StateMachine::StopCooperate(Context &context, const CooperateEvent &event)
{
    CALL_DEBUG_ENTER;
    context.CloseDistributedFileConnection(context.Peer());
    context.OnStopCooperate();
    context.inputDevMgr_.RemoveAllVirtualInputDevice();
    Transfer(context, event);
}

void StateMachine::StartCooperateWithOptions(Context &context, const CooperateEvent &event)
{
    CALL_INFO_TRACE;
    CHKPV(env_);
    StartWithOptionsEvent withOptionsEvent = std::get<StartWithOptionsEvent>(event.event);
    if (!env_->GetDDM().CheckSameAccountToLocal(withOptionsEvent.remoteNetworkId)) {
        FI_HILOGE("CheckSameAccountToLocal failed");
        withOptionsEvent.errCode->set_value(COMMON_PERMISSION_CHECK_ERROR);
        CooperateRadarInfo radarInfo {
            .funcName =  __FUNCTION__,
            .bizState = static_cast<int32_t> (BizState::STATE_END),
            .bizStage = static_cast<int32_t> (BizCooperateStage::STAGE_CHECK_SAME_ACCOUNT),
            .stageRes = static_cast<int32_t> (BizCooperateStageRes::RES_FAIL),
            .bizScene = static_cast<int32_t> (BizCooperateScene::SCENE_ACTIVE),
            .errCode = static_cast<int32_t> (CooperateRadarErrCode::CHECK_SAME_ACCOUNT_FAILED),
            .hostName = "",
            .localNetId = Utility::DFXRadarAnonymize(context.Local().c_str()),
            .peerNetId = Utility::DFXRadarAnonymize(withOptionsEvent.remoteNetworkId.c_str())
        };
        CooperateRadar::ReportCooperateRadarInfo(radarInfo);
        return;
    }
    UpdateApplicationStateObserver(withOptionsEvent.pid);
    if (!context.IsAllowCooperate()) {
        FI_HILOGI("Not allow cooperate");
        withOptionsEvent.errCode->set_value(COMMON_NOT_ALLOWED_DISTRIBUTED);
        CooperateRadarInfo radarInfo {
            .funcName = __FUNCTION__,
            .bizState = static_cast<int32_t> (BizState::STATE_END),
            .bizStage = static_cast<int32_t> (BizCooperateStage::STAGE_CHECK_ALLOW_COOPERATE),
            .stageRes = static_cast<int32_t> (BizCooperateStageRes::RES_FAIL),
            .bizScene = static_cast<int32_t> (BizCooperateScene::SCENE_ACTIVE),
            .errCode = static_cast<int32_t> (CooperateRadarErrCode::CHECK_ALLOW_COOPERATE_FAILED),
            .hostName = "",
            .localNetId = Utility::DFXRadarAnonymize(context.Local().c_str()),
            .peerNetId = Utility::DFXRadarAnonymize(withOptionsEvent.remoteNetworkId.c_str())
        };
        CooperateRadar::ReportCooperateRadarInfo(radarInfo);
        return;
    }
    withOptionsEvent.errCode->set_value(RET_OK);
    Transfer(context, event);
}

void StateMachine::GetCooperateState(Context &context, const CooperateEvent &event)
{
    CALL_INFO_TRACE;
    GetCooperateStateEvent stateEvent = std::get<GetCooperateStateEvent>(event.event);
    UpdateApplicationStateObserver(stateEvent.pid);
    EventManager::CooperateStateNotice notice {
        .pid = stateEvent.pid,
        .msgId = MessageId::COORDINATION_GET_STATE,
        .userData = stateEvent.userData,
        .state = isCooperateEnable_,
    };
    context.eventMgr_.GetCooperateState(notice);
}

void StateMachine::OnProcessClientDied(Context &context, const CooperateEvent &event)
{
    CALL_INFO_TRACE;
    ClientDiedEvent notice = std::get<ClientDiedEvent>(event.event);
    context.eventMgr_.OnClientDied(notice);
    context.hotArea_.OnClientDied(notice);
    context.mouseLocation_.OnClientDied(notice);
    Transfer(context, event);
}

void StateMachine::RegisterEventListener(Context &context, const CooperateEvent &event)
{
    RegisterEventListenerEvent notice = std::get<RegisterEventListenerEvent>(event.event);
    context.mouseLocation_.AddListener(notice);
}

void StateMachine::UnregisterEventListener(Context &context, const CooperateEvent &event)
{
    UnregisterEventListenerEvent notice = std::get<UnregisterEventListenerEvent>(event.event);
    context.mouseLocation_.RemoveListener(notice);
}

void StateMachine::OnBoardOnline(Context &context, const CooperateEvent &event)
{
    CALL_INFO_TRACE;
    DDMBoardOnlineEvent onlineEvent = std::get<DDMBoardOnlineEvent>(event.event);

    auto ret = onlineBoards_.insert(onlineEvent.networkId);
    if (ret.second) {
        FI_HILOGD("Watch \'%{public}s\'", Utility::Anonymize(onlineEvent.networkId).c_str());
        Transfer(context, event);
    }
}

void StateMachine::OnBoardOffline(Context &context, const CooperateEvent &event)
{
    CALL_INFO_TRACE;
    DDMBoardOfflineEvent offlineEvent = std::get<DDMBoardOfflineEvent>(event.event);

    if (auto iter = onlineBoards_.find(offlineEvent.networkId); iter != onlineBoards_.end()) {
        onlineBoards_.erase(iter);
        FI_HILOGD("Remove watch \'%{public}s\'", Utility::Anonymize(offlineEvent.networkId).c_str());
        context.CloseDistributedFileConnection(offlineEvent.networkId);
        Transfer(context, event);
    }
}

void StateMachine::OnProfileChanged(Context &context, const CooperateEvent &event)
{
    CALL_INFO_TRACE;
    DDPCooperateSwitchChanged notice = std::get<DDPCooperateSwitchChanged>(event.event);
    context.eventMgr_.OnProfileChanged(notice);
    Transfer(context, event);
}

void StateMachine::OnPointerEvent(Context &context, const CooperateEvent &event)
{
    CALL_DEBUG_ENTER;
    InputPointerEvent pointerEvent = std::get<InputPointerEvent>(event.event);
    Coordinate cursorPos = context.CursorPosition();
    context.OnPointerEvent(pointerEvent);
    pointerEvent.position = cursorPos;
    Transfer(context, CooperateEvent { CooperateEventType::INPUT_POINTER_EVENT, pointerEvent });
}

void StateMachine::OnSoftbusSessionClosed(Context &context, const CooperateEvent &event)
{
    CALL_INFO_TRACE;
    DSoftbusSessionClosed notice = std::get<DSoftbusSessionClosed>(event.event);
    context.eventMgr_.OnSoftbusSessionClosed(notice);
    context.inputDevMgr_.OnSoftbusSessionClosed(notice);
    context.mouseLocation_.OnSoftbusSessionClosed(notice);
    context.CloseDistributedFileConnection(notice.networkId);
    Transfer(context, event);
}

void StateMachine::OnSoftbusSessionOpened(Context &context, const CooperateEvent &event)
{
    CALL_INFO_TRACE;
    CHKPV(env_);
    DSoftbusSessionOpened notice = std::get<DSoftbusSessionOpened>(event.event);
    context.inputDevMgr_.OnSoftbusSessionOpened(notice);
    env_->GetDSoftbus().StartHeartBeat(notice.networkId);
    Transfer(context, event);
}

void StateMachine::OnHotPlugEvent(Context &context, const CooperateEvent &event)
{
    CALL_INFO_TRACE;
    InputHotplugEvent notice = std::get<InputHotplugEvent>(event.event);
    context.inputDevMgr_.OnLocalHotPlug(notice);
    Transfer(context, event);
}

void StateMachine::OnRemoteInputDevice(Context &context, const CooperateEvent &event)
{
    CALL_INFO_TRACE;
    DSoftbusSyncInputDevice notice = std::get<DSoftbusSyncInputDevice>(event.event);
    context.inputDevMgr_.OnRemoteInputDevice(notice);
    Transfer(context, event);
}

void StateMachine::OnRemoteHotPlug(Context &context, const CooperateEvent &event)
{
    CALL_INFO_TRACE;
    DSoftbusHotPlugEvent notice = std::get<DSoftbusHotPlugEvent>(event.event);
    context.inputDevMgr_.OnRemoteHotPlug(notice);
    Transfer(context, event);
}

void StateMachine::OnSoftbusSubscribeMouseLocation(Context &context, const CooperateEvent &event)
{
    CALL_INFO_TRACE;
    DSoftbusSubscribeMouseLocation notice = std::get<DSoftbusSubscribeMouseLocation>(event.event);
    context.mouseLocation_.OnSubscribeMouseLocation(notice);
}

void StateMachine::OnSoftbusUnSubscribeMouseLocation(Context &context, const CooperateEvent &event)
{
    CALL_INFO_TRACE;
    DSoftbusUnSubscribeMouseLocation notice = std::get<DSoftbusUnSubscribeMouseLocation>(event.event);
    context.mouseLocation_.OnUnSubscribeMouseLocation(notice);
}

void StateMachine::OnSoftbusReplySubscribeMouseLocation(Context &context, const CooperateEvent &event)
{
    CALL_INFO_TRACE;
    DSoftbusReplySubscribeMouseLocation notice = std::get<DSoftbusReplySubscribeMouseLocation>(event.event);
    context.mouseLocation_.OnReplySubscribeMouseLocation(notice);
}

void StateMachine::OnSoftbusReplyUnSubscribeMouseLocation(Context &context, const CooperateEvent &event)
{
    CALL_INFO_TRACE;
    DSoftbusReplyUnSubscribeMouseLocation notice = std::get<DSoftbusReplyUnSubscribeMouseLocation>(event.event);
    context.mouseLocation_.OnReplyUnSubscribeMouseLocation(notice);
}

void StateMachine::OnSoftbusMouseLocation(Context &context, const CooperateEvent &event)
{
    CALL_DEBUG_ENTER;
    DSoftbusSyncMouseLocation notice = std::get<DSoftbusSyncMouseLocation>(event.event);
    context.mouseLocation_.OnRemoteMouseLocation(notice);
}

void StateMachine::OnRemoteStart(Context &context, const CooperateEvent &event)
{
    CALL_DEBUG_ENTER;
    CHKPV(env_);
    DSoftbusStartCooperate startEvent = std::get<DSoftbusStartCooperate>(event.event);
    CooperateRadarInfo radarInfo {
        .funcName =  __FUNCTION__,
        .bizState = static_cast<int32_t> (BizState::STATE_END),
        .bizScene = static_cast<int32_t> (BizCooperateScene::SCENE_PASSIVE),
        .hostName = "",
        .localNetId = Utility::DFXRadarAnonymize(context.Local().c_str()),
        .peerNetId = Utility::DFXRadarAnonymize(startEvent.originNetworkId.c_str())
    };
    bool checkSameAccount = env_->GetDDM().CheckSameAccountToLocal(startEvent.originNetworkId);
    if (!checkSameAccount) {
        radarInfo.bizStage = static_cast<int32_t> (BizCooperateStage::STAGE_PASSIVE_CHECK_SAME_ACCOUNT);
        radarInfo.stageRes = static_cast<int32_t> (BizCooperateStageRes::RES_FAIL);
        radarInfo.errCode = static_cast<int32_t> (CooperateRadarErrCode::PASSIVE_CHECK_SAME_ACCOUNT_FAILED);
        CooperateRadar::ReportCooperateRadarInfo(radarInfo);
    }
    if (!isCooperateEnable_) {
        radarInfo.bizStage = static_cast<int32_t> (BizCooperateStage::STAGE_CHECK_PEER_SWITCH);
        radarInfo.stageRes = static_cast<int32_t> (BizCooperateStageRes::RES_FAIL);
        radarInfo.errCode = static_cast<int32_t> (CooperateRadarErrCode::CHECK_PEER_SWITCH_FAILED);
        CooperateRadar::ReportCooperateRadarInfo(radarInfo);
    }
    if (!checkSameAccount || !isCooperateEnable_) {
        FI_HILOGE("CheckSameAccountToLocal failed, switch is : %{public}d, unchain", isCooperateEnable_);
        CooperateEvent stopEvent(
            CooperateEventType::STOP,
            StopCooperateEvent {
                .isUnchained = true
            }
        );
        Transfer(context, stopEvent);
        return;
    }
    Transfer(context, event);
}

void StateMachine::OnRemoteStartWithOptions(Context &context, const CooperateEvent &event)
{
    CALL_DEBUG_ENTER;
    DSoftbusCooperateOptions startEvent = std::get<DSoftbusCooperateOptions>(event.event);
    CooperateRadarInfo radarInfo {
        .funcName =  __FUNCTION__,
        .bizState = static_cast<int32_t> (BizState::STATE_END),
        .bizScene = static_cast<int32_t> (BizCooperateScene::SCENE_PASSIVE),
        .hostName = "",
        .localNetId = Utility::DFXRadarAnonymize(context.Local().c_str()),
        .peerNetId = Utility::DFXRadarAnonymize(startEvent.originNetworkId.c_str())
    };
    bool checkSameAccount = env_->GetDDM().CheckSameAccountToLocal(startEvent.originNetworkId);
    if (!checkSameAccount) {
        radarInfo.bizStage = static_cast<int32_t> (BizCooperateStage::STAGE_PASSIVE_CHECK_SAME_ACCOUNT);
        radarInfo.stageRes = static_cast<int32_t> (BizCooperateStageRes::RES_FAIL);
        radarInfo.errCode = static_cast<int32_t> (CooperateRadarErrCode::PASSIVE_CHECK_SAME_ACCOUNT_FAILED);
        CooperateRadar::ReportCooperateRadarInfo(radarInfo);
    }
    if (!isCooperateEnable_) {
        radarInfo.bizStage = static_cast<int32_t> (BizCooperateStage::STAGE_CHECK_PEER_SWITCH);
        radarInfo.stageRes = static_cast<int32_t> (BizCooperateStageRes::RES_FAIL);
        radarInfo.errCode = static_cast<int32_t> (CooperateRadarErrCode::CHECK_PEER_SWITCH_FAILED);
        CooperateRadar::ReportCooperateRadarInfo(radarInfo);
    }
    if (!checkSameAccount || !isCooperateEnable_) {
        FI_HILOGE("CheckSameAccountToLocal failed, switch is : %{public}d, unchain", isCooperateEnable_);
        CooperateEvent stopEvent(
            CooperateEventType::STOP,
            StopCooperateEvent {
                .isUnchained = true
            }
        );
        Transfer(context, stopEvent);
        return;
    }
    Transfer(context, event);
}

void StateMachine::UpdateVirtualDeviceIdMap(Context &context, const CooperateEvent &event)
{
    CALL_DEBUG_ENTER;
    UpdateVirtualDeviceIdMapEvent notice = std::get<UpdateVirtualDeviceIdMapEvent>(event.event);
    context.inputEventBuilder_.UpdateVirtualDeviceIdMap(notice.remote2VirtualIds);
}

void StateMachine::Transfer(Context &context, const CooperateEvent &event)
{
    states_[current_]->OnEvent(context, event);
}

sptr<AppExecFwk::IAppMgr> StateMachine::GetAppMgr()
{
    CALL_INFO_TRACE;
    auto saMgr = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    CHKPP(saMgr);
    auto appMgrObj = saMgr->GetSystemAbility(APP_MGR_SERVICE_ID);
    CHKPP(appMgrObj);
    return iface_cast<AppExecFwk::IAppMgr>(appMgrObj);
}

int32_t StateMachine::RegisterApplicationStateObserver(Channel<CooperateEvent>::Sender sender,
    const EnableCooperateEvent &event)
{
    CALL_INFO_TRACE;
    auto bundleName = GetPackageName(event.tokenId);
    clientBundleNames_.push_back(bundleName);
    FI_HILOGI("Register application %{public}s state observer", bundleName.c_str());
    auto appMgr = GetAppMgr();
    CHKPR(appMgr, RET_ERR);
    appStateObserver_ = sptr<AppStateObserver>::MakeSptr(sender, event.pid);
    auto err = appMgr->RegisterApplicationStateObserver(appStateObserver_, clientBundleNames_);
    if (err != RET_OK) {
        appStateObserver_.clear();
        FI_HILOGE("IAppMgr::RegisterApplicationStateObserver fail, error:%{public}d", err);
        return RET_ERR;
    }
    return RET_OK;
}

void StateMachine::UnregisterApplicationStateObserver()
{
    CALL_INFO_TRACE;
    CHKPV(appStateObserver_);
    auto appMgr = GetAppMgr();
    CHKPV(appMgr);
    FI_HILOGI("Unregister application associateassistant state observer");
    auto err = appMgr->UnregisterApplicationStateObserver(appStateObserver_);
    if (err != RET_OK) {
        FI_HILOGE("IAppMgr::UnregisterApplicationStateObserver fail, error:%{public}d", err);
    }
    appStateObserver_.clear();
}

void StateMachine::UpdateApplicationStateObserver(int32_t clientPid)
{
    CALL_INFO_TRACE;
    CHKPV(appStateObserver_);
    appStateObserver_->UpdateClientPid(clientPid);
}

void StateMachine::AddSessionObserver(Context &context, const EnableCooperateEvent &event)
{
    CALL_INFO_TRACE;
    RegisterApplicationStateObserver(context.Sender(), event);
}

std::string StateMachine::GetPackageName(Security::AccessToken::AccessTokenID tokenId)
{
    CALL_INFO_TRACE;
    std::string bundleName {"Default"};
    int32_t tokenType = Security::AccessToken::AccessTokenKit::GetTokenTypeFlag(tokenId);
    switch (tokenType) {
        case Security::AccessToken::ATokenTypeEnum::TOKEN_HAP: {
            Security::AccessToken::HapTokenInfo hapInfo;
            if (Security::AccessToken::AccessTokenKit::GetHapTokenInfo(tokenId, hapInfo) != RET_OK) {
                FI_HILOGE("Get hap token info failed");
            } else {
                bundleName = hapInfo.bundleName;
            }
            break;
        }
        case Security::AccessToken::ATokenTypeEnum::TOKEN_NATIVE:
        case Security::AccessToken::ATokenTypeEnum::TOKEN_SHELL: {
            Security::AccessToken::NativeTokenInfo tokenInfo;
            if (Security::AccessToken::AccessTokenKit::GetNativeTokenInfo(tokenId, tokenInfo) != RET_OK) {
                FI_HILOGE("Get native token info failed");
            } else {
                bundleName = tokenInfo.processName;
            }
            break;
        }
        default: {
            FI_HILOGW("token type not match");
            break;
        }
    }
    return bundleName;
}

void StateMachine::RemoveSessionObserver(Context &context, const DisableCooperateEvent &event)
{
    UnregisterApplicationStateObserver();
}

void StateMachine::OnCommonEvent(Context &context, const std::string &commonEvent)
{
    FI_HILOGD("Current common event:%{public}s", commonEvent.c_str());
    CHKPV(env_);
    if (commonEvent == EventFwk::CommonEventSupport::COMMON_EVENT_SCREEN_ON ||
        commonEvent == EventFwk::CommonEventSupport::COMMON_EVENT_SCREEN_UNLOCKED) {
        if ((screenEventTimer_ >= 0) && (env_->GetTimerManager().IsExist(screenEventTimer_))) {
            env_->GetTimerManager().RemoveTimer(screenEventTimer_);
            screenEventTimer_ = -1;
        }
    }
    if (commonEvent == EventFwk::CommonEventSupport::COMMON_EVENT_SCREEN_OFF ||
        commonEvent == EventFwk::CommonEventSupport::COMMON_EVENT_SCREEN_LOCKED) {
        FI_HILOGD("Receive common event:%{public}s, stop cooperate", commonEvent.c_str());
        auto ret = context.Sender().Send(CooperateEvent(
            CooperateEventType::STOP,
            StopCooperateEvent{
                .isUnchained = false
            }));
        if (ret != Channel<CooperateEvent>::NO_ERROR) {
            FI_HILOGE("Failed to send event via channel, error:%{public}d", ret);
        }
        screenEventTimer_ = env_->GetTimerManager().AddTimer(SCREEN_LOCKED_TIMEOUT, REPEAT_ONCE,
            [sender = context.Sender(), this]() mutable {
                auto res = sender.Send(CooperateEvent(
                    CooperateEventType::STOP,
                    StopCooperateEvent{
                        .isUnchained = true
                    }));
                if (res != Channel<CooperateEvent>::NO_ERROR) {
                    FI_HILOGE("Failed to send event via channel, error:%{public}d", res);
                }
                screenEventTimer_ = -1;
            });
    }
}

void StateMachine::AddMonitor(Context &context)
{
    CALL_INFO_TRACE;
    if (monitorId_ >= 0) {
        return;
    }
    CHKPV(env_);
    monitorId_ = env_->GetInput().AddMonitor([&context, this] (
            std::shared_ptr<MMI::PointerEvent> pointerEvent) mutable {
            context.hotArea_.ProcessData(pointerEvent);
            context.mouseLocation_.ProcessData(pointerEvent);

            MMI::PointerEvent::PointerItem pointerItem;
            if (!pointerEvent->GetPointerItem(pointerEvent->GetPointerId(), pointerItem)) {
                FI_HILOGE("Corrupted pointer event");
                return;
            }
            auto pointerAction = pointerEvent->GetPointerAction();
            auto sourceType = pointerEvent->GetSourceType();
            if (pointerEvent->HasFlag(MMI::InputEvent::EVENT_FLAG_SIMULATE) &&
                (pointerAction == MMI::PointerEvent::POINTER_ACTION_PULL_IN_WINDOW ||
                pointerAction == MMI::PointerEvent::POINTER_ACTION_PULL_OUT_WINDOW)) {
                FI_HILOGW("PointerAction:%{public}d is simulated, skip", pointerAction);
                return;
            }
            auto ret = context.Sender().Send(CooperateEvent(
                CooperateEventType::INPUT_POINTER_EVENT,
                InputPointerEvent {
                    .deviceId = pointerEvent->GetDeviceId(),
                    .pointerAction = pointerAction,
                    .sourceType = sourceType,
                    .position = Coordinate {
                        .x = pointerItem.GetDisplayX(),
                        .y = pointerItem.GetDisplayY(),
                    },
                    .currentDisplayId = pointerEvent->GetTargetDisplayId()
                }));
            if (ret != Channel<CooperateEvent>::NO_ERROR) {
                FI_HILOGE("Failed to send event via channel, error:%{public}d", ret);
            }
        }, nullptr, MMI::HANDLE_EVENT_TYPE_MOUSE);
    if (monitorId_ < 0) {
        FI_HILOGE("MMI::Add Monitor fail");
    }
}

void StateMachine::AddPreMonitor(Context &context)
{
    CALL_INFO_TRACE;
    if (preMonitorId_ >= 0) {
        return;
    }
    CHKPV(env_);
    std::vector<int32_t> keys;
    keys.push_back(MMI::KeyEvent::KEYCODE_KEY_PEN_AIR_MOUSE);
    preMonitorId_ = env_->GetInput().AddPreMonitor(nullptr, [&context, this]
        (std::shared_ptr<MMI::KeyEvent> keyEvent) mutable {
            CHKPV(keyEvent);
            if (keyEvent->GetKeyCode() == MMI::KeyEvent::KEYCODE_KEY_PEN_AIR_MOUSE) {
                if (keyEvent->GetKeyAction() == MMI::KeyEvent::KEY_ACTION_DOWN) {
                    FI_HILOGI("Air mouse key down");
                    env_->GetDeviceManager().SetPencilAirMouse(true);
                } else {
                    FI_HILOGI("Air mouse key up or cancel or unknow");
                    env_->GetDeviceManager().SetPencilAirMouse(false);
                }
            }
        }, MMI::HANDLE_EVENT_TYPE_PRE_KEY, keys);
    if (preMonitorId_ < 0) {
        FI_HILOGE("MMI::Add Monitor fail");
    }
}

void StateMachine::RemoveMonitor(Context &context)
{
    CALL_INFO_TRACE;
    CHKPV(env_);
    if (monitorId_ < 0) {
        return;
    }
    env_->GetInput().RemoveMonitor(monitorId_);
    monitorId_ = -1;
}

void StateMachine::RemovePreMonitor(Context &context)
{
    CALL_INFO_TRACE;
    CHKPV(env_);
    if (preMonitorId_ < 0) {
        return;
    }
    env_->GetInput().RemovePreMonitor(preMonitorId_);
    preMonitorId_ = -1;
}

void StateMachine::RemoveWatches(Context &context)
{
    CALL_INFO_TRACE;
    for (auto iter = onlineBoards_.begin();
         iter != onlineBoards_.end(); iter = onlineBoards_.begin()) {
        FI_HILOGD("Remove watch \'%{public}s\'", Utility::Anonymize(*iter).c_str());
        onlineBoards_.erase(iter);
    }
}
bool StateMachine::IsCooperateEnable()
{
    return isCooperateEnable_;
}

void StateMachine::ResetCooperate(Context &context)
{
    CALL_INFO_TRACE;
    auto ret = context.Sender().Send(CooperateEvent(
        CooperateEventType::STOP_ABOUT_VIRTUALTRACKPAD,
        StopCooperateEvent {}));
    if (ret != Channel<CooperateEvent>::NO_ERROR) {
        FI_HILOGE("Failed to send event via channel, error:%{public}d", ret);
    }
}

bool StateMachine::CheckIsVirtualTrackpad(int32_t deviceId)
{
    CALL_INFO_TRACE;
    bool isLocalPointerDevice = false;
    MMI::InputManager::GetInstance()->GetDevice(deviceId, [&isLocalPointerDevice, this] (
        std::shared_ptr<MMI::InputDevice> device) -> bool {
            CHKPR(device, false);
            if (device->GetName() == VIRTUAL_TRACK_PAD_NAME) {
                isLocalPointerDevice = true;
                FI_HILOGI("Has virtualTrackpad");
                return isLocalPointerDevice;
            }
            return isLocalPointerDevice;
        });
    return isLocalPointerDevice;
}

} // namespace Cooperate
} // namespace DeviceStatus
} // namespace Msdp
} // namespace OHOS
