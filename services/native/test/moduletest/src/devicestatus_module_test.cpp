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

#include "devicestatus_module_test.h"

#include <iostream>
#include <chrono>
#include <thread>
#include <gtest/gtest.h>
#include <if_system_ability_manager.h>
#include <ipc_skeleton.h>
#include <string_ex.h>

#include "fi_log.h"
#include "stationary_manager.h"

using namespace testing::ext;
using namespace ::OHOS;
using namespace ::OHOS::Msdp;
using namespace ::OHOS::Msdp::DeviceStatus;

namespace {
constexpr ::OHOS::HiviewDFX::HiLogLabel LABEL { LOG_CORE, MSDP_DOMAIN_ID, "DeviceStatusModuleTest" };
}

Type DeviceStatusModuleTest::g_moduleTest = Type::TYPE_INVALID;

void DeviceStatusModuleTest::DeviceStatusModuleTestCallback::OnDeviceStatusChanged(const Data& devicestatusData)
{
    GTEST_LOG_(INFO) << "DeviceStatusModuleTestCallback type: " << devicestatusData.type;
    GTEST_LOG_(INFO) << "DeviceStatusModuleTestCallback value: " << devicestatusData.value;
    EXPECT_EQ(true, devicestatusData.type == g_moduleTest &&
        (devicestatusData.value >= OnChangedValue::VALUE_INVALID &&
        devicestatusData.value <= OnChangedValue::VALUE_EXIT)) <<
        "DeviceStatusModuleTestCallback failed";
}

/**
 * @tc.name: DeviceStatusCallbackTest
 * @tc.desc: test devicestatus callback in proxy
 * @tc.type: FUNC
 */
HWTEST_F (DeviceStatusModuleTest, DeviceStatusCallbackTest, TestSize.Level0)
{
    g_moduleTest = Type::TYPE_ABSOLUTE_STILL;
    Type type = g_moduleTest;
    auto stationaryManager = StationaryManager::GetInstance();
    sptr<IRemoteDevStaCallback> cb = new (std::nothrow) DeviceStatusModuleTestCallback();
    EXPECT_FALSE(cb == nullptr);
    GTEST_LOG_(INFO) << "Start register";
    stationaryManager->SubscribeCallback(type, ActivityEvent::ENTER_EXIT, ReportLatencyNs::LONG, cb);
    GTEST_LOG_(INFO) << "Cancel register";
    stationaryManager->UnsubscribeCallback(type, ActivityEvent::ENTER_EXIT, cb);
}

/**
 * @tc.name: GetDeviceStatusDataTest
 * @tc.desc: test get devicestatus data in proxy
 * @tc.type: FUNC
 */
HWTEST_F (DeviceStatusModuleTest, GetDeviceStatusDataTest001, TestSize.Level0)
{
    CALL_TEST_DEBUG;
    g_moduleTest = Type::TYPE_HORIZONTAL_POSITION;
    Type type = g_moduleTest;
    auto stationaryManager = StationaryManager::GetInstance();
    Data data = stationaryManager->GetDeviceStatusData(type);
    GTEST_LOG_(INFO) << "type: " << data.type;
    GTEST_LOG_(INFO) << "value: " << data.value;
    EXPECT_EQ(true, data.type == g_moduleTest &&
        data.value >= OnChangedValue::VALUE_INVALID &&
        data.value <= OnChangedValue::VALUE_EXIT) << "GetDeviceStatusData failed";
}

/**
 * @tc.name: GetDeviceStatusDataTest
 * @tc.desc: test get devicestatus data in proxy
 * @tc.type: FUNC
 */
HWTEST_F (DeviceStatusModuleTest, GetDeviceStatusDataTest002, TestSize.Level0)
{
    CALL_TEST_DEBUG;
    g_moduleTest = Type::TYPE_ABSOLUTE_STILL;
    Type type = g_moduleTest;
    auto stationaryManager = StationaryManager::GetInstance();
    Data data = stationaryManager->GetDeviceStatusData(type);
    GTEST_LOG_(INFO) << "type: " << data.type;
    GTEST_LOG_(INFO) << "value: " << data.value;
    EXPECT_EQ(true, data.type == g_moduleTest &&
        data.value >= OnChangedValue::VALUE_INVALID &&
        data.value <= OnChangedValue::VALUE_EXIT) << "GetDeviceStatusData failed";
}

/**
 * @tc.name: GetDeviceStatusDataTest
 * @tc.desc: test get devicestatus data in proxy
 * @tc.type: FUNC
 */
HWTEST_F (DeviceStatusModuleTest, GetDeviceStatusDataTest003, TestSize.Level0)
{
    CALL_TEST_DEBUG;
    g_moduleTest = Type::TYPE_VERTICAL_POSITION;
    Type type = g_moduleTest;
    auto stationaryManager = StationaryManager::GetInstance();
    Data data = stationaryManager->GetDeviceStatusData(type);
    GTEST_LOG_(INFO) << "type: " << data.type;
    GTEST_LOG_(INFO) << "value: " << data.value;
    EXPECT_EQ(true, data.type == g_moduleTest &&
        data.value >= OnChangedValue::VALUE_INVALID &&
        data.value <= OnChangedValue::VALUE_EXIT) << "GetDeviceStatusData failed";
}
