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

#ifndef DEVICESTATUS_SRV_PROXY_H
#define DEVICESTATUS_SRV_PROXY_H

#include "iremote_proxy.h"
#include <nocopyable.h>
#include "refbase.h"

#include "drag_data.h"
#include "idevicestatus.h"
#include "i_drag_stop_callback.h"

namespace OHOS {
namespace Msdp {
namespace DeviceStatus {
class DeviceStatusSrvProxy : public IRemoteProxy<Idevicestatus> {
public:
    explicit DeviceStatusSrvProxy(const sptr<IRemoteObject>& impl)
        : IRemoteProxy<Idevicestatus>(impl) {}
    ~DeviceStatusSrvProxy() = default;
    DISALLOW_COPY_AND_MOVE(DeviceStatusSrvProxy);

    virtual void Subscribe(Type type, ActivityEvent event, ReportLatencyNs latency,
        sptr<IRemoteDevStaCallback> callback) override;
    virtual void Unsubscribe(Type type, ActivityEvent event,
        sptr<IRemoteDevStaCallback> callback) override;
    virtual Data GetCache(const Type& type) override;
    int32_t AllocSocketFd(const std::string &programName, const int32_t moduleType,
        int32_t &socketFd, int32_t &tokenType) override;

    virtual int32_t RegisterCoordinationListener() override;
    virtual int32_t UnregisterCoordinationListener() override;
    virtual int32_t EnableCoordination(int32_t userData, bool enabled) override;
    virtual int32_t StartCoordination(int32_t userData, const std::string &remoteNetworkId,
        int32_t startDeviceId) override;
    virtual int32_t StopCoordination(int32_t userData) override;
    virtual int32_t GetCoordinationState(int32_t userData, const std::string &deviceId) override;

    virtual int32_t StartDrag(const DragData &dragData, sptr<IDragStopCallback> callback) override;
    virtual int32_t StopDrag(DragResult result, bool hasCustomAnimation) override;
    virtual int32_t UpdateDragStyle(DragCursorStyle style) override;
    virtual int32_t GetDragTargetPid() override;
    virtual int32_t GetUdKey(std::string &udKey) override;
    virtual int32_t AddDraglistener() override;
    virtual int32_t RemoveDraglistener() override;
    virtual int32_t SetDragWindowVisible(bool visible) override;
    virtual int32_t GetShadowOffset(int32_t& offsetX, int32_t& offsetY, int32_t& width, int32_t& height) override;

private:
    static inline BrokerDelegator<DeviceStatusSrvProxy> delegator_;
};
} // namespace DeviceStatus
} // namespace Msdp
} // namespace OHOS
#endif // DEVICESTATUS_SRV_PROXY_H
