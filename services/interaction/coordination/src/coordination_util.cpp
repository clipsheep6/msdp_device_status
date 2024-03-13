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

#include "coordination_util.h"

#include "softbus_bus_center.h"

#include "devicestatus_define.h"

#undef LOG_TAG
#define LOG_TAG "CoordinationUtil"

namespace OHOS {
namespace Msdp {
namespace DeviceStatus {
namespace COORDINATION {

std::string GetLocalNetworkId()
{
    CALL_DEBUG_ENTER;
    auto localNode = std::make_unique<NodeBasicInfo>();
    int32_t ret = GetLocalNodeDeviceInfo(FI_PKG_NAME, localNode.get());
    if (ret != RET_OK) {
        FI_HILOGE("Get local node device info, ret:%{public}d", ret);
        return {};
    }
    std::string networkId(localNode->networkId, sizeof(localNode->networkId));
    FI_HILOGD("Get local node device info, networkId:%{public}s", GetAnonyString(networkId).c_str());
    return localNode->networkId;
}
} // namespace COORDINATION
} // namespace DeviceStatus
} // namespace Msdp
} // namespace OHOS
