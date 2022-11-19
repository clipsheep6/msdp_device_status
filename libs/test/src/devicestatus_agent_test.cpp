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

#include <gtest/gtest.h>

#include "devicestatus_agent.h"
#include "devicestatus_data_utils.h"

using namespace testing::ext;
using namespace OHOS::Msdp;
using namespace OHOS::Msdp::DeviceStatus;
using namespace OHOS;
using namespace std;

class DeviceStatusAgentTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};

class DeviceStatusAgentListenerMockFirstClient : public DeviceStatusAgent::DeviceStatusAgentEvent {
public:
    virtual ~DeviceStatusAgentListenerMockFirstClient() {};
    bool OnEventResult(const Data& devicestatusData) override;
};

class DeviceStatusAgentListenerMockSecondClient : public DeviceStatusAgent::DeviceStatusAgentEvent {
public:
    virtual ~DeviceStatusAgentListenerMockSecondClient() {};
    bool OnEventResult(const Data& devicestatusData) override;
};

static std::shared_ptr<DeviceStatusAgent> agent1Ptr_;
static std::shared_ptr<DeviceStatusAgent> agent2Ptr_;

void DeviceStatusAgentTest::SetUpTestCase() {}

void DeviceStatusAgentTest::TearDownTestCase() {}

void DeviceStatusAgentTest::SetUp()
{
    agent1Ptr_ = std::make_shared<DeviceStatusAgent>();
    agent2Ptr_ = std::make_shared<DeviceStatusAgent>();
}

void DeviceStatusAgentTest::TearDown() {}

bool DeviceStatusAgentListenerMockFirstClient::OnEventResult(const Data& devicestatusData)
{
    GTEST_LOG_(INFO) << "agent type: " << devicestatusData.type;
    GTEST_LOG_(INFO) << "agent value: " << devicestatusData.value;
    EXPECT_TRUE(devicestatusData.type == Type::TYPE_LID_OPEN);
    return true;
}

bool DeviceStatusAgentListenerMockSecondClient::OnEventResult(const Data& devicestatusData)
{
    GTEST_LOG_(INFO) << "agent type: " << devicestatusData.type;
    GTEST_LOG_(INFO) << "agent value: " << devicestatusData.value;
    EXPECT_TRUE(devicestatusData.type == Type::TYPE_LID_OPEN);
    return true;
}

