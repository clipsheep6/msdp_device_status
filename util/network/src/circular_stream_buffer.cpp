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
#include "circular_stream_buffer.h"

namespace OHOS {
namespace MMI {
void CircularStreamBuffer::MoveMemoryToBegin()
{
    size_t unreadSize = UnreadSize();
    if (unreadSize == 0 || rIdx_ == 0) {
        MMI_HILOGE("Meaningless memory copy");
        return;
    }
    int32_t idx = 0;
    for (int32_t i = rIdx_; i <= wIdx_; i++) {
        szBuff_[idx++] = szBuff_[i];
        szBuff_[i] = '\0';
    }
    rIdx_ = 0;
    wIdx_ = static_cast<int32_t>(unreadSize);
}

bool CircularStreamBuffer::Write(const char *buf, size_t size)
{
    size_t aviSize = AvailableSize();
    if (size > aviSize) {
        MoveMemoryToBegin();
    }
    return StreamBuffer::Write(buf, size);
}
} // namespace MMI
} // namespace OHOS