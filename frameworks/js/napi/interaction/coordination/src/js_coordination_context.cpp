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

#include "js_coordination_context.h"

#include "devicestatus_define.h"
#include "napi_constants.h"
#include "util_napi_error.h"
#include "utility.h"

#undef LOG_TAG
#define LOG_TAG "JsCoordinationContext"

namespace OHOS {
namespace Msdp {
namespace DeviceStatus {
namespace {
const char* COORDINATION_CLASS { "Coordination_class" };
const char* COORDINATION { "Coordination" };
inline constexpr std::string_view GET_VALUE_BOOL { "napi_get_value_bool" };
inline constexpr std::string_view GET_VALUE_INT32 { "napi_get_value_int32" };
inline constexpr std::string_view GET_VALUE_STRING_UTF8 { "napi_get_value_string_utf8" };
inline constexpr size_t MAX_STRING_LEN { 1024 };
} // namespace

JsCoordinationContext::JsCoordinationContext()
    : mgr_(std::make_shared<JsCoordinationManager>()) {}

JsCoordinationContext::~JsCoordinationContext()
{
    std::lock_guard<std::mutex> guard(mutex_);
    auto jsCoordinationMgr = mgr_;
    mgr_.reset();
    if (jsCoordinationMgr != nullptr) {
        jsCoordinationMgr->ResetEnv();
    }
}

napi_value JsCoordinationContext::Export(napi_env env, napi_value exports)
{
    CALL_INFO_TRACE;
    auto instance = CreateInstance(env);
    CHKPP(instance);
    DeclareDeviceCoordinationInterface(env, exports);
    DeclareDeviceCoordinationData(env, exports);
    DeclareDeviceCooperateData(env, exports);
    return exports;
}

napi_value JsCoordinationContext::Prepare(napi_env env, napi_callback_info info)
{
    CALL_INFO_TRACE;
    return PrepareCompatible(env, info);
}

napi_value JsCoordinationContext::PrepareCooperate(napi_env env, napi_callback_info info)
{
    CALL_INFO_TRACE;
    return PrepareCompatible(env, info, true);
}

napi_value JsCoordinationContext::PrepareCompatible(napi_env env, napi_callback_info info, bool isCompatible)
{
    size_t argc = 1;
    napi_value argv[1] = { nullptr };
    CHKRP(napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr), GET_CB_INFO);

    JsCoordinationContext *jsDev = JsCoordinationContext::GetInstance(env);
    CHKPP(jsDev);
    std::shared_ptr<JsCoordinationManager> jsCoordinationMgr = jsDev->GetJsCoordinationMgr();
    CHKPP(jsCoordinationMgr);
    if (argc == 0) {
        return jsCoordinationMgr->Prepare(env, isCompatible);
    }
    if (!UtilNapi::TypeOf(env, argv[0], napi_function)) {
        THROWERR(env, COMMON_PARAMETER_ERROR, "callback", "function");
        return nullptr;
    }
    return jsCoordinationMgr->Prepare(env, isCompatible, argv[0]);
}

napi_value JsCoordinationContext::Unprepare(napi_env env, napi_callback_info info)
{
    CALL_INFO_TRACE;
    return UnprepareCompatible(env, info);
}

napi_value JsCoordinationContext::UnprepareCooperate(napi_env env, napi_callback_info info)
{
    CALL_INFO_TRACE;
    return UnprepareCompatible(env, info, true);
}

napi_value JsCoordinationContext::UnprepareCompatible(napi_env env, napi_callback_info info, bool isCompatible)
{
    size_t argc = 1;
    napi_value argv[1] = { nullptr };
    CHKRP(napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr), GET_CB_INFO);

    JsCoordinationContext *jsDev = JsCoordinationContext::GetInstance(env);
    CHKPP(jsDev);
    std::shared_ptr<JsCoordinationManager> jsCoordinationMgr = jsDev->GetJsCoordinationMgr();
    CHKPP(jsCoordinationMgr);
    if (argc == 0) {
        return jsCoordinationMgr->Unprepare(env, isCompatible);
    }
    if (!UtilNapi::TypeOf(env, argv[0], napi_function)) {
        THROWERR(env, COMMON_PARAMETER_ERROR, "callback", "function");
        return nullptr;
    }
    return jsCoordinationMgr->Unprepare(env, isCompatible, argv[0]);
}

napi_value JsCoordinationContext::Activate(napi_env env, napi_callback_info info)
{
    CALL_INFO_TRACE;
    return ActivateCompatible(env, info);
}

napi_value JsCoordinationContext::ActivateCooperate(napi_env env, napi_callback_info info)
{
    CALL_INFO_TRACE;
    return ActivateCompatible(env, info, true);
}

napi_value JsCoordinationContext::ActivateCompatible(napi_env env, napi_callback_info info, bool isCompatible)
{
    size_t argc = 3;
    napi_value argv[3] = { nullptr };
    CHKRP(napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr), GET_CB_INFO);

