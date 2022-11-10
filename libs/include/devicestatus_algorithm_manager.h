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

#ifndef DEVICESTATUS_ALGORITHM_MANAGER_H
#define DEVICESTATUS_ALGORITHM_MANAGER_H

#include <string>
#include <memory>
#include <vector>
#include <thread>
#include <mutex>
#include <map>
#include <errors.h>

#include "devicestatus_data_utils.h"
#include "devicestatus_msdp_interface.h"
#include "devicestatus_data_define.h"
#include "sensor_data_callback.h"
#include "device_status_absolute_still.h"
#include "device_status_horizontal.h"
#include "device_status_vertical.h"

namespace OHOS {
namespace Msdp {
namespace DeviceStatus {
class AlgoMgr final : public IMsdp {
public:
    AlgoMgr() = default;
    virtual ~AlgoMgr() = default;

    bool Init();
    ErrCode RegisterCallback(std::shared_ptr<MsdpAlgoCallback> callback) override;
    ErrCode UnregisterCallback() override;
    ErrCode Enable(Type type) override;
    ErrCode Disable(Type type) override;
    std::shared_ptr<MsdpAlgoCallback> GetCallbacksImpl()
    {
        std::unique_lock lock(mutex_);
        return callback_;
    }
    bool SensorStop(std::vector<Type> algoTypes);
    bool CheckSensorTypeId(int32_t sensorTypeId);
    bool SensorStart(Type type);
    std::vector<int32_t> GetSensorTypeId(Type type);
    std::vector<Type> GetAlgoType(int32_t type);
private:
    int32_t type_[Type::TYPE_MAX] = {0};
    std::shared_ptr<MsdpAlgoCallback> callback_ {nullptr};
    std::mutex mutex_;
    std::shared_ptr<SensorDataCallback> sensorEventCb_ {nullptr};
    std::shared_ptr<AbsoluteStill> still_ {nullptr};
    std::shared_ptr<Horizontal> horizontalPosition_ {nullptr};
    std::shared_ptr<Vertical> verticalPosition_ {nullptr};
    std::map<Type, int32_t> callAlgoNum_ = {};
    int32_t callSensorNum_ = 0;
    Type algoType_;
    int32_t ERR_NG = -1;
};
} // namespace DeviceStatus
} // namespace Msdp
} // namespace OHOS
#endif // DEVICESTATUS_ALGORITHM_MANAGER_H
