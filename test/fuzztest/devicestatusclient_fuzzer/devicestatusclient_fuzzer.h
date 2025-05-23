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

#include <cstdint>
#include <unistd.h>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <iostream>
#include <random>
#include <thread>

#include "devicestatus_callback_stub.h"
#include "stationary_manager.h"

#define FUZZ_PROJECT_NAME "devicestatusclient_fuzzer"

namespace OHOS {
namespace Msdp {
namespace DeviceStatus {

class DeviceStatusClientFuzzer {
public:
    static bool DoSomethingInterestingWithMyAPI(const uint8_t* data, size_t size);
    static void TestSubscribeCallback(const uint8_t* data);
    static void TestGetDevicestatusData(Type type);
    static void TestUnSubscribeCallback(Type type);

    class DeviceStatusTestCallback : public DeviceStatusCallbackStub {
    public:
        DeviceStatusTestCallback() {};
        virtual ~DeviceStatusTestCallback() {};
        virtual void OnDeviceStatusChanged(const Data& devicestatusData) override;
    };
};
} // namespace DeviceStatus
} // namespace Msdp
} // namespace OHOS
#endif // DEVICESTATUSCLIENT_FUZZER_H
