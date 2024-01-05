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

#ifndef COOPERATE_H
#define COOPERATE_H

#include <mutex>

#include "nocopyable.h"

#include "i_context.h"
#include "i_cooperate.h"
#include "state_machine.h"

namespace OHOS {
namespace Msdp {
namespace DeviceStatus {
namespace Cooperate {
class Cooperate final : public ICooperate {
public:
    Cooperate(IContext *env);
    ~Cooperate() = default;
    DISALLOW_COPY_AND_MOVE(Cooperate);

    int32_t RegisterListener(int32_t pid) override;
    int32_t UnregisterListener(int32_t pid) override;
    int32_t RegisterHotAreaListener(int32_t pid) override;
    int32_t UnregisterHotAreaListener(int32_t pid) override;

    int32_t Enable(int32_t pid) override;
    int32_t Disable(int32_t pid) override;
    int32_t Start(int32_t pid, int32_t userData, const std::string &remoteNetworkId, int32_t startDeviceId) override;
    int32_t Stop(int32_t pid, int32_t userData, bool isUnchained) override;

    int32_t GetCooperateState(int32_t pid, int32_t userData, const std::string &networkId) override;

    void Dump(int32_t fd) override;

private:
    void Loop();
    void StartWorker();
    void StopWorker();

    Context context_;
    StateMachine sm_;
    std::mutex lock_;
    std::thread worker_;
    std::atomic_bool running_ { false };
    Channel<CooperateEvent>::Sender sender_;
    Channel<CooperateEvent>::Receiver receiver_;
};
} // namespace Cooperate
} // namespace DeviceStatus
} // namespace Msdp
} // namespace OHOS
#endif // COOPERATE_H