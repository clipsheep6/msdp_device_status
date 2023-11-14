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
#include <vector>

#include <unistd.h>

#include <gtest/gtest.h>
#include "pointer_event.h"
#include "securec.h"

#include "devicestatus_define.h"
#include "devicestatus_errors.h"
#include "interaction_manager.h"

namespace OHOS {
namespace Msdp {
namespace DeviceStatus {
using namespace testing::ext;
namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL { LOG_CORE, MSDP_DOMAIN_ID, "InteractionDragDrawingTest" };
constexpr uint32_t DEFAULT_ICON_COLOR { 0xFF };
constexpr int32_t TIME_WAIT_FOR_OP_MS { 20 };
constexpr int32_t PIXEL_MAP_WIDTH { 300 };
constexpr int32_t PIXEL_MAP_HEIGHT { 300 };
constexpr int32_t MIDDLE_PIXEL_MAP_WIDTH { 400 };
constexpr int32_t MIDDLE_PIXEL_MAP_HEIGHT { 400 };
constexpr int32_t MAX_PIXEL_MAP_WIDTH { 600 };
constexpr int32_t MAX_PIXEL_MAP_HEIGHT { 600 };
constexpr int32_t POINTER_ID { 0 };
constexpr int32_t DISPLAY_ID { 0 };
constexpr int32_t DISPLAY_X { 50 };
constexpr int32_t DISPLAY_Y { 50 };
constexpr int32_t DRAG_NUM_ONE { 1 };
constexpr int32_t DRAG_NUM_MULTIPLE { 10 };
constexpr int32_t INT32_BYTE { 4 };
constexpr int32_t PROMISE_WAIT_SPAN_MS { 2000 };
constexpr int32_t TIME_WAIT_FOR_UPDATE_DRAG_STYLE { 50 };
constexpr int32_t TIME_WAIT_FOR_ANIMATION_END { 1000 };
constexpr int32_t WINDOW_ID { -1 };
constexpr bool HAS_CANCELED_ANIMATION { true };
constexpr bool HAS_CUSTOM_ANIMATION { true };
constexpr bool NOT_HAS_CUSTOM_ANIMATION { false };
constexpr bool DRAG_WINDOW_VISIBLE { true };
constexpr int32_t FOREGROUND_COLOR { 0x99FF0000 };
constexpr int32_t RADIUS { 42 };
constexpr int32_t ALPHA { 51 };
const std::string UD_KEY { "Unified data key" };
const std::string FILTER_INFO { "Undefined filter info" };
const std::string EXTRA_INFO { "Undefined extra info" };
} // namespace

class InteractionDragDrawingTest : public testing::Test {
public:
    void SetUp();
    void TearDown();
    static void SetUpTestCase();
    static std::shared_ptr<Media::PixelMap> CreatePixelMap(int32_t width, int32_t height);
    static std::optional<DragData> CreateDragData(int32_t sourceType, int32_t pointerId, int32_t dragNum);
};

void InteractionDragDrawingTest::SetUpTestCase() {}

void InteractionDragDrawingTest::SetUp() {}

void InteractionDragDrawingTest::TearDown()
{
    std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP_MS));
}

