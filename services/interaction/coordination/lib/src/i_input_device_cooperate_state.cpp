/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "i_input_device_cooperate_state.h"

#include "cooperate_event_manager.h"
#include "devicestatus_define.h"
#include "distributed_input_adapter.h"
#include "input_device_cooperate_sm.h"

namespace OHOS {
namespace Msdp {
namespace DeviceStatus {
namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, MSDP_DOMAIN_ID, "IInputDeviceCooperateState" };
} // namespace

IInputDeviceCooperateState::IInputDeviceCooperateState()
{
    runner_ = AppExecFwk::EventRunner::Create(true);
    CHKPL(runner_);
    eventHandler_ = std::make_shared<CooperateEventHandler>(runner_);
}

int32_t IInputDeviceCooperateState::PrepareAndStart(const std::string &srcNetworkId, int32_t startInputDeviceId)
{
    CALL_INFO_TRACE;
    auto* context = CooperateEventMgr->GetIContext();
    CHKPR(context, RET_ERR);
    std::string sinkNetworkId = context->GetDeviceManager().GetOriginNetworkId(startInputDeviceId); // 本端发起穿越，那么本端就是sink端
    int32_t ret = RET_ERR;
    if (NeedPrepare(srcNetworkId, sinkNetworkId)) {
        InputDevCooSM->UpdatePreparedDevices(srcNetworkId, sinkNetworkId);
        ret = DistributedAdapter->PrepareRemoteInput(
            srcNetworkId, sinkNetworkId, [this, srcNetworkId, startInputDeviceId](bool isSuccess) {
                this->OnPrepareDistributedInput(isSuccess, srcNetworkId, startInputDeviceId);
            }); // 分布式输入的Adapter
        if (ret != RET_OK) {
            FI_HILOGE("Prepare remote input fail");
            InputDevCooSM->OnStartFinish(false, sinkNetworkId, startInputDeviceId);
            InputDevCooSM->UpdatePreparedDevices("", "");
        }
    } else {
        ret = StartRemoteInput(startInputDeviceId);
        if (ret != RET_OK) {
            FI_HILOGE("Start remoteNetworkId input fail");
            InputDevCooSM->OnStartFinish(false, sinkNetworkId, startInputDeviceId);
        }
    }
    return ret;
}

void IInputDeviceCooperateState::OnPrepareDistributedInput(
    bool isSuccess, const std::string &srcNetworkId, int32_t startInputDeviceId)
{
    FI_HILOGI("isSuccess: %{public}s", isSuccess ? "true" : "false");
    if (!isSuccess) {
        InputDevCooSM->UpdatePreparedDevices("", "");
        InputDevCooSM->OnStartFinish(false, srcNetworkId, startInputDeviceId);
    } else {
        std::string taskName = "start_dinput_task";
        std::function<void()> handleStartDinputFunc =
            std::bind(&IInputDeviceCooperateState::StartRemoteInput, this, startInputDeviceId);
        CHKPV(eventHandler_);
        eventHandler_->ProxyPostTask(handleStartDinputFunc, taskName, 0);
    }
}

int32_t IInputDeviceCooperateState::StartRemoteInput(int32_t startInputDeviceId)
{
    CALL_DEBUG_ENTER;
    std::pair<std::string, std::string> networkIds = InputDevCooSM->GetPreparedDevices();
    auto* context = CooperateEventMgr->GetIContext();
    CHKPR(context, RET_ERR);
    std::vector<std::string> dhids = context->GetDeviceManager().GetCooperateDhids(startInputDeviceId);  // 什么是 dhids 
    if (dhids.empty()) {
        InputDevCooSM->OnStartFinish(false, networkIds.first, startInputDeviceId);
        return static_cast<int32_t>(CooperationMessage::INPUT_DEVICE_ID_ERROR);
    }
    int32_t ret = DistributedAdapter->StartRemoteInput(
        networkIds.first, networkIds.second, dhids, [this, src = networkIds.first, startInputDeviceId](bool isSuccess) {
            this->OnStartRemoteInput(isSuccess, src, startInputDeviceId);
        }); // 这一行是正常处理分支，其他都是异常分支
    if (ret != RET_OK) {
        InputDevCooSM->OnStartFinish(false, networkIds.first, startInputDeviceId);
        return static_cast<int32_t>(CooperationMessage::COOPERATE_FAIL);
    }
    return RET_OK; 
}

void IInputDeviceCooperateState::OnStartRemoteInput(
    bool isSuccess, const std::string &srcNetworkId, int32_t startInputDeviceId)
{
    CALL_DEBUG_ENTER;
    std::string taskName = "start_finish_task";
    std::function<void()> handleStartFinishFunc =
        std::bind(&InputDeviceCooperateSM::OnStartFinish, InputDevCooSM, isSuccess, srcNetworkId, startInputDeviceId);
    CHKPV(eventHandler_);
    eventHandler_->ProxyPostTask(handleStartFinishFunc, taskName, 0);
}

bool IInputDeviceCooperateState::NeedPrepare(const std::string &srcNetworkId, const std::string &sinkNetworkId)
{
    CALL_DEBUG_ENTER;
    std::pair<std::string, std::string> prepared = InputDevCooSM->GetPreparedDevices();
    bool isNeed =  !(srcNetworkId == prepared.first && sinkNetworkId == prepared.second);
    FI_HILOGI("NeedPrepare?: %{public}s", isNeed ? "true" : "false");
    return isNeed;
}
} // namespace DeviceStatus
} // namespace Msdp
} // namespace OHOS
