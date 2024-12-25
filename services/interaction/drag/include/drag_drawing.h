/*
 * Copyright (c) 2023-2024 Huawei Device Co., Ltd.
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

#ifndef DRAG_DRAWING_H
#define DRAG_DRAWING_H

#include <vector>
#include <shared_mutex>

#include "display_manager.h"
#include "event_handler.h"
#include "event_runner.h"
#include "json_parser.h"
#include "libxml/tree.h"
#include "libxml/parser.h"
#include "modifier/rs_extended_modifier.h"
#include "modifier/rs_modifier.h"
#include "vsync_receiver.h"

#include "drag_data.h"
#include "drag_smooth_processor.h"
#include "drag_vsync_station.h"
#ifndef OHOS_BUILD_ENABLE_ARKUI_X
#include "i_context.h"
#endif // OHOS_BUILD_ENABLE_ARKUI_X
#include "i_drag_animation.h"
#ifdef OHOS_BUILD_ENABLE_ARKUI_X
#include "pointer_style.h"
#include "virtual_rs_window.h"
#endif // OHOS_BUILD_ENABLE_ARKUI_X

namespace OHOS {
namespace Msdp {
namespace DeviceStatus {
struct DrawingInfo;
class DragDrawing;
using DragStartExtFunc = void (*)(DragData &dragData);
using DragNotifyExtFunc = void (*)(DragEventInfo &dragEventInfo);
class DrawSVGModifier : public Rosen::RSContentStyleModifier {
public:
    explicit DrawSVGModifier(std::shared_ptr<Media::PixelMap> stylePixelMap) : stylePixelMap_(stylePixelMap) {}
    ~DrawSVGModifier() = default;
    void Draw(Rosen::RSDrawingContext& context) const override;

private:
    std::shared_ptr<Media::PixelMap> stylePixelMap_ { nullptr };
};

class DrawPixelMapModifier : public Rosen::RSContentStyleModifier {
public:
    DrawPixelMapModifier() = default;
    ~DrawPixelMapModifier() = default;
    void SetDragShadow(std::shared_ptr<Rosen::RSCanvasNode> pixelMapNode) const;
    void SetTextDragShadow(std::shared_ptr<Rosen::RSCanvasNode> pixelMapNode) const;
    Rosen::SHADOW_COLOR_STRATEGY ConvertShadowColorStrategy(int32_t shadowColorStrategy) const;
    void Draw(Rosen::RSDrawingContext &context) const override;
};

class DrawMouseIconModifier : public Rosen::RSContentStyleModifier {
public:
    explicit DrawMouseIconModifier(MMI::PointerStyle pointerStyle) : pointerStyle_(pointerStyle) {}
    ~DrawMouseIconModifier() = default;
    void Draw(Rosen::RSDrawingContext &context) const override;

private:
    void OnDraw(std::shared_ptr<Media::PixelMap> pixelMap) const;
    std::shared_ptr<Media::PixelMap> DrawFromSVG() const;

private:
    MMI::PointerStyle pointerStyle_;
};

class DrawDynamicEffectModifier : public Rosen::RSContentStyleModifier {
public:
    DrawDynamicEffectModifier() = default;
    ~DrawDynamicEffectModifier() = default;
    void Draw(Rosen::RSDrawingContext &context) const override;
    void SetAlpha(float alpha);
    void SetScale(float scale);

private:
    std::shared_ptr<Rosen::RSAnimatableProperty<float>> alpha_ { nullptr };
    std::shared_ptr<Rosen::RSAnimatableProperty<float>> scale_ { nullptr };
};

class DrawDragStopModifier : public Rosen::RSContentStyleModifier {
public:
    DrawDragStopModifier() = default;
    ~DrawDragStopModifier() = default;
    void Draw(Rosen::RSDrawingContext &context) const override;
    void SetAlpha(float alpha);
    void SetScale(float scale);
    void SetStyleScale(float scale);
    void SetStyleAlpha(float alpha);

private:
    std::shared_ptr<Rosen::RSAnimatableProperty<float>> alpha_ { nullptr };
    std::shared_ptr<Rosen::RSAnimatableProperty<float>> scale_ { nullptr };
    std::shared_ptr<Rosen::RSAnimatableProperty<float>> styleScale_ { nullptr };
    std::shared_ptr<Rosen::RSAnimatableProperty<float>> styleAlpha_ { nullptr };
};

class DrawStyleChangeModifier : public Rosen::RSContentStyleModifier {
public:
    DrawStyleChangeModifier() = default;
    explicit DrawStyleChangeModifier(std::shared_ptr<Media::PixelMap> stylePixelMap) : stylePixelMap_(stylePixelMap) {}
    ~DrawStyleChangeModifier() = default;
    void Draw(Rosen::RSDrawingContext &context) const override;
    void SetScale(float scale);

private:
    std::shared_ptr<Media::PixelMap> stylePixelMap_ { nullptr };
    std::shared_ptr<Rosen::RSAnimatableProperty<float>> scale_ { nullptr };
};

class DrawStyleScaleModifier : public Rosen::RSContentStyleModifier {
public:
    DrawStyleScaleModifier() = default;
    ~DrawStyleScaleModifier() = default;
    void Draw(Rosen::RSDrawingContext &context) const override;
    void SetScale(float scale);

private:
    std::shared_ptr<Rosen::RSAnimatableProperty<float>> scale_ { nullptr };
};

struct FilterInfo {
    std::string dragType;
    bool shadowEnable { false };
    bool shadowIsFilled { false };
    bool shadowMask { false };
    int32_t shadowColorStrategy { 0 };
    float shadowCorner { 0.0F };
    float dipScale { 0.0f };
    float scale { 1.0f };
    float cornerRadius1 { 0.0f };
    float cornerRadius2 { 0.0f };
    float cornerRadius3 { 0.0f };
    float cornerRadius4 { 0.0f };
    float opacity { 0.95f };
    float offsetX { 0.0f };
    float offsetY { 0.0f };
    uint32_t argb { 0 };
    std::string path;
    float elevation { 0.0f };
    bool isHardwareAcceleration { false };
    Rosen::Vector2f coef;
    float blurRadius { -1.0f };
    float blurStaturation { -1.0f };
    float blurBrightness { -1.0f };
    uint32_t blurColor { 0 };
    int32_t blurStyle { -1 };
    float dragNodeGrayscale { 0.0f };
};

struct ExtraInfo {
    std::string componentType;
    int32_t blurStyle { -1 };
    float cornerRadius { 0.0f };
    bool allowDistributed { true };
    Rosen::Vector2f coef;
};

enum class ScreenSizeType {
    // Undefined screen width
    UNDEFINED = 0,
    // Screen width size is XS, grids is 2
    XS,
    // Screen width size is SM, grids is 4
    SM,
    // Screen width size is MD, grids is 8
    MD,
    // Screen width size is LG, grids is 12
    LG,
    // Screen width size is XL, maxi number of grids
    XL,
};

struct DrawingInfo {
    std::atomic_bool isRunning { false };
    std::atomic_bool isPreviousDefaultStyle { false };
    std::atomic_bool isCurrentDefaultStyle { false };
    bool isInitUiDirector { true };
    bool isExistScalingValue { false };
    std::atomic_bool needDestroyDragWindow { false };
    int32_t sourceType { -1 };
    int32_t currentDragNum { -1 };
    DragCursorStyle currentStyle { DragCursorStyle::DEFAULT };
    int32_t displayId { -1 };
    int32_t pixelMapX { -1 };
    int32_t pixelMapY { -1 };
    int32_t displayX { -1 };
    int32_t displayY { -1 };
    float x { -1.0f };
    float y { -1.0f };
    float currentPositionX { -1.0f };
    float currentPositionY { -1.0f };
    int32_t mouseWidth { 0 };
    int32_t mouseHeight { 0 };
    int32_t rootNodeWidth { -1 };
    int32_t rootNodeHeight { -1 };
    std::atomic<int64_t> startNum { -1 };
    int32_t timerId { -1 };
    float scalingValue { 0.0 };
    std::vector<std::shared_ptr<Rosen::RSCanvasNode>> nodes;
    std::vector<std::shared_ptr<Rosen::RSCanvasNode>> multiSelectedNodes;
    std::vector<std::shared_ptr<Media::PixelMap>> multiSelectedPixelMaps;
    std::shared_ptr<Rosen::RSNode> rootNode { nullptr };
    std::shared_ptr<Rosen::RSNode> parentNode { nullptr };
    std::shared_ptr<Rosen::RSSurfaceNode> surfaceNode { nullptr };
    std::shared_ptr<Media::PixelMap> pixelMap { nullptr };
    std::shared_ptr<Media::PixelMap> stylePixelMap { nullptr };
#ifndef OHOS_BUILD_ENABLE_ARKUI_X
    IContext* context { nullptr };
#endif // OHOS_BUILD_ENABLE_ARKUI_X
    ExtraInfo extraInfo;
    FilterInfo filterInfo;
};

struct DragWindowRotationInfo {
    float rotation { 0.0f };
    float pivotX { 0.0f };
    float pivotY { 0.0f };
};

class DragDrawing : public IDragAnimation {
public:
    DragDrawing() = default;
    DISALLOW_COPY_AND_MOVE(DragDrawing);
    ~DragDrawing();

#ifndef OHOS_BUILD_ENABLE_ARKUI_X
    int32_t Init(const DragData &dragData, IContext* context, bool isLongPressDrag = false);
#else
    int32_t Init(const DragData &dragData, bool isLongPressDrag = false);
#endif // OHOS_BUILD_ENABLE_ARKUI_X
    void NotifyDragInfo(const std::string &sourceName, const std::string &targetName);
    void Draw(int32_t displayId, int32_t displayX, int32_t displayY, bool isNeedAdjustDisplayXY = true,
        bool isMultiSelectedAnimation = true);
    int32_t UpdateDragStyle(DragCursorStyle style);
    int32_t UpdateShadowPic(const ShadowInfo &shadowInfo);
    int32_t UpdatePreviewStyle(const PreviewStyle &previewStyle);
    int32_t UpdatePreviewStyleWithAnimation(const PreviewStyle &previewStyle, const PreviewAnimation &animation);
    int32_t StartVsync();
#ifndef OHOS_BUILD_ENABLE_ARKUI_X
    void OnDragSuccess(IContext* context);
    void OnDragFail(IContext* context);
    void StopVSyncStation();
#else
    void OnDragSuccess();
    void OnDragFail();
    void SetDragWindow(std::shared_ptr<OHOS::Rosen::Window> window);
    void AddDragDestroy(std::function<void()> cb);
    void SetSVGFilePath(const std::string &filePath);
#endif // OHOS_BUILD_ENABLE_ARKUI_X
    void OnDragMove(int32_t displayId, int32_t displayX, int32_t displayY, int64_t actionTime);
    void EraseMouseIcon();
    void DestroyDragWindow();
    void UpdateDrawingState();
    void UpdateDragWindowState(bool visible, bool isZoomInAndAlphaChanged = false);
    void OnStartDrag(const DragAnimationData &dragAnimationData) override;
    void OnDragStyle(std::shared_ptr<Rosen::RSCanvasNode> dragStyleNode,
        std::shared_ptr<Media::PixelMap> stylePixelMap) override;
    void OnStopDragSuccess(std::shared_ptr<Rosen::RSCanvasNode> shadowNode,
        std::shared_ptr<Rosen::RSCanvasNode> dragStyleNode) override;
    void OnStopDragFail(std::shared_ptr<Rosen::RSSurfaceNode> surfaceNode,
        std::shared_ptr<Rosen::RSNode> rootNode) override;
    void OnStopAnimation() override;
    int32_t EnterTextEditorArea(bool enable);
    bool GetAllowDragState();
    void SetScreenId(uint64_t screenId);
    int32_t RotateDragWindowAsync(Rosen::Rotation rotation);
    int32_t RotateDragWindowSync(const std::shared_ptr<Rosen::RSTransaction>& rsTransaction = nullptr);
    void SetRotation(Rosen::Rotation rotation);
    float CalculateWidthScale();
    float GetMaxWidthScale(int32_t width);
    int32_t AddSelectedPixelMap(std::shared_ptr<OHOS::Media::PixelMap> pixelMap);
    void UpdateDragWindowDisplay(int32_t displayId);
    void DetachToDisplay(int32_t displayId);
    void ScreenRotate(Rosen::Rotation rotation, Rosen::Rotation lastRotation);
    void UpdateDragState(DragState dragState);
    static std::shared_ptr<Media::PixelMap> AccessGlobalPixelMapLocked();
    static void UpdataGlobalPixelMapLocked(std::shared_ptr<Media::PixelMap> pixelmap);

private:
    int32_t CheckDragData(const DragData &dragData);
    int32_t InitLayer();
    void InitCanvas(int32_t width, int32_t height);
    void CreateWindow();
    int32_t DrawShadow(std::shared_ptr<Rosen::RSCanvasNode> shadowNode);
    int32_t DrawMouseIcon();
    int32_t DrawStyle(std::shared_ptr<Rosen::RSCanvasNode> dragStyleNode,
        std::shared_ptr<Media::PixelMap> stylePixelMap);
    int32_t RunAnimation(std::function<int32_t()> cb);
    int32_t InitVSync(float endAlpha, float endScale);
    void OnVsync();
    void InitDrawingInfo(const DragData &dragData, bool isLongPressDrag = false);
    int32_t InitDragAnimationData(DragAnimationData &dragAnimationData);
    void RemoveModifier();
    int32_t UpdateSvgNodeInfo(xmlNodePtr curNode, int32_t extendSvgWidth);
    xmlNodePtr GetRectNode(xmlNodePtr curNode);
    xmlNodePtr UpdateRectNode(int32_t extendSvgWidth, xmlNodePtr curNode);
    void UpdateTspanNode(xmlNodePtr curNode);
    int32_t ParseAndAdjustSvgInfo(xmlNodePtr curNode);
    std::shared_ptr<Media::PixelMap> DecodeSvgToPixelMap(const std::string &filePath);
    int32_t GetFilePath(std::string &filePath);
    bool NeedAdjustSvgInfo();
    void SetDecodeOptions(Media::DecodeOptions &decodeOpts);
    bool ParserFilterInfo(const std::string &filterInfoStr, FilterInfo &filterInfo);
    void ParserCornerRadiusInfo(const cJSON *cornerRadiusInfoStr, FilterInfo &filterInfo);
    void ParserBlurInfo(const cJSON *BlurInfoInfoStr, FilterInfo &filterInfo);
    void SetCustomDragBlur(const FilterInfo &filterInfo, std::shared_ptr<Rosen::RSCanvasNode> filterNode);
    void SetComponentDragBlur(const FilterInfo &filterInfo, const ExtraInfo &extraInfo,
        std::shared_ptr<Rosen::RSCanvasNode> filterNode);
    void ParserDragShadowInfo(cJSON* filterInfoParser, FilterInfo &filterInfo);
    void ParserTextDragShadowInfo(cJSON* filterInfoParser, FilterInfo &filterInfo);
    void PrintDragShadowInfo();
    void ProcessFilter();
    bool ParserExtraInfo(const std::string &extraInfoStr, ExtraInfo &extraInfo);
    static float RadiusVp2Sigma(float radiusVp, float dipScale);
    void DoDrawMouse(int32_t mousePositionX, int32_t mousePositionY);
    void UpdateMousePosition(float mousePositionX, float mousePositionY);
    int32_t UpdateDefaultDragStyle(DragCursorStyle style);
    int32_t UpdateValidDragStyle(DragCursorStyle style);
    int32_t SetNodesLocation();
    int32_t CreateEventRunner(int32_t positionX, int32_t positionY);
    int32_t ModifyPreviewStyle(std::shared_ptr<Rosen::RSCanvasNode> node, const PreviewStyle &previewStyle);
    int32_t ModifyMultiPreviewStyle(const std::vector<PreviewStyle> &previewStyles);
    void MultiSelectedAnimation(int32_t positionX, int32_t positionY, int32_t adjustSize,
        bool isMultiSelectedAnimation);
    void DoMultiSelectedAnimation(float positionX, float positionY, float adjustSize,
        bool isMultiSelectedAnimation = true);
    void InitMultiSelectedNodes();
    void ClearMultiSelectedData();
    bool ParserRadius(float &radius);
    void OnStopAnimationSuccess();
    void OnStopAnimationFail();
    void OnDragStyleAnimation();
    void ChangeStyleAnimation();
    void CheckStyleNodeModifier(std::shared_ptr<Rosen::RSCanvasNode> styleNode);
    void RemoveStyleNodeModifier(std::shared_ptr<Rosen::RSCanvasNode> styleNode);
    void StartStyleAnimation(float startScale, float endScale, int32_t duration);
    void UpdateAnimationProtocol(Rosen::RSAnimationTimingProtocol protocol);
    void RotateDisplayXY(int32_t &displayX, int32_t &displayY);
    void RotatePixelMapXY();
    void ResetAnimationParameter();
    void ResetAnimationFlag(bool isForce = false);
    void DoEndAnimation();
    void ResetParameter();
    int32_t DoRotateDragWindow(float rotation,
        const std::shared_ptr<Rosen::RSTransaction>& rsTransaction, bool isAnimated);
    int32_t DoRotateDragWindowAnimation(float rotation, float pivotX, float pivotY,
        const std::shared_ptr<Rosen::RSTransaction>& rsTransaction);
    int32_t RotateDragWindow(Rosen::Rotation rotation,
        const std::shared_ptr<Rosen::RSTransaction>& rsTransaction = nullptr, bool isAnimated = false);
    void ZoomOutAndAlphaChangedAnimation();
    void AlphaChangedAnimation();
#ifndef OHOS_BUILD_ENABLE_ARKUI_X
    std::shared_ptr<AppExecFwk::EventHandler> GetSuperHubHandler();
#endif // OHOS_BUILD_ENABLE_ARKUI_X
    void RotateCanvasNode(float pivotX, float pivotY, float rotation);
    void FlushDragPosition(uint64_t nanoTimestamp);
    void RotatePosition(float &displayX, float &displayY);
    void UpdateDragPosition(int32_t displayId, float displayX, float displayY);
    float AdjustDoubleValue(double doubleValue);
    int32_t UpdatePixelMapsAngleAndAlpha();
    int32_t UpdatePixeMapDrawingOrder();
    void LoadDragDropLib();
    template <typename T>
    void AdjustRotateDisplayXY(T &displayX, T &displayY);
    void DrawRotateDisplayXY(float positionX, float positionY);
    void ScreenRotateAdjustDisplayXY(
        Rosen::Rotation rotation, Rosen::Rotation lastRotation, float &displayX, float &displayY);
    void UpdateDragDataForSuperHub(const DragData &dragData);
    std::shared_ptr<Rosen::VSyncReceiver> AccessReceiverLocked();
    void UpdateReceiverLocked(std::shared_ptr<Rosen::VSyncReceiver> receiver);

private:
    int64_t interruptNum_ { -1 };
    std::shared_ptr<Rosen::RSCanvasNode> canvasNode_ { nullptr };
    std::shared_ptr<DrawSVGModifier> drawSVGModifier_ { nullptr };
    std::shared_ptr<DrawPixelMapModifier> drawPixelMapModifier_ { nullptr };
    std::shared_ptr<DrawMouseIconModifier> drawMouseIconModifier_ { nullptr };
    std::shared_ptr<DrawDynamicEffectModifier> drawDynamicEffectModifier_ { nullptr };
    std::shared_ptr<DrawDragStopModifier> drawDragStopModifier_ { nullptr };
    std::shared_ptr<DrawStyleChangeModifier> drawStyleChangeModifier_ { nullptr };
    std::shared_ptr<DrawStyleScaleModifier> drawStyleScaleModifier_ { nullptr };
    std::shared_ptr<Rosen::RSUIDirector> rsUiDirector_ { nullptr };
    std::shared_ptr<Rosen::VSyncReceiver> receiver_ { nullptr };
    std::shared_ptr<AppExecFwk::EventHandler> handler_ { nullptr };
    std::shared_ptr<AppExecFwk::EventHandler> superHubHandler_ { nullptr };
    std::atomic_bool hasRunningStopAnimation_ { false };
    std::atomic_bool hasRunningScaleAnimation_ { false };
    std::atomic_bool needBreakStyleScaleAnimation_ { false };
    std::atomic_bool hasRunningAnimation_ { false };
    void* dragExtHandler_ { nullptr };
    bool needRotatePixelMapXY_ { false };
    uint64_t screenId_ { 0 };
    Rosen::Rotation rotation_ { Rosen::Rotation::ROTATION_0 };
    ScreenSizeType currentScreenSize_ = ScreenSizeType::UNDEFINED;
    MMI::PointerStyle pointerStyle_;
    DragVSyncStation vSyncStation_;
    DragSmoothProcessor dragSmoothProcessor_;
    std::shared_ptr<DragFrameCallback> frameCallback_ { nullptr };
    std::atomic_bool isRunningRotateAnimation_ { false };
    DragWindowRotationInfo DragWindowRotateInfo_;
    DragState dragState_ { DragState::STOP };
    int32_t timerId_ { -1 };
    std::shared_mutex receiverMutex_;
#ifdef OHOS_BUILD_ENABLE_ARKUI_X
    std::shared_ptr<OHOS::Rosen::Window> window_ { nullptr };
    std::function<void()> callback_ { nullptr };
    std::string svgFilePath_;
    int64_t actionTime_ { 0 };
#else
    IContext* context_ { nullptr} ;
#endif // OHOS_BUILD_ENABLE_ARKUI_X
};
} // namespace DeviceStatus
} // namespace Msdp
} // namespace OHOS
#endif // DRAG_DRAWING_H