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

#include "cooperate_event_manager.h"
#include "devicestatus_hilog_wrapper.h"
#include "devicestatus_define.h"

namespace OHOS {
namespace Msdp {
namespace DeviceStatus {
namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, MSDP_DOMAIN_ID, "CooperateEventManager" };
} // namespace

CooperateEventManager::CooperateEventManager() {}
CooperateEventManager::~CooperateEventManager() {}

void CooperateEventManager::AddCooperationEvent(sptr<EventInfo> event)
{
    CALL_DEBUG_ENTER;
    std::lock_guard<std::mutex> guard(lock_);
    if (event->type == EventType::LISTENER) {
        remoteCooperateCallbacks_.emplace_back(event);
    } else {
        cooperateCallbacks_[event->type] = event;
    }
}

void CooperateEventManager::RemoveCooperationEvent(sptr<EventInfo> event)
{
    CALL_DEBUG_ENTER;
    if (remoteCooperateCallbacks_.empty() || event == nullptr) {
        FI_HILOGE("Remove listener failed");
        return;
    }
    for (auto it = remoteCooperateCallbacks_.begin(); it != remoteCooperateCallbacks_.end(); ++it) {
        if ((*it)->sess == event->sess) {
            remoteCooperateCallbacks_.erase(it);
            return;
        }
    }
}

int32_t CooperateEventManager::OnCooperateMessage(CooperationMessage msg, const std::string &deviceId)
{
    CALL_DEBUG_ENTER;
    std::lock_guard<std::mutex> guard(lock_);
    if (remoteCooperateCallbacks_.empty()) {
        FI_HILOGW("The current listener is empty, unable to invoke the listening interface");
        return RET_ERR;
    }
    for (auto it = remoteCooperateCallbacks_.begin(); it != remoteCooperateCallbacks_.end(); ++it) {
        sptr<EventInfo> info = *it;
        CHKPC(info);
        NotifyCooperateMessage(info->sess, info->msgId, info->userData, deviceId, msg);
    }
    return RET_OK;
}

void CooperateEventManager::OnEnable(CooperationMessage msg, const std::string &deviceId)
{
    CALL_DEBUG_ENTER;
    std::lock_guard<std::mutex> guard(lock_);
    sptr<EventInfo> info = cooperateCallbacks_[EventType::ENABLE];
    CHKPV(info);
    NotifyCooperateMessage(info->sess, info->msgId, info->userData, deviceId, msg);
    cooperateCallbacks_[EventType::ENABLE] =  nullptr;
}

void CooperateEventManager::OnStart(CooperationMessage msg, const std::string &deviceId)
{
    CALL_DEBUG_ENTER;
    std::lock_guard<std::mutex> guard(lock_);
    sptr<EventInfo> info = cooperateCallbacks_[EventType::START];
    CHKPV(info);
    NotifyCooperateMessage(info->sess, info->msgId, info->userData, deviceId, msg);
    cooperateCallbacks_[EventType::START] =  nullptr;
}

void CooperateEventManager::OnStop(CooperationMessage msg, const std::string &deviceId)
{
    CALL_DEBUG_ENTER;
    std::lock_guard<std::mutex> guard(lock_);
    sptr<EventInfo> info = cooperateCallbacks_[EventType::STOP];
    CHKPV(info);
    NotifyCooperateMessage(info->sess, info->msgId, info->userData, deviceId, msg);
    cooperateCallbacks_[EventType::STOP] =  nullptr;
}

void CooperateEventManager::OnGetState(bool state)
{
    CALL_DEBUG_ENTER;
    std::lock_guard<std::mutex> guard(lock_);
    sptr<EventInfo> info = cooperateCallbacks_[EventType::STATE];
    CHKPV(info);
    NotifyCooperateState(info->sess, info->msgId, info->userData, state);
    cooperateCallbacks_[EventType::STATE] =  nullptr;
}

void CooperateEventManager::OnErrorMessage(EventType type, CooperationMessage msg)
{
    std::lock_guard<std::mutex> guard(lock_);
    sptr<EventInfo> info = cooperateCallbacks_[type];
    CHKPV(info);
    NotifyCooperateMessage(info->sess, info->msgId, info->userData, "", msg);
    cooperateCallbacks_[type] =  nullptr;
}

void CooperateEventManager::SetIContext(IContext *context)
{
    context_ = context;
}

IContext* CooperateEventManager::GetIContext() const
{
    return context_;
}

void CooperateEventManager::NotifyCooperateMessage(
    SessionPtr sess, MessageId msgId, int32_t userData, const std::string &deviceId, CooperationMessage msg)
{
    CALL_DEBUG_ENTER;
    CHKPV(sess);
    NetPacket pkt(msgId);
    pkt << userData << deviceId << static_cast<int32_t>(msg);
    if (pkt.ChkRWError()) {
        FI_HILOGE("Packet write data failed");
        return;
    }
    if (!sess->SendMsg(pkt)) {
        FI_HILOGE("Sending failed");
        return;
    }
}

void CooperateEventManager::NotifyCooperateState(SessionPtr sess, MessageId msgId, int32_t userData, bool state)
{
    CALL_DEBUG_ENTER;
    CHKPV(sess);
    NetPacket pkt(msgId);
    pkt << userData << state;
    if (pkt.ChkRWError()) {
        FI_HILOGE("Packet write data failed");
        return;
    }
    if (!sess->SendMsg(pkt)) {
        FI_HILOGE("Sending failed");
        return;
    }
}
} // namespace DeviceStatus
} // namespace Msdp
} // namespace OHOS
