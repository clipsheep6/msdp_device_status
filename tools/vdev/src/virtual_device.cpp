/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "virtual_device.h"

#include <iostream>
#include <map>

#include <unistd.h>

#include "devicestatus_define.h"
#include "fi_log.h"

namespace OHOS {
namespace Msdp {
namespace DeviceStatus {
namespace {
constexpr ::OHOS::HiviewDFX::HiLogLabel LABEL { LOG_CORE, MSDP_DOMAIN_ID, "VirtualDevice" };
constexpr size_t DEFAULT_BUF_SIZE = 1024;
constexpr int32_t MINIMUM_INTERVAL_ALLOWED = 1;
constexpr int32_t MAXIMUM_INTERVAL_ALLOWED = 100;
} // namespace

VirtualDevice::VirtualDevice(const std::string &node)
{
    inputDev_ = std::make_unique<VInputDevice>(node);
    inputDev_->Open();
}

bool VirtualDevice::FindDeviceNode(const std::string &name, std::string &node)
{
    CALL_DEBUG_ENTER;
    std::map<std::string, std::string> nodes;
    GetInputDeviceNodes(nodes);
    FI_HILOGD("There are %{public}zu device nodes.", nodes.size());

    std::map<std::string, std::string>::const_iterator cItr = nodes.find(name);
    if (cItr == nodes.cend()) {
        FI_HILOGE("No virtual stylus is found.");
        return false;
    }
    FI_HILOGD("Node name : \'%{public}s\'.", cItr->second.c_str());
    std::ostringstream ss;
    ss << "/dev/input/" << cItr->second;
    node = ss.str();
    return true;
}

void VirtualDevice::Execute(const std::string &command, std::vector<std::string> &results)
{
    CALL_DEBUG_ENTER;
    char comRealPath[PATH_MAX] = {};
    if (realpath(command.c_str(), comRealPath) == nullptr) {
        FI_HILOGE("Invalid command:%{public}s", command.c_str());
        return;
    }
    FI_HILOGD("Execute comRealPath:%{public}s.", comRealPath);
    char buffer[DEFAULT_BUF_SIZE] {};
    FILE *pin = popen(comRealPath, "r");
    if (pin == nullptr) {
        FI_HILOGE("Failed to popen command.");
        return;
    }
    while (!feof(pin)) {
        if (fgets(buffer, sizeof(buffer), pin) != nullptr) {
            results.push_back(buffer);
        }
    }
    FI_HILOGD("Close phandle.");
    pclose(pin);
}

void VirtualDevice::GetInputDeviceNodes(std::map<std::string, std::string> &nodes)
{
    CALL_DEBUG_ENTER;
    std::string command = "cat /proc/bus/input/devices";
    std::vector<std::string> results;
    Execute(command, results);
    if (results.empty()) {
        FI_HILOGE("Failed to list devices.");
        return;
    }
    const std::string kname { "Name=\"" };
    const std::string kevent { "event" };
    std::string name;
    for (const auto &item : results) {
        FI_HILOGD("item:%{public}s.", item.c_str());
        if (item[0] == 'N') {
            std::string::size_type spos = item.find(kname);
            if (spos != std::string::npos) {
                spos += kname.size();
                std::string::size_type tpos = item.find("\"", spos);
                if (tpos != std::string::npos) {
                    name = item.substr(spos, tpos - spos);
                }
            }
        } else if (!name.empty() && (item[0] == 'H')) {
            std::string::size_type spos = item.find(kevent);
            if (spos != std::string::npos) {
                std::map<std::string, std::string>::const_iterator cItr = nodes.find(name);
                if (cItr != nodes.end()) {
                    nodes.erase(cItr);
                }
                std::string::size_type tpos = spos + kevent.size();
                while (std::isalnum(item[tpos])) {
                    ++tpos;
                }
                nodes.emplace(name, item.substr(spos, tpos - spos));
                name.clear();
            }
        }
    }
}

int32_t VirtualDevice::SendEvent(uint16_t type, uint16_t code, int32_t value)
{
    CALL_DEBUG_ENTER;
    CHKPR(inputDev_, RET_ERR);
    inputDev_->SendEvent(type, code, value);

    if ((type == EV_SYN) && (code == SYN_REPORT)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(minimumInterval_));
    }
    return RET_OK;
}

std::string VirtualDevice::GetName() const
{
    if (!name_.empty()) {
        return name_;
    }
    if (inputDev_ != nullptr) {
        return inputDev_->GetName();
    }
    return {};
}

void VirtualDevice::SetMinimumInterval(int32_t interval)
{
    minimumInterval_ = std::max(MINIMUM_INTERVAL_ALLOWED, std::min(interval, MAXIMUM_INTERVAL_ALLOWED));
}
} // namespace DeviceStatus
} // namespace Msdp
} // namespace OHOS