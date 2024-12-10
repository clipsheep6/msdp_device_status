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

#ifndef JS_COORDINATION_MANAGER_H
#define JS_COORDINATION_MANAGER_H

#include <mutex>
#include <string>

#include "js_event_target.h"

namespace OHOS {
namespace Msdp {
namespace DeviceStatus {
class JsCoordinationManager : public JsEventTarget {
public:
    JsCoordinationManager() = default;
    DISALLOW_COPY_AND_MOVE(JsCoordinationManager);
    ~JsCoordinationManager() = default;

    napi_value Prepare(napi_env env, bool isCompatible, napi_value handle = nullptr);
    napi_value Unprepare(napi_env env, bool isCompatible, napi_value handle = nullptr);
    napi_value Activate(napi_env env, const std::string &remoteNetworkDescriptor,
        int32_t startDeviceId, bool isCompatible, napi_value handle = nullptr);
    napi_value ActivateCooperateWithOptions(napi_env env, const std::string &remoteNetworkDescriptor,
        int32_t startDeviceId, CooperateOptions &cooperateOptions);
    napi_value Deactivate(napi_env env, bool isUnchained, bool isCompatible, napi_value handle = nullptr);
    napi_value GetCrossingSwitchState(napi_env env, const std::string &networkId,
        bool isCompatible, napi_value handle = nullptr);
    void ResetEnv();
    void RegisterListener(napi_env env, const std::string &type, napi_value handle);
    void UnregisterListener(napi_env env, const std::string &type, napi_value handle = nullptr);
    void RegisterListener(napi_env env, const std::string &type, const std::string &networkId, napi_value handle);
    void UnregisterListener(napi_env env, const std::string &type, const std::string &networkId,
        napi_value handle = nullptr);
};
} // namespace DeviceStatus
} // namespace Msdp
} // namespace OHOS
#endif // JS_COORDINATION_MANAGER_H
