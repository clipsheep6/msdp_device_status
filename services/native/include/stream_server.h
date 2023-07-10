/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
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
#ifndef STREAM_SERVER_H
#define STREAM_SERVER_H

#include <functional>
#include <list>
#include <map>
#include <mutex>

#include "nocopyable.h"

#include "circle_stream_buffer.h"
#include "epoll_manager.h"
#include "i_stream_server.h"
#include "stream_socket.h"

namespace OHOS {
namespace Msdp {
namespace DeviceStatus {
using MsgServerFunCallback = std::function<void(SessionPtr, NetPacket&)>;
class StreamServer : public StreamSocket, 
                     public IStreamServer,
                     public IEpollEventSource {
public:
    StreamServer() = default;
    DISALLOW_COPY_AND_MOVE(StreamServer);
    virtual ~StreamServer();
    void UdsStop();
    bool SendMsg(int32_t fd, NetPacket& pkt);
    void Multicast(const std::vector<int32_t>& fdList, NetPacket& pkt);
    int32_t GetClientFd(int32_t pid) const;
    int32_t GetClientPid(int32_t fd) const;
    void AddSessionDeletedCallback(int32_t pid, std::function<void(SessionPtr)> callback);
    int32_t AddSocketPairInfo(const std::string& programName, int32_t moduleType, int32_t uid, int32_t pid,
        int32_t& serverFd, int32_t& toReturnClientFd, int32_t& tokenType) override;

    SessionPtr GetSession(int32_t fd) const;
    SessionPtr GetSessionByPid(int32_t pid) const override;
    void Dispatch(const struct epoll_event &ev) override;
    int32_t GetFd() const override
    {
        return epollManager_.GetFd();
    }

protected:
    virtual void OnConnected(SessionPtr s);
    virtual void OnDisconnected(SessionPtr s);

    void SetRecvFun(MsgServerFunCallback fun);
    void ReleaseSession(int32_t fd, struct epoll_event& ev);
    void OnPacket(int32_t fd, NetPacket& pkt);
    void OnEpollRecv(int32_t fd, struct epoll_event& ev);
    bool AddSession(SessionPtr ses);
    void DelSession(int32_t fd);
    void DumpSession(const std::string& title);
    void NotifySessionDeleted(SessionPtr ses);

protected:
    MsgServerFunCallback recvFun_ { nullptr };
    std::map<int32_t, SessionPtr> sessionsMap_;
    std::map<int32_t, int32_t> idxPidMap_;
    std::map<int32_t, CircleStreamBuffer> circleBufMap_;
    std::map<int32_t, std::function<void(SessionPtr)>> callbacks_;
    EpollManager epollManager_;
};
} // namespace DeviceStatus
} // namespace Msdp
} // namespace OHOS
#endif // STREAM_SERVER_H