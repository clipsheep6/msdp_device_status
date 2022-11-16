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

#include "devicestatus_napi_error.h"

#include "devicestatus_hilog_wrapper.h"

namespace OHOS {
namespace Msdp {
namespace DeviceStatus {
bool GetApiError(int32_t code, NapiError& codeMsg)
{
    auto iter = NAPI_ERRORS.find(code);
    if (iter == NAPI_ERRORS.end()) {
        DEV_HILOGE(COMMON, "Error code %{public}d not found", code);
        return false;
    }
    codeMsg = iter->second;
    return true;
}
} // namespace DeviceStatus
} // namespace Msdp
} // namespace OHOS