    if (argc < 2) {
        THROWERR_CUSTOM(env, COMMON_PARAMETER_ERROR, "Wrong number of parameters");
        return nullptr;
    }
    if (!UtilNapi::TypeOf(env, argv[0], napi_string)) {
        THROWERR(env, COMMON_PARAMETER_ERROR, "targetNetworkId", "string");
        return nullptr;
    }
    if (!UtilNapi::TypeOf(env, argv[1], napi_number)) {
        THROWERR(env, COMMON_PARAMETER_ERROR, "inputDeviceId", "number");
        return nullptr;
    }
    char remoteNetworkId[MAX_STRING_LEN] = { 0 };
    int32_t startDeviceId = 0;
    size_t length = 0;
    CHKRP(napi_get_value_string_utf8(env, argv[0], remoteNetworkId,
        sizeof(remoteNetworkId), &length), GET_VALUE_STRING_UTF8);
    CHKRP(napi_get_value_int32(env, argv[1], &startDeviceId), GET_VALUE_INT32);

    JsCoordinationContext *jsDev = JsCoordinationContext::GetInstance(env);
    CHKPP(jsDev);
    std::shared_ptr<JsCoordinationManager> jsCoordinationMgr = jsDev->GetJsCoordinationMgr();
    CHKPP(jsCoordinationMgr);
    if (argc == 2) {
        return jsCoordinationMgr->Activate(env, remoteNetworkId, startDeviceId, isCompatible);
    }
    if (!UtilNapi::TypeOf(env, argv[2], napi_function)) {
        THROWERR(env, COMMON_PARAMETER_ERROR, "callback", "function");
        return nullptr;
    }
    return jsCoordinationMgr->Activate(env, std::string(remoteNetworkId), startDeviceId, isCompatible, argv[2]);
}

CooperateOptions JsCoordinationContext::GetCooperationsData(napi_env env, CooperateOptions &cooperateOptions, napi_value optionsHandle)
{
    int32_t displayX = 0;
    cooperateOptions.displayX = JsUtil::GetNamePropertyInt32(env, optionsHandle, "displayX", displayX);
    FI_HILOGE("szy___GetCooperationsData____displayX :%{public}d", cooperateOptions.displayX);
    int32_t displayY = 0;
    cooperateOptions.displayY = JsUtil::GetNamePropertyInt32(env, optionsHandle, "displayY", displayY);
    FI_HILOGE("szy___GetCooperationsData____displayY :%{public}d", cooperateOptions.displayY);
    uint32_t displayId = 0;
    cooperateOptions.displayId = JsUtil::GetNamePropertyInt32(env, optionsHandle, "displayId", displayId);
    FI_HILOGE("szy___GetCooperationsData____displayId :%{public}d", cooperateOptions.displayId);
    return cooperateOptions;
}

napi_value JsCoordinationContext::ActivateCooperateWithOptions(napi_env env, napi_callback_info info)
{
    size_t argc = 3;
    napi_value argv[3] = { nullptr };
    CHKRP(napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr), GET_CB_INFO);

    if (argc < 3) {
        THROWERR_CUSTOM(env, COMMON_PARAMETER_ERROR, "Wrong number of parameters");
        return nullptr;
    }
    if (!UtilNapi::TypeOf(env, argv[0], napi_string)) {
        THROWERR(env, COMMON_PARAMETER_ERROR, "targetNetworkId", "string");
        return nullptr;
    }
    if (!UtilNapi::TypeOf(env, argv[1], napi_number)) {
        THROWERR(env, COMMON_PARAMETER_ERROR, "inputDeviceId", "number");
        return nullptr;
    }
    char remoteNetworkId[MAX_STRING_LEN] = { 0 };
    int32_t startDeviceId = 0;
    size_t length = 0;
    CHKRP(napi_get_value_string_utf8(env, argv[0], remoteNetworkId, sizeof(remoteNetworkId), &length), GET_VALUE_STRING_UTF8);
    CHKRP(napi_get_value_int32(env, argv[1], &startDeviceId), GET_VALUE_INT32);

    if (!UtilNapi::TypeOf(env, argv[2], napi_object)) {
        THROWERR(env, COMMON_PARAMETER_ERROR, "cooperateOptions", "object");
        return nullptr;
    }

    CooperateOptions cooperateOptions = GetCooperationsData(env, cooperateOptions, argv[2]);
    JsCoordinationContext *jsDev = JsCoordinationContext::GetInstance(env);
    CHKPP(jsDev);
    std::shared_ptr<JsCoordinationManager> jsCoordinationMgr = jsDev->GetJsCoordinationMgr();
    CHKPP(jsCoordinationMgr);
    return jsCoordinationMgr->ActivateCooperateWithOptions(env, remoteNetworkId, startDeviceId, cooperateOptions);
}

