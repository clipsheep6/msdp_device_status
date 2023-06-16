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

#include "coordination_softbus_adapter.h"

#include <chrono>
#include <thread>

#include "softbus_bus_center.h"
#include "softbus_common.h"

#include "coordination_sm.h"
#include "coordination_util.h"
#include "device_coordination_softbus_define.h"
#include "devicestatus_define.h"

namespace OHOS {
namespace Msdp {
namespace DeviceStatus {
namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL { LOG_CORE, MSDP_DOMAIN_ID, "CoordinationSoftbusAdapter" };
std::shared_ptr<CoordinationSoftbusAdapter> g_instance = nullptr;
constexpr int32_t DINPUT_LINK_TYPE_MAX { 4 };
const SessionAttribute g_sessionAttr = {
    .dataType = SessionType::TYPE_BYTES,
    .linkTypeNum = DINPUT_LINK_TYPE_MAX,
    .linkType = {
        LINK_TYPE_WIFI_WLAN_2G,
        LINK_TYPE_WIFI_WLAN_5G,
        LINK_TYPE_WIFI_P2P,
        LINK_TYPE_BR
    }
};

void ResponseStartRemoteCoordination(int32_t sessionId, const JsonParser& parser)
{
    CALL_DEBUG_ENTER;
    cJSON* deviceId = cJSON_GetObjectItemCaseSensitive(parser.json_, FI_SOFTBUS_KEY_LOCAL_DEVICE_ID);
    cJSON* buttonIsPressed = cJSON_GetObjectItemCaseSensitive(parser.json_, FI_SOFTBUS_POINTER_BUTTON_IS_PRESS);
    if (!cJSON_IsString(deviceId) || !cJSON_IsBool(buttonIsPressed)) {
        FI_HILOGE("OnBytesReceived cmdType is TRANS_SINK_MSG_ONPREPARE, data type is error");
        return;
    }
    COOR_SM->StartRemoteCoordination(deviceId->valuestring, cJSON_IsTrue(buttonIsPressed));
}

void ResponseStartRemoteCoordinationResult(int32_t sessionId, const JsonParser& parser)
{
    CALL_DEBUG_ENTER;
    cJSON* result = cJSON_GetObjectItemCaseSensitive(parser.json_, FI_SOFTBUS_KEY_RESULT);
    cJSON* dhid = cJSON_GetObjectItemCaseSensitive(parser.json_, FI_SOFTBUS_KEY_START_DHID);
    cJSON* x = cJSON_GetObjectItemCaseSensitive(parser.json_, FI_SOFTBUS_KEY_POINTER_X);
    cJSON* y = cJSON_GetObjectItemCaseSensitive(parser.json_, FI_SOFTBUS_KEY_POINTER_Y);
    if (!cJSON_IsBool(result) || !cJSON_IsString(dhid) || !cJSON_IsNumber(x) || !cJSON_IsNumber(y)) {
        FI_HILOGE("OnBytesReceived cmdType is TRANS_SINK_MSG_ONPREPARE, data type is error");
        return;
    }
    COOR_SM->StartRemoteCoordinationResult(cJSON_IsTrue(result), dhid->valuestring, x->valueint, y->valueint);
}

void ResponseStopRemoteCoordination(int32_t sessionId, const JsonParser& parser)
{
    CALL_DEBUG_ENTER;
    cJSON* result = cJSON_GetObjectItemCaseSensitive(parser.json_, FI_SOFTBUS_KEY_RESULT);

    if (!cJSON_IsBool(result)) {
        FI_HILOGE("OnBytesReceived cmdType is TRANS_SINK_MSG_ONPREPARE, data type is error");
        return;
    }
    COOR_SM->StopRemoteCoordination(cJSON_IsTrue(result));
}

void ResponseStopRemoteCoordinationResult(int32_t sessionId, const JsonParser& parser)
{
    CALL_DEBUG_ENTER;
    cJSON* result = cJSON_GetObjectItemCaseSensitive(parser.json_, FI_SOFTBUS_KEY_RESULT);

    if (!cJSON_IsBool(result)) {
        FI_HILOGE("OnBytesReceived cmdType is TRANS_SINK_MSG_ONPREPARE, data type is error");
        return;
    }
    COOR_SM->StopRemoteCoordinationResult(cJSON_IsTrue(result));
}

void ResponseStartCoordinationOtherResult(int32_t sessionId, const JsonParser& parser)
{
    CALL_DEBUG_ENTER;
    cJSON* deviceId = cJSON_GetObjectItemCaseSensitive(parser.json_, FI_SOFTBUS_KEY_OTHER_DEVICE_ID);

    if (!cJSON_IsString(deviceId)) {
        FI_HILOGE("OnBytesReceived cmdType is TRANS_SINK_MSG_ONPREPARE, data type is error");
        return;
    }
    COOR_SM->StartCoordinationOtherResult(deviceId->valuestring);
}
} // namespace

static int32_t SessionOpened(int32_t sessionId, int32_t result)
{
    return COOR_SOFTBUS_ADAPTER->OnSessionOpened(sessionId, result);
}

static void SessionClosed(int32_t sessionId)
{
    COOR_SOFTBUS_ADAPTER->OnSessionClosed(sessionId);
}

static void BytesReceived(int32_t sessionId, const void *data, uint32_t dataLen)
{
    COOR_SOFTBUS_ADAPTER->OnBytesReceived(sessionId, data, dataLen);
}

static void MessageReceived(int32_t sessionId, const void *data, uint32_t dataLen)
{
    (void)sessionId;
    (void)data;
    (void)dataLen;
}

static void StreamReceived(int32_t sessionId, const StreamData *data, const StreamData *ext,
    const StreamFrameInfo *param)
{
    (void)sessionId;
    (void)data;
    (void)ext;
    (void)param;
}

int32_t CoordinationSoftbusAdapter::Init()
{
    CALL_INFO_TRACE;
    const std::string SESSION_NAME = "ohos.msdp.device_status.";
    sessListener_ = {
        .OnSessionOpened = SessionOpened,
        .OnSessionClosed = SessionClosed,
        .OnBytesReceived = BytesReceived,
        .OnMessageReceived = MessageReceived,
        .OnStreamReceived = StreamReceived
    };
    std::string localNetworkId = COORDINATION::GetLocalNetworkId();
    if (localNetworkId.empty()) {
        FI_HILOGE("Local networkid is empty");
        return RET_ERR;
    }
    localSessionName_ = SESSION_NAME + localNetworkId.substr(0, INTERCEPT_STRING_LENGTH);
    int32_t ret = CreateSessionServer(FI_PKG_NAME, localSessionName_.c_str(), &sessListener_);
    if (ret != RET_OK) {
        FI_HILOGE("Create session server failed, error code:%{public}d", ret);
        return RET_ERR;
    }
    return RET_OK;
}

CoordinationSoftbusAdapter::~CoordinationSoftbusAdapter()
{
    Release();
}

void CoordinationSoftbusAdapter::Release()
{
    CALL_INFO_TRACE;
    std::unique_lock<std::mutex> sessionLock(operationMutex_);
    std::for_each(sessionDevMap_.begin(), sessionDevMap_.end(), [](auto item) {
        CloseSession(item.second);
        FI_HILOGD("Close session success");
    });
    int32_t ret = RemoveSessionServer(FI_PKG_NAME, localSessionName_.c_str());
    FI_HILOGD("RemoveSessionServer ret:%{public}d", ret);
    sessionDevMap_.clear();
    channelStatusMap_.clear();
}

bool CoordinationSoftbusAdapter::CheckDeviceSessionState(const std::string &remoteNetworkId)
{
    std::unique_lock<std::mutex> sessionLock(operationMutex_);
    if (sessionDevMap_.find(remoteNetworkId) == sessionDevMap_.end()) {
        FI_HILOGE("Check session state error");
        return false;
    }
    return true;
}

int32_t CoordinationSoftbusAdapter::OpenInputSoftbus(const std::string &remoteNetworkId)
{
    CALL_INFO_TRACE;
    const std::string SESSION_NAME = "ohos.msdp.device_status.";
    const std::string GROUP_ID = "fi_softbus_group_id";
    if (CheckDeviceSessionState(remoteNetworkId)) {
        FI_HILOGD("Softbus session has already  opened");
        return RET_OK;
    }

    int32_t ret = Init();
    if (ret != RET_OK) {
        FI_HILOGE("Init failed");
        return RET_ERR;
    }

    std::string peerSessionName = SESSION_NAME + remoteNetworkId.substr(0, INTERCEPT_STRING_LENGTH);
    int32_t sessionId = OpenSession(localSessionName_.c_str(), peerSessionName.c_str(), remoteNetworkId.c_str(),
        GROUP_ID.c_str(), &g_sessionAttr);
    if (sessionId < 0) {
        FI_HILOGE("OpenSession failed");
        return RET_ERR;
    }
    return WaitSessionOpend(remoteNetworkId, sessionId);
}

int32_t CoordinationSoftbusAdapter::WaitSessionOpend(const std::string &remoteNetworkId, int32_t sessionId)
{
    CALL_INFO_TRACE;
    std::unique_lock<std::mutex> waitLock(operationMutex_);
    sessionDevMap_[remoteNetworkId] = sessionId;
    auto status = openSessionWaitCond_.wait_for(waitLock, std::chrono::seconds(SESSION_WAIT_TIMEOUT_SECOND),
        [this, remoteNetworkId] () { return channelStatusMap_[remoteNetworkId]; });
    if (!status) {
        FI_HILOGE("OpenSession timeout");
        return RET_ERR;
    }
    channelStatusMap_[remoteNetworkId] = false;
    return RET_OK;
}

void CoordinationSoftbusAdapter::CloseInputSoftbus(const std::string &remoteNetworkId)
{
    CALL_INFO_TRACE;
    std::unique_lock<std::mutex> sessionLock(operationMutex_);
    if (sessionDevMap_.find(remoteNetworkId) == sessionDevMap_.end()) {
        FI_HILOGI("SessionDevIdMap not found");
        return;
    }
    int32_t sessionId = sessionDevMap_[remoteNetworkId];

    CloseSession(sessionId);
    sessionDevMap_.erase(remoteNetworkId);
    channelStatusMap_.erase(remoteNetworkId);
}

std::shared_ptr<CoordinationSoftbusAdapter> CoordinationSoftbusAdapter::GetInstance()
{
    static std::once_flag flag;
    std::call_once(flag, [&]() {
        g_instance.reset(new (std::nothrow) CoordinationSoftbusAdapter());
    });
    return g_instance;
}

int32_t CoordinationSoftbusAdapter::StartRemoteCoordination(const std::string &localNetworkId,
    const std::string &remoteNetworkId)
{
    CALL_DEBUG_ENTER;
    std::unique_lock<std::mutex> sessionLock(operationMutex_);
    if (sessionDevMap_.find(remoteNetworkId) == sessionDevMap_.end()) {
        FI_HILOGE("Start remote coordination error, not found this device");
        return RET_ERR;
    }
    int32_t sessionId = sessionDevMap_[remoteNetworkId];
    auto pointerEvent = COOR_SM->GetLastPointerEvent();
    CHKPR(pointerEvent, RET_ERR);
    bool isPointerButtonPressed = pointerEvent->GetPointerAction() == MMI::PointerEvent::POINTER_ACTION_BUTTON_DOWN;
    cJSON *jsonStr = cJSON_CreateObject();
    cJSON_AddItemToObject(jsonStr, FI_SOFTBUS_KEY_CMD_TYPE, cJSON_CreateNumber(REMOTE_COORDINATION_START));
    cJSON_AddItemToObject(jsonStr, FI_SOFTBUS_KEY_LOCAL_DEVICE_ID, cJSON_CreateString(localNetworkId.c_str()));
    cJSON_AddItemToObject(jsonStr, FI_SOFTBUS_KEY_SESSION_ID, cJSON_CreateNumber(sessionId));
    cJSON_AddItemToObject(jsonStr, FI_SOFTBUS_POINTER_BUTTON_IS_PRESS, cJSON_CreateBool(isPointerButtonPressed));
    char *smsg = cJSON_Print(jsonStr);
    cJSON_Delete(jsonStr);
    int32_t ret = SendMsg(sessionId, smsg);
    cJSON_free(smsg);
    if (ret != RET_OK) {
        FI_HILOGE("Start remote coordination send session msg failed, ret:%{public}d", ret);
        return RET_ERR;
    }
    return RET_OK;
}

int32_t CoordinationSoftbusAdapter::StartRemoteCoordinationResult(const std::string &remoteNetworkId,
    bool isSuccess, const std::string &startDeviceDhid, int32_t xPercent, int32_t yPercent)
{
    CALL_DEBUG_ENTER;
    std::unique_lock<std::mutex> sessionLock(operationMutex_);
    if (sessionDevMap_.find(remoteNetworkId) == sessionDevMap_.end()) {
        FI_HILOGE("Stop remote coordination error, not found this device");
        return RET_ERR;
    }
    int32_t sessionId = sessionDevMap_[remoteNetworkId];
    cJSON *jsonStr = cJSON_CreateObject();
    cJSON_AddItemToObject(jsonStr, FI_SOFTBUS_KEY_CMD_TYPE, cJSON_CreateNumber(REMOTE_COORDINATION_START_RES));
    cJSON_AddItemToObject(jsonStr, FI_SOFTBUS_KEY_RESULT, cJSON_CreateBool(isSuccess));
    cJSON_AddItemToObject(jsonStr, FI_SOFTBUS_KEY_START_DHID, cJSON_CreateString(startDeviceDhid.c_str()));
    cJSON_AddItemToObject(jsonStr, FI_SOFTBUS_KEY_POINTER_X, cJSON_CreateNumber(xPercent));
    cJSON_AddItemToObject(jsonStr, FI_SOFTBUS_KEY_POINTER_Y, cJSON_CreateNumber(yPercent));
    cJSON_AddItemToObject(jsonStr, FI_SOFTBUS_KEY_SESSION_ID, cJSON_CreateNumber(sessionId));
    char *smsg = cJSON_Print(jsonStr);
    cJSON_Delete(jsonStr);
    int32_t ret = SendMsg(sessionId, smsg);
    cJSON_free(smsg);
    if (ret != RET_OK) {
        FI_HILOGE("Start remote coordination result send session msg failed");
        return RET_ERR;
    }
    return RET_OK;
}

int32_t CoordinationSoftbusAdapter::StopRemoteCoordination(const std::string &remoteNetworkId, bool isUnchained)
{
    CALL_DEBUG_ENTER;
    std::unique_lock<std::mutex> sessionLock(operationMutex_);
    if (sessionDevMap_.find(remoteNetworkId) == sessionDevMap_.end()) {
        FI_HILOGE("Stop remote coordination error, not found this device");
        return RET_ERR;
    }
    int32_t sessionId = sessionDevMap_[remoteNetworkId];
    cJSON *jsonStr = cJSON_CreateObject();
    cJSON_AddItemToObject(jsonStr, FI_SOFTBUS_KEY_CMD_TYPE, cJSON_CreateNumber(REMOTE_COORDINATION_STOP));
    cJSON_AddItemToObject(jsonStr, FI_SOFTBUS_KEY_RESULT, cJSON_CreateBool(isUnchained));
    cJSON_AddItemToObject(jsonStr, FI_SOFTBUS_KEY_SESSION_ID, cJSON_CreateNumber(sessionId));
    char *smsg = cJSON_Print(jsonStr);
    cJSON_Delete(jsonStr);
    int32_t ret = SendMsg(sessionId, smsg);
    cJSON_free(smsg);
    if (ret != RET_OK) {
        FI_HILOGE("Stop remote coordination send session msg failed");
        return RET_ERR;
    }
    return RET_OK;
}

int32_t CoordinationSoftbusAdapter::StopRemoteCoordinationResult(const std::string &remoteNetworkId,
    bool isSuccess)
{
    CALL_DEBUG_ENTER;
    std::unique_lock<std::mutex> sessionLock(operationMutex_);
    if (sessionDevMap_.find(remoteNetworkId) == sessionDevMap_.end()) {
        FI_HILOGE("Stop remote coordination result error, not found this device");
        return RET_ERR;
    }
    int32_t sessionId = sessionDevMap_[remoteNetworkId];
    cJSON *jsonStr = cJSON_CreateObject();
    cJSON_AddItemToObject(jsonStr, FI_SOFTBUS_KEY_CMD_TYPE, cJSON_CreateNumber(REMOTE_COORDINATION_STOP_RES));
    cJSON_AddItemToObject(jsonStr, FI_SOFTBUS_KEY_RESULT, cJSON_CreateBool(isSuccess));
    cJSON_AddItemToObject(jsonStr, FI_SOFTBUS_KEY_SESSION_ID, cJSON_CreateNumber(sessionId));
    char *smsg = cJSON_Print(jsonStr);
    cJSON_Delete(jsonStr);
    int32_t ret = SendMsg(sessionId, smsg);
    cJSON_free(smsg);
    if (ret != RET_OK) {
        FI_HILOGE("Stop remote coordination result send session msg failed");
        return RET_ERR;
    }
    return RET_OK;
}

int32_t CoordinationSoftbusAdapter::StartCoordinationOtherResult(const std::string &originNetworkId,
    const std::string &remoteNetworkId)
{
    CALL_DEBUG_ENTER;
    std::unique_lock<std::mutex> sessionLock(operationMutex_);
    if (sessionDevMap_.find(originNetworkId) == sessionDevMap_.end()) {
        FI_HILOGE("Start coordination other result error, not found this device");
        return RET_ERR;
    }
    int32_t sessionId = sessionDevMap_[originNetworkId];
    cJSON *jsonStr = cJSON_CreateObject();
    cJSON_AddItemToObject(jsonStr, FI_SOFTBUS_KEY_CMD_TYPE, cJSON_CreateNumber(REMOTE_COORDINATION_STOP_OTHER_RES));
    cJSON_AddItemToObject(jsonStr, FI_SOFTBUS_KEY_OTHER_DEVICE_ID, cJSON_CreateString(remoteNetworkId.c_str()));
    cJSON_AddItemToObject(jsonStr, FI_SOFTBUS_KEY_SESSION_ID, cJSON_CreateNumber(sessionId));
    char *smsg = cJSON_Print(jsonStr);
    cJSON_Delete(jsonStr);
    int32_t ret = SendMsg(sessionId, smsg);
    cJSON_free(smsg);
    if (ret != RET_OK) {
        FI_HILOGE("Start coordination other result send session msg failed");
        return RET_ERR;
    }
    return RET_OK;
}

void CoordinationSoftbusAdapter::HandleSessionData(int32_t sessionId, const std::string& message)
{
    if (message.empty()) {
        FI_HILOGE("Message is empty");
        return;
    }
    JsonParser parser;
    parser.json_ = cJSON_Parse(message.c_str());
    if (!cJSON_IsObject(parser.json_)) {
        FI_HILOGI("Parser json is not object");
        if (message.size() < sizeof(DataPacket)) {
            FI_HILOGE("Data packet is incomplete");
            return;
        }
        const DataPacket* dataPacket = reinterpret_cast<const DataPacket*>(message.c_str());
        if ((message.size() - sizeof(DataPacket)) < dataPacket->dataLen) {
            FI_HILOGE("Data is corrupt");
            return;
        }
        if (registerRecvMap_.find(dataPacket->messageId) == registerRecvMap_.end()) {
            FI_HILOGW("Message:%{public}d does not register", dataPacket->messageId);
            return;
        }
        FI_HILOGI("Message:%{public}d", dataPacket->messageId);
        if (dataPacket->messageId == DRAGGING_DATA || dataPacket->messageId == STOPDRAG_DATA) {
            CHKPV(registerRecvMap_[dataPacket->messageId]);
            registerRecvMap_[dataPacket->messageId](dataPacket->data, dataPacket->dataLen);
        }
        return;
    }
    cJSON* comType = cJSON_GetObjectItemCaseSensitive(parser.json_, FI_SOFTBUS_KEY_CMD_TYPE);
    if (!cJSON_IsNumber(comType)) {
        FI_HILOGE("OnBytesReceived cmdType is not number type");
        return;
    }
    FI_HILOGD("valueint:%{public}d", comType->valueint);
    switch (comType->valueint) {
        case REMOTE_COORDINATION_START: {
            ResponseStartRemoteCoordination(sessionId, parser);
            break;
        }
        case REMOTE_COORDINATION_START_RES: {
            ResponseStartRemoteCoordinationResult(sessionId, parser);
            break;
        }
        case REMOTE_COORDINATION_STOP: {
            ResponseStopRemoteCoordination(sessionId, parser);
            break;
        }
        case REMOTE_COORDINATION_STOP_RES: {
            ResponseStopRemoteCoordinationResult(sessionId, parser);
            break;
        }
        case REMOTE_COORDINATION_STOP_OTHER_RES: {
            ResponseStartCoordinationOtherResult(sessionId, parser);
            break;
        }
        default: {
            FI_HILOGE("OnBytesReceived cmdType is undefined");
            break;
        }
    }
}

void CoordinationSoftbusAdapter::OnBytesReceived(int32_t sessionId, const void *data, uint32_t dataLen)
{
    FI_HILOGD("dataLen:%{public}d", dataLen);
    if (sessionId < 0 || data == nullptr || dataLen <= 0) {
        FI_HILOGE("Param check failed");
        return;
    }
    std::string message = std::string(static_cast<const char *>(data), dataLen);
    HandleSessionData(sessionId, message);
}

int32_t CoordinationSoftbusAdapter::SendMsg(int32_t sessionId, const std::string &message)
{
    CALL_DEBUG_ENTER;
    if (message.size() > MSG_MAX_SIZE) {
        FI_HILOGW("error:message.size() > MSG_MAX_SIZE message size:%{public}zu", message.size());
        return RET_ERR;
    }
    return SendBytes(sessionId, message.c_str(), strlen(message.c_str()));
}

std::string CoordinationSoftbusAdapter::FindDevice(int32_t sessionId)
{
    std::unique_lock<std::mutex> sessionLock(operationMutex_);
    auto find_item = std::find_if(sessionDevMap_.begin(), sessionDevMap_.end(),
        [sessionId](const std::map<std::string, int32_t>::value_type item) {
        return item.second == sessionId;
    });
    if (find_item == sessionDevMap_.end()) {
        FI_HILOGE("FindDevice error");
        return {};
    }
    return find_item->first;
}

int32_t CoordinationSoftbusAdapter::OnSessionOpened(int32_t sessionId, int32_t result)
{
    CALL_INFO_TRACE;
    char peerDevId[DEVICE_ID_SIZE_MAX] = {};
    int32_t getPeerDeviceIdResult = GetPeerDeviceId(sessionId, peerDevId, sizeof(peerDevId));
    FI_HILOGD("Get peer device id ret:%{public}d", getPeerDeviceIdResult);
    if (result != RET_OK) {
        std::string deviceId = FindDevice(sessionId);
        FI_HILOGE("Session open failed result:%{public}d", result);
        std::unique_lock<std::mutex> sessionLock(operationMutex_);
        if (sessionDevMap_.find(deviceId) != sessionDevMap_.end()) {
            sessionDevMap_.erase(deviceId);
        }
        if (getPeerDeviceIdResult == RET_OK) {
            channelStatusMap_[peerDevId] = true;
        }
        openSessionWaitCond_.notify_all();
        return RET_OK;
    }

    int32_t sessionSide = GetSessionSide(sessionId);
    FI_HILOGI("session open succeed, sessionId:%{public}d, sessionSide:%{public}d(1 is client side)",
        sessionId, sessionSide);
    std::lock_guard<std::mutex> notifyLock(operationMutex_);
    if (sessionSide == SESSION_SIDE_SERVER) {
        if (getPeerDeviceIdResult == RET_OK) {
            sessionDevMap_[peerDevId] = sessionId;
        }
    } else {
        if (getPeerDeviceIdResult == RET_OK) {
            channelStatusMap_[peerDevId] = true;
        }
        openSessionWaitCond_.notify_all();
    }
    return RET_OK;
}

void CoordinationSoftbusAdapter::OnSessionClosed(int32_t sessionId)
{
    CALL_DEBUG_ENTER;
    std::string deviceId = FindDevice(sessionId);
    std::unique_lock<std::mutex> sessionLock(operationMutex_);
    if (sessionDevMap_.find(deviceId) != sessionDevMap_.end()) {
        sessionDevMap_.erase(deviceId);
    }
    if (GetSessionSide(sessionId) != 0) {
        channelStatusMap_.erase(deviceId);
    }
    COOR_SM->Reset(deviceId);
    COOR_SM->NotifySessionClosed();
}

void CoordinationSoftbusAdapter::RegisterRecvFunc(MessageId messageId, std::function<void(void*, uint32_t)> callback)
{
    CALL_DEBUG_ENTER;
    if (messageId <= MIN_ID || messageId >= MAX_ID) {
        FI_HILOGE("Message id is invalid:%{public}d", messageId);
        return;
    }
    CHKPV(callback);
    registerRecvMap_[messageId] = callback;
}

int32_t CoordinationSoftbusAdapter::SendData(const std::string& deviceId, MessageId messageId,
    void* data, uint32_t dataLen)
{
    CALL_DEBUG_ENTER;
    DataPacket* dataPacket = (DataPacket*)malloc(sizeof(DataPacket) + dataLen);
    CHKPR(dataPacket, RET_ERR);
    dataPacket->messageId = messageId;
    dataPacket->dataLen = dataLen;
    errno_t ret = memcpy_s(dataPacket->data, dataPacket->dataLen, data, dataPacket->dataLen);
    if (ret != EOK) {
        FI_HILOGE("Memcpy data packet failed");
        free(dataPacket);
        return RET_ERR;
    }
    int32_t result = SendBytes(sessionDevMap_[deviceId], dataPacket, sizeof(DataPacket) + dataLen);
    free(dataPacket);
    if (result != RET_OK) {
        FI_HILOGE("Send bytes failed");
        return RET_ERR;
    }
    return RET_OK;
}
} // namespace DeviceStatus
} // namespace Msdp
} // namespace OHOS
