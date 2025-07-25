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

#include "motion_napi.h"

#include <mutex>
#include "devicestatus_define.h"
#include "fi_log.h"
#ifdef MOTION_ENABLE
#include "motion_client.h"
#include "napi_event_utils.h"
#endif
#include "motion_napi_error.h"

#undef LOG_TAG
#define LOG_TAG "DeviceMotionNapi"

namespace OHOS {
namespace Msdp {
namespace {
#ifdef MOTION_ENABLE
auto &g_motionClient = MotionClient::GetInstance();
constexpr int32_t PERMISSION_DENIED = 201;
static constexpr uint8_t ARG_1 = 1;
constexpr int32_t HOLDING_HAND_FEATURE_DISABLE = 11;
constexpr int32_t EVENT_NOT_SUPPORT = -200;
#endif
static constexpr uint8_t ARG_0 = 0;
static constexpr uint8_t ARG_2 = 2;
constexpr int32_t INVALID_MOTION_TYPE = -1;
constexpr size_t MAX_ARG_STRING_LEN = 512;
constexpr int32_t MOTION_TYPE_OPERATING_HAND = 3601;
constexpr int32_t MOTION_TYPE_STAND = 3602;
constexpr int32_t MOTION_TYPE_REMOTE_PHOTO = 3604;
constexpr int32_t MOTION_TYPE_HOLDING_HAND_STATUS = 3605;
constexpr int32_t BASE_HAND = 0;
constexpr int32_t LEFT_HAND = 1;
constexpr int32_t RIGHT_HAND = 2;
enum HoldPostureStatus : int32_t {
    NOT_HELD = 0,
    LEFT_HAND_HELD,
    RIGHT_HAND_HELD,
    BOTH_HAND_HELD,
    UNKNOWN = 16,
};
const std::vector<std::string> EXPECTED_SUB_ARG_TYPES = { "string", "function" };
const std::vector<std::string> EXPECTED_UNSUB_ONE_ARG_TYPES = { "string" };
const std::vector<std::string> EXPECTED_UNSUB_TWO_ARG_TYPES = { "string", "function" };
const std::map<const std::string, int32_t> MOTION_TYPE_MAP = {
    { "operatingHandChanged", MOTION_TYPE_OPERATING_HAND },
    { "steadyStandingDetect", MOTION_TYPE_STAND },
    { "remotePhotoStandingDetect", MOTION_TYPE_REMOTE_PHOTO },
    { "holdingHandChanged", MOTION_TYPE_HOLDING_HAND_STATUS },
};
MotionNapi *g_motionObj = nullptr;
} // namespace

std::mutex g_mutex;

#ifdef MOTION_ENABLE
void MotionCallback::OnMotionChanged(const MotionEvent &event)
{
    FI_HILOGD("Enter");
    std::lock_guard<std::mutex> guard(g_mutex);
    auto* data = new (std::nothrow) MotionEvent();
    CHKPV(data);
    data->type = event.type;
    data->status = event.status;
    data->dataLen = event.dataLen;
    data->data = event.data;

    auto task = [data]() {
        FI_HILOGI("Execute lamdba");
        EmitOnEvent(data);
    };
    if (napi_status::napi_ok != napi_send_event(env_, task, napi_eprio_immediate)) {
        FI_HILOGE("Failed to SendEvent");
        delete data;
    }
    FI_HILOGD("Exit");
}

void MotionCallback::EmitOnEvent(MotionEvent* data)
{
    if (data == nullptr) {
        FI_HILOGE("data is nullptr");
        return;
    }

    if (g_motionObj == nullptr) {
        FI_HILOGE("Failed to get g_motionObj");
        delete data;
        return;
    }
    g_motionObj->OnEventOperatingHand(data->type, 1, *data);
    delete data;
}
#endif

MotionNapi::MotionNapi(napi_env env, napi_value thisVar) : MotionEventNapi(env, thisVar)
{
    env_ = env;
}

MotionNapi::~MotionNapi()
{}

int32_t MotionNapi::GetMotionType(const std::string &type)
{
    FI_HILOGD("Enter");
    auto iter = MOTION_TYPE_MAP.find(type);
    if (iter == MOTION_TYPE_MAP.end()) {
        FI_HILOGD("Don't find this type");
        return INVALID_MOTION_TYPE;
    }
    FI_HILOGD("Exit");
    return iter->second;
}

#ifdef MOTION_ENABLE
bool MotionNapi::SubscribeCallback(napi_env env, int32_t type)
{
    if (g_motionObj == nullptr) {
        ThrowMotionErr(env, SUBSCRIBE_EXCEPTION, "g_motionObj is nullptr");
        return false;
    }

    auto iter = g_motionObj->callbacks_.find(type);
    if (iter == g_motionObj->callbacks_.end()) {
        FI_HILOGD("Don't find callback, to create");
        sptr<IMotionCallback> callback = new (std::nothrow) MotionCallback(env);
        int32_t ret = g_motionClient.SubscribeCallback(type, callback);
        if (ret == RET_OK) {
            g_motionObj->callbacks_.insert(std::make_pair(type, callback));
            return true;
        }

        if (ret == PERMISSION_DENIED) {
            FI_HILOGE("failed to subscribe");
            ThrowMotionErr(env, PERMISSION_EXCEPTION, "Permission denined");
            return false;
        } else if (ret == DEVICE_EXCEPTION || ret == HOLDING_HAND_FEATURE_DISABLE) {
            FI_HILOGE("failed to subscribe");
            ThrowMotionErr(env, DEVICE_EXCEPTION, "Device not support");
            return false;
        } else {
            FI_HILOGE("failed to subscribe");
            ThrowMotionErr(env, SUBSCRIBE_EXCEPTION, "Subscribe failed");
            return false;
        }
    }
    return true;
}

bool MotionNapi::UnSubscribeCallback(napi_env env, int32_t type)
{
    if (g_motionObj == nullptr) {
        ThrowMotionErr(env, UNSUBSCRIBE_EXCEPTION, "g_motionObj is nullptr");
        return false;
    }

    if (g_motionObj->CheckEvents(type)) {
        auto iter = g_motionObj->callbacks_.find(type);
        if (iter == g_motionObj->callbacks_.end()) {
            FI_HILOGE("faild to find callback");
            ThrowMotionErr(env, UNSUBSCRIBE_EXCEPTION, "Unsubscribe failed");
            return false;
        }
        int32_t ret = g_motionClient.UnsubscribeCallback(type, iter->second);
        if (ret == RET_OK) {
            g_motionObj->callbacks_.erase(iter);
            return true;
        }

        if (ret == PERMISSION_DENIED) {
            FI_HILOGE("failed to unsubscribe");
            ThrowMotionErr(env, PERMISSION_EXCEPTION, "Permission denined");
            return false;
        } else if (ret == DEVICE_EXCEPTION || ret == HOLDING_HAND_FEATURE_DISABLE) {
            FI_HILOGE("failed to unsubscribe");
            ThrowMotionErr(env, DEVICE_EXCEPTION, "Device not support");
            return false;
        } else {
            FI_HILOGE("failed to unsubscribe");
            ThrowMotionErr(env, UNSUBSCRIBE_EXCEPTION, "Unsubscribe failed");
            return false;
        }
    }
    return false;
}
#endif

bool MotionNapi::ConstructMotion(napi_env env, napi_value jsThis) __attribute__((no_sanitize("cfi")))
{
    std::lock_guard<std::mutex> guard(g_mutex);
    if (g_motionObj == nullptr) {
        g_motionObj = new (std::nothrow) MotionNapi(env, jsThis);
        if (g_motionObj == nullptr) {
            FI_HILOGE("faild to get g_motionObj");
            return false;
        }
        napi_status status = napi_wrap(env, jsThis, reinterpret_cast<void *>(g_motionObj),
            [](napi_env env, void *data, void *hint) {
                (void)env;
                (void)hint;
                if (data != nullptr) {
                    MotionNapi *motion = reinterpret_cast<MotionNapi *>(data);
                    delete motion;
                }
            }, nullptr, nullptr);
        if (status != napi_ok) {
            delete g_motionObj;
            g_motionObj = nullptr;
            FI_HILOGE("napi_wrap failed");
            return false;
        }
    }
    return true;
}

napi_value MotionNapi::SubscribeMotion(napi_env env, napi_callback_info info)
{
    FI_HILOGD("Enter");
#ifdef MOTION_ENABLE
    int64_t processorId = NapiEventUtils::AddProcessor();
    int64_t beginTime = NapiEventUtils::GetSysClockTime();
    std::string transId = std::string("transId_") + std::to_string(std::rand());
#endif
    size_t argc = ARG_2;
    napi_value args[ARG_2] = { nullptr };
    napi_value jsThis = nullptr;
    napi_value result = nullptr;
    napi_status status = napi_get_cb_info(env, info, &argc, args, &jsThis, nullptr);
    if (status != napi_ok) {
        ThrowMotionErr(env, SUBSCRIBE_EXCEPTION, "napi_get_cb_info failed");
        return nullptr;
    }

    if (!ValidateArgsType(env, args, argc, EXPECTED_SUB_ARG_TYPES)) {
        ThrowMotionErr(env, PARAM_EXCEPTION, "validateargstype failed");
        return nullptr;
    }

    std::string typeStr;
    if (!TransJsToStr(env, args[ARG_0], typeStr)) {
        ThrowMotionErr(env, SUBSCRIBE_EXCEPTION, "Trans to string failed");
        return nullptr;
    }

    int32_t type = GetMotionType(typeStr);
    if (type == INVALID_MOTION_TYPE) {
        ThrowMotionErr(env, PARAM_EXCEPTION, "Type is illegal");
        return nullptr;
    }

    if (!ConstructMotion(env, jsThis)) {
        ThrowMotionErr(env, SUBSCRIBE_EXCEPTION, "Failed to get g_motionObj");
        return nullptr;
    }

#ifdef MOTION_ENABLE
    if (!SubscribeCallback(env, type)) {
        return nullptr;
    }

    if (!g_motionObj->AddCallback(type, args[ARG_1])) {
        ThrowMotionErr(env, SERVICE_EXCEPTION, "AddCallback failed");
        return nullptr;
    }
    if (processorId == EVENT_NOT_SUPPORT) {
        FI_HILOGW("Non-applications do not support breakpoint");
    } else {
        NapiEventUtils::WriteEndEvent(transId, "motion.on", beginTime, 0, 0);
    }
    napi_get_undefined(env, &result);
    return result;
#else
    ThrowMotionErr(env, DEVICE_EXCEPTION, "Device not support");
    return result;
#endif
}

napi_value MotionNapi::UnSubscribeMotion(napi_env env, napi_callback_info info)
{
    FI_HILOGD("Enter");
#ifdef MOTION_ENABLE
    int64_t processorId = NapiEventUtils::AddProcessor();
    int64_t beginTime = NapiEventUtils::GetSysClockTime();
    std::string transId = std::string("transId_") + std::to_string(std::rand());
#endif
    if (g_motionObj == nullptr) {
        ThrowMotionErr(env, UNSUBSCRIBE_EXCEPTION, "g_motionObj is nullptr");
        return nullptr;
    }

    size_t argc = ARG_2;
    napi_value args[ARG_2] = { nullptr };
    napi_value jsThis = nullptr;
    napi_value result = nullptr;
    napi_status status = napi_get_cb_info(env, info, &argc, args, &jsThis, nullptr);
    if (status != napi_ok) {
        ThrowMotionErr(env, UNSUBSCRIBE_EXCEPTION, "napi_get_cb_info is failed");
        return nullptr;
    }

    auto expectedArgs = EXPECTED_UNSUB_TWO_ARG_TYPES;
    if (argc != ARG_2) {
        expectedArgs = EXPECTED_UNSUB_ONE_ARG_TYPES;
    }
    if (!ValidateArgsType(env, args, argc, expectedArgs)) {
        ThrowMotionErr(env, PARAM_EXCEPTION, "validateargstype failed");
        return nullptr;
    }

    std::string typeStr;
    if (!TransJsToStr(env, args[ARG_0], typeStr)) {
        ThrowMotionErr(env, UNSUBSCRIBE_EXCEPTION, "Trans to string failed");
        return nullptr;
    }

    int32_t type = GetMotionType(typeStr);
    if (type == INVALID_MOTION_TYPE) {
        ThrowMotionErr(env, PARAM_EXCEPTION, "Type is illegal");
        return nullptr;
    }

#ifdef MOTION_ENABLE
    if (argc != ARG_2) {
        if (!g_motionObj->RemoveAllCallback(type)) {
            ThrowMotionErr(env, SERVICE_EXCEPTION, "RemoveCallback failed");
            return nullptr;
        }
    } else {
        if (!g_motionObj->RemoveCallback(type, args[ARG_1])) {
            ThrowMotionErr(env, SERVICE_EXCEPTION, "RemoveCallback failed");
            return nullptr;
        }
    }

    if (!UnSubscribeCallback(env, type)) {
        return nullptr;
    }
    if (processorId == EVENT_NOT_SUPPORT) {
        FI_HILOGW("Non-applications do not support breakpoint");
    } else {
        NapiEventUtils::WriteEndEvent(transId, "motion.off", beginTime, 0, 0);
    }
    napi_get_undefined(env, &result);
    return result;
#else
    ThrowMotionErr(env, DEVICE_EXCEPTION, "Device not support");
    return result;
#endif
}

napi_value MotionNapi::GetRecentOptHandStatus(napi_env env, napi_callback_info info)
{
    FI_HILOGD("Enter");
    napi_value result = nullptr;
    size_t argc = ARG_0;
    napi_value jsThis;

    napi_status status = napi_get_cb_info(env, info, &argc, NULL, &jsThis, nullptr);
    if (status != napi_ok) {
        ThrowMotionErr(env, SERVICE_EXCEPTION, "napi_get_cb_info failed");
        return nullptr;
    }

#ifdef MOTION_ENABLE
    MotionEvent motionEvent = g_motionClient.GetMotionData(MOTION_TYPE_OPERATING_HAND);
    if (motionEvent.status == DEVICE_EXCEPTION) {
        ThrowMotionErr(env, DEVICE_EXCEPTION, "Device not support");
        return nullptr;
    }
    if (motionEvent.status == -1) {
        ThrowMotionErr(env, PERMISSION_EXCEPTION, "Invalid Type");
        return nullptr;
    }
#endif

    ConstructMotion(env, jsThis);
#ifdef MOTION_ENABLE
    if (g_motionObj == nullptr) {
        ThrowMotionErr(env, SERVICE_EXCEPTION, "Error invalid type");
        return nullptr;
    }
    napi_status ret = napi_create_int32(env, static_cast<int32_t>(motionEvent.status), &result);
    if (ret != napi_ok) {
        ThrowMotionErr(env, SERVICE_EXCEPTION, "napi_create_int32 failed");
        return nullptr;
    }
#else
    ThrowMotionErr(env, DEVICE_EXCEPTION, "Device not support");
#endif
    FI_HILOGD("Exit");
    return result;
}

napi_value MotionNapi::Init(napi_env env, napi_value exports)
{
    FI_HILOGD("Enter");
    napi_property_descriptor desc[] = {
        DECLARE_NAPI_STATIC_FUNCTION("on", SubscribeMotion),
        DECLARE_NAPI_STATIC_FUNCTION("off", UnSubscribeMotion),
        DECLARE_NAPI_STATIC_FUNCTION("getRecentOperatingHandStatus", GetRecentOptHandStatus),
    };
    NAPI_CALL(env, napi_define_properties(env, exports, sizeof(desc)/sizeof(desc[0]), desc));

    napi_value operatingHandStatus;
    napi_status status = napi_create_object(env, &operatingHandStatus);
    if (status != napi_ok) {
        FI_HILOGE("Failed create object");
        return exports;
    }

    SetInt32Property(env, operatingHandStatus, BASE_HAND, "UNKNOWN_STATUS");
    SetInt32Property(env, operatingHandStatus, LEFT_HAND, "LEFT_HAND_OPERATED");
    SetInt32Property(env, operatingHandStatus, RIGHT_HAND, "RIGHT_HAND_OPERATED");
    SetPropertyName(env, exports, "OperatingHandStatus", operatingHandStatus);
    DefineHoldingHandStatus(env, exports);
    FI_HILOGD("Exit");
    return exports;
}

void MotionNapi::DefineHoldingHandStatus(napi_env env, napi_value exports)
{
    napi_value holdingHandStatus;
    napi_status status = napi_create_object(env, &holdingHandStatus);
    if (status != napi_ok) {
        FI_HILOGE("Failed create object");
        return;
    }

    SetInt32Property(env, holdingHandStatus, HoldPostureStatus::UNKNOWN, "UNKNOWN_STATUS");
    SetInt32Property(env, holdingHandStatus, HoldPostureStatus::NOT_HELD, "NOT_HELD");
    SetInt32Property(env, holdingHandStatus, HoldPostureStatus::LEFT_HAND_HELD, "LEFT_HAND_HELD");
    SetInt32Property(env, holdingHandStatus, HoldPostureStatus::RIGHT_HAND_HELD, "RIGHT_HAND_HELD");
    SetInt32Property(env, holdingHandStatus, HoldPostureStatus::BOTH_HAND_HELD, "BOTH_HANDS_HELD");
    SetPropertyName(env, exports, "HoldingHandStatus", holdingHandStatus);
}

void MotionNapi::SetInt32Property(napi_env env, napi_value targetObj, int32_t value, const char *propName)
{
    napi_value prop = nullptr;
    napi_status ret = napi_create_int32(env, value, &prop);
    if (ret != napi_ok) {
        FI_HILOGE("napi_create_int32 failed");
        return;
    }
    SetPropertyName(env, targetObj, propName, prop);
}

void MotionNapi::SetPropertyName(napi_env env, napi_value targetObj, const char *propName, napi_value propValue)
{
    napi_status status = napi_set_named_property(env, targetObj, propName, propValue);
    if (status != napi_ok) {
        FI_HILOGE("Failed to set the name property");
        return;
    }
}

bool MotionNapi::ValidateArgsType(napi_env env, napi_value *args, size_t argc,
    const std::vector<std::string> &expectedTypes)
{
    FI_HILOGD("Enter");
    napi_status status = napi_ok;
    napi_valuetype valueType = napi_undefined;

    if (argc != expectedTypes.size()) {
        FI_HILOGE("Wrong number of arguments");
        return false;
    }

    for (size_t i = 0; i < argc; ++i) {
        status = napi_typeof(env, args[i], &valueType);
        if (status != napi_ok) {
            FI_HILOGE("Error while checking arguments types");
            return false;
        }
        std::string expectedType = expectedTypes[i];
        if ((expectedType == "string" && valueType != napi_string) ||
            (expectedType == "function" && valueType != napi_function)) {
                FI_HILOGE("Wrong argument type");
                return false;
        }
    }
    return true;
}

bool MotionNapi::TransJsToStr(napi_env env, napi_value value, std::string &str)
{
    FI_HILOGD("Enter");
    size_t strlen = 0;
    napi_status status = napi_get_value_string_utf8(env, value, nullptr, 0, &strlen);
    if (status != napi_ok) {
        FI_HILOGE("Error string length invalid");
        return false;
    }
    if (strlen < 0 || strlen > MAX_ARG_STRING_LEN) {
        FI_HILOGE("The string length invalid");
        return false;
    }
    std::vector<char> buf(strlen + 1);
    status = napi_get_value_string_utf8(env, value, buf.data(), strlen+1, &strlen);
    if (status != napi_ok) {
        FI_HILOGE("napi_get_value_string_utf8 failed");
        return false;
    }
    str = buf.data();
    return true;
}

EXTERN_C_START
/*
 * function for module exports
 */
static napi_value MotionInit(napi_env env, napi_value exports)
{
    FI_HILOGD("Enter");
    napi_value ret = MotionNapi::Init(env, exports);
    if (ret == nullptr) {
        FI_HILOGE("Failed to init");
        return ret;
    }
    FI_HILOGD("Exit");
    return ret;
}
EXTERN_C_END

/*
 * Module definition
 */
static napi_module g_module = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = "multimodalAwareness.motion",
    .nm_register_func = MotionInit,
    .nm_modname = "multimodalAwareness.motion",
    .nm_priv = (static_cast<void *>(nullptr)),
    .reserved = { nullptr }
};

/*
 * Module registration
 */
extern "C" __attribute__((constructor)) void RegisterModule(void)
{
    napi_module_register(&g_module);
}
} // namespace Msdp
} // namespace OHOS
