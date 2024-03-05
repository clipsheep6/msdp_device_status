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

#ifndef DRAG_DATA_MANAGER_H
#define DRAG_DATA_MANAGER_H

#include <string>

#include "pixel_map.h"
#include "pointer_style.h"
#include "singleton.h"

#include "drag_data.h"

namespace OHOS {
namespace Msdp {
namespace DeviceStatus {
class DragDataManager final {
    DECLARE_SINGLETON(DragDataManager);

public:
    DISALLOW_MOVE(DragDataManager);

    void Init(const DragData &dragData);
    void SetDragStyle(DragCursorStyle style);
    void SetShadowInfos(const std::vector<ShadowInfo> &shadowInfos);
    DragCursorStyle GetDragStyle() const;
    void SetDragWindowVisible(bool visible);
    bool GetDragWindowVisible() const;
    int32_t GetShadowOffset(ShadowOffset &shadowOffset) const;
    void SettargetTid(int32_t tokenId);
    int32_t GettargetTid() const;
    void SetTargetPid(int32_t pid);
    int32_t GetTargetPid() const;
    bool IsMotionDrag() const;
    void SetPreviewStyle(const PreviewStyle &previewStyle);
    PreviewStyle GetPreviewStyle();
    void ResetDragData();
    DragData GetDragData() const;
    void SetMotionDrag(bool isMotionDrag);
    bool GetCoordinateCorrected();
    void SetPixelMapLocation(const std::pair<int32_t, int32_t> &location);
    void SetTextEditorAreaFlag(bool enable);
    bool GetTextEditorAreaFlag();
    void SetInitialPixelMapLocation(const std::pair<int32_t, int32_t> &location);
    std::pair<int32_t, int32_t> GetInitialPixelMapLocation();
private:
    bool isMotionDrag_ { false };
    bool visible_ { false };
    int32_t targetPid_ { -1 };
    PreviewStyle previewStyle_;
    int32_t targetTid_ { -1 };
    std::u16string dragMessage_;
    DragCursorStyle dragStyle_ { DragCursorStyle::DEFAULT };
    DragData dragData_;
    bool textEditorAreaFlag_ { false };
    std::pair<int32_t, int32_t> initialPixelMapLocation_;
};

#define DRAG_DATA_MGR OHOS::Singleton<DragDataManager>::GetInstance()

} // namespace DeviceStatus
} // namespace Msdp
} // namespace OHOS
#endif // DRAG_DATA_MANAGER_H