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

#include "devicestatus_napi.h"

#include <js_native_api.h>

#include "napi/native_api.h"
#include "napi/native_node_api.h"

#include "devicestatus_common.h"

using namespace OHOS;
using namespace OHOS::Msdp;
using namespace OHOS::Msdp::DeviceStatus;
namespace {
constexpr size_t ARG_0 = 0;
constexpr size_t ARG_1 = 1;
constexpr size_t ARG_2 = 2;
constexpr size_t ARG_3 = 3;
constexpr size_t ARG_4 = 4;
constexpr int32_t NAPI_BUF_LENGTH  = 256;
static const std::vector<std::string> vecDeviceStatusValue {
    "VALUE_ENTER", "VALUE_EXIT"
};
thread_local DeviceStatusNapi *g_obj = nullptr;
} // namespace

std::shared_ptr<DeviceStatusAgent> DeviceStatusNapi::agent_ = nullptr;
napi_ref DeviceStatusNapi::devicestatusValueRef_ = nullptr;
std::shared_ptr<DeviceStatusCallback> DeviceStatusNapi::callback_ = nullptr;
struct ResponseEntity {
    OnChangedValue value;
};

bool DeviceStatusCallback::OnEventResult(const Data& devicestatusData)
{
    DEV_HILOGD(JS_NAPI, "OnDeviceStatusChanged enter");
    std::lock_guard<std::mutex> guard(mutex_);
    uv_loop_s *loop = nullptr;
    napi_get_uv_event_loop(env_, &loop);
    if (loop == nullptr) {
        DEV_HILOGE(JS_NAPI, "loop is nullptr");
        return false;
    }
    uv_work_t *work = new (std::nothrow) uv_work_t;
    if (work == nullptr) {
        DEV_HILOGE(JS_NAPI, "work is nullptr");
        return false;
    }
    DEV_HILOGD(JS_NAPI, "devicestatusData.type:%{public}d, devicestatusData.value:%{public}d",
        devicestatusData.type, devicestatusData.value);
    data_ = devicestatusData;
    work->data = static_cast<void *>(&data_);
    int ret = uv_queue_work(loop, work, [] (uv_work_t *work) {}, EmitOnEvent);
    if (ret != 0) {
        DEV_HILOGE(JS_NAPI, "Failed to execute work queue");
        return false;
    }
    return true;
}

void DeviceStatusCallback::EmitOnEvent(uv_work_t *work, int status)
{
    Data* data = static_cast<Data*>(work->data);
    delete work;
    if (data == nullptr) {
        DEV_HILOGE(JS_NAPI, "work->data is nullptr");
        return;
    }
    DeviceStatusNapi* deviceStatusNapi = DeviceStatusNapi::GetDeviceStatusNapi();
    if (deviceStatusNapi == nullptr) {
        DEV_HILOGE(JS_NAPI, "deviceStatusNapi is nullptr");
        return;
    }

    int32_t type = static_cast<int32_t>(data->type);
    int32_t value = static_cast<int32_t>(data->value);
    DEV_HILOGD(JS_NAPI, "type:%{public}d, value:%{public}d", type, value);
    deviceStatusNapi->OnDeviceStatusChangedDone(type, value, false);
}

DeviceStatusNapi* DeviceStatusNapi::GetDeviceStatusNapi()
{
    DEV_HILOGD(JS_NAPI, "Enter");
    return g_obj;
}

DeviceStatusNapi::DeviceStatusNapi(napi_env env) : DeviceStatusEvent(env)
{
    env_ = env;
    callbackRef_ = nullptr;
    devicestatusValueRef_ = nullptr;
    agent_->RegisterDeathListener([this] {
        DEV_HILOGI(JS_NAPI, "Receive death notification");
        ClearEventMap();
    });
}

DeviceStatusNapi::~DeviceStatusNapi()
{
    if (callbackRef_ != nullptr) {
        napi_delete_reference(env_, callbackRef_);
    }
    if (devicestatusValueRef_ != nullptr) {
        napi_delete_reference(env_, devicestatusValueRef_);
    }
    if (g_obj != nullptr) {
        delete g_obj;
        g_obj = nullptr;
    }
}

void DeviceStatusNapi::OnDeviceStatusChangedDone(int32_t type, int32_t value, bool isOnce)
{
    DEV_HILOGD(JS_NAPI, "Enter, value:%{public}d", value);
    OnEvent(type, ARG_1, value, isOnce);
    DEV_HILOGD(JS_NAPI, "Exit");
}

