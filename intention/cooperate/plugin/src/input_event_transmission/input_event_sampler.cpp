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

#undef LOG_TAG
#define LOG_TAG "InputEventSampler"

#include <chrono>
#include "input_event_transmission/input_event_sampler.h"
#include "devicestatus_define.h"

namespace OHOS {
namespace Msdp {
namespace DeviceStatus {
namespace Cooperate {

int32_t InputEventSampler::idealEventIntervalMS_ { 4 };
int32_t InputEventSampler::expiredIntervalMS_ { 24 };
int32_t InputEventSampler::rawDxThreshold_ { 20 };
int32_t InputEventSampler::rawDyThreshold_ { 20 };
std::unordered_set<int32_t> InputEventSampler::filterPointerActions_ {
    MMI::PointerEvent::POINTER_ACTION_ENTER_WINDOW,
    MMI::PointerEvent::POINTER_ACTION_LEAVE_WINDOW,
    MMI::PointerEvent::POINTER_ACTION_PULL_IN_WINDOW,
    MMI::PointerEvent::POINTER_ACTION_PULL_OUT_WINDOW,
};

InputEventSampler::InputEventSampler() { }

void InputEventSampler::OnPointerEvent(std::shared_ptr<MMI::PointerEvent> pointerEvent)
{
    if (IsSkipNeeded(pointerEvent)) {
        return;
    }
    if (IsRawEventsExpired()) {
        ClearRawEvents();
    }
    if (IsTouchPadEvent(pointerEvent)) {
        HandleTouchPadEvent(pointerEvent);
    } else {
        HandleMouseEvent(pointerEvent);
    }
    OnDownSampledEvent();
}

void InputEventSampler::SetPointerEventHandler(PointerEventHandler pointerEventHandler)
{
    pointerEventHandler_ = pointerEventHandler;
}

bool InputEventSampler::IsTouchPadEvent(std::shared_ptr<MMI::PointerEvent> pointerEvent)
{
    if (pointerEvent == nullptr) {
        FI_HILOGW("Null pointerEvent, skip");
        return false;
    }
    MMI::PointerEvent::PointerItem item;
    if (!pointerEvent->GetPointerItem(pointerEvent->GetPointerId(), item)) {
        FI_HILOGE("Corrupted pointerEvent, skip");
        return false;
    }
    FI_HILOGD("Current toolType:%{public}d", item.GetToolType());
    return (item.GetToolType() == MMI::PointerEvent::TOOL_TYPE_TOUCHPAD);
}

bool InputEventSampler::IsSpecialEvent(std::shared_ptr<MMI::PointerEvent> pointerEvent)
{
    if (pointerEvent == nullptr) {
        FI_HILOGW("Null pointerEvent, skip");
        return false;
    }
    if (auto pointerAction = pointerEvent->GetPointerAction();
        pointerAction != MMI::PointerEvent::POINTER_ACTION_MOVE &&
        pointerAction != MMI::PointerEvent::POINTER_ACTION_PULL_MOVE) {
        FI_HILOGI("Special event, action:%{public}d", pointerAction);
        return true;
    }
    return false;
}

bool InputEventSampler::IsDurationMatched()
{
    std::lock_guard<std::mutex> guard(rawEventMutex_);
    auto currentTimeStamp = std::chrono::steady_clock::now();
    if (rawEvents_.empty()) {
        return false;
    }
    auto headEvent = rawEvents_.front();
    if (auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        currentTimeStamp - headEvent.rcvdTimeStamp).count(); duration >= idealEventIntervalMS_) {
        FI_HILOGD("Current timeSpan:%{public}lld, matched condition", duration);
        return true;
    }
    return false;
}

bool InputEventSampler::IsOffsetMatched(std::shared_ptr<MMI::PointerEvent> pointerEvent)
{
    MMI::PointerEvent::PointerItem item;
    if (pointerEvent == nullptr) {
        return false;
    }
    if (!pointerEvent->GetPointerItem(pointerEvent->GetPointerId(), item)) {
        FI_HILOGE("Corrupted pointerEvent, skip");
        return false;
    }
    auto rawDxSum = prefixRawDxSum_ + item.GetRawDx();
    auto rawDySum = prefixRawDySum_ + item.GetRawDy();
    if (abs(rawDxSum) >= rawDxThreshold_ || abs(rawDySum) >= rawDyThreshold_) {
        FI_HILOGD("Current rawDxSum:%{public}d, rawDySum:%{public}d, match offset condition", rawDxSum, rawDySum);
        return true;
    }
    return false;
}

bool InputEventSampler::IsRawEventsExpired()
{
    std::lock_guard<std::mutex> guard(rawEventMutex_);
    if (rawEvents_.empty()) {
        FI_HILOGD("Raw event expired,skip");
        return false;
    }
    auto lastEvent = rawEvents_.back();
    if (lastEvent.pointerEvent == nullptr) {
        return false;
    }
    auto currentTimeStamp = std::chrono::steady_clock::now();
    if (auto duration = std::chrono::duration_cast<std::chrono::milliseconds> (
        currentTimeStamp - lastEvent.rcvdTimeStamp).count() > expiredIntervalMS_) {
        FI_HILOGD("Raw event expired, action:%{public}d", duration);
        return true;
    }
    return false;
}

bool InputEventSampler::IsSkipNeeded(std::shared_ptr<MMI::PointerEvent> pointerEvent)
{
    if (pointerEvent == nullptr) {
        FI_HILOGW("Null pointerEvent, skip");
        return true;
    }
    if (auto pointerAction = pointerEvent->GetPointerAction();
        filterPointerActions_.find(pointerAction) != filterPointerActions_.end()) {
        FI_HILOGI("Unexpected pointerEvent, action:%{public}d, skip", pointerAction);
        return true;
    }
    return false;
}

void InputEventSampler::AggregateRawEvents(std::shared_ptr<MMI::PointerEvent> pointerEvent)
{
    MMI::PointerEvent::PointerItem item;
    CHKPV(pointerEvent);
    if (!pointerEvent->GetPointerItem(pointerEvent->GetPointerId(), item)) {
        FI_HILOGW("Corrupted pointerEvent, skip");
        return;
    }
    auto pointerId = item.GetPointerId();
    auto sampledRawDx = prefixRawDxSum_ + item.GetRawDx();
    auto sampledRawDy = prefixRawDySum_ + item.GetRawDy();
    MMI::PointerEvent::PointerItem aggregatedItem;
    aggregatedItem.SetPointerId(pointerId);
    aggregatedItem.SetRawDx(sampledRawDx);
    aggregatedItem.SetRawDy(sampledRawDy);
    pointerEvent->UpdatePointerItem(pointerId, aggregatedItem);
    {
        std::lock_guard<std::mutex> guard(sampledEventMutex_);
        sampledEvents_.push(pointerEvent);
    }
    ClearRawEvents();
    prefixRawDxSum_ = 0;
    prefixRawDySum_ = 0;
}

void InputEventSampler::HandleTouchPadEvent(std::shared_ptr<MMI::PointerEvent> pointerEvent)
{
    std::lock_guard<std::mutex> guard(sampledEventMutex_);
    sampledEvents_.push(pointerEvent);
}

void InputEventSampler::HandleMouseEvent(std::shared_ptr<MMI::PointerEvent> pointerEvent)
{
    CHKPV(pointerEvent);
    if (IsSpecialEvent(pointerEvent) || IsDurationMatched()) {
        AggregateRawEvents(pointerEvent);
    } else {
        MMI::PointerEvent::PointerItem item;
        if (!pointerEvent->GetPointerItem(pointerEvent->GetPointerId(), item)) {
            FI_HILOGE("Corrupted pointerEvent, skip");
            return;
        }
        prefixRawDxSum_ += item.GetRawDx();
        prefixRawDySum_ += item.GetRawDy();
        {
            std::lock_guard<std::mutex> guard(rawEventMutex_);
            rawEvents_.push({pointerEvent, std::chrono::steady_clock::now()});
            FI_HILOGD("Raw events count:%{public}zu", rawEvents_.size());
        }
    }
}

void InputEventSampler::OnDownSampledEvent()
{
    std::lock_guard<std::mutex> guard(sampledEventMutex_);
    FI_HILOGD("Sampled events existed count:%{public}zu", sampledEvents_.size());
    while (!sampledEvents_.empty()) {
        auto curEvent = sampledEvents_.front();
        sampledEvents_.pop();
        CHKPV(pointerEventHandler_);
        pointerEventHandler_(curEvent);
    }
}

void InputEventSampler::ClearRawEvents()
{
    std::lock_guard<std::mutex> guard(rawEventMutex_);
    auto rawEvents = std::queue<RawEvent>();
    rawEvents_ = rawEvents;
}
} // namespace Cooperate
} // namespace DeviceStatus
} // namespace Msdp
} // namespace OHOS