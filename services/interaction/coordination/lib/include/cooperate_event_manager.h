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

#ifndef COOPERATE_EVENT_MANAGER_H
#define COOPERATE_EVENT_MANAGER_H

#include <list>
#include <mutex>
#include <string>

#include "nocopyable.h"
#include "refbase.h"
#include "singleton.h"
#include "uds_session.h"

#include "fi_log.h"
#include "i_context.h"

namespace OHOS {
namespace Msdp {
namespace DeviceStatus {
enum class CooperationMessage {
    OPEN_SUCCESS = 100,
    OPEN_FAIL = 101,
    INFO_START = 200,
    INFO_SUCCESS = 201,
    INFO_FAIL = 202,
    CLOSE = 300,
    CLOSE_SUCCESS = 301,
    STOP = 400,
    STOP_SUCCESS = 401,
    STOP_FAIL = 402,
    STATE_ON = 500,
    STATE_OFF = 501,
    INPUT_DEVICE_ID_ERROR = 4400001,
    COOPERATE_FAIL = 4400002,
    COOPERATION_DEVICE_ERROR = 4400003,
};

class CooperateEventManager final {
    DECLARE_DELAYED_SINGLETON(CooperateEventManager);
public:
    DISALLOW_COPY_AND_MOVE(CooperateEventManager);

    enum EventType { LISTENER, ENABLE, START, STOP, STATE };

    struct EventInfo : public RefBase {
        EventType type;
        SessionPtr sess;
        MmiMessageId msgId;
        int32_t userData;
        std::string deviceId;
        CooperationMessage msg;
        bool state;
    };

    void AddCooperationEvent(sptr<EventInfo> event);
    void RemoveCooperationEvent(sptr<EventInfo> event);
    int32_t OnCooperateMessage(CooperationMessage msg, const std::string &deviceId = "");
    void OnEnable(CooperationMessage msg, const std::string &deviceId = "");
    void OnStart(CooperationMessage msg, const std::string &deviceId = "");
    void OnStop(CooperationMessage msg, const std::string &deviceId = "");
    void OnGetState(bool state);
    void OnErrorMessage(EventType type, CooperationMessage msg);

    void SetIContext(IContext *context);
    IContext* GetIContext() const;

private:
    void NotifyCooperateMessage(SessionPtr sess, MmiMessageId msgId, int32_t userData,
        const std::string &deviceId, CooperationMessage msg);
    void NotifyCooperateState(SessionPtr sess, MmiMessageId msgId, int32_t userData, bool state);

private:
    std::mutex lock_;
    std::list<sptr<EventInfo>> remoteCooperateCallbacks_;
    std::map<EventType, sptr<EventInfo>> cooperateCallbacks_ {
        {EventType::ENABLE, nullptr},
        {EventType::START, nullptr},
        {EventType::STOP, nullptr},
        {EventType::STATE, nullptr}
    };
    IContext *context_ { nullptr };
};

#define CooperateEventMgr ::OHOS::DelayedSingleton<CooperateEventManager>::GetInstance()
} // namespace DeviceStatus
} // namespace Msdp
} // namespace OHOS
#endif // COOPERATE_EVENT_MANAGER_H
