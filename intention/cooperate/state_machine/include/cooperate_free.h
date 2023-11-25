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

#ifndef COOPERATE_FREE_H
#define COOPERATE_FREE_H

#include "i_cooperate_state.h"

namespace OHOS {
namespace Msdp {
namespace DeviceStatus {
class CooperateFree final : public ICooperateState
{
public:
    CooperateFree();
    ~CooperateFree() = default;
    void OnEvent(Context &context, CooperateEvent &event) override;
    void OnEnter(Context &context) override;
    void OnLeave(Context &context) override;

private:
    class Initial final : public ICooperateState
    {
    public:
        Initial(CooperateFree &parent);
        ~Initial() = default;

        void OnEvent(Context &context, CooperateEvent &event) override;
        void OnProgress(Context &context, CooperateEvent &event) override;
        void OnReset(Context &context, CooperateEvent &event) override;

        static void BuildChains(std::shared_ptr<Initial> self, CooperateFree &parent);

    private:
        void OnStart(Context &context, CooperateEvent &event);

        std::shared_ptr<ICooperateStep> start_ { nullptr };
    }

    class PrepareRemoteInput final : public ICooperateState
    {
    public:
        PrepareRemoteInput(CooperateFree &parent, std::shared_ptr<ICooperateState> prev);
        ~PrepareRemoteInput() = default;

        void OnEvent(Context &context, CooperateEvent &event) override;
        void OnProgress(Context &context, CooperateEvent &event) override;
        void OnReset(Context &context, CooperateEvent &event) override;
    };
    
    class StartRemoteInput final : public ICooperateState
    {
    public:
        StartRemoteInput(CooperateFree &parent, std::shared_ptr<ICooperateState> prev);
        ~StartRemoteInput() = default;

        void OnEvent(Context &context, CooperateEvent &event) override;
        void OnProgress(Context &context, CooperateEvent &event) override;
        void OnReset(Context &context, CooperateEvent &event) override;
    }

    class OpenSession final : public ICooperateState
    {
    public:
        OpenSession(CooperateFree &parent, std::shared_ptr<ICooperateState> prev);
        ~OpenSession() = default;

        void OnEvent(Context &context, CooperateEvent &event) override;
        void OnProgress(Context &context, CooperateEvent &event) override;
        void OnReset(Context &context, CooperateEvent &event) override;
    };
};
} // namespace DeviceStatus
} // namespace Msdp
} // namespace OHOS
#endif // COOPERATE_FREE_H
