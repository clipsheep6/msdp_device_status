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

#include "stream_socket.h"

#include <cinttypes>

namespace OHOS {
namespace Msdp {
namespace DeviceStatus {
namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL { LOG_CORE, MSDP_DOMAIN_ID, "StreamSocket" };
} // namespace

StreamSocket::StreamSocket() {}

StreamSocket::~StreamSocket()
{
    Close();
    EpollClose();
}

void StreamSocket::OnReadPackets(CircleStreamBuffer &circBuf, StreamSocket::PacketCallBackFun callbackFun)
{
    constexpr int32_t headSize = static_cast<int32_t>(sizeof(PackHead));
    for (int32_t i = 0; i < ONCE_PROCESS_NETPACKET_LIMIT; i++) {
        const int32_t unreadSize = circBuf.UnreadSize();
        if (unreadSize < headSize) {
            break;
        }
        int32_t dataSize = unreadSize - headSize;
        char *buf = const_cast<char *>(circBuf.ReadBuf());
        CHKPB(buf);
        PackHead *head = reinterpret_cast<PackHead *>(buf);
        CHKPB(head);
        if (static_cast<int32_t>(head->size) < 0 || static_cast<size_t>(head->size) > MAX_PACKET_BUF_SIZE) {
            FI_HILOGE("Packet header parsing error, and this error cannot be recovered, the buffer will be reset, "
                "head->size:%{public}d, unreadSize:%{public}d", head->size, unreadSize);
            circBuf.Reset();
            break;
        }
        if (head->size > dataSize) {
            break;
        }
        NetPacket pkt(head->idMsg);
        if ((head->size > 0) && (!pkt.Write(&buf[headSize], head->size))) {
            FI_HILOGW("Error writing data in the NetPacket, it will be retried next time, messageid:%{public}d, "
                "size:%{public}d", head->idMsg, head->size);
            break;
        }
        if (!circBuf.SeekReadPos(pkt.GetPacketLength())) {
            FI_HILOGW("Set read position error, and this error cannot be recovered, and the buffer will be reset, "
                "packetSize:%{public}d, unreadSize:%{public}d", pkt.GetPacketLength(), unreadSize);
            circBuf.Reset();
            break;
        }
        callbackFun(pkt);
        if (circBuf.IsEmpty()) {
            circBuf.Reset();
            break;
        }
    }
}

void StreamSocket::Close()
{
    if (fd_ >= 0) {
        int32_t rf = close(fd_);
        if (rf < 0) {
            FI_HILOGE("Socket close failed rf:%{public}d", rf);
        }
    }
    fd_ = -1;
}
} // namespace DeviceStatus
} // namespace Msdp
} // namespace OHOS