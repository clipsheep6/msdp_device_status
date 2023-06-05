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

namespace OHOS {
namespace Msdp {
namespace DeviceStatus {
namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, MSDP_DOMAIN_ID, "JsCoordinationContext" };
const char* g_coordinationClass = "Coordination_class";
const char* g_coordination = "Coordination";
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
    if (instance == nullptr) {
        FI_HILOGE("instance is nullptr");
        return nullptr;
    }
    DeclareDeviceCoordinationInterface(env, exports);
    DeclareDeviceCoordinationData(env, exports);
    return exports;
}

napi_value JsCoordinationContext::Prepare(napi_env env, napi_callback_info info)
{
    CALL_INFO_TRACE;
    size_t argc = ONE_PARAM;
    napi_value argv[ONE_PARAM] = { nullptr };
    CHKRP(napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr), GET_CB_INFO);

    JsCoordinationContext *jsDev = JsCoordinationContext::GetInstance(env);
    CHKPP(jsDev);
    std::shared_ptr<JsCoordinationManager> jsCoordinationMgr = jsDev->GetJsCoordinationMgr();
    CHKPP(jsCoordinationMgr);
    if (argc == ZERO_PARAM) {
        return jsCoordinationMgr->Prepare(env);
    }
    if (!UtilNapi::TypeOf(env, argv[ZERO_PARAM], napi_function)) {
        THROWERR(env, COMMON_PARAMETER_ERROR, "callback", "function");
        return nullptr;
    }
    return jsCoordinationMgr->Prepare(env, argv[ZERO_PARAM]);
}

napi_value JsCoordinationContext::Unprepare(napi_env env, napi_callback_info info)
{
    CALL_INFO_TRACE;
    size_t argc = ONE_PARAM;
    napi_value argv[ONE_PARAM] = { nullptr };
    CHKRP(napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr), GET_CB_INFO);

    JsCoordinationContext *jsDev = JsCoordinationContext::GetInstance(env);
    CHKPP(jsDev);
    std::shared_ptr<JsCoordinationManager> jsCoordinationMgr = jsDev->GetJsCoordinationMgr();
    CHKPP(jsCoordinationMgr);
    if (argc == ZERO_PARAM) {
        return jsCoordinationMgr->Unprepare(env);
    }
    if (!UtilNapi::TypeOf(env, argv[ZERO_PARAM], napi_function)) {
        THROWERR(env, COMMON_PARAMETER_ERROR, "callback", "function");
        return nullptr;
    }
    return jsCoordinationMgr->Unprepare(env, argv[ZERO_PARAM]);
}

napi_value JsCoordinationContext::Activate(napi_env env, napi_callback_info info)
{
    CALL_INFO_TRACE;
    size_t argc = THREE_PARAM;
    napi_value argv[THREE_PARAM] = { nullptr };
    CHKRP(napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr), GET_CB_INFO);

    if (argc < TWO_PARAM) {
        THROWERR_CUSTOM(env, COMMON_PARAMETER_ERROR, "Wrong number of parameters");
        return nullptr;
    }
    if (!UtilNapi::TypeOf(env, argv[ZERO_PARAM], napi_string)) {
        THROWERR(env, COMMON_PARAMETER_ERROR, "targetNetworkId", "string");
        return nullptr;
    }
    if (!UtilNapi::TypeOf(env, argv[ONE_PARAM], napi_number)) {
        THROWERR(env, COMMON_PARAMETER_ERROR, "inputDeviceId", "number");
        return nullptr;
    }
    char remoteNetworkId[MAX_STRING_LEN] = { 0 };
    int32_t startDeviceId = 0;
    size_t length = 0;
    CHKRP(napi_get_value_string_utf8(env, argv[ZERO_PARAM], remoteNetworkId,
        sizeof(remoteNetworkId), &length), GET_VALUE_STRING_UTF8);
    CHKRP(napi_get_value_int32(env, argv[ONE_PARAM], &startDeviceId), GET_VALUE_INT32);

    JsCoordinationContext *jsDev = JsCoordinationContext::GetInstance(env);
    CHKPP(jsDev);
    std::shared_ptr<JsCoordinationManager> jsCoordinationMgr = jsDev->GetJsCoordinationMgr();
    CHKPP(jsCoordinationMgr);
    if (argc == TWO_PARAM) {
        return jsCoordinationMgr->Activate(env, remoteNetworkId, startDeviceId);
    }
    if (!UtilNapi::TypeOf(env, argv[TWO_PARAM], napi_function)) {
        THROWERR(env, COMMON_PARAMETER_ERROR, "callback", "function");
        return nullptr;
    }
    return jsCoordinationMgr->Activate(env, std::string(remoteNetworkId), startDeviceId, argv[TWO_PARAM]);
}

