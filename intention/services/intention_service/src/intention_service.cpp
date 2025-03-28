/*
 * Copyright (c) 2023-2024 Huawei Device Co., Ltd.
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

#include "intention_service.h"

#include <ipc_skeleton.h>
#include <string_ex.h>
#include <xcollie/xcollie.h>
#include <xcollie/xcollie_define.h>

#include "devicestatus_define.h"
#include "i_plugin.h"

#undef LOG_TAG
#define LOG_TAG "IntentionService"

namespace OHOS {
namespace Msdp {
namespace DeviceStatus {
namespace {
constexpr int32_t SERVER_TIMEOUT { 5 };
}

IntentionService::IntentionService(IContext *context)
    : context_(context), socketServer_(context), cooperate_(context), drag_(context), dumper_(context, stationary_),
    boomerangDumper_(context, boomerang_)
{}

int32_t IntentionService::Dump(int32_t fd, const std::vector<std::u16string> &args)
{
    std::vector<std::string> argList;
    std::transform(args.begin(), args.end(), std::back_inserter(argList),
        [](const std::u16string &arg) {
            return Str16ToStr8(arg);
        });
    dumper_.Dump(fd, argList);
    return RET_OK;
}

int32_t IntentionService::Enable(Intention intention, MessageParcel &data, MessageParcel &reply)
{
    CallingContext context {
        .intention = intention,
        .fullTokenId = IPCSkeleton::GetCallingFullTokenID(),
        .tokenId = IPCSkeleton::GetCallingTokenID(),
        .uid = IPCSkeleton::GetCallingUid(),
        .pid = IPCSkeleton::GetCallingPid(),
    };
    CHKPR(context_, RET_ERR);
    int32_t ret = context_->GetDelegateTasks().PostSyncTask([&] {
        IPlugin *plugin = LoadPlugin(context.intention);
        CHKPR(plugin, RET_ERR);
        return plugin->Enable(context, data, reply);
    });
    if (ret != RET_OK) {
        FI_HILOGE("Enable failed, ret:%{public}d", ret);
    }
    return ret;
}

int32_t IntentionService::Disable(Intention intention, MessageParcel &data, MessageParcel &reply)
{
    CallingContext context {
        .intention = intention,
        .fullTokenId = IPCSkeleton::GetCallingFullTokenID(),
        .tokenId = IPCSkeleton::GetCallingTokenID(),
        .uid = IPCSkeleton::GetCallingUid(),
        .pid = IPCSkeleton::GetCallingPid(),
    };
    CHKPR(context_, RET_ERR);
    int32_t ret = context_->GetDelegateTasks().PostSyncTask([&] {
        IPlugin *plugin = LoadPlugin(context.intention);
        CHKPR(plugin, RET_ERR);
        return plugin->Disable(context, data, reply);
    });
    if (ret != RET_OK) {
        FI_HILOGE("Disable failed, ret:%{public}d", ret);
    }
    return ret;
}

int32_t IntentionService::Start(Intention intention, MessageParcel &data, MessageParcel &reply)
{
    CallingContext context {
        .intention = intention,
        .fullTokenId = IPCSkeleton::GetCallingFullTokenID(),
        .tokenId = IPCSkeleton::GetCallingTokenID(),
        .uid = IPCSkeleton::GetCallingUid(),
        .pid = IPCSkeleton::GetCallingPid(),
    };
    CHKPR(context_, RET_ERR);
    int32_t ret = context_->GetDelegateTasks().PostSyncTask([&] {
        IPlugin *plugin = LoadPlugin(context.intention);
        CHKPR(plugin, RET_ERR);
        return plugin->Start(context, data, reply);
    });
    if (ret != RET_OK) {
        FI_HILOGE("Start failed, ret:%{public}d", ret);
    }
    return ret;
}

int32_t IntentionService::Stop(Intention intention, MessageParcel &data, MessageParcel &reply)
{
    CallingContext context {
        .intention = intention,
        .fullTokenId = IPCSkeleton::GetCallingFullTokenID(),
        .tokenId = IPCSkeleton::GetCallingTokenID(),
        .uid = IPCSkeleton::GetCallingUid(),
        .pid = IPCSkeleton::GetCallingPid(),
    };
    CHKPR(context_, RET_ERR);
    int32_t ret = context_->GetDelegateTasks().PostSyncTask([&] {
        IPlugin *plugin = LoadPlugin(context.intention);
        CHKPR(plugin, RET_ERR);
        return plugin->Stop(context, data, reply);
    });
    if (ret != RET_OK) {
        FI_HILOGE("Stop failed, ret:%{public}d", ret);
    }
    return ret;
}

int32_t IntentionService::AddWatch(Intention intention, uint32_t id, MessageParcel &data, MessageParcel &reply)
{
    CHKPR(context_, RET_ERR);
    int32_t timerId = HiviewDFX::XCollie::GetInstance().SetTimer("DeviceStatusIntensionServerAddWatch", SERVER_TIMEOUT,
        nullptr, nullptr, HiviewDFX::XCOLLIE_FLAG_LOG);
    CallingContext context {
        .intention = intention,
        .fullTokenId = IPCSkeleton::GetCallingFullTokenID(),
        .tokenId = IPCSkeleton::GetCallingTokenID(),
        .uid = IPCSkeleton::GetCallingUid(),
        .pid = IPCSkeleton::GetCallingPid(),
    };
    int32_t ret = context_->GetDelegateTasks().PostSyncTask([&] {
        IPlugin *plugin = LoadPlugin(context.intention);
        CHKPR(plugin, RET_ERR);
        return plugin->AddWatch(context, id, data, reply);
    });
    if (ret != RET_OK) {
        FI_HILOGE("AddWatch failed, ret:%{public}d", ret);
    }
    HiviewDFX::XCollie::GetInstance().CancelTimer(timerId);
    return ret;
}

int32_t IntentionService::RemoveWatch(Intention intention, uint32_t id, MessageParcel &data, MessageParcel &reply)
{
    CallingContext context {
        .intention = intention,
        .fullTokenId = IPCSkeleton::GetCallingFullTokenID(),
        .tokenId = IPCSkeleton::GetCallingTokenID(),
        .uid = IPCSkeleton::GetCallingUid(),
        .pid = IPCSkeleton::GetCallingPid(),
    };
    CHKPR(context_, RET_ERR);
    int32_t ret = context_->GetDelegateTasks().PostSyncTask([&] {
        IPlugin *plugin = LoadPlugin(context.intention);
        CHKPR(plugin, RET_ERR);
        return plugin->RemoveWatch(context, id, data, reply);
    });
    if (ret != RET_OK) {
        FI_HILOGE("RemoveWatch failed, ret:%{public}d", ret);
    }
    return ret;
}

int32_t IntentionService::SetParam(Intention intention, uint32_t id, MessageParcel &data, MessageParcel &reply)
{
    CallingContext context {
        .intention = intention,
        .fullTokenId = IPCSkeleton::GetCallingFullTokenID(),
        .tokenId = IPCSkeleton::GetCallingTokenID(),
        .uid = IPCSkeleton::GetCallingUid(),
        .pid = IPCSkeleton::GetCallingPid(),
    };
    CHKPR(context_, RET_ERR);
    int32_t ret = context_->GetDelegateTasks().PostSyncTask([&] {
        IPlugin *plugin = LoadPlugin(context.intention);
        CHKPR(plugin, RET_ERR);
        return plugin->SetParam(context, id, data, reply);
    });
    if (ret != RET_OK) {
        FI_HILOGE("SetParam failed, ret:%{public}d", ret);
    }
    return ret;
}

int32_t IntentionService::GetParam(Intention intention, uint32_t id, MessageParcel &data, MessageParcel &reply)
{
    CallingContext context {
        .intention = intention,
        .fullTokenId = IPCSkeleton::GetCallingFullTokenID(),
        .tokenId = IPCSkeleton::GetCallingTokenID(),
        .uid = IPCSkeleton::GetCallingUid(),
        .pid = IPCSkeleton::GetCallingPid(),
    };
    CHKPR(context_, RET_ERR);
    int32_t ret = context_->GetDelegateTasks().PostSyncTask([&] {
        IPlugin *plugin = LoadPlugin(context.intention);
        CHKPR(plugin, RET_ERR);
        return plugin->GetParam(context, id, data, reply);
    });
    if (ret != RET_OK) {
        FI_HILOGE("GetParam failed, ret:%{public}d", ret);
    }
    return ret;
}

int32_t IntentionService::Control(Intention intention, uint32_t id, MessageParcel &data, MessageParcel &reply)
{
    CHKPR(context_, RET_ERR);
    int32_t timerId = HiviewDFX::XCollie::GetInstance().SetTimer("DeviceStatusIntensionServerControl", SERVER_TIMEOUT,
        nullptr, nullptr, HiviewDFX::XCOLLIE_FLAG_LOG);
    CallingContext context {
        .intention = intention,
        .fullTokenId = IPCSkeleton::GetCallingFullTokenID(),
        .tokenId = IPCSkeleton::GetCallingTokenID(),
        .uid = IPCSkeleton::GetCallingUid(),
        .pid = IPCSkeleton::GetCallingPid(),
    };
    int32_t ret = context_->GetDelegateTasks().PostSyncTask([&] {
        IPlugin *plugin = LoadPlugin(context.intention);
        CHKPR(plugin, RET_ERR);
        return plugin->Control(context, id, data, reply);
    });
    if (ret != RET_OK) {
        FI_HILOGE("Control failed, ret:%{public}d", ret);
    }
    HiviewDFX::XCollie::GetInstance().CancelTimer(timerId);
    return ret;
}

IPlugin* IntentionService::LoadPlugin(Intention intention)
{
    CALL_DEBUG_ENTER;
    switch (intention) {
        case Intention::SOCKET: {
            return &socketServer_;
        }
        case Intention::STATIONARY: {
            return &stationary_;
        }
        case Intention::COOPERATE: {
            return &cooperate_;
        }
        case Intention::DRAG: {
            return &drag_;
        }
        case Intention::BOOMERANG: {
            return &boomerang_;
        }
        default: {
            return nullptr;
        }
    }
}
} // namespace DeviceStatus
} // namespace Msdp
} // namespace OHOS
