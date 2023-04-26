/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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
#ifndef STREAM_SESSION_H
#define STREAM_SESSION_H

#include <list>
#include <memory>
#include <map>

#include <sys/socket.h>
#include <sys/un.h>

#include "nocopyable.h"

#include "net_packet.h"
#include "proto.h"
#include "rust_binding.h"

namespace OHOS {
namespace Msdp {
class StreamSession;
using SessionPtr = std::shared_ptr<StreamSession>;
class StreamSession : public std::enable_shared_from_this<StreamSession> {
public:
    StreamSession(const std::string &programName, const int32_t moduleType, const int32_t fd, const int32_t uid,
               const int32_t pid);
    DISALLOW_COPY_AND_MOVE(StreamSession);
    virtual ~StreamSession() = default;

    bool SendMsg(const char *buf, size_t size) const;
    bool SendMsg(NetPacket &pkt) const;
    void Close();

    int32_t GetUid() const
    {
        return get_uid(&rustStreamSession_);
    }

    int32_t GetPid() const
    {
        return get_pid(&rustStreamSession_);
    }

    int32_t GetModuleType() const
    {
        return get_module_type(&rustStreamSession_);
    }

    SessionPtr GetSharedPtr()
    {
        return shared_from_this();
    }

    int32_t GetFd() const
    {
        return get_session_fd(&rustStreamSession_);
    }

    const std::string& GetDescript() const
    {
        return descript_;
    }

    const std::string GetProgramName() const
    {
        return programName_;
    }

    void SetTokenType(int32_t type)
    {
        set_token_type(&rustStreamSession_, type);
    }

    int32_t GetTokenType() const
    {
        return get_token_type(&rustStreamSession_);
    }

    void UpdateDescript();
protected:
    struct EventTime {
        int32_t id { 0 };
        int64_t eventTime { 0 };
        int32_t timerId { -1 };
    };
    struct RustStreamSession rustStreamSession_;
    std::map<int32_t, std::vector<EventTime>> events_;
    std::string descript_;
    const std::string programName_;
};
} // namespace Msdp
} // namespace OHOS
#endif // STREAM_SESSION_H