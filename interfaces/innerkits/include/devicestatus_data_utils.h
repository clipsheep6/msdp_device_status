/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

#ifndef DEVICESTATUS_DATA_UTILS_H
#define DEVICESTATUS_DATA_UTILS_H

namespace OHOS {
namespace Msdp {
namespace DeviceStatus {

enum Type {
    TYPE_INVALID = -1,
    TYPE_STILL,
    TYPE_HORIZONTAL_POSITION,
    TYPE_VERTICAL_POSITION,
    TYPE_LID_OPEN,
    TYPE_MAX
};

enum TypeValue:bool {
    INVALID = false,
    VALID = true,
};

enum OnChangedValue {
    VALUE_INVALID = -1,
    VALUE_EXIT,
    VALUE_ENTER
};

enum ActivityEvent {
    EVENT_INVALID = 0,
    ENTER = 1,
    EXIT = 2,
    ENTER_EXIT = 3
};

enum ReportLatencyNs {
    Latency_INVALID = -1,
    SHORT = 1,
    MIDDLE = 2,
    LONG = 3
};
enum Status {
    STATUS_INVALID = -1,
    STATUS_CANCEL,
    STATUS_START,
    STATUS_PROCESS,
    STATUS_FINISH
};

enum Action {
    ACTION_INVALID = -1,
    ACTION_ENLARGE,
    ACTION_REDUCE,
    ACTION_UP,
    ACTION_LEFT,
    ACTION_DOWN,
    ACTION_RIGHT
};

struct Data {
    Type type;
    OnChangedValue value;
    Status status;
    Action action;
    double movement;

    bool operator!= (const Data& r) const 
    {
        if (type == r.type && value == r.value
        && status - r.status && action == r.action && (movement - r.movement) < 0.001) {
            return false;
        }
        return true;
    }
};

} // namespace DeviceStatus
} // namespace Msdp
} // namespace OHOS
#endif // DEVICESTATUS_DATA_UTILS_H