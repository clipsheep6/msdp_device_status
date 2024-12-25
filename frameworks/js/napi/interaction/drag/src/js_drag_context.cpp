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

#include "js_drag_context.h"

#include "devicestatus_define.h"
#include "drag_data.h"
#include "napi_constants.h"
#include "util_napi_error.h"

#undef LOG_TAG
#define LOG_TAG "JsDragContext"

namespace OHOS {
namespace Msdp {
namespace DeviceStatus {
namespace {
const char* DRAG_CLASS { "drag_class" };
const char* DRAG { "drag" };
inline constexpr size_t MAX_STRING_LEN { 1024 };
inline constexpr size_t MAX_PKG_NAME_LEN { 128 };
inline constexpr std::string_view GET_VALUE_BOOL { "napi_get_value_bool" };
} // namespace

JsDragContext::JsDragContext()
    : mgr_(std::make_shared<JsDragManager>()) {}

JsDragContext::~JsDragContext()
{
    std::lock_guard<std::mutex> guard(mutex_);
    if (mgr_ != nullptr) {
        mgr_->ResetEnv();
        mgr_ = nullptr;
    }
}

std::shared_ptr<JsDragManager> JsDragContext::GetJsDragMgr()
{
    std::lock_guard<std::mutex> guard(mutex_);
    return mgr_;
}

napi_value JsDragContext::CreateInstance(napi_env env)
{
    CALL_INFO_TRACE;
    napi_value global = nullptr;
    CHKRP(napi_get_global(env, &global), GET_GLOBAL);

    constexpr char className[] = "JsDragContext";
    napi_value jsClass = nullptr;
    napi_property_descriptor desc[] = {};
    napi_status status = napi_define_class(env, className, sizeof(className),
        JsDragContext::JsConstructor, nullptr, sizeof(desc) / sizeof(desc[0]), nullptr, &jsClass);
    CHKRP(status, DEFINE_CLASS);

    status = napi_set_named_property(env, global, DRAG_CLASS, jsClass);
    CHKRP(status, SET_NAMED_PROPERTY);

    napi_value jsInstance = nullptr;
    CHKRP(napi_new_instance(env, jsClass, 0, nullptr, &jsInstance), NEW_INSTANCE);
    CHKRP(napi_set_named_property(env, global, DRAG, jsInstance),
        SET_NAMED_PROPERTY);

    JsDragContext *jsContext = nullptr;
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

napi_value JsDragContext::JsConstructor(napi_env env, napi_callback_info info)
{
    CALL_INFO_TRACE;
    napi_value thisVar = nullptr;
    void *data = nullptr;
    CHKRP(napi_get_cb_info(env, info, nullptr, nullptr, &thisVar, &data), GET_CB_INFO);

    JsDragContext *jsContext = new (std::nothrow) JsDragContext();
    CHKPP(jsContext);
    napi_status status = napi_wrap(env, thisVar, jsContext, [](napi_env env, void *data, void *hin) {
        FI_HILOGI("Jsvm ends");
        JsDragContext *context = static_cast<JsDragContext*>(data);
        delete context;
    }, nullptr, nullptr);
    if (status != napi_ok) {
        delete jsContext;
        FI_HILOGE("%{public}s failed", std::string(WRAP).c_str());
        auto errInfoTemp = std::string(__FUNCTION__) + ": " + std::string(WRAP) + " failed";
        napi_throw_error(env, nullptr, errInfoTemp.c_str());
        return nullptr;
    }
    return thisVar;
}

JsDragContext *JsDragContext::GetInstance(napi_env env)
{
    CALL_INFO_TRACE;
    napi_value global = nullptr;
    CHKRP(napi_get_global(env, &global), GET_GLOBAL);

    bool result = false;
    CHKRP(napi_has_named_property(env, global, DRAG, &result), HAS_NAMED_PROPERTY);
    if (!result) {
        FI_HILOGE("Drag was not found");
        return nullptr;
    }

    napi_handle_scope scope = nullptr;
    napi_open_handle_scope(env, &scope);
    if (scope == nullptr) {
        FI_HILOGE("scope is nullptr");
        return nullptr;
    }
    napi_value object = nullptr;
    CHKRP_SCOPE(env, napi_get_named_property(env, global, DRAG, &object), GET_NAMED_PROPERTY, scope);
    if (object == nullptr) {
        napi_close_handle_scope(env, scope);
        FI_HILOGE("object is nullptr");
        return nullptr;
    }

    JsDragContext *instance = nullptr;
    CHKRP_SCOPE(env, napi_unwrap(env, object, reinterpret_cast<void**>(&instance)), UNWRAP, scope);
    if (instance == nullptr) {
        napi_close_handle_scope(env, scope);
        FI_HILOGE("instance is nullptr");
        return nullptr;
    }
    napi_close_handle_scope(env, scope);
    return instance;
}

napi_value JsDragContext::On(napi_env env, napi_callback_info info)
{
    CALL_INFO_TRACE;
    size_t argc = 2;
    napi_value argv[2] = { nullptr };
    CHKRP(napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr), GET_CB_INFO);
    if (argc < 2) {
        THROWERR_CUSTOM(env, COMMON_PARAMETER_ERROR, "Wrong number of parameters");
        return nullptr;
    }
    JsDragContext *jsDev = JsDragContext::GetInstance(env);
    CHKPP(jsDev);
    auto jsDragMgr = jsDev->GetJsDragMgr();
    CHKPP(jsDragMgr);
    if (!UtilNapi::TypeOf(env, argv[0], napi_string)) {
        THROWERR(env, COMMON_PARAMETER_ERROR, "type", "string");
        return nullptr;
    }
    char type[MAX_STRING_LEN] = { 0 };
    size_t strLength = 0;
    CHKRP(napi_get_value_string_utf8(env, argv[0], type, sizeof(type), &strLength), CREATE_STRING_UTF8);
    if ((DRAG_TYPE.compare(type)) != 0) {
        THROWERR_CUSTOM(env, COMMON_PARAMETER_ERROR, "Type must be drag");
        return nullptr;
    }
    if (!UtilNapi::TypeOf(env, argv[1], napi_function)) {
        THROWERR(env, COMMON_PARAMETER_ERROR, "callback", "function");
        return nullptr;
    }
    jsDragMgr->RegisterListener(env, argv[1]);
    return nullptr;
}

napi_value JsDragContext::Off(napi_env env, napi_callback_info info)
{
    CALL_INFO_TRACE;
    size_t argc = 2;
    napi_value argv[2] = { nullptr };
    CHKRP(napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr), GET_CB_INFO);

