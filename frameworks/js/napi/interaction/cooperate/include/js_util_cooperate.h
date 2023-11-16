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

#ifndef JS_UTIL_COOPERATE_H
#define JS_UTIL_COOPERATE_H

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "napi/native_api.h"

#include "coordination_message.h"

#include "refbase.h"

namespace OHOS {
namespace Msdp {
namespace DeviceStatus {
class JsUtilCooperate {
public:
    struct UserData {
        int32_t userData { 0 };
        int32_t deviceId { 0 };
        napi_value handle { nullptr };
        std::vector<int32_t> keys;
    };
    struct CallbackData {
        bool enableResult { false };
        bool startResult { false };
        bool stopResult { false };
        bool coordinationOpened { false };
        std::string deviceDescriptor;
        int32_t errCode { 0 };
        CoordinationMessage msg = CoordinationMessage::PREPARE;
    };
    struct CallbackInfo : RefBase {
        CallbackInfo() = default;
        ~CallbackInfo() = default;
        int32_t errCode { 0 };
        napi_ref ref { nullptr };
        napi_env env { nullptr };
        napi_deferred deferred { nullptr };
        CallbackData data;
    };
    
    template <typename T>
    static void DeletePtr(T &ptr)
    {
        if (ptr != nullptr) {
            delete ptr;
            ptr = nullptr;
        }
    }

    static napi_value GetEnableInfo(sptr<CallbackInfo> cb);
    static napi_value GetStartInfo(sptr<CallbackInfo> cb);
    static napi_value GetStopInfo(sptr<CallbackInfo> cb);
    static napi_value GetStateInfo(sptr<CallbackInfo> cb);
    static napi_value GetStateResult(napi_env env, bool result);
    static napi_value GetResult(napi_env env, bool result, int32_t errorCode);
    static bool IsSameHandle(napi_env env, napi_value handle, napi_ref ref);
};
} // namespace DeviceStatus
} // namespace Msdp
} // namespace OHOS
#endif // JS_UTIL_COOPERATE_H