napi_value JsCoordinationContext::Deactivate(napi_env env, napi_callback_info info)
{
    CALL_INFO_TRACE;
    return DeactivateCompatible(env, info);
}

napi_value JsCoordinationContext::DeactivateCooperate(napi_env env, napi_callback_info info)
{
    CALL_INFO_TRACE;
    return DeactivateCompatible(env, info, true);
}

napi_value JsCoordinationContext::DeactivateCompatible(napi_env env, napi_callback_info info, bool isCompatible)
{
    size_t argc = 2;
    napi_value argv[2] = { nullptr };
    CHKRP(napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr), GET_CB_INFO);

    if (argc == 0) {
        THROWERR_CUSTOM(env, COMMON_PARAMETER_ERROR, "Wrong number of parameters");
        return nullptr;
    }
    if (!UtilNapi::TypeOf(env, argv[0], napi_boolean)) {
        THROWERR(env, COMMON_PARAMETER_ERROR, "isUnchained", "boolean");
        return nullptr;
    }
    bool isUnchained = false;
    CHKRP(napi_get_value_bool(env, argv[0], &isUnchained), GET_VALUE_BOOL);

    JsCoordinationContext *jsDev = JsCoordinationContext::GetInstance(env);
    CHKPP(jsDev);
    std::shared_ptr<JsCoordinationManager> jsCoordinationMgr = jsDev->GetJsCoordinationMgr();
    CHKPP(jsCoordinationMgr);
    if (argc == 1) {
        return jsCoordinationMgr->Deactivate(env, isUnchained, isCompatible);
    }
    if (!UtilNapi::TypeOf(env, argv[1], napi_function)) {
        THROWERR(env, COMMON_PARAMETER_ERROR, "callback", "function");
        return nullptr;
    }
    return jsCoordinationMgr->Deactivate(env, isUnchained, isCompatible, argv[1]);
}

napi_value JsCoordinationContext::GetCrossingSwitchState(napi_env env, napi_callback_info info)
{
    CALL_INFO_TRACE;
    return GetCrossingSwitchStateCompatible(env, info);
}

napi_value JsCoordinationContext::GetCooperateSwitchState(napi_env env, napi_callback_info info)
{
    CALL_INFO_TRACE;
    return GetCrossingSwitchStateCompatible(env, info, true);
}

napi_value JsCoordinationContext::GetCrossingSwitchStateCompatible(napi_env env,
    napi_callback_info info, bool isCompatible)
{
    size_t argc = 2;
    napi_value argv[2] = { nullptr };
    CHKRP(napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr), GET_CB_INFO);

    if (argc == 0) {
        THROWERR_CUSTOM(env, COMMON_PARAMETER_ERROR, "Incorrect parameter count");
        return nullptr;
    }
    if (!UtilNapi::TypeOf(env, argv[0], napi_string)) {
        THROWERR(env, COMMON_PARAMETER_ERROR, "networkId", "string");
        return nullptr;
    }
    char networkId[MAX_STRING_LEN] = { 0 };
    size_t length = 0;
    CHKRP(napi_get_value_string_utf8(env, argv[0], networkId,
        sizeof(networkId), &length), GET_VALUE_STRING_UTF8);
    std::string networkIdTemp = networkId;

    JsCoordinationContext *jsDev = JsCoordinationContext::GetInstance(env);
    CHKPP(jsDev);
    std::shared_ptr<JsCoordinationManager> jsCoordinationMgr = jsDev->GetJsCoordinationMgr();
    CHKPP(jsCoordinationMgr);
    if (argc == 1) {
        return jsCoordinationMgr->GetCrossingSwitchState(env, networkIdTemp, isCompatible);
    }
    if (!UtilNapi::TypeOf(env, argv[1], napi_function)) {
        THROWERR(env, COMMON_PARAMETER_ERROR, "callback", "function");
        return nullptr;
    }
    return jsCoordinationMgr->GetCrossingSwitchState(env, networkIdTemp, isCompatible, argv[1]);
}

