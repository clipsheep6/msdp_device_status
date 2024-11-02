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

#include "devicestatus_service.h"

#include <unistd.h>
#include <vector>

#include <ipc_skeleton.h>
#ifdef MSDP_HIVIEWDFX_HITRACE_ENABLE
#include <hitrace_meter.h>
#endif // MSDP_HIVIEWDFX_HITRACE_ENABLE
#ifdef MSDP_HIVIEWDFX_HISYSEVENT_ENABLE
#include <hisysevent.h>
#endif // MSDP_HIVIEWDFX_HISYSEVENT_ENABLE
#include <if_system_ability_manager.h>
#include <iservice_registry.h>
#ifdef MEMMGR_ENABLE
#include <mem_mgr_client.h>
#endif // MEMMGR_ENABLE
#include <system_ability_definition.h>

#include "concurrent_task_client.h"
#include "ddm_adapter.h"
#include "devicestatus_common.h"
#ifdef MSDP_HIVIEWDFX_HISYSEVENT_ENABLE
#include "devicestatus_hisysevent.h"
#endif // MSDP_HIVIEWDFX_HISYSEVENT_ENABLE
#include "dsoftbus_adapter.h"
#include "input_adapter.h"
#include "plugin_manager.h"
#include "qos.h"

#undef LOG_TAG
#define LOG_TAG "DeviceStatusService"

