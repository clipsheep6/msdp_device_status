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
 
#ifndef ON_SCREEN_DATA_H
#define ON_SCREEN_DATA_H

#include <memory>
#include <string>
#include <vector>

namespace OHOS {
namespace Msdp {
namespace DeviceStatus {
namespace OnScreen {
namespace {
constexpr int32_t MIN_PARA_SIZE = 300;
constexpr int32_t MAX_PARA_SIZE = 500;
enum OnScreenErrCode {
    RET_ERR = -1,
    RET_OK = 0,
    RET_NO_PERMISSION = 201,
    RET_NO_SYSTEM_CALLING = 202,
    RET_PARAM_ERR = 401,
    RET_NO_SUPPORT = 801,
    RET_SERVICE_EXCEPTION = 34000001,
    RET_NOT_IN_WHITELIST,
    RET_WINDOW_ID_ERR,
    RET_EVENT_ID_NOT_FIND,
};
}

enum class Scenario {
    UNKNOWN = 0,
    ARTICLE,
    END,
};

enum class EventType {
    UNKNOWN = 0,
    SCROLL_TO_HOOK,
    END,
};

struct ParagraphSizeRange {
    int32_t minSize;
    int32_t maxSize;
};

struct ContentOption {
    int32_t windowId = -1;
    // 是否进行内容理解
    bool contentUnderstand = false;
    // 是否采集页面链接
    bool pageLink = false;
    // 是否只采集文本
    bool textOnly = false;
    // 设置最小分段长度和最大分段长度，默认为300 500
    ParagraphSizeRange paragraphSizeRange = {
        .minSize = MIN_PARA_SIZE,
        .maxSize = MAX_PARA_SIZE
    };
};

struct Paragraph {
    int64_t hookId = 0;
    std::string title;
    std::string content;
};

struct PageContent {
    int32_t windowId = -1;
    int64_t sessionId = 0;
    std::string bundleName;
    // content understand = true
    Scenario scenario = Scenario::UNKNOWN;
    std::string title;
    std::string content;
    // page_link = true
    std::string pageLink;
    // text only = true
    std::vector<Paragraph> paragraphs;
};

struct ControlEvent {
    int32_t windowId = 0;
    int64_t sessionId = 0;
    EventType eventType = EventType::UNKNOWN;
    int64_t hookId = 0;
};

struct OnScreenCallingContext {
    uint64_t fullTokenId = 0;
    uint32_t tokenId = 0;
    int32_t uid = -1;
    int32_t pid = -1;
};
} // namespace OnScreen
} // namespace DeviceStatus
} // namespace Msdp
} // namespace OHOS
#endif // ON_SCREEN_DATA_H