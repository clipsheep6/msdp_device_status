/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2023. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef JS_GESTURE_EVENT
#define JS_GESTURE_EVENT

#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "nocopyable.h"

namespace OHOS {
namespace MMI {
class JsGestureEvent final {
public:
    JsGestureEvent() = default;
    ~JsGestureEvent() = default;
    DISALLOW_COPY_AND_MOVE(JsGestureEvent);
    static napi_value Export(napi_env env, napi_value exports);
private:
    static napi_value GetNapiInt32(napi_env env, int32_t code);
    static napi_value EnumClassConstructor(napi_env env, napi_callback_info info);
};
} // namespace MMI
} // namespace OHOS

#endif // JS_GESTURE_EVENT