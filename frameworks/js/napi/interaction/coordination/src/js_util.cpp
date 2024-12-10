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

#include "js_util.h"

#include "devicestatus_define.h"
#include "napi_constants.h"
#include "util_napi_error.h"

#undef LOG_TAG
#define LOG_TAG "JsUtil"

namespace OHOS {
namespace Msdp {
namespace DeviceStatus {
namespace {
inline constexpr std::string_view GET_BOOLEAN { "napi_get_boolean" };
inline constexpr std::string_view COERCE_TO_BOOL { "napi_coerce_to_bool" };
inline constexpr std::string_view CREATE_ERROR { "napi_create_error" };
} // namespace

napi_value JsUtil::GetPrepareInfo(sptr<CallbackInfo> cb)
{
    CHKPP(cb);
    CHKPP(cb->env);
    return GetResult(cb->env, cb->data.prepareResult, cb->data.msgInfo);
}

napi_value JsUtil::GetActivateInfo(sptr<CallbackInfo> cb)
{
    CHKPP(cb);
    CHKPP(cb->env);
    return GetResult(cb->env, cb->data.activateResult, cb->data.msgInfo);
}

napi_value JsUtil::GetDeactivateInfo(sptr<CallbackInfo> cb)
{
    CHKPP(cb);
    CHKPP(cb->env);
    return GetResult(cb->env, cb->data.deactivateResult, cb->data.msgInfo);
}

napi_value JsUtil::GetCrossingSwitchStateInfo(sptr<CallbackInfo> cb)
{
    CHKPP(cb);
    CHKPP(cb->env);
    napi_value ret = nullptr;
    napi_value stateInfo = nullptr;
    CHKRP(napi_create_int32(cb->env, cb->data.coordinationOpened ? 1 : 0, &ret),
        CREATE_INT32);
    CHKRP(napi_coerce_to_bool(cb->env, ret, &stateInfo), COERCE_TO_BOOL);
    return stateInfo;
}

napi_value JsUtil::GetResult(napi_env env, bool result, const CoordinationMsgInfo &msgInfo)
{
    CHKPP(env);
    napi_value object = nullptr;
    if (result) {
        napi_get_undefined(env, &object);
        return object;
    }
    std::string errMsg;
    if (!GetErrMsg(msgInfo, errMsg)) {
        FI_HILOGE("GetErrMsg failed");
        return nullptr;
    }
    int32_t errCode = GetErrCode(msgInfo);
    FI_HILOGI("errCode:%{public}d, msg:%{public}s", errCode, errMsg.c_str());
    napi_value resultCode = nullptr;
    CHKRP(napi_create_int32(env, errCode, &resultCode), CREATE_INT32);
    napi_value resultMessage = nullptr;
    CHKRP(napi_create_string_utf8(env, errMsg.c_str(), NAPI_AUTO_LENGTH, &resultMessage),
        CREATE_STRING_UTF8);
    CHKRP(napi_create_error(env, nullptr, resultMessage, &object), CREATE_ERROR);
    CHKRP(napi_set_named_property(env, object, ERR_CODE.c_str(), resultCode), SET_NAMED_PROPERTY);
    return object;
}

bool JsUtil::GetErrMsg(const CoordinationMsgInfo &msgInfo, std::string &msg)
{
    auto iter = MSG_MAP.find(msgInfo.msg);
    if (iter == MSG_MAP.end()) {
        FI_HILOGE("Error code:%{public}d is not founded in MSG_MAP", static_cast<int32_t> (msgInfo.msg));
        return false;
    }
    msg = iter->second;
    switch (static_cast<CoordinationErrCode>(msgInfo.errCode)) {
        case CoordinationErrCode::COORDINATION_OK: {
            msg += "Everything is fine";
            break;
        }
        case CoordinationErrCode::OPEN_SESSION_FAILED: {
            msg += "Open session failed";
            break;
        }
        case CoordinationErrCode::SEND_PACKET_FAILED: {
            msg += "Send packet failed";
            break;
        }
        case CoordinationErrCode::UNEXPECTED_START_CALL: {
            msg += "Unexpected start call";
            break;
        }
        case CoordinationErrCode::WORKER_THREAD_TIMEOUT: {
            msg += "Worker thread timeout";
            break;
        }
        default:
            msg +="Softbus bind failed";
    }
    return true;
}

int32_t JsUtil::GetErrCode(const CoordinationMsgInfo &msgInfo)
{
    switch (static_cast<CoordinationErrCode>(msgInfo.errCode)) {
        case CoordinationErrCode::OPEN_SESSION_FAILED: {
            return CustomErrCode::OPEN_SESSION_FAILED;
        }
        case CoordinationErrCode::SEND_PACKET_FAILED: {
            return CustomErrCode::SEND_PACKET_FAILED;
        }
        case CoordinationErrCode::UNEXPECTED_START_CALL: {
            return CustomErrCode::UNEXPECTED_START_CALL;
        }
        case CoordinationErrCode::WORKER_THREAD_TIMEOUT: {
            return CustomErrCode::WORKER_THREAD_TIMEOUT;
        }
        default:
            return msgInfo.errCode;
    }
}

napi_value JsUtil::GetCrossingSwitchStateResult(napi_env env, bool result)
{
    CHKPP(env);
    napi_value state = nullptr;
    CHKRP(napi_get_boolean(env, result, &state), GET_BOOLEAN);
    napi_value object = nullptr;
    CHKRP(napi_create_object(env, &object), CREATE_OBJECT);
    CHKRP(napi_set_named_property(env, object, "state", state), SET_NAMED_PROPERTY);
    return object;
}

bool JsUtil::IsSameHandle(napi_env env, napi_value handle, napi_ref ref)
{
    CHKPF(ref);
    napi_handle_scope scope = nullptr;
    napi_open_handle_scope(env, &scope);
    CHKPF(scope);
    napi_value tempHandler = nullptr;
    CHKRF_SCOPE(env, napi_get_reference_value(env, ref, &tempHandler), GET_REFERENCE_VALUE, scope);
    bool isEqual = false;
    CHKRF_SCOPE(env, napi_strict_equals(env, handle, tempHandler, &isEqual), STRICT_EQUALS, scope);
    napi_close_handle_scope(env, scope);
    return isEqual;
}

int32_t JsUtil::GetNamePropertyInt32(const napi_env& env, const napi_value& object,
    const std::string& name, int32_t& ret)
{
    napi_value napiValue = {};
    CHKRF(napi_get_named_property(env, object, name.c_str(), &napiValue), GET_NAMED_PROPERTY);
    napi_valuetype tmpType = napi_undefined;
    if (napi_typeof(env, napiValue, &tmpType) != napi_ok) {
        FI_HILOGE("Call napi_typeof failed");
        return false;
    }
    if (tmpType != napi_number) {
        FI_HILOGE("The value is not number");
        return false;
    }
    CHKRF(napi_get_value_int32(env, napiValue, &ret), GET_VALUE_INT32);
    FI_HILOGE("szy___GetNamePropertyInt32____ret :%{public}d", ret);
    return ret;
}
} // namespace DeviceStatus
} // namespace Msdp
} // namespace OHOS
