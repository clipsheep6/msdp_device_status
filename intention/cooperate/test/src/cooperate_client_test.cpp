/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include <future>
#include <optional>

#include <unistd.h>
#include <utility>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "cooperate_client_test_mock.h"
#include "cooperate_client.h"
#include "cooperate_hisysevent.h"
#include "devicestatus_define.h"
#include "devicestatus_errors.h"
#include "i_hotarea_listener.h"
#include "i_event_listener.h"
#include "tunnel_client.h"

#undef LOG_TAG
#define LOG_TAG "CooperateClientTest"

namespace OHOS {
namespace Msdp {
namespace DeviceStatus {
using namespace testing::ext;
using namespace testing;
namespace {
constexpr int32_t TIME_WAIT_FOR_OP_MS { 20 };
const std::string SYSTEM_BASIC { "system_basic" };
} // namespace

class CooperateClientTest : public testing::Test {
public:
    void SetUp();
    void TearDown();
    static void SetUpTestCase();
    static void TearDownTestCase();

    static inline std::shared_ptr<CooperateClientMock> cooperateClientMock_ = nullptr;
};

void CooperateClientTest::SetUpTestCase()
{
    cooperateClientMock_ = std::make_shared<CooperateClientMock>();
    CooperateClientMock::cooperateClientInterface = cooperateClientMock_;
}

void CooperateClientTest::TearDownTestCase()
{
    CooperateClientMock::cooperateClientInterface = nullptr;
    cooperateClientMock_ = nullptr;
}

void CooperateClientTest::SetUp() {}

void CooperateClientTest::TearDown()
{
    std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP_MS));
}

class CoordinationListenerTest : public ICoordinationListener {
    public:
        CoordinationListenerTest() : ICoordinationListener() {}
        void OnCoordinationMessage(const std::string &networkId, CoordinationMessage msg) override
        {
            FI_HILOGD("Register coordination listener test");
            (void) networkId;
        };
    };

class TestEventListener final : public IEventListener {
public:
    TestEventListener() : IEventListener() {};
    ~TestEventListener() = default;

    void OnMouseLocationEvent(const std::string &networkId, const Event &event) override
    {
        (void) networkId;
        (void) event;
    };
};

class TestHotAreaListener final : public IHotAreaListener {
public:
    TestHotAreaListener() : IHotAreaListener() {};
    ~TestHotAreaListener() = default;

