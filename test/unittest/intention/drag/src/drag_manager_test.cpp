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
#include "drag_manager_test.h"

#define BUFF_SIZE 100
#include <future>
#include "pointer_event.h"
#include "securec.h"
#include "message_parcel.h"

#include "devicestatus_define.h"
#include "devicestatus_errors.h"
#include "drag_params.h"
#include "interaction_manager.h"
#include "stationary_data.h"

namespace OHOS {
namespace Msdp {
namespace DeviceStatus {
using namespace testing::ext;
namespace {
constexpr int32_t TIME_WAIT_FOR_OP_MS { 20 };
constexpr uint32_t DEFAULT_ICON_COLOR { 0xFF };
constexpr int32_t PIXEL_MAP_HEIGHT { 3 };
constexpr int32_t PIXEL_MAP_WIDTH { 3 };
constexpr int32_t MAX_PIXEL_MAP_WIDTH { 600 };
constexpr int32_t MAX_PIXEL_MAP_HEIGHT { 600 };
constexpr int32_t POINTER_ID { 0 };
constexpr int32_t DISPLAY_X { 50 };
constexpr int32_t DISPLAY_ID { 0 };
constexpr int32_t DISPLAY_Y { 50 };
constexpr int32_t DRAG_NUM_ONE { 1 };
constexpr int32_t INT32_BYTE { 4 };
constexpr int32_t PROMISE_WAIT_SPAN_MS { 2000 };
constexpr int32_t FOREGROUND_COLOR_IN { 0x33FF0000 };
constexpr int32_t FOREGROUND_COLOR_OUT { 0x00000000 };
int32_t g_shadowinfo_x { 0 };
int32_t g_shadowinfo_y { 0 };
constexpr int32_t WINDOW_ID { -1 };
constexpr int32_t ANIMATION_DURATION { 500 };
const std::string CURVE_NAME { "cubic-bezier" };
constexpr bool HAS_CUSTOM_ANIMATION { true };
constexpr int32_t TARGET_MAIN_WINDOW { 0 };
constexpr bool HAS_CANCELED_ANIMATION { true };
const std::string UD_KEY { "Unified data key" };
const std::string FILTER_INFO { "Undefined filter info" };
const std::string EXTRA_INFO { "Undefined extra info" };
constexpr int32_t SHADOW_NUM_ONE { 1 };
} // namespace

void DragManagerTest::SetUpTestCase() {}

void DragManagerTest::SetUp() {
}

void DragManagerTest::TearDown()
{
    std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP_MS));
}

std::shared_ptr<Media::PixelMap> DragManagerTest::CreatePixelMap(int32_t width, int32_t height)
{
    CALL_DEBUG_ENTER;
    if (width <= 0 || width > MAX_PIXEL_MAP_WIDTH || height <= 0 || height > MAX_PIXEL_MAP_HEIGHT) {
        FI_HILOGE("invalid, height:%{public}d, width:%{public}d", height, width);
        return nullptr;
    }
    Media::InitializationOptions opts;
    opts.size.width = width;
    opts.size.height = height;
    opts.pixelFormat = Media::PixelFormat::BGRA_8888;
    opts.alphaType = Media::AlphaType::IMAGE_ALPHA_TYPE_OPAQUE;
    opts.scaleMode = Media::ScaleMode::FIT_TARGET_SIZE;

    int32_t colorLen = width * height;
    uint32_t *pixelColors = new (std::nothrow) uint32_t[BUFF_SIZE];
    CHKPP(pixelColors);
    int32_t colorByteCount = colorLen * INT32_BYTE;
    errno_t ret = memset_s(pixelColors, BUFF_SIZE, DEFAULT_ICON_COLOR, colorByteCount);
    if (ret != EOK) {
        FI_HILOGE("memset_s failed");
        delete[] pixelColors;
        return nullptr;
    }
    std::shared_ptr<Media::PixelMap> pixelMap = Media::PixelMap::Create(pixelColors, colorLen, opts);
    if (pixelMap == nullptr) {
        FI_HILOGE("Create pixelMap failed");
        delete[] pixelColors;
        return nullptr;
    }
    delete[] pixelColors;
    return pixelMap;
}

std::optional<DragData> DragManagerTest::CreateDragData(int32_t sourceType,
    int32_t pointerId, int32_t dragNum, bool hasCoordinateCorrected, int32_t shadowNum)
{
    CALL_DEBUG_ENTER;
    DragData dragData;
    for (int32_t i = 0; i < shadowNum; i++) {
        std::shared_ptr<Media::PixelMap> pixelMap = CreatePixelMap(PIXEL_MAP_WIDTH, PIXEL_MAP_HEIGHT);
        if (pixelMap == nullptr) {
            FI_HILOGE("pixelMap nullptr");
            return std::nullopt;
        }
        dragData.shadowInfos.push_back({ pixelMap, g_shadowinfo_x, g_shadowinfo_y });
    }
    dragData.buffer = std::vector<uint8_t>(MAX_BUFFER_SIZE, 0);
    dragData.extraInfo = FILTER_INFO;
    dragData.udKey = UD_KEY;
    dragData.sourceType = sourceType;
    dragData.extraInfo = EXTRA_INFO;
    dragData.displayId = DISPLAY_ID;
    dragData.pointerId = pointerId;
    dragData.dragNum = dragNum;
    dragData.displayX = DISPLAY_X;
    dragData.displayY = DISPLAY_Y;
    dragData.hasCoordinateCorrected = hasCoordinateCorrected;
    dragData.hasCanceledAnimation = HAS_CANCELED_ANIMATION;
    return dragData;
}

class TestStartDragListener : public IStartDragListener {
public:
    explicit TestStartDragListener(std::function<void(const DragNotifyMsg&)> function) : function_(function) { }
    void OnDragEndMessage(const DragNotifyMsg &msg) override
    {
        FI_HILOGD("DisplayX:%{public}d, displayY:%{public}d, targetPid:%{public}d, result:%{public}d",
            msg.displayX, msg.displayY, msg.targetPid, static_cast<int32_t>(msg.result));
        if (function_ != nullptr) {
            function_(msg);
        }
        FI_HILOGD("Test OnDragEndMessage");
    }

    void OnHideIconMessage() override
    {
        FI_HILOGD("Test OnHideIconMessage");
    }
private:
    std::function<void(const DragNotifyMsg&)> function_;
};

class DragListenerTest : public IDragListener {
public:
    DragListenerTest() {}
    explicit DragListenerTest(const std::string& name) : moduleName_(name) {}
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
        std::string type = "unknow";
        const std::map<DragState, std::string> stateType = {
            { DragState::ERROR, "error"},
            { DragState::START, "start"},
            { DragState::STOP, "stop"},
            { DragState::CANCEL, "cancel"}
        };
        auto item = stateType.find(state);
        if (item != stateType.end()) {
            type = item->second;
        }
        return type;
    }
private:
    std::string moduleName_;
};

class SubscriptListenerTest : public ISubscriptListener {
public:
    SubscriptListenerTest() {}
    explicit SubscriptListenerTest(const std::string& name) : moduleName_(name) {}
    void OnMessage(DragCursorStyle style) override
    {
        SetDragSyle(style);
        if (moduleName_.empty()) {
            moduleName_ = std::string("SubscriptListenerTest");
        }
        FI_HILOGD("subscriptListener, %{public}s, state:%{public}s",
            moduleName_.c_str(), PrintStyleMessage(style).c_str());
    }

    DragCursorStyle GetDragStyle()
    {
        return dragStyle_;
    }

private:
    void SetDragSyle(DragCursorStyle style)
    {
        dragStyle_ = style;
    }

    std::string PrintStyleMessage(DragCursorStyle style)
    {
        std::string type = "unknow";
        const std::map<DragCursorStyle, std::string> cursorStyles = {
            { DragCursorStyle::DEFAULT, "default"},
            { DragCursorStyle::FORBIDDEN, "forbidden"},
            { DragCursorStyle::COPY, "copy"},
            { DragCursorStyle::MOVE, "move"}
        };
        auto item = cursorStyles.find(style);
        if (item != cursorStyles.end()) {
            type = item->second;
        }
        return type;
    }

private:
    DragCursorStyle dragStyle_ { DragCursorStyle::DEFAULT };
    std::string moduleName_;
};

void DragManagerTest::AssignToAnimation(PreviewAnimation &animation)
{
    animation.duration = ANIMATION_DURATION;
    animation.curveName = CURVE_NAME;
    animation.curve = { 0.33, 0, 0.67, 1 };
}

