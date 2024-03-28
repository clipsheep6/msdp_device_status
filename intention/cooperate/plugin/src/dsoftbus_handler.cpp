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

#include "dsoftbus_handler.h"

#include "devicestatus_define.h"
#include "ipc_skeleton.h"
#include "token_setproc.h"
#include "utility.h"

#undef LOG_TAG
#define LOG_TAG "DSoftbusHandler"

namespace OHOS {
namespace Msdp {
namespace DeviceStatus {
namespace Cooperate {

DSoftbusHandler::DSoftbusHandler(IContext *env)
    : env_(env)
{
    observer_ = std::make_shared<DSoftbusObserver>(*this);
    env_->GetDSoftbus().AddObserver(observer_);
}

DSoftbusHandler::~DSoftbusHandler()
{
    env_->GetDSoftbus().RemoveObserver(observer_);
}

void DSoftbusHandler::AttachSender(Channel<CooperateEvent>::Sender sender)
{
    CALL_DEBUG_ENTER;
    std::lock_guard guard(lock_);
    sender_ = sender;
}

int32_t DSoftbusHandler::OpenSession(const std::string &networkId)
{
    CALL_INFO_TRACE;
    uint64_t tokenId = IPCSkeleton::GetCallingTokenID();
    SetFirstCallerTokenID(tokenId);
    return env_->GetDSoftbus().OpenSession(networkId);
}

void DSoftbusHandler::CloseSession(const std::string &networkId)
{
    CALL_INFO_TRACE;
    env_->GetDSoftbus().CloseSession(networkId);
}

void DSoftbusHandler::CloseAllSessions()
{
    CALL_INFO_TRACE;
    env_->GetDSoftbus().CloseAllSessions();
}

int32_t DSoftbusHandler::StartCooperate(const std::string &networkId, const DSoftbusStartCooperate &event)
{
    CALL_INFO_TRACE;
    NetPacket packet(MessageId::DSOFTBUS_START_COOPERATE);
    int32_t ret = env_->GetDSoftbus().SendPacket(networkId, packet);
    if (ret != RET_OK) {
        OnCommunicationFailure(networkId);
    }
    return ret;
}

int32_t DSoftbusHandler::StartCooperateResponse(const std::string &networkId,
    const DSoftbusStartCooperateResponse &event)
{
    CALL_INFO_TRACE;
    NetPacket packet(MessageId::DSOFTBUS_START_COOPERATE_RESPONSE);
    packet << event.normal;
    if (packet.ChkRWError()) {
        FI_HILOGE("Failed to write data packet");
        return RET_ERR;
    }
    int32_t ret = env_->GetDSoftbus().SendPacket(networkId, packet);
    if (ret != RET_OK) {
        OnCommunicationFailure(networkId);
    }
    return ret;
}

int32_t DSoftbusHandler::StartCooperateFinish(const std::string &networkId,
    const DSoftbusStartCooperateFinished &event)
{
    CALL_INFO_TRACE;
    NetPacket packet(MessageId::DSOFTBUS_START_COOPERATE_FINISHED);
    packet << event.originNetworkId << event.cursorPos.x
        << event.cursorPos.y << event.success;
    if (packet.ChkRWError()) {
        FI_HILOGE("Failed to write data packet");
        return RET_ERR;
    }
    int32_t ret = env_->GetDSoftbus().SendPacket(networkId, packet);
    if (ret != RET_OK) {
        OnCommunicationFailure(networkId);
    }
    return ret;
}

int32_t DSoftbusHandler::StopCooperate(const std::string &networkId, const DSoftbusStopCooperate &event)
{
    CALL_INFO_TRACE;
    NetPacket packet(MessageId::DSOFTBUS_STOP_COOPERATE);
    int32_t ret = env_->GetDSoftbus().SendPacket(networkId, packet);
    if (ret != RET_OK) {
        OnCommunicationFailure(networkId);
    }
    return ret;
}

int32_t DSoftbusHandler::ComeBack(const std::string &networkId, const DSoftbusComeBack &event)
{
    CALL_INFO_TRACE;
    NetPacket packet(MessageId::DSOFTBUS_COME_BACK);
    packet << event.originNetworkId << event.cursorPos.x << event.cursorPos.y;
    if (packet.ChkRWError()) {
        FI_HILOGE("Failed to write data packet");
        return RET_ERR;
    }
    int32_t ret = env_->GetDSoftbus().SendPacket(networkId, packet);
    if (ret != RET_OK) {
        OnCommunicationFailure(networkId);
    }
    return ret;
}

int32_t DSoftbusHandler::RelayCooperate(const std::string &networkId, const DSoftbusRelayCooperate &event)
{
    CALL_INFO_TRACE;
    NetPacket packet(MessageId::DSOFTBUS_RELAY_COOPERATE);
    packet << event.targetNetworkId;
    if (packet.ChkRWError()) {
        FI_HILOGE("Failed to write data packet");
        return RET_ERR;
    }
    int32_t ret = env_->GetDSoftbus().SendPacket(networkId, packet);
    if (ret != RET_OK) {
        OnCommunicationFailure(networkId);
    }
    return ret;
}

int32_t DSoftbusHandler::RelayCooperateFinish(const std::string &networkId, const DSoftbusRelayCooperateFinished &event)
{
    CALL_INFO_TRACE;
    NetPacket packet(MessageId::DSOFTBUS_RELAY_COOPERATE_FINISHED);
    packet << event.targetNetworkId << event.normal;
    if (packet.ChkRWError()) {
        FI_HILOGE("Failed to write data packet");
        return RET_ERR;
    }
    int32_t ret = env_->GetDSoftbus().SendPacket(networkId, packet);
    if (ret != RET_OK) {
        OnCommunicationFailure(networkId);
    }
    return ret;
}

std::string DSoftbusHandler::GetLocalNetworkId()
{
    return IDSoftbusAdapter::GetLocalNetworkId();
}

void DSoftbusHandler::OnBind(const std::string &networkId)
{
    FI_HILOGI("Bind to \'%{public}s\'", Utility::Anonymize(networkId));
    SendEvent(CooperateEvent(
        CooperateEventType::DSOFTBUS_SESSION_OPEND,
        DSoftbusSessionOpened {
            .networkId = networkId
        }));
}

void DSoftbusHandler::OnShutdown(const std::string &networkId)
{
    FI_HILOGI("Connection with \'%{public}s\' shutdown", Utility::Anonymize(networkId));
    SendEvent(CooperateEvent(
        CooperateEventType::DSOFTBUS_SESSION_CLOSED,
        DSoftbusSessionClosed {
            .networkId = networkId
        }));
}

bool DSoftbusHandler::OnPacket(const std::string &networkId, NetPacket &packet)
{
    CALL_DEBUG_ENTER;
    switch (packet.GetMsgId()) {
        case MessageId::DSOFTBUS_START_COOPERATE: {
            OnStartCooperate(networkId, packet);
            break;
        }
        case MessageId::DSOFTBUS_START_COOPERATE_RESPONSE: {
            OnStartCooperateResponse(networkId, packet);
            break;
        }
        case MessageId::DSOFTBUS_START_COOPERATE_FINISHED: {
            OnStartCooperateFinish(networkId, packet);
            break;
        }
        case MessageId::DSOFTBUS_STOP_COOPERATE: {
            OnStopCooperate(networkId, packet);
            break;
        }
        case MessageId::DSOFTBUS_COME_BACK: {
            OnComeBack(networkId, packet);
            break;
        }
        case MessageId::DSOFTBUS_RELAY_COOPERATE: {
            OnRelayCooperate(networkId, packet);
            break;
        }
        case MessageId::DSOFTBUS_RELAY_COOPERATE_FINISHED: {
            OnRelayCooperateFinish(networkId, packet);
            break;
        }
        default: {
            return false;
        }
    }
    return true;
}

void DSoftbusHandler::SendEvent(const CooperateEvent &event)
{
    std::lock_guard guard(lock_);
    sender_.Send(event);
}

void DSoftbusHandler::OnCommunicationFailure(const std::string &networkId)
{
    env_->GetDSoftbus().CloseSession(networkId);
    FI_HILOGI("Notify communication failure with peer(%{public}s)", Utility::Anonymize(networkId));
    SendEvent(CooperateEvent(
        CooperateEventType::DSOFTBUS_SESSION_CLOSED,
        DSoftbusSessionClosed {
            .networkId = networkId
        }));
}

void DSoftbusHandler::OnStartCooperate(const std::string &networkId, NetPacket &packet)
{
    CALL_INFO_TRACE;
    DSoftbusStartCooperate event {
        .networkId = networkId,
        .normal = true,
    };
    SendEvent(CooperateEvent(
        CooperateEventType::DSOFTBUS_START_COOPERATE,
        event));
}

void DSoftbusHandler::OnStartCooperateResponse(const std::string &networkId, NetPacket &packet)
{
    CALL_INFO_TRACE;
    DSoftbusStartCooperateResponse event {
        .networkId = networkId,
    };
    packet >> event.normal;
    if (packet.ChkRWError()) {
        FI_HILOGE("Failed to read data packet");
        return;
    }
    SendEvent(CooperateEvent(
        CooperateEventType::DSOFTBUS_START_COOPERATE_RESPONSE,
        event));
}

void DSoftbusHandler::OnStartCooperateFinish(const std::string &networkId, NetPacket &packet)
{
    CALL_INFO_TRACE;
    DSoftbusStartCooperateFinished event {
        .networkId = networkId,
    };
    packet >> event.originNetworkId >> event.cursorPos.x
        >> event.cursorPos.y >> event.success;
    if (packet.ChkRWError()) {
        FI_HILOGE("Failed to read data packet");
        return;
    }
    SendEvent(CooperateEvent(
        CooperateEventType::DSOFTBUS_START_COOPERATE_FINISHED,
        event));
}

void DSoftbusHandler::OnStopCooperate(const std::string &networkId, NetPacket &packet)
{
    CALL_INFO_TRACE;
    DSoftbusStopCooperate event {
        .networkId = networkId,
        .normal = true,
    };
    SendEvent(CooperateEvent(
        CooperateEventType::DSOFTBUS_STOP_COOPERATE,
        event));
}

void DSoftbusHandler::OnComeBack(const std::string &networkId, NetPacket &packet)
{
    CALL_INFO_TRACE;
    DSoftbusComeBack event {
        .networkId = networkId,
        .success = true,
    };
    packet >> event.originNetworkId >> event.cursorPos.x >> event.cursorPos.y;
    if (packet.ChkRWError()) {
        FI_HILOGE("Failed to read data packet");
        return;
    }
    SendEvent(CooperateEvent(
        CooperateEventType::DSOFTBUS_COME_BACK,
        event));
}

void DSoftbusHandler::OnRelayCooperate(const std::string &networkId, NetPacket &packet)
{
    CALL_INFO_TRACE;
    DSoftbusRelayCooperate event {
        .networkId = networkId,
        .normal = true,
    };
    packet >> event.targetNetworkId;
    if (packet.ChkRWError()) {
        FI_HILOGE("Failed to read data packet");
        return;
    }
    SendEvent(CooperateEvent(
        CooperateEventType::DSOFTBUS_RELAY_COOPERATE,
        event));
}

void DSoftbusHandler::OnRelayCooperateFinish(const std::string &networkId, NetPacket &packet)
{
    CALL_INFO_TRACE;
    DSoftbusRelayCooperate event {
        .networkId = networkId,
    };
    packet >> event.targetNetworkId >> event.normal;
    if (packet.ChkRWError()) {
        FI_HILOGE("Failed to read data packet");
        return;
    }
    SendEvent(CooperateEvent(
        CooperateEventType::DSOFTBUS_RELAY_COOPERATE_FINISHED,
        event));
}
} // namespace Cooperate
} // namespace DeviceStatus
} // namespace Msdp
} // namespace OHOS
