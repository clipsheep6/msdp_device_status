/*
 * Copyright (c) 2022-2023 Huawei Device Co., Ltd.
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

#include "interaction_manager.h"

#include "devicestatus_define.h"
#include "drag_data.h"
#ifndef OHOS_BUILD_ENABLE_ARKUI_X
#include "intention_manager.h"
#else
#include "drag_manager.h"
#endif // OHOS_BUILD_ENABLE_ARKUI_X

namespace OHOS {
namespace Msdp {
namespace DeviceStatus {

#ifndef OHOS_BUILD_ENABLE_ARKUI_X
using CooperateMsgInfoCallback = std::function<void(const std::string&, const CoordinationMsgInfo&)>;
#endif // OHOS_BUILD_ENABLE_ARKUI_X

InteractionManager *InteractionManager::instance_ = new (std::nothrow) InteractionManager();

InteractionManager *InteractionManager::GetInstance()
{
    return instance_;
}

#ifndef OHOS_BUILD_ENABLE_ARKUI_X
int32_t InteractionManager::RegisterCoordinationListener(std::shared_ptr<ICoordinationListener> listener,
    bool isCompatible)
{
    return INTER_MGR_IMPL.RegisterCoordinationListener(listener, isCompatible);
}

int32_t InteractionManager::UnregisterCoordinationListener(std::shared_ptr<ICoordinationListener> listener,
    bool isCompatible)
{
    return INTER_MGR_IMPL.UnregisterCoordinationListener(listener, isCompatible);
}

int32_t InteractionManager::PrepareCoordination(CooperateMsgInfoCallback callback, bool isCompatible)
{
    return INTER_MGR_IMPL.PrepareCoordination(callback, isCompatible);
}

int32_t InteractionManager::UnprepareCoordination(CooperateMsgInfoCallback callback, bool isCompatible)
{
    return INTER_MGR_IMPL.UnprepareCoordination(callback, isCompatible);
}

int32_t InteractionManager::ActivateCoordination(const std::string &remoteNetworkId, int32_t startDeviceId,
    CooperateMsgInfoCallback callback, bool isCompatible)
{
    return INTER_MGR_IMPL.ActivateCoordination(remoteNetworkId, startDeviceId, callback, isCompatible);
}

int32_t InteractionManager::ActivateCooperateWithOptions(const std::string &remoteNetworkId, int32_t startDeviceId,
    CooperateMsgInfoCallback callback, CooperateOptions &cooperateOptions)
{
    FI_HILOGE("szy__InteractionManager__remoteNetworkId :%{public}s", remoteNetworkId.c_str());
    FI_HILOGE("szy__InteractionManager__startDeviceId :%{public}d", startDeviceId);
    FI_HILOGE("szy__InteractionManager__displayX :%{public}d", cooperateOptions.displayX);
    FI_HILOGE("szy__InteractionManager__displayY :%{public}d", cooperateOptions.displayY);
    FI_HILOGE("szy__InteractionManager__displayId :%{public}d", cooperateOptions.displayId);
    return INTER_MGR_IMPL.ActivateCooperateWithOptions(remoteNetworkId, startDeviceId, callback, cooperateOptions);
}

int32_t InteractionManager::DeactivateCoordination(bool isUnchained, CooperateMsgInfoCallback callback,
    bool isCompatible)
{
    return INTER_MGR_IMPL.DeactivateCoordination(isUnchained, callback, isCompatible);
}

int32_t InteractionManager::SetDraggableState(bool state)
{
    return INTER_MGR_IMPL.SetDraggableState(state);
}

int32_t InteractionManager::GetCoordinationState(
    const std::string &networkId, std::function<void(bool)> callback, bool isCompatible)
{
    return INTER_MGR_IMPL.GetCoordinationState(networkId, callback, isCompatible);
}

int32_t InteractionManager::GetCoordinationState(const std::string &udId, bool &state)
{
    return INTER_MGR_IMPL.GetCoordinationState(udId, state);
}

int32_t InteractionManager::RegisterEventListener(const std::string &networkId,
    std::shared_ptr<IEventListener> listener)
{
#ifdef OHOS_BUILD_ENABLE_INTENTION_FRAMEWORK
    return INTER_MGR_IMPL.RegisterEventListener(networkId, listener);
#else
    return RET_OK;
#endif // OHOS_BUILD_ENABLE_INTENTION_FRAMEWORK
}

int32_t InteractionManager::UnregisterEventListener(const std::string &networkId,
    std::shared_ptr<IEventListener> listener)
{
#ifdef OHOS_BUILD_ENABLE_INTENTION_FRAMEWORK
    return INTER_MGR_IMPL.UnregisterEventListener(networkId, listener);
#else
    return RET_OK;
#endif // OHOS_BUILD_ENABLE_INTENTION_FRAMEWORK
}

int32_t InteractionManager::SetDamplingCoefficient(uint32_t direction, double coefficient)
{
#ifdef OHOS_BUILD_ENABLE_INTENTION_FRAMEWORK
    return INTER_MGR_IMPL.SetDamplingCoefficient(direction, coefficient);
#else
    return RET_OK;
#endif // OHOS_BUILD_ENABLE_INTENTION_FRAMEWORK
}

int32_t InteractionManager::UpdateDragStyle(DragCursorStyle style, int32_t eventId)
{
    return INTER_MGR_IMPL.UpdateDragStyle(style, eventId);
}

int32_t InteractionManager::StartDrag(const DragData &dragData, std::shared_ptr<IStartDragListener> listener)
{
    return INTER_MGR_IMPL.StartDrag(dragData, listener);
}

int32_t InteractionManager::StopDrag(const DragDropResult &dropResult)
{
    return INTER_MGR_IMPL.StopDrag(dropResult);
}

int32_t InteractionManager::GetDragTargetPid()
{
    return INTER_MGR_IMPL.GetDragTargetPid();
}

int32_t InteractionManager::GetUdKey(std::string &udKey)
{
    return INTER_MGR_IMPL.GetUdKey(udKey);
}

int32_t InteractionManager::AddDraglistener(DragListenerPtr listener, bool isJsCaller)
{
    return INTER_MGR_IMPL.AddDraglistener(listener, isJsCaller);
}

int32_t InteractionManager::RemoveDraglistener(DragListenerPtr listener, bool isJsCaller)
{
    return INTER_MGR_IMPL.RemoveDraglistener(listener, isJsCaller);
}

int32_t InteractionManager::AddSubscriptListener(SubscriptListenerPtr listener)
{
    return INTER_MGR_IMPL.AddSubscriptListener(listener);
}

int32_t InteractionManager::RemoveSubscriptListener(SubscriptListenerPtr listener)
{
    return INTER_MGR_IMPL.RemoveSubscriptListener(listener);
}

int32_t InteractionManager::SetDragWindowVisible(bool visible, bool isForce)
{
    return INTER_MGR_IMPL.SetDragWindowVisible(visible, isForce);
}

int32_t InteractionManager::GetShadowOffset(int32_t &offsetX, int32_t &offsetY, int32_t &width, int32_t &height)
{
    ShadowOffset shadowOffset;
    int32_t ret = INTER_MGR_IMPL.GetShadowOffset(shadowOffset);
    offsetX = shadowOffset.offsetX;
    offsetY = shadowOffset.offsetY;
    width = shadowOffset.width;
    height = shadowOffset.height;
    return ret;
}

int32_t InteractionManager::UpdateShadowPic(const ShadowInfo &shadowInfo)
{
    return INTER_MGR_IMPL.UpdateShadowPic(shadowInfo);
}

int32_t InteractionManager::GetDragData(DragData &dragData)
{
    return INTER_MGR_IMPL.GetDragData(dragData);
}

int32_t InteractionManager::GetDragState(DragState &dragState)
{
    return INTER_MGR_IMPL.GetDragState(dragState);
}

int32_t InteractionManager::AddHotAreaListener(std::shared_ptr<IHotAreaListener> listener)
{
    return INTER_MGR_IMPL.AddHotAreaListener(listener);
}

int32_t InteractionManager::RemoveHotAreaListener(std::shared_ptr<IHotAreaListener> listener)
{
    return INTER_MGR_IMPL.RemoveHotAreaListener(listener);
}

int32_t InteractionManager::UpdatePreviewStyle(const PreviewStyle &previewStyle)
{
    return INTER_MGR_IMPL.UpdatePreviewStyle(previewStyle);
}

int32_t InteractionManager::UpdatePreviewStyleWithAnimation(const PreviewStyle &previewStyle,
    const PreviewAnimation &animation)
{
    return INTER_MGR_IMPL.UpdatePreviewStyleWithAnimation(previewStyle, animation);
}

int32_t InteractionManager::RotateDragWindowSync(const std::shared_ptr<Rosen::RSTransaction>& rsTransaction)
{
    return INTER_MGR_IMPL.RotateDragWindowSync(rsTransaction);
}

int32_t InteractionManager::SetDragWindowScreenId(uint64_t displayId, uint64_t screenId)
{
    return INTER_MGR_IMPL.SetDragWindowScreenId(displayId, screenId);
}

int32_t InteractionManager::GetDragSummary(std::map<std::string, int64_t> &summarys, bool isJsCaller)
{
    return INTER_MGR_IMPL.GetDragSummary(summarys, isJsCaller);
}

int32_t InteractionManager::SetDragSwitchState(bool enable, bool isJsCaller)
{
    return INTER_MGR_IMPL.SetDragSwitchState(enable);
}

int32_t InteractionManager::SetAppDragSwitchState(bool enable, const std::string &pkgName, bool isJsCaller)
{
    return INTER_MGR_IMPL.SetAppDragSwitchState(enable, pkgName);
}

int32_t InteractionManager::GetDragAction(DragAction &dragAction)
{
    return INTER_MGR_IMPL.GetDragAction(dragAction);
}

int32_t InteractionManager::EnterTextEditorArea(bool enable)
{
    return INTER_MGR_IMPL.EnterTextEditorArea(enable);
}

int32_t InteractionManager::GetExtraInfo(std::string &extraInfo)
{
    return INTER_MGR_IMPL.GetExtraInfo(extraInfo);
}

int32_t InteractionManager::AddPrivilege()
{
    return INTER_MGR_IMPL.AddPrivilege();
}

int32_t InteractionManager::EraseMouseIcon()
{
    return INTER_MGR_IMPL.EraseMouseIcon();
}

int32_t InteractionManager::SetMouseDragMonitorState(bool state)
{
    return INTER_MGR_IMPL.SetMouseDragMonitorState(state);
}

int32_t InteractionManager::AddSelectedPixelMap(std::shared_ptr<OHOS::Media::PixelMap> pixelMap,
    std::function<void(bool)> callback)
{
    return INTER_MGR_IMPL.AddSelectedPixelMap(pixelMap, callback);
}
#else
int32_t InteractionManager::StartDrag(const DragData &dragData)
{
    return DRAG_MANAGER.StartDrag(dragData);
}

int32_t InteractionManager::UpdateDragStyle(DragCursorStyle style)
{
    return DRAG_MANAGER.UpdateDragStyle(style);
}

int32_t InteractionManager::StopDrag(const DragDropResult &dropResult)
{
    return DRAG_MANAGER.StopDrag(dropResult);
}

int32_t InteractionManager::GetDragTargetPid()
{
    return DRAG_MANAGER.GetDragTargetPid();
}

int32_t InteractionManager::GetUdKey(std::string &udKey)
{
    return DRAG_MANAGER.GetUdKey(udKey);
}

int32_t InteractionManager::SetDragWindowVisible(bool visible, bool isForce)
{
    return DRAG_MANAGER.OnSetDragWindowVisible(visible, isForce);
}

int32_t InteractionManager::GetShadowOffset(int32_t &offsetX, int32_t &offsetY, int32_t &width, int32_t &height)
{
    ShadowOffset shadowOffset;
    int32_t ret = DRAG_MANAGER.OnGetShadowOffset(shadowOffset);
    if (ret != 0) {
        FI_HILOGE("shadowOffset is empty");
        return ret;
    }
    offsetX = shadowOffset.offsetX;
    offsetY = shadowOffset.offsetY;
    width = shadowOffset.width;
    height = shadowOffset.height;
    return ret;
}

int32_t InteractionManager::UpdateShadowPic(const ShadowInfo &shadowInfo)
{
    return DRAG_MANAGER.UpdateShadowPic(shadowInfo);
}

int32_t InteractionManager::GetDragData(DragData &dragData)
{
    return DRAG_MANAGER.GetDragData(dragData);
}

int32_t InteractionManager::GetDragState(DragState &dragState)
{
    return DRAG_MANAGER.GetDragState(dragState);
}

int32_t InteractionManager::UpdatePreviewStyle(const PreviewStyle &previewStyle)
{
    return DRAG_MANAGER.UpdatePreviewStyle(previewStyle);
}

int32_t InteractionManager::UpdatePreviewStyleWithAnimation(const PreviewStyle &previewStyle,
    const PreviewAnimation &animation)
{
    return DRAG_MANAGER.UpdatePreviewStyleWithAnimation(previewStyle, animation);
}

int32_t InteractionManager::GetDragSummary(std::map<std::string, int64_t> &summarys)
{
    return DRAG_MANAGER.GetDragSummary(summarys);
}

int32_t InteractionManager::GetDragAction(DragAction &dragAction)
{
    return DRAG_MANAGER.GetDragAction(dragAction);
}

int32_t InteractionManager::EnterTextEditorArea(bool enable)
{
    return DRAG_MANAGER.EnterTextEditorArea(enable);
}

int32_t InteractionManager::GetExtraInfo(std::string &extraInfo)
{
    return DRAG_MANAGER.GetExtraInfo(extraInfo);
}

int32_t InteractionManager::UpdatePointerAction(std::shared_ptr<MMI::PointerEvent> pointerEvent)
{
    return DRAG_MANAGER.UpdatePointerAction(pointerEvent);
}

void InteractionManager::SetDragWindow(std::shared_ptr<OHOS::Rosen::Window> window)
{
    DRAG_MANAGER.SetDragWindow(window);
}

void InteractionManager::RegisterDragWindow(std::function<void()> cb)
{
    DRAG_MANAGER.AddDragDestroy(cb);
}

void InteractionManager::SetSVGFilePath(const std::string &filePath)
{
    DRAG_MANAGER.SetSVGFilePath(filePath);
}
#endif // OHOS_BUILD_ENABLE_ARKUI_X
} // namespace DeviceStatus
} // namespace Msdp
} // namespace OHOS
