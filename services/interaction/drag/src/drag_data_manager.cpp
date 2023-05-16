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

#include "drag_data_manager.h"

#include "hitrace_meter.h"
#include "pointer_style.h"

#include "devicestatus_define.h"
#include "drag_data.h"
#include "fi_log.h"

namespace OHOS {
namespace Msdp {
namespace DeviceStatus {
namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, MSDP_DOMAIN_ID, "DragDataManager" };
} // namespace

DragDataManager::DragDataManager() = default;
DragDataManager::~DragDataManager() = default;

void DragDataManager::Init(const DragData &dragData, const MMI::PointerStyle &pointerStyle)
{
    CALL_DEBUG_ENTER;
    dragData_ = dragData;
    pointerStyle_ = pointerStyle;
}

void DragDataManager::SetDragStyle(DragCursorStyle style)
{
    dragStyle_ = style;
}

DragCursorStyle DragDataManager::GetDragStyle() const
{
    return dragStyle_;
}

std::u16string DragDataManager::GetDragMessage() const
{
    return dragMessage_;
}

DragData DragDataManager::GetDragData() const
{
    return dragData_;
}

void DragDataManager::SetDragWindowVisible(bool visible)
{
    visible_ = visible;
}

bool DragDataManager::GetDragWindowVisible() const
{
    return visible_;
}

int32_t DragDataManager::GetShadowOffset(int32_t& offsetX, int32_t& offsetY, int32_t& width, int32_t& height) const
{
    offsetX = dragData_.shadowInfo.x;
    offsetY = dragData_.shadowInfo.y;
    auto pixelMap = dragData_.shadowInfo.pixelMap;
    CHKPR(pixelMap, RET_ERR);
    width = pixelMap->GetWidth();
    height = pixelMap->GetHeight();
    FI_HILOGD("offsetX:%{public}d, offsetY:%{public}d, width:%{public}d, height:%{public}d",
        offsetX, offsetY, width, height);
    return RET_OK;
}

void DragDataManager::ResetDragData()
{
    CALL_DEBUG_ENTER;
    ShadowInfo shadowInfo;
    std::vector<uint8_t> buffer;
    dragData_ = { shadowInfo, buffer, "", -1, -1, -1, -1, -1, -1, false };
}
} // namespace DeviceStatus
} // namespace Msdp
} // namespace OHOS