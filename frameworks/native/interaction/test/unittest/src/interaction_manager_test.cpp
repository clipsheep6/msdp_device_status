/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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
#include <utility>
#include <vector>

#include <unistd.h>

#include <gtest/gtest.h>
#include "input_device.h"
#include "input_manager.h"
#include "pointer_event.h"
#include "securec.h"

#include "coordination_message.h"
#include "devicestatus_define.h"
#include "devicestatus_errors.h"
#include "interaction_manager.h"

namespace OHOS {
namespace Msdp {
namespace DeviceStatus {
using namespace testing::ext;
namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL { LOG_CORE, MSDP_DOMAIN_ID, "InteractionManagerTest" };
constexpr int32_t TIME_WAIT_FOR_OP_MS { 20 };
constexpr int32_t TIME_WAIT_FOR_INJECT_MS { 80 };
constexpr int32_t TIME_WAIT_FOR_TOUCH_DOWN_MS { 1000 };
constexpr int32_t PROMISE_WAIT_SPAN_MS { 2000 };
constexpr int32_t TEST_PIXEL_MAP_WIDTH { 200 };
constexpr int32_t TEST_PIXEL_MAP_HEIGHT { 200 };
constexpr int32_t MAX_PIXEL_MAP_WIDTH { 600 };
constexpr int32_t MAX_PIXEL_MAP_HEIGHT { 600 };
constexpr int32_t MOUSE_POINTER_ID { 0 };
constexpr int32_t TOUCH_POINTER_ID { 1 };
constexpr int32_t DISPLAY_ID { 0 };
constexpr int32_t DRAG_SRC_X { 0 };
constexpr int32_t DRAG_SRC_Y { 0 };
constexpr int32_t DRAG_DST_X { 500 };
constexpr int32_t DRAG_DST_Y { 500 };
constexpr int32_t DRAG_NUM { 1 };
constexpr int32_t INT32_BYTE { 4 };
constexpr uint32_t DEFAULT_ICON_COLOR { 0xFF };
constexpr bool HAS_CANCELED_ANIMATION { true };
constexpr bool HAS_CUSTOM_ANIMATION { true };
constexpr int32_t MOVE_STEP { 10 };
const std::string UD_KEY { "Unified data key" };
int32_t g_deviceMouseId { -1 };
int32_t g_deviceTouchId { -1 };
} // namespace

class InteractionManagerTest : public testing::Test {
public:
    void SetUp();
    void TearDown();
    static void SetUpTestCase();
    static std::vector<int32_t> GetInputDeviceIds();
    static std::shared_ptr<MMI::InputDevice> GetDevice(int32_t deviceId);
    static std::pair<int32_t, int32_t> GetMouseAndTouch();
    static std::shared_ptr<Media::PixelMap> CreatePixelMap(int32_t width, int32_t height);
    static std::optional<DragData> CreateDragData(const std::pair<int32_t, int32_t> &pixelMapSize, int32_t sourceType,
        int32_t pointerId, int32_t displayId, const std::pair<int32_t, int32_t> &location);
    static MMI::PointerEvent::PointerItem CreatePointerItem(int32_t pointerId,
        int32_t deviceId, const std::pair<int32_t, int32_t> &displayLocation, bool isPressed);
    static std::shared_ptr<MMI::PointerEvent> SetupPointerEvent(const std::pair<int32_t, int32_t> &displayLocation,
        int32_t action, int32_t sourceType, int32_t pointerId, bool isPressed);
    static void SimulateDownEvent(const std::pair<int32_t, int32_t> &location, int32_t sourceType, int32_t pointerId);
    static void SimulateMoveEvent(const std::pair<int32_t, int32_t> &srcLocation, const std::pair<int32_t, int32_t>
        &dstLocation, int32_t sourceType, int32_t pointerId, bool isPressed);
    static int32_t TestAddMonitor(std::shared_ptr<MMI::IInputEventConsumer> consumer);
    static void TestRemoveMonitor(int32_t monitorId);
};

class DragListenerTest : public IDragListener {
public:
    DragListenerTest() {}
    explicit DragListenerTest(std::string name) : moduleName_(name) {}
    void OnDragMessage(DragState state) override
    {
        if (moduleName_.empty()) {
            moduleName_ = std::string("DragListenerTest");
        }
        FI_HILOGD("%{public}s, state:%{public}s", moduleName_.c_str(), PrintDragMessage(state).c_str());
    }
private:
    std::string PrintDragMessage(DragState state)
    {
        switch (state) {
            case DragState::ERROR: {
                return std::string("error");
            }
            case DragState::START: {
                return std::string("start");
            }
            case DragState::STOP: {
                return std::string("stop");
            }
            case DragState::CANCEL: {
                return std::string("cancel");
            }
            default: {
                return std::string("unknow");
            }
        }
    }
private:
    std::string moduleName_;
};

std::vector<int32_t> InteractionManagerTest::GetInputDeviceIds()
{
    std::vector<int32_t> realDeviceIds;
    auto callback = [&realDeviceIds](std::vector<int32_t>& deviceIds) {
        realDeviceIds = deviceIds;
    };
    int32_t ret = MMI::InputManager::GetInstance()->GetDeviceIds(callback);
    if (ret != RET_OK) {
        FI_HILOGE("Get device ids failed");
        return {};
    }
    return realDeviceIds;
}

std::shared_ptr<MMI::InputDevice> InteractionManagerTest::GetDevice(int32_t deviceId)
{
    std::shared_ptr<MMI::InputDevice> inputDevice;
    auto callback = [&inputDevice](std::shared_ptr<MMI::InputDevice> device) {
        inputDevice = device;
    };
    int32_t ret = MMI::InputManager::GetInstance()->GetDevice(deviceId, callback);
    if (ret != RET_OK || inputDevice == nullptr) {
        FI_HILOGE("Get device failed");
        return nullptr;
    }
    return inputDevice;
}

std::pair<int32_t, int32_t> InteractionManagerTest::GetMouseAndTouch()
{
    std::vector<int32_t> deviceIds = GetInputDeviceIds();
    std::pair<int32_t, int32_t> mouseAndTouch { -1, -1 };
    for (const auto& id : deviceIds) {
        std::shared_ptr<MMI::InputDevice> device = GetDevice(id);
        CHKPC(device);
        if (device->HasCapability(MMI::InputDeviceCapability::INPUT_DEV_CAP_POINTER)) {
            mouseAndTouch.first = device->GetId();
        }
        if (device->HasCapability(MMI::InputDeviceCapability::INPUT_DEV_CAP_TOUCH)) {
            mouseAndTouch.second = device->GetId();
        }
    }
    return mouseAndTouch;
}

void InteractionManagerTest::SetUpTestCase()
{
    auto mouseAndTouch = GetMouseAndTouch();
    g_deviceMouseId = mouseAndTouch.first;
    g_deviceTouchId = mouseAndTouch.second;
}

void InteractionManagerTest::SetUp() {}

void InteractionManagerTest::TearDown()
{
    std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP_MS));
}

