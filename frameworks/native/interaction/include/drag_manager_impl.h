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
#ifndef DRAG_MANAGER_IMPL_H
#define DRAG_MANAGER_IMPL_H

#include <functional>

#include "singleton.h"
#include "drag_data.h"

namespace OHOS {
namespace Msdp {
namespace DeviceStatus {
class DragManagerImpl final {
    DECLARE_SINGLETON(DragManagerImpl);

public:
    DISALLOW_MOVE(DragManagerImpl);

    int32_t UpdateDragStyle(int32_t style);
    int32_t UpdateDragMessage(const std::u16string &message);
    int32_t StartDrag(const DragData &dragData, std::function<void(int32_t&)> callback);
    int32_t StopDrag(int32_t &dragResult);
private:
    void SetCallback(std::function<void(int32_t&)> callback);
    std::function<void(int32_t&)> GetCallback();

private:
    std::function<void(int32_t&)> stopCallback_;

};
} // namespace DeviceStatus
} // namespace Msdp
} // namespace OHOS

#define DragMgrImpl ::OHOS::Singleton<DragManagerImpl>::GetInstance()

#endif // DRAG_MANAGER_IMPL_H
