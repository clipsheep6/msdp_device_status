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
#ifndef STREAM_BUFFER_H
#define STREAM_BUFFER_H

#include <cstdint>
#include <string>
#include <vector>

#include "proto.h"
#include "nocopyable.h"
#include "securec.h"
#include "devicestatus_define.h"

namespace OHOS {
namespace Msdp {
class StreamBuffer {
    static constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, MSDP_DOMAIN_ID, "StreamBuffer" };
public:
    StreamBuffer() = default;
    DISALLOW_MOVE(StreamBuffer);
    virtual ~StreamBuffer() = default;
    explicit StreamBuffer(const StreamBuffer &buf);
    virtual StreamBuffer &operator=(const StreamBuffer &other);

    void Reset();
    void Clean();
    bool SeekReadPos(int32_t n);

    bool Read(std::string &buf);
    bool Write(const std::string &buf);

    bool Read(StreamBuffer &buf);
    bool Write(const StreamBuffer &buf);

    bool Read(char *buf, size_t size);
    virtual bool Write(const char *buf, size_t size);

    bool IsEmpty() const;
    size_t Size() const;
    int32_t UnreadSize() const;
    int32_t GetAvailableBufSize() const;

    bool ChkRWError() const;
    const std::string &GetErrorStatusRemark() const;
    const char *Data() const;

    template<typename T>
    bool Read(T &data);
    template<typename T>
    bool Write(const T &data);

    const char *ReadBuf() const;

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
        ERROR_STATUS_WRITE,
    };
    ErrorStatus rwErrorStatus_ = ErrorStatus::ERROR_STATUS_OK;
    int32_t rCount_ { 0 };
    int32_t wCount_ { 0 };

    int32_t rPos_ { 0 };
    int32_t wPos_ { 0 };
    char szBuff_[MAX_STREAM_BUF_SIZE+1] = {};
};

template<typename T>
bool StreamBuffer::Read(T &data)
{
    if (!Read(reinterpret_cast<char *>(&data), sizeof(data))) {
        FI_HILOGE("[%{public}s] size:%{public}zu count:%{public}d,errCode:%{public}d",
            GetErrorStatusRemark().c_str(), sizeof(data), rCount_ + 1, STREAM_BUF_READ_FAIL);
        return false;
    }
    return true;
}

template<typename T>
bool StreamBuffer::Write(const T &data)
{
    if (!Write(reinterpret_cast<const char *>(&data), sizeof(data))) {
        FI_HILOGE("[%{public}s] size:%{public}zu,count:%{public}d,errCode:%{public}d",
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
#endif // STREAM_BUFFER_H