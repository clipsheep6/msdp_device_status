/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

    
#ifndef COOPERATE_HISYSEVENT_H
#define COOPERATE_HISYSEVENT_H
    
#include <string>

namespace OHOS {
namespace Msdp {
namespace DeviceStatus {
    
enum class BizCooperateScene {
    SCENE_ACTIVE = 1,
    SCENE_PASSIVE,
    SCENE_LATENCY
};
    
enum class BizCooperateStageRes {
    RES_IDLE = 0,
    RES_SUCCESS,
    RES_FAIL
};
    
enum class BizCooperateStage {
    STAGE_CALLING_COOPERATE = 1,
    STAGE_CHECK_SAME_ACCOUNT,
    STAGE_CHECK_LOCAL_SWITCH,
    STAGE_CHECK_ALLOW_COOPERATE,
    STAGE_OPEN_DSOFTBUS_SESSION,
    STAGE_CHECK_UNECPECTED_CALLING,
    STAGE_SERIALIZE_INSTRUCTION,
    STAGE_SEND_INSTRUCTION_TO_REMOTE,
    STAGE_ADD_MMI_EVENT_INTERCEPOR,
    STAGE_SWITCH_STATE_MACHINE,
    STAGE_SET_CURSOR_VISIBILITY,
    STAGE_SRV_EVENT_MGR_NOTIFY,
    STAGE_CLIENT_ON_MESSAGE_RCVD,
    STAGE_PASSIVE_DEASERIALIZATION,
    STAGE_PASSIVE_CHECK_SAME_ACCOUNT,
    STAGE_CHECK_PEER_SWITCH,
    STATE_INPUT_EVENT_BUILDER_ENABLE,
    STAGE_PASSIVE_CURSOR_VISIBILITY
};
    
enum class CooperateRadarErrCode {
    CALLING_COOPERATE_SUCCESS = 0,
    CALLING_COOPERATE_FAILED = 61145108,
    CHECK_SAME_ACCOUNT_FAILED = 61145109,
    CHECK_LOCAL_SWITCH_FAILED = 61145110,
    CHECK_ALLOW_COOPERATE_FAILED = 61145111,
    OPEN_DSOFTBUS_SESSION_FAILED = 61145112,
    CHECK_UNECPECTED_CALLING_FAILED = 61145113,
    SERIALIZE_INSTRUCTION_FAILED = 61145114,
    SEND_INSTRUCTION_TO_REMOTE_FAILED = 61145115,
    ADD_MMI_EVENT_INTERCEPOR_FAILED = 61145116,
    SWITCH_STATE_MACHINE_FAILED = 61145117,
    SET_CURSOR_VISIBILITY_FAILED = 61145118,
    SRV_EVENT_MGR_NOTIFY_FAILED = 61145119,
    CLIENT_ON_MESSAGE_RCVD_FAILED = 61145120,
    PASSIVE_DEASERIALIZATION_FAILED = 61145121,
    PASSIVE_CHECK_SAME_ACCOUNT_FAILED = 61145122,
    CHECK_PEER_SWITCH_FAILED = 61145123,
    INPUT_EVENT_BUILDER_ENABLE_FAILED = 61145124,
    PASSIVE_CURSOR_VISIBILITY_FAILED = 61145125
};
    
struct CooperateRadarInfo {
    std::string funcName;
    int32_t bizState { -1 };
    int32_t bizStage { -1 };
    int32_t stageRes { -1 };
    int32_t bizScene { -1 };
    int32_t errCode { -1 };
    std::string hostName;
    std::string localNetId;
    std::string peerNetId;
    std::string toCallPkg;
    std::string localDeviceType;
    std::string peerDeviceType;
};

struct TransmissionLatencyRadarInfo {
    std::string funcName;
    int32_t bizState { -1 };
    int32_t bizStage { -1 };
    int32_t stageRes { -1 };
    int32_t bizScene { -1 };
    std::string localNetId;
    std::string peerNetId;
    int64_t driveEventTimeDT { -1 };
    int64_t cooperateInterceptorTimeDT { -1 };
    int64_t crossPlatformTimeDT { -1 };
    int32_t pointerSpeed { -1 };
    int32_t touchPadSpeed { -1 };
};

class CooperateRadar {
public:
    static void ReportCooperateRadarInfo(struct CooperateRadarInfo &cooperateRadarInfo);
    static void ReportTransmissionLatencyRadarInfo(
        struct TransmissionLatencyRadarInfo &transmissionLatencyRadarInfo);
};
} // namespace DeviceStatus
} // namespace Msdp
} // namespace OHOS
#endif // COOPERATE_HISYSEVENT_H