namespace OHOS {
namespace Msdp {
namespace DeviceStatus {
namespace {
constexpr int32_t DEFAULT_WAIT_TIME_MS { 1000 };
constexpr int32_t WAIT_FOR_ONCE { 1 };
constexpr int32_t MAX_N_RETRIES { 100 };

struct device_status_epoll_event {
    int32_t fd { 0 };
    EpollEventType event_type { EPOLL_EVENT_BEGIN };
};

const bool REGISTER_RESULT =
    SystemAbility::MakeAndRegisterAbility(DelayedSpSingleton<DeviceStatusService>::GetInstance().GetRefPtr());
} // namespace

DeviceStatusService::DeviceStatusService()
    : SystemAbility(MSDP_DEVICESTATUS_SERVICE_ID, true)
{
    ddm_ = std::make_unique<DDMAdapter>();
    input_ = std::make_unique<InputAdapter>();
    pluginMgr_ = std::make_unique<PluginManager>(this);
    dsoftbus_ = std::make_unique<DSoftbusAdapter>();
}

DeviceStatusService::~DeviceStatusService()
{}

void DeviceStatusService::OnDump()
{}

void DeviceStatusService::OnAddSystemAbility(int32_t saId, const std::string &deviceId)
{
    FI_HILOGI("OnAddSystemAbility systemAbilityId:%{public}d added!", saId);
#ifdef MEMMGR_ENABLE
    if (saId == MEMORY_MANAGER_SA_ID) {
        Memory::MemMgrClient::GetInstance().NotifyProcessStatus(getpid(), PROCESS_TYPE_SA, PROCESS_STATUS_STARTED,
            MSDP_DEVICESTATUS_SERVICE_ID);
    }
#endif
}

void DeviceStatusService::OnStart()
{
    CALL_INFO_TRACE;
    if (ready_) {
        FI_HILOGE("On start is ready, nothing to do");
        return;
    }

    uint64_t tid = GetThisThreadId();
    delegateTasks_.SetWorkerThreadId(tid);

    if (!Init()) {
        FI_HILOGE("On start call init failed");
        return;
    }
#ifdef MEMMGR_ENABLE
    AddSystemAbilityListener(MEMORY_MANAGER_SA_ID);
#endif
    EnableDSoftbus();
    EnableDDM();
    intention_ = sptr<IntentionService>::MakeSptr(this);
    if (!Publish(intention_)) {
        FI_HILOGE("On start register to system ability manager failed");
        return;
    }
    state_ = ServiceRunningState::STATE_RUNNING;
    ready_ = true;
    worker_ = std::thread([this] { this->OnThread(); });
}

void DeviceStatusService::OnStop()
{
    CALL_INFO_TRACE;
    if (!ready_) {
        return;
    }
    ready_ = false;
    state_ = ServiceRunningState::STATE_EXIT;

    delegateTasks_.PostAsyncTask([]() -> int32_t {
        FI_HILOGD("No op");
        return RET_OK;
    });
    if (worker_.joinable()) {
        worker_.join();
    }
#ifdef MEMMGR_ENABLE
    Memory::MemMgrClient::GetInstance().NotifyProcessStatus(getpid(), PROCESS_TYPE_SA, PROCESS_STATUS_DIED,
        MSDP_DEVICESTATUS_SERVICE_ID);
#endif
}

IDelegateTasks& DeviceStatusService::GetDelegateTasks()
{
    return delegateTasks_;
}

IDeviceManager& DeviceStatusService::GetDeviceManager()
{
    return devMgr_;
}

ITimerManager& DeviceStatusService::GetTimerManager()
{
    return timerMgr_;
}

IDragManager& DeviceStatusService::GetDragManager()
{
    return dragMgr_;
}

ISocketSessionManager& DeviceStatusService::GetSocketSessionManager()
{
    return socketSessionMgr_;
}

IDDMAdapter& DeviceStatusService::GetDDM()
{
    return *ddm_;
}

IPluginManager& DeviceStatusService::GetPluginManager()
{
    return *pluginMgr_;
}

IInputAdapter& DeviceStatusService::GetInput()
{
    return *input_;
}

IDSoftbusAdapter& DeviceStatusService::GetDSoftbus()
{
    return *dsoftbus_;
}

void DeviceStatusService::EnableDSoftbus()
{
    CALL_INFO_TRACE;
    int32_t ret = dsoftbus_->Enable();
    if (ret != RET_OK) {
        FI_HILOGE("Failed to enable dsoftbus, try again later");
        int32_t timerId = timerMgr_.AddTimer(DEFAULT_WAIT_TIME_MS, WAIT_FOR_ONCE,
            [this] { this->EnableDSoftbus(); });
        if (timerId < 0) {
            FI_HILOGE("AddTimer failed, Failed to enable dsoftbus");
        }
    } else {
        FI_HILOGI("Enable dsoftbus successfully");
    }
}

void DeviceStatusService::EnableDDM()
{
    CALL_INFO_TRACE;
    int32_t ret = ddm_->Enable();
    if (ret != RET_OK) {
        FI_HILOGE("Failed to enable DistributedDeviceManager, try again later");
        int32_t timerId = timerMgr_.AddTimer(DEFAULT_WAIT_TIME_MS, WAIT_FOR_ONCE,
            [this] { this->EnableDDM(); });
        if (timerId < 0) {
            FI_HILOGE("AddTimer failed, Failed to enable DistributedDeviceManager");
        }
    } else {
        FI_HILOGI("Enable DistributedDeviceManager successfully");
    }
}

int32_t DeviceStatusService::Dump(int32_t fd, const std::vector<std::u16string> &args)
{
    CALL_DEBUG_ENTER;
    if (fd < 0) {
        FI_HILOGE("fd is invalid");
        return RET_NG;
    }
    if (args.empty()) {
        FI_HILOGE("Param cannot be empty");
        dprintf(fd, "param cannot be empty\n");
        DS_DUMPER->DumpHelpInfo(fd);
        return RET_NG;
    }
    std::vector<std::string> argList;
    std::transform(args.begin(), args.end(), std::back_inserter(argList),
        [](const std::u16string &arg) {
        return Str16ToStr8(arg);
    });

    std::vector<Data> datas;
    for (auto type = TYPE_ABSOLUTE_STILL; type <= TYPE_LID_OPEN; type = static_cast<Type>(type + 1)) {
        Data data = GetCache(type);
        if (data.value != OnChangedValue::VALUE_INVALID) {
            datas.emplace_back(data);
        }
    }
    DS_DUMPER->ParseCommand(fd, argList, datas);
    return RET_OK;
}

bool DeviceStatusService::Init()
{
    CALL_INFO_TRACE;
    if (devicestatusManager_ == nullptr) {
        FI_HILOGW("devicestatusManager_ is nullptr");
        devicestatusManager_ = std::make_shared<DeviceStatusManager>();
    }
    if (!devicestatusManager_->Init()) {
        FI_HILOGE("OnStart init failed");
        return false;
    }
    if (EpollCreate() != RET_OK) {
        FI_HILOGE("Create epoll failed");
        return false;
    }
    if (InitDelegateTasks() != RET_OK) {
        FI_HILOGE("Delegate tasks init failed");
        goto INIT_FAIL;
    }
    if (InitTimerMgr() != RET_OK) {
        FI_HILOGE("TimerMgr init failed");
        goto INIT_FAIL;
    }
    if (devMgr_.Init(this) != RET_OK) {
        FI_HILOGE("DevMgr init failed");
        goto INIT_FAIL;
    }
    if (dragMgr_.Init(this) != RET_OK) {
        FI_HILOGE("Drag manager init failed");
        goto INIT_FAIL;
    }
    if (DS_DUMPER->Init(this) != RET_OK) {
        FI_HILOGE("Dump init failed");
        goto INIT_FAIL;
    }
    return true;

INIT_FAIL:
    EpollClose();
    return false;
}

bool DeviceStatusService::IsServiceReady() const
{
    return ready_;
}

void DeviceStatusService::Subscribe(Type type, ActivityEvent event, ReportLatencyNs latency,
    sptr<IRemoteDevStaCallback> callback)
{
    FI_HILOGI("Enter event:%{public}d, latency:%{public}d", event, latency);
    CHKPV(devicestatusManager_);
    auto appInfo = std::make_shared<AppInfo>();
    appInfo->uid = GetCallingUid();
    appInfo->pid = GetCallingPid();
    appInfo->tokenId = GetCallingTokenID();
    devicestatusManager_->GetPackageName(appInfo->tokenId, appInfo->packageName);
    appInfo->type = type;
    appInfo->callback = callback;
    DS_DUMPER->SaveAppInfo(appInfo);
#ifdef MSDP_HIVIEWDFX_HITRACE_ENABLE
    StartTrace(HITRACE_TAG_MSDP, "serviceSubscribeStart");
#endif // MSDP_HIVIEWDFX_HITRACE_ENABLE
    devicestatusManager_->Subscribe(type, event, latency, callback);
#ifdef MSDP_HIVIEWDFX_HITRACE_ENABLE
    FinishTrace(HITRACE_TAG_MSDP);
#endif // MSDP_HIVIEWDFX_HITRACE_ENABLE
#ifdef MSDP_HIVIEWDFX_HISYSEVENT_ENABLE
    ReportSensorSysEvent(type, true);
    WriteSubscribeHiSysEvent(appInfo->uid, appInfo->packageName, type);
#endif
}

void DeviceStatusService::Unsubscribe(Type type, ActivityEvent event, sptr<IRemoteDevStaCallback> callback)
{
    FI_HILOGI("Enter event:%{public}d", event);
    CHKPV(devicestatusManager_);
    auto appInfo = std::make_shared<AppInfo>();
    appInfo->uid = IPCSkeleton::GetCallingUid();
    appInfo->pid = IPCSkeleton::GetCallingPid();
    appInfo->tokenId = IPCSkeleton::GetCallingTokenID();
    appInfo->packageName = DS_DUMPER->GetPackageName(appInfo->tokenId);
    appInfo->type = type;
    appInfo->callback = callback;
    DS_DUMPER->RemoveAppInfo(appInfo);
#ifdef MSDP_HIVIEWDFX_HITRACE_ENABLE
    StartTrace(HITRACE_TAG_MSDP, "serviceUnSubscribeStart");
#endif // MSDP_HIVIEWDFX_HITRACE_ENABLE
    devicestatusManager_->Unsubscribe(type, event, callback);
#ifdef MSDP_HIVIEWDFX_HITRACE_ENABLE
    FinishTrace(HITRACE_TAG_MSDP);
#endif // MSDP_HIVIEWDFX_HITRACE_ENABLE
#ifdef MSDP_HIVIEWDFX_HISYSEVENT_ENABLE
    ReportSensorSysEvent(type, false);
    WriteUnSubscribeHiSysEvent(appInfo->uid, appInfo->packageName, type);
#endif
}

Data DeviceStatusService::GetCache(const Type &type)
{
    CALL_DEBUG_ENTER;
    if (devicestatusManager_ == nullptr) {
        Data data = {type, OnChangedValue::VALUE_EXIT};
        data.value = OnChangedValue::VALUE_INVALID;
        FI_HILOGI("Get latest device status data func is nullptr, return default!");
        return data;
    }
    return devicestatusManager_->GetLatestDeviceStatusData(type);
}

#ifdef MSDP_HIVIEWDFX_HISYSEVENT_ENABLE
void DeviceStatusService::ReportSensorSysEvent(int32_t type, bool enable)
{
    auto callerToken = GetCallingTokenID();
    std::string packageName;
    CHKPV(devicestatusManager_);
    devicestatusManager_->GetPackageName(callerToken, packageName);
    auto uid = GetCallingUid();
    std::string str = enable ? "Subscribe" : "Unsubscribe";
    int32_t ret = HiSysEventWrite(
        OHOS::HiviewDFX::HiSysEvent::Domain::MSDP,
        str,
        OHOS::HiviewDFX::HiSysEvent::EventType::STATISTIC,
        "UID", uid,
        "PKGNAME", packageName,
        "TYPE", type);
    if (ret != 0) {
        FI_HILOGE("HiviewDFX write failed, ret:%{public}d", ret);
    }
}
#endif

int32_t DeviceStatusService::AllocSocketFd(const std::string &programName, int32_t moduleType,
    int32_t &toReturnClientFd, int32_t &tokenType)
{
    FI_HILOGI("Enter, programName:%{public}s, moduleType:%{public}d", programName.c_str(), moduleType);

    toReturnClientFd = -1;
    int32_t serverFd = -1;
    int32_t pid = GetCallingPid();
    int32_t uid = GetCallingUid();
    int32_t ret = delegateTasks_.PostSyncTask(
        [this, programName, moduleType, uid, pid, &serverFd, &toReturnClientFd, &tokenType] {
            return this->AddSocketPairInfo(programName, moduleType, uid, pid, serverFd,
                toReturnClientFd, tokenType);
        });
    if (ret != RET_OK) {
        FI_HILOGE("Call Add socket pair info failed, return:%{public}d", ret);
        return RET_ERR;
    }
    FI_HILOGI("Leave, programName:%{public}s, moduleType:%{public}d, alloc success",
        programName.c_str(), moduleType);
    return RET_OK;
}

void DeviceStatusService::OnConnected(SessionPtr s)
{
    CHKPV(s);
    FI_HILOGI("fd:%{public}d", s->GetFd());
}

void DeviceStatusService::OnDisconnected(SessionPtr s)
{
    CHKPV(s);
    FI_HILOGW("Enter, session, fd:%{public}d", s->GetFd());
}

int32_t DeviceStatusService::AddEpoll(EpollEventType type, int32_t fd)
{
    if (!(type >= EPOLL_EVENT_BEGIN && type < EPOLL_EVENT_END)) {
        FI_HILOGE("Invalid type:%{public}d", type);
        return RET_ERR;
    }
    if (fd < 0) {
        FI_HILOGE("Invalid fd:%{public}d", fd);
        return RET_ERR;
    }
    auto eventData = static_cast<device_status_epoll_event*>(malloc(sizeof(device_status_epoll_event)));
    if (!eventData) {
        FI_HILOGE("Malloc failed");
        return RET_ERR;
    }
    eventData->fd = fd;
    eventData->event_type = type;
    FI_HILOGD("EventData:[fd:%{public}d, type:%{public}d]", eventData->fd, eventData->event_type);

    struct epoll_event ev {};
    ev.events = EPOLLIN;
    ev.data.ptr = eventData;
    if (EpollCtl(fd, EPOLL_CTL_ADD, ev) != RET_OK) {
        free(eventData);
        eventData = nullptr;
        ev.data.ptr = nullptr;
        FI_HILOGE("EpollCtl failed");
        return RET_ERR;
    }
    return RET_OK;
}

int32_t DeviceStatusService::DelEpoll(EpollEventType type, int32_t fd)
{
    if (!(type >= EPOLL_EVENT_BEGIN && type < EPOLL_EVENT_END)) {
        FI_HILOGE("Invalid type:%{public}d", type);
        return RET_ERR;
    }
    if (fd < 0) {
        FI_HILOGE("Invalid fd:%{public}d", fd);
        return RET_ERR;
    }
    struct epoll_event ev {};
    if (EpollCtl(fd, EPOLL_CTL_DEL, ev) != RET_OK) {
        FI_HILOGE("DelEpoll failed");
        return RET_ERR;
    }
    return RET_OK;
}

bool DeviceStatusService::IsRunning() const
{
    return (state_ == ServiceRunningState::STATE_RUNNING);
}

int32_t DeviceStatusService::InitDelegateTasks()
{
    CALL_INFO_TRACE;
    if (!delegateTasks_.Init()) {
        FI_HILOGE("The delegate task init failed");
        return RET_ERR;
    }
    int32_t ret = AddEpoll(EPOLL_EVENT_ETASK, delegateTasks_.GetReadFd());
    if (ret != RET_OK) {
        FI_HILOGE("AddEpoll error ret:%{public}d", ret);
    }
    FI_HILOGI("AddEpoll, epollfd:%{public}d, fd:%{public}d", epollFd_, delegateTasks_.GetReadFd());
    return ret;
}

int32_t DeviceStatusService::InitTimerMgr()
{
    CALL_INFO_TRACE;
    int32_t ret = timerMgr_.Init(this);
    if (ret != RET_OK) {
        FI_HILOGE("TimerMgr init failed");
        return ret;
    }
    ret = AddEpoll(EPOLL_EVENT_TIMER, timerMgr_.GetTimerFd());
    if (ret != RET_OK) {
        FI_HILOGE("AddEpoll for timer failed");
    }
    return ret;
}

void DeviceStatusService::OnThread()
{
    SetThreadName(std::string("os_ds_service"));
    uint64_t tid = GetThisThreadId();
    std::unordered_map<std::string, std::string> payload;
    payload["pid"] = std::to_string(getpid());
    ConcurrentTask::ConcurrentTaskClient::GetInstance().RequestAuth(payload);
    auto ret = QOS::SetQosForOtherThread(QOS::QosLevel::QOS_USER_INTERACTIVE, tid);
    if (ret != 0) {
        FI_HILOGE("Set device status thread qos failed, ret:%{public}d", ret);
    } else {
        FI_HILOGW("Set device status thread qos success");
    }
    delegateTasks_.SetWorkerThreadId(tid);
    FI_HILOGD("Main worker thread start, tid:%{public}" PRId64 "", tid);
    EnableSocketSessionMgr(MAX_N_RETRIES);
    EnableDevMgr(MAX_N_RETRIES);

    while (state_ == ServiceRunningState::STATE_RUNNING) {
        struct epoll_event ev[MAX_EVENT_SIZE] {};
        int32_t count = EpollWait(MAX_EVENT_SIZE, -1, ev[0]);
        for (int32_t i = 0; i < count && state_ == ServiceRunningState::STATE_RUNNING; i++) {
            auto epollEvent = reinterpret_cast<device_status_epoll_event*>(ev[i].data.ptr);
            CHKPC(epollEvent);
            if (epollEvent->event_type == EPOLL_EVENT_SOCKET) {
                OnSocketEvent(ev[i]);
            } else if (epollEvent->event_type == EPOLL_EVENT_ETASK) {
                OnDelegateTask(ev[i]);
            } else if (epollEvent->event_type == EPOLL_EVENT_TIMER) {
                OnTimeout(ev[i]);
            } else if (epollEvent->event_type == EPOLL_EVENT_DEVICE_MGR) {
                OnDeviceMgr(ev[i]);
            } else {
                FI_HILOGW("Unknown epoll event type:%{public}d", epollEvent->event_type);
            }
        }
    }
    FI_HILOGD("Main worker thread stop, tid:%{public}" PRId64 "", tid);
}

void DeviceStatusService::OnSocketEvent(const struct epoll_event &ev)
{
    CALL_INFO_TRACE;
    if ((ev.events & EPOLLIN) == EPOLLIN) {
        socketSessionMgr_.Dispatch(ev);
    } else if ((ev.events & (EPOLLHUP | EPOLLERR)) != 0) {
        FI_HILOGE("Epoll hangup:%{public}s", ::strerror(errno));
    }
}

void DeviceStatusService::OnDelegateTask(const struct epoll_event &ev)
{
    if ((ev.events & EPOLLIN) == 0) {
        FI_HILOGW("Not epollin");
        return;
    }
    DelegateTasks::TaskData data {};
    ssize_t res = read(delegateTasks_.GetReadFd(), &data, sizeof(data));
    if (res == -1) {
        FI_HILOGW("Read failed erron:%{public}d", errno);
    }
    FI_HILOGD("RemoteRequest notify td:%{public}" PRId64 ", std:%{public}" PRId64 ""
        ", taskId:%{public}d", GetThisThreadId(), data.tid, data.taskId);
    delegateTasks_.ProcessTasks();
}

void DeviceStatusService::OnTimeout(const struct epoll_event &ev)
{
    CALL_DEBUG_ENTER;
    if ((ev.events & EPOLLIN) == EPOLLIN) {
        uint64_t expiration {};
        ssize_t ret = read(timerMgr_.GetTimerFd(), &expiration, sizeof(expiration));
        if (ret < 0) {
            FI_HILOGE("Read expiration failed:%{public}s", strerror(errno));
        }
        timerMgr_.ProcessTimers();
    } else if ((ev.events & (EPOLLHUP | EPOLLERR)) != 0) {
        FI_HILOGE("Epoll hangup:%{public}s", strerror(errno));
    }
}

void DeviceStatusService::OnDeviceMgr(const struct epoll_event &ev)
{
    CALL_DEBUG_ENTER;
    if ((ev.events & EPOLLIN) == EPOLLIN) {
        devMgr_.Dispatch(ev);
    } else if ((ev.events & (EPOLLHUP | EPOLLERR)) != 0) {
        FI_HILOGE("Epoll hangup:%{public}s", strerror(errno));
    }
}

int32_t DeviceStatusService::EnableSocketSessionMgr(int32_t nRetries)
{
    CALL_INFO_TRACE;
    int32_t ret = socketSessionMgr_.Enable();
    if (ret != RET_OK) {
        FI_HILOGE("Failed to enable SocketSessionManager");
        if (nRetries > 0) {
            auto timerId = timerMgr_.AddTimer(DEFAULT_WAIT_TIME_MS, WAIT_FOR_ONCE,
                [this, nRetries]() {
                    return EnableSocketSessionMgr(nRetries - 1);
                });
            if (timerId < 0) {
                FI_HILOGE("AddTimer failed, Failed to enable SocketSessionManager");
            }
        } else {
            FI_HILOGE("Maximum number of retries exceeded, Failed to enable SocketSessionManager");
        }
        return ret;
    }
    FI_HILOGI("Enable SocketSessionManager successfully");
    AddEpoll(EPOLL_EVENT_SOCKET, socketSessionMgr_.GetFd());
    return RET_OK;
}

void DeviceStatusService::DisableSocketSessionMgr()
{
    CALL_INFO_TRACE;
    DelEpoll(EPOLL_EVENT_SOCKET, socketSessionMgr_.GetFd());
    socketSessionMgr_.Disable();
}

int32_t DeviceStatusService::EnableDevMgr(int32_t nRetries)
{
    CALL_INFO_TRACE;
    static int32_t timerId { -1 };
    int32_t ret = devMgr_.Enable();
    if (ret != RET_OK) {
        FI_HILOGE("Failed to enable device manager");
        if (nRetries > 0) {
            timerId = timerMgr_.AddTimer(DEFAULT_WAIT_TIME_MS, WAIT_FOR_ONCE,
                [this, nRetries] { return this->EnableDevMgr(nRetries - 1); });
            if (timerId < 0) {
                FI_HILOGE("AddTimer failed, Failed to enable device manager");
            }
        } else {
            FI_HILOGE("Maximum number of retries exceeded, Failed to enable device manager");
        }
        return ret;
    }
    AddEpoll(EPOLL_EVENT_DEVICE_MGR, devMgr_.GetFd());
    if (timerId >= 0) {
        timerMgr_.RemoveTimer(timerId);
        timerId = -1;
    }
    return RET_OK;
}

void DeviceStatusService::DisableDevMgr()
{
    DelEpoll(EPOLL_EVENT_DEVICE_MGR, devMgr_.GetFd());
    devMgr_.Disable();
}

int32_t DeviceStatusService::RegisterCoordinationListener(bool isCompatible)
{
    CALL_DEBUG_ENTER;
    (void)(isCompatible);
#ifdef OHOS_BUILD_ENABLE_COORDINATION
#ifndef OHOS_BUILD_ENABLE_INTENTION_FRAMEWORK
    int32_t pid = GetCallingPid();
    int32_t ret = delegateTasks_.PostSyncTask(
        [this, pid] { return this->OnRegisterCoordinationListener(pid); });
    if (ret != RET_OK) {
        FI_HILOGE("On register coordination listener failed, ret:%{public}d", ret);
        return RET_ERR;
    }
#endif // OHOS_BUILD_ENABLE_INTENTION_FRAMEWORK
#endif // OHOS_BUILD_ENABLE_COORDINATION
    return RET_OK;
}

int32_t DeviceStatusService::UnregisterCoordinationListener(bool isCompatible)
{
    CALL_DEBUG_ENTER;
    (void)(isCompatible);
#ifdef OHOS_BUILD_ENABLE_COORDINATION
#ifndef OHOS_BUILD_ENABLE_INTENTION_FRAMEWORK
    int32_t pid = GetCallingPid();
    int32_t ret = delegateTasks_.PostSyncTask(
        [this, pid] { return this->OnUnregisterCoordinationListener(pid); });
    if (ret != RET_OK) {
        FI_HILOGE("On unregister coordination listener failed, ret:%{public}d", ret);
        return RET_ERR;
    }
#endif // OHOS_BUILD_ENABLE_INTENTION_FRAMEWORK
#endif // OHOS_BUILD_ENABLE_COORDINATION
    return RET_OK;
}

int32_t DeviceStatusService::PrepareCoordination(int32_t userData, bool isCompatible)
{
    CALL_DEBUG_ENTER;
    (void)(isCompatible);
#ifdef OHOS_BUILD_ENABLE_COORDINATION
#ifndef OHOS_BUILD_ENABLE_INTENTION_FRAMEWORK
    int32_t pid = GetCallingPid();
    int32_t ret = delegateTasks_.PostSyncTask(
        [this, pid, userData] { return this->OnPrepareCoordination(pid, userData); });
    if (ret != RET_OK) {
        FI_HILOGE("On prepare coordination failed, ret:%{public}d", ret);
        return ret;
    }
#endif // OHOS_BUILD_ENABLE_INTENTION_FRAMEWORK
#else
    (void)(userData);
#endif // OHOS_BUILD_ENABLE_COORDINATION
    return RET_OK;
}

int32_t DeviceStatusService::UnprepareCoordination(int32_t userData, bool isCompatible)
{
    CALL_DEBUG_ENTER;
    (void)(isCompatible);
#ifdef OHOS_BUILD_ENABLE_COORDINATION
#ifndef OHOS_BUILD_ENABLE_INTENTION_FRAMEWORK
    int32_t pid = GetCallingPid();
    int32_t ret = delegateTasks_.PostSyncTask(
        [this, pid, userData] { return this->OnUnprepareCoordination(pid, userData); });
    if (ret != RET_OK) {
        FI_HILOGE("OnUnprepareCoordination failed, ret:%{public}d", ret);
        return ret;
    }
#endif // OHOS_BUILD_ENABLE_INTENTION_FRAMEWORK
#else
    (void)(userData);
#endif // OHOS_BUILD_ENABLE_COORDINATION
    return RET_OK;
}

int32_t DeviceStatusService::ActivateCoordination(int32_t userData,
    const std::string &remoteNetworkId, int32_t startDeviceId, bool isCompatible)
{
    CALL_DEBUG_ENTER;
    (void)(isCompatible);
#ifdef OHOS_BUILD_ENABLE_COORDINATION
#ifndef OHOS_BUILD_ENABLE_INTENTION_FRAMEWORK
    int32_t pid = GetCallingPid();
    int32_t ret = delegateTasks_.PostSyncTask([this, pid, userData, remoteNetworkId, startDeviceId] {
        return this->OnActivateCoordination(pid, userData, remoteNetworkId, startDeviceId);
    });
    if (ret != RET_OK) {
        FI_HILOGE("On activate coordination failed, ret:%{public}d", ret);
        return ret;
    }
#endif // OHOS_BUILD_ENABLE_INTENTION_FRAMEWORK
#else
    (void)(userData);
    (void)(remoteNetworkId);
    (void)(startDeviceId);
#endif // OHOS_BUILD_ENABLE_COORDINATION
    return RET_OK;
}

int32_t DeviceStatusService::DeactivateCoordination(int32_t userData, bool isUnchained,
    bool isCompatible)
{
    CALL_DEBUG_ENTER;
    (void)(isCompatible);
#ifdef OHOS_BUILD_ENABLE_COORDINATION
#ifndef OHOS_BUILD_ENABLE_INTENTION_FRAMEWORK
    int32_t pid = GetCallingPid();
    int32_t ret = delegateTasks_.PostSyncTask([this, pid, userData, isUnchained] {
        return this->OnDeactivateCoordination(pid, userData, isUnchained);
    });
    if (ret != RET_OK) {
        FI_HILOGE("On deactivate coordination failed, ret:%{public}d", ret);
        return ret;
    }
#endif // OHOS_BUILD_ENABLE_INTENTION_FRAMEWORK
#else
    (void)(userData);
    (void)(isUnchained);
#endif // OHOS_BUILD_ENABLE_COORDINATION
    return RET_OK;
}

int32_t DeviceStatusService::GetCoordinationState(int32_t userData, const std::string &networkId,
    bool isCompatible)
{
    CALL_DEBUG_ENTER;
    (void)(isCompatible);
#ifdef OHOS_BUILD_ENABLE_COORDINATION
#ifndef OHOS_BUILD_ENABLE_INTENTION_FRAMEWORK
    int32_t pid = GetCallingPid();
    int32_t ret = delegateTasks_.PostSyncTask([this, pid, userData, networkId] {
        return this->OnGetCoordinationState(pid, userData, networkId);
    });
    if (ret != RET_OK) {
        FI_HILOGE("OnGetCoordinationState failed, ret:%{public}d", ret);
        return ret;
    }
#endif // OHOS_BUILD_ENABLE_INTENTION_FRAMEWORK
#else
    (void)(userData);
    (void)(networkId);
    FI_HILOGW("Get coordination state does not support");
#endif // OHOS_BUILD_ENABLE_COORDINATION
    return RET_OK;
}

int32_t DeviceStatusService::GetCoordinationState(const std::string &udId, bool &state)
{
    CALL_DEBUG_ENTER;
#ifdef OHOS_BUILD_ENABLE_COORDINATION
#ifndef OHOS_BUILD_ENABLE_INTENTION_FRAMEWORK
    int32_t ret = delegateTasks_.PostSyncTask([this, udId, &state] {
        return this->OnGetCoordinationStateSync(pid, udId, state);
    });
    if (ret != RET_OK) {
        FI_HILOGE("OnGetCoordinationStateSync failed, ret:%{public}d", ret);
        return ret;
    }
#endif // OHOS_BUILD_ENABLE_INTENTION_FRAMEWORK
#else
    (void)(udId);
    FI_HILOGW("Get coordination state does not support");
#endif // OHOS_BUILD_ENABLE_COORDINATION
    return RET_OK;
}

int32_t DeviceStatusService::AddDraglistener()
{
    CALL_DEBUG_ENTER;
    int32_t session = GetCallingPid();
    int32_t ret = delegateTasks_.PostSyncTask([this, session] {
        return this->dragMgr_.AddListener(session);
    });
    if (ret != RET_OK) {
        FI_HILOGE("AddListener failed, ret:%{public}d", ret);
    }
    return ret;
}

int32_t DeviceStatusService::RemoveDraglistener()
{
    CALL_DEBUG_ENTER;
    int32_t session = GetCallingPid();
    int32_t ret = delegateTasks_.PostSyncTask([this, session] {
        return this->dragMgr_.RemoveListener(session);
    });
    if (ret != RET_OK) {
        FI_HILOGE("Remove listener failed, ret:%{public}d", ret);
    }
    return ret;
}

int32_t DeviceStatusService::AddSubscriptListener()
{
    CALL_DEBUG_ENTER;
    int32_t session = GetCallingPid();
    int32_t ret = delegateTasks_.PostSyncTask([this, session] {
        return this->dragMgr_.AddSubscriptListener(session);
    });
    if (ret != RET_OK) {
        FI_HILOGE("AddListener failed, ret:%{public}d", ret);
    }
    return ret;
}

int32_t DeviceStatusService::RemoveSubscriptListener()
{
    CALL_DEBUG_ENTER;
    int32_t session = GetCallingPid();
    int32_t ret = delegateTasks_.PostSyncTask([this, session] {
        return this->dragMgr_.RemoveSubscriptListener(session);
    });
    if (ret != RET_OK) {
        FI_HILOGE("AddListener failed, ret:%{public}d", ret);
    }
    return ret;
}

int32_t DeviceStatusService::StartDrag(const DragData &dragData)
{
    CALL_DEBUG_ENTER;
    int32_t session = GetCallingPid();
    int32_t ret = delegateTasks_.PostSyncTask([this, &dragData, session] {
        return this->dragMgr_.StartDrag(std::cref(dragData), session);
    });
    if (ret != RET_OK) {
        FI_HILOGE("StartDrag failed, ret:%{public}d", ret);
    }
    return ret;
}

int32_t DeviceStatusService::StopDrag(const DragDropResult &dropResult)
{
    CALL_DEBUG_ENTER;
    int32_t ret = delegateTasks_.PostSyncTask([this, dropResult] {
        return this->dragMgr_.StopDrag(dropResult, std::string());
    });
    if (ret != RET_OK) {
        FI_HILOGE("StopDrag failed, ret:%{public}d", ret);
    }
    return ret;
}

int32_t DeviceStatusService::SetDragWindowVisible(bool visible, bool isForce)
{
    CALL_DEBUG_ENTER;
    int32_t ret = delegateTasks_.PostSyncTask([this, visible, isForce] {
        return this->dragMgr_.OnSetDragWindowVisible(visible, isForce);
    });
    if (ret != RET_OK) {
        FI_HILOGE("On set drag window visible failed, ret:%{public}d", ret);
    }
    return ret;
}

int32_t DeviceStatusService::EnterTextEditorArea(bool enable)
{
    CALL_DEBUG_ENTER;
    int32_t ret = delegateTasks_.PostSyncTask([this, enable] {
        return this->dragMgr_.EnterTextEditorArea(enable);
    });
    if (ret != RET_OK) {
        FI_HILOGE("Enter Text Editor Area failed, ret:%{public}d", ret);
    }
    return ret;
}

int32_t DeviceStatusService::GetShadowOffset(ShadowOffset &shadowOffset)
{
    CALL_DEBUG_ENTER;
    int32_t ret = delegateTasks_.PostSyncTask([this, &shadowOffset] {
        return this->dragMgr_.OnGetShadowOffset(shadowOffset);
    });
    if (ret != RET_OK) {
        FI_HILOGE("Get shadow offset failed, ret:%{public}d", ret);
    }
    return ret;
}

int32_t DeviceStatusService::UpdateShadowPic(const ShadowInfo &shadowInfo)
{
    CALL_DEBUG_ENTER;
    int32_t ret = delegateTasks_.PostSyncTask([this, &shadowInfo] {
        return this->dragMgr_.UpdateShadowPic(std::cref(shadowInfo));
    });
    if (ret != RET_OK) {
        FI_HILOGE("Update shadow picture failed, ret:%{public}d", ret);
    }
    return ret;
}

int32_t DeviceStatusService::GetDragData(DragData &dragData)
{
    CALL_DEBUG_ENTER;
    int32_t ret = delegateTasks_.PostSyncTask([this, &dragData] {
        return this->dragMgr_.GetDragData(dragData);
    });
    if (ret != RET_OK) {
        FI_HILOGE("Get drag data failed, ret:%{public}d", ret);
    }
    return ret;
}

int32_t DeviceStatusService::GetDragState(DragState &dragState)
{
    CALL_DEBUG_ENTER;
    int32_t ret = delegateTasks_.PostSyncTask([this, &dragState] {
        return this->dragMgr_.GetDragState(dragState);
    });
    if (ret != RET_OK) {
        FI_HILOGE("Get drag state failed, ret:%{public}d", ret);
    }
    return ret;
}

int32_t DeviceStatusService::UpdateDragStyle(DragCursorStyle style)
{
    CALL_DEBUG_ENTER;
    int32_t tid = static_cast<int32_t>(GetCallingTokenID());
    int32_t pid = GetCallingPid();
    int32_t ret = delegateTasks_.PostSyncTask([this, style, pid, tid] {
        return this->dragMgr_.UpdateDragStyle(style, pid, tid);
    });
    if (ret != RET_OK) {
        FI_HILOGE("Update drag style failed, ret:%{public}d", ret);
    }
    return ret;
}

int32_t DeviceStatusService::GetUdKey(std::string &udKey)
{
    CALL_DEBUG_ENTER;
    int32_t ret = delegateTasks_.PostSyncTask([this, &udKey] {
        return this->dragMgr_.GetUdKey(udKey);
    });
    if (ret != RET_OK) {
        FI_HILOGE("Get udkey failed, ret:%{public}d", ret);
    }
    return ret;
}

int32_t DeviceStatusService::GetDragTargetPid()
{
    CALL_DEBUG_ENTER;
    int32_t ret = delegateTasks_.PostSyncTask([this] {
        return this->dragMgr_.GetDragTargetPid();
    });
    if (ret != RET_OK) {
        FI_HILOGE("Get drag target pid failed, ret:%{public}d", ret);
    }
    return ret;
}

int32_t DeviceStatusService::GetDragAction(DragAction &dragAction)
{
    int32_t ret = delegateTasks_.PostSyncTask([this, &dragAction] {
        return this->dragMgr_.GetDragAction(dragAction);
    });
    if (ret != RET_OK) {
        FI_HILOGE("Get drag action failed, ret:%{public}d", ret);
    }
    return ret;
}

int32_t DeviceStatusService::GetExtraInfo(std::string &extraInfo)
{
    int32_t ret = delegateTasks_.PostSyncTask([this, &extraInfo] {
        return this->dragMgr_.GetExtraInfo(extraInfo);
    });
    if (ret != RET_OK) {
        FI_HILOGE("Get extraInfo failed, ret:%{public}d", ret);
    }
    return ret;
}

#ifdef OHOS_BUILD_ENABLE_COORDINATION
#ifndef OHOS_BUILD_ENABLE_INTENTION_FRAMEWORK
int32_t DeviceStatusService::OnRegisterCoordinationListener(int32_t pid)
{
    CALL_DEBUG_ENTER;
    SessionPtr sess = GetSession(GetClientFd(pid));
    CHKPR(sess, RET_ERR);
    sptr<CoordinationEventManager::EventInfo> event = new (std::nothrow) CoordinationEventManager::EventInfo();
    CHKPR(event, RET_ERR);
    event->type = CoordinationEventManager::EventType::LISTENER;
    event->sess = sess;
    event->msgId = MessageId::COORDINATION_ADD_LISTENER;
    return RET_OK;
}

int32_t DeviceStatusService::OnUnregisterCoordinationListener(int32_t pid)
{
    CALL_DEBUG_ENTER;
    SessionPtr sess = GetSession(GetClientFd(pid));
    sptr<CoordinationEventManager::EventInfo> event = new (std::nothrow) CoordinationEventManager::EventInfo();
    CHKPR(event, RET_ERR);
    event->type = CoordinationEventManager::EventType::LISTENER;
    event->sess = sess;
    return RET_OK;
}

int32_t DeviceStatusService::OnPrepareCoordination(int32_t pid, int32_t userData)
{
    CALL_DEBUG_ENTER;
    std::string networkId;
    CoordinationMessage msg = CoordinationMessage::PREPARE;
    NetPacket pkt(MessageId::COORDINATION_MESSAGE);
    pkt << userData << networkId << static_cast<int32_t>(msg);
    if (pkt.ChkRWError()) {
        FI_HILOGE("Packet write data failed");
        return RET_ERR;
    }
    SessionPtr sess = GetSession(GetClientFd(pid));
    CHKPR(sess, RET_ERR);
    if (!sess->SendMsg(pkt)) {
        FI_HILOGE("Sending failed");
        return MSG_SEND_FAIL;
    }
    return RET_OK;
}

int32_t DeviceStatusService::OnUnprepareCoordination(int32_t pid, int32_t userData)
{
    CALL_DEBUG_ENTER;
    std::string networkId;
    CoordinationMessage msg = CoordinationMessage::UNPREPARE;
    NetPacket pkt(MessageId::COORDINATION_MESSAGE);
    pkt << userData << networkId << static_cast<int32_t>(msg);
    if (pkt.ChkRWError()) {
        FI_HILOGE("Packet write data failed");
        return RET_ERR;
    }
    SessionPtr sess = GetSession(GetClientFd(pid));
    CHKPR(sess, RET_ERR);
    if (!sess->SendMsg(pkt)) {
        FI_HILOGE("Sending failed");
        return MSG_SEND_FAIL;
    }
    return RET_OK;
}

int32_t DeviceStatusService::OnActivateCoordination(int32_t pid,
    int32_t userData, const std::string &remoteNetworkId, int32_t startDeviceId)
{
    CALL_DEBUG_ENTER;
    SessionPtr sess = GetSession(GetClientFd(pid));
    CHKPR(sess, RET_ERR);
    sptr<CoordinationEventManager::EventInfo> event = new (std::nothrow) CoordinationEventManager::EventInfo();
    CHKPR(event, RET_ERR);
    event->type = CoordinationEventManager::EventType::START;
    event->sess = sess;
    event->msgId = MessageId::COORDINATION_MESSAGE;
    event->userData = userData;
    return RET_OK;
}

int32_t DeviceStatusService::OnDeactivateCoordination(int32_t pid, int32_t userData, bool isUnchained)
{
    CALL_DEBUG_ENTER;
    SessionPtr sess = GetSession(GetClientFd(pid));
    CHKPR(sess, RET_ERR);
    sptr<CoordinationEventManager::EventInfo> event = new (std::nothrow) CoordinationEventManager::EventInfo();
    CHKPR(event, RET_ERR);
    event->type = CoordinationEventManager::EventType::STOP;
    event->sess = sess;
    event->msgId = MessageId::COORDINATION_MESSAGE;
    event->userData = userData;
    return RET_OK;
}

int32_t DeviceStatusService::OnGetCoordinationState(
    int32_t pid, int32_t userData, const std::string &networkId)
{
    CALL_DEBUG_ENTER;
    SessionPtr sess = GetSession(GetClientFd(pid));
    CHKPR(sess, RET_ERR);
    sptr<CoordinationEventManager::EventInfo> event = new (std::nothrow) CoordinationEventManager::EventInfo();
    CHKPR(event, RET_ERR);
    event->type = CoordinationEventManager::EventType::STATE;
    event->sess = sess;
    event->msgId = MessageId::COORDINATION_GET_STATE;
    event->userData = userData;
    return RET_OK;
}

int32_t DeviceStatusService::OnGetCoordinationStateSync(const std::string &udId, bool &state)
{
    CALL_DEBUG_ENTER;
    return RET_OK;
}

int32_t DeviceStatusService::OnAddHotAreaListener(int32_t pid)
{
    CALL_DEBUG_ENTER;
    auto sess = GetSession(GetClientFd(pid));
    CHKPR(sess, RET_ERR);
    sptr<CoordinationHotArea::HotAreaInfo> event = new (std::nothrow) CoordinationHotArea::HotAreaInfo();
    CHKPR(event, RET_ERR);
    event->sess = sess;
    event->msgId = MessageId::HOT_AREA_ADD_LISTENER;
    HOT_AREA->AddHotAreaListener(event);
    return RET_OK;
}

int32_t DeviceStatusService::OnRemoveHotAreaListener(int32_t pid)
{
    CALL_DEBUG_ENTER;
    auto sess = GetSession(GetClientFd(pid));
    CHKPR(sess, RET_ERR);
    sptr<CoordinationHotArea::HotAreaInfo> event = new (std::nothrow) CoordinationHotArea::HotAreaInfo();
    CHKPR(event, RET_ERR);
    event->sess = sess;
    HOT_AREA->RemoveHotAreaListener(event);
    return RET_OK;
}
#endif // OHOS_BUILD_ENABLE_INTENTION_FRAMEWORK
#endif // OHOS_BUILD_ENABLE_COORDINATION

int32_t DeviceStatusService::AddHotAreaListener()
{
    CALL_DEBUG_ENTER;
#ifdef OHOS_BUILD_ENABLE_COORDINATION
#ifndef OHOS_BUILD_ENABLE_INTENTION_FRAMEWORK
    int32_t pid = GetCallingPid();
    int32_t ret = delegateTasks_.PostSyncTask([this, pid] {
        return this->OnAddHotAreaListener(pid);
    });
    if (ret != RET_OK) {
        FI_HILOGE("Failed to add hot area listener, ret:%{public}d", ret);
        return RET_ERR;
    }
#endif // OHOS_BUILD_ENABLE_INTENTION_FRAMEWORK
#endif // OHOS_BUILD_ENABLE_COORDINATION
    return RET_OK;
}

int32_t DeviceStatusService::RemoveHotAreaListener()
{
    CALL_DEBUG_ENTER;
#ifdef OHOS_BUILD_ENABLE_COORDINATION
#ifndef OHOS_BUILD_ENABLE_INTENTION_FRAMEWORK
    int32_t pid = GetCallingPid();
    int32_t ret = delegateTasks_.PostSyncTask([this, pid] {
        return this->OnRemoveHotAreaListener(pid);
    });
    if (ret != RET_OK) {
        FI_HILOGE("Failed to remove hot area listener, ret:%{public}d", ret);
        return RET_ERR;
    }
#endif // OHOS_BUILD_ENABLE_INTENTION_FRAMEWORK
#endif // OHOS_BUILD_ENABLE_COORDINATION
    return RET_OK;
}

int32_t DeviceStatusService::UpdatePreviewStyle(const PreviewStyle &previewStyle)
{
    CALL_DEBUG_ENTER;
    int32_t ret = delegateTasks_.PostSyncTask([this, previewStyle] {
        return this->dragMgr_.UpdatePreviewStyle(previewStyle);
    });
    if (ret != RET_OK) {
        FI_HILOGE("UpdatePreviewStyle failed, ret:%{public}d", ret);
    }
    return ret;
}

int32_t DeviceStatusService::UpdatePreviewStyleWithAnimation(const PreviewStyle &previewStyle,
    const PreviewAnimation &animation)
{
    CALL_DEBUG_ENTER;
    int32_t ret = delegateTasks_.PostSyncTask([this, previewStyle, animation] {
        return this->dragMgr_.UpdatePreviewStyleWithAnimation(previewStyle, animation);
    });
    if (ret != RET_OK) {
        FI_HILOGE("UpdatePreviewStyleWithAnimation failed, ret:%{public}d", ret);
    }
    return ret;
}

int32_t DeviceStatusService::GetDragSummary(std::map<std::string, int64_t> &summarys)
{
    int32_t ret = delegateTasks_.PostSyncTask([this, &summarys] {
        return this->dragMgr_.GetDragSummary(summarys);
    });
    if (ret != RET_OK) {
        FI_HILOGE("Failed to get drag summarys, ret:%{public}d", ret);
        return RET_ERR;
    }
    return RET_OK;
}

int32_t DeviceStatusService::AddPrivilege()
{
    CALL_DEBUG_ENTER;
    int32_t tokenId = static_cast<int32_t>(GetCallingTokenID());
    int32_t ret = delegateTasks_.PostSyncTask([this, tokenId] {
        return this->dragMgr_.AddPrivilege(tokenId);
    });
    if (ret != RET_OK) {
        FI_HILOGE("Failed to add privilege, ret:%{public}d", ret);
        return RET_ERR;
    }
    return RET_OK;
}

int32_t DeviceStatusService::EraseMouseIcon()
{
    CALL_DEBUG_ENTER;
    int32_t ret = delegateTasks_.PostSyncTask([this] {
        return this->dragMgr_.EraseMouseIcon();
    });
    if (ret != RET_OK) {
        FI_HILOGE("Failed to erase mouse, ret:%{public}d", ret);
    }
    return ret;
}
} // namespace DeviceStatus
} // namespace Msdp
} // namespace OHOS
