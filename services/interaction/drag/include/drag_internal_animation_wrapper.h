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
 
 #ifndef DRAG_INTERNAL_ANIMATION
 #define DRAG_INTERNAL_ANIMATION
  
#include "i_context.h"

namespace OHOS {
namespace Msdp {
namespace DeviceStatus {

typedef int32_t (*EnableInternalDropAnimationFunc)(const char *);
typedef int32_t (*PerformInternalDropAnimationFunc)(void);
typedef bool (*NeedPerformInternalDropAnimationFunc)(void);

class DragInternalAnimationWrapper {
public:
    DragInternalAnimationWrapper(IContext *env)  : env_(env) {}
    ~DragInternalAnimationWrapper();
    int32_t EnableInternalDropAnimation(const std::string &animationInfo);
    bool NeedPerformInternalDropAnimation();
    int32_t PerformInternalDropAnimation();
 
private:
    IContext* env_ { nullptr };
    void* dragInternalAnimationHandle_ { nullptr };
    EnableInternalDropAnimationFunc enableInternalDropAnimationHandle_ { nullptr };
    PerformInternalDropAnimationFunc performInternalDropAnimationHandle_ { nullptr };
    NeedPerformInternalDropAnimationFunc needPerformInternalDropAnimationHandle_ { nullptr };
};
} // namespace DeviceStatus
} // namespace Msdp
} // namespace OHOS
#endif // DRAG_INTERNAL_ANIMATION