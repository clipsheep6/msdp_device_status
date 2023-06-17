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

#include "devicestatus_msdp_mock.h"

#include <cerrno>
#include <string>
#include <unistd.h>

#include <sys/epoll.h>
#include <sys/timerfd.h>

#include "devicestatus_common.h"
#include "devicestatus_define.h"
#include "fi_log.h"

namespace OHOS {
namespace Msdp {
namespace DeviceStatus {
namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL { LOG_CORE, MSDP_DOMAIN_ID, "DeviceStatusMsdpMock" };
constexpr int32_t TIMER_INTERVAL { 3 };
constexpr int32_t ERR_INVALID_FD { -1 };
DeviceStatusMsdpMock* g_msdpMock { nullptr };
} // namespace

DeviceStatusMsdpMock::DeviceStatusMsdpMock()
{
    enabledType_ = {
        Type::TYPE_STILL,
        Type::TYPE_RELATIVE_STILL,
        Type::TYPE_CAR_BLUETOOTH
    };
    if (dataParse_ == nullptr) {
        dataParse_ = std::make_unique<DeviceStatusDataParse>();
    }
}

DeviceStatusMsdpMock::~DeviceStatusMsdpMock()
{
    callbacks_.clear();
}

bool DeviceStatusMsdpMock::Init()
{
    CALL_DEBUG_ENTER;
    InitTimer();
    StartThread();
    return true;
}

ErrCode DeviceStatusMsdpMock::RegisterCallback(std::shared_ptr<MsdpAlgoCallback> callback)
{
    std::lock_guard lock(mutex_);
    callback_ = callback;
    return RET_OK;
}

ErrCode DeviceStatusMsdpMock::UnregisterCallback()
{
    std::lock_guard lock(mutex_);
    callback_ = nullptr;
    return RET_OK;
}

ErrCode DeviceStatusMsdpMock::Enable(Type type)
{
    CALL_DEBUG_ENTER;
    alive_ = true;
    Init();
    return RET_OK;
}

ErrCode DeviceStatusMsdpMock::Disable(Type type)
{
    CALL_DEBUG_ENTER;
    alive_ = false;
    CloseTimer();
    return RET_OK;
}

ErrCode DeviceStatusMsdpMock::DisableCount(Type type)
{
    CALL_DEBUG_ENTER;
    dataParse_->DisableCount(type);
    return RET_OK;
}

ErrCode DeviceStatusMsdpMock::NotifyMsdpImpl(const Data& data)
{
    CALL_DEBUG_ENTER;
    CHKPR(g_msdpMock, RET_ERR);
    CHKPR(g_msdpMock->GetCallbackImpl(), RET_ERR);
    FI_HILOGI("type:%{public}d, value:%{public}d", data.type, data.value);
    g_msdpMock->GetCallbackImpl()->OnResult(data);
    return RET_OK;
}

void DeviceStatusMsdpMock::InitTimer()
{
    CALL_DEBUG_ENTER;
    epFd_ = epoll_create1(EPOLL_CLOEXEC);
    if (epFd_ == -1) {
        FI_HILOGE("create epoll fd failed");
        return;
    }
    timerFd_ = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
    if (timerFd_ == ERR_INVALID_FD) {
        FI_HILOGE("create timer fd failed");
        close(epFd_);
        epFd_ = ERR_INVALID_FD;
        return;
    }
    SetTimerInterval(TIMER_INTERVAL);
    fcntl(timerFd_, F_SETFL, O_NONBLOCK);
    auto [_, ret] = callbacks_.insert(std::make_pair(timerFd_, &DeviceStatusMsdpMock::TimerCallback));
    if (!ret) {
        FI_HILOGW("insert timer fd failed");
    }
    if (RegisterTimerCallback(timerFd_, EventType::EVENT_TIMER_FD)) {
        FI_HILOGE("register timer fd failed");
        return;
    }
}

int32_t DeviceStatusMsdpMock::SetTimerInterval(int32_t interval)
{
    if (timerFd_ == ERR_INVALID_FD) {
        FI_HILOGE("create timer fd failed");
        return RET_ERR;
    }

    if (interval < 0) {
        FI_HILOGE("Illegal time interval");
        return RET_ERR;
    }
    struct itimerspec itval;
    itval.it_interval.tv_sec = interval;
    itval.it_interval.tv_nsec = 0;
    itval.it_value.tv_sec = interval;
    itval.it_value.tv_nsec = 0;
    if (timerfd_settime(timerFd_, 0, &itval, nullptr) == -1) {
        FI_HILOGE("set timer failed");
        return RET_ERR;
    }
    return RET_OK;
}

void DeviceStatusMsdpMock::CloseTimer()
{
    close(timerFd_);
}

void DeviceStatusMsdpMock::TimerCallback()
{
    uint64_t timers;
    if (read(timerFd_, &timers, sizeof(timers)) == -1) {
        FI_HILOGE("read timer fd failed");
        return;
    }
    GetDeviceStatusData();
}

int32_t DeviceStatusMsdpMock::GetDeviceStatusData()
{
    for (const auto &item : enabledType_) {
        Type type = item;
        CHKPR(dataParse_, RET_ERR);
        Data data;
        dataParse_->ParseDeviceStatusData(type, data);
        FI_HILOGD("mock type:%{public}d, value:%{public}d", data.type, data.value);
        NotifyMsdpImpl(data);
    }
    return RET_OK;
}

int32_t DeviceStatusMsdpMock::RegisterTimerCallback(int32_t fd, const EventType et)
{
    CALL_DEBUG_ENTER;
    struct epoll_event ev;
    ev.events = EPOLLIN;
    if (et == EventType::EVENT_TIMER_FD) {
        ev.events |= EPOLLWAKEUP;
    }

    ev.data.ptr = reinterpret_cast<void*>(this);
    ev.data.fd = fd;
    if (epoll_ctl(epFd_, EPOLL_CTL_ADD, fd, &ev) == -1) {
        FI_HILOGE("epoll_ctl failed, error num:%{public}d", errno);
        return RET_ERR;
    }

    return RET_OK;
}

void DeviceStatusMsdpMock::StartThread()
{
    CALL_DEBUG_ENTER;
    thread_ = std::make_shared<std::thread>(&DeviceStatusMsdpMock::LoopingThreadEntry, this);
    thread_ ->detach();
}

void DeviceStatusMsdpMock::LoopingThreadEntry()
{
    SetThreadName("device_status_mock");
    if (callbacks_.empty()) {
        FI_HILOGD("callbacks_ is empty");
        return;
    }
    size_t cbct = callbacks_.size();
    struct epoll_event events[cbct];
    while (alive_) {
        int32_t timeout = 200;
        int32_t nevents = epoll_wait(epFd_, events, cbct, timeout);
        if (nevents == -1) {
            FI_HILOGE("No events available");
            return;
        }
        for (int32_t n = 0; n < nevents; ++n) {
            if (events[n].data.ptr) {
                DeviceStatusMsdpMock *func = const_cast<DeviceStatusMsdpMock *>(this);
                (callbacks_.find(events[n].data.fd)->second)(func);
            }
        }
    }
}

extern "C" IMsdp *Create(void)
{
    CALL_DEBUG_ENTER;
    g_msdpMock = new (std::nothrow) DeviceStatusMsdpMock();
    CHKPP(g_msdpMock);
    return g_msdpMock;
}

extern "C" void Destroy(const IMsdp* algorithm)
{
    CALL_INFO_TRACE;
    if (algorithm != nullptr) {
        FI_HILOGD("algorithm is not nullptr");
        delete algorithm;
    }
}
} // namespace DeviceStatus
} // namespace Msdp
} // namespace OHOS
