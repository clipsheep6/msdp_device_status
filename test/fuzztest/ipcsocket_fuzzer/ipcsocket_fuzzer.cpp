/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "ipcsocket_fuzzer.h"
111
#include "singleton.h"

#include "devicestatus_define.h"
#include "i_tunnel_client.h"
#include "i_plugin.h"
#include "socket_client.h"
#include "socket_session_manager.h"
#include "socket_session.h"
#include "socket_connection.h"
#include "socket_params.h"
#include "stream_client.h"
#include "tunnel_client.h"

#include "message_parcel.h"

#undef LOG_TAG
#define LOG_TAG "IpcSocketFuzzTest"
namespace OHOS {
namespace Msdp {
namespace DeviceStatus {
const std::u16string FORMMGR_INTERFACE_TOKEN { u"ohos.msdp.Idevicestatus" };
inline constexpr int32_t MAX_EVENT_SIZE { 100 };

namespace OHOS {

bool SocketClientFuzzTest(const uint8_t* data, size_t size)
{
    std::shared_ptr<ITunnelClient> tunnel = std::make_shared<TunnelClient>();
    SocketClient socketClient(tunnel);
    auto callback = [](const StreamClient &client, NetPacket &pkt) {
        return 0;
    };

    NetPacket packet(MessageId::INVALID);

    socketClient.Start();
    socketClient.RegisterEvent(MessageId::INVALID, callback);
    socketClient.OnMsgHandler(socketClient, packet);
    socketClient.Socket();
    socketClient.OnPacket(packet);
    socketClient.Connect();
    socketClient.Reconnect();
    socketClient.OnDisconnected();
    socketClient.Stop();
    return true;
}

bool SocketConnectionFuzzTest(const uint8_t* data, size_t size)
{
    auto recv = [](const NetPacket &pkt) {
        return;
    };
    auto onDisconnected = []() {
        return;
    };
    int32_t fd = *(reinterpret_cast<const int32_t*>(data));
    SocketConnection socketConnection(1, recv, onDisconnected);
    
    auto socket = []() {
        return 0;
    };
    socketConnection.OnReadable(fd);
    socketConnection.OnShutdown(fd);
    socketConnection.OnException(fd);
    Msdp::DeviceStatus::SocketConnection::Connect(socket, recv, onDisconnected);
    return true;
}

bool SocketParamsFuzzTest(const uint8_t* data, size_t size)
{
    MessageParcel datas;
    if (!datas.WriteInterfaceToken(FORMMGR_INTERFACE_TOKEN) ||
        !datas.WriteBuffer(data, size) || !datas.RewindRead(0)) {
        return false;
    }
    AllocSocketPairParam allocSocketPairParam("testProgramName", 1);
    AllocSocketPairReply allocSocketPairReply(1, 1);
    
    allocSocketPairParam.Marshalling(datas);
    allocSocketPairParam.Unmarshalling(datas);
    allocSocketPairReply.Marshalling(datas);
    allocSocketPairReply.Unmarshalling(datas);
    return true;
}


bool SocketSessionFuzzTest(const uint8_t* data, size_t size)
{
    NetPacket packet(MessageId::COORDINATION_ADD_LISTENER);
    struct epoll_event ev{};

    SocketSession socketSession("testProgramName", 1, 1, 1, 1, 1);
    socketSession.SendMsg(packet);
    socketSession.ToString();
    socketSession.Dispatch(ev);
    return true;
}

} // namespace OHOS
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    /* Run your code on data */
    if (data == nullptr) {
        return 0;
    }

    OHOS::SocketClientFuzzTest(data, size);
    OHOS::SocketConnectionFuzzTest(data, size);
    OHOS::SocketParamsFuzzTest(data, size);
    OHOS::SocketSessionFuzzTest(data, size);
    return 0;
}
} // namespace DeviceStatus
} // namespace Msdp
} // namespace OHOS