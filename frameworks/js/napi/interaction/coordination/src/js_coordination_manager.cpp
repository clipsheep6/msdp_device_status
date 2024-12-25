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

#include "js_coordination_manager.h"

#include "devicestatus_define.h"
#include "interaction_manager.h"
#include "util_napi_error.h"

#undef LOG_TAG
#define LOG_TAG "JsCoordinationManager"

namespace OHOS {
namespace Msdp {
namespace DeviceStatus {

napi_value JsCoordinationManager::Prepare(napi_env env, bool isCompatible, napi_value handle)
{
    CALL_INFO_TRACE;
    sptr<JsUtil::CallbackInfo> cb = new (std::nothrow) JsUtil::CallbackInfo();
    CHKPP(cb);
    napi_value result = CreateCallbackInfo(env, handle, cb);
    auto callback = [this, cb](const std::string &networkId, const CoordinationMsgInfo &msgInfo) {
        this->EmitJsPrepare(cb, networkId, msgInfo);
    };
    int32_t errCode = INTERACTION_MGR->PrepareCoordination(callback, isCompatible);
    if (errCode != RET_OK) {
        UtilNapiError::HandleExecuteResult(env, errCode, "prepare", COOPERATE_PERMISSION);
        RELEASE_CALLBACKINFO(cb->env, cb->ref);
    }
    return result;
}

napi_value JsCoordinationManager::Unprepare(napi_env env, bool isCompatible, napi_value handle)
{
    CALL_INFO_TRACE;
    sptr<JsUtil::CallbackInfo> cb = new (std::nothrow) JsUtil::CallbackInfo();
    CHKPP(cb);
    napi_value result = CreateCallbackInfo(env, handle, cb);
    auto callback = [this, cb](const std::string &networkId, const CoordinationMsgInfo &msgInfo) {
        this->EmitJsPrepare(cb, networkId, msgInfo);
    };
    int32_t errCode = INTERACTION_MGR->UnprepareCoordination(callback, isCompatible);
    if (errCode != RET_OK) {
        UtilNapiError::HandleExecuteResult(env, errCode, "unprepare", COOPERATE_PERMISSION);
        RELEASE_CALLBACKINFO(cb->env, cb->ref);
    }
    return result;
}

napi_value JsCoordinationManager::Activate(napi_env env, const std::string &remoteNetworkId,
    int32_t startDeviceId, bool isCompatible, napi_value handle)
{
    CALL_INFO_TRACE;
    sptr<JsUtil::CallbackInfo> cb = new (std::nothrow) JsUtil::CallbackInfo();
    CHKPP(cb);
    napi_value result = CreateCallbackInfo(env, handle, cb);
    auto callback = [this, cb](const std::string &remoteNetworkId, const CoordinationMsgInfo &msgInfo) {
        this->EmitJsActivate(cb, remoteNetworkId, msgInfo);
    };
    int32_t errCode = INTERACTION_MGR->ActivateCoordination(
        remoteNetworkId, startDeviceId, callback, isCompatible);
    if (errCode != RET_OK) {
        UtilNapiError::HandleExecuteResult(env, errCode, "activate", COOPERATE_PERMISSION);
        RELEASE_CALLBACKINFO(cb->env, cb->ref);
    }
    return result;
}

napi_value JsCoordinationManager::ActivateCooperateWithOptions(napi_env env, const std::string &remoteNetworkId,
    int32_t startDeviceId, CooperateOptions &cooperateOptions)
{
    CALL_INFO_TRACE;
    sptr<JsUtil::CallbackInfo> cb = new (std::nothrow) JsUtil::CallbackInfo();
    CHKPP(cb);
    napi_value handle = nullptr;
    napi_value result = CreateCallbackInfo(env, handle, cb);
    auto callback = [this, cb](const std::string &remoteNetworkId, const CoordinationMsgInfo &msgInfo) {
        this->EmitJsActivate(cb, remoteNetworkId, msgInfo);
    };
    int32_t errCode = INTERACTION_MGR->ActivateCooperateWithOptions(remoteNetworkId, startDeviceId, callback, cooperateOptions);
    if (errCode != RET_OK) {
        UtilNapiError::HandleExecuteResult(env, errCode, "activateCooperateWithOptions", COOPERATE_PERMISSION);
        RELEASE_CALLBACKINFO(cb->env, cb->ref);
    }
    return result;
}

napi_value JsCoordinationManager::Deactivate(napi_env env,
    bool isUnchained, bool isCompatible, napi_value handle)
{
    CALL_INFO_TRACE;
    sptr<JsUtil::CallbackInfo> cb = new (std::nothrow) JsUtil::CallbackInfo();
    CHKPP(cb);
    napi_value result = CreateCallbackInfo(env, handle, cb);
    auto callback = [this, cb](const std::string &networkId, const CoordinationMsgInfo &msgInfo) {
        this->EmitJsDeactivate(cb, networkId, msgInfo);
    };
    int32_t errCode = INTERACTION_MGR->DeactivateCoordination(isUnchained, callback, isCompatible);
    if (errCode != RET_OK) {
        UtilNapiError::HandleExecuteResult(env, errCode, "deactivate", COOPERATE_PERMISSION);
        RELEASE_CALLBACKINFO(cb->env, cb->ref);
    }
    return result;
}

napi_value JsCoordinationManager::GetCrossingSwitchState(napi_env env,
    const std::string &networkId, bool isCompatible, napi_value handle)
{
    CALL_INFO_TRACE;
    sptr<JsUtil::CallbackInfo> cb = new (std::nothrow) JsUtil::CallbackInfo();
    CHKPP(cb);
    napi_value result = CreateCallbackInfo(env, handle, cb);
    auto callback = [this, cb](bool state) {
        this->EmitJsGetCrossingSwitchState(cb, state);
    };
    int32_t errCode = INTERACTION_MGR->GetCoordinationState(networkId, callback, isCompatible);
    if (errCode != RET_OK) {
        UtilNapiError::HandleExecuteResult(env, errCode, "getCrossingSwitchState", COOPERATE_PERMISSION);
        RELEASE_CALLBACKINFO(cb->env, cb->ref);
    }
    return result;
}

void JsCoordinationManager::RegisterListener(napi_env env, const std::string &type, napi_value handle)
{
    CALL_INFO_TRACE;
    AddListener(env, type, handle);
}

void JsCoordinationManager::UnregisterListener(napi_env env, const std::string &type, napi_value handle)
{
    CALL_INFO_TRACE;
    RemoveListener(env, type, handle);
}

void JsCoordinationManager::RegisterListener(napi_env env, const std::string &type, const std::string &networkId,
    napi_value handle)
{
    CALL_INFO_TRACE;
    AddListener(env, type, networkId, handle);
}

void JsCoordinationManager::UnregisterListener(napi_env env, const std::string &type, const std::string &networkId,
    napi_value handle)
{
    CALL_INFO_TRACE;
    RemoveListener(env, type, networkId, handle);
}

void JsCoordinationManager::ResetEnv()
{
    CALL_INFO_TRACE;
    JsEventTarget::ResetEnv();
}
} // namespace DeviceStatus
} // namespace Msdp
} // namespace OHOS