napi_value JsCoordinationContext::Deactivate(napi_env env, napi_callback_info info)
{
    CALL_INFO_TRACE;
    size_t argc = TWO_PARAM;
    napi_value argv[TWO_PARAM] = { nullptr };
    CHKRP(napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr), GET_CB_INFO);

    if (argc == ZERO_PARAM) {
        THROWERR_CUSTOM(env, COMMON_PARAMETER_ERROR, "Wrong number of parameters");
        return nullptr;
    }
    if (!UtilNapi::TypeOf(env, argv[ZERO_PARAM], napi_boolean)) {
        THROWERR(env, COMMON_PARAMETER_ERROR, "isUnchained", "boolean");
        return nullptr;
    }
    bool isUnchained = false;
    CHKRP(napi_get_value_bool(env, argv[ZERO_PARAM], &isUnchained), GET_VALUE_BOOL);

    JsCoordinationContext *jsDev = JsCoordinationContext::GetInstance(env);
    CHKPP(jsDev);
    std::shared_ptr<JsCoordinationManager> jsCoordinationMgr = jsDev->GetJsCoordinationMgr();
    CHKPP(jsCoordinationMgr);
    if (argc == ONE_PARAM) {
        return jsCoordinationMgr->Deactivate(env, isUnchained);
    }
    if (!UtilNapi::TypeOf(env, argv[ONE_PARAM], napi_function)) {
        THROWERR(env, COMMON_PARAMETER_ERROR, "callback", "function");
        return nullptr;
    }
    return jsCoordinationMgr->Deactivate(env, isUnchained, argv[ONE_PARAM]);
}

napi_value JsCoordinationContext::GetCrossingSwitchState(napi_env env, napi_callback_info info)
{
    CALL_INFO_TRACE;
    size_t argc = TWO_PARAM;
    napi_value argv[TWO_PARAM] = { nullptr };
    CHKRP(napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr), GET_CB_INFO);

    if (argc == ZERO_PARAM) {
        THROWERR_CUSTOM(env, COMMON_PARAMETER_ERROR, "Wrong number of parameters");
        return nullptr;
    }
    if (!UtilNapi::TypeOf(env, argv[ZERO_PARAM], napi_string)) {
        THROWERR(env, COMMON_PARAMETER_ERROR, "networkId", "string");
        return nullptr;
    }
    char networkId[MAX_STRING_LEN] = { 0 };
    size_t length = 0;
    CHKRP(napi_get_value_string_utf8(env, argv[ZERO_PARAM], networkId,
        sizeof(networkId), &length), GET_VALUE_STRING_UTF8);
    std::string networkId_ = networkId;

    JsCoordinationContext *jsDev = JsCoordinationContext::GetInstance(env);
    CHKPP(jsDev);
    std::shared_ptr<JsCoordinationManager> jsCoordinationMgr = jsDev->GetJsCoordinationMgr();
    CHKPP(jsCoordinationMgr);
    if (argc == ONE_PARAM) {
        return jsCoordinationMgr->GetCrossingSwitchState(env, networkId_);
    }
    if (!UtilNapi::TypeOf(env, argv[ONE_PARAM], napi_function)) {
        THROWERR(env, COMMON_PARAMETER_ERROR, "callback", "function");
        return nullptr;
    }
    return jsCoordinationMgr->GetCrossingSwitchState(env, networkId_, argv[ONE_PARAM]);
}