/**
 * @tc.name: DragManagerTest1
 * @tc.desc: Drag Drawing
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(DragManagerTest, DragManagerTest1, TestSize.Level0)
{
    CALL_TEST_DEBUG;
    std::optional<DragData> dragData = CreateDragData(
        MMI::PointerEvent::SOURCE_TYPE_MOUSE, POINTER_ID, DRAG_NUM_ONE, false, SHADOW_NUM_ONE);
    EXPECT_TRUE(dragData);
    int32_t ret = InteractionManager::GetInstance()->StartDrag(dragData.value(), nullptr);
    ASSERT_EQ(ret, RET_ERR);
}

/**
 * @tc.name: DragManagerTest2
 * @tc.desc: Drag Drawing
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(DragManagerTest, DragManagerTest2, TestSize.Level0)
{
    CALL_TEST_DEBUG;
    std::promise<bool> promiseFlag;
    auto callback = [&promiseFlag](const DragNotifyMsg &notifyMessage) {
        FI_HILOGD("displayX:%{public}d, displayY:%{public}d, result:%{public}d",
            notifyMessage.displayX, notifyMessage.displayY, notifyMessage.result);
        promiseFlag.set_value(true);
    };
    std::optional<DragData> dragData = CreateDragData(
        MMI::PointerEvent::SOURCE_TYPE_MOUSE, POINTER_ID, DRAG_NUM_ONE, false, 0);
    EXPECT_TRUE(dragData);
    int32_t ret = InteractionManager::GetInstance()->StartDrag(dragData.value(),
        std::make_shared<TestStartDragListener>(callback));
    ASSERT_EQ(ret, ERR_INVALID_VALUE);
}

/**
 * @tc.name: DragManagerTest3
 * @tc.desc: Drag Drawing
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(DragManagerTest, DragManagerTest3, TestSize.Level0)
{
    CALL_TEST_DEBUG;
    std::promise<bool> promiseFlag;
    g_shadowinfo_x = 2;
    auto callback = [&promiseFlag](const DragNotifyMsg &notifyMessage) {
        FI_HILOGD("displayX:%{public}d, displayY:%{public}d, result:%{public}d",
            notifyMessage.displayX, notifyMessage.displayY, notifyMessage.result);
        promiseFlag.set_value(true);
    };
    std::optional<DragData> dragData = CreateDragData(
        MMI::PointerEvent::SOURCE_TYPE_MOUSE, POINTER_ID, DRAG_NUM_ONE, false, SHADOW_NUM_ONE);
    EXPECT_TRUE(dragData);
    int32_t ret = InteractionManager::GetInstance()->StartDrag(dragData.value(),
        std::make_shared<TestStartDragListener>(callback));
    ASSERT_EQ(ret, RET_ERR);
}

/**
 * @tc.name: DragManagerTest4
 * @tc.desc: Drag Drawing
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(DragManagerTest, DragManagerTest4, TestSize.Level0)
{
    CALL_TEST_DEBUG;
    g_shadowinfo_x = 0;
    std::promise<bool> promiseFlag;
    auto callback = [&promiseFlag](const DragNotifyMsg &notifyMessage) {
        FI_HILOGD("displayX:%{public}d, displayY:%{public}d, result:%{public}d",
            notifyMessage.displayX, notifyMessage.displayY, notifyMessage.result);
        promiseFlag.set_value(true);
    };
    std::optional<DragData> dragData = CreateDragData(
        MMI::PointerEvent::SOURCE_TYPE_MOUSE, POINTER_ID, -1, false, SHADOW_NUM_ONE);
    EXPECT_TRUE(dragData);
    int32_t ret = InteractionManager::GetInstance()->StartDrag(dragData.value(),
        std::make_shared<TestStartDragListener>(callback));
    ASSERT_EQ(ret, RET_ERR);
}

/**
 * @tc.name: DragManagerTest5
 * @tc.desc: Drag Drawing
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(DragManagerTest, DragManagerTest5, TestSize.Level0)
{
    CALL_TEST_DEBUG;
    int32_t ret = InteractionManager::GetInstance()->AddDraglistener(nullptr);
    ASSERT_EQ(ret, RET_ERR);
    ret = InteractionManager::GetInstance()->RemoveDraglistener(nullptr);
    ASSERT_EQ(ret, RET_OK);
}

/**
 * @tc.name: DragManagerTest6
 * @tc.desc: Drag Drawing
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(DragManagerTest, DragManagerTest6, TestSize.Level0)
{
    CALL_TEST_DEBUG;
    auto listener = std::make_shared<DragListenerTest>(std::string("Draglistener_Mouse"));
    int32_t ret = InteractionManager::GetInstance()->AddDraglistener(listener);
    ASSERT_EQ(ret, RET_OK);
}

/**
 * @tc.name: DragManagerTest7
 * @tc.desc: Drag Drawing
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(DragManagerTest, DragManagerTest7, TestSize.Level0)
{
    CALL_TEST_DEBUG;
    int32_t ret = InteractionManager::GetInstance()->AddSubscriptListener(nullptr);
    ASSERT_EQ(ret, RET_ERR);
}

/**
 * @tc.name: DragManagerTest8
 * @tc.desc: Drag Drawing
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(DragManagerTest, DragManagerTest8, TestSize.Level0)
{
    CALL_TEST_DEBUG;
    auto listener = std::make_shared<SubscriptListenerTest>("SubscriptListener");
    int32_t ret = InteractionManager::GetInstance()->AddSubscriptListener(listener);
    ASSERT_EQ(ret, RET_OK);
}

/**
 * @tc.name: DragManagerTest9
 * @tc.desc: Drag Drawing
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(DragManagerTest, DragManagerTest9, TestSize.Level0)
{
    CALL_TEST_DEBUG;
    auto listener = std::make_shared<SubscriptListenerTest>("SubscriptListener");
    int32_t ret = InteractionManager::GetInstance()->RemoveSubscriptListener(listener);
    ASSERT_EQ(ret, RET_OK);
}

/**
 * @tc.name: DragManagerTest10
 * @tc.desc: Drag Drawing
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(DragManagerTest, DragManagerTest10, TestSize.Level0)
{
    CALL_TEST_DEBUG;
    int32_t ret = InteractionManager::GetInstance()->RemoveSubscriptListener(nullptr);
    ASSERT_EQ(ret, RET_OK);
}

/**
 * @tc.name: DragManagerTest11
 * @tc.desc: Drag Drawing
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(DragManagerTest, DragManagerTest11, TestSize.Level0)
{
    CALL_TEST_DEBUG;
    int32_t ret = InteractionManager::GetInstance()->RemoveSubscriptListener(nullptr);
    auto listener = std::make_shared<SubscriptListenerTest>("SubscriptListener");
    ret = InteractionManager::GetInstance()->AddSubscriptListener(listener);
    ret = InteractionManager::GetInstance()->RemoveSubscriptListener(nullptr);
    ASSERT_EQ(ret, RET_OK);
}

/**
 * @tc.name: DragManagerTest12
 * @tc.desc: Drag Drawing
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(DragManagerTest, DragManagerTest12, TestSize.Level0)
{
    CALL_TEST_DEBUG;
    int32_t ret = InteractionManager::GetInstance()->UpdateDragStyle(static_cast<DragCursorStyle>(-1));
    ASSERT_EQ(ret, RET_ERR);
}

/**
 * @tc.name: DragManagerTest13
 * @tc.desc: Drag Drawing
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(DragManagerTest, DragManagerTest13, TestSize.Level0)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<Media::PixelMap> pixelMap = CreatePixelMap(PIXEL_MAP_WIDTH, PIXEL_MAP_HEIGHT);
    ASSERT_NE(pixelMap, nullptr);
    ShadowInfo shadowInfo = { pixelMap, 1, 0 };
    int32_t ret = InteractionManager::GetInstance()->UpdateShadowPic(shadowInfo);
    ASSERT_EQ(ret, RET_ERR);
}

/**
 * @tc.name: DragManagerTest14
 * @tc.desc: Drag Drawing
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(DragManagerTest, DragManagerTest14, TestSize.Level0)
{
    CALL_TEST_DEBUG;
    std::optional<DragData> dragData = CreateDragData(
        MMI::PointerEvent::SOURCE_TYPE_MOUSE, POINTER_ID, DRAG_NUM_ONE, false, SHADOW_NUM_ONE);
    EXPECT_TRUE(dragData);
    std::promise<bool> promiseFlag;
    std::future<bool> futureFlag = promiseFlag.get_future();
    auto callback = [&promiseFlag](const DragNotifyMsg &notifyMessage) {
        FI_HILOGD("displayX:%{public}d, displayY:%{public}d, result:%{public}d, target:%{public}d",
            notifyMessage.displayX, notifyMessage.displayY, notifyMessage.result, notifyMessage.targetPid);
        promiseFlag.set_value(true);
    };
    int32_t ret = InteractionManager::GetInstance()->StartDrag(dragData.value(),
        std::make_shared<TestStartDragListener>(callback));
    ASSERT_EQ(ret, RET_OK);
    std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP_MS));
    std::vector<DragCursorStyle> dragCursorStyles = {DragCursorStyle::DEFAULT,
        DragCursorStyle::FORBIDDEN, DragCursorStyle::COPY, DragCursorStyle::MOVE};
    for (const auto& dragCursorStyle : dragCursorStyles) {
        ret = InteractionManager::GetInstance()->UpdateDragStyle(dragCursorStyle);
        ASSERT_EQ(ret, RET_OK);
    }
    DragDropResult dropResult { DragResult::DRAG_SUCCESS,
        HAS_CUSTOM_ANIMATION, TARGET_MAIN_WINDOW };
    ret = InteractionManager::GetInstance()->StopDrag(dropResult);
    ASSERT_EQ(ret, RET_OK);
    EXPECT_TRUE(futureFlag.wait_for(std::chrono::milliseconds(PROMISE_WAIT_SPAN_MS)) !=
        std::future_status::timeout);
}

/**
 * @tc.name: DragManagerTest15
 * @tc.desc: Drag Drawing
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(DragManagerTest, DragManagerTest15, TestSize.Level0)
{
    CALL_TEST_DEBUG;
    std::promise<bool> promiseFlag;
    std::future<bool> futureFlag = promiseFlag.get_future();
    auto callback = [&promiseFlag](const DragNotifyMsg &notifyMessage) {
        FI_HILOGD("displayX:%{public}d, displayY:%{public}d, result:%{public}d, target:%{public}d",
            notifyMessage.displayX, notifyMessage.displayY, notifyMessage.result, notifyMessage.targetPid);
        promiseFlag.set_value(true);
    };
    std::optional<DragData> dragData = CreateDragData(
        MMI::PointerEvent::SOURCE_TYPE_TOUCHSCREEN, POINTER_ID, DRAG_NUM_ONE, false, SHADOW_NUM_ONE);
    EXPECT_TRUE(dragData);
    int32_t ret = InteractionManager::GetInstance()->StartDrag(dragData.value(),
        std::make_shared<TestStartDragListener>(callback));
    ASSERT_EQ(ret, RET_OK);
    std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP_MS));
    DragDropResult dropResult { DragResult::DRAG_SUCCESS,
        HAS_CUSTOM_ANIMATION, TARGET_MAIN_WINDOW };
    ret = InteractionManager::GetInstance()->StopDrag(dropResult);
    ASSERT_EQ(ret, RET_OK);
    EXPECT_TRUE(futureFlag.wait_for(std::chrono::milliseconds(PROMISE_WAIT_SPAN_MS)) !=
        std::future_status::timeout);}

/**
 * @tc.name: DragManagerTest16
 * @tc.desc: Drag Drawing
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(DragManagerTest, DragManagerTest16, TestSize.Level0)
{
    CALL_TEST_DEBUG;
    std::promise<bool> promiseFlag;
    std::future<bool> futureFlag = promiseFlag.get_future();
    auto callback = [&promiseFlag](const DragNotifyMsg &notifyMessage) {
        FI_HILOGD("displayX:%{public}d, displayY:%{public}d, result:%{public}d, target:%{public}d",
            notifyMessage.displayX, notifyMessage.displayY, notifyMessage.result, notifyMessage.targetPid);
        promiseFlag.set_value(true);
    };
    std::optional<DragData> dragData = CreateDragData(
        MMI::PointerEvent::SOURCE_TYPE_TOUCHSCREEN, POINTER_ID, DRAG_NUM_ONE, false, SHADOW_NUM_ONE);
    EXPECT_TRUE(dragData);
    int32_t ret = InteractionManager::GetInstance()->StartDrag(dragData.value(),
        std::make_shared<TestStartDragListener>(callback));
    ASSERT_EQ(ret, RET_OK);
    std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP_MS));
    std::string udKey;
    ret = InteractionManager::GetInstance()->GetUdKey(udKey);
    DragDropResult dropResult { DragResult::DRAG_SUCCESS,
        HAS_CUSTOM_ANIMATION, TARGET_MAIN_WINDOW };
    ret = InteractionManager::GetInstance()->StopDrag(dropResult);
    ASSERT_EQ(ret, RET_OK);
    EXPECT_TRUE(futureFlag.wait_for(std::chrono::milliseconds(PROMISE_WAIT_SPAN_MS)) !=
        std::future_status::timeout);}

/**
 * @tc.name: DragManagerTest17
 * @tc.desc: Drag Drawing
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(DragManagerTest, DragManagerTest17, TestSize.Level0)
{
    CALL_TEST_DEBUG;
    std::promise<bool> promiseFlag;
    std::future<bool> futureFlag = promiseFlag.get_future();
    auto callback = [&promiseFlag](const DragNotifyMsg &notifyMessage) {
        FI_HILOGD("displayX:%{public}d, displayY:%{public}d, result:%{public}d, target:%{public}d",
            notifyMessage.displayX, notifyMessage.displayY, notifyMessage.result, notifyMessage.targetPid);
        promiseFlag.set_value(true);
    };
    std::optional<DragData> dragData = CreateDragData(
        MMI::PointerEvent::SOURCE_TYPE_TOUCHSCREEN, POINTER_ID, DRAG_NUM_ONE, false, SHADOW_NUM_ONE);
    EXPECT_TRUE(dragData);
    int32_t ret = InteractionManager::GetInstance()->StartDrag(dragData.value(),
        std::make_shared<TestStartDragListener>(callback));
    ASSERT_EQ(ret, RET_OK);
    std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP_MS));
    ret = InteractionManager::GetInstance()->SetDragWindowVisible(true);
    ASSERT_EQ(ret, RET_OK);
    std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP_MS));
    ret = InteractionManager::GetInstance()->SetDragWindowVisible(false);
    ASSERT_EQ(ret, RET_OK);
    std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP_MS));
    DragDropResult dropResult { DragResult::DRAG_SUCCESS,
        HAS_CUSTOM_ANIMATION, TARGET_MAIN_WINDOW };
    ret = InteractionManager::GetInstance()->StopDrag(dropResult);
    ASSERT_EQ(ret, RET_OK);
    EXPECT_TRUE(futureFlag.wait_for(std::chrono::milliseconds(PROMISE_WAIT_SPAN_MS)) !=
        std::future_status::timeout);
}

/**
 * @tc.name: DragManagerTest18
 * @tc.desc: Drag Drawing
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(DragManagerTest, DragManagerTest18, TestSize.Level0)
{
    CALL_TEST_DEBUG;
    int32_t offsetX = 0;
    int32_t offsetY = 0;
    int32_t width = 0;
    int32_t height = 0;
    std::promise<bool> promiseFlag;
    std::future<bool> futureFlag = promiseFlag.get_future();
    auto callback = [&promiseFlag](const DragNotifyMsg &notifyMessage) {
        FI_HILOGD("displayX:%{public}d, displayY:%{public}d, result:%{public}d, target:%{public}d",
            notifyMessage.displayX, notifyMessage.displayY, notifyMessage.result, notifyMessage.targetPid);
        promiseFlag.set_value(true);
    };
    std::optional<DragData> dragData = CreateDragData(
        MMI::PointerEvent::SOURCE_TYPE_TOUCHSCREEN, POINTER_ID, DRAG_NUM_ONE, false, SHADOW_NUM_ONE);
    EXPECT_TRUE(dragData);
    int32_t ret = InteractionManager::GetInstance()->StartDrag(dragData.value(),
        std::make_shared<TestStartDragListener>(callback));
    ASSERT_EQ(ret, RET_OK);
    std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP_MS));
    ret = InteractionManager::GetInstance()->GetShadowOffset(offsetX, offsetY, width, height);
    ASSERT_EQ(ret, RET_OK);
    std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP_MS));
    DragDropResult dropResult { DragResult::DRAG_SUCCESS,
        HAS_CUSTOM_ANIMATION, TARGET_MAIN_WINDOW };
    ret = InteractionManager::GetInstance()->StopDrag(dropResult);
    ASSERT_EQ(ret, RET_OK);
    EXPECT_TRUE(futureFlag.wait_for(std::chrono::milliseconds(PROMISE_WAIT_SPAN_MS)) !=
        std::future_status::timeout);
}

/**
 * @tc.name: DragManagerTest19
 * @tc.desc: Drag Drawing
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(DragManagerTest, DragManagerTest19, TestSize.Level0)
{
    CALL_TEST_DEBUG;
    std::promise<bool> promiseFlag;
    std::future<bool> futureFlag = promiseFlag.get_future();
    auto callback = [&promiseFlag](const DragNotifyMsg &notifyMessage) {
        FI_HILOGD("displayX:%{public}d, displayY:%{public}d, result:%{public}d, target:%{public}d",
            notifyMessage.displayX, notifyMessage.displayY, notifyMessage.result, notifyMessage.targetPid);
        promiseFlag.set_value(true);
    };
    std::optional<DragData> dragData = CreateDragData(
        MMI::PointerEvent::SOURCE_TYPE_TOUCHSCREEN, POINTER_ID, DRAG_NUM_ONE, false, SHADOW_NUM_ONE);
    EXPECT_TRUE(dragData);
    int32_t ret = InteractionManager::GetInstance()->StartDrag(dragData.value(),
        std::make_shared<TestStartDragListener>(callback));
    ASSERT_EQ(ret, RET_OK);
    std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP_MS));
    std::shared_ptr<Media::PixelMap> pixelMap = CreatePixelMap(PIXEL_MAP_WIDTH, PIXEL_MAP_HEIGHT);
    ASSERT_NE(pixelMap, nullptr);
    ShadowInfo shadowInfo = { pixelMap, 0, 0 };
    ret = InteractionManager::GetInstance()->UpdateShadowPic(shadowInfo);
    ASSERT_EQ(ret, RET_OK);
    std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP_MS));
    DragDropResult dropResult { DragResult::DRAG_SUCCESS,
        HAS_CUSTOM_ANIMATION, TARGET_MAIN_WINDOW };
    ret = InteractionManager::GetInstance()->StopDrag(dropResult);
    ASSERT_EQ(ret, RET_OK);
    EXPECT_TRUE(futureFlag.wait_for(std::chrono::milliseconds(PROMISE_WAIT_SPAN_MS)) !=
        std::future_status::timeout);
}

/**
 * @tc.name: DragManagerTest20
 * @tc.desc: Drag Drawing
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(DragManagerTest, DragManagerTest20, TestSize.Level0)
{
    CALL_TEST_DEBUG;
    std::promise<bool> promiseFlag;
    std::future<bool> futureFlag = promiseFlag.get_future();
    auto callback = [&promiseFlag](const DragNotifyMsg &notifyMessage) {
        FI_HILOGD("displayX:%{public}d, displayY:%{public}d, result:%{public}d, target:%{public}d",
            notifyMessage.displayX, notifyMessage.displayY, notifyMessage.result, notifyMessage.targetPid);
        promiseFlag.set_value(true);
    };
    std::optional<DragData> dragData = CreateDragData(
        MMI::PointerEvent::SOURCE_TYPE_TOUCHSCREEN, POINTER_ID, DRAG_NUM_ONE, false, SHADOW_NUM_ONE);
    EXPECT_TRUE(dragData);
    int32_t ret = InteractionManager::GetInstance()->StartDrag(dragData.value(),
        std::make_shared<TestStartDragListener>(callback));
    ASSERT_EQ(ret, RET_OK);
    std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP_MS));
    DragData replyDragData;
    ret = InteractionManager::GetInstance()->GetDragData(replyDragData);
    ASSERT_EQ(ret, RET_OK);
    std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP_MS));
    DragDropResult dropResult { DragResult::DRAG_SUCCESS,
        HAS_CUSTOM_ANIMATION, TARGET_MAIN_WINDOW };
    ret = InteractionManager::GetInstance()->StopDrag(dropResult);
    ASSERT_EQ(ret, RET_OK);
    EXPECT_TRUE(futureFlag.wait_for(std::chrono::milliseconds(PROMISE_WAIT_SPAN_MS)) !=
        std::future_status::timeout);
}

/**
 * @tc.name: DragManagerTest21
 * @tc.desc: Drag Drawing
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(DragManagerTest, DragManagerTest21, TestSize.Level0)
{
    CALL_TEST_DEBUG;
    std::promise<bool> promiseFlag;
    std::future<bool> futureFlag = promiseFlag.get_future();
    auto callback = [&promiseFlag](const DragNotifyMsg &notifyMessage) {
        FI_HILOGD("displayX:%{public}d, displayY:%{public}d, result:%{public}d, target:%{public}d",
            notifyMessage.displayX, notifyMessage.displayY, notifyMessage.result, notifyMessage.targetPid);
        promiseFlag.set_value(true);
    };
    std::optional<DragData> dragData = CreateDragData(
        MMI::PointerEvent::SOURCE_TYPE_TOUCHSCREEN, POINTER_ID, DRAG_NUM_ONE, false, SHADOW_NUM_ONE);
    EXPECT_TRUE(dragData);
    int32_t ret = InteractionManager::GetInstance()->StartDrag(dragData.value(),
        std::make_shared<TestStartDragListener>(callback));
    ASSERT_EQ(ret, RET_OK);
    std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP_MS));
    DragState dragState;
    ret = InteractionManager::GetInstance()->GetDragState(dragState);
    ASSERT_EQ(ret, RET_OK);
    std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP_MS));
    DragDropResult dropResult { DragResult::DRAG_SUCCESS,
        HAS_CUSTOM_ANIMATION, TARGET_MAIN_WINDOW };
    ret = InteractionManager::GetInstance()->StopDrag(dropResult);
    ASSERT_EQ(ret, RET_OK);
    EXPECT_TRUE(futureFlag.wait_for(std::chrono::milliseconds(PROMISE_WAIT_SPAN_MS)) !=
        std::future_status::timeout);
}

/**
 * @tc.name: DragManagerTest22
 * @tc.desc: Drag Drawing
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(DragManagerTest, DragManagerTest22, TestSize.Level0)
{
    CALL_TEST_DEBUG;
    std::promise<bool> promiseFlag;
    std::future<bool> futureFlag = promiseFlag.get_future();
    auto callback = [&promiseFlag](const DragNotifyMsg &notifyMessage) {
        FI_HILOGD("displayX:%{public}d, displayY:%{public}d, result:%{public}d, target:%{public}d",
            notifyMessage.displayX, notifyMessage.displayY, notifyMessage.result, notifyMessage.targetPid);
        promiseFlag.set_value(true);
    };
    std::optional<DragData> dragData = CreateDragData(
        MMI::PointerEvent::SOURCE_TYPE_TOUCHSCREEN, POINTER_ID, DRAG_NUM_ONE, false, SHADOW_NUM_ONE);
    EXPECT_TRUE(dragData);
    int32_t ret = InteractionManager::GetInstance()->StartDrag(dragData.value(),
        std::make_shared<TestStartDragListener>(callback));
    ASSERT_EQ(ret, RET_OK);
    std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP_MS));
    DragAction dragAction { DragAction::INVALID };
    ret = InteractionManager::GetInstance()->GetDragAction(dragAction);
    ASSERT_EQ(ret, RET_OK);
    std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP_MS));
    DragDropResult dropResult { DragResult::DRAG_SUCCESS,
        HAS_CUSTOM_ANIMATION, TARGET_MAIN_WINDOW };
    ret = InteractionManager::GetInstance()->StopDrag(dropResult);
    ASSERT_EQ(ret, RET_OK);
    EXPECT_TRUE(futureFlag.wait_for(std::chrono::milliseconds(PROMISE_WAIT_SPAN_MS)) !=
        std::future_status::timeout);
}

/**
 * @tc.name: DragManagerTest23
 * @tc.desc: Drag Drawing
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(DragManagerTest, DragManagerTest23, TestSize.Level0)
{
    CALL_TEST_DEBUG;
    std::promise<bool> promiseFlag;
    std::future<bool> futureFlag = promiseFlag.get_future();
    auto callback = [&promiseFlag](const DragNotifyMsg &notifyMessage) {
        FI_HILOGD("displayX:%{public}d, displayY:%{public}d, result:%{public}d, target:%{public}d",
            notifyMessage.displayX, notifyMessage.displayY, notifyMessage.result, notifyMessage.targetPid);
        promiseFlag.set_value(true);
    };
    std::optional<DragData> dragData = CreateDragData(
        MMI::PointerEvent::SOURCE_TYPE_TOUCHSCREEN, POINTER_ID, DRAG_NUM_ONE, false, SHADOW_NUM_ONE);
    EXPECT_TRUE(dragData);
    int32_t ret = InteractionManager::GetInstance()->StartDrag(dragData.value(),
        std::make_shared<TestStartDragListener>(callback));
    ASSERT_EQ(ret, RET_OK);
    std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP_MS));
    std::string extraInfo;
    ret = InteractionManager::GetInstance()->GetExtraInfo(extraInfo);
    ASSERT_EQ(ret, RET_OK);
    std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP_MS));
    DragDropResult dropResult { DragResult::DRAG_SUCCESS,
        HAS_CUSTOM_ANIMATION, TARGET_MAIN_WINDOW };
    ret = InteractionManager::GetInstance()->StopDrag(dropResult);
    ASSERT_EQ(ret, RET_OK);
    EXPECT_TRUE(futureFlag.wait_for(std::chrono::milliseconds(PROMISE_WAIT_SPAN_MS)) !=
        std::future_status::timeout);
}

/**
 * @tc.name: DragManagerTest24
 * @tc.desc: Drag Drawing
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(DragManagerTest, DragManagerTest24, TestSize.Level0)
{
    CALL_TEST_DEBUG;
    std::promise<bool> promiseFlag;
    std::future<bool> futureFlag = promiseFlag.get_future();
    auto callback = [&promiseFlag](const DragNotifyMsg &notifyMessage) {
        FI_HILOGD("displayX:%{public}d, displayY:%{public}d, result:%{public}d, target:%{public}d",
            notifyMessage.displayX, notifyMessage.displayY, notifyMessage.result, notifyMessage.targetPid);
        promiseFlag.set_value(true);
    };
    std::optional<DragData> dragData = CreateDragData(
        MMI::PointerEvent::SOURCE_TYPE_TOUCHSCREEN, POINTER_ID, DRAG_NUM_ONE, false, SHADOW_NUM_ONE);
    EXPECT_TRUE(dragData);
    int32_t ret = InteractionManager::GetInstance()->StartDrag(dragData.value(),
        std::make_shared<TestStartDragListener>(callback));
    ASSERT_EQ(ret, RET_OK);
    std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP_MS));
    PreviewStyle previewStyleIn;
    previewStyleIn.types = { PreviewType::FOREGROUND_COLOR };
    previewStyleIn.foregroundColor = FOREGROUND_COLOR_IN;
    ret = InteractionManager::GetInstance()->UpdatePreviewStyle(previewStyleIn);
    ASSERT_EQ(ret, RET_OK);
    std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP_MS));
    PreviewStyle previewStyleOut;
    previewStyleOut.types = { PreviewType::FOREGROUND_COLOR };
    previewStyleOut.foregroundColor = FOREGROUND_COLOR_OUT;
    ret = InteractionManager::GetInstance()->UpdatePreviewStyle(previewStyleOut);
    EXPECT_EQ(ret, RET_OK);
    std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP_MS));
    DragDropResult dropResult { DragResult::DRAG_SUCCESS,
        HAS_CUSTOM_ANIMATION, TARGET_MAIN_WINDOW };
    ret = InteractionManager::GetInstance()->StopDrag(dropResult);
    ASSERT_EQ(ret, RET_OK);
    EXPECT_TRUE(futureFlag.wait_for(std::chrono::milliseconds(PROMISE_WAIT_SPAN_MS)) !=
        std::future_status::timeout);
}

/**
 * @tc.name: DragManagerTest25
 * @tc.desc: Drag Drawing
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(DragManagerTest, DragManagerTest25, TestSize.Level0)
{
    CALL_TEST_DEBUG;
    std::promise<bool> promiseFlag;
    std::future<bool> futureFlag = promiseFlag.get_future();
    auto callback = [&promiseFlag](const DragNotifyMsg &notifyMessage) {
        FI_HILOGD("displayX:%{public}d, displayY:%{public}d, result:%{public}d, target:%{public}d",
            notifyMessage.displayX, notifyMessage.displayY, notifyMessage.result, notifyMessage.targetPid);
        promiseFlag.set_value(true);
    };
    std::optional<DragData> dragData = CreateDragData(
        MMI::PointerEvent::SOURCE_TYPE_TOUCHSCREEN, POINTER_ID, DRAG_NUM_ONE, false, SHADOW_NUM_ONE);
    EXPECT_TRUE(dragData);
    int32_t ret = InteractionManager::GetInstance()->StartDrag(dragData.value(),
        std::make_shared<TestStartDragListener>(callback));
    ASSERT_EQ(ret, RET_OK);
    std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP_MS));
    PreviewStyle previewStyleIn;
    previewStyleIn.types = { PreviewType::FOREGROUND_COLOR };
    previewStyleIn.foregroundColor = FOREGROUND_COLOR_IN;
    PreviewAnimation animationIn;
    AssignToAnimation(animationIn);
    ret = InteractionManager::GetInstance()->UpdatePreviewStyleWithAnimation(previewStyleIn, animationIn);
    ASSERT_EQ(ret, RET_OK);
    PreviewStyle previewStyleOut;
    previewStyleOut.types = { PreviewType::FOREGROUND_COLOR };
    previewStyleOut.foregroundColor = FOREGROUND_COLOR_OUT;
    PreviewAnimation animationOut;
    AssignToAnimation(animationOut);
    ret = InteractionManager::GetInstance()->UpdatePreviewStyleWithAnimation(previewStyleOut, animationOut);
    EXPECT_EQ(ret, RET_OK);
    std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP_MS));
    DragDropResult dropResult { DragResult::DRAG_SUCCESS,
        HAS_CUSTOM_ANIMATION, TARGET_MAIN_WINDOW };
    ret = InteractionManager::GetInstance()->StopDrag(dropResult);
    ASSERT_EQ(ret, RET_OK);
    EXPECT_TRUE(futureFlag.wait_for(std::chrono::milliseconds(PROMISE_WAIT_SPAN_MS)) !=
        std::future_status::timeout);
}

/**
 * @tc.name: DragManagerTest26
 * @tc.desc: Drag Drawing
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(DragManagerTest, DragManagerTest26, TestSize.Level0)
{
    CALL_TEST_DEBUG;
    std::promise<bool> promiseFlag;
    std::future<bool> futureFlag = promiseFlag.get_future();
    auto callback = [&promiseFlag](const DragNotifyMsg &notifyMessage) {
        FI_HILOGD("displayX:%{public}d, displayY:%{public}d, result:%{public}d, target:%{public}d",
            notifyMessage.displayX, notifyMessage.displayY, notifyMessage.result, notifyMessage.targetPid);
        promiseFlag.set_value(true);
    };
    std::optional<DragData> dragData = CreateDragData(
        MMI::PointerEvent::SOURCE_TYPE_TOUCHSCREEN, POINTER_ID, DRAG_NUM_ONE, false, SHADOW_NUM_ONE);
    EXPECT_TRUE(dragData);
    const std::string udType = "general.message";
    constexpr int64_t recordSize = 20;
    std::map<std::string, int64_t> summarys = { { udType, recordSize } };
    dragData.value().summarys = summarys;
    int32_t ret = InteractionManager::GetInstance()->StartDrag(dragData.value(),
        std::make_shared<TestStartDragListener>(callback));
    ASSERT_EQ(ret, RET_OK);
    std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP_MS));
    summarys.clear();
    ret = InteractionManager::GetInstance()->GetDragSummary(summarys);
    ASSERT_EQ(ret, RET_OK);
    std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP_MS));
    DragDropResult dropResult { DragResult::DRAG_SUCCESS,
        HAS_CUSTOM_ANIMATION, TARGET_MAIN_WINDOW };
    ret = InteractionManager::GetInstance()->StopDrag(dropResult);
    ASSERT_EQ(ret, RET_OK);
    EXPECT_TRUE(futureFlag.wait_for(std::chrono::milliseconds(PROMISE_WAIT_SPAN_MS)) !=
        std::future_status::timeout);
}

/**
 * @tc.name: DragManagerTest27
 * @tc.desc: Drag Drawing
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(DragManagerTest, DragManagerTest27, TestSize.Level0)
{
    CALL_TEST_DEBUG;
    std::promise<bool> promiseFlag;
    std::future<bool> futureFlag = promiseFlag.get_future();
    auto callback = [&promiseFlag](const DragNotifyMsg &notifyMessage) {
        FI_HILOGD("displayX:%{public}d, displayY:%{public}d, result:%{public}d, target:%{public}d",
            notifyMessage.displayX, notifyMessage.displayY, notifyMessage.result, notifyMessage.targetPid);
        promiseFlag.set_value(true);
    };
    std::optional<DragData> dragData = CreateDragData(
        MMI::PointerEvent::SOURCE_TYPE_TOUCHSCREEN, POINTER_ID, DRAG_NUM_ONE, false, SHADOW_NUM_ONE);
    EXPECT_TRUE(dragData);
    int32_t ret = InteractionManager::GetInstance()->StartDrag(dragData.value(),
        std::make_shared<TestStartDragListener>(callback));
    ASSERT_EQ(ret, RET_OK);
    std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP_MS));
    ret = InteractionManager::GetInstance()->EnterTextEditorArea(true);
    ASSERT_EQ(ret, RET_OK);
    std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP_MS));
    ret = InteractionManager::GetInstance()->EnterTextEditorArea(false);
    ASSERT_EQ(ret, RET_OK);
    std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP_MS));
    DragDropResult dropResult { DragResult::DRAG_SUCCESS,
        HAS_CUSTOM_ANIMATION, TARGET_MAIN_WINDOW };
    ret = InteractionManager::GetInstance()->StopDrag(dropResult);
    ASSERT_EQ(ret, RET_OK);
    EXPECT_TRUE(futureFlag.wait_for(std::chrono::milliseconds(PROMISE_WAIT_SPAN_MS)) !=
        std::future_status::timeout);
}

/**
 * @tc.name: DragManagerTest28
 * @tc.desc: Drag Drawing
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(DragManagerTest, DragManagerTest28, TestSize.Level0)
{
    CALL_TEST_DEBUG;
    std::promise<bool> promiseFlag;
    std::future<bool> futureFlag = promiseFlag.get_future();
    auto callback = [&promiseFlag](const DragNotifyMsg &notifyMessage) {
        FI_HILOGD("displayX:%{public}d, displayY:%{public}d, result:%{public}d, target:%{public}d",
            notifyMessage.displayX, notifyMessage.displayY, notifyMessage.result, notifyMessage.targetPid);
        promiseFlag.set_value(true);
    };
    std::optional<DragData> dragData = CreateDragData(
        MMI::PointerEvent::SOURCE_TYPE_TOUCHSCREEN, POINTER_ID, DRAG_NUM_ONE, false, SHADOW_NUM_ONE);
    EXPECT_TRUE(dragData);
    int32_t ret = InteractionManager::GetInstance()->StartDrag(dragData.value(),
        std::make_shared<TestStartDragListener>(callback));
    ASSERT_EQ(ret, RET_OK);
    std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP_MS));
    ret = InteractionManager::GetInstance()->AddPrivilege();
    ASSERT_EQ(ret, RET_OK);
    std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP_MS));
    DragDropResult dropResult { DragResult::DRAG_SUCCESS,
        HAS_CUSTOM_ANIMATION, TARGET_MAIN_WINDOW };
    ret = InteractionManager::GetInstance()->StopDrag(dropResult);
    ASSERT_EQ(ret, RET_OK);
    EXPECT_TRUE(futureFlag.wait_for(std::chrono::milliseconds(PROMISE_WAIT_SPAN_MS)) !=
        std::future_status::timeout);
}

/**
 * @tc.name: DragManagerTest29
 * @tc.desc: Drag Drawing
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(DragManagerTest, DragManagerTest29, TestSize.Level0)
{
    CALL_TEST_DEBUG;
    std::promise<bool> promiseFlag;
    std::future<bool> futureFlag = promiseFlag.get_future();
    auto callback = [&promiseFlag](const DragNotifyMsg &notifyMessage) {
        FI_HILOGD("displayX:%{public}d, displayY:%{public}d, result:%{public}d, target:%{public}d",
            notifyMessage.displayX, notifyMessage.displayY, notifyMessage.result, notifyMessage.targetPid);
        promiseFlag.set_value(true);
    };
    std::optional<DragData> dragData = CreateDragData(
        MMI::PointerEvent::SOURCE_TYPE_TOUCHSCREEN, POINTER_ID, DRAG_NUM_ONE, false, SHADOW_NUM_ONE);
    EXPECT_TRUE(dragData);
    int32_t ret = InteractionManager::GetInstance()->StartDrag(dragData.value(),
        std::make_shared<TestStartDragListener>(callback));
    ASSERT_EQ(ret, RET_OK);
    ret = InteractionManager::GetInstance()->StartDrag(dragData.value(),
        std::make_shared<TestStartDragListener>(callback));
    ASSERT_EQ(ret, RET_ERR);
    std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP_MS));
    DragDropResult dropResult { DragResult::DRAG_SUCCESS,
        HAS_CUSTOM_ANIMATION, TARGET_MAIN_WINDOW };
    ret = InteractionManager::GetInstance()->StopDrag(dropResult);
    ASSERT_EQ(ret, RET_OK);
    EXPECT_TRUE(futureFlag.wait_for(std::chrono::milliseconds(PROMISE_WAIT_SPAN_MS)) !=
        std::future_status::timeout);
}

/**
 * @tc.name: DragManagerTest30
 * @tc.desc: Drag Drawing
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(DragManagerTest, DragManagerTest30, TestSize.Level0)
{
    CALL_TEST_DEBUG;
    DragDropResult dropResult { DragResult::DRAG_SUCCESS,
        HAS_CUSTOM_ANIMATION, TARGET_MAIN_WINDOW };
    int32_t ret = InteractionManager::GetInstance()->StopDrag(dropResult);
    ASSERT_EQ(ret, RET_ERR);
}

/**
 * @tc.name: DragManagerTest31
 * @tc.desc: Drag Drawing
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(DragManagerTest, DragManagerTest31, TestSize.Level0)
{
    CALL_TEST_DEBUG;
    int32_t ret = InteractionManager::GetInstance()->SetDragWindowVisible(true);
    ASSERT_EQ(ret, RET_ERR);
    std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP_MS));
    ret = InteractionManager::GetInstance()->SetDragWindowVisible(false);
    ASSERT_EQ(ret, RET_ERR);
}

/**
 * @tc.name: DragManagerTest32
 * @tc.desc: Drag Drawing
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(DragManagerTest, DragManagerTest32, TestSize.Level0)
{
    CALL_TEST_DEBUG;
    int32_t ret = -1;
    std::vector<DragCursorStyle> dragCursorStyles = {DragCursorStyle::FORBIDDEN,
        DragCursorStyle::COPY, DragCursorStyle::MOVE};
    for (const auto& dragCursorStyle : dragCursorStyles) {
        ret = InteractionManager::GetInstance()->UpdateDragStyle(dragCursorStyle);
        ASSERT_EQ(ret, RET_ERR);
    }
}

/**
 * @tc.name: DragManagerTest33
 * @tc.desc: Drag Drawing
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(DragManagerTest, DragManagerTest33, TestSize.Level0)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<Media::PixelMap> pixelMap = CreatePixelMap(PIXEL_MAP_WIDTH, PIXEL_MAP_HEIGHT);
    ASSERT_NE(pixelMap, nullptr);
    ShadowInfo shadowInfo = { pixelMap, 0, 0 };
    int32_t ret = InteractionManager::GetInstance()->UpdateShadowPic(shadowInfo);
    ASSERT_EQ(ret, RET_ERR);
}

/**
 * @tc.name: DragManagerTest34
 * @tc.desc: Drag Drawing
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(DragManagerTest, DragManagerTest34, TestSize.Level0)
{
    CALL_TEST_DEBUG;
    int32_t pid = InteractionManager::GetInstance()->GetDragTargetPid();
    EXPECT_GT(pid, 0);
}

/**
 * @tc.name: DragManagerTest35
 * @tc.desc: Drag Drawing
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(DragManagerTest, DragManagerTest35, TestSize.Level0)
{
    CALL_TEST_DEBUG;
    std::string udKey;
    int32_t ret = InteractionManager::GetInstance()->GetUdKey(udKey);
    ASSERT_EQ(ret, RET_ERR);
}

/**
 * @tc.name: DragManagerTest36
 * @tc.desc: Drag Drawing
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(DragManagerTest, DragManagerTest36, TestSize.Level0)
{
    CALL_TEST_DEBUG;
    int32_t offsetX = 0;
    int32_t offsetY = 0;
    int32_t width = 0;
    int32_t height = 0;
    int32_t ret = InteractionManager::GetInstance()->GetShadowOffset(offsetX, offsetY, width, height);
    ASSERT_EQ(ret, RET_ERR);
}

/**
 * @tc.name: DragManagerTest37
 * @tc.desc: Drag Drawing
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(DragManagerTest, DragManagerTest37, TestSize.Level0)
{
    CALL_TEST_DEBUG;
    DragData replyDragData;
    int32_t ret = InteractionManager::GetInstance()->GetDragData(replyDragData);
    ASSERT_EQ(ret, RET_ERR);
}

/**
 * @tc.name: DragManagerTest38
 * @tc.desc: Drag Drawing
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(DragManagerTest, DragManagerTest38, TestSize.Level0)
{
    CALL_TEST_DEBUG;
    PreviewStyle previewStyleOut;
    previewStyleOut.types = { PreviewType::FOREGROUND_COLOR };
    previewStyleOut.foregroundColor = FOREGROUND_COLOR_OUT;
    int32_t ret = InteractionManager::GetInstance()->UpdatePreviewStyle(previewStyleOut);
    ASSERT_EQ(ret, RET_ERR);
    PreviewStyle previewStyleIn;
    previewStyleIn.types = { PreviewType::FOREGROUND_COLOR };
    previewStyleIn.foregroundColor = FOREGROUND_COLOR_IN;
    ret = InteractionManager::GetInstance()->UpdatePreviewStyle(previewStyleIn);
    ASSERT_EQ(ret, RET_ERR);
}

/**
 * @tc.name: DragManagerTest39
 * @tc.desc: Drag Drawing
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(DragManagerTest, DragManagerTest39, TestSize.Level0)
{
    CALL_TEST_DEBUG;
    PreviewStyle previewStyleIn;
    previewStyleIn.types = { PreviewType::FOREGROUND_COLOR };
    previewStyleIn.foregroundColor = FOREGROUND_COLOR_IN;
    PreviewAnimation animationIn;
    AssignToAnimation(animationIn);
    int32_t ret = InteractionManager::GetInstance()->UpdatePreviewStyleWithAnimation(previewStyleIn, animationIn);
    ASSERT_EQ(ret, RET_ERR);
    PreviewStyle previewStyleOut;
    previewStyleOut.types = { PreviewType::FOREGROUND_COLOR };
    previewStyleOut.foregroundColor = FOREGROUND_COLOR_OUT;
    PreviewAnimation animationOut;
    AssignToAnimation(animationOut);
    ret = InteractionManager::GetInstance()->UpdatePreviewStyleWithAnimation(previewStyleOut, animationOut);
    ASSERT_EQ(ret, RET_ERR);
}

/**
 * @tc.name: DragManagerTest40
 * @tc.desc: Drag Drawing
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(DragManagerTest, DragManagerTest40, TestSize.Level0)
{
    CALL_TEST_DEBUG;
    int32_t ret = InteractionManager::GetInstance()->EnterTextEditorArea(true);
    ASSERT_EQ(ret, RET_ERR);
    ret = InteractionManager::GetInstance()->EnterTextEditorArea(false);
    ASSERT_EQ(ret, RET_ERR);
}

/**
 * @tc.name: DragManagerTest41
 * @tc.desc: Drag Drawing
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(DragManagerTest, DragManagerTest41, TestSize.Level0)
{
    CALL_TEST_DEBUG;
    DragAction dragAction { DragAction::INVALID };
    int32_t ret = InteractionManager::GetInstance()->GetDragAction(dragAction);
    ASSERT_EQ(ret, RET_ERR);
}

/**
 * @tc.name: DragManagerTest42
 * @tc.desc: Drag Drawing
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(DragManagerTest, DragManagerTest42, TestSize.Level0)
{
    CALL_TEST_DEBUG;
    std::string extraInfo;
    int32_t ret = InteractionManager::GetInstance()->GetExtraInfo(extraInfo);
    ASSERT_EQ(ret, RET_ERR);
}

/**
 * @tc.name: DragManagerTest43
 * @tc.desc: Drag Drawing
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(DragManagerTest, DragManagerTest43, TestSize.Level0)
{
    CALL_TEST_DEBUG;
    int32_t ret = InteractionManager::GetInstance()->AddPrivilege();
    ASSERT_EQ(ret, RET_ERR);
}

/**
 * @tc.name: DragManagerTest44
 * @tc.desc: Drag Drawing
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(DragManagerTest, DragManagerTest44, TestSize.Level0)
{
    CALL_TEST_DEBUG;
    MessageParcel data;
    StopDragParam param {};
    bool ret = param.Unmarshalling(data);
    EXPECT_FALSE(ret);
}

/**
 * @tc.name: DragManagerTest45
 * @tc.desc: Drag Drawing
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(DragManagerTest, DragManagerTest45, TestSize.Level0)
{
    CALL_TEST_DEBUG;
    MessageParcel data;
    SetDragWindowVisibleParam param {};
    bool ret = param.Unmarshalling(data);
    EXPECT_FALSE(ret);
}

/**
 * @tc.name: DragManagerTest46
 * @tc.desc: Drag Drawing
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(DragManagerTest, DragManagerTest46, TestSize.Level0)
{
    CALL_TEST_DEBUG;
    int32_t pid = InteractionManager::GetInstance()->GetDragTargetPid();
    MessageParcel data;
    GetDragTargetPidReply targetPidReply { pid };
    bool ret = targetPidReply.Marshalling(data);
    EXPECT_TRUE(ret);
}

/**
 * @tc.name: DragManagerTest47
 * @tc.desc: Drag Drawing
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(DragManagerTest, DragManagerTest47, TestSize.Level0)
{
    CALL_TEST_DEBUG;
    int32_t pid = InteractionManager::GetInstance()->GetDragTargetPid();
    MessageParcel data;
    GetDragTargetPidReply targetPidReply { pid };
    bool ret = targetPidReply.Marshalling(data);
    EXPECT_TRUE(ret);
}

/**
 * @tc.name: DragManagerTest48
 * @tc.desc: Drag Drawing
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(DragManagerTest, DragManagerTest48, TestSize.Level0)
{
    CALL_TEST_DEBUG;
    std::string udKey;
    MessageParcel data;
    GetUdKeyReply udKeyReply { std::move(udKey) };
    bool ret = udKeyReply.Marshalling(data);
    EXPECT_TRUE(ret);
}

/**
 * @tc.name: DragManagerTest49
 * @tc.desc: Drag Drawing
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(DragManagerTest, DragManagerTest49, TestSize.Level0)
{
    CALL_TEST_DEBUG;
    ShadowOffset shadowOffset {};
    MessageParcel data;
    GetShadowOffsetReply shadowOffsetReply { shadowOffset };
    bool ret = shadowOffsetReply.Marshalling(data);
    EXPECT_TRUE(ret);
}

/**
 * @tc.name: DragManagerTest50
 * @tc.desc: Drag Drawing
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(DragManagerTest, DragManagerTest50, TestSize.Level0)
{
    CALL_TEST_DEBUG;
    UpdatePreviewAnimationParam param {};
    MessageParcel data;
    bool ret = param.Unmarshalling(data);
    EXPECT_FALSE(ret);
}

/**
 * @tc.name: DragManagerTest51
 * @tc.desc: Drag Drawing
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(DragManagerTest, DragManagerTest51, TestSize.Level0)
{
    CALL_TEST_DEBUG;
    std::map<std::string, int64_t> summaries;
    GetDragSummaryReply summaryReply { std::move(summaries) };
    MessageParcel data;
    bool ret = summaryReply.Marshalling(data);
    EXPECT_TRUE(ret);
}

/**
 * @tc.name: DragManagerTest52
 * @tc.desc: Drag Drawing
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(DragManagerTest, DragManagerTest52, TestSize.Level0)
{
    CALL_TEST_DEBUG;
    DragState dragState {};
    GetDragStateReply dragStateReply { dragState };
    MessageParcel data;
    bool ret = dragStateReply.Marshalling(data);
    EXPECT_TRUE(ret);
}

/**
 * @tc.name: DragManagerTest53
 * @tc.desc: Drag Drawing
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(DragManagerTest, DragManagerTest53, TestSize.Level0)
{
    CALL_TEST_DEBUG;
    DragAction dragAction {};
    GetDragActionReply dragActionReply { dragAction };
    MessageParcel data;
    bool ret = dragActionReply.Marshalling(data);
    EXPECT_TRUE(ret);
}

/**
 * @tc.name: DragManagerTest54
 * @tc.desc: Drag Drawing
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(DragManagerTest, DragManagerTest54, TestSize.Level0)
{
    CALL_TEST_DEBUG;
    std::string extraInfo;
    GetExtraInfoReply extraInfoReply { std::move(extraInfo) };
    MessageParcel data;
    bool ret = extraInfoReply.Marshalling(data);
    EXPECT_TRUE(ret);
}

/**
 * @tc.name: DragManagerTest55
 * @tc.desc: Drag Drawing
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(DragManagerTest, DragManagerTest55, TestSize.Level0)
{
    CALL_TEST_DEBUG;
    std::string extraInfo;
    GetExtraInfoReply extraInfoReply { std::move(extraInfo) };
    MessageParcel data;
    bool ret = extraInfoReply.Marshalling(data);
    EXPECT_TRUE(ret);
}

/**
 * @tc.name: DragManagerTest56
 * @tc.desc: Drag Drawing
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(DragManagerTest, DragManagerTest56, TestSize.Level0)
{
    CALL_TEST_DEBUG;
    std::string extraInfo;
    UpdateDragStyleParam param;
    MessageParcel data;
    bool ret = param.Marshalling(data);
    EXPECT_TRUE(ret);
}

/**
 * @tc.name: DragManagerTest57
 * @tc.desc: Drag Drawing
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(DragManagerTest, DragManagerTest57, TestSize.Level0)
{
    CALL_TEST_DEBUG;
    std::string extraInfo;
    UpdateDragStyleParam param;
    MessageParcel data;
    bool ret = param.Unmarshalling(data);
    EXPECT_FALSE(ret);
}

/**
 * @tc.name: DragManagerTest58
 * @tc.desc: Drag Drawing
 * @tc.type: FUNCdSession
 * @tc.require:
 */