napi_value JsCoordinationContext::On(napi_env env, napi_callback_info info)
{
    CALL_INFO_TRACE;
    size_t argc = 1;
    napi_value argv[1] = { nullptr };
    CHKRP(napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr), GET_CB_INFO);

    if (argc < 1) {
        THROWERR_CUSTOM(env, COMMON_PARAMETER_ERROR, "Parameter mismatch error");
        return nullptr;
    }
    if (!UtilNapi::TypeOf(env, argv[0], napi_string)) {
        THROWERR(env, COMMON_PARAMETER_ERROR, "type", "string");
        return nullptr;
    }

    char type[MAX_STRING_LEN] = { 0 };
    size_t length = 0;
    CHKRP(napi_get_value_string_utf8(env, argv[0], type, sizeof(type), &length), GET_VALUE_STRING_UTF8);

    if ((COOPERATE_NAME.compare(type)) == 0 || (COOPERATE_MESSAGE_NAME.compare(type)) == 0) {
        return RegisterCooperateListener(env, type, info);
    }
    if ((COOPERATE_MOUSE_NAME.compare(type)) == 0) {
        return RegisterMouseListener(env, info);
    }
    FI_HILOGE("Unknow type:%{public}s", std::string(type).c_str());
    THROWERR_CUSTOM(env, COMMON_PARAMETER_ERROR, "Type must be cooperate, cooperateMessage or cooperateMouse");
    return nullptr;
}

napi_value JsCoordinationContext::Off(napi_env env, napi_callback_info info)
{
    CALL_INFO_TRACE;
    napi_value argv[1] = { nullptr };
    size_t argc = 1;
    CHKRP(napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr), GET_CB_INFO);
    if (argc < 1) {
        THROWERR_CUSTOM(env, COMMON_PARAMETER_ERROR, "Wrong number of parameters");
        return nullptr;
    }
    if (!UtilNapi::TypeOf(env, argv[0], napi_string)) {
        THROWERR(env, COMMON_PARAMETER_ERROR, "type", "string");
        return nullptr;
    }
    size_t length = 0;
    char type[MAX_STRING_LEN] = { 0 };
    CHKRP(napi_get_value_string_utf8(env, argv[0], type, sizeof(type), &length), GET_VALUE_STRING_UTF8);

    if ((COOPERATE_NAME.compare(type)) == 0 || (COOPERATE_MESSAGE_NAME.compare(type)) == 0) {
        return UnregisterCooperateListener(env, type, info);
    }
    if ((COOPERATE_MOUSE_NAME.compare(type)) == 0) {
        return UnregisterMouseListener(env, info);
    }
    FI_HILOGE("Unknow type:%{public}s", std::string(type).c_str());
    THROWERR_CUSTOM(env, COMMON_PARAMETER_ERROR, "Type must be cooperate, cooperateMessage or cooperateMouse");
    return nullptr;
}

napi_value JsCoordinationContext::RegisterCooperateListener(
    napi_env env, const std::string &type, napi_callback_info info)
{
    CALL_INFO_TRACE;
    size_t argc = 2;
    napi_value argv[2] = { nullptr };
    CHKRP(napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr), GET_CB_INFO);
    if (argc < 2) {
        THROWERR_CUSTOM(env, COMMON_PARAMETER_ERROR, "Wrong number of parameters");
        return nullptr;
    }
    JsCoordinationContext *jsDev = JsCoordinationContext::GetInstance(env);
    CHKPP(jsDev);
    std::shared_ptr<JsCoordinationManager> jsCoordinationMgr = jsDev->GetJsCoordinationMgr();
    CHKPP(jsCoordinationMgr);
    if (!UtilNapi::TypeOf(env, argv[1], napi_function)) {
        THROWERR(env, COMMON_PARAMETER_ERROR, "callback", "function");
        return nullptr;
    }
    jsCoordinationMgr->RegisterListener(env, type, argv[1]);
    return nullptr;
}

