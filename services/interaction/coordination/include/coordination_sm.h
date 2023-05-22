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

#ifndef COORDINATION_SM_H
#define COORDINATION_SM_H

#include <functional>

#include "singleton.h"

#include "devicestatus_define.h"
#include "device_manager_callback.h"
#include "distributed_input_adapter.h"
#include "dm_device_info.h"
#include "input_manager.h"
#include "i_coordination_state.h"
#include "i_input_event_consumer.h"
#include "i_input_event_filter.h"

namespace OHOS {
namespace Msdp {
namespace DeviceStatus {
enum class CoordinationState {
    STATE_FREE = 0,
    STATE_IN = 1,
    STATE_OUT = 2,
};

enum class CoordinationMsg {
    COORDINATION_ON_SUCCESS = 0,
    COORDINATION_ON_FAIL = 1,
    COORDINATION_OFF_SUCCESS = 2,
    COORDINATION_OFF_FAIL = 3,
    COORDINATION_START = 4,
    COORDINATION_START_SUCCESS = 5,
    COORDINATION_START_FAIL = 6,
    COORDINATION_STOP = 7,
    COORDINATION_STOP_SUCCESS = 8,
    COORDINATION_STOP_FAIL = 9,
    COORDINATION_NULL = 10,
};

enum class CooStateChangeType {
    STATE_NONE = -1,
    STATE_FREE_TO_IN = 0,
    STATE_FREE_TO_OUT = 1,
    STATE_IN_TO_FREE = 2,
    STATE_OUT_TO_FREE = 3,
};

struct PointerFilter : public MMI::IInputEventFilter {
    bool OnInputEvent(std::shared_ptr<MMI::KeyEvent> keyEvent) const override
    {
        return false;
    }

    bool OnInputEvent(std::shared_ptr<MMI::PointerEvent> pointerEvent) const override
    {
        if (pointerEvent == nullptr) {
            return false;
        }
        if (pointerEvent->GetPointerAction() == MMI::PointerEvent::POINTER_ACTION_BUTTON_DOWN) {
            OHOS::MMI::InputManager::GetInstance()->RemoveInputEventFilter(filterId_);
            filterId_ = -1;
            return true;
        }
        return false;
    }

    inline void UpdateCurrentFilterId(int32_t filterId)
    {
        filterId_ = filterId;
    }
private:
    mutable int32_t filterId_ { -1 };
};

class CoordinationSM final {
    DECLARE_DELAYED_SINGLETON(CoordinationSM);

    class DeviceInitCallBack : public DistributedHardware::DmInitCallback {
        void OnRemoteDied() override;
    };

    class DmDeviceStateCallback : public DistributedHardware::DeviceStateCallback {
        void OnDeviceOnline(const DistributedHardware::DmDeviceInfo &deviceInfo) override;
        void OnDeviceChanged(const DistributedHardware::DmDeviceInfo &deviceInfo) override;
        void OnDeviceReady(const DistributedHardware::DmDeviceInfo &deviceInfo) override;
        void OnDeviceOffline(const DistributedHardware::DmDeviceInfo &deviceInfo) override;
    };

    class InterceptorConsumer : public MMI::IInputEventConsumer {
    public:
        void OnInputEvent(std::shared_ptr<MMI::KeyEvent> keyEvent) const override;
        void OnInputEvent(std::shared_ptr<MMI::PointerEvent> pointerEvent) const override;
        void OnInputEvent(std::shared_ptr<MMI::AxisEvent> axisEvent) const override;
    };