    JsDragContext *jsDev = JsDragContext::GetInstance(env);
    CHKPP(jsDev);
    auto jsDragMgr = jsDev->GetJsDragMgr();
    CHKPP(jsDragMgr);
    if ((argc == 0) || (!UtilNapi::TypeOf(env, argv[0], napi_string))) {
        THROWERR(env, COMMON_PARAMETER_ERROR, "type", "string");
        return nullptr;
    }
    char type[MAX_STRING_LEN] = { 0 };
    size_t length = 0;
    CHKRP(napi_get_value_string_utf8(env, argv[0], type, sizeof(type), &length), CREATE_STRING_UTF8);
    if ((DRAG_TYPE.compare(type)) != 0) {
        THROWERR_CUSTOM(env, COMMON_PARAMETER_ERROR, "Type must be drag");
        return nullptr;
    }
    if (argc == 1) {
        jsDragMgr->UnregisterListener(env);
        return nullptr;
    }
    if (UtilNapi::TypeOf(env, argv[1], napi_undefined) || UtilNapi::TypeOf(env, argv[1], napi_null)) {
        jsDragMgr->UnregisterListener(env);
        return nullptr;
    }
    if (!UtilNapi::TypeOf(env, argv[1], napi_function)) {
        THROWERR(env, COMMON_PARAMETER_ERROR, "callback", "function");
        return nullptr;
    }
    jsDragMgr->UnregisterListener(env, argv[1]);
    return nullptr;
}

napi_value JsDragContext::GetDataSummary(napi_env env, napi_callback_info info)
{
    CALL_INFO_TRACE;
    JsDragContext *jsDev = JsDragContext::GetInstance(env);
    CHKPP(jsDev);
    auto jsDragMgr = jsDev->GetJsDragMgr();
    CHKPP(jsDragMgr);
    return jsDragMgr->GetDataSummary(env);
}

napi_value JsDragContext::SetDragSwitchState(napi_env env, napi_callback_info info)
{
    CALL_INFO_TRACE;
    size_t argc = ONE_PARAM;
    napi_value argv[ONE_PARAM] = { nullptr };
    CHKRP(napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr), GET_CB_INFO);

    JsDragContext *jsDev = JsDragContext::GetInstance(env);
    CHKPP(jsDev);
    auto jsDragMgr = jsDev->GetJsDragMgr();
    CHKPP(jsDragMgr);
    if (argc < ONE_PARAM) {
        THROWERR_CUSTOM(env, COMMON_PARAMETER_ERROR, "Wrong number of parameters");
        return nullptr;
    }
    if (!UtilNapi::TypeOf(env, argv[ZERO_PARAM], napi_boolean)) {
        THROWERR(env, COMMON_PARAMETER_ERROR, "enable", "boolean");
        return nullptr;
    }
    bool enable = false;
    CHKRP(napi_get_value_bool(env, argv[ZERO_PARAM], &enable), GET_VALUE_BOOL);
    jsDragMgr->SetDragSwitchState(env, enable);
    return nullptr;
}

napi_value JsDragContext::SetAppDragSwitchState(napi_env env, napi_callback_info info)
{
    CALL_INFO_TRACE;
    size_t argc = TWO_PARAM;
    napi_value argv[TWO_PARAM] = { nullptr };
    CHKRP(napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr), GET_CB_INFO);

