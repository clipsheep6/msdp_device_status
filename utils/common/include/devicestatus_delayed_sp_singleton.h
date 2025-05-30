/*
 * Copyright (c) 2022-2023 Huawei Device Co., Ltd.
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

#ifndef DEVICESTATUS_DELAYED_SP_SINGLETON_H
#define DEVICESTATUS_DELAYED_SP_SINGLETON_H

#include <mutex>
#include <memory>
#include <refbase.h>

#include "nocopyable.h"

namespace OHOS {
namespace Msdp {
namespace DeviceStatus {
#define DECLARE_DELAYED_SP_SINGLETON(MyClass) \
public: \
    ~MyClass(); \
private: \
    friend DelayedSpSingleton<MyClass>; \
    MyClass();

template<typename T>
class DelayedSpSingleton : public NoCopyable {
public:
    static void DestroyInstance();
    static sptr<T> GetInstance();

private:
    static std::mutex mutex_;
    static sptr<T> instance_;
};

template<typename T>
sptr<T> DelayedSpSingleton<T>::instance_ { nullptr };

template<typename T>
std::mutex DelayedSpSingleton<T>::mutex_;

template<typename T>
sptr<T> DelayedSpSingleton<T>::GetInstance()
{
    if (instance_ == nullptr) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (instance_ == nullptr) {
            instance_ = new T();
        }
    }
    return instance_;
}

template<typename T>
void DelayedSpSingleton<T>::DestroyInstance()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (instance_ != nullptr) {
        instance_.clear();
        instance_ = nullptr;
    }
}
} // namespace DeviceStatus
} // namespace Msdp
} // namespace OHOS
#endif // DEVICESTATUS_DELAYED_SP_SINGLETON_H
