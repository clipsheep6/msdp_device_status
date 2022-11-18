/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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
    
#ifndef DEVICESTATUSCLIENT_FUZZER_H
#define DEVICESTATUSCLIENT_FUZZER_H

#include <climits>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <thread>

#include <unistd.h>
#include <fcntl.h>

#include "devicestatus_callback_stub.h"
#include "devicestatus_client.h"

#define FUZZ_PROJECT_NAME "devicestatusclient_fuzzer"
namespace OHOS {
namespace Msdp {
namespace DeviceStatus {
class DevicestatusClientFuzzer {
public:
    static bool DoSomethingInterestingWithMyAPI(const uint8_t* data, size_t size);
    static void TestSubscribeCallback(const uint8_t* data);
    static void TestGetDevicestatusData();
    static void TestUnSubscribeCallback();

    class DevicestatusTestCallback : public DevicestatusCallbackStub {
    public:
        DevicestatusTestCallback() {};
        virtual ~DevicestatusTestCallback() {};
        virtual void OnDevicestatusChanged(const DevicestatusDataUtils::DevicestatusData& devicestatusData) override;
    };
};
} // namespace DeviceStatus
} // namespace Msdp
} // namespace OHOS
#endif // DEVICESTATUSCLIENT_FUZZER_H