std::shared_ptr<Media::PixelMap> InteractionDragDrawingTest::CreatePixelMap(int32_t width, int32_t height)
{
    CALL_DEBUG_ENTER;
    if (width <= 0 || width > MAX_PIXEL_MAP_WIDTH || height <= 0 || height > MAX_PIXEL_MAP_HEIGHT) {
        FI_HILOGE("Size invalid, height:%{public}d, width:%{public}d", height, width);
        return nullptr;
    }
    Media::InitializationOptions opts;
    opts.size.height = height;
    opts.size.width = width;
    opts.pixelFormat = Media::PixelFormat::BGRA_8888;
    opts.alphaType = Media::AlphaType::IMAGE_ALPHA_TYPE_OPAQUE;
    opts.scaleMode = Media::ScaleMode::FIT_TARGET_SIZE;

    int32_t colorLen = width * height;
    uint32_t *pixelColors = new (std::nothrow) uint32_t[colorLen];
    CHKPP(pixelColors);
    int32_t colorByteCount = colorLen * INT32_BYTE;
    errno_t ret = memset_s(pixelColors, colorByteCount, DEFAULT_ICON_COLOR, colorByteCount);
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

std::optional<DragData> InteractionDragDrawingTest::CreateDragData(int32_t sourceType,
    int32_t pointerId, int32_t dragNum)
{
    CALL_DEBUG_ENTER;
    std::shared_ptr<Media::PixelMap> pixelMap = CreatePixelMap(PIXEL_MAP_WIDTH, PIXEL_MAP_HEIGHT);
    if (pixelMap == nullptr) {
        FI_HILOGE("CreatePixelMap failed");
        return std::nullopt;
    }
    DragData dragData;
    dragData.shadowInfo.pixelMap = pixelMap;
    dragData.shadowInfo.x = 0;
    dragData.shadowInfo.y = 0;
    dragData.buffer = std::vector<uint8_t>(MAX_BUFFER_SIZE, 0);
    dragData.udKey = UD_KEY;
    dragData.extraInfo = FILTER_INFO;
    dragData.extraInfo = EXTRA_INFO;
    dragData.sourceType = sourceType;
    dragData.pointerId = pointerId;
    dragData.dragNum = dragNum;
    dragData.displayX = DISPLAY_X;
    dragData.displayY = DISPLAY_Y;
    dragData.displayId = DISPLAY_ID;
    dragData.hasCanceledAnimation = HAS_CANCELED_ANIMATION;
    return dragData;
}

/**
 * @tc.name: InteractionDragDrawingTest_Mouse_DragNum_One
 * @tc.desc: Drag Drawing
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InteractionDragDrawingTest, InteractionDragDrawingTest_Mouse_DragNum_One, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::optional<DragData> dragData = CreateDragData(
        MMI::PointerEvent::SOURCE_TYPE_MOUSE, POINTER_ID, DRAG_NUM_ONE);
    ASSERT_TRUE(dragData);
    std::promise<bool> promiseFlag;
    std::future<bool> futureFlag = promiseFlag.get_future();
    auto callback = [&promiseFlag](const DragNotifyMsg& notifyMessage) {
        FI_HILOGD("displayX:%{public}d, displayY:%{public}d, result:%{public}d, target:%{public}d",
            notifyMessage.displayX, notifyMessage.displayY, notifyMessage.result, notifyMessage.targetPid);
        promiseFlag.set_value(true);
    };
    int32_t ret = InteractionManager::GetInstance()->StartDrag(dragData.value(), callback);
    ASSERT_EQ(ret, RET_OK);
    ret = InteractionManager::GetInstance()->SetDragWindowVisible(DRAG_WINDOW_VISIBLE);
    ASSERT_EQ(ret, RET_OK);
    ret = InteractionManager::GetInstance()->UpdateDragStyle(DragCursorStyle::COPY);
    ASSERT_EQ(ret, RET_OK);
    std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_UPDATE_DRAG_STYLE));
    ret = InteractionManager::GetInstance()->UpdateDragStyle(DragCursorStyle::FORBIDDEN);
    ASSERT_EQ(ret, RET_OK);
    std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_UPDATE_DRAG_STYLE));
    ret = InteractionManager::GetInstance()->UpdateDragStyle(DragCursorStyle::MOVE);
    ASSERT_EQ(ret, RET_OK);
    std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_UPDATE_DRAG_STYLE));
    ret = InteractionManager::GetInstance()->UpdateDragStyle(DragCursorStyle::DEFAULT);
    ASSERT_EQ(ret, RET_OK);
    std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_UPDATE_DRAG_STYLE));
    DragDropResult dropResult { DragResult::DRAG_SUCCESS, HAS_CUSTOM_ANIMATION, WINDOW_ID };
    ret = InteractionManager::GetInstance()->StopDrag(dropResult);
    ASSERT_EQ(ret, RET_OK);
    ASSERT_TRUE(futureFlag.wait_for(std::chrono::milliseconds(PROMISE_WAIT_SPAN_MS)) != std::future_status::timeout);
}

/**
 * @tc.name: InteractionDragDrawingTest_Mouse_DragNum_Multiple
 * @tc.desc: Drag Drawing
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InteractionDragDrawingTest, InteractionDragDrawingTest_Mouse_DragNum_Multiple, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::promise<bool> promiseFlag;
    std::future<bool> futureFlag = promiseFlag.get_future();
    auto callback = [&promiseFlag](const DragNotifyMsg& notifyMessage) {
        FI_HILOGD("displayX:%{public}d, displayY:%{public}d, result:%{public}d",
            notifyMessage.displayX, notifyMessage.displayY, notifyMessage.result);
        promiseFlag.set_value(true);
    };
    std::optional<DragData> dragData = CreateDragData(
        MMI::PointerEvent::SOURCE_TYPE_MOUSE, POINTER_ID, DRAG_NUM_MULTIPLE);
    ASSERT_TRUE(dragData);
    int32_t ret = InteractionManager::GetInstance()->StartDrag(dragData.value(), callback);
    ASSERT_EQ(ret, RET_OK);
    ret = InteractionManager::GetInstance()->SetDragWindowVisible(DRAG_WINDOW_VISIBLE);
    ASSERT_EQ(ret, RET_OK);
    ret = InteractionManager::GetInstance()->UpdateDragStyle(DragCursorStyle::COPY);
    ASSERT_EQ(ret, RET_OK);
    std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_UPDATE_DRAG_STYLE));
    ret = InteractionManager::GetInstance()->UpdateDragStyle(DragCursorStyle::FORBIDDEN);
    ASSERT_EQ(ret, RET_OK);
    std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_UPDATE_DRAG_STYLE));
    ret = InteractionManager::GetInstance()->UpdateDragStyle(DragCursorStyle::DEFAULT);
    ASSERT_EQ(ret, RET_OK);
    std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_UPDATE_DRAG_STYLE));
    ret = InteractionManager::GetInstance()->UpdateDragStyle(DragCursorStyle::MOVE);
    ASSERT_EQ(ret, RET_OK);
    std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_UPDATE_DRAG_STYLE));
    DragDropResult dropResult { DragResult::DRAG_SUCCESS, HAS_CUSTOM_ANIMATION, WINDOW_ID };
    ret = InteractionManager::GetInstance()->StopDrag(dropResult);
    ASSERT_EQ(ret, RET_OK);
    ASSERT_TRUE(futureFlag.wait_for(std::chrono::milliseconds(PROMISE_WAIT_SPAN_MS)) != std::future_status::timeout);
}

/**
 * @tc.name: InteractionDragDrawingTest_Touchscreen_DragNum_One
 * @tc.desc: Drag Drawing
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InteractionDragDrawingTest, InteractionDragDrawingTest_Touchscreen_DragNum_One, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::promise<bool> promiseFlag;
    std::future<bool> futureFlag = promiseFlag.get_future();
    auto callback = [&promiseFlag](const DragNotifyMsg& notifyMessage) {
        FI_HILOGD("displayX:%{public}d, displayY:%{public}d, arget:%{public}d, result:%{public}d",
            notifyMessage.displayX, notifyMessage.displayY, notifyMessage.targetPid, notifyMessage.result);
        promiseFlag.set_value(true);
    };
    std::optional<DragData> dragData = CreateDragData(
        MMI::PointerEvent::SOURCE_TYPE_TOUCHSCREEN, POINTER_ID, DRAG_NUM_ONE);
    ASSERT_TRUE(dragData);
    int32_t ret = InteractionManager::GetInstance()->StartDrag(dragData.value(), callback);
    ASSERT_EQ(ret, RET_OK);
    ret = InteractionManager::GetInstance()->SetDragWindowVisible(DRAG_WINDOW_VISIBLE);
    ASSERT_EQ(ret, RET_OK);
    ret = InteractionManager::GetInstance()->UpdateDragStyle(DragCursorStyle::COPY);
    ASSERT_EQ(ret, RET_OK);
    std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_UPDATE_DRAG_STYLE));
    ret = InteractionManager::GetInstance()->UpdateDragStyle(DragCursorStyle::DEFAULT);
    ASSERT_EQ(ret, RET_OK);
    std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_UPDATE_DRAG_STYLE));
    ret = InteractionManager::GetInstance()->UpdateDragStyle(DragCursorStyle::MOVE);
    ASSERT_EQ(ret, RET_OK);
    std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_UPDATE_DRAG_STYLE));
    ret = InteractionManager::GetInstance()->UpdateDragStyle(DragCursorStyle::FORBIDDEN);
    ASSERT_EQ(ret, RET_OK);
    std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_UPDATE_DRAG_STYLE));
    DragDropResult dropResult { DragResult::DRAG_SUCCESS, HAS_CUSTOM_ANIMATION, WINDOW_ID };
    ret = InteractionManager::GetInstance()->StopDrag(dropResult);
    ASSERT_EQ(ret, RET_OK);
    ASSERT_TRUE(futureFlag.wait_for(std::chrono::milliseconds(PROMISE_WAIT_SPAN_MS)) != std::future_status::timeout);
}

/**
 * @tc.name: InteractionDragDrawingTest_Touchscreen_DragNum_Multiple
 * @tc.desc: Drag Drawing
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InteractionDragDrawingTest, InteractionDragDrawingTest_Touchscreen_DragNum_Multiple, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::optional<DragData> dragData = CreateDragData(
        MMI::PointerEvent::SOURCE_TYPE_TOUCHSCREEN, POINTER_ID, DRAG_NUM_MULTIPLE);
    ASSERT_TRUE(dragData);
    std::promise<bool> promiseFlag;
    std::future<bool> futureFlag = promiseFlag.get_future();
    auto callback = [&promiseFlag](const DragNotifyMsg& notifyMessage) {
        FI_HILOGD("displayX:%{public}d, displayY:%{public}d, result:%{public}d",
            notifyMessage.displayX, notifyMessage.displayY, notifyMessage.result);
        promiseFlag.set_value(true);
    };
    int32_t ret = InteractionManager::GetInstance()->StartDrag(dragData.value(), callback);
    ASSERT_EQ(ret, RET_OK);
    ret = InteractionManager::GetInstance()->SetDragWindowVisible(DRAG_WINDOW_VISIBLE);
    ASSERT_EQ(ret, RET_OK);
    ret = InteractionManager::GetInstance()->UpdateDragStyle(DragCursorStyle::COPY);
    ASSERT_EQ(ret, RET_OK);
    std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_UPDATE_DRAG_STYLE));
    ret = InteractionManager::GetInstance()->UpdateDragStyle(DragCursorStyle::DEFAULT);
    ASSERT_EQ(ret, RET_OK);
    std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_UPDATE_DRAG_STYLE));
    ret = InteractionManager::GetInstance()->UpdateDragStyle(DragCursorStyle::FORBIDDEN);
    ASSERT_EQ(ret, RET_OK);
    std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_UPDATE_DRAG_STYLE));
    ret = InteractionManager::GetInstance()->UpdateDragStyle(DragCursorStyle::MOVE);
    ASSERT_EQ(ret, RET_OK);
    std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_UPDATE_DRAG_STYLE));
    DragDropResult dropResult { DragResult::DRAG_SUCCESS, HAS_CUSTOM_ANIMATION, WINDOW_ID };
    ret = InteractionManager::GetInstance()->StopDrag(dropResult);
    ASSERT_EQ(ret, RET_OK);
    ASSERT_TRUE(futureFlag.wait_for(std::chrono::milliseconds(PROMISE_WAIT_SPAN_MS)) != std::future_status::timeout);
}

/**
 * @tc.name: InteractionDragDrawingTest_UpdateShadowPic
 * @tc.desc: Drag Drawing
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InteractionDragDrawingTest, InteractionDragDrawingTest_UpdateShadowPic, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::promise<bool> promiseFlag;
    std::future<bool> futureFlag = promiseFlag.get_future();
    auto callback = [&promiseFlag](const DragNotifyMsg& notifyMessage) {
        FI_HILOGD("displayX:%{public}d, displayY:%{public}d, result:%{public}d, target:%{public}d",
            notifyMessage.displayX, notifyMessage.displayY, notifyMessage.result, notifyMessage.targetPid);
        promiseFlag.set_value(true);
    };
    std::optional<DragData> dragData = CreateDragData(
        MMI::PointerEvent::SOURCE_TYPE_MOUSE, POINTER_ID, DRAG_NUM_ONE);
    ASSERT_TRUE(dragData);
    int32_t ret = InteractionManager::GetInstance()->StartDrag(dragData.value(), callback);
    ASSERT_EQ(ret, RET_OK);
    std::shared_ptr<Media::PixelMap> pixelMap = CreatePixelMap(MIDDLE_PIXEL_MAP_WIDTH, MIDDLE_PIXEL_MAP_HEIGHT);
    ASSERT_NE(pixelMap, nullptr);
    ShadowInfo shadowInfo = { pixelMap, 0, 0 };
    ret = InteractionManager::GetInstance()->UpdateShadowPic(shadowInfo);
    ASSERT_EQ(ret, RET_OK);
    ret = InteractionManager::GetInstance()->SetDragWindowVisible(DRAG_WINDOW_VISIBLE);
    ASSERT_EQ(ret, RET_OK);
    ret = InteractionManager::GetInstance()->UpdateDragStyle(DragCursorStyle::COPY);
    ASSERT_EQ(ret, RET_OK);
    DragDropResult dropResult { DragResult::DRAG_FAIL, HAS_CUSTOM_ANIMATION, WINDOW_ID };
    ret = InteractionManager::GetInstance()->StopDrag(dropResult);
    ASSERT_EQ(ret, RET_OK);
    ASSERT_TRUE(futureFlag.wait_for(std::chrono::milliseconds(PROMISE_WAIT_SPAN_MS)) != std::future_status::timeout);
    std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_ANIMATION_END));
}

/**
 * @tc.name: InteractionDragDrawingTest_Mouse_Animation
 * @tc.desc: Drag Drawing
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InteractionDragDrawingTest, InteractionDragDrawingTest_Mouse_Animation, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::promise<bool> promiseFlag;
    std::future<bool> futureFlag = promiseFlag.get_future();
    auto callback = [&promiseFlag](const DragNotifyMsg& notifyMessage) {
        FI_HILOGD("displayX:%{public}d, displayY:%{public}d, result:%{public}d, target:%{public}d",
            notifyMessage.displayX, notifyMessage.displayY, notifyMessage.result, notifyMessage.targetPid);
        promiseFlag.set_value(true);
    };
    std::optional<DragData> dragData = CreateDragData(
        MMI::PointerEvent::SOURCE_TYPE_MOUSE, POINTER_ID, DRAG_NUM_ONE);
    ASSERT_TRUE(dragData);
    int32_t ret = InteractionManager::GetInstance()->StartDrag(dragData.value(), callback);
    ASSERT_EQ(ret, RET_OK);
    ret = InteractionManager::GetInstance()->SetDragWindowVisible(DRAG_WINDOW_VISIBLE);
    ASSERT_EQ(ret, RET_OK);
    ret = InteractionManager::GetInstance()->UpdateDragStyle(DragCursorStyle::COPY);
    ASSERT_EQ(ret, RET_OK);
    DragDropResult dropResult { DragResult::DRAG_SUCCESS, HAS_CUSTOM_ANIMATION, WINDOW_ID };
    ret = InteractionManager::GetInstance()->StopDrag(dropResult);
    ASSERT_EQ(ret, RET_OK);
    ASSERT_TRUE(futureFlag.wait_for(std::chrono::milliseconds(PROMISE_WAIT_SPAN_MS)) != std::future_status::timeout);
    std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_ANIMATION_END));
}

/**
 * @tc.name: InteractionDragDrawingTest_Touchscreen_Animation
 * @tc.desc: Drag Drawing
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InteractionDragDrawingTest, InteractionDragDrawingTest_Touchscreen_Animation, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::promise<bool> promiseFlag;
    std::future<bool> futureFlag = promiseFlag.get_future();
    auto callback = [&promiseFlag](const DragNotifyMsg& notifyMessage) {
        FI_HILOGD("displayX:%{public}d, displayY:%{public}d, result:%{public}d, target:%{public}d",
            notifyMessage.displayX, notifyMessage.displayY, notifyMessage.result, notifyMessage.targetPid);
        promiseFlag.set_value(true);
    };
    std::optional<DragData> dragData = CreateDragData(
        MMI::PointerEvent::SOURCE_TYPE_TOUCHSCREEN, POINTER_ID, DRAG_NUM_ONE);
    ASSERT_TRUE(dragData);
    int32_t ret = InteractionManager::GetInstance()->StartDrag(dragData.value(), callback);
    ASSERT_EQ(ret, RET_OK);
    ret = InteractionManager::GetInstance()->SetDragWindowVisible(DRAG_WINDOW_VISIBLE);
    ASSERT_EQ(ret, RET_OK);
    ret = InteractionManager::GetInstance()->UpdateDragStyle(DragCursorStyle::COPY);
    ASSERT_EQ(ret, RET_OK);
    DragDropResult dropResult { DragResult::DRAG_FAIL, NOT_HAS_CUSTOM_ANIMATION, WINDOW_ID };
    ret = InteractionManager::GetInstance()->StopDrag(dropResult);
    ASSERT_EQ(ret, RET_OK);
    ASSERT_TRUE(futureFlag.wait_for(std::chrono::milliseconds(PROMISE_WAIT_SPAN_MS)) != std::future_status::timeout);
    std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_ANIMATION_END));
}

/**
 * @tc.name: InteractionDragDrawingTest_UpdateDragItemStyle
 * @tc.desc: Drag Drawing
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(InteractionDragDrawingTest, InteractionDragDrawingTest_UpdateDragItemStyle, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    std::promise<bool> promiseFlag;
    std::future<bool> futureFlag = promiseFlag.get_future();
    auto callback = [&promiseFlag](const DragNotifyMsg& notifyMessage) {
        FI_HILOGD("displayX:%{public}d, displayY:%{public}d, result:%{public}d, target:%{public}d",
            notifyMessage.displayX, notifyMessage.displayY, notifyMessage.result, notifyMessage.targetPid);
        promiseFlag.set_value(true);
    };
    std::optional<DragData> dragData = CreateDragData(
        MMI::PointerEvent::SOURCE_TYPE_TOUCHSCREEN, POINTER_ID, DRAG_NUM_ONE);
    ASSERT_TRUE(dragData);
    int32_t ret = InteractionManager::GetInstance()->StartDrag(dragData.value(), callback);
    ASSERT_EQ(ret, RET_OK);
    DragItemStyle dragItemStyle { FOREGROUND_COLOR, RADIUS, ALPHA };
    ret = InteractionManager::GetInstance()->UpdateDragItemStyle(dragItemStyle);
    ASSERT_EQ(ret, RET_OK);
    ret = InteractionManager::GetInstance()->SetDragWindowVisible(DRAG_WINDOW_VISIBLE);
    std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_ANIMATION_END));
    ASSERT_EQ(ret, RET_OK);
    DragDropResult dropResult { DragResult::DRAG_FAIL, NOT_HAS_CUSTOM_ANIMATION, WINDOW_ID };
    ret = InteractionManager::GetInstance()->StopDrag(dropResult);
    ASSERT_EQ(ret, RET_OK);
    ASSERT_TRUE(futureFlag.wait_for(std::chrono::milliseconds(PROMISE_WAIT_SPAN_MS)) != std::future_status::timeout);
}

} // namespace DeviceStatus
} // namespace Msdp
} // namespace OHOS
