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

#include "getcoordinationstate_fuzzer.h"

#include "singleton.h"

#define private public
#include "devicestatus_service.h"
#include "message_parcel.h"

using namespace OHOS::Msdp::DeviceStatus;
namespace OHOS {
const std::u16string FORMMGR_INTERFACE_TOKEN { u"ohos.msdp.Idevicestatus" };

bool GetCoordinationStateFuzzTest(const uint8_t* data, size_t size)
{
    MessageParcel datas;
    if (!datas.WriteInterfaceToken(FORMMGR_INTERFACE_TOKEN)) {
        return;
    }
    if (!datas.WriteBuffer(data, size)) {
        return;
    }
    if (!datas.RewindRead(0)) {
        return;
    }
    MessageParcel reply;
    MessageOption option;
    DelayedSingleton<DeviceStatusService>::GetInstance()->OnRemoteRequest(
        static_cast<uint32_t>(Msdp::DeviceInterfaceCode::GET_COORDINATION_STATE), datas, reply, option);
    return true;
}
} // namespace OHOS

extern "C" int LLVMFuzzerTestOneInput(const uint8_t * data, size_t size)
{
    /* Run your code on data */
    if (data == nullptr) {
        return 0;
    }
    OHOS::GetCoordinationStateFuzzTest(data, size);
    return 0;
}
