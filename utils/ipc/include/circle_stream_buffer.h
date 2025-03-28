/*
 * Copyright (c) 2022-2023 Huawei Device Co., Ltd.
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

#ifndef CIRCLE_STREAM_BUFFER_H
#define CIRCLE_STREAM_BUFFER_H

#include "devicestatus_stream_buffer.h"

#undef LOG_TAG
#define LOG_TAG "CircleStreamBuffer"

namespace OHOS {
namespace Msdp {
class CircleStreamBuffer : public StreamBuffer {
public:
    CircleStreamBuffer() = default;
    DISALLOW_MOVE(CircleStreamBuffer);
    virtual ~CircleStreamBuffer() = default;

    bool CheckWrite(size_t size);
    virtual bool Write(const char *buf, size_t size) override;

protected:
    void CopyDataToBegin();
};
} // namespace Msdp
} // namespace OHOS
#endif // CIRCLE_STREAM_BUFFER_H