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

#ifndef OHOS_MSDP_DEVICESTATUS_AGENT_H
#define OHOS_MSDP_DEVICESTATUS_AGENT_H

#include <memory>

#include "devicestatus_callback_stub.h"
#include "devicestatus_data_utils.h"

namespace OHOS {
namespace Msdp {
namespace DeviceStatus {
class DeviceStatusAgent : public std::enable_shared_from_this<DeviceStatusAgent> {
public:
    DeviceStatusAgent() {};
    ~DeviceStatusAgent() {};
    class DeviceStatusAgentEvent {
    public:
        virtual ~DeviceStatusAgentEvent() = default;
        virtual bool OnEventResult(const DataUtils::Data& devicestatusData) = 0;
    };

    class DeviceStatusAgentCallback : public DevicestatusCallbackStub {
    public:
        explicit DeviceStatusAgentCallback(std::shared_ptr<DeviceStatusAgent> agent) : agent_(agent) {};
        virtual ~DeviceStatusAgentCallback() {};
        void OnDevicestatusChanged(const DataUtils::Data& devicestatusData) override;
    private:
        std::weak_ptr<DeviceStatusAgent> agent_;
    };

    int32_t SubscribeAgentEvent(const DataUtils::Type& type,
        const DataUtils::ActivityEvent& event,
        const DataUtils::ReportLatencyNs& latency,
        const std::shared_ptr<DeviceStatusAgent::DeviceStatusAgentEvent>& agentEvent);
    int32_t UnSubscribeAgentEvent(const DataUtils::Type& type,
        const DataUtils::ActivityEvent& event);
    friend class DeviceStatusAgentCallback;
private:
    void RegisterServiceEvent(const DataUtils::Type& type, \
        const DataUtils::ActivityEvent& event, \
        const DataUtils::ReportLatencyNs& latency);
    void UnRegisterServiceEvent(const DataUtils::Type& type, \
    const DataUtils::ActivityEvent& event);
    sptr<IdevicestatusCallback> callback_;
    std::shared_ptr<DeviceStatusAgentEvent> agentEvent_;
};
} // namespace DeviceStatus
} // namespace Msdp
} // namespace OHOS

#endif // OHOS_MSDP_DEVICESTATUS_AGENT_H
