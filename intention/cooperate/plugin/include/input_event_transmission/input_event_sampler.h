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

#ifndef INPUT_EVENT_SAMPLER_H
#define INPUT_EVENT_SAMPLER_H

#include <atomic>
#include <mutex>
#include <queue>
#include <unordered_set>

#include "pointer_event.h"

namespace OHOS {
namespace Msdp {
namespace DeviceStatus {
namespace Cooperate {

using PointerEventHandler = std::function<void(std::shared_ptr<MMI::PointerEvent>)>;

struct RawEvent {
    std::shared_ptr<MMI::PointerEvent> pointerEvent;
    std::chrono::steady_clock::time_point rcvdTimeStamp;
};

class InputEventSampler final {
public:
    InputEventSampler();
    void OnPointerEvent(std::shared_ptr<MMI::PointerEvent> pointerEvent);
    void SetPointerEventHandler(PointerEventHandler pointerEventHandler);
private:
    bool IsTouchPadEvent(std::shared_ptr<MMI::PointerEvent> pointerEvent);
    bool IsSpecialEvent(std::shared_ptr<MMI::PointerEvent> pointerEvent);
    bool IsDurationMatched();
    bool IsOffsetMatched(std::shared_ptr<MMI::PointerEvent> pointerEvent);
    bool IsRawEventsExpired();
    bool IsSkipNeeded(std::shared_ptr<MMI::PointerEvent> pointerEvent);
    void AggregateRawEvents(std::shared_ptr<MMI::PointerEvent> pointerEvent);
    void HandleTouchPadEvent(std::shared_ptr<MMI::PointerEvent> pointerEvent);
    void HandleMouseEvent(std::shared_ptr<MMI::PointerEvent> pointerEvent);
    void OnDownSampledEvent();
    void ClearRawEvents();
private:
    PointerEventHandler pointerEventHandler_;
    std::mutex rawEventMutex_;
    std::queue<RawEvent> rawEvents_;
    std::mutex sampledEventMutex_;
    std::queue<std::shared_ptr<MMI::PointerEvent>> sampledEvents_;

    std::atomic_int32_t prefixRawDxSum_ { 0 };
    std::atomic_int32_t prefixRawDySum_ { 0 };
    
    static int32_t idealEventIntervalMS_;
    static int32_t expiredIntervalMS_;
    static int32_t rawDxThreshold_;
    static int32_t rawDyThreshold_;
    static std::unordered_set<int32_t> filterPointerActions_;
};
} // namespace Cooperate
} // namespace DeviceStatus
} // namespace Msdp
} // namespace OHOS
#endif // INPUT_EVENT_SAMPLER_H
