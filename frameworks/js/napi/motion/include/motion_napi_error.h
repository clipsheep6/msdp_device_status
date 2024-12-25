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

#ifndef MOTION_NAPI_ERROR_H
#define MOTION_NAPI_ERROR_H

#include <map>

#include "napi/native_api.h"
#include "napi/native_node_api.h"

namespace OHOS {
namespace Msdp {
constexpr int32_t PERMISSION_EXCEPTION { 201 };
constexpr int32_t PARAM_EXCEPTION { 401 };
constexpr int32_t DEVICE_EXCEPTION { 801 };
constexpr int32_t SUBSCRIBE_EXCEPTION { 31500001 };
constexpr int32_t UNSUBSCRIBE_EXCEPTION { 31500002 };
constexpr int32_t GETOPT_EXCEPTION { 31500003 };
const std::map<int32_t, std::string> ERROR_MESSAGES = {
    {PERMISSION_EXCEPTION, "Permission check failed."},
    {PARAM_EXCEPTION, "Params check failed."},
    {DEVICE_EXCEPTION, "The device does not support this API."},
    {SUBSCRIBE_EXCEPTION, "Subscribe failed."},
    {UNSUBSCRIBE_EXCEPTION, "UnSubscribe failed."},
    {GETOPT_EXCEPTION, "GetOpt failed."}
};

napi_value CreateMotionNapiError(const napi_env &env, int32_t errorCode, const std::string &errorMsg);
std::optional<std::string> GetMotionErrMsg(int32_t errorCode);
void ThrowMotionErr(const napi_env &env, int32_t errorCode, const std::string &printMsg);
} // namespace Msdp
} // namespae OHOS
#endif // MOTION_NAPI_ERROR_H