    JsDragContext *jsDev = JsDragContext::GetInstance(env);
    CHKPP(jsDev);
    auto jsDragMgr = jsDev->GetJsDragMgr();
    CHKPP(jsDragMgr);
    if (argc < TWO_PARAM) {
        THROWERR_CUSTOM(env, COMMON_PARAMETER_ERROR, "Wrong number of parameters");
        return nullptr;
    }
    if (!UtilNapi::TypeOf(env, argv[ZERO_PARAM], napi_boolean)) {
        THROWERR(env, COMMON_PARAMETER_ERROR, "enable", "boolean");
        return nullptr;
    }
    if (!UtilNapi::TypeOf(env, argv[ONE_PARAM], napi_string)) {
        THROWERR(env, COMMON_PARAMETER_ERROR, "pkgName", "string");
        return nullptr;
    }
    bool enable = false;
    CHKRP(napi_get_value_bool(env, argv[ZERO_PARAM], &enable), GET_VALUE_BOOL);
    char param[MAX_STRING_LEN] = { 0 };
    size_t length = 0;
    CHKRP(napi_get_value_string_utf8(env, argv[ONE_PARAM], param, sizeof(param), &length), CREATE_STRING_UTF8);
    if (length <= 0 || length > MAX_PKG_NAME_LEN) {
        FI_HILOGE("Invalid pkgName length:%{public}zu", length);
        THROWERR_CUSTOM(env, COMMON_PARAMETER_ERROR, "Invalid pkgName length");
        return nullptr;
    }
    std::string pkgName = param;
    jsDragMgr->SetAppDragSwitchState(env, enable, pkgName);
    return nullptr;
}

void JsDragContext::DeclareDragData(napi_env env, napi_value exports)
{
    napi_value startMsg = nullptr;
    CHKRV(napi_create_int32(env, static_cast<int32_t>(DragState::START), &startMsg),
        CREATE_INT32);
    napi_value stopMsg = nullptr;
    CHKRV(napi_create_int32(env, static_cast<int32_t>(DragState::STOP), &stopMsg),
        CREATE_INT32);
    napi_value cancelMsg = nullptr;
    CHKRV(napi_create_int32(env, static_cast<int32_t>(DragState::CANCEL), &cancelMsg),
        CREATE_INT32);

    napi_property_descriptor msg[] = {
        DECLARE_NAPI_STATIC_PROPERTY("MSG_DRAG_STATE_START", startMsg),
        DECLARE_NAPI_STATIC_PROPERTY("MSG_DRAG_STATE_STOP", stopMsg),
        DECLARE_NAPI_STATIC_PROPERTY("MSG_DRAG_STATE_CANCEL", cancelMsg)
    };

    napi_value eventMsg = nullptr;
    CHKRV(napi_define_class(env, "DragState", NAPI_AUTO_LENGTH, EnumClassConstructor, nullptr,
        sizeof(msg) / sizeof(*msg), msg, &eventMsg), DEFINE_CLASS);
    CHKRV(napi_set_named_property(env, exports, "DragState", eventMsg), SET_NAMED_PROPERTY);
}

void JsDragContext::DeclareDragInterface(napi_env env, napi_value exports)
{
    napi_property_descriptor functions[] = {
        DECLARE_NAPI_STATIC_FUNCTION("on", On),
        DECLARE_NAPI_STATIC_FUNCTION("off", Off),
        DECLARE_NAPI_STATIC_FUNCTION("getDataSummary", GetDataSummary),
        DECLARE_NAPI_STATIC_FUNCTION("setDragSwitchState", SetDragSwitchState),
        DECLARE_NAPI_STATIC_FUNCTION("setAppDragSwitchState", SetAppDragSwitchState)
    };
    CHKRV(napi_define_properties(env, exports,
        sizeof(functions) / sizeof(*functions), functions), DEFINE_PROPERTIES);
}

napi_value JsDragContext::EnumClassConstructor(napi_env env, napi_callback_info info)
{
    size_t argc = 0;
    napi_value args[1] = { nullptr };
    napi_value result = nullptr;
    void *data = nullptr;
    CHKRP(napi_get_cb_info(env, info, &argc, args, &result, &data), GET_CB_INFO);
    return result;
}

napi_value JsDragContext::Export(napi_env env, napi_value exports)
{
    CALL_INFO_TRACE;
    auto instance = CreateInstance(env);
    CHKPP(instance);
    DeclareDragData(env, exports);
    DeclareDragInterface(env, exports);
    return exports;
}
} // namespace DeviceStatus
} // namespace Msdp
} // namespace OHOS