napi_value JsCoordinationContext::UnregisterCooperateListener(
    napi_env env, const std::string &type, napi_callback_info info)
{
    CALL_INFO_TRACE;
    size_t argc = 2;
    napi_value argv[2] = { nullptr };
    CHKRP(napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr), GET_CB_INFO);
    if (argc < 1) {
        THROWERR_CUSTOM(env, COMMON_PARAMETER_ERROR, "Wrong number of parameters");
        return nullptr;
    }
    JsCoordinationContext *jsDev = JsCoordinationContext::GetInstance(env);
    CHKPP(jsDev);
    std::shared_ptr<JsCoordinationManager> jsCoordinationMgr = jsDev->GetJsCoordinationMgr();
    CHKPP(jsCoordinationMgr);
    if (argc < 2) {
        jsCoordinationMgr->UnregisterListener(env, type);
        return nullptr;
    }
    if (UtilNapi::TypeOf(env, argv[1], napi_undefined) || UtilNapi::TypeOf(env, argv[1], napi_null)) {
        FI_HILOGW("Undefined callback, unregister all listener of type:%{public}s", type.c_str());
        jsCoordinationMgr->UnregisterListener(env, type);
        return nullptr;
    }
    if (UtilNapi::TypeOf(env, argv[1], napi_function)) {
        jsCoordinationMgr->UnregisterListener(env, type, argv[1]);
        return nullptr;
    }
    THROWERR(env, COMMON_PARAMETER_ERROR, "callback", "function");
    FI_HILOGE("UnregisterCooperateListener failed, invalid parameter");
    return nullptr;
}

napi_value JsCoordinationContext::RegisterMouseListener(napi_env env, napi_callback_info info)
{
    CALL_INFO_TRACE;
    size_t argc = 3;
    napi_value argv[3] = { nullptr };
    CHKRP(napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr), GET_CB_INFO);
    if (argc < 3) {
        THROWERR_CUSTOM(env, COMMON_PARAMETER_ERROR, "Wrong number of parameters");
        return nullptr;
    }
    char type[MAX_STRING_LEN] = { 0 };
    size_t length = 0;
    CHKRP(napi_get_value_string_utf8(env, argv[0], type, sizeof(type), &length), GET_VALUE_STRING_UTF8);
    if ((COOPERATE_MOUSE_NAME.compare(type)) != 0) {
        THROWERR_CUSTOM(env, COMMON_PARAMETER_ERROR, "Type must be cooperateMouse");
        return nullptr;
    }

    char networkId[MAX_STRING_LEN] = { 0 };
    size_t len = 0;
    CHKRP(napi_get_value_string_utf8(env, argv[1], networkId, sizeof(networkId), &len), GET_VALUE_STRING_UTF8);

    if (!UtilNapi::TypeOf(env, argv[2], napi_function)) {
        THROWERR(env, COMMON_PARAMETER_ERROR, "callback", "function");
        return nullptr;
    }

    JsCoordinationContext *jsDev = JsCoordinationContext::GetInstance(env);
    CHKPP(jsDev);
    std::shared_ptr<JsCoordinationManager> jsCoordinationMgr = jsDev->GetJsCoordinationMgr();
    CHKPP(jsCoordinationMgr);
    jsCoordinationMgr->RegisterListener(env, type, networkId, argv[2]);
    return nullptr;
}

napi_value JsCoordinationContext::UnregisterMouseListener(napi_env env, napi_callback_info info)
{
    CALL_INFO_TRACE;
    napi_value argv[3] = { nullptr };
    size_t argc = 3;
    CHKRP(napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr), GET_CB_INFO);
    if (argc < 2) {
        THROWERR_CUSTOM(env, COMMON_PARAMETER_ERROR, "Wrong number of parameters");
        return nullptr;
    }
    char type[MAX_STRING_LEN] = { 0 };
    size_t length = 0;
    CHKRP(napi_get_value_string_utf8(env, argv[0], type, sizeof(type), &length), GET_VALUE_STRING_UTF8);
    if ((COOPERATE_MOUSE_NAME.compare(type)) != 0) {
        THROWERR_CUSTOM(env, COMMON_PARAMETER_ERROR, "Type must be cooperateMouse");
        return nullptr;
    }

    char networkId[MAX_STRING_LEN] = { 0 };
    size_t len = 0;
    CHKRP(napi_get_value_string_utf8(env, argv[1], networkId, sizeof(networkId), &len), GET_VALUE_STRING_UTF8);

    JsCoordinationContext *jsDev = JsCoordinationContext::GetInstance(env);
    CHKPP(jsDev);
    std::shared_ptr<JsCoordinationManager> jsCoordinationMgr = jsDev->GetJsCoordinationMgr();
    CHKPP(jsCoordinationMgr);

    if (argc == 2) {
        jsCoordinationMgr->UnregisterListener(env, type, networkId);
        return nullptr;
    }
    if (UtilNapi::TypeOf(env, argv[2], napi_undefined) || UtilNapi::TypeOf(env, argv[2], napi_null)) {
        FI_HILOGW("Undefined callback, unregister all listener of networkId: %{public}s",
            Utility::Anonymize(networkId).c_str());
        jsCoordinationMgr->UnregisterListener(env, type, networkId);
        return nullptr;
    }
    if (UtilNapi::TypeOf(env, argv[2], napi_function)) {
        jsCoordinationMgr->UnregisterListener(env, type, networkId, argv[2]);
        return nullptr;
    }
    THROWERR(env, COMMON_PARAMETER_ERROR, "callback", "function");
    FI_HILOGE("UnregisterMouseListener failed, invalid parameter");
    return nullptr;
}

