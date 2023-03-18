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
#ifndef I_COORDINATION_H
#define I_COORDINATION_H

#include <functional>
#include <memory>
#include <vector>

#include "stream_session.h"

namespace OHOS {
namespace Msdp {
namespace DeviceStatus {
class ICoordination {
public:
    ICoordination() = default;
    virtual ~ICoordination() = default;

    virtual int32_t EnableCoordination(SessionPtr sess, int32_t userData, bool enabled) = 0;
    virtual int32_t StartCoordination(SessionPtr sess, int32_t userData, const std::string &sinkDeviceId,
        int32_t srcDeviceId) = 0;
    virtual int32_t StopCoordination(SessionPtr sess, int32_t userData) = 0;
    virtual int32_t GetCoordinationState(SessionPtr sess, int32_t userData, const std::string &deviceId) = 0;
    virtual int32_t RegisterCoordinationListener(SessionPtr sess) = 0;
    virtual int32_t UnregisterCoordinationListener(SessionPtr sess) = 0;
    virtual void Dump(int32_t fd) = 0;
    virtual void OnSessionLost(SessionPtr session) = 0;
};
} // namespace DeviceStatus
} // namespace Msdp
} // namespace OHOS
#endif // I_COORDINATION_H