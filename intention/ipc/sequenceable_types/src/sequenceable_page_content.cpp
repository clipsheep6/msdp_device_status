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

#include "sequenceable_page_content.h"

#include "devicestatus_common.h"

namespace OHOS {
namespace Msdp {
namespace DeviceStatus {
namespace OnScreen {
bool SequenceablePageContent::Marshalling(Parcel &parcel) const
{
    WRITEINT32(parcel, pageContent_.windowId, false);
    WRITESTRING(parcel, pageContent_.bundleName, false);
    WRITEINT32(parcel, static_cast<int32_t>(pageContent_.scenario), false);
    WRITESTRING(parcel, pageContent_.title, false);
    WRITESTRING(parcel, pageContent_.content, false);
    WRITESTRING(parcel, pageContent_.links, false);
    WRITEINT32(parcel, static_cast<int32_t>(pageContent_.paragraphs.size()), false);
    for (size_t i = 0; i < pageContent_.paragraphs.size(); i++) {
        WRITEUINT64(parcel, pageContent_.paragraphs[i].elementId, false);
        WRITESTRING(parcel, pageContent_.paragraphs[i].title, false);
        WRITESTRING(parcel, pageContent_.paragraphs[i].text, false);
    }
    return true;
}

SequenceablePageContent* SequenceablePageContent::Unmarshalling(Parcel &parcel)
{
    auto content = new (std::nothrow) SequenceablePageContent();
    if (content != nullptr && !content->ReadFromParcel(parcel)) {
        FI_HILOGE("read from parcel failed");
        delete content;
        content = nullptr;
    }
    return content;
}

bool SequenceablePageContent::ReadFromParcel(Parcel &parcel)
{
    READINT32(parcel, pageContent_.windowId, false);
    READSTRING(parcel, pageContent_.bundleName, false);
    int32_t scenario = 0;
    READINT32(parcel, scenario, false);
    if (scenario < 0 || scenario >= static_cast<int32_t>(Scenario::END)) {
        return false;
    }
    pageContent_.scenario = static_cast<Scenario>(scenario);
    READSTRING(parcel, pageContent_.title, false);
    READSTRING(parcel, pageContent_.content, false);
    READSTRING(parcel, pageContent_.links, false);
    int32_t paragraphSize = 0;
    READINT32(parcel, paragraphSize, false);
    std::vector<Paragraph>().swap(pageContent_.paragraphs);
    for (int32_t i = 0; i < paragraphSize; i++) {
        Paragraph para;
        READUINT64(parcel, para.elementId, false);
        READSTRING(parcel, para.title, false);
        READSTRING(parcel, para.text, false);
        pageContent_.paragraphs.push_back(para);
    }
    return true;
}
} // namespace OnScreen
} // namespace DeviceStatus
} // namespace Msdp
} // namespace OHOS