std::shared_ptr<JsCoordinationManager> JsCoordinationContext::GetJsCoordinationMgr()
{
    std::lock_guard<std::mutex> guard(mutex_);
    return mgr_;
}

napi_value JsCoordinationContext::CreateInstance(napi_env env)
{
    CALL_INFO_TRACE;
    napi_value global = nullptr;
    CHKRP(napi_get_global(env, &global), GET_GLOBAL);

    constexpr char className[] = "JsCoordinationContext";
    napi_value jsClass = nullptr;
    napi_property_descriptor desc[] = {};
    napi_status status = napi_define_class(env, className, sizeof(className),
        JsCoordinationContext::JsConstructor, nullptr, sizeof(desc) / sizeof(desc[0]), nullptr, &jsClass);
    CHKRP(status, DEFINE_CLASS);

    status = napi_set_named_property(env, global, COORDINATION_CLASS, jsClass);
    CHKRP(status, SET_NAMED_PROPERTY);

    napi_value jsInstance = nullptr;
    CHKRP(napi_new_instance(env, jsClass, 0, nullptr, &jsInstance), NEW_INSTANCE);
    CHKRP(napi_set_named_property(env, global, COORDINATION, jsInstance),
        SET_NAMED_PROPERTY);

    JsCoordinationContext *jsContext = nullptr;
    CHKRP(napi_unwrap(env, jsInstance, reinterpret_cast<void**>(&jsContext)), UNWRAP);
    CHKPP(jsContext);
    CHKRP(napi_create_reference(env, jsInstance, 1, &(jsContext->contextRef_)), CREATE_REFERENCE);

    uint32_t refCount = 0;
    status = napi_reference_ref(env, jsContext->contextRef_, &refCount);
    if (status != napi_ok) {
        FI_HILOGE("Reference to nullptr");
        napi_delete_reference(env, jsContext->contextRef_);
        return nullptr;
    }
    return jsInstance;
}

napi_value JsCoordinationContext::JsConstructor(napi_env env, napi_callback_info info)
{
    CALL_INFO_TRACE;
    napi_value thisVar = nullptr;
    void *data = nullptr;
    CHKRP(napi_get_cb_info(env, info, nullptr, nullptr, &thisVar, &data), GET_CB_INFO);

    JsCoordinationContext *jsContext = new (std::nothrow) JsCoordinationContext();
    CHKPP(jsContext);
    napi_status status = napi_wrap(env, thisVar, jsContext, [](napi_env env, void *data, void *hin) {
        FI_HILOGI("Jsvm ends");
        JsCoordinationContext *context = static_cast<JsCoordinationContext*>(data);
        delete context;
    }, nullptr, nullptr);
    if (status != napi_ok) {
        delete jsContext;
        FI_HILOGE("%{public}s failed", std::string(WRAP).c_str());
        auto infoTemp = std::string(__FUNCTION__) + ": " + std::string(WRAP) + " failed";
        napi_throw_error(env, nullptr, infoTemp.c_str());
        return nullptr;
    }
    return thisVar;
}

JsCoordinationContext *JsCoordinationContext::GetInstance(napi_env env)
{
    CALL_INFO_TRACE;
    napi_value napiGlobal = nullptr;
    CHKRP(napi_get_global(env, &napiGlobal), GET_GLOBAL);

    bool result = false;
    CHKRP(napi_has_named_property(env, napiGlobal, COORDINATION, &result), HAS_NAMED_PROPERTY);
    if (!result) {
        FI_HILOGE("Coordination was not found");
        return nullptr;
    }

    napi_handle_scope handleScope = nullptr;
    napi_open_handle_scope(env, &handleScope);
    CHKPP(handleScope);
    napi_value object = nullptr;
    CHKRP_SCOPE(env, napi_get_named_property(env, napiGlobal, COORDINATION, &object),
        GET_NAMED_PROPERTY, handleScope);
    if (object == nullptr) {
        napi_close_handle_scope(env, handleScope);
        FI_HILOGE("object is nullptr");
        return nullptr;
    }

    JsCoordinationContext *instance = nullptr;
    CHKRP_SCOPE(env, napi_unwrap(env, object, reinterpret_cast<void**>(&instance)), UNWRAP, handleScope);
    if (instance == nullptr) {
        napi_close_handle_scope(env, handleScope);
        FI_HILOGE("instance is nullptr");
        return nullptr;
    }
    napi_close_handle_scope(env, handleScope);
    return instance;
}

