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

#ifndef DRAG_DRAWING_MANAGER_H
#define DRAG_DRAWING_MANAGER_H

#include <vector>

#include "event_handler.h"
#include "event_runner.h"
#include "libxml/tree.h"
#include "libxml/parser.h"
#include "modifier/rs_extended_modifier.h"
#include "modifier/rs_modifier.h"
#include "pixel_map.h"
#include "ui/rs_canvas_node.h"
#include "vsync_receiver.h"

#include "drag_data.h"

namespace OHOS {
namespace Msdp {
namespace DeviceStatus {
class DrawSVGModifier : public OHOS::Rosen::RSContentStyleModifier {
public:
    DrawSVGModifier() = default;
    ~DrawSVGModifier() = default;
    void Draw(OHOS::Rosen::RSDrawingContext& context) const override;

private:
    int32_t UpdateSvgNodeInfo(const xmlNodePtr curNode, int32_t extendSvgWidth) const;
    xmlNodePtr GetRectNode(xmlNodePtr curNode) const;
    xmlNodePtr UpdateRectNode(xmlNodePtr curNode, int32_t extendSvgWidth) const;
    void UpdateTspanNode(xmlNodePtr curNode) const;
    int32_t ParseAndAdjustSvgInfo(xmlNodePtr curNode) const;
    std::shared_ptr<OHOS::Media::PixelMap> DecodeSvgToPixelMap(const std::string &filePath) const;
};

class DrawPixelMapModifier : public OHOS::Rosen::RSContentStyleModifier {
public:
    DrawPixelMapModifier() = default;
    ~DrawPixelMapModifier() = default;
    void Draw(OHOS::Rosen::RSDrawingContext &context) const override;
};

class DrawMouseIconModifier : public OHOS::Rosen::RSContentStyleModifier {
public:
    DrawMouseIconModifier() = default;
    ~DrawMouseIconModifier() = default;
    void Draw(OHOS::Rosen::RSDrawingContext &context) const override;

private:
    int32_t GetIconSize() const;
};

class DragDrawing final {
public:
    DragDrawing() = default;
    ~DragDrawing() = default;
    DISALLOW_COPY_AND_MOVE(DragDrawing);

    int32_t Init(const DragData &dragData);
    void Draw(int32_t displayId, int32_t displayX, int32_t displayY);

private:
    int32_t InitLayer();
    void InitCanvas(int32_t width, int32_t height);
    void CreateWindow(int32_t displayX, int32_t displayY);
    int32_t DrawShadow();
    int32_t DrawMouseIcon();
    int32_t DrawStyle();

private:
    std::shared_ptr<OHOS::Rosen::RSCanvasNode> canvasNode_ { nullptr };
    std::weak_ptr<DrawSVGModifier> drawSVGModifier_;
    std::weak_ptr<DrawPixelMapModifier> drawPixelMapModifier_;
    std::weak_ptr<DrawMouseIconModifier> drawMouseIconModifier_;
    std::shared_ptr<OHOS::Rosen::RSUIDirector> rsUiDirector_ { nullptr };
};
} // namespace DeviceStatus
} // namespace Msdp
} // namespace OHOS
#endif // DRAG_DRAWING_MANAGER_H