napi_value JsCoordinationContext::On(napi_env env, napi_callback_info info)
{
    CALL_INFO_TRACE;
    size_t argc = TWO_PARAM;
    napi_value argv[TWO_PARAM] = { nullptr };
    CHKRP(napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr), GET_CB_INFO);

    if (argc == ZERO_PARAM) {
        THROWERR_CUSTOM(env, COMMON_PARAMETER_ERROR, "Wrong number of parameters");
        return nullptr;
    }
    if (!UtilNapi::TypeOf(env, argv[ZERO_PARAM], napi_string)) {
        THROWERR(env, COMMON_PARAMETER_ERROR, "type", "string");
        return nullptr;
    }
    char type[MAX_STRING_LEN] = { 0 };
    size_t length = 0;
    CHKRP(napi_get_value_string_utf8(env, argv[ZERO_PARAM], type, sizeof(type), &length), GET_VALUE_STRING_UTF8);
    if ((COOPERATE.compare(type)) != 0) {
        THROWERR_CUSTOM(env, COMMON_PARAMETER_ERROR, "Type must be cooperate");
        return nullptr;
    }
    JsCoordinationContext *jsDev = JsCoordinationContext::GetInstance(env);
    CHKPP(jsDev);
    std::shared_ptr<JsCoordinationManager> jsCoordinationMgr = jsDev->GetJsCoordinationMgr();
    CHKPP(jsCoordinationMgr);
    if (!UtilNapi::TypeOf(env, argv[ONE_PARAM], napi_function)) {
        THROWERR(env, COMMON_PARAMETER_ERROR, "callback", "function");
        return nullptr;
    }
    jsCoordinationMgr->RegisterListener(env, type, argv[ONE_PARAM]);
    return nullptr;
}

napi_value JsCoordinationContext::Off(napi_env env, napi_callback_info info)
{
    CALL_INFO_TRACE;
    size_t argc = TWO_PARAM;
    napi_value argv[TWO_PARAM] = { nullptr };
    CHKRP(napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr), GET_CB_INFO);

    if (argc == ZERO_PARAM) {
        THROWERR_CUSTOM(env, COMMON_PARAMETER_ERROR, "Wrong number of parameters");
        return nullptr;
    }
    if (!UtilNapi::TypeOf(env, argv[ZERO_PARAM], napi_string)) {
        THROWERR(env, COMMON_PARAMETER_ERROR, "type", "string");
        return nullptr;
    }
    char type[MAX_STRING_LEN] = { 0 };
    size_t length = 0;
    CHKRP(napi_get_value_string_utf8(env, argv[ZERO_PARAM], type, sizeof(type), &length), GET_VALUE_STRING_UTF8);
    std::string type_ = type;

    JsCoordinationContext *jsDev = JsCoordinationContext::GetInstance(env);
    CHKPP(jsDev);
    std::shared_ptr<JsCoordinationManager> jsCoordinationMgr = jsDev->GetJsCoordinationMgr();
    CHKPP(jsCoordinationMgr);
    if (argc == ONE_PARAM) {
        jsCoordinationMgr->UnregisterListener(env, type_);
        return nullptr;
    }
    if (!UtilNapi::TypeOf(env, argv[ONE_PARAM], napi_function)) {
        THROWERR(env, COMMON_PARAMETER_ERROR, "callback", "function");
        return nullptr;
    }
    jsCoordinationMgr->UnregisterListener(env, type_, argv[ONE_PARAM]);
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

    status = napi_set_named_property(env, global, g_coordinationClass, jsClass);
    CHKRP(status, SET_NAMED_PROPERTY);

    napi_value jsInstance = nullptr;
    CHKRP(napi_new_instance(env, jsClass, 0, nullptr, &jsInstance), NEW_INSTANCE);
    CHKRP(napi_set_named_property(env, global, g_coordination, jsInstance),
        SET_NAMED_PROPERTY);

    JsCoordinationContext *jsContext = nullptr;
    CHKRP(napi_unwrap(env, jsInstance, reinterpret_cast<void**>(&jsContext)), UNWRAP);
    CHKPP(jsContext);
    CHKRP(napi_create_reference(env, jsInstance, 1, &(jsContext->contextRef_)), CREATE_REFERENCE);

    uint32_t refCount = 0;
    status = napi_reference_ref(env, jsContext->contextRef_, &refCount);
    if (status != napi_ok) {
        FI_HILOGE("ref is nullptr");
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
        FI_HILOGI("jsvm ends");
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
    napi_value global = nullptr;
    CHKRP(napi_get_global(env, &global), GET_GLOBAL);

    bool result = false;
    CHKRP(napi_has_named_property(env, global, g_coordination, &result), HAS_NAMED_PROPERTY);
    if (!result) {
        FI_HILOGE("Coordination was not found");
        return nullptr;
    }

    napi_handle_scope scope = nullptr;
    napi_open_handle_scope(env, &scope);
    if (scope == nullptr) {
        FI_HILOGE("scope is nullptr");
        return nullptr;
    }
    napi_value object = nullptr;
    CHKRP_SCOPE(env, napi_get_named_property(env, global, g_coordination, &object), GET_NAMED_PROPERTY, scope);
    if (object == nullptr) {
        napi_close_handle_scope(env, scope);
        FI_HILOGE("object is nullptr");
        return nullptr;
    }

    JsCoordinationContext *instance = nullptr;
    CHKRP_SCOPE(env, napi_unwrap(env, object, reinterpret_cast<void**>(&instance)), UNWRAP, scope);
    if (instance == nullptr) {
        napi_close_handle_scope(env, scope);
        FI_HILOGE("instance is nullptr");
        return nullptr;
    }
    napi_close_handle_scope(env, scope);
    return instance;
}

