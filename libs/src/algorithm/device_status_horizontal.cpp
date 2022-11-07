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

#include "device_status_horizontal.h"

#include <cmath>
#include <cstdio>

#include "devicestatus_common.h"

namespace OHOS {
namespace Msdp {
namespace DeviceStatus {

bool Horizontal::Init(Type type)
{
    DEV_HILOGD(SERVICE, "enter");
    algoCallback_ = std::bind(&Horizontal::StartAlgorithm, this, std::placeholders::_1, std::placeholders::_2);
    if (algoCallback_ != nullptr) {
        sensorCallback_->SubscribeSensorEvent(type, algoCallback_);
    }
    DEV_HILOGE(SERVICE, "sensorCallback is null");
    return true;
}

void Horizontal::StartAlgorithm(int32_t sensorTypeId, AccelData* sensorData)
{
    DEV_HILOGD(SERVICE, "enter");
    GetData(sensorTypeId, sensorData);
    ExecuteAlgorithm();
}

void Horizontal::ExecuteAlgorithm()
{
    DEV_HILOGD(SERVICE, "enter");

    algoPara_.pitch = -atan2(algoPara_.y, algoPara_.z) * (ANGLE_180_DEGREE / PI);
    algoPara_.roll = atan2(algoPara_.x, algoPara_.z) * (ANGLE_180_DEGREE / PI);
    DEV_HILOGI(SERVICE, "pitch:%{public}f, roll:%{public}f", algoPara_.pitch, algoPara_.roll);

    if (((abs(algoPara_.pitch) > ANGLE_HOR_LOW_THRHD) && (abs(algoPara_.pitch) <= ANGLE_HOR_UP_THRHD)) &&
        ((abs(algoPara_.roll) > ANGLE_HOR_LOW_THRHD) && (abs(algoPara_.roll) <= ANGLE_HOR_UP_THRHD))) {
        if (state_ == HORIZONTAL) {
            return;
        }
        counter_--;
        if (counter_ == 0) {
            counter_ = COUNTER_THRESHOLD;
            Report(VALUE_ENTER, HORIZONTAL, TYPE_HORIZONTAL_POSITION);
        }
    } else {
        counter_ = COUNTER_THRESHOLD;
        if (state_ == NON_HORIZONTAL) {
            return;
        }
        Report(VALUE_EXIT, NON_HORIZONTAL, TYPE_HORIZONTAL_POSITION);
    }
}
} // namespace DeviceStatus
} // namespace Msdp
} // namespace OHOS
