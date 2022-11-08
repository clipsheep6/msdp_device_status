/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "sensor_data_callback.h"

#include <cstdio>

#include "devicestatus_common.h"

namespace OHOS {
namespace Msdp {
namespace DeviceStatus {

std::shared_ptr<SensorDataCallback> SensorDataCallback::instance_ = nullptr;

SensorDataCallback::SensorDataCallback()
{
    DEV_HILOGI(SERVICE, "SensorDataCallback is created");
    alive_ = true;
}

SensorDataCallback::~SensorDataCallback()
{
    DEV_HILOGD(SERVICE, "enter");
    std::lock_guard cbLock(callbackMutex_);
    std::lock_guard dataLock(dataMutex_);
    algoMap_.clear();
    alive_ = false;
    if (algorithmThread_ == nullptr) {
        DEV_HILOGE(SERVICE, "algorithmThread_ is nullptr");
        return;
    }
    algorithmThread_->join();
    accelDataList_.clear();
}

bool SensorDataCallback::Init()
{
    DEV_HILOGI(SERVICE, "SensorDataCallback is initiated");
    algorithmThread_ = std::make_unique<std::thread>(&SensorDataCallback::AlgorithmLoop, this);
    return true;
}

bool SensorDataCallback::Unregister()
{
    bool ret = UnregisterCallbackSensor(SensorTypeId::SENSOR_TYPE_ID_ACCELEROMETER);
    if (ret) {
        alive_ = false;
        algorithmThread_->join();
    }
    return ret;
}

bool SensorDataCallback::SubscribeSensorEvent(int32_t sensorTypeId, SensorCallback callback)
{
    std::lock_guard lock(callbackMutex_);
    DEV_HILOGI(SERVICE, "SubscribeSensorEvent");
    algoMap_.insert(std::pair(sensorTypeId, callback));
    if (algoMap_.second) {
        return true;
    } 
    return false;
}

bool SensorDataCallback::UnSubscribeSensorEvent(int32_t sensorTypeId, SensorCallback callback)
{
    DEV_HILOGD(SERVICE, "enter");
    std::lock_guard lock(callbackMutex_);
    auto callbackIter = algoMap_.find(sensorTypeId);
    if (callbackIter != algoMap_.end()) {
        DEV_HILOGI(SERVICE, "map erase");
        algoMap_.erase(sensorTypeId);
    }
    DEV_HILOGD(SERVICE, "exit");
    return true;
}

bool SensorDataCallback::NotifyCallback(int32_t sensorTypeId, AccelData* data)
{
    DEV_HILOGD(SERVICE, "enter");
    if (data == nullptr) {
        return false;
    }
    std::lock_guard lock(callbackMutex_);
    for (auto iter = algoMap_.begin(); iter != algoMap_.end(); ++iter) {
        (iter->second)(sensorTypeId, data);
    }
    return true;
}

bool SensorDataCallback::PushData(int32_t sensorTypeId, uint8_t* data)
{
    DEV_HILOGD(SERVICE, "enter");
    if (data == nullptr) {
        return false;
    }
    std::lock_guard lock(dataMutex_);
    DEV_HILOGD(SERVICE, "PushData sensorTypeId: %{public}d", sensorTypeId);
    AccelData* ad = reinterpret_cast<AccelData*>(data);
    if ((abs(ad->x) > ACC_VALID_THRHD) ||
        (abs(ad->y) > ACC_VALID_THRHD) ||
        (abs(ad->z) > ACC_VALID_THRHD)) {
        DEV_HILOGE(SERVICE, "Acc Data wrong");
        return false;
    }
    accelDataList_.emplace_back(*ad);
    DEV_HILOGD(SERVICE, "ACCEL PushData: x=%{public}f, y=%{public}f, z=%{public}f", ad->x, ad->y, ad->z);
    sem_post(&sem_);
    return true;
}

bool SensorDataCallback::PopData(int32_t sensorTypeId, AccelData& data)
{
    DEV_HILOGD(SERVICE, "enter");
    std::lock_guard lock(dataMutex_);
    DEV_HILOGD(SERVICE, "PopData sensorTypeId: %{public}d", sensorTypeId);
    if (sensorTypeId != SENSOR_TYPE_ID_ACCELEROMETER) {
        DEV_HILOGE(SERVICE, "wrong sensorTypeId: %{public}d", sensorTypeId);
        return false;
    }
    if (accelDataList_.empty()) {
        DEV_HILOGI(SERVICE, "No Accel Data");
        return false;
    }
    data = accelDataList_.front();
    accelDataList_.pop_front();
    DEV_HILOGI(SERVICE, "ACCEL PopData: x=%{public}f, y=%{public}f, z=%{public}f", data.x, data.y, data.z);
    return true;
}

static void SensorDataCallbackImpl(SensorEvent *event)
{
    DEV_HILOGD(SERVICE, "enter");
    if (event == nullptr) {
        DEV_HILOGE(SERVICE, "SensorDataCallbackImpl event is null");
        return;
    }
    DEV_HILOGI(SERVICE, "SensorDataCallbackImpl sensorTypeId: %{public}d", event->sensorTypeId);
    std::shared_ptr<SensorDataCallback> instance = SensorDataCallback::GetInstance();
    if (instance == nullptr) {
        return;
    }
    instance->PushData(event->sensorTypeId, event->data);
}

bool SensorDataCallback::RegisterCallbackSensor(int32_t sensorTypeId)
{
    user_.callback = SensorDataCallbackImpl;
    int32_t ret = SubscribeSensor(sensorTypeId, &user_);
    if (ret != 0) {
        DEV_HILOGD(SERVICE, "SubscribeSensor failed");
        return false;
    }
    ret = SetBatch(sensorTypeId, &user_, RATE_MILLISEC, RATE_MILLISEC);
    if (ret != 0) {
        DEV_HILOGD(SERVICE, "SetBatch failed");
        return false;
    }
    ret = ActivateSensor(sensorTypeId, &user_);
    if (ret != 0) {
        DEV_HILOGD(SERVICE, "ActivateSensor failed");
        return false;
    }
    return true;
}

bool SensorDataCallback::UnregisterCallbackSensor(int32_t sensorTypeId)
{
    DEV_HILOGD(SERVICE, "enter");
    user_.callback = SensorDataCallbackImpl;
    int32_t ret = DeactivateSensor(sensorTypeId, &user_);
    if (ret != 0) {
        DEV_HILOGE(SERVICE, "DeactivateSensor failed");
        return false;
    }
    ret = UnsubscribeSensor(sensorTypeId, &user_);
    if (ret != 0) {
        DEV_HILOGE(SERVICE, "UnsubscribeSensor failed");
        return false;
    }
    return true;
}

bool SensorDataCallback::AlgorithmLoop()
{
    DEV_HILOGD(SERVICE, "enter");
    while (alive_) {
        sem_wait(&sem_);
        HandleSensorEvent();
    }
    return true;
}

bool SensorDataCallback::HandleSensorEvent()
{
    DEV_HILOGD(SERVICE, "enter");
    AccelData ad;
    bool exist = PopData(SENSOR_TYPE_ID_ACCELEROMETER, ad);
    if (exist) {
        NotifyCallback(SENSOR_TYPE_ID_ACCELEROMETER, (AccelData*)(&ad));
    }
    return true;
}
} // namespace DeviceStatus
} // namespace Msdp
} // namespace OHOS
