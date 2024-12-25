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

#ifndef I_DRAG_ANIMATION_H
#define I_DRAG_ANIMATION_H

#include "pixel_map.h"
#include "transaction/rs_transaction.h"
#include "ui/rs_canvas_node.h"
#include "ui/rs_root_node.h"

#include "drag_data.h"

namespace OHOS {
namespace Msdp {
namespace DeviceStatus {
class IDragAnimation {
public:
    IDragAnimation() = default;
    virtual ~IDragAnimation() = default;
    virtual void OnStartDrag(const DragAnimationData &dragAnimationData) = 0;
    virtual void OnDragStyle(std::shared_ptr<Rosen::RSCanvasNode> styleNode,
        std::shared_ptr<Media::PixelMap> stylePixelMap) = 0;
    virtual void OnStopDragSuccess(std::shared_ptr<Rosen::RSCanvasNode> shadowNode,
        std::shared_ptr<Rosen::RSCanvasNode> styleNode) = 0;
    virtual void OnStopDragFail(std::shared_ptr<Rosen::RSSurfaceNode> surfaceNode,
        std::shared_ptr<Rosen::RSNode> rootNode) = 0;
    virtual void OnStopAnimation() = 0;
};
} // namespace DeviceStatus
} // namespace Msdp
} // namespace OHOS
#endif // I_DRAG_ANIMATION_H
