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

#ifdef DEVICE_STATUS_SENSOR_ENABLE
#include <memory>
#include <dlfcn.h>

#include <gtest/gtest.h>

#include "accesstoken_kit.h"
#include "devicestatus_define.h"

#define private public
#include "devicestatus_algorithm_manager.h"
#include "sensor_agent_type.h"
#include "sensor_data_callback.h"
#undef private
#include "devicestatus_dumper.h"
#include "devicestatus_manager.h"

#undef LOG_TAG
#define LOG_TAG "DeviceStatusAlgorithmTest"

namespace OHOS {
namespace Msdp {
namespace DeviceStatus {
using namespace testing::ext;
namespace {
std::shared_ptr<AlgoMgr> g_manager { nullptr };
#ifdef __aarch64__
const std::string DEVICESTATUS_ALGO_LIB_PATH { "/system/lib64/libdevicestatus_algo.z.so" };
#else
const std::string DEVICESTATUS_ALGO_LIB_PATH { "/system/lib/libdevicestatus_algo.z.so" };
#endif
} // namespace

class DeviceStatusAlgorithmTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
    class DeviceStatusMock : public IMsdp::MsdpAlgoCallback {
    public:
        DeviceStatusMock() = default;
        virtual ~DeviceStatusMock() = default;
        void OnResult(const Data& data) {};
    };
    int32_t LoadAlgoLibrary(const std::shared_ptr<MsdpAlgoHandle> &algoHandler);
    int32_t UnloadAlgoLibrary(const std::shared_ptr<MsdpAlgoHandle> &algoHandler);
};

void DeviceStatusAlgorithmTest::SetUpTestCase()
{
    g_manager = std::make_shared<AlgoMgr>();
}

void DeviceStatusAlgorithmTest::TearDownTestCase()
{
    g_manager = nullptr;
}

void DeviceStatusAlgorithmTest::SetUp() {}

void DeviceStatusAlgorithmTest::DeviceStatusAlgorithmTest::TearDown() {}

int32_t DeviceStatusAlgorithmTest::LoadAlgoLibrary(const std::shared_ptr<MsdpAlgoHandle> &algoHandler)
{
    FI_HILOGI("Enter");
    if (algoHandler == nullptr) {
        FI_HILOGE("algoHandler is nullptr");
        return RET_ERR;
    }
    if (algoHandler->handle != nullptr) {
        FI_HILOGE("handle already exists");
        return RET_OK;
    }

    std::string dlName = DEVICESTATUS_ALGO_LIB_PATH;
    char libRealPath[PATH_MAX] = { 0 };
    if (realpath(dlName.c_str(), libRealPath) == nullptr) {
        FI_HILOGE("Get absolute algoPath is error, errno:%{public}d", errno);
        return RET_ERR;
    }

    algoHandler->handle = dlopen(libRealPath, RTLD_LAZY);
    if (algoHandler->handle == nullptr) {
        FI_HILOGE("Cannot load library error:%{public}s", dlerror());
        return RET_ERR;
    }

    algoHandler->create = reinterpret_cast<IMsdp* (*)()>(dlsym(algoHandler->handle, "Create"));
    algoHandler->destroy = reinterpret_cast<void *(*)(IMsdp*)>(dlsym(algoHandler->handle, "Destroy"));
    if (algoHandler->create == nullptr || algoHandler->destroy == nullptr) {
        FI_HILOGE("%{public}s dlsym create or destroy failed", dlName.c_str());
        dlclose(algoHandler->handle);
        algoHandler->Clear();
        return RET_ERR;
    }
    return RET_OK;
}

int32_t DeviceStatusAlgorithmTest::UnloadAlgoLibrary(const std::shared_ptr<MsdpAlgoHandle> &algoHandler)
{
    FI_HILOGI("Enter");
    if (algoHandler == nullptr) {
        FI_HILOGE("algoHandler is nullptr");
        return RET_ERR;
    }
    if (algoHandler->handle == nullptr) {
        FI_HILOGE("handle is nullptr");
        return RET_ERR;
    }

    if (algoHandler->pAlgorithm != nullptr) {
        algoHandler->destroy(algoHandler->pAlgorithm);
        algoHandler->pAlgorithm = nullptr;
    }
    dlclose(algoHandler->handle);
    algoHandler->Clear();
    return RET_OK;
}

/**
 * @tc.name: DeviceStatusAlgoAbsoluteStillTest
 * @tc.desc: test devicestatus Absolute Still Algorithm
 * @tc.type: FUNC
 */
HWTEST_F(DeviceStatusAlgorithmTest, DeviceStatusAlgorithmTest001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    bool result = g_manager->StartSensor(Type::TYPE_ABSOLUTE_STILL);
    EXPECT_TRUE(result);
    int32_t ret = g_manager->Enable(Type::TYPE_ABSOLUTE_STILL);
    ASSERT_EQ(ret, RET_OK);
    result = g_manager->UnregisterSensor(Type::TYPE_ABSOLUTE_STILL);
    EXPECT_TRUE(result);
    ret = g_manager->Disable(Type::TYPE_ABSOLUTE_STILL);
    ASSERT_EQ(ret, RET_OK);
}

/**
 * @tc.name: DeviceStatusAlgoHorizontalTest
 * @tc.desc: test devicestatus AlgoHorizontal Algorithm
 * @tc.type: FUNC
 */
HWTEST_F(DeviceStatusAlgorithmTest, DeviceStatusAlgorithmTest002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    bool result = g_manager->StartSensor(Type::TYPE_HORIZONTAL_POSITION);
    EXPECT_TRUE(result);
    int32_t ret = g_manager->Enable(Type::TYPE_HORIZONTAL_POSITION);
    ASSERT_EQ(ret, RET_OK);
    result = g_manager->UnregisterSensor(Type::TYPE_HORIZONTAL_POSITION);
    EXPECT_TRUE(result);
    ret = g_manager->Disable(Type::TYPE_HORIZONTAL_POSITION);
    ASSERT_EQ(ret, RET_OK);
}

/**
 * @tc.name: DeviceStatusVeriticalTest
 * @tc.desc: test devicestatus Veritical Algorithm
 * @tc.type: FUNC
 */
HWTEST_F(DeviceStatusAlgorithmTest, DeviceStatusAlgorithmTest003, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    bool result = g_manager->StartSensor(Type::TYPE_INVALID);
    EXPECT_FALSE(result);
    int32_t ret = g_manager->Enable(Type::TYPE_INVALID);
    ASSERT_EQ(ret, RET_ERR);
    result = g_manager->UnregisterSensor(Type::TYPE_INVALID);
    EXPECT_FALSE(result);
    ret = g_manager->Disable(Type::TYPE_INVALID);
    ASSERT_EQ(ret, RET_ERR);
}

/**
 * @tc.name: DeviceStatusAlgorithmManagerTest
 * @tc.desc: test devicestatus Algorithm g_manager
 * @tc.type: FUNC
 */
HWTEST_F(DeviceStatusAlgorithmTest, DeviceStatusAlgorithmTest004, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<AlgoMgr> g_manager = std::make_shared<AlgoMgr>();
    int32_t ret = g_manager->UnregisterCallback();
    EXPECT_EQ(ret, ERR_OK);
    bool result = g_manager->StartSensor(Type::TYPE_LID_OPEN);
    EXPECT_FALSE(result);
    ret = g_manager->UnregisterCallback();
    EXPECT_EQ(ret, ERR_OK);
    int32_t sensorTypeId = SENSOR_TYPE_ID_ACCELEROMETER;
    ret = g_manager->CheckSensorTypeId(sensorTypeId);
    ASSERT_TRUE(ret);
    g_manager->GetSensorTypeId(Type::TYPE_ABSOLUTE_STILL);
    g_manager->GetSensorTypeId(Type::TYPE_HORIZONTAL_POSITION);
    g_manager->GetSensorTypeId(Type::TYPE_VERTICAL_POSITION);
}

/**
 * @tc.name: DeviceStatusAlgorithmManagerTest
 * @tc.desc: test Enable Disable
 * @tc.type: FUNC
 */
HWTEST_F(DeviceStatusAlgorithmTest, DeviceStatusAlgorithmTest005, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    int32_t ret = g_manager->Enable(Type::TYPE_LID_OPEN);
    ASSERT_TRUE(ret);
}

/**
 * @tc.name: DeviceStatusAlgorithmManagerTest
 * @tc.desc: test Enable Disable
 * @tc.type: FUNC
 */
HWTEST_F(DeviceStatusAlgorithmTest, DeviceStatusAlgorithmTest006, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    bool result = g_manager->StartSensor(Type::TYPE_ABSOLUTE_STILL);
    EXPECT_TRUE(result);
    int32_t ret = g_manager->Enable(Type::TYPE_ABSOLUTE_STILL);
    ASSERT_EQ(ret, RET_OK);
    AlgoAbsoluteStill still;
    std::shared_ptr<DeviceStatusMsdpClientImpl> callback = std::make_shared<DeviceStatusMsdpClientImpl>();
    still.RegisterCallback(callback);
    ret = g_manager->Disable(Type::TYPE_ABSOLUTE_STILL);
    ASSERT_EQ(ret, RET_OK);
}

/**
 * @tc.name: DeviceStatusAlgorithmManagerTest
 * @tc.desc: test Enable Disable
 * @tc.type: FUNC
 */
HWTEST_F(DeviceStatusAlgorithmTest, DeviceStatusAlgorithmTest007, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    bool result = g_manager->StartSensor(Type::TYPE_ABSOLUTE_STILL);
    EXPECT_TRUE(result);
    int32_t ret = g_manager->Enable(Type::TYPE_HORIZONTAL_POSITION);
    ASSERT_EQ(ret, RET_OK);
    AlgoHorizontal horizontal;
    std::shared_ptr<DeviceStatusMsdpClientImpl> callback = std::make_shared<DeviceStatusMsdpClientImpl>();
    horizontal.RegisterCallback(callback);
    ret = g_manager->Disable(Type::TYPE_HORIZONTAL_POSITION);
    ASSERT_EQ(ret, RET_OK);
}

/**
 * @tc.name: DeviceStatusAlgorithmManagerTest
 * @tc.desc: test Enable Disable
 * @tc.type: FUNC
 */
HWTEST_F(DeviceStatusAlgorithmTest, DeviceStatusAlgorithmTest008, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    bool result = g_manager->StartSensor(Type::TYPE_ABSOLUTE_STILL);
    EXPECT_TRUE(result);
    int32_t ret = g_manager->Enable(Type::TYPE_VERTICAL_POSITION);
    ASSERT_EQ(ret, RET_OK);
    AlgoVertical vertical;
    std::shared_ptr<DeviceStatusMsdpClientImpl> callback = std::make_shared<DeviceStatusMsdpClientImpl>();
    vertical.RegisterCallback(callback);
    ret = g_manager->Disable(Type::TYPE_VERTICAL_POSITION);
    ASSERT_EQ(ret, RET_OK);
}

/**
 * @tc.name: DeviceStatusAlgorithmManagerTest
 * @tc.desc: test DisableCount
 * @tc.type: FUNC
 */
HWTEST_F(DeviceStatusAlgorithmTest, DeviceStatusAlgorithmTest009, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    bool result = g_manager->StartSensor(Type::TYPE_ABSOLUTE_STILL);
    EXPECT_TRUE(result);
    int32_t ret = g_manager->Enable(Type::TYPE_HORIZONTAL_POSITION);
    ASSERT_EQ(ret, RET_OK);
    AlgoVertical vertical;
    std::shared_ptr<DeviceStatusMsdpClientImpl> callback = std::make_shared<DeviceStatusMsdpClientImpl>();
    vertical.RegisterCallback(callback);
    ret = g_manager->Disable(Type::TYPE_HORIZONTAL_POSITION);
    ASSERT_EQ(ret, RET_OK);
    ret = g_manager->DisableCount(Type::TYPE_HORIZONTAL_POSITION);
    ASSERT_EQ(ret, RET_OK);
}

/**
 * @tc.name: DeviceStatusAlgorithmManagerTest
 * @tc.desc: test Enable Disable
 * @tc.type: FUNC
 */
HWTEST_F(DeviceStatusAlgorithmTest, DeviceStatusAlgorithmTest010, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    int32_t typeError = 5;
    int32_t ret = g_manager->Enable(static_cast<Type>(typeError));
    ASSERT_EQ(ret, RET_ERR);
    bool result = g_manager->StartSensor(Type::TYPE_ABSOLUTE_STILL);
    EXPECT_TRUE(result);
    ret = g_manager->Enable(static_cast<Type>(typeError));
    ASSERT_EQ(ret, RET_ERR);
}

/**
 * @tc.name: DeviceStatusAlgorithmManagerTest
 * @tc.desc: test StartSensor and UnregisterSensor
 * @tc.type: FUNC
 */
HWTEST_F(DeviceStatusAlgorithmTest, DeviceStatusAlgorithmTest011, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    bool result = g_manager->StartSensor(Type::TYPE_ABSOLUTE_STILL);
    EXPECT_TRUE(result);
    int32_t ret = g_manager->UnregisterSensor(Type::TYPE_ABSOLUTE_STILL);
    ASSERT_TRUE(ret);
}

/**
 * @tc.name: DeviceStatusAlgorithmManagerTest
 * @tc.desc: test StartSensor and UnregisterSensor
 * @tc.type: FUNC
 */
HWTEST_F(DeviceStatusAlgorithmTest, DeviceStatusAlgorithmTest012, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    bool result = g_manager->StartSensor(Type::TYPE_INVALID);
    EXPECT_FALSE(result);
}

/**
 * @tc.name: DeviceStatusAlgorithmManagerTest
 * @tc.desc: test StartSensor and UnregisterSensor
 * @tc.type: FUNC
 */
HWTEST_F(DeviceStatusAlgorithmTest, DeviceStatusAlgorithmTest013, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    bool result = g_manager->StartSensor(Type::TYPE_HORIZONTAL_POSITION);
    EXPECT_TRUE(result);
    int32_t ret = g_manager->UnregisterSensor(Type::TYPE_HORIZONTAL_POSITION);
    ASSERT_TRUE(ret);
}

/**
 * @tc.name: DeviceStatusAlgorithmManagerTest
 * @tc.desc: test StartSensor and UnregisterSensor
 * @tc.type: FUNC
 */
HWTEST_F(DeviceStatusAlgorithmTest, DeviceStatusAlgorithmTest014, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    bool result = g_manager->StartSensor(Type::TYPE_VERTICAL_POSITION);
    EXPECT_TRUE(result);
    int32_t ret = g_manager->UnregisterSensor(Type::TYPE_VERTICAL_POSITION);
    ASSERT_TRUE(ret);
}

/**
 * @tc.name: DeviceStatusAlgorithmManagerTest
 * @tc.desc: test StartSensor and UnregisterSensor
 * @tc.type: FUNC
 */
HWTEST_F(DeviceStatusAlgorithmTest, DeviceStatusAlgorithmTest015, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    int32_t typeError = 5;
    bool result = g_manager->StartSensor(static_cast<Type>(typeError));
    EXPECT_FALSE(result);
}

/**
 * @tc.name: DeviceStatusAlgorithmManagerTest
 * @tc.desc: test CheckSensorTypeId001
 * @tc.type: FUNC
 */
HWTEST_F(DeviceStatusAlgorithmTest, DeviceStatusAlgorithmTest016, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    bool result = g_manager->StartSensor(Type::TYPE_INVALID);
    EXPECT_FALSE(result);
    g_manager->GetSensorTypeId(Type::TYPE_INVALID);
    int32_t sensorTypeId = SENSOR_TYPE_ID_ACCELEROMETER;
    int32_t ret = g_manager->CheckSensorTypeId(sensorTypeId);
    ASSERT_TRUE(ret);
}

/**
 * @tc.name: DeviceStatusAlgorithmManagerTest
 * @tc.desc: test CheckSensorTypeId002
 * @tc.type: FUNC
 */
HWTEST_F(DeviceStatusAlgorithmTest, DeviceStatusAlgorithmTest017, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    bool result = g_manager->StartSensor(Type::TYPE_ABSOLUTE_STILL);
    EXPECT_TRUE(result);
    g_manager->GetSensorTypeId(Type::TYPE_ABSOLUTE_STILL);
    int32_t sensorTypeId = SENSOR_TYPE_ID_NONE;
    int32_t ret = g_manager->CheckSensorTypeId(sensorTypeId);
    ASSERT_FALSE(ret);
    ret = g_manager->UnregisterSensor(Type::TYPE_ABSOLUTE_STILL);
    ASSERT_TRUE(ret);
}

/**
 * @tc.name: DeviceStatusAlgorithmManagerTest
 * @tc.desc: test CheckSensorTypeId003
 * @tc.type: FUNC
 */
HWTEST_F(DeviceStatusAlgorithmTest, DeviceStatusAlgorithmTest018, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    bool state = g_manager->StartSensor(Type::TYPE_ABSOLUTE_STILL);
    EXPECT_TRUE(state);
    g_manager->GetSensorTypeId(Type::TYPE_ABSOLUTE_STILL);
    int32_t sensorTypeId = SENSOR_TYPE_ID_MAX;
    int32_t ret = g_manager->CheckSensorTypeId(sensorTypeId);
    EXPECT_FALSE(ret);
    ret = g_manager->UnregisterSensor(Type::TYPE_ABSOLUTE_STILL);
    ASSERT_TRUE(ret);
}

/**
 * @tc.name: DeviceStatusAlgorithmManagerTest
 * @tc.desc: test CheckSensorTypeId004
 * @tc.type: FUNC
 */
HWTEST_F(DeviceStatusAlgorithmTest, DeviceStatusAlgorithmTest019, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    bool result = g_manager->StartSensor(Type::TYPE_ABSOLUTE_STILL);
    EXPECT_TRUE(result);
    g_manager->GetSensorTypeId(Type::TYPE_ABSOLUTE_STILL);
    int32_t sensorTypeId = SENSOR_TYPE_ID_MAX;
    int32_t ret = g_manager->CheckSensorTypeId(sensorTypeId);
    EXPECT_FALSE(ret);
    ret = g_manager->UnregisterSensor(Type::TYPE_ABSOLUTE_STILL);
    ASSERT_TRUE(ret);
}

/**
 * @tc.name: DeviceStatusAlgorithmManagerTest
 * @tc.desc: test CheckSensorTypeId005
 * @tc.type: FUNC
 */
HWTEST_F(DeviceStatusAlgorithmTest, DeviceStatusAlgorithmTest020, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    bool result = g_manager->StartSensor(Type::TYPE_ABSOLUTE_STILL);
    EXPECT_TRUE(result);
    g_manager->GetSensorTypeId(Type::TYPE_ABSOLUTE_STILL);
    int32_t typeError = -100;
    int32_t ret = g_manager->CheckSensorTypeId(static_cast<SensorTypeId>(typeError));
    EXPECT_FALSE(ret);
    ret = g_manager->UnregisterSensor(Type::TYPE_ABSOLUTE_STILL);
    ASSERT_TRUE(ret);
}

/**
 * @tc.name: DeviceStatusAlgorithmManagerTest
 * @tc.desc: test GetSensorTypeId001
 * @tc.type: FUNC
 */
HWTEST_F(DeviceStatusAlgorithmTest, DeviceStatusAlgorithmTest021, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    bool result = g_manager->StartSensor(Type::TYPE_HORIZONTAL_POSITION);
    EXPECT_TRUE(result);
    g_manager->GetSensorTypeId(Type::TYPE_HORIZONTAL_POSITION);
    int32_t ret = g_manager->UnregisterSensor(Type::TYPE_HORIZONTAL_POSITION);
    ASSERT_TRUE(ret);
}

/**
 * @tc.name: DeviceStatusAlgorithmManagerTest
 * @tc.desc: test GetSensorTypeId002
 * @tc.type: FUNC
 */
HWTEST_F(DeviceStatusAlgorithmTest, DeviceStatusAlgorithmTest022, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    bool result = g_manager->StartSensor(Type::TYPE_VERTICAL_POSITION);
    EXPECT_TRUE(result);
    g_manager->GetSensorTypeId(Type::TYPE_VERTICAL_POSITION);
    int32_t ret = g_manager->UnregisterSensor(Type::TYPE_VERTICAL_POSITION);
    ASSERT_TRUE(ret);
}

/**
 * @tc.name: DeviceStatusAlgorithmManagerTest
 * @tc.desc: test GetSensorTypeId003
 * @tc.type: FUNC
 */
HWTEST_F(DeviceStatusAlgorithmTest, DeviceStatusAlgorithmTest023, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    bool result = g_manager->StartSensor(Type::TYPE_VERTICAL_POSITION);
    EXPECT_TRUE(result);
    g_manager->GetSensorTypeId(Type::TYPE_MAX);
    int32_t ret = g_manager->UnregisterSensor(Type::TYPE_VERTICAL_POSITION);
    ASSERT_TRUE(ret);
}

/**
 * @tc.name: DeviceStatusAlgorithmManagerTest
 * @tc.desc: test Enable
 * @tc.type: FUNC
 */
HWTEST_F(DeviceStatusAlgorithmTest, DeviceStatusAlgorithmTest024, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    int32_t ret = g_manager->Enable(Type::TYPE_MAX);
    EXPECT_EQ(ret, RET_ERR);
}

/**
 * @tc.name: DeviceStatusAlgorithmManagerTest
 * @tc.desc: test Disable
 * @tc.type: FUNC
 */
HWTEST_F(DeviceStatusAlgorithmTest, DeviceStatusAlgorithmTest025, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    g_manager->callAlgoNums_[Type::TYPE_ABSOLUTE_STILL] = 2;
    int32_t ret = g_manager->Disable(Type::TYPE_ABSOLUTE_STILL);
    EXPECT_EQ(ret, RET_ERR);
}

/**
 * @tc.name: DeviceStatusAlgorithmManagerTest
 * @tc.desc: test Disable
 * @tc.type: FUNC
 */
HWTEST_F(DeviceStatusAlgorithmTest, DeviceStatusAlgorithmTest026, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    int32_t ret = g_manager->Enable(Type::TYPE_MAX);
    ASSERT_TRUE(ret);
    g_manager->callAlgoNums_[Type::TYPE_MAX] = 1;
    ret = g_manager->Disable(Type::TYPE_MAX);
    EXPECT_EQ(ret, RET_OK);
}

/**
 * @tc.name: DeviceStatusAlgorithmManagerTest
 * @tc.desc: test UnregisterSensor
 * @tc.type: FUNC
 */
HWTEST_F(DeviceStatusAlgorithmTest, DeviceStatusAlgorithmTest027, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    int32_t ret = g_manager->UnregisterSensor(Type::TYPE_MAX);
    EXPECT_EQ(ret, false);
}

/**
 * @tc.name: DeviceStatusAlgorithmManagerTest
 * @tc.desc: test UnregisterSensor
 * @tc.type: FUNC
 */
HWTEST_F(DeviceStatusAlgorithmTest, DeviceStatusAlgorithmTest028, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    SENSOR_DATA_CB.user_.callback = nullptr;
    bool result = g_manager->UnregisterSensor(Type::TYPE_ABSOLUTE_STILL);
    EXPECT_FALSE(result);
}

/**
 * @tc.name: DeviceStatusAlgorithmManagerTest
 * @tc.desc: test Enable
 * @tc.type: FUNC
 */
HWTEST_F(DeviceStatusAlgorithmTest, DeviceStatusAlgorithmTest029, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<MsdpAlgoHandle> algo = std::make_shared<MsdpAlgoHandle>();
    int32_t ret = LoadAlgoLibrary(algo);
    ASSERT_EQ(ret, RET_OK);
    ASSERT_NE(algo->handle, nullptr);
    algo->pAlgorithm = algo->create();

    ret = algo->pAlgorithm->Enable(Type::TYPE_LID_OPEN);
    ASSERT_TRUE(ret);

    ret = UnloadAlgoLibrary(algo);
    ASSERT_EQ(ret, RET_OK);
}

/**
 * @tc.name: DeviceStatusAlgorithmManagerTest
 * @tc.desc: test RegisterCallback
 * @tc.type: FUNC
 */
HWTEST_F(DeviceStatusAlgorithmTest, DeviceStatusAlgorithmTest030, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    int32_t ret = g_manager->Enable(Type::TYPE_ABSOLUTE_STILL);
    EXPECT_EQ(ret, RET_OK);

    std::shared_ptr<DeviceStatusMock> callback = std::make_shared<DeviceStatusMock>();
    ret = g_manager->RegisterCallback(callback);
    EXPECT_EQ(ret, RET_OK);

    ret = g_manager->Disable(Type::TYPE_ABSOLUTE_STILL);
    EXPECT_EQ(ret, RET_OK);
}

/**
 * @tc.name: DeviceStatusAlgorithmManagerTest
 * @tc.desc: test RegisterCallback
 * @tc.type: FUNC
 */
HWTEST_F(DeviceStatusAlgorithmTest, DeviceStatusAlgorithmTest031, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    int32_t ret = g_manager->Enable(Type::TYPE_HORIZONTAL_POSITION);
    EXPECT_EQ(ret, RET_OK);

    std::shared_ptr<DeviceStatusMock> callback = std::make_shared<DeviceStatusMock>();
    ret = g_manager->RegisterCallback(callback);
    EXPECT_EQ(ret, RET_OK);

    ret = g_manager->Disable(Type::TYPE_HORIZONTAL_POSITION);
    EXPECT_EQ(ret, RET_OK);
}

/**
 * @tc.name: DeviceStatusAlgorithmManagerTest
 * @tc.desc: test RegisterCallback
 * @tc.type: FUNC
 */
HWTEST_F(DeviceStatusAlgorithmTest, DeviceStatusAlgorithmTest032, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    int32_t ret = g_manager->Enable(Type::TYPE_VERTICAL_POSITION);
    EXPECT_EQ(ret, RET_OK);

    std::shared_ptr<DeviceStatusMock> callback = std::make_shared<DeviceStatusMock>();
    ret = g_manager->RegisterCallback(callback);
    EXPECT_EQ(ret, RET_OK);

    ret = g_manager->Disable(Type::TYPE_VERTICAL_POSITION);
    EXPECT_EQ(ret, RET_OK);
}
} // namespace DeviceStatus
} // namespace Msdp
} // namespace OHOS
#endif // DEVICE_STATUS_SENSOR_ENABLE
