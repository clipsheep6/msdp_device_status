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
#include <memory>
#include <optional>
#include <utility>
#include <vector>
#include <gtest/gtest.h>

#include "devicestatus_define.h"
#include "key_event.h"
#include "input_event_interceptor.h"
#include "input_event_serialization.h"

#undef LOG_TAG
#define LOG_TAG "InputEventSerializationTest"

namespace OHOS {
namespace Msdp {
namespace DeviceStatus {
using namespace testing::ext;
namespace {
NetPacket pkt(MessageId::INVALID);
} // namespace

class InputEventSerializationTest : public testing::Test {
public:
    void SetUp();
    static void SetUpTestCase();
};

void InputEventSerializationTest::SetUpTestCase() {}

void InputEventSerializationTest::SetUp() {}

/**
 * @tc.name: TestKeyEventToNetPacket
 * @tc.desc: Test KeyEventToNetPacket
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputEventSerializationTest, TestKeyEventToNetPacket, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<MMI::KeyEvent> keyEvent = MMI::KeyEvent::Create();
    ASSERT_NE(keyEvent, nullptr);
    keyEvent->SetKeyCode(OHOS::MMI::KeyEvent::KEYCODE_BACK);
    keyEvent->SetActionTime(1);
    keyEvent->SetKeyAction(OHOS::MMI::KeyEvent::KEY_ACTION_DOWN);
    OHOS::MMI::KeyEvent::KeyItem item;
    item.SetKeyCode(OHOS::MMI::KeyEvent::KEYCODE_BACK);
    item.SetDownTime(1);
    item.SetPressed(true);
    keyEvent->AddKeyItem(item);
    NetPacket packet(MessageId::DSOFTBUS_INPUT_KEY_EVENT);
    int32_t ret = Cooperate::InputEventSerialization::KeyEventToNetPacket(keyEvent, packet);
    ASSERT_EQ(ret, RET_OK);
    ret = Cooperate::InputEventSerialization::NetPacketToKeyEvent(packet, keyEvent);
    ASSERT_EQ(ret, RET_OK);
}

/**
 * @tc.name: TestSerializeInputEvent
 * @tc.desc: Test SerializeInputEvent
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputEventSerializationTest, TestSerializeInputEvent, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<MMI::KeyEvent> keyEvent = MMI::KeyEvent::Create();
    ASSERT_NE(keyEvent, nullptr);
    keyEvent->SetKeyCode(OHOS::MMI::KeyEvent::KEYCODE_BACK);
    keyEvent->SetActionTime(1);
    keyEvent->SetKeyAction(OHOS::MMI::KeyEvent::KEY_ACTION_DOWN);
    OHOS::MMI::KeyEvent::KeyItem item;
    item.SetKeyCode(OHOS::MMI::KeyEvent::KEYCODE_BACK);
    item.SetDownTime(1);
    item.SetPressed(true);
    keyEvent->AddKeyItem(item);
    NetPacket packet(MessageId::DSOFTBUS_INPUT_KEY_EVENT);
    int32_t ret = Cooperate::InputEventSerialization::SerializeInputEvent(keyEvent, packet);
    ASSERT_EQ(ret, RET_OK);
    ret = Cooperate::InputEventSerialization::DeserializeInputEvent(packet, keyEvent);
    ASSERT_EQ(ret, RET_OK);
}

/**
 * @tc.name: TestReadFunctionKeys
 * @tc.desc: Test ReadFunctionKeys
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputEventSerializationTest, TestReadFunctionKeys, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<MMI::KeyEvent> keyEvent = MMI::KeyEvent::Create();
    ASSERT_NE(keyEvent, nullptr);
    keyEvent->SetKeyCode(OHOS::MMI::KeyEvent::KEYCODE_BACK);
    keyEvent->SetActionTime(1);
    keyEvent->SetKeyAction(OHOS::MMI::KeyEvent::KEY_ACTION_DOWN);
    OHOS::MMI::KeyEvent::KeyItem item;
    item.SetKeyCode(OHOS::MMI::KeyEvent::KEYCODE_BACK);
    item.SetDownTime(1);
    item.SetPressed(true);
    keyEvent->AddKeyItem(item);
    NetPacket packet(MessageId::DSOFTBUS_INPUT_KEY_EVENT);
    Cooperate::InputEventSerialization::ReadFunctionKeys(packet, keyEvent);
}

/**
 * @tc.name: TestSwitchEventToNetPacket
 * @tc.desc: Test SwitchEventToNetPacket
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputEventSerializationTest, TestSwitchEventToNetPacket, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<MMI::SwitchEvent> switchEvent = std::make_shared<MMI::SwitchEvent>(0);
    ASSERT_NE(switchEvent, nullptr);
    NetPacket packet(MessageId::DSOFTBUS_INPUT_KEY_EVENT);
    int32_t ret = Cooperate::InputEventSerialization::SwitchEventToNetPacket(switchEvent, packet);
    ASSERT_EQ(ret, RET_OK);
    ret = Cooperate::InputEventSerialization::NetPacketToSwitchEvent(packet, switchEvent);
    ASSERT_EQ(ret, RET_OK);
}

/**
 * @tc.name: TestSerializeBaseInfo
 * @tc.desc: Test SerializeBaseInfo
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputEventSerializationTest, TestSerializeBaseInfo, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<MMI::PointerEvent> pointerEvent = MMI::PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    NetPacket packet(MessageId::DSOFTBUS_INPUT_KEY_EVENT);
    int32_t ret = Cooperate::InputEventSerialization::SerializeBaseInfo(pointerEvent, packet);
    ASSERT_EQ(ret, RET_OK);
    ret = Cooperate::InputEventSerialization::DeserializeBaseInfo(packet, pointerEvent);
    ASSERT_EQ(ret, RET_OK);
}

/**
 * @tc.name: TestSerializeAxes
 * @tc.desc: Test SerializeAxes
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputEventSerializationTest, TestSerializeAxes, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<MMI::PointerEvent> pointerEvent = MMI::PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    NetPacket packet(MessageId::DSOFTBUS_INPUT_KEY_EVENT);
    int32_t ret = Cooperate::InputEventSerialization::SerializeAxes(pointerEvent, packet);
    ASSERT_EQ(ret, RET_OK);
    ret = Cooperate::InputEventSerialization::DeserializeAxes(packet, pointerEvent);
    ASSERT_EQ(ret, RET_OK);
}

/**
 * @tc.name: TestSerializePressedButtons
 * @tc.desc: Test SerializePressedButtons
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputEventSerializationTest, TestSerializePressedButtons, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<MMI::PointerEvent> pointerEvent = MMI::PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    NetPacket packet(MessageId::DSOFTBUS_INPUT_KEY_EVENT);
    int32_t ret = Cooperate::InputEventSerialization::SerializePressedButtons(pointerEvent, packet);
    ASSERT_EQ(ret, RET_OK);
    ret = Cooperate::InputEventSerialization::DeserializePressedButtons(packet, pointerEvent);
    ASSERT_EQ(ret, RET_OK);
}

/**
 * @tc.name: TestSerializePointerItem
 * @tc.desc: Test SerializePointerItem
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputEventSerializationTest, TestSerializePointerItem, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<MMI::PointerEvent> pointerEvent = MMI::PointerEvent::Create();
    OHOS::MMI::PointerEvent::PointerItem item;
    pointerEvent->AddPointerItem(item);
    ASSERT_NE(pointerEvent, nullptr);
    NetPacket packet(MessageId::DSOFTBUS_INPUT_KEY_EVENT);
    int32_t ret = Cooperate::InputEventSerialization::SerializePointerItem(packet, item);
    ASSERT_EQ(ret, RET_OK);
    ret = Cooperate::InputEventSerialization::DeserializePointerItem(packet, item);
    ASSERT_EQ(ret, RET_OK);
}

/**
 * @tc.name: TestSerializePointers
 * @tc.desc: Test SerializePointers
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputEventSerializationTest, TestSerializePointers, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<MMI::PointerEvent> pointerEvent = MMI::PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    NetPacket packet(MessageId::DSOFTBUS_INPUT_KEY_EVENT);
    int32_t ret = Cooperate::InputEventSerialization::SerializePointers(pointerEvent, packet);
    ASSERT_EQ(ret, RET_OK);
    ret = Cooperate::InputEventSerialization::DeserializePointers(packet, pointerEvent);
    ASSERT_EQ(ret, RET_OK);
}

/**
 * @tc.name: TestSerializePressedKeys
 * @tc.desc: Test SerializePressedKeys
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputEventSerializationTest, TestSerializePressedKeys, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<MMI::PointerEvent> pointerEvent = MMI::PointerEvent::Create();
    std::vector<int32_t> pressedKeys { OHOS::MMI::KeyEvent::KEYCODE_CTRL_LEFT };
    pointerEvent->SetPressedKeys(pressedKeys);
    ASSERT_NE(pointerEvent, nullptr);
    NetPacket packet(MessageId::DSOFTBUS_INPUT_KEY_EVENT);
    int32_t ret = Cooperate::InputEventSerialization::SerializePressedKeys(pointerEvent, packet);
    ASSERT_EQ(ret, RET_OK);
    ret = Cooperate::InputEventSerialization::DeserializePressedKeys(packet, pointerEvent);
    ASSERT_EQ(ret, RET_OK);
}

/**
 * @tc.name: TestSerializeBuffer
 * @tc.desc: Test SerializeBuffer
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputEventSerializationTest, TestSerializeBuffer, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<MMI::PointerEvent> pointerEvent = MMI::PointerEvent::Create();
    std::vector<uint8_t> enhanceData;
    pointerEvent->SetBuffer(enhanceData);
    ASSERT_NE(pointerEvent, nullptr);
    NetPacket packet(MessageId::DSOFTBUS_INPUT_KEY_EVENT);
    int32_t ret = Cooperate::InputEventSerialization::SerializeBuffer(pointerEvent, packet);
    ASSERT_EQ(ret, RET_OK);
    ret = Cooperate::InputEventSerialization::DeserializeBuffer(packet, pointerEvent);
    ASSERT_EQ(ret, RET_OK);
}

/**
 * @tc.name: TestMarshalling
 * @tc.desc: Test Marshalling
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputEventSerializationTest, TestMarshalling, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<MMI::PointerEvent> pointerEvent = MMI::PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    NetPacket packet(MessageId::DSOFTBUS_INPUT_KEY_EVENT);
    int32_t ret = Cooperate::InputEventSerialization::Marshalling(pointerEvent, packet);
    ASSERT_EQ(ret, RET_OK);
    ret = Cooperate::InputEventSerialization::Unmarshalling(packet, pointerEvent);
    ASSERT_EQ(ret, RET_OK);
}

/**
 * @tc.name: TestNetPacketToKeyEvent_01
 * @tc.desc: Test NetPacketToKeyEvent
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputEventSerializationTest, TestNetPacketToKeyEvent_01, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<MMI::KeyEvent> keyEvent = MMI::KeyEvent::Create();
    ASSERT_NE(keyEvent, nullptr);
    int32_t ret = Cooperate::InputEventSerialization::NetPacketToKeyEvent(pkt, keyEvent);
    ASSERT_EQ(ret, RET_ERR);
}

/**
 * @tc.name: TestSwitchEventToNetPacket_01
 * @tc.desc: Test SwitchEventToNetPacket
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputEventSerializationTest, TestSwitchEventToNetPacket_01, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<MMI::SwitchEvent> switchEvent = std::make_shared<MMI::SwitchEvent>(0);
    ASSERT_NE(switchEvent, nullptr);
    int32_t ret = Cooperate::InputEventSerialization::SwitchEventToNetPacket(switchEvent, pkt);
    ASSERT_EQ(ret, RET_ERR);
    ret = Cooperate::InputEventSerialization::NetPacketToSwitchEvent(pkt, switchEvent);
    ASSERT_EQ(ret, RET_ERR);
}

/**
 * @tc.name: TestSerializeInputEvent_01
 * @tc.desc: Test SerializeInputEvent
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputEventSerializationTest, TestSerializeInputEvent_01, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<MMI::KeyEvent> keyEvent = MMI::KeyEvent::Create();
    ASSERT_NE(keyEvent, nullptr);
    int32_t ret = Cooperate::InputEventSerialization::SerializeInputEvent(keyEvent, pkt);
    ASSERT_EQ(ret, RET_ERR);
    ret = Cooperate::InputEventSerialization::DeserializeInputEvent(pkt, keyEvent);
    ASSERT_EQ(ret, RET_ERR);
}

/**
 * @tc.name: TestSerializeBaseInfo_01
 * @tc.desc: Test SerializeBaseInfo
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputEventSerializationTest, TestSerializeBaseInfo_01, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<MMI::PointerEvent> pointerEvent = MMI::PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    int32_t ret = Cooperate::InputEventSerialization::SerializeBaseInfo(pointerEvent, pkt);
    ASSERT_EQ(ret, RET_ERR);
    ret = Cooperate::InputEventSerialization::DeserializeBaseInfo(pkt, pointerEvent);
    ASSERT_EQ(ret, RET_ERR);
}

/**
 * @tc.name: TestSerializeAxes_01
 * @tc.desc: Test SerializeAxes
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputEventSerializationTest, TestSerializeAxes_01, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<MMI::PointerEvent> pointerEvent = MMI::PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    int32_t ret = Cooperate::InputEventSerialization::SerializeAxes(pointerEvent, pkt);
    ASSERT_EQ(ret, RET_ERR);
    ret = Cooperate::InputEventSerialization::DeserializeAxes(pkt, pointerEvent);
    ASSERT_EQ(ret, RET_ERR);
}

/**
 * @tc.name: TestSerializePressedButtons_01
 * @tc.desc: Test SerializePressedButtons
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputEventSerializationTest, TestSerializePressedButtons_01, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<MMI::PointerEvent> pointerEvent = MMI::PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    int32_t ret = Cooperate::InputEventSerialization::SerializePressedButtons(pointerEvent, pkt);
    ASSERT_EQ(ret, RET_ERR);
    ret = Cooperate::InputEventSerialization::DeserializePressedButtons(pkt, pointerEvent);
    ASSERT_EQ(ret, RET_ERR);
}

/**
 * @tc.name: TestSerializePointerItem_01
 * @tc.desc: Test SerializePointerItem
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputEventSerializationTest, TestSerializePointerItem_01, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<MMI::PointerEvent> pointerEvent = MMI::PointerEvent::Create();
    OHOS::MMI::PointerEvent::PointerItem item;
    pointerEvent->AddPointerItem(item);
    ASSERT_NE(pointerEvent, nullptr);
    int32_t ret = Cooperate::InputEventSerialization::SerializePointerItem(pkt, item);
    ASSERT_EQ(ret, RET_ERR);
    ret = Cooperate::InputEventSerialization::DeserializePointerItem(pkt, item);
    ASSERT_EQ(ret, RET_ERR);
}

/**
 * @tc.name: TestSerializePointers_01
 * @tc.desc: Test SerializePointers
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputEventSerializationTest, TestSerializePointers_01, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<MMI::PointerEvent> pointerEvent = MMI::PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    int32_t ret = Cooperate::InputEventSerialization::SerializePointers(pointerEvent, pkt);
    ASSERT_EQ(ret, RET_ERR);
    ret = Cooperate::InputEventSerialization::DeserializePointers(pkt, pointerEvent);
    ASSERT_EQ(ret, RET_ERR);
}

/**
 * @tc.name: TestSerializeBuffer_01
 * @tc.desc: Test SerializeBuffer
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputEventSerializationTest, TestSerializeBuffer_01, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<MMI::PointerEvent> pointerEvent = MMI::PointerEvent::Create();
    std::vector<uint8_t> enhanceData;
    pointerEvent->SetBuffer(enhanceData);
    ASSERT_NE(pointerEvent, nullptr);
    int32_t ret = Cooperate::InputEventSerialization::SerializeBuffer(pointerEvent, pkt);
    ASSERT_EQ(ret, RET_ERR);
    ret = Cooperate::InputEventSerialization::DeserializeBuffer(pkt, pointerEvent);
    ASSERT_EQ(ret, RET_ERR);
}

/**
 * @tc.name: TestSerializePressedKeys_01
 * @tc.desc: Test SerializePressedKeys
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputEventSerializationTest, TestSerializePressedKeys_01, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<MMI::PointerEvent> pointerEvent = MMI::PointerEvent::Create();
    std::vector<int32_t> pressedKeys { OHOS::MMI::KeyEvent::KEYCODE_CTRL_LEFT };
    pointerEvent->SetPressedKeys(pressedKeys);
    ASSERT_NE(pointerEvent, nullptr);
    int32_t ret = Cooperate::InputEventSerialization::SerializePressedKeys(pointerEvent, pkt);
    ASSERT_EQ(ret, RET_ERR);
    ret = Cooperate::InputEventSerialization::DeserializePressedKeys(pkt, pointerEvent);
    ASSERT_EQ(ret, RET_ERR);
}

/**
 * @tc.name: TestKeyEventToNetPacket_01
 * @tc.desc: Test KeyEventToNetPacket
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputEventSerializationTest, TestKeyEventToNetPacket_01, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<MMI::KeyEvent> keyEvent = MMI::KeyEvent::Create();
    ASSERT_NE(keyEvent, nullptr);
    int32_t ret = Cooperate::InputEventSerialization::KeyEventToNetPacket(keyEvent, pkt);
    ASSERT_EQ(ret, RET_ERR);
}

/**
 * @tc.name: TestMarshalling_01
 * @tc.desc: Test Marshalling
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputEventSerializationTest, TestMarshalling_01, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<MMI::PointerEvent> pointerEvent = MMI::PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    int32_t ret = Cooperate::InputEventSerialization::Marshalling(pointerEvent, pkt);
    ASSERT_EQ(ret, RET_ERR);
}

/**
 * @tc.name: TestUnmarshalling_01
 * @tc.desc: Test Unmarshalling
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InputEventSerializationTest, TestUnmarshalling_01, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<MMI::PointerEvent> pointerEvent = MMI::PointerEvent::Create();
    ASSERT_NE(pointerEvent, nullptr);
    int32_t ret = Cooperate::InputEventSerialization::Unmarshalling(pkt, pointerEvent);
    ASSERT_EQ(ret, RET_ERR);
}
} // namespace DeviceStatus
} // namespace Msdp
} // namespace OHOS