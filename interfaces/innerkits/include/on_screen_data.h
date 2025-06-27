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
#include <vector>

#include "pixel_map.h"

namespace OHOS {
namespace Msdp {
namespace DeviceStatus {
namespace OnScreen {
enum Scenario {
    UNKNOWN = 0,
    ARTICLE = 1,
};

enum EventType {
    UNKNOWN = 0,
    SCROLL_TO_HOOK,
};

struct ContentOption {
    int32_t windowId = 0;
    // 是否进行内容理解
    bool contentUnderstand = false;
    // 是否采集页面链接
    bool pageLink = false;
    // 是否只采集文本
    bool textOnly = false;
    // 是否拆解长文本
    bool longTextSplit = false;
    // 是否提供hook点
    bool elementHook = false;
    // 是否采集页面截图
    bool screenshot = false;
};

struct AppInfo {
    std::string name;
    std::string bundleName;
    std::string iconPath;
};

struct Paragraph {
    int32_t hookId;
    std::string text;
};

struct PageContent {
    int32_t winId = 0;
    AppInfo appInfo;
    // content understand = true
    Scenario scenario = Scenario::UNKNOWN;
    std::string title;
    std::string content;
    // page_link = true
    std::string links;
    // text only = true
    std::vector<Paragraph> paragraphs;
    // screenshot = true
    std::shared_ptr<Media::PixelMap> screenshot;
};

struct ControlEvent {
    int32_t windowId = 0;
    EventType eventType = EventType::SCROLL_TO_HOOK;
    int32_t hookId = 0;
};
} // namespace OnScreen
} // namespace DeviceStatus
} // namespace Msdp
} // namespace OHOS
#endif // ON_SCREEN_DATA_H