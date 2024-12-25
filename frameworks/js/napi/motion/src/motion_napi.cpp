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
#endif
static constexpr uint8_t ARG_0 = 0;
static constexpr uint8_t ARG_1 = 1;
static constexpr uint8_t ARG_2 = 2;
constexpr int32_t INVALID_MOTION_TYPE = -1;
constexpr size_t MAX_ARG_STRING_LEN = 512;
constexpr int32_t MOTION_TYPE_OPERATING_HAND = 3601;
const std::vector<std::string> EXPECTED_SUB_ARG_TYPES = { "string", "function" };
const std::vector<std::string> EXPECTED_UNSUB_ONE_ARG_TYPES = { "string" };
const std::vector<std::string> EXPECTED_UNSUB_TWO_ARG_TYPES = { "string", "function" };
const std::map<const std::string, int32_t> MOTION_TYPE_MAP = {
    { "operatingHandChanged", MOTION_TYPE_OPERATING_HAND },
};
MotionNapi *g_motionObj = nullptr;
} // namespace

std::mutex g_mutex;

#ifdef MOTION_ENABLE
void MotionCallback::OnMotionChanged(const MotionEvent &event)
{
    FI_HILOGD("Enter");
    std::lock_guard<std::mutex> guard(g_mutex);
    uv_loop_s *loop = nullptr;
    napi_status status = napi_get_uv_event_loop(env_, &loop);
    if (status != napi_ok) {
        FI_HILOGE("napi_get_uv_event_loop failed");
        return;
    }
    uv_work_t *work = new (std::nothrow) uv_work_t;
    if (work == nullptr) {
        FI_HILOGE("Failed to get work");
        return;
    }
    auto data = new (std::nothrow) MotionEvent();
    if (data == nullptr) {
        FI_HILOGE("The data create failed");
        delete work;
        return;
    }
    data->type = event.type;
    data->status = event.status;
    data->dataLen = event.dataLen;
    data->data = event.data;
    work->data = static_cast<void*>(data);
    int32_t ret = uv_queue_work_with_qos(loop, work, [] (uv_work_t *work) {}, EmitOnEvent, uv_qos_default);
    if (ret != 0) {
        FI_HILOGE("Failed to execute work queue");
        delete static_cast<MotionEvent*>(work->data);
        delete work;
    }
    FI_HILOGD("Exit");
}

void MotionCallback::EmitOnEvent(uv_work_t *work, int32_t status)
{
    if (work == nullptr) {
        FI_HILOGE("Failed to get work");
        return;
    }

    if (work->data == nullptr) {
        FI_HILOGE("work is nullptr");
        delete work;
        return;
    }

    MotionEvent* data = static_cast<MotionEvent *>(work->data);
    if (g_motionObj == nullptr) {
        FI_HILOGE("Failed to get g_motionObj");
        delete static_cast<MotionEvent*>(work->data);
        delete work;
    }
    g_motionObj->OnEventOperatingHand(data->type, 1, *data);
    delete static_cast<MotionEvent*>(work->data);
    delete work;
}
#endif

MotionNapi::MotionNapi(napi_env env, napi_value thisVar) : MotionEventNapi(env, thisVar)
{
    env_ = env;
}

MotionNapi::~MotionNapi()
{
    if (motionValueRef_ != nullptr) {
        napi_delete_reference(env_, motionValueRef_);
    }
}

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
        } else {
            FI_HILOGE("failed to subscribe");
            ThrowMotionErr(env, SUBSCRIBE_EXCEPTION, "Subscribe failed");
            return false;
        }
    }
    return false;
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
        ThrowMotionErr(env, SUBSCRIBE_EXCEPTION, "AddCallback failed");
        return nullptr;
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
    if (g_motionObj == nullptr) {
        ThrowMotionErr(env, UNSUBSCRIBE_EXCEPTION, "g_motionObj is nullptr");
        return nullptr;
    }

    size_t argc = ARG_1;
    napi_value args[ARG_2] = { nullptr };
    napi_value jsThis = nullptr;
    napi_value result = nullptr;
    napi_status status = napi_get_cb_info(env, info, &argc, args, &jsThis, nullptr);
    if (status != napi_ok) {
        ThrowMotionErr(env, UNSUBSCRIBE_EXCEPTION, "napi_get_cb_info is failed");
        return nullptr;
    }

    auto expectedArgs = EXPECTED_UNSUB_ONE_ARG_TYPES;
    if (argc == ARG_2) {
        expectedArgs = EXPECTED_UNSUB_TWO_ARG_TYPES;
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
    if (!g_motionObj->RemoveCallback(type)) {
        ThrowMotionErr(env, UNSUBSCRIBE_EXCEPTION, "RemoveCallback failed");
        return nullptr;
    }

    if (!UnSubscribeCallback(env, type)) {
        return nullptr;
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
        ThrowMotionErr(env, GETOPT_EXCEPTION, "napi_get_cb_info failed");
        return nullptr;
    }

#ifdef MOTION_ENABLE
    MotionEvent motionEvent = g_motionClient.GetMotionData(MOTION_TYPE_OPERATING_HAND);
    if (motionEvent.status == -1) {
        ThrowMotionErr(env, PERMISSION_EXCEPTION, "Invalid Type");
        return nullptr;
    }
#endif

    napi_status objStatus = napi_create_object(env, &result);
    if (objStatus != napi_ok) {
        return nullptr;
    }

    ConstructMotion(env, jsThis);
#ifdef MOTION_ENABLE
    if (g_motionObj == nullptr) {
        ThrowMotionErr(env, GETOPT_EXCEPTION, "Error invalid type");
        return nullptr;
    }
    napi_value tmpValue = nullptr;
    g_motionObj->CreateIntData(env, tmpValue, result, "OperatingHandStatus", static_cast<int32_t>(motionEvent.status));
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
    FI_HILOGD("Exit");
    return exports;
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
    .nm_filename = "device_motion",
    .nm_register_func = MotionInit,
    .nm_modname = "device_motion",
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
