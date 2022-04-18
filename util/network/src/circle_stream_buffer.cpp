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
#include "circle_stream_buffer.h"

namespace OHOS {
namespace MMI {
void CircleStreamBuffer::MoveMemoryToBegin()
{
    int32_t unreadSize = UnreadSize();
    if (unreadSize > 0 && rPos_ > 0) {
        int32_t idx = 0;
        for (int32_t i = rPos_; i <= wPos_; i++) {
            szBuff_[idx] = szBuff_[i];
            szBuff_[i] = '\0';
            idx++;
        }
    }
    MMI_HILOGD("unreadSize:%{public}d rPos:%{public}d wPos:%{public}d", unreadSize, rPos_, wPos_);
    rPos_ = 0;
    wPos_ = unreadSize;
}

bool CircleStreamBuffer::CheckWrite(size_t size)
{
    int32_t aviSize = GetAvailableSize();
    if (size > aviSize && rPos_ > 0) {
        MoveMemoryToBegin();
        aviSize = GetAvailableSize();
    }
    return (aviSize >= size);
}

bool CircleStreamBuffer::Write(const char *buf, size_t size)
{
    if (!CheckWrite(size)) {
        MMI_HILOGE("Out of buffer memory, availableSize:%{public}d, size:%{public}zu,"
            "unreadSize:%{public}d, rPos:%{public}d, wPos:%{public}d",
            GetAvailableSize(), size, UnreadSize(), rPos_, wPos_);
        return false;
    }
    return StreamBuffer::Write(buf, size);
}
} // namespace MMI
} // namespace OHOS