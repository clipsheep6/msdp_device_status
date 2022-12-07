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

#ifndef DEVICESTATUS_MSDP_INTERFACE_H
#define DEVICESTATUS_MSDP_INTERFACE_H

#include <string>
#include <memory>
#include <map>
#include <errors.h>

#include "devicestatus_data_utils.h"

namespace OHOS {
namespace Msdp {
namespace DeviceStatus {
class DeviceStatusMsdpInterface {
public:
    DeviceStatusMsdpInterface() {}
    virtual ~DeviceStatusMsdpInterface() {}
    class MsdpAlgorithmCallback {
    public:
        MsdpAlgorithmCallback() = default;
        virtual ~MsdpAlgorithmCallback() = default;
        virtual void OnResult(const DeviceStatusDataUtils::DeviceStatusData& data) = 0;
    };

    virtual void RegisterCallback(std::shared_ptr<MsdpAlgorithmCallback> callback) = 0;
    virtual void UnregisterCallback() = 0;
    virtual void Enable(DeviceStatusDataUtils::DeviceStatusType type) = 0;
    virtual void Disable(DeviceStatusDataUtils::DeviceStatusType type) = 0;
    virtual void DisableCount(DeviceStatusDataUtils::DeviceStatusType type) = 0;
};

struct MsdpAlgorithmHandle {
    void* handle;
    DeviceStatusMsdpInterface* (*create)();
    void* (*destroy)(DeviceStatusMsdpInterface*);
    DeviceStatusMsdpInterface* pAlgorithm;
    MsdpAlgorithmHandle() : handle(nullptr), create(nullptr), destroy(nullptr), pAlgorithm(nullptr) {}
    ~MsdpAlgorithmHandle() {}
    void Clear()
    {
        handle = nullptr;
        create = nullptr;
        destroy = nullptr;
        pAlgorithm = nullptr;
    }
};
} // namespace DeviceStatus
}
}
#endif // DEVICESTATUS_MSDP_INTERFACE_H
