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

#ifndef JS_EVENT_TARGET_H
#define JS_EVENT_TARGET_H

#include <cstdint>
#include <map>
#include <memory>
#include <queue>
#include <string>
#include <string_view>
#include <vector>

#include "nocopyable.h"
#include "uv.h"

#include "coordination_message.h"
#include "i_coordination_listener.h"
#include "i_event_listener.h"
#include "js_util.h"

namespace OHOS {
namespace Msdp {
namespace DeviceStatus {
class JsEventTarget : public ICoordinationListener,
                      public IEventListener,
                      public std::enable_shared_from_this<JsEventTarget> {
public:
    JsEventTarget();
    DISALLOW_COPY_AND_MOVE(JsEventTarget);
    virtual ~JsEventTarget() = default;

    static void EmitJsPrepare(sptr<JsUtil::CallbackInfo> cb, const std::string &networkId,
        const CoordinationMsgInfo &msgInfo);
    static void EmitJsActivate(sptr<JsUtil::CallbackInfo> cb, const std::string &remoteNetworkId,
        const CoordinationMsgInfo &msgInfo);
    static void EmitJsDeactivate(sptr<JsUtil::CallbackInfo> cb, const std::string &networkId,
        const CoordinationMsgInfo &msgInfo);
    static void EmitJsActivateCooperate(sptr<JsUtil::CallbackInfo> cb, const std::string &remoteNetworkId,
        const CoordinationMsgInfo &msgInfo);
    static void EmitJsGetCrossingSwitchState(sptr<JsUtil::CallbackInfo> cb, bool state);
    void AddListener(napi_env env, const std::string &type, napi_value handle);
    void RemoveListener(napi_env env, const std::string &type, napi_value handle);
    void AddListener(napi_env env, const std::string &type, const std::string &networkId, napi_value handle);
    void RemoveListener(napi_env env, const std::string &type, const std::string &networkId, napi_value handle);
    napi_value CreateCallbackInfo(napi_env env, napi_value handle, sptr<JsUtil::CallbackInfo> callback);
    napi_value CreateMouseCallbackInfo(napi_env env, napi_value handle, sptr<JsUtil::MouseCallbackInfo> callback);
    void ResetEnv();
    void OnCoordinationMessage(const std::string &networkId, CoordinationMessage msg) override;
    void OnMouseLocationEvent(const std::string &networkId, const Event &event) override;

private:
    static void CallPreparePromiseWork(uv_work_t *work, int32_t status);
    static void CallPrepareAsyncWork(uv_work_t *work, int32_t status);
    static void CallActivatePromiseWork(uv_work_t *work, int32_t status);
    static void CallActivateAsyncWork(uv_work_t *work, int32_t status);
    static void CallDeactivatePromiseWork(uv_work_t *work, int32_t status);
    static void CallDeactivateAsyncWork(uv_work_t *work, int32_t status);
    static void CallGetCrossingSwitchStatePromiseWork(uv_work_t *work, int32_t status);
    static void CallGetCrossingSwitchStateAsyncWork(uv_work_t *work, int32_t status);
    static void EmitCoordinationMessageEvent(uv_work_t *work, int32_t status);
    static void EmitMouseLocationEvent(uv_work_t *work, int32_t status);
    bool IsHandleExist(napi_env env, const std::string &networkId, napi_value handle);

private:
    std::atomic_bool isListeningProcess_ { false };
    struct CoordinationEvent {
        std::string networkId;
        CoordinationMessage msg { CoordinationMessage::UNKNOW };
    };
    inline static std::queue<CoordinationEvent> eventQueue_;
    inline static std::map<std::string_view, std::vector<sptr<JsUtil::CallbackInfo>>> coordinationListeners_;
    inline static std::map<std::string, std::vector<sptr<JsUtil::MouseCallbackInfo>>> mouseLocationListeners_;
};
} // namespace DeviceStatus
} // namespace Msdp
} // namespace OHOS
#endif // JS_EVENT_TARGET_H