std::shared_ptr<Media::PixelMap> InteractionManagerTest::CreatePixelMap(int32_t width, int32_t height)
{
    CALL_DEBUG_ENTER;
    OHOS::Media::InitializationOptions opts;
    opts.size.width = width;
    opts.size.height = height;
    opts.pixelFormat = Media::PixelFormat::BGRA_8888;
    opts.alphaType = Media::AlphaType::IMAGE_ALPHA_TYPE_OPAQUE;
    opts.scaleMode = Media::ScaleMode::FIT_TARGET_SIZE;

    int32_t colorLen = width * height;
    uint32_t *colorPixels = new (std::nothrow) uint32_t[colorLen];
    CHKPP(colorPixels);
    int32_t colorByteCount = colorLen * INT32_BYTE;
    if (memset_s(colorPixels, colorByteCount, DEFAULT_ICON_COLOR, colorByteCount) != EOK) {
        FI_HILOGE("memset_s failed");
        delete[] colorPixels;
        return nullptr;
    }
    std::shared_ptr<Media::PixelMap> pixelMap = Media::PixelMap::Create(colorPixels, colorLen, opts);
    if (pixelMap == nullptr) {
        FI_HILOGE("Create pixelMap failed");
        delete[] colorPixels;
        return nullptr;
    }
    delete[] colorPixels;
    return pixelMap;
}

std::optional<DragData> InteractionManagerTest::CreateDragData(const std::pair<int32_t, int32_t> &pixelMapSize,
    int32_t sourceType, int32_t pointerId, int32_t displayId, const std::pair<int32_t, int32_t> &location)
{
    CALL_DEBUG_ENTER;
    std::shared_ptr<Media::PixelMap> pixelMap = CreatePixelMap(pixelMapSize.first, pixelMapSize.second);
    if (pixelMap == nullptr) {
        FI_HILOGE("Create pixelmap failed");
        return std::nullopt;
    }
    DragData dragData;
    dragData.shadowInfo.pixelMap = pixelMap;
    dragData.shadowInfo.x = 0;
    dragData.shadowInfo.y = 0;
    dragData.buffer = std::vector<uint8_t>(MAX_BUFFER_SIZE, 0);
    dragData.udKey = UD_KEY;
    dragData.sourceType = sourceType;
    dragData.pointerId = pointerId;
    dragData.dragNum = DRAG_NUM;
    dragData.displayX = location.first;
    dragData.displayY = location.second;
    dragData.displayId = displayId;
    dragData.hasCanceledAnimation = HAS_CANCELED_ANIMATION;
    return dragData;
}

MMI::PointerEvent::PointerItem InteractionManagerTest::CreatePointerItem(int32_t pointerId, int32_t deviceId,
    const std::pair<int32_t, int32_t> &displayLocation, bool isPressed)
{
    MMI::PointerEvent::PointerItem item;
    item.SetPointerId(pointerId);
    item.SetDeviceId(deviceId);
    item.SetDisplayX(displayLocation.first);
    item.SetDisplayY(displayLocation.second);
    item.SetPressed(isPressed);
    return item;
}

std::shared_ptr<MMI::PointerEvent> InteractionManagerTest::SetupPointerEvent(const std::pair<int32_t, int32_t>
    &displayLocation, int32_t action, int32_t sourceType, int32_t pointerId, bool isPressed)
{
    CALL_DEBUG_ENTER;
    auto pointerEvent = MMI::PointerEvent::Create();
    CHKPP(pointerEvent);
    pointerEvent->SetPointerAction(action);
    pointerEvent->SetSourceType(sourceType);
    pointerEvent->SetPointerId(pointerId);
    MMI::PointerEvent::PointerItem curPointerItem;
    if (sourceType == MMI::PointerEvent::SOURCE_TYPE_MOUSE) {
        curPointerItem = CreatePointerItem(pointerId, g_deviceMouseId, displayLocation, isPressed);
    } else if (sourceType == MMI::PointerEvent::SOURCE_TYPE_TOUCHSCREEN) {
        curPointerItem = CreatePointerItem(pointerId, g_deviceTouchId, displayLocation, isPressed);
    } else {
        FI_HILOGE("Unknow sourceType:%{public}d", sourceType);
        return nullptr;
    }
    pointerEvent->AddPointerItem(curPointerItem);
    return pointerEvent;
}

void InteractionManagerTest::SimulateDownEvent(const std::pair<int32_t, int32_t> &location, int32_t sourceType,
    int32_t pointerId)
{
    CALL_DEBUG_ENTER;
    std::shared_ptr<MMI::PointerEvent> pointerEvent =
        SetupPointerEvent(location, MMI::PointerEvent::POINTER_ACTION_DOWN, sourceType, pointerId, true);
    CHKPV(pointerEvent);
    FI_HILOGD("TEST:sourceType:%{public}d, pointerId:%{public}d, pointerAction:%{public}d",
        pointerEvent->GetSourceType(), pointerEvent->GetPointerId(), pointerEvent->GetPointerAction());
    MMI::InputManager::GetInstance()->SimulateInputEvent(pointerEvent);
}

void InteractionManagerTest::SimulateMoveEvent(const std::pair<int32_t, int32_t> &srcLocation,
    const std::pair<int32_t, int32_t> &dstLocation, int32_t sourceType, int32_t pointerId, bool isPressed)
{
    CALL_DEBUG_ENTER;
    int32_t srcX = srcLocation.first;
    int32_t srcY = srcLocation.second;
    int32_t dstX = dstLocation.first;
    int32_t dstY = dstLocation.second;
    std::vector<std::pair<int32_t, int32_t>> coordinates;
    if (dstX == srcX) {
        for (int32_t y = srcY; y <= dstY; y += MOVE_STEP) {
            coordinates.push_back({srcX, y});
        }
    } else {
        double ratio = (dstY - srcY) * 1.0 / (dstX - srcX);
        for (int32_t x = srcX; x < dstX; x += MOVE_STEP) {
            coordinates.push_back({x, srcY + static_cast<int32_t>(ratio * (x - srcX))});
        }
        coordinates.push_back({dstX, dstY});
    }
    for (const auto& pointer : coordinates) {
        std::shared_ptr<MMI::PointerEvent> pointerEvent =
            SetupPointerEvent(pointer, MMI::PointerEvent::POINTER_ACTION_MOVE, sourceType, pointerId, isPressed);
        CHKPC(pointerEvent);
        FI_HILOGD("TEST:sourceType:%{public}d, pointerId:%{public}d, pointerAction:%{public}d",
            pointerEvent->GetSourceType(), pointerEvent->GetPointerId(), pointerEvent->GetPointerAction());
        MMI::InputManager::GetInstance()->SimulateInputEvent(pointerEvent);
        std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_INJECT_MS));
    }
}

int32_t InteractionManagerTest::TestAddMonitor(std::shared_ptr<MMI::IInputEventConsumer> consumer)
{
    CHKPR(consumer, RET_ERR);
    return MMI::InputManager::GetInstance()->AddMonitor(consumer);
}

void InteractionManagerTest::TestRemoveMonitor(int32_t monitorId)
{
    MMI::InputManager::GetInstance()->RemoveMonitor(monitorId);
}

class InputEventCallbackTest : public MMI::IInputEventConsumer {
public:
    InputEventCallbackTest() {}
    explicit InputEventCallbackTest(std::function<void()> callback) : callback_(callback) {}
    void OnInputEvent(std::shared_ptr<MMI::KeyEvent> keyEvent) const override {};
    void OnInputEvent(std::shared_ptr<MMI::PointerEvent> pointerEvent) const override;
    void OnInputEvent(std::shared_ptr<MMI::AxisEvent> axisEvent) const override {};
private:
    std::function<void()> callback_ { nullptr };
};