namespace {
/**
 * @tc.name: DeviceStatusAgentTest001
 * @tc.desc: test subscribing lid open event
 * @tc.type: FUNC
 */
HWTEST_F(DeviceStatusAgentTest, DeviceStatusAgentTest001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "DeviceStatusAgentTest001 start";
    std::shared_ptr<DeviceStatusAgentListenerMockFirstClient> agentEvent =
        std::make_shared<DeviceStatusAgentListenerMockFirstClient>();
    int32_t ret = agent1Ptr_->SubscribeAgentEvent(Type::TYPE_LID_OPEN,
        ActivityEvent::ENTER,
        ReportLatencyNs::Latency_INVALID,
        agentEvent);
    EXPECT_TRUE(ret == ERR_OK);
    GTEST_LOG_(INFO) << "Open and close the lid, and event will report";
    sleep(2);
    agent1Ptr_->UnsubscribeAgentEvent(Type::TYPE_LID_OPEN, ActivityEvent::ENTER);
    GTEST_LOG_(INFO) << "DeviceStatusAgentTest001 end";
}

/**
 * @tc.name: DeviceStatusAgentTest002
 * @tc.desc: test subscribing lid open event repeatedly
 * @tc.type: FUNC
 */
HWTEST_F(DeviceStatusAgentTest, DeviceStatusAgentTest002, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "DeviceStatusAgentTest002 start";
    std::shared_ptr<DeviceStatusAgentListenerMockFirstClient> agentEvent =
        std::make_shared<DeviceStatusAgentListenerMockFirstClient>();
    int32_t ret = agent1Ptr_->SubscribeAgentEvent(Type::TYPE_LID_OPEN,
        ActivityEvent::ENTER,
        ReportLatencyNs::Latency_INVALID,
        agentEvent);
    EXPECT_TRUE(ret == ERR_OK);
    GTEST_LOG_(INFO) << "Open and close the lid, and event will report";
    sleep(2);
    ret = agent1Ptr_->UnsubscribeAgentEvent(Type::TYPE_LID_OPEN, ActivityEvent::ENTER);
    EXPECT_TRUE(ret == ERR_OK);
    GTEST_LOG_(INFO) << "Open and close the lid, and event will not report";
    sleep(2);
    ret = agent1Ptr_->SubscribeAgentEvent(Type::TYPE_LID_OPEN,
        ActivityEvent::ENTER,
        ReportLatencyNs::Latency_INVALID,
        agentEvent);
    EXPECT_TRUE(ret == ERR_OK);
    GTEST_LOG_(INFO) << "Open and close the lid, and event will report again";
    sleep(2);
    agent1Ptr_->UnsubscribeAgentEvent(Type::TYPE_LID_OPEN, ActivityEvent::ENTER);
    GTEST_LOG_(INFO) << "DeviceStatusAgentTest002 end";
}

/**
 * @tc.name: DeviceStatusAgentTest003
 * @tc.desc: test subscribing lid open event for 2 client
 * @tc.type: FUNC
 */
HWTEST_F(DeviceStatusAgentTest, DeviceStatusAgentTest003, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "DeviceStatusAgentTest003 start";
    std::shared_ptr<DeviceStatusAgentListenerMockFirstClient> agentEvent1 =
        std::make_shared<DeviceStatusAgentListenerMockFirstClient>();
    std::shared_ptr<DeviceStatusAgentListenerMockSecondClient> agentEvent2 =
        std::make_shared<DeviceStatusAgentListenerMockSecondClient>();
    int32_t ret = agent1Ptr_->SubscribeAgentEvent(Type::TYPE_LID_OPEN,
        ActivityEvent::ENTER,
        ReportLatencyNs::Latency_INVALID,
        agentEvent1);
    EXPECT_TRUE(ret == ERR_OK);
    ret = agent2Ptr_->SubscribeAgentEvent(Type::TYPE_LID_OPEN,
        ActivityEvent::ENTER,
        ReportLatencyNs::Latency_INVALID,
        agentEvent2);
    EXPECT_TRUE(ret == ERR_OK);
    GTEST_LOG_(INFO) << "Open and close the lid, and event will report";
    sleep(2);
    GTEST_LOG_(INFO) << "Unsubscribe agentEvent1";
    agent1Ptr_->UnsubscribeAgentEvent(Type::TYPE_LID_OPEN, ActivityEvent::ENTER);
    sleep(2);
    GTEST_LOG_(INFO) << "Unsubscribe agentEvent2";
    agent2Ptr_->UnsubscribeAgentEvent(Type::TYPE_LID_OPEN, ActivityEvent::ENTER);
    GTEST_LOG_(INFO) << "DeviceStatusAgentTest003 end";
}

HWTEST_F(DeviceStatusAgentTest, DeviceStatusAgentTest004, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "DeviceStatusAgentTest004 start";
    std::shared_ptr<DeviceStatusAgentListenerMockFirstClient> agentEvent =
        std::make_shared<DeviceStatusAgentListenerMockFirstClient>();
    int32_t ret = agent1Ptr_->SubscribeAgentEvent(Type::TYPE_HORIZONTAL_POSITION,
        ActivityEvent::ENTER,
        ReportLatencyNs::Latency_INVALID,
        agentEvent);
    EXPECT_TRUE(ret == ERR_OK);
    GTEST_LOG_(INFO) << "Open and close the lid, and event will report";
    sleep(2);
    agent1Ptr_->UnsubscribeAgentEvent(Type::TYPE_HORIZONTAL_POSITION, ActivityEvent::ENTER);
    GTEST_LOG_(INFO) << "DeviceStatusAgentTest004 end";
}

HWTEST_F(DeviceStatusAgentTest, DeviceStatusAgentTest005, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "DeviceStatusAgentTest005 start";
    std::shared_ptr<DeviceStatusAgentListenerMockFirstClient> agentEvent =
        std::make_shared<DeviceStatusAgentListenerMockFirstClient>();
    int32_t ret = agent1Ptr_->SubscribeAgentEvent(Type::TYPE_VERTICAL_POSITION,
        ActivityEvent::ENTER,
        ReportLatencyNs::Latency_INVALID,
        agentEvent);
    EXPECT_TRUE(ret == ERR_OK);
    GTEST_LOG_(INFO) << "Open and close the lid, and event will report";
    sleep(2);
    agent1Ptr_->UnsubscribeAgentEvent(Type::TYPE_VERTICAL_POSITION, ActivityEvent::ENTER);
    GTEST_LOG_(INFO) << "DeviceStatusAgentTest005 end";
}

HWTEST_F(DeviceStatusAgentTest, DeviceStatusAgentTest006, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "DeviceStatusAgentTest006 start";
    std::shared_ptr<DeviceStatusAgentListenerMockFirstClient> agentEvent =
        std::make_shared<DeviceStatusAgentListenerMockFirstClient>();
    int32_t ret = agent1Ptr_->SubscribeAgentEvent(Type::TYPE_STILL,
        ActivityEvent::ENTER,
        ReportLatencyNs::Latency_INVALID,
        agentEvent);
    EXPECT_TRUE(ret == ERR_OK);
    GTEST_LOG_(INFO) << "Open and close the lid, and event will report";
    sleep(2);
    agent1Ptr_->UnsubscribeAgentEvent(Type::TYPE_STILL, ActivityEvent::ENTER);
    GTEST_LOG_(INFO) << "DeviceStatusAgentTest006 end";
}

HWTEST_F(DeviceStatusAgentTest, DeviceStatusAgentTest007, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "DeviceStatusAgentTest007 start";
    std::shared_ptr<DeviceStatusAgentListenerMockFirstClient> agentEvent =
        std::make_shared<DeviceStatusAgentListenerMockFirstClient>();
    int32_t ret = agent1Ptr_->SubscribeAgentEvent(Type::TYPE_INVALID,
        ActivityEvent::ENTER,
        ReportLatencyNs::Latency_INVALID,
        agentEvent);
    EXPECT_TRUE(ret == ERR_INVALID_VALUE);
    GTEST_LOG_(INFO) << "Open and close the lid, and event will report";
    sleep(2);
    ret = agent1Ptr_->UnsubscribeAgentEvent(Type::TYPE_INVALID, ActivityEvent::ENTER);
    EXPECT_TRUE(ret == ERR_INVALID_VALUE);
    GTEST_LOG_(INFO) << "DeviceStatusAgentTest007 end";
}

HWTEST_F(DeviceStatusAgentTest, DeviceStatusAgentTest008, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "DeviceStatusAgentTest008 start";
    std::shared_ptr<DeviceStatusAgentListenerMockFirstClient> agentEvent =
        std::make_shared<DeviceStatusAgentListenerMockFirstClient>();
    int32_t ret = agent1Ptr_->SubscribeAgentEvent(static_cast<Type>(10),
        ActivityEvent::ENTER,
        ReportLatencyNs::Latency_INVALID,
        agentEvent);
    EXPECT_TRUE(ret == ERR_INVALID_VALUE);
    GTEST_LOG_(INFO) << "Open and close the lid, and event will report";
    sleep(2);
    ret = agent1Ptr_->UnsubscribeAgentEvent(Type::TYPE_INVALID, ActivityEvent::ENTER);
    EXPECT_TRUE(ret == ERR_INVALID_VALUE);
    GTEST_LOG_(INFO) << "DeviceStatusAgentTest008 end";
}

HWTEST_F(DeviceStatusAgentTest, DeviceStatusAgentTest009, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "DeviceStatusAgentTest009 start";
    std::shared_ptr<DeviceStatusAgentListenerMockFirstClient> agentEvent =
        std::make_shared<DeviceStatusAgentListenerMockFirstClient>();
    int32_t ret = agent1Ptr_->SubscribeAgentEvent(Type::TYPE_INVALID,
        ActivityEvent::ENTER,
        ReportLatencyNs::Latency_INVALID,
        agentEvent);
    EXPECT_TRUE(ret == ERR_INVALID_VALUE);
    ret = agent1Ptr_->SubscribeAgentEvent(Type::TYPE_STILL,
        ActivityEvent::ENTER,
        ReportLatencyNs::Latency_INVALID,
        agentEvent);
    EXPECT_TRUE(ret == ERR_OK);
    GTEST_LOG_(INFO) << "Open and close the lid, and event will report";
    sleep(2);
    ret = agent1Ptr_->UnsubscribeAgentEvent(Type::TYPE_INVALID, ActivityEvent::ENTER);
    EXPECT_TRUE(ret == ERR_INVALID_VALUE);
    ret = agent1Ptr_->UnsubscribeAgentEvent(Type::TYPE_STILL, ActivityEvent::ENTER);
    EXPECT_TRUE(ret == ERR_OK);
    GTEST_LOG_(INFO) << "DeviceStatusAgentTest009 end";
}

HWTEST_F(DeviceStatusAgentTest, DeviceStatusAgentTest010, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "DeviceStatusAgentTest010 start";
    std::shared_ptr<DeviceStatusAgentListenerMockFirstClient> agentEvent = nullptr;
    int32_t ret = agent1Ptr_->SubscribeAgentEvent(Type::TYPE_LID_OPEN,
        ActivityEvent::ENTER,
        ReportLatencyNs::Latency_INVALID,
        agentEvent);
    EXPECT_TRUE(ret == ERR_INVALID_VALUE);
    GTEST_LOG_(INFO) << "Open and close the lid, and event will report";
    GTEST_LOG_(INFO) << "DeviceStatusAgentTest010 end";
}

HWTEST_F(DeviceStatusAgentTest, DeviceStatusAgentTest011, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "DeviceStatusAgentTest011 start";
    std::shared_ptr<DeviceStatusAgentListenerMockFirstClient> agentEvent =
        std::make_shared<DeviceStatusAgentListenerMockFirstClient>();
    std::shared_ptr<DeviceStatusAgentListenerMockFirstClient> agentEvent1 = nullptr;
    int32_t ret = agent1Ptr_->SubscribeAgentEvent(Type::TYPE_INVALID,
        ActivityEvent::ENTER,
        ReportLatencyNs::Latency_INVALID,
        agentEvent);
    EXPECT_TRUE(ret == ERR_INVALID_VALUE);

    GTEST_LOG_(INFO) << "Open and close the lid, and event will report";
    sleep(2);
    ret = agent1Ptr_->UnsubscribeAgentEvent(Type::TYPE_INVALID, ActivityEvent::ENTER);
    EXPECT_TRUE(ret == ERR_INVALID_VALUE);
    GTEST_LOG_(INFO) << "DeviceStatusAgentTest011 end";
}

HWTEST_F(DeviceStatusAgentTest, DeviceStatusAgentTest012, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "DeviceStatusAgentTest012 start";
    std::shared_ptr<DeviceStatusAgentListenerMockFirstClient> agentEvent =
        std::make_shared<DeviceStatusAgentListenerMockFirstClient>();
    std::shared_ptr<DeviceStatusAgentListenerMockFirstClient> agentEvent1 = nullptr;
    int32_t ret = agent1Ptr_->SubscribeAgentEvent(Type::TYPE_INVALID,
        ActivityEvent::ENTER,
        ReportLatencyNs::Latency_INVALID,
        agentEvent);
    EXPECT_TRUE(ret == ERR_INVALID_VALUE);
    ret = agent1Ptr_->SubscribeAgentEvent(Type::TYPE_STILL,
        ActivityEvent::ENTER,
        ReportLatencyNs::Latency_INVALID,
        agentEvent);
    EXPECT_TRUE(ret == ERR_OK);
    ret = agent1Ptr_->SubscribeAgentEvent(static_cast<Type>(10),
        ActivityEvent::ENTER,
        ReportLatencyNs::Latency_INVALID,
        agentEvent);
    EXPECT_TRUE(ret == ERR_INVALID_VALUE);
    ret = agent1Ptr_->SubscribeAgentEvent(Type::TYPE_HORIZONTAL_POSITION,
        ActivityEvent::ENTER,
        ReportLatencyNs::Latency_INVALID,
        agentEvent1);
    EXPECT_TRUE(ret == ERR_INVALID_VALUE);
    GTEST_LOG_(INFO) << "Open and close the lid, and event will report";
    sleep(2);
    ret = agent1Ptr_->UnsubscribeAgentEvent(Type::TYPE_INVALID, ActivityEvent::ENTER);
    EXPECT_TRUE(ret == ERR_INVALID_VALUE);
    ret = agent1Ptr_->UnsubscribeAgentEvent(Type::TYPE_STILL, ActivityEvent::ENTER);
    EXPECT_TRUE(ret == ERR_OK);
    ret = agent1Ptr_->UnsubscribeAgentEvent(static_cast<Type>(10), ActivityEvent::ENTER);
    EXPECT_TRUE(ret == ERR_INVALID_VALUE);
    ret = agent1Ptr_->UnsubscribeAgentEvent(Type::TYPE_VERTICAL_POSITION, ActivityEvent::ENTER);
    EXPECT_TRUE(ret == ERR_OK);
    GTEST_LOG_(INFO) << "DeviceStatusAgentTest012 end";
}

HWTEST_F(DeviceStatusAgentTest, DeviceStatusAgentTest013, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "DeviceStatusAgentTest013 start";
    std::shared_ptr<DeviceStatusAgentListenerMockFirstClient> agentEvent =
        std::make_shared<DeviceStatusAgentListenerMockFirstClient>();
    std::shared_ptr<DeviceStatusAgentListenerMockFirstClient> agentEvent1 = nullptr;
    int32_t ret = agent1Ptr_->SubscribeAgentEvent(Type::TYPE_INVALID,
        ActivityEvent::ENTER,
        ReportLatencyNs::Latency_INVALID,
        agentEvent);
    EXPECT_TRUE(ret == ERR_INVALID_VALUE);
    ret = agent1Ptr_->SubscribeAgentEvent(static_cast<Type>(10),
        ActivityEvent::ENTER,
        ReportLatencyNs::Latency_INVALID,
        agentEvent);
    EXPECT_TRUE(ret == ERR_INVALID_VALUE);
    ret = agent1Ptr_->SubscribeAgentEvent(Type::TYPE_STILL,
        ActivityEvent::ENTER,
        ReportLatencyNs::Latency_INVALID,
        agentEvent1);
    EXPECT_TRUE(ret == ERR_INVALID_VALUE);
    GTEST_LOG_(INFO) << "Open and close the lid, and event will report";
    sleep(2);
    ret = agent1Ptr_->UnsubscribeAgentEvent(Type::TYPE_INVALID, ActivityEvent::ENTER);
    EXPECT_TRUE(ret == ERR_INVALID_VALUE);
    ret = agent1Ptr_->UnsubscribeAgentEvent(Type::TYPE_STILL, ActivityEvent::ENTER);
    EXPECT_TRUE(ret == ERR_OK);
    ret = agent1Ptr_->UnsubscribeAgentEvent(static_cast<Type>(10), ActivityEvent::ENTER);
    EXPECT_TRUE(ret == ERR_INVALID_VALUE);
    GTEST_LOG_(INFO) << "DeviceStatusAgentTest013 end";
}

HWTEST_F(DeviceStatusAgentTest, DeviceStatusAgentTest014, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "DeviceStatusAgentTest014 start";
    std::shared_ptr<DeviceStatusAgentListenerMockFirstClient> agentEvent =
        std::make_shared<DeviceStatusAgentListenerMockFirstClient>();
    int32_t ret = agent1Ptr_->SubscribeAgentEvent(Type::TYPE_LID_OPEN,
        ActivityEvent::ENTER,
        ReportLatencyNs::LONG,
        agentEvent);
    EXPECT_TRUE(ret == ERR_OK);

    sptr<DeviceStatusCallbackStub> callback = new DeviceStatusAgent::DeviceStatusAgentCallback(agent1Ptr_);
    double movement = 0.2f;
    Data devicestatusData = {
        Type::TYPE_LID_OPEN,
        OnChangedValue::VALUE_ENTER,
        Status::STATUS_START,
        Action::ACTION_DOWN,
        movement
    };
    callback->OnDeviceStatusChanged(devicestatusData);
    GTEST_LOG_(INFO) << "DeviceStatusAgentTest014 end";
}
}
