/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "drag_adapter.h"

#include "devicestatus_define.h"

namespace OHOS {
namespace Msdp {
namespace DeviceStatus {
namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, MSDP_DOMAIN_ID, "DragAdapter" };
} // namespace

DragAdapter::DragAdapter() {}

DragAdapter::~DragAdapter() {}

int32_t DragAdapter::RegisterCallback(GetDragStateCallback callback)
{
    CHKPR(callback, RET_ERR);
    getDragStateCallback_ = callback;
    return RET_OK;
}

bool DragAdapter::IsDragging()
{
    int32_t dragState;
    getDragStateCallback_(dragState);
    if (dragState == draging) {
        return true;
    }
    return false;
}
} // namespace DeviceStatus
} // namespace Msdp
} // namespace OHOS
