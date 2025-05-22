/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef INTENTION_BOOMERANG_CLIENT_H
#define INTENTION_BOOMERANG_CLIENT_H

#include <map>

#include "nocopyable.h"

#include "iremote_boomerang_callback.h"

namespace OHOS {
namespace Msdp {
namespace DeviceStatus {
class BoomerangClient final {
public:
    BoomerangClient() = default;
    ~BoomerangClient() = default;
    DISALLOW_COPY_AND_MOVE(BoomerangClient);

    int32_t SubscribeCallback(BoomerangType type, const std::string& bundleName,
        const sptr<IRemoteBoomerangCallback>& callback);
    int32_t UnsubscribeCallback(BoomerangType type, const std::string& bundleName,
        const sptr<IRemoteBoomerangCallback>& callback);
    int32_t NotifyMetadataBindingEvent(const std::string& bundleName, const sptr<IRemoteBoomerangCallback>& callback);
    int32_t SubmitMetadata(const std::string& metaData);
    int32_t BoomerangEncodeImage(const std::shared_ptr<Media::PixelMap>& pixelMap,
        const std::string& metaData, const sptr<IRemoteBoomerangCallback>& callback);
    int32_t BoomerangDecodeImage(const std::shared_ptr<Media::PixelMap>& pixelMap,
        const sptr<IRemoteBoomerangCallback>& callback);

private:
    std::mutex mtx_;
    std::map<BoomerangType, int32_t> typeMap_;
};
} // namespace DeviceStatus
} // namespace Msdp
} // namespace OHOS
#endif // INTENTION_BOOMERANG_CLIENT_H