    void OnHotAreaMessage(int32_t displayX, int32_t displayY, HotAreaType msg, bool isEdge) override
    {
        return;
    };
};

class StreamClientTest : public StreamClient {
    public:
        StreamClientTest() = default;
        void Stop() override
        {}
        int32_t Socket() override
        {
            return RET_ERR;
        }
    };

/**
 * @tc.name: CooperateClientTest_RegisterListener_001
 * @tc.desc: On Coordination Listener
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CooperateClientTest, CooperateClientTest_RegisterListener_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<CoordinationListenerTest> consumer =
        std::make_shared<CoordinationListenerTest>();
    bool isCompatible = true;
    TunnelClient tunnel;
    CooperateClient cooperateClient;
    EXPECT_CALL(*cooperateClientMock_, AddWatch(_, _, _, _)).WillOnce(Return(RET_OK));
    int32_t ret = cooperateClient.RegisterListener(tunnel, consumer, isCompatible);
    ASSERT_EQ(ret, RET_OK);
    EXPECT_CALL(*cooperateClientMock_, AddWatch(_, _, _, _)).WillOnce(Return(RET_ERR));
    ret = cooperateClient.RegisterListener(tunnel, consumer, isCompatible);
    ASSERT_EQ(ret, RET_ERR);
    cooperateClient.isListeningProcess_ = true;
    ret = cooperateClient.RegisterListener(tunnel, consumer, isCompatible);
    ASSERT_EQ(ret, RET_ERR);
}

/**
 * @tc.name: CooperateClientTest_RegisterListener_002
 * @tc.desc: On Coordination Listener
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CooperateClientTest, CooperateClientTest_RegisterListener_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<CoordinationListenerTest> consumer =
        std::make_shared<CoordinationListenerTest>();
    bool isCompatible = true;
    TunnelClient tunnel;
    CooperateClient cooperateClient;
    EXPECT_CALL(*cooperateClientMock_, AddWatch(_, _, _, _)).WillOnce(Return(RET_OK));
    int32_t ret = cooperateClient.RegisterListener(tunnel, consumer, isCompatible);
    ASSERT_EQ(ret, RET_OK);
    ret = cooperateClient.UnregisterListener(tunnel, consumer, isCompatible);
    ASSERT_EQ(ret, RET_OK);
}

/**
 * @tc.name: CooperateClientTest_RegisterListener_003
 * @tc.desc: On Coordination Listener
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CooperateClientTest, CooperateClientTest_RegisterListener_003, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<CoordinationListenerTest> consumer =
        std::make_shared<CoordinationListenerTest>();
    bool isCompatible = true;
    TunnelClient tunnel;
    EXPECT_CALL(*cooperateClientMock_, AddWatch(_, _, _, _)).WillOnce(Return(RET_ERR));
    CooperateClient cooperateClient;
    int32_t ret = cooperateClient.RegisterListener(tunnel, consumer, isCompatible);
    ASSERT_EQ(ret, RET_ERR);
}

/**
 * @tc.name: CooperateClientTest_OnCoordinationListener_001
 * @tc.desc: On Coordination Listener
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CooperateClientTest, CooperateClientTest_OnCoordinationListener_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<CoordinationListenerTest> consumer =
        std::make_shared<CoordinationListenerTest>();
    bool isCompatible = true;
    TunnelClient tunnel;
    CooperateClient cooperateClient;
    EXPECT_CALL(*cooperateClientMock_, AddWatch(_, _, _, _)).WillOnce(Return(RET_OK));
    int32_t ret = cooperateClient.RegisterListener(tunnel, consumer, isCompatible);
    ASSERT_EQ(ret, RET_OK);
    StreamClientTest client;
    int32_t userData = 0;
    std::string networkId = "networkId";
    CoordinationMessage msg = CoordinationMessage::ACTIVATE_SUCCESS;
    MessageId msgId = MessageId::COORDINATION_ADD_LISTENER;
    NetPacket pkt(msgId);
    pkt << userData << networkId << static_cast<int32_t>(msg);
    ret = cooperateClient.OnCoordinationListener(client, pkt);
    ASSERT_EQ(ret, RET_OK);
}

/**
 * @tc.name: CooperateClientTest_OnCoordinationListener_002
 * @tc.desc: On Coordination Listener
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CooperateClientTest, CooperateClientTest_OnCoordinationListener_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<CoordinationListenerTest> consumer =
        std::make_shared<CoordinationListenerTest>();
    bool isCompatible = true;
    TunnelClient tunnel;
    CooperateClient cooperateClient;
    EXPECT_CALL(*cooperateClientMock_, AddWatch(_, _, _, _)).WillOnce(Return(RET_OK));
    int32_t ret = cooperateClient.RegisterListener(tunnel, consumer, isCompatible);
    ASSERT_EQ(ret, RET_OK);
    StreamClientTest client;
    CoordinationMessage msg = CoordinationMessage::ACTIVATE_SUCCESS;
    MessageId msgId = MessageId::COORDINATION_ADD_LISTENER;
    NetPacket pkt(msgId);
    pkt << static_cast<int32_t>(msg);
    ret = cooperateClient.OnCoordinationListener(client, pkt);
    ASSERT_EQ(ret, RET_ERR);
}

/**
 * @tc.name: CooperateClientTest_OnMouseLocationListener_001
 * @tc.desc: On Hot Area Listener
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CooperateClientTest, CooperateClientTest_OnMouseLocationListener_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<CoordinationListenerTest> consumer =
        std::make_shared<CoordinationListenerTest>();
    bool isCompatible = true;
    TunnelClient tunnel;
    CooperateClient cooperateClient;
    EXPECT_CALL(*cooperateClientMock_, AddWatch(_, _, _, _)).WillOnce(Return(RET_OK));
    int32_t ret = cooperateClient.RegisterListener(tunnel, consumer, isCompatible);
    ASSERT_EQ(ret, RET_OK);
    Event event;
    std::string networkId = "networkId";
    MessageId msgId = MessageId::COORDINATION_ADD_LISTENER;
    NetPacket pkt(msgId);
    pkt << networkId << event.displayX << event.displayY << event.displayWidth << event.displayHeight;
    StreamClientTest client;
    ret = cooperateClient.OnMouseLocationListener(client, pkt);
    ASSERT_EQ(ret, RET_OK);
}

/**
 * @tc.name: CooperateClientTest_OnMouseLocationListener_002
 * @tc.desc: On Hot Area Listener
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CooperateClientTest, CooperateClientTest_OnMouseLocationListener_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<CoordinationListenerTest> consumer =
        std::make_shared<CoordinationListenerTest>();
    bool isCompatible = true;
    TunnelClient tunnel;
    CooperateClient cooperateClient;
    EXPECT_CALL(*cooperateClientMock_, AddWatch(_, _, _, _)).WillRepeatedly(Return(RET_OK));
    int32_t ret = cooperateClient.RegisterListener(tunnel, consumer, isCompatible);
    ASSERT_EQ(ret, RET_OK);
    std::string networkId = "networkId";
    MessageId msgId = MessageId::COORDINATION_ADD_LISTENER;
    NetPacket pkt(msgId);
    StreamClientTest client;
    ret = cooperateClient.OnMouseLocationListener(client, pkt);
    ASSERT_EQ(ret, RET_ERR);
}

/**
 * @tc.name: CooperateClientTest_RegisterEventListener_001
 * @tc.desc: On Hot Area Listener
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CooperateClientTest, CooperateClientTest_RegisterEventListener_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<TestEventListener> listenerPtr = nullptr;
    TunnelClient tunnel;
    CooperateClient cooperateClient;
    std::string networkId = "networkId";
    int32_t ret = cooperateClient.RegisterEventListener(tunnel, networkId, listenerPtr);
    ASSERT_EQ(ret, 401);
}

/**
 * @tc.name: CooperateClientTest_RegisterEventListener_002
 * @tc.desc: On Hot Area Listener
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CooperateClientTest, CooperateClientTest_RegisterEventListener_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<TestEventListener> listenerPtr = std::make_shared<TestEventListener>();
    TunnelClient tunnel;
    CooperateClient cooperateClient;
    std::string networkId = "networkId";
    int32_t ret = cooperateClient.RegisterEventListener(tunnel, networkId, listenerPtr);
    ASSERT_EQ(ret, RET_OK);
    ret = cooperateClient.RegisterEventListener(tunnel, networkId, listenerPtr);
    ASSERT_EQ(ret, RET_ERR);
    networkId = "networkId2";
    EXPECT_CALL(*cooperateClientMock_, AddWatch(_, _, _, _)).WillOnce(Return(RET_ERR));
    ret = cooperateClient.RegisterEventListener(tunnel, networkId, listenerPtr);
    ASSERT_EQ(ret, RET_ERR);
}

/**
 * @tc.name: CooperateClientTest_UnregisterEventListener_001
 * @tc.desc: On Hot Area Listener
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CooperateClientTest, CooperateClientTest_UnregisterEventListener_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<TestEventListener> listenerPtr = std::make_shared<TestEventListener>();
    TunnelClient tunnel;
    CooperateClient cooperateClient;
    std::string networkId = "networkId";
    int32_t ret = cooperateClient.UnregisterEventListener(tunnel, networkId, listenerPtr);
    ASSERT_EQ(ret, RET_ERR);
    EXPECT_CALL(*cooperateClientMock_, AddWatch(_, _, _, _)).WillOnce(Return(RET_OK));
    ret = cooperateClient.RegisterEventListener(tunnel, networkId, listenerPtr);
    ASSERT_EQ(ret, RET_OK);
    EXPECT_CALL(*cooperateClientMock_, RemoveWatch(_, _, _, _)).WillOnce(Return(RET_OK));
    ret = cooperateClient.UnregisterEventListener(tunnel, networkId, listenerPtr);
    ASSERT_EQ(ret, RET_OK);
}

/**
 * @tc.name: CooperateClientTest_UnregisterEventListener_002
 * @tc.desc: On Hot Area Listener
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CooperateClientTest, CooperateClientTest_UnregisterEventListener_002, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<TestEventListener> listenerPtr = std::make_shared<TestEventListener>();
    TunnelClient tunnel;
    CooperateClient cooperateClient;
    std::string networkId = "networkId";
    EXPECT_CALL(*cooperateClientMock_, AddWatch(_, _, _, _)).WillOnce(Return(RET_OK));
    int32_t ret = cooperateClient.RegisterEventListener(tunnel, networkId, listenerPtr);
    ASSERT_EQ(ret, RET_OK);
    EXPECT_CALL(*cooperateClientMock_, RemoveWatch(_, _, _, _)).WillOnce(Return(RET_ERR));
    ret = cooperateClient.UnregisterEventListener(tunnel, networkId, listenerPtr);
    ASSERT_EQ(ret, RET_ERR);
}

/**
 * @tc.name: CooperateClientTest_AddHotAreaListener_001
 * @tc.desc: On Hot Area Listener
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CooperateClientTest, CooperateClientTest_AddHotAreaListener_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<TestHotAreaListener> listenerPtr = std::make_shared<TestHotAreaListener>();
    TunnelClient tunnel;
    CooperateClient cooperateClient;
    int32_t ret = cooperateClient.RemoveHotAreaListener(tunnel, listenerPtr);
    ASSERT_EQ(ret, RET_ERR);
    EXPECT_CALL(*cooperateClientMock_, AddWatch(_, _, _, _)).WillOnce(Return(RET_OK));
    ret = cooperateClient.AddHotAreaListener(tunnel, listenerPtr);
    ASSERT_EQ(ret, RET_OK);
    ret = cooperateClient.AddHotAreaListener(tunnel, listenerPtr);
    ASSERT_EQ(ret, RET_ERR);
    EXPECT_CALL(*cooperateClientMock_, RemoveWatch(_, _, _, _)).WillOnce(Return(RET_ERR));
    ret = cooperateClient.RemoveHotAreaListener(tunnel, listenerPtr);
    ASSERT_EQ(ret, RET_ERR);
}
} // namespace DeviceStatus
} // namespace Msdp
} // namespace OHOS
