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
namespace Msdp {
void CircleStreamBuffer::CopyDataToBegin()
{
    copy_data_to_begin(&rustStreamBuffer_);
}

bool CircleStreamBuffer::CheckWrite(size_t size)
{
    return check_write(&rustStreamBuffer_, size);
}

bool CircleStreamBuffer::Write(const char *buf, size_t size)
{
    return circle_write(&rustStreamBuffer_, buf, size);
}
} // namespace Msdp
} // namespace OHOS