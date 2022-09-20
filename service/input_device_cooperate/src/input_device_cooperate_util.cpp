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

#include "input_device_cooperate_util.h"

#include "softbus_bus_center.h"

#include "config_multimodal.h"
#include "define_multimodal.h"
namespace OHOS {
namespace MMI {
namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, MMI_LOG_DOMAIN, "InputDeviceCooperateUtil"};
} // namespace
void GetLocalDeviceId(std::string &local)
{
    local = "";
    auto localNode = std::make_unique<NodeBasicInfo>();
    CHKPV(localNode);
    int32_t errCode = GetLocalNodeDeviceInfo(MMI_DSOFTBUS_PKG_NAME, localNode.get());
    if (errCode != RET_OK) {
        MMI_HILOGE("GetLocalNodeDeviceInfo errCode: %{public}d", errCode);
        return;
    }
    local = localNode->networkId;
}
} // namespace MMI
} // namespace OHOS
