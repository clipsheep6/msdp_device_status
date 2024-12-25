/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef MOTION_NAPI_H
#define MOTION_NAPI_H

#include <map>
#include <uv.h>

#include "napi/native_api.h"
#include "napi/native_node_api.h"

#ifdef MOTION_ENABLE
#include "motion_callback_stub.h"
#endif

#include "motion_event_napi.h"

namespace OHOS {
namespace Msdp {
#ifdef MOTION_ENABLE
class MotionCallback : public MotionCallbackStub {
public:
    explicit MotionCallback(napi_env env) : env_(env) {}
    ~MotionCallback() override {};
    void OnMotionChanged(const MotionEvent& event) override;
    static void EmitOnEvent(uv_work_t *work, int32_t status);

private:
    napi_env env_;
};
#endif

class MotionNapi : public MotionEventNapi {
public:
    explicit MotionNapi(napi_env env, napi_value thisVar);
    ~MotionNapi() override;

    static napi_value Init(napi_env env, napi_value exports);
    static napi_value SubscribeMotion(napi_env env, napi_callback_info info);
    static napi_value UnSubscribeMotion(napi_env env, napi_callback_info info);
    static napi_value GetRecentOptHandStatus(napi_env env, napi_callback_info info);

public:
#ifdef MOTION_ENABLE
    std::map<int32_t, sptr<IMotionCallback>> callbacks_;
#endif

private:
#ifdef MOTION_ENABLE
    static bool SubscribeCallback(napi_env env, int32_t type);
    static bool UnSubscribeCallback(napi_env env, int32_t type);
#endif
    static int32_t GetMotionType(const std::string &type);
    static bool ConstructMotion(napi_env env, napi_value jsThis);
    static bool ValidateArgsType(napi_env env, napi_value *args, size_t argc,
        const std::vector<std::string> &expectedTypes);
    static bool TransJsToStr(napi_env env, napi_value value, std::string &str);

private:
    napi_ref motionValueRef_ = nullptr;
    napi_env env_;
};
} // namespace Msdp
} // namespace OHOS
#endif //MOTION_NAPI_H
