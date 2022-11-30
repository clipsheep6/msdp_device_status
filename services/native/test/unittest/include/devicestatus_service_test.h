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

#ifndef DEVICESTATUS_SERVICE_TEST_H
#define DEVICESTATUS_SERVICE_TEST_H

#include <gtest/gtest.h>

#include "devicestatus_callback_stub.h"

namespace OHOS {
namespace Msdp {
namespace DeviceStatus {
const std::u16string ARGS_H = u"-h";
class DevicestatusServiceTest : public testing::Test {
public:

    class DevicestatusServiceTestCallback : public DevicestatusCallbackStub {
    public:
        DevicestatusServiceTestCallback() {};
        virtual ~DevicestatusServiceTestCallback() {};
        virtual void OnDevicestatusChanged(const DevicestatusDataUtils::DevicestatusData& devicestatusData) override;
    };
};
} // namespace DeviceStatus
} // namespace Msdp
} // namespace OHOS
#endif // DEVICESTATUS_SERVICE_TEST_H