void InputEventCallbackTest::OnInputEvent(std::shared_ptr<MMI::PointerEvent> pointerEvent) const
{
    CALL_DEBUG_ENTER;
    ASSERT_TRUE(pointerEvent != nullptr);
    auto pointerAction = pointerEvent->GetPointerAction();
    ASSERT_TRUE(pointerAction == MMI::PointerEvent::POINTER_ACTION_PULL_MOVE ||
                pointerAction == MMI::PointerEvent::POINTER_ACTION_PULL_UP);
    ASSERT_TRUE(!pointerEvent->GetBuffer().empty());
    MMI::PointerEvent::PointerItem pointerItem;
    pointerEvent->GetPointerItem(pointerEvent->GetPointerId(), pointerItem);
    if (callback_ != nullptr && pointerItem.GetDisplayX() == DRAG_DST_X && pointerItem.GetDisplayY() == DRAG_DST_Y) {
        callback_();
    }
}

/**
 * @tc.name: InteractionManagerTest_RegisterCoordinationListener_001
 * @tc.desc: Register coordination listener
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InteractionManagerTest, InteractionManagerTest_RegisterCoordinationListener_001, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<ICoordinationListener> consumer = nullptr;
    int32_t ret = InteractionManager::GetInstance()->RegisterCoordinationListener(consumer);
#ifdef OHOS_BUILD_ENABLE_COORDINATION
    ASSERT_EQ(ret, RET_ERR);
#else
    ASSERT_EQ(ret, ERROR_UNSUPPORT);
#endif // OHOS_BUILD_ENABLE_COORDINATION
}

/**
 * @tc.name: InteractionManagerTest_RegisterCoordinationListener_002
 * @tc.desc: Register coordination listener
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InteractionManagerTest, InteractionManagerTest_RegisterCoordinationListener_002, TestSize.Level1)
{
    CALL_DEBUG_ENTER;
    class CoordinationListenerTest : public ICoordinationListener {
    public:
        CoordinationListenerTest() : ICoordinationListener() {}
        void OnCoordinationMessage(const std::string &deviceId, CoordinationMessage msg) override
        {
            FI_HILOGD("Register coordination listener test");
            (void) deviceId;
        };
    };
    std::shared_ptr<CoordinationListenerTest> consumer =
        std::make_shared<CoordinationListenerTest>();
    int32_t ret = InteractionManager::GetInstance()->RegisterCoordinationListener(consumer);
#ifdef OHOS_BUILD_ENABLE_COORDINATION
    ASSERT_EQ(ret, RET_OK);
#else
    ASSERT_EQ(ret, ERROR_UNSUPPORT);
#endif // OHOS_BUILD_ENABLE_COORDINATION
    ret = InteractionManager::GetInstance()->UnregisterCoordinationListener(consumer);
#ifdef OHOS_BUILD_ENABLE_COORDINATION
    ASSERT_EQ(ret, RET_OK);
#else
    ASSERT_EQ(ret, ERROR_UNSUPPORT);
#endif // OHOS_BUILD_ENABLE_COORDINATION
}

/**
 * @tc.name: InteractionManagerTest_UnregisterCoordinationListener
 * @tc.desc: Unregister coordination listener
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InteractionManagerTest, InteractionManagerTest_UnregisterCoordinationListener, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<ICoordinationListener> consumer = nullptr;
    int32_t ret = InteractionManager::GetInstance()->UnregisterCoordinationListener(consumer);
#ifdef OHOS_BUILD_ENABLE_COORDINATION
    ASSERT_EQ(ret, RET_OK);
#else
    ASSERT_EQ(ret, ERROR_UNSUPPORT);
#endif // OHOS_BUILD_ENABLE_COORDINATION
}

/**
 * @tc.name: InteractionManagerTest_PrepareCoordination
 * @tc.desc: Prepare coordination
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InteractionManagerTest, InteractionManagerTest_PrepareCoordination, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::promise<bool> promiseFlag;
    std::future<bool> futureFlag = promiseFlag.get_future();
    auto fun = [&promiseFlag](std::string listener, CoordinationMessage coordinationMessages) {
        FI_HILOGD("Prepare coordination success, listener:%{public}s", listener.c_str());
        promiseFlag.set_value(true);
    };
    int32_t ret = InteractionManager::GetInstance()->PrepareCoordination(fun);
#ifdef OHOS_BUILD_ENABLE_COORDINATION
    ASSERT_EQ(ret, RET_OK);
    ASSERT_TRUE(futureFlag.wait_for(std::chrono::milliseconds(PROMISE_WAIT_SPAN_MS)) != std::future_status::timeout);
#else
    ASSERT_EQ(ret, ERROR_UNSUPPORT);
#endif // OHOS_BUILD_ENABLE_COORDINATION
}

/**
 * @tc.name: InteractionManagerTest_UnprepareCoordination
 * @tc.desc: Prepare coordination
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InteractionManagerTest, InteractionManagerTest_UnprepareCoordination, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::promise<bool> promiseFlag;
    std::future<bool> futureFlag = promiseFlag.get_future();
    auto fun = [&promiseFlag](std::string listener, CoordinationMessage coordinationMessages) {
        FI_HILOGD("Prepare coordination success, listener:%{public}s", listener.c_str());
        promiseFlag.set_value(true);
    };
    int32_t ret = InteractionManager::GetInstance()->UnprepareCoordination(fun);
#ifdef OHOS_BUILD_ENABLE_COORDINATION
    ASSERT_EQ(ret, RET_OK);
    ASSERT_TRUE(futureFlag.wait_for(std::chrono::milliseconds(PROMISE_WAIT_SPAN_MS)) != std::future_status::timeout);
#else
    ASSERT_EQ(ret, ERROR_UNSUPPORT);
#endif // OHOS_BUILD_ENABLE_COORDINATION
}

/**
 * @tc.name: InteractionManagerTest_ActivateCoordination
 * @tc.desc: Activate coordination
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InteractionManagerTest, InteractionManagerTest_ActivateCoordination, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::string remoteNetworkId("");
    int32_t startDeviceId = -1;
    auto fun = [](std::string listener, CoordinationMessage coordinationMessages) {
        FI_HILOGD("Start coordination success");
        (void) listener;
    };
    int32_t ret = InteractionManager::GetInstance()->ActivateCoordination(remoteNetworkId, startDeviceId, fun);
#ifdef OHOS_BUILD_ENABLE_COORDINATION
    ASSERT_NE(ret, RET_OK);
#else
    ASSERT_EQ(ret, ERROR_UNSUPPORT);
#endif // OHOS_BUILD_ENABLE_COORDINATION
}

/**
 * @tc.name: InteractionManagerTest_DeactivateCoordination
 * @tc.desc: Deactivate coordination
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InteractionManagerTest, InteractionManagerTest_DeactivateCoordination, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    auto fun = [](std::string listener, CoordinationMessage coordinationMessages) {
        FI_HILOGD("Stop coordination success");
        (void) listener;
    };
    int32_t ret = InteractionManager::GetInstance()->DeactivateCoordination(false, fun);
#ifdef OHOS_BUILD_ENABLE_COORDINATION
    ASSERT_NE(ret, RET_OK);
#else
    ASSERT_EQ(ret, ERROR_UNSUPPORT);
    ret = InteractionManager::GetInstance()->DeactivateCoordination(true, fun);
    ASSERT_EQ(ret, ERROR_UNSUPPORT);
#endif // OHOS_BUILD_ENABLE_COORDINATION
}

/**
 * @tc.name: InteractionManagerTest_GetCoordinationState_Abnormal
 * @tc.desc: Get coordination state
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InteractionManagerTest, InteractionManagerTest_GetCoordinationState_Abnormal, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    const std::string deviceId("");
    auto fun = [](bool state) {
        FI_HILOGD("Get coordination state failed, state:%{public}d", state);
    };
    int32_t ret = InteractionManager::GetInstance()->GetCoordinationState(deviceId, fun);
#ifdef OHOS_BUILD_ENABLE_COORDINATION
    ASSERT_NE(ret, RET_OK);
#else
    ASSERT_EQ(ret, ERROR_UNSUPPORT);
#endif // OHOS_BUILD_ENABLE_COORDINATION
}

/**
 * @tc.name: InteractionManagerTest_GetCoordinationState_Normal
 * @tc.desc: Get coordination state
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InteractionManagerTest, InteractionManagerTest_GetCoordinationState_Normal, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::promise<bool> promiseFlag;
    std::future<bool> futureFlag = promiseFlag.get_future();
    const std::string deviceId("deviceId");
    auto fun = [&promiseFlag](bool state) {
        FI_HILOGD("Get coordination state success, state:%{public}d", state);
        promiseFlag.set_value(true);
    };
    int32_t ret = InteractionManager::GetInstance()->GetCoordinationState(deviceId, fun);
#ifdef OHOS_BUILD_ENABLE_COORDINATION
    ASSERT_EQ(ret, RET_OK);
    ASSERT_TRUE(futureFlag.wait_for(std::chrono::milliseconds(PROMISE_WAIT_SPAN_MS)) != std::future_status::timeout);
#else
    ASSERT_EQ(ret, ERROR_UNSUPPORT);
#endif // OHOS_BUILD_ENABLE_COORDINATION
}

/**
 * @tc.name: InteractionManagerTest_Draglistener_Mouse
 * @tc.desc: Drag listener
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InteractionManagerTest, InteractionManagerTest_Draglistener_Mouse, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    if (g_deviceMouseId < 0) {
        ASSERT_TRUE(g_deviceMouseId < 0);
    } else {
        auto listener = std::make_shared<DragListenerTest>(std::string("Draglistener_Mouse"));
        int32_t ret = InteractionManager::GetInstance()->AddDraglistener(listener);
        ASSERT_EQ(ret, RET_OK);
        std::promise<bool> promiseFlag;
        std::future<bool> futureFlag = promiseFlag.get_future();
        auto callback = [&promiseFlag](const DragNotifyMsg& notifyMessage) {
            FI_HILOGD("displayX:%{public}d, displayY:%{public}d, result:%{public}d, target:%{public}d",
                notifyMessage.displayX, notifyMessage.displayY, notifyMessage.result, notifyMessage.targetPid);
            promiseFlag.set_value(true);
        };
        SimulateDownEvent({ DRAG_SRC_X, DRAG_SRC_Y }, MMI::PointerEvent::SOURCE_TYPE_MOUSE, MOUSE_POINTER_ID);
        std::optional<DragData> dragData = CreateDragData({ TEST_PIXEL_MAP_WIDTH, TEST_PIXEL_MAP_HEIGHT },
            MMI::PointerEvent::SOURCE_TYPE_MOUSE, MOUSE_POINTER_ID, DISPLAY_ID, { DRAG_SRC_X, DRAG_SRC_Y });
        ASSERT_TRUE(dragData);
        ret = InteractionManager::GetInstance()->StartDrag(dragData.value(), callback);
        ASSERT_EQ(ret, RET_OK);
        ret = InteractionManager::GetInstance()->SetDragWindowVisible(true);
        EXPECT_EQ(ret, RET_OK);
        SimulateMoveEvent({ DRAG_SRC_X, DRAG_SRC_Y }, { DRAG_DST_X, DRAG_DST_Y },
            MMI::PointerEvent::SOURCE_TYPE_MOUSE, MOUSE_POINTER_ID, true);
        InteractionManager::GetInstance()->StopDrag(DragResult::DRAG_SUCCESS, HAS_CUSTOM_ANIMATION);
        ASSERT_TRUE(futureFlag.wait_for(std::chrono::milliseconds(PROMISE_WAIT_SPAN_MS)) !=
            std::future_status::timeout);
        ret = InteractionManager::GetInstance()->RemoveDraglistener(listener);
        ASSERT_EQ(ret, RET_OK);
    }
}

/**
 * @tc.name: InteractionManagerTest_Draglistener_Touch
 * @tc.desc: Drag listener
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InteractionManagerTest, InteractionManagerTest_Draglistener_Touch, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    if (g_deviceTouchId < 0) {
        ASSERT_TRUE(g_deviceTouchId < 0);
    } else {
        auto listener = std::make_shared<DragListenerTest>(std::string("Draglistener_Touch"));
        int32_t ret = InteractionManager::GetInstance()->AddDraglistener(listener);
        ASSERT_EQ(ret, RET_OK);
        std::promise<bool> promiseFlag;
        std::future<bool> futureFlag = promiseFlag.get_future();
        auto callback = [&promiseFlag](const DragNotifyMsg& notifyMessage) {
            FI_HILOGD("displayX:%{public}d, displayY:%{public}d, result:%{public}d, target:%{public}d",
                notifyMessage.displayX, notifyMessage.displayY, notifyMessage.result, notifyMessage.targetPid);
            promiseFlag.set_value(true);
        };
        SimulateDownEvent({ DRAG_SRC_X, DRAG_SRC_Y }, MMI::PointerEvent::SOURCE_TYPE_TOUCHSCREEN, TOUCH_POINTER_ID);
        std::optional<DragData> dragData = CreateDragData({ MAX_PIXEL_MAP_WIDTH, MAX_PIXEL_MAP_HEIGHT },
            MMI::PointerEvent::SOURCE_TYPE_TOUCHSCREEN, TOUCH_POINTER_ID, DISPLAY_ID, { DRAG_SRC_X, DRAG_SRC_Y });
        ASSERT_TRUE(dragData);
        ret = InteractionManager::GetInstance()->StartDrag(dragData.value(), callback);
        ASSERT_EQ(ret, RET_OK);
        SimulateMoveEvent({ DRAG_SRC_X, DRAG_SRC_Y }, { DRAG_DST_X, DRAG_DST_Y },
            MMI::PointerEvent::SOURCE_TYPE_TOUCHSCREEN, TOUCH_POINTER_ID, true);
        InteractionManager::GetInstance()->StopDrag(DragResult::DRAG_SUCCESS, HAS_CUSTOM_ANIMATION);
        ASSERT_TRUE(futureFlag.wait_for(std::chrono::milliseconds(PROMISE_WAIT_SPAN_MS)) !=
            std::future_status::timeout);
        ret = InteractionManager::GetInstance()->RemoveDraglistener(listener);
        ASSERT_EQ(ret, RET_OK);
    }
}

/**
 * @tc.name: InteractionManagerTest_Draglistener
 * @tc.desc: Drag listener
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InteractionManagerTest, InteractionManagerTest_Draglistener, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    if (g_deviceTouchId < 0) {
        ASSERT_TRUE(g_deviceTouchId < 0);
    } else {
        int32_t ret = RET_ERR;
        std::vector<std::shared_ptr<DragListenerTest>> dragListeners;
        constexpr size_t listenerCount = 5;
        for (size_t i = 0; i < listenerCount; ++i) {
            std::string moduleName = "Draglistener_" + std::to_string(i);
            auto listener = std::make_shared<DragListenerTest>(moduleName);
            ret = InteractionManager::GetInstance()->AddDraglistener(listener);
            if (ret == RET_OK) {
                dragListeners.push_back(listener);
            }
        }
        std::promise<bool> promiseFlag;
        std::future<bool> futureFlag = promiseFlag.get_future();
        auto callback = [&promiseFlag](const DragNotifyMsg& notifyMessage) {
            FI_HILOGD("displayX:%{public}d, displayY:%{public}d, result:%{public}d, target:%{public}d",
                notifyMessage.displayX, notifyMessage.displayY, notifyMessage.result, notifyMessage.targetPid);
            promiseFlag.set_value(true);
        };
        SimulateDownEvent({ DRAG_SRC_X, DRAG_SRC_Y }, MMI::PointerEvent::SOURCE_TYPE_TOUCHSCREEN, TOUCH_POINTER_ID);
        std::optional<DragData> dragData = CreateDragData({ MAX_PIXEL_MAP_WIDTH, MAX_PIXEL_MAP_HEIGHT },
            MMI::PointerEvent::SOURCE_TYPE_TOUCHSCREEN, TOUCH_POINTER_ID, DISPLAY_ID, { DRAG_SRC_X, DRAG_SRC_Y });
        ASSERT_TRUE(dragData);
        ret = InteractionManager::GetInstance()->StartDrag(dragData.value(), callback);
        ASSERT_EQ(ret, RET_OK);
        SimulateMoveEvent({ DRAG_SRC_X, DRAG_SRC_Y }, { DRAG_DST_X, DRAG_DST_Y },
            MMI::PointerEvent::SOURCE_TYPE_TOUCHSCREEN, TOUCH_POINTER_ID, true);
        InteractionManager::GetInstance()->StopDrag(DragResult::DRAG_SUCCESS, HAS_CUSTOM_ANIMATION);
        ASSERT_TRUE(futureFlag.wait_for(std::chrono::milliseconds(PROMISE_WAIT_SPAN_MS)) !=
            std::future_status::timeout);
        for (auto listener : dragListeners) {
            ret = InteractionManager::GetInstance()->RemoveDraglistener(listener);
            EXPECT_EQ(ret, RET_OK);
        }
    }
}

/**
 * @tc.name: InteractionManagerTest_StartDrag_Mouse
 * @tc.desc: Start Drag
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InteractionManagerTest, InteractionManagerTest_StartDrag_Mouse, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    if (g_deviceMouseId < 0) {
        ASSERT_TRUE(g_deviceMouseId < 0);
    } else {
        std::promise<bool> promiseFlag;
        std::future<bool> futureFlag = promiseFlag.get_future();
        auto callback = [&promiseFlag](const DragNotifyMsg& notifyMessage) {
            FI_HILOGD("displayX:%{public}d, displayY:%{public}d, result:%{public}d, target:%{public}d",
                notifyMessage.displayX, notifyMessage.displayY, notifyMessage.result, notifyMessage.targetPid);
            promiseFlag.set_value(true);
        };
        SimulateDownEvent({ DRAG_SRC_X, DRAG_SRC_Y }, MMI::PointerEvent::SOURCE_TYPE_MOUSE, MOUSE_POINTER_ID);
        std::optional<DragData> dragData = CreateDragData({ TEST_PIXEL_MAP_WIDTH, TEST_PIXEL_MAP_HEIGHT },
            MMI::PointerEvent::SOURCE_TYPE_MOUSE, MOUSE_POINTER_ID, DISPLAY_ID, { DRAG_SRC_X, DRAG_SRC_Y });
        ASSERT_TRUE(dragData);
        int32_t ret = InteractionManager::GetInstance()->StartDrag(dragData.value(), callback);
        ASSERT_EQ(ret, RET_OK);
        ret = InteractionManager::GetInstance()->SetDragWindowVisible(true);
        EXPECT_EQ(ret, RET_OK);
        SimulateMoveEvent({ DRAG_SRC_X, DRAG_SRC_Y }, { DRAG_DST_X, DRAG_DST_Y },
            MMI::PointerEvent::SOURCE_TYPE_MOUSE, MOUSE_POINTER_ID, true);
        InteractionManager::GetInstance()->StopDrag(DragResult::DRAG_SUCCESS, HAS_CUSTOM_ANIMATION);
        ASSERT_TRUE(futureFlag.wait_for(std::chrono::milliseconds(PROMISE_WAIT_SPAN_MS)) !=
            std::future_status::timeout);
    }
}

/**
 * @tc.name: InteractionManagerTest_StopDrag_Mouse
 * @tc.desc: Stop drag
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InteractionManagerTest, InteractionManagerTest_StopDrag_Mouse, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    if (g_deviceMouseId < 0) {
        ASSERT_TRUE(g_deviceMouseId < 0);
    } else {
        std::promise<bool> promiseFlag;
        std::future<bool> futureFlag = promiseFlag.get_future();
        auto callback = [&promiseFlag](const DragNotifyMsg& notifyMessage) {
            FI_HILOGD("displayX:%{public}d, displayY:%{public}d, result:%{public}d, target:%{public}d",
                notifyMessage.displayX, notifyMessage.displayY, notifyMessage.result, notifyMessage.targetPid);
            promiseFlag.set_value(true);
        };
        SimulateDownEvent({ DRAG_SRC_X, DRAG_SRC_Y }, MMI::PointerEvent::SOURCE_TYPE_MOUSE, MOUSE_POINTER_ID);
        std::optional<DragData> dragData = CreateDragData({ TEST_PIXEL_MAP_WIDTH, TEST_PIXEL_MAP_HEIGHT },
            MMI::PointerEvent::SOURCE_TYPE_MOUSE, MOUSE_POINTER_ID, DISPLAY_ID, { DRAG_SRC_X, DRAG_SRC_Y });
        ASSERT_TRUE(dragData);
        int32_t ret = InteractionManager::GetInstance()->StartDrag(dragData.value(), callback);
        ASSERT_EQ(ret, RET_OK);
        InteractionManager::GetInstance()->StopDrag(DragResult::DRAG_SUCCESS, HAS_CUSTOM_ANIMATION);
        ASSERT_TRUE(futureFlag.wait_for(std::chrono::milliseconds(PROMISE_WAIT_SPAN_MS)) !=
            std::future_status::timeout);
    }
}

/**
 * @tc.name: InteractionManagerTest_StopDrag_Failed_Mouse
 * @tc.desc: Stop drag
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InteractionManagerTest, InteractionManagerTest_StopDrag_Failed_Mouse, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    if (g_deviceMouseId < 0) {
        ASSERT_TRUE(g_deviceMouseId < 0);
    } else {
        std::promise<bool> promiseFlag;
        std::future<bool> futureFlag = promiseFlag.get_future();
        SimulateDownEvent({ DRAG_SRC_X, DRAG_SRC_Y }, MMI::PointerEvent::SOURCE_TYPE_MOUSE, MOUSE_POINTER_ID);
        std::optional<DragData> dragData = CreateDragData({ TEST_PIXEL_MAP_WIDTH, TEST_PIXEL_MAP_HEIGHT },
            MMI::PointerEvent::SOURCE_TYPE_MOUSE, MOUSE_POINTER_ID, DISPLAY_ID, { DRAG_SRC_X, DRAG_SRC_Y });
        ASSERT_TRUE(dragData);
        int32_t ret = InteractionManager::GetInstance()->StartDrag(dragData.value(), nullptr);
        ASSERT_EQ(ret, RET_ERR);

        ret = InteractionManager::GetInstance()->StopDrag(DragResult::DRAG_SUCCESS, HAS_CUSTOM_ANIMATION);
        ASSERT_TRUE(futureFlag.wait_for(std::chrono::milliseconds(PROMISE_WAIT_SPAN_MS)) !=
            std::future_status::timeout);
        ASSERT_EQ(ret, RET_ERR);
    }
}

/**
 * @tc.name: InteractionManagerTest_StartDrag_Touch
 * @tc.desc: Start Drag
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InteractionManagerTest, InteractionManagerTest_StartDrag_Touch, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    if (g_deviceTouchId < 0) {
        ASSERT_TRUE(g_deviceTouchId < 0);
    } else {
        std::promise<bool> promiseFlag;
        std::future<bool> futureFlag = promiseFlag.get_future();
        auto callback = [&promiseFlag](const DragNotifyMsg& notifyMessage) {
            FI_HILOGD("displayX:%{public}d, displayY:%{public}d, result:%{public}d, target:%{public}d",
                notifyMessage.displayX, notifyMessage.displayY, notifyMessage.result, notifyMessage.targetPid);
            promiseFlag.set_value(true);
        };
        SimulateDownEvent({ DRAG_SRC_X, DRAG_SRC_Y }, MMI::PointerEvent::SOURCE_TYPE_TOUCHSCREEN, TOUCH_POINTER_ID);
        std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_TOUCH_DOWN_MS));
        std::optional<DragData> dragData = CreateDragData({ TEST_PIXEL_MAP_WIDTH, TEST_PIXEL_MAP_HEIGHT },
            MMI::PointerEvent::SOURCE_TYPE_TOUCHSCREEN, TOUCH_POINTER_ID, DISPLAY_ID, { DRAG_SRC_X, DRAG_SRC_Y });
        ASSERT_TRUE(dragData);
        int32_t ret = InteractionManager::GetInstance()->StartDrag(dragData.value(), callback);
        ASSERT_EQ(ret, RET_OK);
        ret = InteractionManager::GetInstance()->SetDragWindowVisible(true);
        EXPECT_EQ(ret, RET_OK);
        SimulateMoveEvent({ DRAG_SRC_X, DRAG_SRC_Y }, { DRAG_DST_X, DRAG_DST_Y },
            MMI::PointerEvent::SOURCE_TYPE_TOUCHSCREEN, TOUCH_POINTER_ID, true);
        InteractionManager::GetInstance()->StopDrag(DragResult::DRAG_SUCCESS, HAS_CUSTOM_ANIMATION);
        ASSERT_TRUE(futureFlag.wait_for(std::chrono::milliseconds(PROMISE_WAIT_SPAN_MS)) !=
            std::future_status::timeout);
    }
}

/**
 * @tc.name: InteractionManagerTest_StopDrag_Touch
 * @tc.desc: Stop drag
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InteractionManagerTest, InteractionManagerTest_StopDrag_Touch, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    if (g_deviceTouchId < 0) {
        ASSERT_TRUE(g_deviceTouchId < 0);
    } else {
        std::promise<bool> promiseFlag;
        std::future<bool> futureFlag = promiseFlag.get_future();
        auto callback = [&promiseFlag](const DragNotifyMsg& notifyMessage) {
            FI_HILOGD("displayX:%{public}d, displayY:%{public}d, result:%{public}d, target:%{public}d",
                notifyMessage.displayX, notifyMessage.displayY, notifyMessage.result, notifyMessage.targetPid);
            promiseFlag.set_value(true);
        };
        SimulateDownEvent({ DRAG_SRC_X, DRAG_SRC_Y }, MMI::PointerEvent::SOURCE_TYPE_TOUCHSCREEN, TOUCH_POINTER_ID);
        std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_TOUCH_DOWN_MS));
        std::optional<DragData> dragData = CreateDragData({ TEST_PIXEL_MAP_WIDTH, TEST_PIXEL_MAP_HEIGHT },
            MMI::PointerEvent::SOURCE_TYPE_TOUCHSCREEN, TOUCH_POINTER_ID, DISPLAY_ID, { DRAG_SRC_X, DRAG_SRC_Y });
        ASSERT_TRUE(dragData);
        int32_t ret = InteractionManager::GetInstance()->StartDrag(dragData.value(), callback);
        ASSERT_EQ(ret, RET_OK);
        InteractionManager::GetInstance()->StopDrag(DragResult::DRAG_SUCCESS, HAS_CUSTOM_ANIMATION);
        ASSERT_TRUE(futureFlag.wait_for(std::chrono::milliseconds(PROMISE_WAIT_SPAN_MS)) !=
            std::future_status::timeout);
    }
}

/**
 * @tc.name: InteractionManagerTest_GetDragTargetPid
 * @tc.desc: Get the target pid dragged by the mouse
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InteractionManagerTest, GetDragTargetPid_Mouse, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    if (g_deviceMouseId < 0) {
        ASSERT_TRUE(g_deviceMouseId < 0);
    } else {
        std::promise<bool> promiseStopFlag;
        std::future<bool> futureStopFlag = promiseStopFlag.get_future();
        auto callback = [&promiseStopFlag](const DragNotifyMsg& notifyMessage) {
            FI_HILOGD("displayX:%{public}d, displayY:%{public}d, result:%{public}d, target:%{public}d",
                notifyMessage.displayX, notifyMessage.displayY, notifyMessage.result, notifyMessage.targetPid);
            promiseStopFlag.set_value(true);
        };
        SimulateDownEvent({ DRAG_SRC_X, DRAG_SRC_Y }, MMI::PointerEvent::SOURCE_TYPE_MOUSE, MOUSE_POINTER_ID);
        std::optional<DragData> dragData = CreateDragData({ TEST_PIXEL_MAP_WIDTH, TEST_PIXEL_MAP_HEIGHT },
            MMI::PointerEvent::SOURCE_TYPE_MOUSE, MOUSE_POINTER_ID, DISPLAY_ID, { DRAG_SRC_X, DRAG_SRC_Y });
        ASSERT_TRUE(dragData);
        int32_t ret = InteractionManager::GetInstance()->StartDrag(dragData.value(), callback);
        ASSERT_EQ(ret, RET_OK);
        ret = InteractionManager::GetInstance()->SetDragWindowVisible(true);
        EXPECT_EQ(ret, RET_OK);
        ret = InteractionManager::GetInstance()->UpdateDragStyle(DragCursorStyle::DEFAULT);
        ASSERT_EQ(ret, RET_OK);
        int32_t pid = InteractionManager::GetInstance()->GetDragTargetPid();
        FI_HILOGI("Target pid:%{public}d", pid);
        ASSERT_TRUE(pid > 0);
        InteractionManager::GetInstance()->StopDrag(DragResult::DRAG_SUCCESS, HAS_CUSTOM_ANIMATION);
        ASSERT_TRUE(futureStopFlag.wait_for(std::chrono::milliseconds(PROMISE_WAIT_SPAN_MS)) !=
            std::future_status::timeout);
    }
}

/**
 * @tc.name: InteractionManagerTest_GetDragTargetPid
 * @tc.desc: Get the target pid dragged by the touchscreen
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InteractionManagerTest, GetDragTargetPid_Touch, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    if (g_deviceTouchId < 0) {
        ASSERT_TRUE(g_deviceTouchId < 0);
    } else {
        std::promise<bool> promiseStopFlag;
        std::future<bool> futureStopFlag = promiseStopFlag.get_future();
        auto callback = [&promiseStopFlag](const DragNotifyMsg& notifyMessage) {
            FI_HILOGD("displayX:%{public}d, displayY:%{public}d, result:%{public}d, target:%{public}d",
                notifyMessage.displayX, notifyMessage.displayY, notifyMessage.result, notifyMessage.targetPid);
            promiseStopFlag.set_value(true);
        };
        SimulateDownEvent({ DRAG_SRC_X, DRAG_SRC_Y }, MMI::PointerEvent::SOURCE_TYPE_TOUCHSCREEN, TOUCH_POINTER_ID);
        std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_TOUCH_DOWN_MS));
        std::optional<DragData> dragData = CreateDragData({ TEST_PIXEL_MAP_WIDTH, TEST_PIXEL_MAP_HEIGHT },
            MMI::PointerEvent::SOURCE_TYPE_TOUCHSCREEN, TOUCH_POINTER_ID, DISPLAY_ID, { DRAG_SRC_X, DRAG_SRC_Y });
        ASSERT_TRUE(dragData);
        int32_t ret = InteractionManager::GetInstance()->StartDrag(dragData.value(), callback);
        ASSERT_EQ(ret, RET_OK);
        ret = InteractionManager::GetInstance()->SetDragWindowVisible(true);
        EXPECT_EQ(ret, RET_OK);
        ret = InteractionManager::GetInstance()->UpdateDragStyle(DragCursorStyle::DEFAULT);
        ASSERT_EQ(ret, RET_OK);
        int32_t pid = InteractionManager::GetInstance()->GetDragTargetPid();
        FI_HILOGI("Target pid:%{public}d", pid);
        ASSERT_TRUE(pid > 0);
        InteractionManager::GetInstance()->StopDrag(DragResult::DRAG_SUCCESS, HAS_CUSTOM_ANIMATION);
        ASSERT_TRUE(futureStopFlag.wait_for(std::chrono::milliseconds(PROMISE_WAIT_SPAN_MS)) !=
            std::future_status::timeout);
    }
}

/**
 * @tc.name: InteractionManagerTest_TouchEventDispatch
 * @tc.desc: Get Drag Target Pid
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InteractionManagerTest, TouchEventDispatch, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    if (g_deviceTouchId < 0) {
        ASSERT_TRUE(g_deviceTouchId < 0);
    } else {
        auto callback = [](const DragNotifyMsg& notifyMessage) {
            FI_HILOGD("displayX:%{public}d, displayY:%{public}d, result:%{public}d, target:%{public}d",
                notifyMessage.displayX, notifyMessage.displayY, notifyMessage.result, notifyMessage.targetPid);
        };
        SimulateDownEvent({ DRAG_SRC_X, DRAG_SRC_Y }, MMI::PointerEvent::SOURCE_TYPE_TOUCHSCREEN, TOUCH_POINTER_ID);
        std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_TOUCH_DOWN_MS));
        std::optional<DragData> dragData = CreateDragData({ TEST_PIXEL_MAP_WIDTH, TEST_PIXEL_MAP_HEIGHT },
            MMI::PointerEvent::SOURCE_TYPE_TOUCHSCREEN, TOUCH_POINTER_ID, DISPLAY_ID, { DRAG_SRC_X, DRAG_SRC_Y });
        ASSERT_TRUE(dragData);
        int32_t ret = InteractionManager::GetInstance()->StartDrag(dragData.value(), callback);
        ASSERT_EQ(ret, RET_OK);
        std::promise<bool> promiseEventFlag;
        std::future<bool> futureEventFlag = promiseEventFlag.get_future();
        auto callbackPtr = std::make_shared<InputEventCallbackTest>(
            [&promiseEventFlag]{promiseEventFlag.set_value(true);});
        int32_t monitorId = TestAddMonitor(callbackPtr);
        SimulateMoveEvent({ DRAG_DST_X, DRAG_DST_Y }, { DRAG_DST_X, DRAG_DST_Y },
            MMI::PointerEvent::SOURCE_TYPE_TOUCHSCREEN, TOUCH_POINTER_ID, true);
        ASSERT_TRUE(futureEventFlag.wait_for(std::chrono::milliseconds(PROMISE_WAIT_SPAN_MS)) !=
            std::future_status::timeout);
        TestRemoveMonitor(monitorId);
        ret = InteractionManager::GetInstance()->StopDrag(DragResult::DRAG_SUCCESS, HAS_CUSTOM_ANIMATION);
        ASSERT_EQ(ret, RET_OK);
    }
}

/**
 * @tc.name: InteractionManagerTest_MouseEventDispatch
 * @tc.desc: Get Drag Target Pid
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InteractionManagerTest, MouseEventDispatch, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    if (g_deviceMouseId < 0) {
        ASSERT_TRUE(g_deviceMouseId < 0);
    } else {
        auto callback = [](const DragNotifyMsg& notifyMessage) {
            FI_HILOGD("displayX:%{public}d, displayY:%{public}d, result:%{public}d, target:%{public}d",
                notifyMessage.displayX, notifyMessage.displayY, notifyMessage.result, notifyMessage.targetPid);
        };
        SimulateDownEvent({ DRAG_SRC_X, DRAG_SRC_Y }, MMI::PointerEvent::SOURCE_TYPE_MOUSE, 0);
        std::optional<DragData> dragData = CreateDragData({ TEST_PIXEL_MAP_WIDTH, TEST_PIXEL_MAP_HEIGHT },
            MMI::PointerEvent::SOURCE_TYPE_MOUSE, 0, DISPLAY_ID, { DRAG_SRC_X, DRAG_SRC_Y });
        ASSERT_TRUE(dragData);
        int32_t ret = InteractionManager::GetInstance()->StartDrag(dragData.value(), callback);
        ASSERT_EQ(ret, RET_OK);
        std::promise<bool> promiseEventFlag;
        std::future<bool> futureEventFlag = promiseEventFlag.get_future();
        auto callbackPtr = std::make_shared<InputEventCallbackTest>(
            [&promiseEventFlag]{promiseEventFlag.set_value(true);});
        int32_t monitorId = TestAddMonitor(callbackPtr);
        SimulateMoveEvent({ DRAG_DST_X, DRAG_DST_Y }, { DRAG_DST_X, DRAG_DST_Y },
            MMI::PointerEvent::SOURCE_TYPE_MOUSE, TOUCH_POINTER_ID, true);
        ASSERT_TRUE(futureEventFlag.wait_for(std::chrono::milliseconds(PROMISE_WAIT_SPAN_MS)) !=
            std::future_status::timeout);
        TestRemoveMonitor(monitorId);
        ret = InteractionManager::GetInstance()->StopDrag(DragResult::DRAG_SUCCESS, HAS_CUSTOM_ANIMATION);
        ASSERT_EQ(ret, RET_OK);
    }
}

/**
 * @tc.name: InteractionManagerTest_SetDragWindowVisible
 * @tc.desc: Set Drag Window Visible
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InteractionManagerTest, InteractionManagerTest_SetDragWindowVisible, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    int32_t ret = InteractionManager::GetInstance()->SetDragWindowVisible(true);
    FI_HILOGD("ret:%{public}d", ret);
    ASSERT_EQ(ret, RET_ERR);
    ret = InteractionManager::GetInstance()->SetDragWindowVisible(false);
    FI_HILOGD("ret:%{public}d", ret);
    ASSERT_EQ(ret, RET_ERR);
}

/**
 * @tc.name: InteractionManagerTest_GetShadowOffset
 * @tc.desc: Get Shadow Offset
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InteractionManagerTest, InteractionManagerTest_GetShadowOffset, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    int32_t offsetX = 0;
    int32_t offsetY = 0;
    int32_t width = 0;
    int32_t height = 0;
    std::promise<bool> promiseFlag;
    std::future<bool> futureFlag = promiseFlag.get_future();
    auto callback = [&promiseFlag](const DragNotifyMsg& notifyMessage) {
        FI_HILOGD("displayX:%{public}d, displayY:%{public}d, result:%{public}d, target:%{public}d",
            notifyMessage.displayX, notifyMessage.displayY, notifyMessage.result, notifyMessage.targetPid);
        promiseFlag.set_value(true);
    };
    SimulateDownEvent({ DRAG_SRC_X, DRAG_SRC_Y }, MMI::PointerEvent::SOURCE_TYPE_TOUCHSCREEN, TOUCH_POINTER_ID);
    std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_TOUCH_DOWN_MS));
    std::optional<DragData> dragData = CreateDragData({ TEST_PIXEL_MAP_WIDTH, TEST_PIXEL_MAP_HEIGHT },
        MMI::PointerEvent::SOURCE_TYPE_TOUCHSCREEN, TOUCH_POINTER_ID, DISPLAY_ID, { DRAG_SRC_X, DRAG_SRC_Y });
    ASSERT_TRUE(dragData);
    int32_t ret = InteractionManager::GetInstance()->StartDrag(dragData.value(), callback);
    ASSERT_EQ(ret, RET_OK);
    ret = InteractionManager::GetInstance()->GetShadowOffset(offsetX, offsetY, width, height);
    FI_HILOGD("offsetX:%{public}d, offsetY:%{public}d, width:%{public}d, height:%{public}d",
        offsetX, offsetY, width, height);
    ASSERT_EQ(ret, RET_OK);
    ret = InteractionManager::GetInstance()->StopDrag(DragResult::DRAG_SUCCESS, HAS_CUSTOM_ANIMATION);
    ASSERT_EQ(ret, RET_OK);
    ASSERT_TRUE(futureFlag.wait_for(std::chrono::milliseconds(PROMISE_WAIT_SPAN_MS)) != std::future_status::timeout);
}

/**
 * @tc.name: InteractionManagerTest_GetUdKey
 * @tc.desc: Get the udKey dragged by the mouse
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InteractionManagerTest, GetUdKey_Mouse, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    if (g_deviceMouseId < 0) {
        ASSERT_TRUE(g_deviceMouseId < 0);
    } else {
        std::promise<bool> promiseFlag;
        std::future<bool> futureFlag = promiseFlag.get_future();
        auto callback = [&promiseFlag](const DragNotifyMsg& notifyMessage) {
            FI_HILOGD("displayX:%{public}d, displayY:%{public}d, result:%{public}d, target:%{public}d",
                notifyMessage.displayX, notifyMessage.displayY, notifyMessage.result, notifyMessage.targetPid);
            promiseFlag.set_value(true);
        };
        SimulateDownEvent({ DRAG_SRC_X, DRAG_SRC_Y }, MMI::PointerEvent::SOURCE_TYPE_MOUSE, MOUSE_POINTER_ID);
        std::optional<DragData> dragData = CreateDragData({ TEST_PIXEL_MAP_WIDTH, TEST_PIXEL_MAP_HEIGHT },
            MMI::PointerEvent::SOURCE_TYPE_MOUSE, MOUSE_POINTER_ID, DISPLAY_ID, { DRAG_SRC_X, DRAG_SRC_Y });
        ASSERT_TRUE(dragData);
        int32_t ret = InteractionManager::GetInstance()->StartDrag(dragData.value(), callback);
        ASSERT_EQ(ret, RET_OK);
        std::string udKey;
        ret = InteractionManager::GetInstance()->GetUdKey(udKey);
        ASSERT_EQ(ret, RET_OK);
        ASSERT_EQ(udKey, UD_KEY);
        ret = InteractionManager::GetInstance()->StopDrag(DragResult::DRAG_SUCCESS, HAS_CUSTOM_ANIMATION);
        ASSERT_EQ(ret, RET_OK);
        ASSERT_TRUE(futureFlag.wait_for(std::chrono::milliseconds(PROMISE_WAIT_SPAN_MS)) !=
            std::future_status::timeout);
    }
}

/**
 * @tc.name: InteractionManagerTest_GetUdKey
 * @tc.desc: Get the udKey dragged by the touchscreen
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InteractionManagerTest, GetUdKey_Touch, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    if (g_deviceTouchId < 0) {
        ASSERT_TRUE(g_deviceTouchId < 0);
    } else {
        std::promise<bool> promiseFlag;
        std::future<bool> futureFlag = promiseFlag.get_future();
        auto callback = [&promiseFlag](const DragNotifyMsg& notifyMessage) {
            FI_HILOGD("displayX:%{public}d, displayY:%{public}d, result:%{public}d, target:%{public}d",
                notifyMessage.displayX, notifyMessage.displayY, notifyMessage.result, notifyMessage.targetPid);
            promiseFlag.set_value(true);
        };
        SimulateDownEvent({ DRAG_SRC_X, DRAG_SRC_Y }, MMI::PointerEvent::SOURCE_TYPE_TOUCHSCREEN, TOUCH_POINTER_ID);
        std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_TOUCH_DOWN_MS));
        std::optional<DragData> dragData = CreateDragData({ TEST_PIXEL_MAP_WIDTH, TEST_PIXEL_MAP_HEIGHT },
            MMI::PointerEvent::SOURCE_TYPE_TOUCHSCREEN, TOUCH_POINTER_ID, DISPLAY_ID, { DRAG_SRC_X, DRAG_SRC_Y });
        ASSERT_TRUE(dragData);
        int32_t ret = InteractionManager::GetInstance()->StartDrag(dragData.value(), callback);
        ASSERT_EQ(ret, RET_OK);
        std::string udKey;
        ret = InteractionManager::GetInstance()->GetUdKey(udKey);
        ASSERT_EQ(ret, RET_OK);
        ASSERT_EQ(udKey, UD_KEY);
        ret = InteractionManager::GetInstance()->StopDrag(DragResult::DRAG_SUCCESS, HAS_CUSTOM_ANIMATION);
        ASSERT_EQ(ret, RET_OK);
        ASSERT_TRUE(futureFlag.wait_for(std::chrono::milliseconds(PROMISE_WAIT_SPAN_MS)) !=
            std::future_status::timeout);
    }
}
} // namespace DeviceStatus
} // namespace Msdp
} // namespace OHOS
