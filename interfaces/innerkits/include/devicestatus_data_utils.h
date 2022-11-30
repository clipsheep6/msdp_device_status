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

#ifndef DEVICESTATUS_DATA_UTILS_H
#define DEVICESTATUS_DATA_UTILS_H

#include<string>

namespace OHOS {
namespace Msdp {
class DevicestatusDataUtils {
public:
    enum DevicestatusType {
        TYPE_INVALID = -1,
        TYPE_HIGH_STILL,
        TYPE_FINE_STILL,
        TYPE_CAR_BLUETOOTH,
		TYPE_STAND,
        TYPE_LID_OPEN,
        TYPE_MAX
    };

    enum DevicestatusValue {
        VALUE_INVALID = -1,
        VALUE_ENTER,
        VALUE_EXIT
    };

    enum Value {
        INVALID = 0,
        VALID
    };

    struct DevicestatusData {
        DevicestatusType type;
        DevicestatusValue value;
    };
};

typedef struct DeviceStatusJsonData {
    int32_t type;
    std::string json;
}DeviceStatusJsonD;

static DeviceStatusJsonD DeviceStatusJson[] = {
    {DevicestatusDataUtils::DevicestatusType::TYPE_INVALID, "TYPE_INVALID"},
    {DevicestatusDataUtils::DevicestatusType::TYPE_HIGH_STILL, "TYPE_HIGH_STILL"},
    {DevicestatusDataUtils::DevicestatusType::TYPE_FINE_STILL, "TYPE_FINE_STILL"},
    {DevicestatusDataUtils::DevicestatusType::TYPE_CAR_BLUETOOTH, "TYPE_CAR_BLUETOOTH"},
    {DevicestatusDataUtils::DevicestatusType::TYPE_STAND, "TYPE_STAND"},
    {DevicestatusDataUtils::DevicestatusType::TYPE_LID_OPEN, "TYPE_LID_OPEN"}
};
} // namespace Msdp
} // namespace OHOS
#endif // DEVICESTATUS_DATA_UTILS_H
