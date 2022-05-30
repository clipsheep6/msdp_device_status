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

#include "devicestatus_service.h"

#include <vector>
#include <ipc_skeleton.h>
#include "if_system_ability_manager.h"
#include "iservice_registry.h"
#include "string_ex.h"
#include "system_ability_definition.h"
#include "devicestatus_permission.h"
#include "devicestatus_common.h"
#include "devicestatus_dumper.h"

namespace OHOS {
namespace Msdp {
namespace {
auto ms = DelayedSpSingleton<DevicestatusService>::GetInstance();
const bool G_REGISTER_RESULT = SystemAbility::MakeAndRegisterAbility(ms.GetRefPtr());
}
DevicestatusService::DevicestatusService() : SystemAbility(MSDP_DEVICESTATUS_SERVICE_ID, true)
{
    DEV_HILOGD(SERVICE, "Add SystemAbility");
}

DevicestatusService::~DevicestatusService() {}

void DevicestatusService::OnDump()
{
    DEV_HILOGI(SERVICE, "OnDump");
}

void DevicestatusService::OnStart()
{
    DEV_HILOGI(SERVICE, "Enter");
    if (ready_) {
        DEV_HILOGE(SERVICE, "OnStart is ready, nothing to do");
        return;
    }

    if (!Init()) {
        DEV_HILOGE(SERVICE, "OnStart call init fail");
        return;
    }
    if (!Publish(DelayedSpSingleton<DevicestatusService>::GetInstance())) {
        DEV_HILOGE(SERVICE, "OnStart register to system ability manager failed");
        return;
    }
    ready_ = true;
    DEV_HILOGI(SERVICE, "OnStart and add system ability success");
}

void DevicestatusService::OnStop()
{
    DEV_HILOGI(SERVICE, "Enter");
    if (!ready_) {
        return;
    }
    ready_ = false;

    if (devicestatusManager_ == nullptr) {
        DEV_HILOGI(SERVICE, "devicestatusManager_ is null");
        return;
    }
    devicestatusManager_->UnloadAlgorithm(false);
    DEV_HILOGI(SERVICE, "unload algorithm library exit");
}

int DevicestatusService::Dump(int fd, const std::vector<std::u16string>& args)
{
    DEV_HILOGI(SERVICE, "dump DeviceStatusServiceInfo");
    DevicestatusDumper &deviceStatusDumper = DevicestatusDumper::GetInstance();

    std::vector<std::string> params;
    for (auto& arg : args) {
        params.emplace_back(Str16ToStr8(arg));
    }

    std::string dumpInfo;
    if (params.empty()) {
        deviceStatusDumper.DumpIllegalArgsInfo(fd);
    }

    if (params[0] == ARG_DUMP_HELP) {
        deviceStatusDumper.DumpHelpInfo(fd);
    } else if (params[0] == ARG_DUMP_DEVICESTATUS_SUBSCRIBER) {
        if (devicestatusManager_ == nullptr) {
            DEV_HILOGI(SERVICE, "Dump func is nullptr");
            return ERR_NG;
        }
        deviceStatusDumper.DumpDevicestatusSubscriber(fd, devicestatusManager_->GetListenerMap());
    } else if (params[0] == ARG_DUMP_DEVICESTATUS_CURRENT_STATE) {
        DevicestatusDataUtils::DevicestatusType type;
        std::vector<DevicestatusDataUtils::DevicestatusData> datas;
        for(type = DevicestatusDataUtils::TYPE_HIGH_STILL;
            type <= DevicestatusDataUtils::TYPE_LID_OPEN;
            type = (DevicestatusDataUtils::DevicestatusType)(type+1)) {
            DevicestatusDataUtils::DevicestatusData data = GetCache(type);
            datas.emplace_back(data);
        }
        deviceStatusDumper.DumpDevicestatusCurrentStatus(fd, datas);
    } else {
        deviceStatusDumper.DumpIllegalArgsInfo(fd);
    }
    return ERR_OK;
}


bool DevicestatusService::Init()
{
    DEV_HILOGI(SERVICE, "Enter");

    if (!devicestatusManager_) {
        devicestatusManager_ = std::make_shared<DevicestatusManager>(ms);
    }
    if (!devicestatusManager_->Init()) {
        DEV_HILOGE(SERVICE, "OnStart init fail");
        return false;
    }

    return true;
}

bool DevicestatusService::IsServiceReady()
{
    DEV_HILOGI(SERVICE, "Enter");
    return ready_;
}

std::shared_ptr<DevicestatusManager> DevicestatusService::GetDevicestatusManager()
{
    DEV_HILOGI(SERVICE, "Enter");
    return devicestatusManager_;
}

void DevicestatusService::Subscribe(const DevicestatusDataUtils::DevicestatusType& type,
    const sptr<IdevicestatusCallback>& callback)
{
    DEV_HILOGI(SERVICE, "Enter");
    if (devicestatusManager_ == nullptr) {
        DEV_HILOGI(SERVICE, "UnSubscribe func is nullptr");
        return;
    }
    devicestatusManager_->Subscribe(type, callback);
}

void DevicestatusService::UnSubscribe(const DevicestatusDataUtils::DevicestatusType& type,
    const sptr<IdevicestatusCallback>& callback)
{
    DEV_HILOGI(SERVICE, "Enter");
    if (devicestatusManager_ == nullptr) {
        DEV_HILOGI(SERVICE, "UnSubscribe func is nullptr");
        return;
    }
    devicestatusManager_->UnSubscribe(type, callback);
}

DevicestatusDataUtils::DevicestatusData DevicestatusService::GetCache(const \
    DevicestatusDataUtils::DevicestatusType& type)
{
    DEV_HILOGI(SERVICE, "Enter");
    if (devicestatusManager_ == nullptr) {
        DevicestatusDataUtils::DevicestatusData data = {type, DevicestatusDataUtils::DevicestatusValue::VALUE_EXIT};
        data.value = DevicestatusDataUtils::DevicestatusValue::VALUE_INVALID;
        DEV_HILOGI(SERVICE, "GetLatestDevicestatusData func is nullptr,return default!");
        return data;
    }
    return devicestatusManager_->GetLatestDevicestatusData(type);
}
} // namespace Msdp
} // namespace OHOS
