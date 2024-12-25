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
 
#ifndef UNIVERSAL_DRAG_WRAPPER_H
#define UNIVERSAL_DRAG_WRAPPER_H
 
#include "i_context.h"
 
namespace OHOS {
namespace Msdp {
namespace DeviceStatus {
 
typedef bool (*InitFunc)(IContext*);
typedef void (*RemoveUniversalDragFunc)(void);
typedef void (*SetDragableStateFunc)(bool);
typedef void (*SetDragSwitchStateFunc)(bool);
typedef void (*SetAppDragSwitchStateFunc)(const char *, bool);
 
class UniversalDragWrapper {
public:
    UniversalDragWrapper(IContext *env)  : env_(env) {}
    ~UniversalDragWrapper();
    bool InitUniversalDrag();
    void RmoveUniversalDrag();
    void SetDragableState(bool state);
    void SetDragSwitchState(bool enable);
    void SetAppDragSwitchState(const std::string &pkgName, bool enable);
 
private:
    IContext* env_ { nullptr };
    void* universalDragHandle_ { nullptr };
    InitFunc initUniversalDragHandle_ { nullptr };
    RemoveUniversalDragFunc removeUniversalDragHandle_ { nullptr };
    SetDragableStateFunc setDragableStateHandle_ { nullptr };
    SetDragSwitchStateFunc setDragSwitchStateHandle_ { nullptr };
    SetAppDragSwitchStateFunc setAppDragSwitchStateHandle_ { nullptr };
};
} // namespace DeviceStatus
} // namespace Msdp
} // namespace OHOS
#endif // I_DRAG_ANIMATION_H