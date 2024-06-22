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

#ifndef COOPERATE_INPUT_DEVICE_MANAGER_H
#define COOPERATE_INPUT_DEVICE_MANAGER_H

#include <memory>
#include <mutex>
#include <set>
#include <unordered_map>

#include "nocopyable.h"

#include "channel.h"
#include "cooperate_events.h"
#include "i_context.h"
#include "net_packet.h"

namespace OHOS {
namespace Msdp {
namespace DeviceStatus {
namespace Cooperate {
class InputDeviceMgr {
class DSoftbusObserver final : public IDSoftbusObserver {
    public:
        DSoftbusObserver(InputDeviceMgr &parent) : parent_(parent) {}
        ~DSoftbusObserver() = default;

        void OnBind(const std::string &networkId) override {}

        void OnShutdown(const std::string &networkId) override {}

        void OnConnected(const std::string &networkId) override {}

        bool OnPacket(const std::string &networkId, Msdp::NetPacket &packet) override
        {
            return parent_.OnPacket(networkId, packet);
        }

        bool OnRawData(const std::string &networkId, const void *data, uint32_t dataLen) override
        {
            return parent_.OnRawData(networkId, data, dataLen);
        }

    private:
        InputDeviceMgr &parent_;
    };

public:
    InputDeviceMgr(IContext *context);
    ~InputDeviceMgr() = default;
    DISALLOW_COPY_AND_MOVE(InputDeviceMgr);

public:
    void Enable(Channel<CooperateEvent>::Sender sender);
    void Disable();
    bool OnRawData(const std::string &networkId, const void *data, uint32_t dataLen);
    bool OnPacket(const std::string &networkId, Msdp::NetPacket &packet);
    void OnSoftbusSessionOpened(const DSoftbusSessionOpened &notice);
    void OnSoftbusSessionClosed(const DSoftbusSessionClosed &notice);
    void OnLocalHotPlug(const InputHotplugEvent &notice);
    void AddVirtualInputDevice(const std::string &networkId);
    void RemoveVirtualInputDevice(const std::string &networkId);
    void OnRemoteHotPlugIn(const RemoteHotPlugEvent &notice);
    void OnRemoteHotUnPlug(const RemoteHotPlugEvent &notice);

private:
    void OnRemoteInputDevice(const std::string &networkId, NetPacket &packet);
    void OnRemoteHotPlug(const std::string &networkId, NetPacket &packet);

    void NotifyInputDeviceToRemote(const std::string &remoteNetworkId);
    void BroadcastHotPlugToRemote(const InputHotplugEvent &notice);

    void AddRemoteInputDevice(const std::string &networkId, std::shared_ptr<IDevice> device);
    void RemoveRemoteInputDevice(const std::string &networkId, std::shared_ptr<IDevice> device);
    void RemoveAllRemoteInputDevice(const std::string &networkId);
    void DumpRemoteInputDevice(const std::string &networkId);
    int32_t SerializeDevice(std::shared_ptr<IDevice> device, NetPacket &packet);
    int32_t DeserializeDevice(std::shared_ptr<IDevice> device, NetPacket &packet);
    std::shared_ptr<MMI::InputDevice> Transform(std::shared_ptr<IDevice> device);
    void AddVirtualInputDevice(const std::string &networkId, int32_t remoteDeviceId);
    void RemoveVirtualInputDevice(const std::string &networkId, int32_t remoteDeviceId);
    void DispDeviceInfo(std::shared_ptr<IDevice> device);
    std::shared_ptr<IDevice> GetRemoteDeviceById(const std::string &networkId, int32_t remoteDeviceId);

private:
    std::mutex mutex_;
    bool enable_ { false };
    IContext *env_ { nullptr };
    Channel<CooperateEvent>::Sender sender_;
    std::shared_ptr<DSoftbusObserver> observer_;
    struct IDeviceCmp {
        bool operator()(const std::shared_ptr<IDevice> &one, const std::shared_ptr<IDevice> &other) const
        {
            return one->GetId() < other->GetId();
        }
    };
    std::unordered_map<std::string, std::set<std::shared_ptr<IDevice>, IDeviceCmp>> remoteDevices_;
    std::unordered_map<std::string, std::set<int32_t>> virtualInputDevicesAdded_;
    std::unordered_map<int32_t, int32_t> remote2VirtualIds_;
};

} // namespace Cooperate
} // namespace DeviceStatus
} // namespace Msdp
} // namespace OHOS
#endif // COOPERATE_INPUT_DEVICE_MANAGER_H