HWTEST_F(DragManagerTest, DragManagerTest58, TestSize.Level0)
{
    CALL_TEST_DEBUG;
    std::string extraInfo;
    UpdateShadowPicParam param;
    MessageParcel data;
    bool ret = param.Unmarshalling(data);
    EXPECT_FALSE(ret);
}

/**
 * @tc.name: DragManagerTest59
 * @tc.desc: Drag Drawing
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(DragManagerTest, DragManagerTest59, TestSize.Level0)
{
    CALL_TEST_DEBUG;
    std::string extraInfo;
    UpdateShadowPicParam param;
    MessageParcel data;
    bool ret = param.Marshalling(data);
    EXPECT_FALSE(ret);
}

/**
 * @tc.name: DragManagerTest60
 * @tc.desc: Drag Drawing
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(DragManagerTest, DragManagerTest60, TestSize.Level0)
{
    CALL_TEST_DEBUG;
    std::string extraInfo;
    UpdatePreviewStyleParam param;
    MessageParcel data;
    bool ret = param.Marshalling(data);
    EXPECT_TRUE(ret);
}

/**
 * @tc.name: DragManagerTest61
 * @tc.desc: Drag Drawingx`
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(DragManagerTest, DragManagerTest61, TestSize.Level0)
{
    CALL_TEST_DEBUG;
    std::string extraInfo;
    UpdatePreviewStyleParam param;
    MessageParcel data;
    bool ret = param.Unmarshalling(data);
    EXPECT_FALSE(ret);
}

/**
 * @tc.name: DragManagerTest62
 * @tc.desc: Drag Drawingx`
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(DragManagerTest, DragManagerTest62, TestSize.Level0)
{
    CALL_TEST_DEBUG;
    DragDropResult dropResult { DragResult::DRAG_SUCCESS, HAS_CUSTOM_ANIMATION, WINDOW_ID };
    StopDragParam param { dropResult };
    MessageParcel data;
    bool ret = param.Marshalling(data);
    EXPECT_TRUE(ret);
    ret = param.Unmarshalling(data);
    EXPECT_TRUE(ret);
}

/**
 * @tc.name: DragManagerTest63
 * @tc.desc: Drag Drawingx`
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(DragManagerTest, DragManagerTest63, TestSize.Level0)
{
    CALL_TEST_DEBUG;
    DragDropResult dropResult { DragResult::DRAG_SUCCESS, HAS_CUSTOM_ANIMATION,
        WINDOW_ID, static_cast<DragBehavior>(-2)};
    StopDragParam param { dropResult };
    MessageParcel data;
    bool ret = param.Marshalling(data);
    EXPECT_TRUE(ret);
    ret = param.Unmarshalling(data);
    EXPECT_FALSE(ret);
}

/**
 * @tc.name: DragManagerTest64
 * @tc.desc: Drag Drawingx`
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(DragManagerTest, DragManagerTest64, TestSize.Level0)
{
    CALL_TEST_DEBUG;
    MessageParcel data;
    SetDragWindowVisibleParam param { true, true };
    bool ret = param.Marshalling(data);
    EXPECT_TRUE(ret);
    ret = param.Unmarshalling(data);
    EXPECT_TRUE(ret);
}

/**
 * @tc.name: DragManagerTest65
 * @tc.desc: Drag Drawingx`
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(DragManagerTest, DragManagerTest65, TestSize.Level0)
{
    CALL_TEST_DEBUG;
    MessageParcel data;
    UpdateDragStyleParam param { DragCursorStyle::DEFAULT, -1 };
    bool ret = param.Marshalling(data);
    EXPECT_TRUE(ret);
    ret = param.Unmarshalling(data);
    EXPECT_TRUE(ret);
}

/**
 * @tc.name: DragManagerTest66
 * @tc.desc: Drag Drawingx`
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(DragManagerTest, DragManagerTest66, TestSize.Level0)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<Media::PixelMap> pixelMap = CreatePixelMap(PIXEL_MAP_WIDTH, PIXEL_MAP_HEIGHT);
    ASSERT_NE(pixelMap, nullptr);
    ShadowInfo shadowInfo = { pixelMap, 0, 0 };
    std::string extraInfo;
    UpdateShadowPicParam param { shadowInfo };
    MessageParcel data;
    bool ret = param.Marshalling(data);
    EXPECT_TRUE(ret);
    ret = param.Unmarshalling(data);
    EXPECT_TRUE(ret);
}

/**
 * @tc.name: DragManagerTest67
 * @tc.desc: Drag Drawingx`
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(DragManagerTest, DragManagerTest67, TestSize.Level0)
{
    CALL_TEST_DEBUG;
    MessageParcel data;
    DragData dragData {};
    StartDragParam param { dragData };
    bool ret = param.Marshalling(data);
    EXPECT_FALSE(ret);
    ret = param.Unmarshalling(data);
    EXPECT_FALSE(ret);
}

/**
 * @tc.name: DragManagerTest68
 * @tc.desc: Drag Drawingx`
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(DragManagerTest, DragManagerTest68, TestSize.Level0)
{
    CALL_TEST_DEBUG;
    std::shared_ptr<Media::PixelMap> pixelMap = CreatePixelMap(PIXEL_MAP_WIDTH, PIXEL_MAP_HEIGHT);
    ASSERT_NE(pixelMap, nullptr);
    ShadowInfo shadowInfo = { pixelMap, 1, 0 };
    ShadowInfo otherShadowInfo = {};
    EXPECT_TRUE(shadowInfo != otherShadowInfo);
    ShadowOffset shadowOffset {};
    ShadowOffset otherShadowOffset {};
    EXPECT_FALSE(shadowOffset != otherShadowOffset);
    DragData dragData {};
    DragData otherDragData {};
    EXPECT_FALSE(dragData != otherDragData);
    PreviewStyle previewStyle {};
    PreviewStyle otherPreviewStyle {};
    EXPECT_FALSE(previewStyle != otherPreviewStyle);
    Data data {};
    Data otherData {};
    EXPECT_TRUE(data != otherData);
    DragItemStyle dragItemStyle = { 1, 1, 0 };
    DragItemStyle otherDragItemStyle = {};
    DragItemStyle dragItemStyleOne = { 1, 1, 0 };
    EXPECT_TRUE(dragItemStyle != otherDragItemStyle);
    EXPECT_TRUE(dragItemStyle == dragItemStyleOne);
}

/**
 * @tc.name: DragManagerTest69
 * @tc.desc: Drag Drawingx`
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(DragManagerTest, DragManagerTest69, TestSize.Level0)
{
    CALL_TEST_DEBUG;
    int32_t ret = InteractionManager::GetInstance()->RotateDragWindowSync(nullptr);
    EXPECT_EQ(ret, RET_ERR);
}
} // namespace DeviceStatus
} // namespace Msdp
} // namespace OHOS