int32_t DeviceStatusNapi::ConvertTypeToInt(const std::string &type)
{
    if (type == "absoluteStill") {
        return Type::TYPE_ABSOLUTE_STILL;
    } else if (type == "horizontalPosition") {
        return Type::TYPE_HORIZONTAL_POSITION;
    } else if (type == "verticalPosition") {
        return Type::TYPE_VERTICAL_POSITION;
    } else if (type == "still") {
        return Type::TYPE_STILL;
    } else if (type == "relativeStill") {
        return Type::TYPE_RELATIVE_STILL;
    } else if (type == "carBluetooth") {
        return Type::TYPE_CAR_BLUETOOTH;
    } else {
        return Type::TYPE_INVALID;
    }
}

bool DeviceStatusNapi::CheckArguments(napi_env env, napi_callback_info info)
{
    DEV_HILOGD(JS_NAPI, "Enter");
    int arr[ARG_4] = {};
    size_t argc = ARG_4;
    napi_value args[ARG_4] = {};
    napi_status status = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    if (status != napi_ok) {
        DEV_HILOGE(JS_NAPI, "Failed to get_cb_info");
        return false;
    }
    for (size_t i = 0; i < ARG_4; i++) {
        napi_valuetype valueType = napi_undefined;
        status = napi_typeof(env, args[i], &valueType);
        if (status != napi_ok) {
            DEV_HILOGE(JS_NAPI, "Failed to get valueType");
            return false;
        }
        DEV_HILOGD(JS_NAPI, "valueType:%{public}d", valueType);
        arr[i] = valueType;
    }
    if (arr[ARG_0] != napi_string || arr[ARG_1] != napi_number || arr[ARG_2] != napi_number ||
        arr[ARG_3] != napi_function) {
        DEV_HILOGE(JS_NAPI, "Failed to get arguements");
        return false;
    }
    DEV_HILOGD(JS_NAPI, "Exit");
    return true;
}

bool DeviceStatusNapi::CheckUnsubArguments(napi_env env, napi_callback_info info)
{
    DEV_HILOGD(JS_NAPI, "Enter");
    int arr[ARG_3] = {};
    size_t argc = ARG_3;
    napi_value args[ARG_3] = {};
    napi_status status = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    if (status != napi_ok) {
        DEV_HILOGE(JS_NAPI, "Failed to get_cb_info");
        return false;
    }
    for (size_t i = 0; i < ARG_3; i++) {
        napi_valuetype valueType = napi_undefined;
        status = napi_typeof(env, args[i], &valueType);
        if (status != napi_ok) {
            DEV_HILOGE(JS_NAPI, "Failed to get valueType");
            return false;
        }
        DEV_HILOGD(JS_NAPI, "valueType:%{public}d", valueType);
        arr[i] = valueType;
    }
    if (arr[ARG_0] != napi_string || arr[ARG_1] != napi_number || arr[ARG_2] != napi_function) {
        DEV_HILOGE(JS_NAPI, "Failed to get arguements");
        return false;
    }
    DEV_HILOGD(JS_NAPI, "Exit");
    return true;
}

bool DeviceStatusNapi::CheckGetArguments(napi_env env, napi_callback_info info)
{
    DEV_HILOGD(JS_NAPI, "Enter");
    int arr[ARG_2] = {};
    size_t argc = ARG_2;
    napi_value args[ARG_2] = {};
    napi_status status = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    if (status != napi_ok) {
        DEV_HILOGE(JS_NAPI, "Failed to get_cb_info");
        return false;
    }
    for (size_t i = 0; i < ARG_2; i++) {
        napi_valuetype valueType = napi_undefined;
        status = napi_typeof(env, args[i], &valueType);
        if (status != napi_ok) {
            DEV_HILOGE(JS_NAPI, "Failed to get valueType");
            return false;
        }
        DEV_HILOGD(JS_NAPI, "valueType:%{public}d", valueType);
        arr[i] = valueType;
    }
    if (arr[ARG_0] != napi_string || arr[ARG_1] != napi_function) {
        DEV_HILOGE(JS_NAPI, "Failed to get arguements");
        return false;
    }
    DEV_HILOGD(JS_NAPI, "Exit");
    return true;
}

