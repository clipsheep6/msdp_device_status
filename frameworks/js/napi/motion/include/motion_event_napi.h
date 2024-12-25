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

#ifndef MOTION_EVENT_NAPI_H
#define MOTION_EVENT_NAPI_H

#include <list>
#include <map>
#include <memory>
#include <set>
#include <string>

#include "napi/native_api.h"
#ifdef MOTION_ENABLE
#include "motion_agent_type.h"
#endif

namespace OHOS {
namespace Msdp {
struct MotionEventListener {
    std::set<napi_ref> onRefSets;
};

class MotionEventNapi {
public:
    MotionEventNapi(napi_env env, napi_value thisVar);
    MotionEventNapi() = default;
    virtual ~MotionEventNapi();
#ifdef MOTION_ENABLE
    bool AddCallback(int32_t eventType, napi_value handler);
    bool CheckEvents(int32_t eventType);
    bool RemoveCallback(int32_t eventType);
    virtual void OnEventOperatingHand(int32_t eventType, size_t argc, const MotionEvent &event);
    void CreateIntData(napi_env env, napi_value motionValue, napi_value result, std::string name, int32_t value);

protected:
    bool IsSameValue(const napi_env &env, const napi_value &lhs, const napi_value &rhs);
    bool InsertRef(std::shared_ptr<MotionEventListener> listener, const napi_value &handler);
    void ConvertOperatingHandData(napi_value handler, size_t argc, const MotionEvent &event);
#endif

protected:
    napi_env env_;
    napi_ref thisVarRef_;
    std::map<int32_t, std::shared_ptr<MotionEventListener>> events_;
    int32_t count_ {0};
};
} // namespace Msdp
} // namespace OHOS
#endif // MOTION_EVENT_NAPI_H