    class MonitorConsumer : public MMI::IInputEventConsumer {
    public:
        explicit MonitorConsumer(std::function<void (std::shared_ptr<MMI::PointerEvent>)> cb) : callback_(cb) {}
        void OnInputEvent(std::shared_ptr<MMI::KeyEvent> keyEvent) const override;
        void OnInputEvent(std::shared_ptr<MMI::PointerEvent> pointerEvent) const override;
        void OnInputEvent(std::shared_ptr<MMI::AxisEvent> axisEvent) const override;
    private:
        std::function<void (std::shared_ptr<MMI::PointerEvent>)> callback_ { nullptr };
    };

public:
    void SetAbsolutionLocation(double xPercent, double yPercent);
    DISALLOW_COPY_AND_MOVE(CoordinationSM);
    void Init();
    void OnSessionLost(SessionPtr session);
    void PrepareCoordination();
    void UnprepareCoordination();
    int32_t ActivateCoordination(const std::string &remoteNetworkId, int32_t startDeviceId);
    int32_t DeactivateCoordination(bool isUnchained);
    int32_t GetCoordinationState(const std::string &deviceId);
    void StartRemoteCoordination(const std::string &remoteNetworkId, bool buttonIsPressed);
    void StartPointerEventFilter();
    void StartRemoteCoordinationResult(bool isSuccess,
        const std::string &startDeviceDhid, int32_t xPercent, int32_t yPercent);
    void StopRemoteCoordination(bool isUnchained);
    void StopRemoteCoordinationResult(bool isSuccess);
    void StartCoordinationOtherResult(const std::string &remoteNetworkId);
    void UpdateState(CoordinationState state);
    void UpdatePreparedDevices(const std::string &remoteNetworkId, const std::string &originNetworkId);
    std::pair<std::string, std::string> GetPreparedDevices() const;
    CoordinationState GetCurrentCoordinationState() const;
    void OnCoordinationChanged(const std::string &networkId, bool isOpen);
    void OnKeyboardOnline(const std::string &dhid);
    void OnPointerOffline(const std::string &dhid, const std::vector<std::string> &keyboards);
    void OnKeyboardOffline(const std::string &dhid);
    bool InitDeviceManager();
    void OnDeviceOnline(const std::string &networkId);
    void OnDeviceOffline(const std::string &networkId);
    void OnStartFinish(bool isSuccess, const std::string &remoteNetworkId, int32_t startDeviceId);
    void OnStopFinish(bool isSuccess, const std::string &remoteNetworkId);
    bool IsStarting() const;
    bool IsStopping() const;
    void Reset(const std::string &networkId);
    void Dump(int32_t fd);
    std::string GetDeviceCoordinationState(CoordinationState value) const;
    void UpdateLastPointerEventCallback(std::shared_ptr<MMI::PointerEvent> pointerEvent);
    std::shared_ptr<MMI::PointerEvent> GetLastPointerEvent() const;
    void RemoveMonitor();
    void RemoveInterceptor();
    bool IsNeedFilterOut(const std::string &deviceId, const std::shared_ptr<MMI::KeyEvent> keyEvent);
    void RegisterStateChange(CooStateChangeType type,
        std::function<void(CoordinationState, CoordinationState)> callback);
    std::string GetRemoteId() const;
    void UnchainCoordination(const std::string &localNetworkId, const std::string &remoteNetworkId);
    void SetUnchainStatus(bool isUnchained);
    void NotifySessionClosed();
    void SetSinkNetworkId(const std::string &sinkNetworkId);
    void RegisterRemoteNetworkId(std::function<void(std::string)> callback);
    void RegisterMouseLocation(std::function<void(int32_t, int32_t)> callback);

private:
    void Reset(bool adjustAbsolutionLocation = false);
    void OnCloseCoordination(const std::string &networkId, bool isLocal);
    void NotifyRemoteStartFail(const std::string &remoteNetworkId);
    void NotifyRemoteStartSuccess(const std::string &remoteNetworkId, const std::string &startDeviceDhid);
    void NotifyRemoteStopFinish(bool isSuccess, const std::string &remoteNetworkId);
    bool UpdateMouseLocation();
    void StateChangedNotify(CoordinationState oldState, CoordinationState newState);
    void ChangeNotify(CooStateChangeType type, CoordinationState oldState, CoordinationState newState);
    void NotifyRemoteNetworkId(const std::string &remoteNetworkId);
    void NotifyMouseLocation(int32_t x, int32_t y);

private:
    std::shared_ptr<ICoordinationState> currentStateSM_ { nullptr };
    std::pair<std::string, std::string> preparedNetworkId_;
    std::string startDeviceDhid_;
    std::string remoteNetworkId_;
    std::string sinkNetworkId_;
    bool isUnchained_ { false };
    CoordinationState coordinationState_ { CoordinationState::STATE_FREE };
    std::shared_ptr<DistributedHardware::DmInitCallback> initCallback_ { nullptr };
    std::shared_ptr<DistributedHardware::DeviceStateCallback> stateCallback_ { nullptr };
    std::vector<std::string> onlineDevice_;
    mutable std::mutex mutex_;
    std::atomic<bool> isStarting_ { false };
    std::atomic<bool> isStopping_ { false };
    std::pair<int32_t, int32_t> mouseLocation_ { std::make_pair(0, 0) };
    std::shared_ptr<MMI::PointerEvent> lastPointerEvent_ { nullptr };
    int32_t displayX_ { -1 };
    int32_t displayY_ { -1 };
    int32_t interceptorId_ { -1 };
    int32_t monitorId_ { -1 };
    int32_t filterId_ { -1 };
    std::map<CooStateChangeType, std::function<void(CoordinationState, CoordinationState)>> stateChangedCallbacks_;
    std::function<void(std::string)> remoteNetworkIdCallback_;
    std::function<void(int32_t, int32_t)> mouseLocationCallback_;
};

#define DIS_HARDWARE DistributedHardware::DeviceManager::GetInstance()
#define COOR_SM OHOS::DelayedSingleton<CoordinationSM>::GetInstance()
} // namespace DeviceStatus
} // namespace Msdp
} // namespace OHOS
#endif // COORDINATION_SM_H
