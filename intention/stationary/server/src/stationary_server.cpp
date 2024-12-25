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

#include "stationary_server.h"
#ifdef MSDP_HIVIEWDFX_HISYSEVENT_ENABLE
#include "hisysevent.h"
#endif // MSDP_HIVIEWDFX_HISYSEVENT_ENABLE
#ifdef MSDP_HIVIEWDFX_HITRACE_ENABLE
#include "hitrace_meter.h"
#endif // MSDP_HIVIEWDFX_HITRACE_ENABLE

#include "default_params.h"
#include "devicestatus_define.h"
#include "devicestatus_dumper.h"
#include "devicestatus_hisysevent.h"
#include "stationary_params.h"

#undef LOG_TAG
#define LOG_TAG "StationaryServer"

namespace OHOS {
namespace Msdp {
namespace DeviceStatus {

StationaryServer::StationaryServer()
{
    manager_.Init();
}

int32_t StationaryServer::Enable(CallingContext &context, MessageParcel &data, MessageParcel &reply)
{
    CALL_DEBUG_ENTER;
    return RET_ERR;
}

int32_t StationaryServer::Disable(CallingContext &context, MessageParcel &data, MessageParcel &reply)
{
    CALL_DEBUG_ENTER;
    return RET_ERR;
}

int32_t StationaryServer::Start(CallingContext &context, MessageParcel &data, MessageParcel &reply)
{
    CALL_DEBUG_ENTER;
    return RET_ERR;
}

int32_t StationaryServer::Stop(CallingContext &context, MessageParcel &data, MessageParcel &reply)
{
    CALL_DEBUG_ENTER;
    return RET_ERR;
}

int32_t StationaryServer::WithOptionsStart(CallingContext &context, MessageParcel &data, MessageParcel &reply)
{
    CALL_DEBUG_ENTER;
    return RET_ERR;
}

int32_t StationaryServer::AddWatch(CallingContext &context, uint32_t id, MessageParcel &data, MessageParcel &reply)
{
    if (id != StationaryRequestID::SUBSCRIBE_STATIONARY) {
        FI_HILOGE("Unexpected request ID (%{public}u)", id);
        return RET_ERR;
    }
    SubscribeStationaryParam param {};

    if (!param.Unmarshalling(data)) {
        FI_HILOGE("SubscribeStationaryParam::Unmarshalling fail");
        return RET_ERR;
    }
    Subscribe(context, param.type_, param.event_, param.latency_, param.callback_);
    return RET_OK;
}

int32_t StationaryServer::RemoveWatch(CallingContext &context, uint32_t id, MessageParcel &data, MessageParcel &reply)
{
    if (id != StationaryRequestID::UNSUBSCRIBE_STATIONARY) {
        FI_HILOGE("Unexpected request ID (%{public}u)", id);
        return RET_ERR;
    }
    UnsubscribeStationaryParam param {};

    if (!param.Unmarshalling(data)) {
        FI_HILOGE("UnsubscribeStationaryParam::Unmarshalling fail");
        return RET_ERR;
    }
    Unsubscribe(context, param.type_, param.event_, param.callback_);
    return RET_OK;
}

int32_t StationaryServer::SetParam(CallingContext &context, uint32_t id, MessageParcel &data, MessageParcel &reply)
{
    CALL_DEBUG_ENTER;
    return RET_ERR;
}

int32_t StationaryServer::GetParam(CallingContext &context, uint32_t id, MessageParcel &data, MessageParcel &reply)
{
    if (id != StationaryRequestID::GET_STATIONARY_DATA) {
        FI_HILOGE("Unexpected request ID (%{public}u)", id);
        return RET_ERR;
    }
    GetStaionaryDataParam param {};

    if (!param.Unmarshalling(data)) {
        FI_HILOGE("GetStaionaryDataParam::Unmarshalling fail");
        return RET_ERR;
    }
    GetStaionaryDataReply dataReply { GetCache(context, param.type_) };
    if (!dataReply.Marshalling(reply)) {
        FI_HILOGE("GetStaionaryDataReply::Marshalling fail");
        return RET_ERR;
    }
    return RET_OK;
}

int32_t StationaryServer::Control(CallingContext &context, uint32_t id, MessageParcel &data, MessageParcel &reply)
{
    CALL_DEBUG_ENTER;
    return RET_ERR;
}

void StationaryServer::DumpDeviceStatusSubscriber(int32_t fd) const
{
    DS_DUMPER->DumpDeviceStatusSubscriber(fd);
}

void StationaryServer::DumpDeviceStatusChanges(int32_t fd) const
{
    DS_DUMPER->DumpDeviceStatusChanges(fd);
}

void StationaryServer::DumpCurrentDeviceStatus(int32_t fd)
{
    std::vector<Data> datas;

    for (auto type = TYPE_ABSOLUTE_STILL; type <= TYPE_LID_OPEN; type = static_cast<Type>(type + 1)) {
        Data data = manager_.GetLatestDeviceStatusData(type);
        if (data.value != OnChangedValue::VALUE_INVALID) {
            datas.emplace_back(data);
        }
    }
    DS_DUMPER->DumpDeviceStatusCurrentStatus(fd, datas);
}

void StationaryServer::Subscribe(CallingContext &context, Type type, ActivityEvent event,
    ReportLatencyNs latency, sptr<IRemoteDevStaCallback> callback)
{
    FI_HILOGI("SubscribeStationary(type:%{public}d,event:%{public}d,latency:%{public}d)", type, event, latency);
    auto appInfo = std::make_shared<AppInfo>();
    appInfo->uid = context.uid;
    appInfo->pid = context.pid;
    appInfo->tokenId = context.tokenId;
    manager_.GetPackageName(appInfo->tokenId, appInfo->packageName);
    appInfo->type = type;
    appInfo->callback = callback;
    DS_DUMPER->SaveAppInfo(appInfo);
#ifdef MSDP_HIVIEWDFX_HITRACE_ENABLE
    StartTrace(HITRACE_TAG_MSDP, "serviceSubscribeStart");
#endif // MSDP_HIVIEWDFX_HITRACE_ENABLE
    manager_.Subscribe(type, event, latency, callback);
#ifdef MSDP_HIVIEWDFX_HITRACE_ENABLE
    FinishTrace(HITRACE_TAG_MSDP);
#endif // MSDP_HIVIEWDFX_HITRACE_ENABLE
    ReportSensorSysEvent(context, type, true);
    WriteSubscribeHiSysEvent(appInfo->uid, appInfo->packageName, type);
}

void StationaryServer::Unsubscribe(CallingContext &context, Type type,
    ActivityEvent event, sptr<IRemoteDevStaCallback> callback)
{
    FI_HILOGI("UnsubscribeStationary(type:%{public}d,event:%{public}d)", type, event);
    auto appInfo = std::make_shared<AppInfo>();
    appInfo->uid = context.uid;
    appInfo->pid = context.pid;
    appInfo->tokenId = context.tokenId;
    appInfo->packageName = DS_DUMPER->GetPackageName(appInfo->tokenId);
    appInfo->type = type;
    appInfo->callback = callback;
    DS_DUMPER->RemoveAppInfo(appInfo);
#ifdef MSDP_HIVIEWDFX_HITRACE_ENABLE
    StartTrace(HITRACE_TAG_MSDP, "serviceUnSubscribeStart");
#endif // MSDP_HIVIEWDFX_HITRACE_ENABLE
    manager_.Unsubscribe(type, event, callback);
#ifdef MSDP_HIVIEWDFX_HITRACE_ENABLE
    FinishTrace(HITRACE_TAG_MSDP);
#endif // MSDP_HIVIEWDFX_HITRACE_ENABLE
    ReportSensorSysEvent(context, type, false);
    WriteUnSubscribeHiSysEvent(appInfo->uid, appInfo->packageName, type);
}

Data StationaryServer::GetCache(CallingContext &context, const Type &type)
{
    return manager_.GetLatestDeviceStatusData(type);
}

void StationaryServer::ReportSensorSysEvent(CallingContext &context, int32_t type, bool enable)
{
#ifdef MSDP_HIVIEWDFX_HISYSEVENT_ENABLE
    std::string packageName;
    manager_.GetPackageName(context.tokenId, packageName);
    int32_t ret = HiSysEventWrite(
        OHOS::HiviewDFX::HiSysEvent::Domain::MSDP,
        std::string(enable ? "Subscribe" : "Unsubscribe"),
        OHOS::HiviewDFX::HiSysEvent::EventType::STATISTIC,
        "UID", context.uid,
        "PKGNAME", packageName,
        "TYPE", type);
    if (ret != 0) {
        FI_HILOGE("HiviewDFX write failed, error:%{public}d", ret);
    }
#endif // MSDP_HIVIEWDFX_HISYSEVENT_ENABLE
}
} // namespace DeviceStatus
} // namespace Msdp
} // namespace OHOS