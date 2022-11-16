/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef DEVICESTATUS_NAPI_H
#define DEVICESTATUS_NAPI_H

#include <map>

#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "devicestatus_callback_stub.h"
#include "devicestatus_data_utils.h"
#include "devicestatus_event.h"

#include "idevicestatus_callback.h"

namespace OHOS {
namespace Msdp {
namespace DeviceStatus {
class DevicestatusCallback : public DevicestatusCallbackStub {
public:
    explicit DevicestatusCallback() {};
    virtual ~DevicestatusCallback() {};
    void OnDevicestatusChanged(const Data& devicestatusData) override;
};

class DevicestatusNapi : public DeviceStatusEvent {
public:
    explicit DevicestatusNapi(napi_env env);
    virtual ~DevicestatusNapi();

    static napi_value Init(napi_env env, napi_value exports);
    static napi_value SubscribeDevicestatus(napi_env env, napi_callback_info info);
    static napi_value UnSubscribeDevicestatus(napi_env env, napi_callback_info info);
    static napi_value GetDevicestatus(napi_env env, napi_callback_info info);
    void OnDevicestatusChangedDone(const int32_t& type, const int32_t& value, bool isOnce);
    static DevicestatusNapi* GetDevicestatusNapi(int32_t type);
    static std::map<int32_t, sptr<IdevicestatusCallback>> callbackMap_;
    static std::map<int32_t, DevicestatusNapi*> objectMap_;

private:
    static bool CheckArguments(napi_env env, napi_callback_info info);
    static bool CheckUnsubArguments(napi_env env, napi_callback_info info);
    static bool CheckGetArguments(napi_env env, napi_callback_info info);
    napi_ref callbackRef_;
    static napi_ref devicestatusValueRef_;
    napi_env env_;
};
} // namespace DeviceStatus
} // namespace Msdp
} // namespace OHOS
#endif // DEVICESTATUS_NAPI_H
