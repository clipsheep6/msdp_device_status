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

#ifndef DEVICESTATUS_STREAM_BUFFER_H
#define DEVICESTATUS_STREAM_BUFFER_H

#include <cstdint>
#include <string>
#include <vector>

#include "devicestatus_proto.h"
#include "fi_log.h"
#include "nocopyable.h"
#include "securec.h"

#undef LOG_TAG
#define LOG_TAG "StreamBuffer"

namespace OHOS {
namespace Msdp {
class StreamBuffer {
public:
    StreamBuffer() = default;
    DISALLOW_MOVE(StreamBuffer);
    explicit StreamBuffer(const StreamBuffer &buf);
    virtual StreamBuffer &operator=(const StreamBuffer &buffer);
    virtual ~StreamBuffer() = default;

    size_t Size() const;
    int32_t ResidualSize() const;
    int32_t GetAvailableBufSize() const;
    void Reset();
    void Clean();
    bool SeekReadPos(int32_t n);
    bool Read(std::string &buf);
    bool Write(const std::string &buf);
    bool Read(StreamBuffer &buf);
    bool Write(const StreamBuffer &buf);
    bool Read(char *buf, size_t size);
    bool empty() const;
    bool ChkRWError() const;
    virtual bool Write(const char *buf, size_t size);
    const std::string &GetErrorStatusRemark() const;
    const char *Data() const;
    const char *ReadBuf() const;
    template<typename T>
    bool Read(T &data);
    template<typename T>
    bool Write(const T &data);
    template<typename T>
    StreamBuffer &operator >> (T &data);
    template<typename T>
    StreamBuffer &operator << (const T &data);

protected:
    bool Clone(const StreamBuffer &buf);

protected:
    enum class ErrorStatus {
        ERROR_STATUS_OK,
        ERROR_STATUS_READ,
        ERROR_STATUS_WRITE
    };
    ErrorStatus rwErrorStatus_ { ErrorStatus::ERROR_STATUS_OK };
    int32_t rCount_ { 0 };
    int32_t wCount_ { 0 };
    int32_t rPos_ { 0 };
    int32_t wPos_ { 0 };
    char szBuff_[MAX_STREAM_BUF_SIZE + 1] {};
};

template<typename T>
bool StreamBuffer::Read(T &data)
{
    if (!Read(reinterpret_cast<char *>(&data), sizeof(data))) {
        FI_HILOGE("[%{public}s], size:%{public}zu, count:%{public}d, errCode:%{public}d",
            GetErrorStatusRemark().c_str(), sizeof(data), rCount_ + 1, STREAM_BUF_READ_FAIL);
        return false;
    }
    return true;
}

template<typename T>
bool StreamBuffer::Write(const T &data)
{
    if (!Write(reinterpret_cast<const char *>(&data), sizeof(data))) {
        FI_HILOGE("[%{public}s], size:%{public}zu, count:%{public}d, errCode:%{public}d",
            GetErrorStatusRemark().c_str(), sizeof(data), wCount_ + 1, STREAM_BUF_WRITE_FAIL);
        return false;
    }
    return true;
}

template<typename T>
StreamBuffer &StreamBuffer::operator>>(T &data)
{
    if (!Read(data)) {
        FI_HILOGW("Read data failed");
    }
    return *this;
}

template<typename T>
StreamBuffer &StreamBuffer::operator<<(const T &data)
{
    if (!Write(data)) {
        FI_HILOGW("Write data failed");
    }
    return *this;
}
} // namespace Msdp
} // namespace OHOS
#endif // DEVICESTATUS_STREAM_BUFFER_H