napi_value DeviceStatusNapi::SubscribeDeviceStatusCallback(napi_env env, napi_callback_info info, napi_value *args,
    int32_t type, int32_t event, int32_t latency)
{
    if (g_obj == nullptr) {
        g_obj = new (std::nothrow) DeviceStatusNapi(env);
        if (g_obj == nullptr) {
            DEV_HILOGE(JS_NAPI, "Failed to new g_obj");
            return nullptr;
        }
        DEV_HILOGD(JS_NAPI, "Didn't find object, so created it");
    }
    napi_wrap(env, nullptr, reinterpret_cast<void *>(g_obj),
        [](napi_env env, void *data, void *hint) {
            (void)env;
            (void)hint;
            DeviceStatusNapi *devicestatus = static_cast<DeviceStatusNapi *>(data);
            delete devicestatus;
        },
        nullptr, &(g_obj->callbackRef_));
    if (!g_obj->On(type, args[ARG_3], false)) {
        DEV_HILOGE(JS_NAPI, "type:%{public}d already exists", type);
        return nullptr;
    }
    if (callback_ == nullptr) {
        callback_ = std::make_shared<DeviceStatusCallback>(env);
    }
    if (agent_ == nullptr) {
        DEV_HILOGE(JS_NAPI, "agent_ is nullptr");
        return nullptr;
    }
    agent_->SubscribeAgentEvent(static_cast<Type>(type), static_cast<ActivityEvent>(event),
       static_cast<ReportLatencyNs>(latency),
       std::static_pointer_cast<DeviceStatusAgent::DeviceStatusAgentEvent>(callback_));
    DEV_HILOGD(JS_NAPI, "Exit");
    return nullptr;
}

napi_value DeviceStatusNapi::SubscribeDeviceStatus(napi_env env, napi_callback_info info)
{
    DEV_HILOGD(JS_NAPI, "Enter");
    size_t argc = ARG_4;
    napi_value args[ARG_4] = {};
    napi_status status = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    NAPI_ASSERT(env, (status == napi_ok) && (argc >= ARG_4), "Bad parameters");
    if (!CheckArguments(env, info)) {
        DEV_HILOGE(JS_NAPI, "Failed to get arguements");
        return nullptr;
    }
    size_t modeLen = 0;
    status = napi_get_value_string_utf8(env, args[ARG_0], nullptr, 0, &modeLen);
    if (status != napi_ok) {
        DEV_HILOGE(JS_NAPI, "Failed to get string item");
        return nullptr;
    }
    char mode[NAPI_BUF_LENGTH] = {};
    status = napi_get_value_string_utf8(env, args[ARG_0], mode, modeLen + 1, &modeLen);
    if (status != napi_ok) {
        DEV_HILOGE(JS_NAPI, "Failed to get string item");
        return nullptr;
    }
    std::string typeMode = mode;
    int32_t eventMode = 0;
    status = napi_get_value_int32(env, args[ARG_1], &eventMode);
    if (status != napi_ok) {
        DEV_HILOGE(JS_NAPI, "Failed to get event value item");
        return nullptr;
    }
    int32_t latencyMode = 0;
    status = napi_get_value_int32(env, args[ARG_2], &latencyMode);
    if (status != napi_ok) {
        DEV_HILOGE(JS_NAPI, "Failed to get latency value item");
        return nullptr;
    }
    int32_t type = ConvertTypeToInt(typeMode);
    int32_t event = eventMode;
    int32_t latency = latencyMode;
    DEV_HILOGD(JS_NAPI, "type:%{public}d, event:%{public}d, latency:%{public}d", type, event, latency);

    NAPI_ASSERT(env, (type >= Type::TYPE_ABSOLUTE_STILL) && (type <= Type::TYPE_LID_OPEN), "type is illegal");
    NAPI_ASSERT(env, (event >= ActivityEvent::ENTER) && (event <= ActivityEvent::ENTER_EXIT), "event is illegal");
    NAPI_ASSERT(env, (latency >= ReportLatencyNs::SHORT) && (latency <= ReportLatencyNs::LONG), "latency is illegal");
    return SubscribeDeviceStatusCallback(env, info, args, type, event, latency);
}

