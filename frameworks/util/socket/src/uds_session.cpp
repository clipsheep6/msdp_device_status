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

#include "uds_session.h"

#include <cinttypes>
#include <sstream>

#include <fcntl.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#include "uds_socket.h"

namespace OHOS {
namespace MMI {
namespace {
constexpr int64_t INPUT_UI_TIMEOUT_TIME = 5 * 1000000;
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, MMI_LOG_DOMAIN, "UDSSession" };
} // namespace

UDSSession::UDSSession(const std::string& programName, const int32_t moduleType, const int32_t fd,
    const int32_t uid, const int32_t pid)
    : programName_(programName),
      moduleType_(moduleType),
      fd_(fd),
      uid_(uid),
      pid_(pid)
{
    UpdateDescript();
}

UDSSession::~UDSSession() {}

bool UDSSession::SendMsg(const char *buf, size_t size) const
{
    CHKPF(buf);
    if ((size == 0) || (size > MAX_PACKET_BUF_SIZE)) {
        MMI_LOGE("buf size:%{public}zu", size);
        return false;
    }
    if (fd_ < 0) {
        MMI_LOGE("fd_ is less than 0");
        return false;
    }
    int32_t sendSize = 0;
    int32_t sendCount = 0;
    constexpr int32_t resendLimit = 10;
    while (sendSize < size && sendCount < resendLimit) {
        sendCount += 1;
        auto ret = send(fd_, buf, size, SOCKET_FLAGS);
        if (ret < 0) {
            int32_t eno = errno;
            if (eno == EAGAIN || eno == EINTR || eno == EWOULDBLOCK) {
                continue;
            }
            MMI_LOGE("Send return failed,error:%{public}d fd:%{public}d", eno, fd_);
            return false;
        }
        sendSize += ret;
    }
    if (sendCount >= resendLimit && sendSize < size) {
        MMI_LOGE("Send too many times:%{public}d/%{public}d,dsize:%{public}zu/%{public}zu fd:%{public}d",
            sendCount, resendLimit, sendSize, size, fd_);
        return false;
    }
    return true;
}

void UDSSession::Close()
{
    CALL_LOG_ENTER;
    MMI_LOGD("enter fd_:%{public}d.", fd_);
    if (fd_ >= 0) {
        close(fd_);
        fd_ = -1;
        UpdateDescript();
    }
}

void UDSSession::UpdateDescript()
{
    std::ostringstream oss;
    oss << "fd = " << fd_
        << ", programName = " << programName_
        << ", moduleType = " << moduleType_
        << ((fd_ < 0) ? ", closed" : ", opened")
#ifdef OHOS_BUILD_MMI_DEBUG
        << ", clientFd = " << clientFd_
#endif // OHOS_BUILD_MMI_DEBUG
        << std::endl;
    descript_ = oss.str().c_str();
}

bool UDSSession::SendMsg(NetPacket& pkt) const
{
    if (pkt.ChkRWError()) {
        MMI_LOGE("Read and write status is error");
        return false;
    }
    StreamBuffer buf;
    pkt.MakeData(buf);
    return SendMsg(buf.Data(), buf.Size());
}

void UDSSession::AddEvent(int32_t id, int64_t time)
{
    CALL_LOG_ENTER;
    EventTime eventTime = {id, time};
    events_.push_back(eventTime);
}

void UDSSession::DelEvents(int32_t id)
{
    CALL_LOG_ENTER;
    int32_t count = 0;
    for (auto &item : events_) {
        ++count;
        if (item.id == id) {
            events_.erase(events_.begin(), events_.begin() + count);
            MMI_LOGD("Delete events");
            break;
        }
    }
    auto currentTime = GetSysClockTime();
    if (events_.empty() || (currentTime < (events_.begin()->eventTime + INPUT_UI_TIMEOUT_TIME))) {
        isANRProcess_ = false;
    }
}

int64_t UDSSession::GetEarlistEventTime() const
{
    CALL_LOG_ENTER;
    if (events_.empty()) {
        MMI_LOGD("events_ is empty");
        return 0;
    }
    return events_.begin()->eventTime;
}

bool UDSSession::IsEventQueueEmpty()
{
    if (events_.empty()) {
        MMI_LOGD("events_ is empty");
        return true;
    }
    return false;
}

void UDSSession::AddPermission(bool hasPermission)
{
    hasPermission_ = hasPermission;
}

bool UDSSession::HasPermission()
{
    return hasPermission_;
}
} // namespace MMI
} // namespace OHOS