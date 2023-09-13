/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define private public
#include "coordination_softbus_adapter_test.h"

#include <gtest/gtest.h>

#include "coordination_sm.h"

namespace OHOS {
namespace Msdp {
namespace DeviceStatus {
using namespace testing::ext;
namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL { LOG_CORE, MSDP_DOMAIN_ID, "CoordinationSoftbusAdapterTest" };
auto g_adapter = CoordinationSoftbusAdapter::GetInstance();
bool g_sendable = false;
const std::string LOCAL_NETWORKID { "testLocalNetworkId" };
const std::string REMOTE_NETWORKID { "testRemoteNetworkId" };
const std::string ORIGIN_NETWORKID { "testRemoteNetworkId" };
} // namespace

int32_t CoordinationSoftbusAdapter::SendMsg(int32_t sessionId, const std::string &message)
{
    if (!g_sendable) {
        return RET_ERR;
    }
    return RET_OK;
}

void CoordinationSoftbusAdapterTest::SetUpTestCase() {}

void CoordinationSoftbusAdapterTest::TearDownTestCase() {}

void CoordinationSoftbusAdapterTest::SetUp() {}

void CoordinationSoftbusAdapterTest::TearDown() {}

/**
 * @tc.name: CoordinationSoftbusAdapterTest001
 * @tc.desc: Test func named StartRemoteCoordination, sessionDevMap_ is null
 * @tc.type: FUNC
 */
HWTEST_F(CoordinationSoftbusAdapterTest, CoordinationSoftbusAdapterTest001, TestSize.Level0)
{
    CALL_TEST_DEBUG;
    ASSERT_TRUE(g_adapter != nullptr);
    int32_t ret = g_adapter->StartRemoteCoordination(LOCAL_NETWORKID, REMOTE_NETWORKID, false);
    EXPECT_TRUE(ret == RET_ERR);
}

/**
 * @tc.name: CoordinationSoftbusAdapterTest002
 * @tc.desc: Test func named StartRemoteCoordination, sessionDevMap_ is not null, sendMsg is err or ok
 * @tc.type: FUNC
 */
HWTEST_F(CoordinationSoftbusAdapterTest, CoordinationSoftbusAdapterTest002, TestSize.Level0)
{
    CALL_TEST_DEBUG;
    ASSERT_TRUE(g_adapter != nullptr);
    g_adapter->sessionDevMap_[REMOTE_NETWORKID] = 1;
    g_sendable = false;
    int32_t ret = g_adapter->StartRemoteCoordination(LOCAL_NETWORKID, REMOTE_NETWORKID, false);
    EXPECT_TRUE(ret == RET_ERR);
    g_sendable = true;
    ret = g_adapter->StartRemoteCoordination(LOCAL_NETWORKID, REMOTE_NETWORKID, false);
    EXPECT_TRUE(ret == RET_OK);
    g_adapter->sessionDevMap_.clear();
}

/**
 * @tc.name: CoordinationSoftbusAdapterTest003
 * @tc.desc: Test func named StartRemoteCoordinationResult, sessionDevMap_ is null
 * @tc.type: FUNC
 */
HWTEST_F(CoordinationSoftbusAdapterTest, CoordinationSoftbusAdapterTest003, TestSize.Level0)
{
    CALL_TEST_DEBUG;
    ASSERT_TRUE(g_adapter != nullptr);
    int32_t ret = g_adapter->StartRemoteCoordinationResult(REMOTE_NETWORKID, true, REMOTE_NETWORKID, 0, 0);
    EXPECT_TRUE(ret == RET_ERR);
}

/**
 * @tc.name: CoordinationSoftbusAdapterTest004
 * @tc.desc: Test func named StartRemoteCoordinationResult, sessionDevMap_ is not null, sendMsg is err or ok
 * @tc.type: FUNC
 */
HWTEST_F(CoordinationSoftbusAdapterTest, CoordinationSoftbusAdapterTest004, TestSize.Level0)
{
    CALL_TEST_DEBUG;
    ASSERT_TRUE(g_adapter != nullptr);
    g_adapter->sessionDevMap_[REMOTE_NETWORKID] = 1;
    g_sendable = false;
    int32_t ret = g_adapter->StartRemoteCoordinationResult(REMOTE_NETWORKID, true, REMOTE_NETWORKID, 0, 0);
    EXPECT_TRUE(ret == RET_ERR);
    g_sendable = true;
    ret = g_adapter->StartRemoteCoordinationResult(REMOTE_NETWORKID, true, REMOTE_NETWORKID, 0, 0);
    EXPECT_TRUE(ret == RET_OK);
    g_adapter->sessionDevMap_.clear();
}

/**
 * @tc.name: CoordinationSoftbusAdapterTest005
 * @tc.desc: Test func named StopRemoteCoordination, sessionDevMap_ is null
 * @tc.type: FUNC
 */
HWTEST_F(CoordinationSoftbusAdapterTest, CoordinationSoftbusAdapterTest005, TestSize.Level0)
{
    CALL_TEST_DEBUG;
    ASSERT_TRUE(g_adapter != nullptr);
    int32_t ret = g_adapter->StopRemoteCoordination(REMOTE_NETWORKID, true);
    EXPECT_TRUE(ret == RET_ERR);
}

/**
 * @tc.name: CoordinationSoftbusAdapterTest006
 * @tc.desc: Test func named StopRemoteCoordination, sessionDevMap_ is not null, sendMsg is err or ok
 * @tc.type: FUNC
 */
HWTEST_F(CoordinationSoftbusAdapterTest, CoordinationSoftbusAdapterTest006, TestSize.Level0)
{
    CALL_TEST_DEBUG;
    ASSERT_TRUE(g_adapter != nullptr);
    g_adapter->sessionDevMap_[REMOTE_NETWORKID] = 1;
    g_sendable = false;
    int32_t ret = g_adapter->StopRemoteCoordination(REMOTE_NETWORKID, true);
    EXPECT_TRUE(ret == RET_ERR);
    g_sendable = true;
    ret = g_adapter->StopRemoteCoordination(REMOTE_NETWORKID, true);
    EXPECT_TRUE(ret == RET_OK);
    g_adapter->sessionDevMap_.clear();
}

/**
 * @tc.name: CoordinationSoftbusAdapterTest007
 * @tc.desc: Test func named StopRemoteCoordinationResult, sessionDevMap_ is null
 * @tc.type: FUNC
 */
HWTEST_F(CoordinationSoftbusAdapterTest, CoordinationSoftbusAdapterTest007, TestSize.Level0)
{
    CALL_TEST_DEBUG;
    ASSERT_TRUE(g_adapter != nullptr);
    int32_t ret = g_adapter->StopRemoteCoordinationResult(REMOTE_NETWORKID, true);
    EXPECT_TRUE(ret == RET_ERR);
}

/**
 * @tc.name: CoordinationSoftbusAdapterTest008
 * @tc.desc: Test func named StopRemoteCoordinationResult, sessionDevMap_ is not null, sendMsg is err or ok
 * @tc.type: FUNC
 */
HWTEST_F(CoordinationSoftbusAdapterTest, CoordinationSoftbusAdapterTest008, TestSize.Level0)
{
    CALL_TEST_DEBUG;
    ASSERT_TRUE(g_adapter != nullptr);
    g_adapter->sessionDevMap_[REMOTE_NETWORKID] = 1;
    g_sendable = false;
    int32_t ret = g_adapter->StopRemoteCoordinationResult(REMOTE_NETWORKID, true);
    EXPECT_TRUE(ret == RET_ERR);
    g_sendable = true;
    ret = g_adapter->StopRemoteCoordinationResult(REMOTE_NETWORKID, true);
    EXPECT_TRUE(ret == RET_OK);
    g_adapter->sessionDevMap_.clear();
}

/**
 * @tc.name: CoordinationSoftbusAdapterTest009
 * @tc.desc: Test func named StartCoordinationOtherResult, sessionDevMap_ is null
 * @tc.type: FUNC
 */
HWTEST_F(CoordinationSoftbusAdapterTest, CoordinationSoftbusAdapterTest009, TestSize.Level0)
{
    CALL_TEST_DEBUG;
    ASSERT_TRUE(g_adapter != nullptr);
    int32_t ret = g_adapter->StartCoordinationOtherResult(ORIGIN_NETWORKID, REMOTE_NETWORKID);
    EXPECT_TRUE(ret == RET_ERR);
}

/**
 * @tc.name: CoordinationSoftbusAdapterTest010
 * @tc.desc: Test func named StartCoordinationOtherResult, sessionDevMap_ is not null, sendMsg is err or ok
 * @tc.type: FUNC
 */
HWTEST_F(CoordinationSoftbusAdapterTest, CoordinationSoftbusAdapterTest010, TestSize.Level0)
{
    CALL_TEST_DEBUG;
    ASSERT_TRUE(g_adapter != nullptr);
    g_adapter->sessionDevMap_[ORIGIN_NETWORKID] = 1;
    g_sendable = false;
    int32_t ret = g_adapter->StartCoordinationOtherResult(ORIGIN_NETWORKID, REMOTE_NETWORKID);
    EXPECT_TRUE(ret == RET_ERR);
    g_sendable = true;
    ret = g_adapter->StartCoordinationOtherResult(ORIGIN_NETWORKID, REMOTE_NETWORKID);
    EXPECT_TRUE(ret == RET_OK);
    g_adapter->sessionDevMap_.clear();
}
} // namespace DeviceStatus
} // namespace Msdp
} // namespace OHOS