napi_value DeviceStatusNapi::UnsubscribeDeviceStatus(napi_env env, napi_callback_info info)
{
    DEV_HILOGD(JS_NAPI, "Enter");
    size_t argc = ARG_3;
    napi_value args[ARG_3] = {};
    napi_status status = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    NAPI_ASSERT(env, (status == napi_ok) && (argc >= ARG_3), "Bad parameters");
    if (!CheckUnsubArguments(env, info)) {
        DEV_HILOGE(JS_NAPI, "Failed to get unsub arguements");
        return nullptr;
    }
    size_t len;
    status = napi_get_value_string_utf8(env, args[0], nullptr, 0, &len);
    if (status != napi_ok) {
        DEV_HILOGE(JS_NAPI, "Failed to get string item");
        return nullptr;
    }
    std::vector<char> typeBuf(len + 1);
    status = napi_get_value_string_utf8(env, args[0], typeBuf.data(), len + 1, &len);
    if (status != napi_ok) {
        DEV_HILOGE(JS_NAPI, "Failed to get string item");
        return nullptr;
    }
    int32_t type = ConvertTypeToInt(typeBuf.data());
    NAPI_ASSERT(env, (type >= Type::TYPE_ABSOLUTE_STILL) && (type <= Type::TYPE_LID_OPEN), "type is illegal");
    int32_t eventMode = 0;
    status = napi_get_value_int32(env, args[ARG_1], &eventMode);
    if (status != napi_ok) {
        DEV_HILOGE(JS_NAPI, "Failed to get int32 item");
        return nullptr;
    }
    int32_t event = eventMode;
    NAPI_ASSERT(env, (event >= ActivityEvent::ENTER) && (event <= ActivityEvent::ENTER_EXIT), "event is illegal");
    DEV_HILOGD(JS_NAPI, "type:%{public}d, event:%{public}d", type, event);
    if (!g_obj->Off(type, args[ARG_2])) {
        DEV_HILOGE(JS_NAPI, "Not ready to Unsubscribe for type:%{public}d", type);
        return nullptr;
    }
    if (agent_ == nullptr) {
        DEV_HILOGE(JS_NAPI, "agent_ is nullptr");
        return nullptr;
    }
    agent_->UnsubscribeAgentEvent(static_cast<Type>(type), static_cast<ActivityEvent>(event));
    DEV_HILOGD(JS_NAPI, "Exit");
    return nullptr;
}

napi_value DeviceStatusNapi::GetDeviceStatus(napi_env env, napi_callback_info info)
{
    DEV_HILOGD(JS_NAPI, "Enter");
    size_t argc = ARG_2;
    napi_value args[ARG_2] = {};
    napi_status status = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    NAPI_ASSERT(env, (status == napi_ok) && (argc >= ARG_2), "Bad parameters");
    if (!CheckGetArguments(env, info)) {
        DEV_HILOGE(JS_NAPI, "Failed to get once arguements");
        return nullptr;
    }
    size_t len;
    status = napi_get_value_string_utf8(env, args[0], nullptr, 0, &len);
    if (status != napi_ok) {
        DEV_HILOGE(JS_NAPI, "Failed to get string item");
        return nullptr;
    }
    std::vector<char> typeBuf(len + 1);
    status = napi_get_value_string_utf8(env, args[0], typeBuf.data(), len + 1, &len);
    if (status != napi_ok) {
        DEV_HILOGE(JS_NAPI, "Failed to get string item");
        return nullptr;
    }
    int32_t type = ConvertTypeToInt(typeBuf.data());
    NAPI_ASSERT(env, (type >= Type::TYPE_ABSOLUTE_STILL) && (type <= Type::TYPE_LID_OPEN), "type is illegal");
    if (g_obj == nullptr) {
        g_obj = new (std::nothrow) DeviceStatusNapi(env);
        if (g_obj == nullptr) {
            DEV_HILOGE(JS_NAPI, "Failed to new g_obj");
            return nullptr;
        }
        napi_wrap(env, nullptr, reinterpret_cast<void *>(g_obj),
            [](napi_env env, void *data, void *hint) {
                (void)env;
                (void)hint;
                DeviceStatusNapi *devicestatus = static_cast<DeviceStatusNapi *>(data);
                delete devicestatus;
            },
            nullptr, &(g_obj->callbackRef_));
    }
    if (!g_obj->On(type, args[ARG_1], true)) {
        DEV_HILOGE(JS_NAPI, "type:%{public}d already exists", type);
        return nullptr;
    }
    if (agent_ == nullptr) {
        DEV_HILOGE(JS_NAPI, "agent_ is nullptr");
        return nullptr;
    }
    Data devicestatusData = agent_->GetDeviceStatusData(static_cast<Type>(type));
    g_obj->OnDeviceStatusChangedDone(devicestatusData.type, devicestatusData.value, true);
    g_obj->OffOnce(devicestatusData.type, args[ARG_1]);
    DEV_HILOGD(JS_NAPI, "Exit");
    return nullptr;
}

