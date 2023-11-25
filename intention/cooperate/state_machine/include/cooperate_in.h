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

#ifndef COOPERATE_IN_H
#define COOPERATE_IN_H

#include <array>
#include <memory>

#include "state_machine.h"

namespace OHOS {
namespace Msdp {
namespace DeviceStatus {
class CooperateIn : public StateMachine {
public:
    CooperateIn();
    ~CooperateIn() = default;

    void OnEvent(Context &context, CooperateEvent &event) override {}
    void OnEvent(CooperateEvent &event) override {}
    void OnLeave(CooperateEvent &event) override {}

private:
    class Initial final : public ICooperateStep {
    public:
        void OnEvent(Context &context, CooperateEvent &event) override;
        void OnProgress(Context &context, CooperateEvent &event) override;
        void OnReset(Context &context, CooperateEvent &event) override;
    };

    class PrepareRemoteInput final : public ICooperateStep {
    public:
        void OnEvent(Context &context, CooperateEvent &event) override;
        void OnProgress(Context &context, CooperateEvent &event) override;
        void OnReset(Context &context, CooperateEvent &event) override;
    
    private:
        std::pair<std::string, std::string> prepared_;
    };

    class StartRemoteInput final : public ICooperateStep {
    public:
        void OnEvent(Context &context, CooperateEvent &event) override;
        void OnProgress(Context &context, CooperateEvent &event) override;
        void OnReset(Context &context, CooperateEvent &event) override;
    };
};
} // namespace DeviceStatus
} // namespace Msdp
} // namespace OHOS
#endif // COOPERATE_IN_H