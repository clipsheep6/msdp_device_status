/*
* Copyright (C) 2023 Huawei Device Co., Ltd.
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
    
#include "cooperate_hisysevent.h"
    
#include "hisysevent.h"
    
#undef LOG_TAG
#define LOG_TAG "CooperateHisysevent"
namespace OHOS {
namespace Msdp {
namespace DeviceStatus {
namespace {
    const std::string COOPERTATE_BEHAVIOR { "COOPERTATE_BEHAVIOR" };
    const std::string ORG_PKG_NAME { "device_status" };
} // namespace
    
void CooperateRadar::ReportCooperateRadarInfo(struct CooperateRadarInfo &cooperateRadarInfo)
{
    HiSysEventWrite(
        OHOS::HiviewDFX::HiSysEvent::Domain::MSDP,
        COOPERTATE_BEHAVIOR,
        HiviewDFX::HiSysEvent::EventType::BEHAVIOR,
        "ORG_PKG", ORG_PKG_NAME,
        "FUNC", cooperateRadarInfo.funcName,
        "BIZ_SCENE", 1,
        "BIZ_STATE", cooperateRadarInfo.bizState,
        "BIZ_STAGE", cooperateRadarInfo.bizStage,
        "STAGE_RES", cooperateRadarInfo.stageRes,
        "ERROR_CODE", cooperateRadarInfo.errCode,
        "HOST_PKG", cooperateRadarInfo.hostName,
        "LOCAL_NET_ID", cooperateRadarInfo.localNetId,
        "PEER_NET_ID", cooperateRadarInfo.peerNetId);
}

} // namespace DeviceStatus
} // namespace Msdp
} // namespace OHOS
