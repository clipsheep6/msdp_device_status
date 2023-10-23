/*
 * Copyright (c) 2022-2023 Huawei Device Co., Ltd.
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

#ifndef COOPERATE_EVENT_MANAGER_H
#define COOPERATE_EVENT_MANAGER_H

#include <list>
#include <mutex>
#include <string>

#include "nocopyable.h"
#include "refbase.h"
#include "singleton.h"
#include "stream_session.h"

#include "cooperate_message.h"
#include "fi_log.h"
#include "i_context.h"

namespace OHOS {
namespace Msdp {
namespace DeviceStatus {
class CooperateEventManager final {
    DECLARE_DELAYED_SINGLETON(CooperateEventManager);
public:
    enum EventType { LISTENER, ENABLE, START, STOP, STATE };
    struct EventInfo : public RefBase {
        EventType type { LISTENER };
        SessionPtr sess { nullptr };
        MessageId msgId { MessageId::INVALID };
        int32_t userData { -1 };
        std::string deviceId;
        CooperateMessage msg { CooperateMessage::PREPARE };
        bool state { false };
    };

    DISALLOW_COPY_AND_MOVE(CooperateEventManager);

    void AddCooperateEvent(sptr<EventInfo> event);
    void RemoveCooperateEvent(sptr<EventInfo> event);
    int32_t OnCooperateMessage(CooperateMessage msg, const std::string &deviceId = "");
    void OnEnable(CooperateMessage msg, const std::string &deviceId = "");
    void OnStart(CooperateMessage msg, const std::string &deviceId = "");
    void OnStop(CooperateMessage msg, const std::string &deviceId = "");
    void OnGetCrossingSwitchState(bool state);
    void OnErrorMessage(EventType type, CooperateMessage msg);
    void SetIContext(IContext *context);
    IContext* GetIContext() const;

private:
    void NotifyCooperateMessage(SessionPtr sess, MessageId msgId, int32_t userData,
        const std::string &deviceId, CooperateMessage msg);
    void NotifyCooperateState(SessionPtr sess, MessageId msgId, int32_t userData, bool state);

private:
    std::mutex lock_;
    std::list<sptr<EventInfo>> remoteCooperateCallbacks_;
    std::map<EventType, sptr<EventInfo>> cooperateCallbacks_{
        {EventType::ENABLE, nullptr},
        {EventType::START, nullptr},
        {EventType::STOP, nullptr},
        {EventType::STATE, nullptr}
    };
    IContext *context_ { nullptr };
};

#define COOR_EVENT_MGR OHOS::DelayedSingleton<CooperateEventManager>::GetInstance()
} // namespace DeviceStatus
} // namespace Msdp
} // namespace OHOS
#endif // COOPERATE_EVENT_MANAGER_H