void JsCoordinationContext::DeclareDeviceCoordinationData(napi_env env, napi_value exports)
{
    napi_value prepare = nullptr;
    CHKRV(napi_create_int32(env, static_cast<int32_t>(CoordinationMessage::PREPARE), &prepare),
        CREATE_INT32);
    napi_value unprepare = nullptr;
    CHKRV(napi_create_int32(env, static_cast<int32_t>(CoordinationMessage::UNPREPARE), &unprepare),
        CREATE_INT32);
    napi_value activate = nullptr;
    CHKRV(napi_create_int32(env, static_cast<int32_t>(CoordinationMessage::ACTIVATE), &activate),
        CREATE_INT32);
    napi_value activateSuccess = nullptr;
    CHKRV(napi_create_int32(env, static_cast<int32_t>(CoordinationMessage::ACTIVATE_SUCCESS), &activateSuccess),
        CREATE_INT32);
    napi_value activateFail = nullptr;
    CHKRV(napi_create_int32(env, static_cast<int32_t>(CoordinationMessage::ACTIVATE_FAIL), &activateFail),
        CREATE_INT32);
    napi_value deactivateSuccess = nullptr;
    CHKRV(napi_create_int32(env, static_cast<int32_t>(CoordinationMessage::DEACTIVATE_SUCCESS), &deactivateSuccess),
        CREATE_INT32);
    napi_value deactivateFail = nullptr;
    CHKRV(napi_create_int32(env, static_cast<int32_t>(CoordinationMessage::DEACTIVATE_FAIL), &deactivateFail),
        CREATE_INT32);
    napi_value sessionClosed = nullptr;
    CHKRV(napi_create_int32(env, static_cast<int32_t>(CoordinationMessage::SESSION_CLOSED), &sessionClosed),
        CREATE_INT32);

    napi_property_descriptor msg[] = {
        DECLARE_NAPI_STATIC_PROPERTY("COOPERATE_PREPARE", prepare),
        DECLARE_NAPI_STATIC_PROPERTY("COOPERATE_UNPREPARE", unprepare),
        DECLARE_NAPI_STATIC_PROPERTY("COOPERATE_ACTIVATE", activate),
        DECLARE_NAPI_STATIC_PROPERTY("COOPERATE_ACTIVATE_SUCCESS", activateSuccess),
        DECLARE_NAPI_STATIC_PROPERTY("COOPERATE_ACTIVATE_FAIL", activateFail),
        DECLARE_NAPI_STATIC_PROPERTY("COOPERATE_DEACTIVATE_SUCCESS", deactivateSuccess),
        DECLARE_NAPI_STATIC_PROPERTY("COOPERATE_DEACTIVATE_FAIL", deactivateFail),
        DECLARE_NAPI_STATIC_PROPERTY("COOPERATE_SESSION_DISCONNECTED", sessionClosed)
    };

    napi_value cooperateMsg = nullptr;
    CHKRV(napi_define_class(env, "CooperateMsg", NAPI_AUTO_LENGTH, EnumClassConstructor, nullptr,
        sizeof(msg) / sizeof(*msg), msg, &cooperateMsg), DEFINE_CLASS);
    CHKRV(napi_set_named_property(env, exports, "CooperateMsg", cooperateMsg), SET_NAMED_PROPERTY);
}