napi_value DeviceStatusNapi::EnumActivityEventConstructor(napi_env env, napi_callback_info info)
{
    DEV_HILOGD(JS_NAPI, "Enter");
    napi_value thisArg = nullptr;
    void *data = nullptr;
    napi_status status = napi_get_cb_info(env, info, nullptr, nullptr, &thisArg, &data);
    if (status != napi_ok) {
        DEV_HILOGE(JS_NAPI, "Failed to get_cb_info item");
        return nullptr;
    }
    napi_value global = nullptr;
    status = napi_get_global(env, &global);
    if (status != napi_ok) {
        DEV_HILOGE(JS_NAPI, "Failed to get_global item");
        return nullptr;
    }
    DEV_HILOGD(JS_NAPI, "Exit");
    return thisArg;
}

napi_value DeviceStatusNapi::DeclareEventTypeInterface(napi_env env, napi_value exports)
{
    DEV_HILOGD(JS_NAPI, "Enter");
    napi_value enter = nullptr;
    napi_status status = napi_create_int32(env, static_cast<int32_t>(ActivityEvent::ENTER), &enter);
    if (status != napi_ok) {
        DEV_HILOGE(JS_NAPI, "Failed to create ENTER item");
        return nullptr;
    }
    napi_value exit = nullptr;
    status = napi_create_int32(env, static_cast<int32_t>(ActivityEvent::EXIT), &exit);
    if (status != napi_ok) {
        DEV_HILOGE(JS_NAPI, "Failed to create EXIT item");
        return nullptr;
    }
    napi_value enter_exit = nullptr;
    status = napi_create_int32(env, static_cast<int32_t>(ActivityEvent::ENTER_EXIT), &enter_exit);
    if (status != napi_ok) {
        DEV_HILOGE(JS_NAPI, "Failed to create ENTER_EXIT item");
        return nullptr;
    }
    napi_property_descriptor desc[] = {
        DECLARE_NAPI_STATIC_PROPERTY("ENTER", enter),
        DECLARE_NAPI_STATIC_PROPERTY("EXIT", exit),
        DECLARE_NAPI_STATIC_PROPERTY("ENTER_EXIT", enter_exit),
    };
    napi_value result = nullptr;
    status = napi_define_class(env, "ActivityEvent", NAPI_AUTO_LENGTH,
        EnumActivityEventConstructor, nullptr, sizeof(desc) / sizeof(*desc), desc, &result);
    if (status != napi_ok) {
        DEV_HILOGE(JS_NAPI, "Failed to define_class item");
        return nullptr;
    }
    status = napi_set_named_property(env, exports, "ActivityEvent", result);
    if (status != napi_ok) {
        DEV_HILOGE(JS_NAPI, "Failed to set_named_property item");
        return nullptr;
    }
    DEV_HILOGD(JS_NAPI, "Exit");
    return exports;
}

napi_value DeviceStatusNapi::Init(napi_env env, napi_value exports)
{
    DEV_HILOGD(JS_NAPI, "Enter");
    napi_property_descriptor desc[] = {
        DECLARE_NAPI_FUNCTION("on", SubscribeDeviceStatus),
        DECLARE_NAPI_FUNCTION("off", UnsubscribeDeviceStatus),
        DECLARE_NAPI_FUNCTION("once", GetDeviceStatus),
    };
    DeclareEventTypeInterface(env, exports);
    if (agent_ == nullptr) {
        agent_ = std::make_shared<DeviceStatusAgent>();
    }
    NAPI_CALL(env, napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc));
    DEV_HILOGD(JS_NAPI, "Exit");
    return exports;
}

EXTERN_C_START
/*
 * function for module exports
 */
static napi_value DeviceStatusInit(napi_env env, napi_value exports)
{
    DEV_HILOGD(JS_NAPI, "Enter");
    napi_value ret = DeviceStatusNapi::Init(env, exports);
    DEV_HILOGD(JS_NAPI, "Exit");
    return ret;
}
EXTERN_C_END

/*
 * Module definition
 */
static napi_module g_module = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = "stationary",
    .nm_register_func = DeviceStatusInit,
    .nm_modname = "stationary",
    .nm_priv = (static_cast<void *>(0)),
    .reserved = {0}
};

/*
 * Module registration
 */
extern "C" __attribute__((constructor)) void RegisterModule(void)
{
    napi_module_register(&g_module);
}
