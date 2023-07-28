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

#ifndef FUSION_DEVICE_PROFILE_ADAPTER_H
#define FUSION_DEVICE_PROFILE_ADAPTER_H
#include <cstddef>
#include <cstdint>

struct CIStringVector {
    CIStringVector* (*clone)(CIStringVector *);
    void (*destruct)(CIStringVector *);
    const char **strVec;
    size_t numOfStrs;
};

struct CICrossStateListener {
    CICrossStateListener* (*clone)(CICrossStateListener *listener);
    void (*destruct)(CICrossStateListener *listener);
    void (*on_update)(CICrossStateListener *listener, const char *device_id, int32_t state);
};

using DeviceProfileCallback = void (*)(const char *, bool, void *);

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

int32_t UpdateCrossSwitchState(int32_t state);
int32_t SyncCrossSwitchState(int32_t state, const CIStringVector *deviceIds);
int32_t GetCrossSwitchState(const char *deviceId);
int32_t RegisterCrossStateListener(const char *deviceId, CICrossStateListener *listener);
int32_t UnregisterCrossStateListener(const char *deviceId);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif // FUSION_DEVICE_PROFILE_ADAPTER_H