void JsCoordinationContext::DeclareDeviceCoordinationInterface(napi_env env, napi_value exports)
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
        DECLARE_NAPI_STATIC_PROPERTY("COOPERATE_SESSION_DISCONNECTED", sessionClosed),
    };

    napi_value cooperateMsg = nullptr;
    CHKRV(napi_define_class(env, "CooperateMsg", NAPI_AUTO_LENGTH, EnumClassConstructor, nullptr,
        sizeof(msg) / sizeof(*msg), msg, &cooperateMsg), DEFINE_CLASS);
    CHKRV(napi_set_named_property(env, exports, "CooperateMsg", cooperateMsg), SET_NAMED_PROPERTY);
}

void JsCoordinationContext::DeclareDeviceCoordinationData(napi_env env, napi_value exports)
{
    napi_property_descriptor functions[] = {
        DECLARE_NAPI_STATIC_FUNCTION("prepare", Prepare),
        DECLARE_NAPI_STATIC_FUNCTION("unprepare", Unprepare),
        DECLARE_NAPI_STATIC_FUNCTION("activate", Activate),
        DECLARE_NAPI_STATIC_FUNCTION("deactivate", Deactivate),
        DECLARE_NAPI_STATIC_FUNCTION("getCrossingSwitchState", GetCrossingSwitchState),
        DECLARE_NAPI_STATIC_FUNCTION("on", On),
        DECLARE_NAPI_STATIC_FUNCTION("off", Off),
    };
    CHKRV(napi_define_properties(env, exports,
        sizeof(functions) / sizeof(*functions), functions), DEFINE_PROPERTIES);
}

napi_value JsCoordinationContext::EnumClassConstructor(napi_env env, napi_callback_info info)
{
    size_t argc = 0;
    napi_value args[1] = {};
    napi_value result = nullptr;
    void *data = nullptr;
    CHKRP(napi_get_cb_info(env, info, &argc, args, &result, &data), GET_CB_INFO);
    return result;
}
} // namespace DeviceStatus
} // namespace Msdp
} // namespace OHOS
