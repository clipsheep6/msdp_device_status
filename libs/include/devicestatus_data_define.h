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

#ifndef DEVICESTATUS_DATA_DEFINE_H
#define DEVICESTATUS_DATA_DEFINE_H

#include <functional>

#ifdef DEVICE_STATUS_SENSOR_ENABLE
#include "sensor_agent.h"
#endif // DEVICE_STATUS_SENSOR_ENABLE
#include "stationary_data.h"

namespace OHOS {
namespace Msdp {
namespace DeviceStatus {
constexpr double PI { 3.141592653589793 };
constexpr int32_t RESULTANT_ACC_LOW_THRHD { 7 };
constexpr int32_t RESULTANT_ACC_UP_THRHD { 11 };
constexpr double ACC_VALID_THRHD { 160.0 };
constexpr double ANGLE_180_DEGREE { 180.0 };
constexpr double ANGLE_HOR_UP_THRHD { 180.1 };
constexpr double ANGLE_HOR_LOW_THRHD { 160.0 };
constexpr double ANGLE_HOR_FLIPPED_THRHD { 5.0 };
constexpr double ANGLE_VER_UP_THRHD { 110.0 };
constexpr double ANGLE_VER_LOW_THRHD { 80.0 };
constexpr double ANGLE_VER_FLIPPED_THRHD { 5.0 };

constexpr int32_t VALID_TIME_THRESHOLD { 500 };
constexpr int32_t ACC_SAMPLE_PERIOD { 100 };
constexpr int32_t COUNTER_THRESHOLD = VALID_TIME_THRESHOLD / ACC_SAMPLE_PERIOD;

#ifdef DEVICE_STATUS_SENSOR_ENABLE
using SensorCallback = std::function<void(int32_t, AccelData*)>;
#endif // DEVICE_STATUS_SENSOR_ENABLE
} // namespace DeviceStatus
} // namespace Msdp
} // namespace OHOS
#endif // DEVICESTATUS_DATA_DEFINE_H
