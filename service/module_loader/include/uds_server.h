/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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
#ifndef OHOS_UDS_SERVER_H
#define OHOS_UDS_SERVER_H

#include <map>
#include <mutex>
#include <thread>
#include <functional>
#include <list>

#include "i_uds_server.h"
#include "uds_session.h"
#include "uds_socket.h"

namespace OHOS {
namespace MMI {
enum EpollEventType {
    EPOLL_EVENT_BEGIN = 0,
    EPOLL_EVENT_INPUT = EPOLL_EVENT_BEGIN,
    EPOLL_EVENT_SOCKET,

    EPOLL_EVENT_END,
};

struct StreamBufData {
    bool isOverflow = false;
    OHOS::MMI::StreamBuffer sBuf;
};
using MsgServerFunCallback = std::function<void(SessionPtr, NetPacket&)>;
class UDSServer : public UDSSocket, public IUdsServer {
public:
    UDSServer();
    virtual ~UDSServer();
    void UdsStop();
    bool SendMsg(int32_t fd, NetPacket& pkt);
    void Broadcast(NetPacket& pkt);
    void Multicast(const std::vector<int32_t>& fdList, NetPacket& pkt);
    void Dump(int32_t fd);
    int32_t GetFdByPid(int32_t pid);
    int32_t GetPidByFd(int32_t fd);
    void OnEpollEvent(epoll_event& ev, std::map<int32_t, StreamBufData>& bufMap);
    void OnEpollRecv(int32_t fd, const char *buf, size_t size);

    void AddSessionDeletedCallback(std::function<void(SessionPtr)> callback);

public:
    virtual int32_t AddSocketPairInfo(const std::string& programName, const int moduleType, int& serverFd,
                                      int& toReturnClientFd, const int32_t uid, const int32_t pid);
    SessionPtr GetSession(int32_t fd) const
    {
        auto it = sessionsMap_.find(fd);
        if (it == sessionsMap_.end()) {
            return nullptr;
        }
        return it->second->GetPtr();
    }

protected:
    void SetRecvFun(MsgServerFunCallback fun);

    virtual void OnConnected(SessionPtr s);
    virtual void OnDisconnected(SessionPtr s);
    virtual int32_t EpollCtlAdd(EpollEventType type, int32_t fd);

    bool StartServer();
    void OnRecv(int32_t fd, const char *buf, size_t size);
    void OnEvent(const epoll_event& ev, std::map<int32_t, StreamBufData>& bufMap);
    void OnThread();
#ifdef OHOS_BUILD_MMI_DEBUG
    virtual void HandleCommandQueue();
#endif // OHOS_BUILD_MMI_DEBUG

    bool AddSession(SessionPtr ses);
    void DelSession(int32_t fd);
    void DumpSession(const std::string& title);
    bool ClearDeadSessionInMap(const int serverFd, const int clientFd);

    void NotifySessionDeleted(SessionPtr ses);

protected:
    std::mutex mux_;
    std::thread t_;
    bool isRun_ = false;
    MsgServerFunCallback recvFun_ = nullptr;
    std::map<int32_t, SessionPtr> sessionsMap_ = {};
    std::map<int32_t, int32_t> idxPidMap_ = {};
    std::list<std::function<void(SessionPtr)>> callbacks_;
};
}
}
#endif
