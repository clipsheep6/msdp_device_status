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

#include <string>

#include "devicestatus_define.h"
#include "drag_data.h"
#include "interaction_manager_impl.h"

namespace OHOS {
namespace Msdp {
namespace DeviceStatus {

InteractionManager *InteractionManager::instance_ = new (std::nothrow) InteractionManager();

InteractionManager *InteractionManager::GetInstance()
{
    return instance_;
}

int32_t InteractionManager::RegisterCoordinationListener(std::shared_ptr<ICoordinationListener> listener)
{
    return INTERACTION_MGR_IMPL.RegisterCoordinationListener(listener);
}

int32_t InteractionManager::UnregisterCoordinationListener(std::shared_ptr<ICoordinationListener> listener)
{
    return INTERACTION_MGR_IMPL.UnregisterCoordinationListener(listener);
}

int32_t InteractionManager::PrepareCoordination(std::function<void(const std::string&, CoordinationMessage)> callback)
{
    return INTERACTION_MGR_IMPL.PrepareCoordination(callback);
}

int32_t InteractionManager::UnprepareCoordination(std::function<void(const std::string&, CoordinationMessage)> callback)
{
    return INTERACTION_MGR_IMPL.UnprepareCoordination(callback);
}

int32_t InteractionManager::ActivateCoordination(const std::string &remoteNetworkId, int32_t startDeviceId,
    std::function<void(const std::string&, CoordinationMessage)> callback)
{
    return INTERACTION_MGR_IMPL.ActivateCoordination(remoteNetworkId, startDeviceId, callback);
}

int32_t InteractionManager::DeactivateCoordination(std::function<void(const std::string&,
    CoordinationMessage)> callback)
{
    return INTERACTION_MGR_IMPL.DeactivateCoordination(callback);
}

int32_t InteractionManager::GetCoordinationState(
    const std::string &deviceId, std::function<void(bool)> callback)
{
    return INTERACTION_MGR_IMPL.GetCoordinationState(deviceId, callback);
}

int32_t InteractionManager::UpdateDragStyle(DragCursorStyle style)
{
    return INTERACTION_MGR_IMPL.UpdateDragStyle(style);
}

int32_t InteractionManager::StartDrag(const DragData &dragData, std::function<void(const DragNotifyMsg&)> callback)
{
    return INTERACTION_MGR_IMPL.StartDrag(dragData, callback);
}

int32_t InteractionManager::StopDrag(DragResult result, bool hasCustomAnimation)
{
    return INTERACTION_MGR_IMPL.StopDrag(result, hasCustomAnimation);
}

int32_t InteractionManager::GetDragTargetPid()
{
    return INTERACTION_MGR_IMPL.GetDragTargetPid();
}

int32_t InteractionManager::GetUdKey(std::string &udKey)
{
    return INTERACTION_MGR_IMPL.GetUdKey(udKey);
}

int32_t InteractionManager::AddDraglistener(DragListenerPtr listener)
{
    return INTERACTION_MGR_IMPL.AddDraglistener(listener);
}

int32_t InteractionManager::RemoveDraglistener(DragListenerPtr listener)
{
    return INTERACTION_MGR_IMPL.RemoveDraglistener(listener);
}

int32_t InteractionManager::SetDragWindowVisible(bool visible)
{
    return INTERACTION_MGR_IMPL.SetDragWindowVisible(visible);
}

int32_t InteractionManager::GetShadowOffset(int32_t& offsetX, int32_t& offsetY, int32_t& width, int32_t& height)
{
    return INTERACTION_MGR_IMPL.GetShadowOffset(offsetX, offsetY, width, height);
}
} // namespace DeviceStatus
} // namespace Msdp
} // namespace OHOS
