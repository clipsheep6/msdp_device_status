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
#ifndef STREAM_SOCKET_H
#define STREAM_SOCKET_H

#include <atomic>
#include <string>
#include <functional>

#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#include "nocopyable.h"

#include "circle_stream_buffer.h"
#include "net_packet.h"

namespace OHOS {
namespace Msdp {
namespace DeviceStatus {
class StreamSocket {
public:
    StreamSocket();
    DISALLOW_COPY_AND_MOVE(StreamSocket);
    virtual ~StreamSocket();
    using PacketCallBackFun = std::function<void(NetPacket&)>;

    void OnReadPackets(CircleStreamBuffer &buf, PacketCallBackFun callbackFun);
    void Close();

    int32_t GetFd() const
    {
        return fd_;
    }

protected:
    int32_t fd_ { -1 };
};
} // namespace DeviceStatus
} // namespace Msdp
} // namespace OHOS
#endif // STREAM_SOCKET_H