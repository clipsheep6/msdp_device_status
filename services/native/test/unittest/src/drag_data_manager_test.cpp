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

#include "drag_data_manager_test.h"

#include <ipc_skeleton.h>

#include "fi_log.h"

namespace OHOS {
namespace Msdp {
namespace DeviceStatus {
using namespace testing::ext;
namespace {
constexpr ::OHOS::HiviewDFX::HiLogLabel LABEL { LOG_CORE, MSDP_DOMAIN_ID, "DragDataManagerTest" };
}
void DragDataManagerTest::SetUpTestCase() {}

void DragDataManagerTest::TearDownTestCase() {}

void DragDataManagerTest::SetUp() {}

void DragDataManagerTest::TearDown() {}

namespace {
/**
 * @tc.name: DragDataManagerTest
 * @tc.desc: test devicestatus callback in proxy
 * @tc.type: FUNC
 */
HWTEST_F(DragDataManagerTest, DragDataManagerTest001, TestSize.Level0)
{
    CALL_TEST_DEBUG;
    DRAG_DATA_MGR.SetDragStyle(DragCursorStyle::DEFAULT);
    EXPECT_TRUE(DRAG_DATA_MGR.GetDragStyle() == DragCursorStyle::DEFAULT);

    DRAG_DATA_MGR.SetDragStyle(DragCursorStyle::FORBIDDEN);
    EXPECT_TRUE(DRAG_DATA_MGR.GetDragStyle() == DragCursorStyle::FORBIDDEN);

    DRAG_DATA_MGR.SetDragStyle(DragCursorStyle::COPY);
    EXPECT_TRUE(DRAG_DATA_MGR.GetDragStyle() == DragCursorStyle::COPY);

    DRAG_DATA_MGR.SetDragStyle(DragCursorStyle::MOVE);
    EXPECT_TRUE(DRAG_DATA_MGR.GetDragStyle() == DragCursorStyle::MOVE);
}

/**
 * @tc.name: DragDataManagerTest002
 * @tc.desc: test normal get devicestatus data in ipc
 * @tc.type: FUNC
 */
HWTEST_F(DragDataManagerTest, DragDataManagerTest002, TestSize.Level0)
{
    CALL_TEST_DEBUG;
    int32_t targetTid = static_cast<int32_t>(IPCSkeleton::GetCallingTokenID());
    DRAG_DATA_MGR.SetTargetTid(targetTid);
    EXPECT_TRUE(targetTid == DRAG_DATA_MGR.GetTargetTid());

    int32_t targetPid = IPCSkeleton::GetCallingPid();
    DRAG_DATA_MGR.SetTargetPid(targetPid);
    EXPECT_TRUE(targetPid == DRAG_DATA_MGR.GetTargetPid());
}

/**
 * @tc.name: DragDataManagerTest003
 * @tc.desc: test abnormal get devicestatus data in ipc
 * @tc.type: FUNC
 */
HWTEST_F(DragDataManagerTest, DragDataManagerTest003, TestSize.Level0)
{
    CALL_TEST_DEBUG;
    int32_t targetTid = static_cast<int32_t>(IPCSkeleton::GetCallingTokenID());
    DRAG_DATA_MGR.SetTargetTid(targetTid);
    EXPECT_FALSE(targetTid != DRAG_DATA_MGR.GetTargetTid());

    int32_t targetPid = IPCSkeleton::GetCallingPid();
    DRAG_DATA_MGR.SetTargetPid(targetPid);
    EXPECT_FALSE(targetPid != DRAG_DATA_MGR.GetTargetPid());
}
} // namespace
} // namespace DeviceStatus
} // namespace Msdp
} // namespace OH