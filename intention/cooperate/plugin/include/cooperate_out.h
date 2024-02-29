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

#ifndef COOPERATE_OUT_H
#define COOPERATE_OUT_H

#include "i_cooperate_state.h"

namespace OHOS {
namespace Msdp {
namespace DeviceStatus {
namespace Cooperate {
class CooperateOut : public ICooperateState {
public:
    CooperateOut(IStateMachine &parent, IContext *env);
    ~CooperateOut();

    void OnEvent(Context &context, const CooperateEvent &event) override;
    void OnEnterState(Context &context) override;
    void OnLeaveState(Context &context) override;

private:
    class Initial final : public ICooperateStep {
    public:
        Initial(CooperateOut &parent);
        ~Initial() = default;

        void OnProgress(Context &context, const CooperateEvent &event) override;
        void OnReset(Context &context, const CooperateEvent &event) override;

        static void BuildChains(std::shared_ptr<Initial> self, CooperateOut &parent);
        static void RemoveChains(std::shared_ptr<Initial> self);

    private:
        void OnDisable(Context &context, const CooperateEvent &event);
        void OnStop(Context &context, const CooperateEvent &event);
        void OnComeBack(Context &context, const CooperateEvent &event);
        void OnRemoteStart(Context &context, const CooperateEvent &event);
        void OnRemoteStop(Context &context, const CooperateEvent &event);
        void OnRelay(Context &context, const CooperateEvent &event);
        void OnHotplug(Context &context, const CooperateEvent &event);
        void OnAppClosed(Context &context, const CooperateEvent &event);
        void OnPointerEvent(Context &context, const CooperateEvent &event);
        void OnBoardOffline(Context &context, const CooperateEvent &event);
        void OnSwitchChanged(Context &context, const CooperateEvent &event);
        void OnSoftbusSessionClosed(Context &context, const CooperateEvent &event);

        CooperateOut &parent_;
        std::shared_ptr<ICooperateStep> remoteStart_ { nullptr };
    };

    class RemoteStart : public ICooperateStep {
    public:
        RemoteStart(CooperateOut &parent, std::shared_ptr<ICooperateStep> prev);
        ~RemoteStart() = default;

        void OnProgress(Context &context, const CooperateEvent &event) override;
        void OnReset(Context &context, const CooperateEvent &event) override;

    private:
        void OnDisable(Context &context, const CooperateEvent &event);
        void OnStop(Context &context, const CooperateEvent &event);
        void OnRemoteStartFinished(Context &context, const CooperateEvent &event);
        void OnSuccess(Context &context, const DSoftbusStartCooperateFinished &notice);
        void OnAppClosed(Context &context, const CooperateEvent &event);
        void OnHotplug(Context &context, const CooperateEvent &event);
        void OnPointerEvent(Context &context, const CooperateEvent &event);
        void OnBoardOffline(Context &context, const CooperateEvent &event);
        void OnSwitchChanged(Context &context, const CooperateEvent &event);
        void OnSoftbusSessionClosed(Context &context, const CooperateEvent &event);

        CooperateOut &parent_;
        int32_t timerId_ { -1 };
    };

    void StopCooperate(Context &context, const CooperateEvent &event);

    IContext *env_ { nullptr };
    std::shared_ptr<Initial> initial_ { nullptr };
};
} // namespace Cooperate
} // namespace DeviceStatus
} // namespace Msdp
} // namespace OHOS
#endif // COOPERATE_OUT_H
