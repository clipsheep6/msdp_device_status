/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

#include "safe_keeper.h"
#include <cinttypes>
#include "mmi_log.h"
#include "util.h"

namespace OHOS {
namespace MMI {
namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, MMI_LOG_DOMAIN, "SafeKeeper" }; // namepace
}

SafeKeeper::SafeKeeper() {}

SafeKeeper::~SafeKeeper() {}

void SafeKeeper::Init(SafeCallbackFun fun)
{
    cbFun_ = fun;
}

bool SafeKeeper::RegisterEvent(uint64_t tid, const std::string& remark)
{
    if (!IsThreadExist(tid)) {
        MMI_LOGE("This thread does not exist");
        return false;
    }
    if (remark.empty()) {
        MMI_LOGE("Thread name is empty");
        return false;
    }
    std::lock_guard<std::mutex> lock(mtx_);
    const SafeEvent event = {tid, GetCurMillisTime(), remark};
    dList_.push_back(event);
    MMI_LOGI("SafeKeeper register tid:%{public}" PRId64 ",remark:%{public}s", tid, remark.c_str());
    return true;
}

void SafeKeeper::ClearAll()
{
    std::lock_guard<std::mutex> lock(mtx_);
    dList_.clear();
    cbFun_ = nullptr;
}

void SafeKeeper::ReportHealthStatus(uint64_t tid)
{
    if (tid <= 0) {
        MMI_LOGE("The in parameter is error");
        return;
    }
    std::lock_guard<std::mutex> lock(mtx_);
    auto ptr = GetEvent(tid);
    if (!ptr) {
        MMI_LOGE("SafeKeeper report ptr = nullptr tid:%{public}" PRId64 ",errCode:%{public}d",
                 tid, ERROR_NULL_POINTER);
        return;
    }
    ptr->lastTime = GetCurMillisTime();
}

void SafeKeeper::ProcessEvents()
{
    if (dList_.empty()) {
        return;
    }
    int32_t pastTime = 0;
    auto curTime = GetCurMillisTime();

    std::lock_guard<std::mutex> lock(mtx_);
    for (const auto &item : dList_) {
        pastTime = static_cast<int32_t>(curTime - item.lastTime);
        if (pastTime > MAX_THREAD_DEATH_TIME) {
            cbFun_(pastTime, item.tid, item.remark);
            return;
        }
    }
}
} // namespace MMI
} // namespace OHOS