void JsCoordinationContext::DeclareDeviceCooperateData(napi_env env, napi_value exports)
{
    napi_value prepare = nullptr;
    CHKRV(napi_create_int32(env, static_cast<int32_t>(CoordinationMessage::PREPARE), &prepare),
        CREATE_INT32);
    napi_value unprepare = nullptr;
    CHKRV(napi_create_int32(env, static_cast<int32_t>(CoordinationMessage::UNPREPARE), &unprepare),
        CREATE_INT32);
    napi_value activate = nullptr;
    CHKRV(napi_create_int32(env, static_cast<int32_t>(CoordinationMessage::ACTIVATE), &activate),
        CREATE_INT32);
    napi_value activateSuccess = nullptr;
    CHKRV(napi_create_int32(env, static_cast<int32_t>(CoordinationMessage::ACTIVATE_SUCCESS), &activateSuccess),
        CREATE_INT32);
    napi_value activateFail = nullptr;
    CHKRV(napi_create_int32(env, static_cast<int32_t>(CoordinationMessage::ACTIVATE_FAIL), &activateFail),
        CREATE_INT32);
    napi_value deactivateSuccess = nullptr;
    CHKRV(napi_create_int32(env, static_cast<int32_t>(CoordinationMessage::DEACTIVATE_SUCCESS), &deactivateSuccess),
        CREATE_INT32);
    napi_value deactivateFail = nullptr;
    CHKRV(napi_create_int32(env, static_cast<int32_t>(CoordinationMessage::DEACTIVATE_FAIL), &deactivateFail),
        CREATE_INT32);
    napi_value sessionClosed = nullptr;
    CHKRV(napi_create_int32(env, static_cast<int32_t>(CoordinationMessage::SESSION_CLOSED), &sessionClosed),
        CREATE_INT32);

    napi_property_descriptor msg[] = {
        DECLARE_NAPI_STATIC_PROPERTY("COOPERATE_PREPARE", prepare),
        DECLARE_NAPI_STATIC_PROPERTY("COOPERATE_UNPREPARE", unprepare),
        DECLARE_NAPI_STATIC_PROPERTY("COOPERATE_ACTIVATE", activate),
        DECLARE_NAPI_STATIC_PROPERTY("COOPERATE_ACTIVATE_SUCCESS", activateSuccess),
        DECLARE_NAPI_STATIC_PROPERTY("COOPERATE_ACTIVATE_FAILURE", activateFail),
        DECLARE_NAPI_STATIC_PROPERTY("COOPERATE_DEACTIVATE_SUCCESS", deactivateSuccess),
        DECLARE_NAPI_STATIC_PROPERTY("COOPERATE_DEACTIVATE_FAILURE", deactivateFail),
        DECLARE_NAPI_STATIC_PROPERTY("COOPERATE_SESSION_DISCONNECTED", sessionClosed)
    };

    napi_value cooperateMsg = nullptr;
    CHKRV(napi_define_class(env, "CooperateState", NAPI_AUTO_LENGTH, EnumClassConstructor, nullptr,
        sizeof(msg) / sizeof(*msg), msg, &cooperateMsg), DEFINE_CLASS);
    CHKRV(napi_set_named_property(env, exports, "CooperateState", cooperateMsg), SET_NAMED_PROPERTY);
}

void JsCoordinationContext::DeclareDeviceCoordinationInterface(napi_env env, napi_value exports)
{
    napi_property_descriptor functions[] = {
        DECLARE_NAPI_STATIC_FUNCTION("prepare", Prepare),
        DECLARE_NAPI_STATIC_FUNCTION("prepareCooperate", PrepareCooperate),
        DECLARE_NAPI_STATIC_FUNCTION("unprepare", Unprepare),
        DECLARE_NAPI_STATIC_FUNCTION("unprepareCooperate", UnprepareCooperate),
        DECLARE_NAPI_STATIC_FUNCTION("activate", Activate),
        DECLARE_NAPI_STATIC_FUNCTION("activateCooperateWithOptions", ActivateCooperateWithOptions),
        DECLARE_NAPI_STATIC_FUNCTION("activateCooperate", ActivateCooperate),
        DECLARE_NAPI_STATIC_FUNCTION("deactivate", Deactivate),
        DECLARE_NAPI_STATIC_FUNCTION("deactivateCooperate", DeactivateCooperate),
        DECLARE_NAPI_STATIC_FUNCTION("getCrossingSwitchState", GetCrossingSwitchState),
        DECLARE_NAPI_STATIC_FUNCTION("getCooperateSwitchState", GetCooperateSwitchState),
        DECLARE_NAPI_STATIC_FUNCTION("on", On),
        DECLARE_NAPI_STATIC_FUNCTION("off", Off)
    };
    CHKRV(napi_define_properties(env, exports,
        sizeof(functions) / sizeof(*functions), functions), DEFINE_PROPERTIES);
}

napi_value JsCoordinationContext::EnumClassConstructor(napi_env env, napi_callback_info info)
{
    size_t argc = 0;
    napi_value args[1] = { nullptr };
    napi_value result = nullptr;
    void *data = nullptr;
    CHKRP(napi_get_cb_info(env, info, &argc, args, &result, &data), GET_CB_INFO);
    return result;
}
} // namespace DeviceStatus
} // namespace Msdp
} // namespace OHOS
