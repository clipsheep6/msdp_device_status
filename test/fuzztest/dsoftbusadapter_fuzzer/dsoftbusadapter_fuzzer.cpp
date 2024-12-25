/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "dsoftbusadapter_fuzzer.h"

#include "accesstoken_kit.h"
#include "nativetoken_kit.h"
#include "singleton.h"
#include "token_setproc.h"

#include "ddm_adapter.h"
#include "devicestatus_define.h"
#include "dsoftbus_adapter_impl.h"
#include "socket_session_manager.h"

#include "message_parcel.h"

#undef LOG_TAG
#define LOG_TAG "DsoftbusAdapterFuzzTest"
namespace OHOS {
namespace Msdp {
namespace DeviceStatus {
#define SERVER_SESSION_NAME "ohos.msdp.device_status.intention.serversession"
uint64_t g_tokenID { 0 };
const std::string SYSTEM_CORE { "system_core" };
const char* g_cores[] = { "ohos.permission.INPUT_MONITORING" };
const uint8_t *g_baseFuzzData = nullptr;
size_t g_baseFuzzSize = 0;
size_t g_baseFuzzPos = 0;
constexpr size_t STR_LEN = 255;
constexpr size_t PKG_NAME_SIZE_MAX { 65 };
constexpr size_t DEVICE_NAME_SIZE_MAX { 256 };

template <class T> T GetData()
{
    T objetct{};
    size_t objetctSize = sizeof(objetct);
    if (g_baseFuzzData == nullptr || objetctSize > g_baseFuzzSize - g_baseFuzzPos) {
        return objetct;
    }
    errno_t ret = memcpy_s(&objetct, objetctSize, g_baseFuzzData + g_baseFuzzPos, objetctSize);
    if (ret != EOK) {
        return {};
    }
    g_baseFuzzPos += objetctSize;
    return objetct;
}

void SetGlobalFuzzData(const uint8_t *data, size_t size)
{
    g_baseFuzzData = data;
    g_baseFuzzSize = size;
    g_baseFuzzPos = 0;
}

std::string GetStringFromData(int strlen)
{
    if (strlen < 1) {
        return "";
    }

    char cstr[strlen];
    cstr[strlen - 1] = '\0';
    for (int i = 0; i < strlen - 1; i++) {
        cstr[i] = GetData<char>();
    }
    std::string str(cstr);
    return str;
}

class DSoftbusObserver final : public IDSoftbusObserver {
public:
    DSoftbusObserver() = default;
    ~DSoftbusObserver() = default;

    void OnBind(const std::string &networkId) {}
    void OnShutdown(const std::string &networkId) {}
    void OnConnected(const std::string &networkId) {}
    bool OnPacket(const std::string &networkId, NetPacket &packet)
    {
        return true;
    }
    bool OnRawData(const std::string &networkId, const void *data, uint32_t dataLen)
    {
        return true;
    }
};

void SetPermission(const std::string &level, const char** perms, size_t permAmount)
{
    CALL_DEBUG_ENTER;
    if (perms == nullptr || permAmount == 0) {
        FI_HILOGE("The perms is empty");
        return;
    }

    NativeTokenInfoParams infoInstance = {
        .dcapsNum = 0,
        .permsNum = permAmount,
        .aclsNum = 0,
        .dcaps = nullptr,
        .perms = perms,
        .acls = nullptr,
        .processName = "DDMAdapterTest",
        .aplStr = level.c_str(),
    };
    g_tokenID = GetAccessTokenId(&infoInstance);
    SetSelfTokenID(g_tokenID);
    OHOS::Security::AccessToken::AccessTokenKit::AccessTokenKit::ReloadNativeTokenInfo();
}

void RemovePermission()
{
    CALL_DEBUG_ENTER;
    int32_t ret = OHOS::Security::AccessToken::AccessTokenKit::DeleteToken(g_tokenID);
    if (ret != RET_OK) {
        FI_HILOGE("Failed to remove permission");
        return;
    }
}

bool EnableFuzzTest(const uint8_t* data, size_t size)
{
    if ((data == nullptr) || (size < 1)) {
        return false;
    }

    DSoftbusAdapterImpl::GetInstance()->Enable();
    DSoftbusAdapterImpl::GetInstance()->SetupServer();
    DSoftbusAdapterImpl::GetInstance()->ShutdownServer();
    DSoftbusAdapterImpl::GetInstance()->CloseAllSessions();
    DSoftbusAdapterImpl::GetInstance()->CloseAllSessionsLocked();
    DSoftbusAdapterImpl::GetInstance()->Disable();
    return true;
}

bool AddObserverFuzzTest(const uint8_t* data, size_t size)
{
    if ((data == nullptr) || (size < 1)) {
        return false;
    }

    std::shared_ptr<IDSoftbusObserver> observer = std::make_shared<DSoftbusObserver>();
    DSoftbusAdapterImpl::GetInstance()->AddObserver(observer);
    DSoftbusAdapterImpl::GetInstance()->RemoveObserver(observer);
    return true;
}

bool CheckDeviceOnlineFuzzTest(const uint8_t* data, size_t size)
{
    if ((data == nullptr) || (size < 1)) {
        return false;
    }

    std::string networkId = GetStringFromData(STR_LEN);
    CircleStreamBuffer circleBuffer;

    DSoftbusAdapterImpl::GetInstance()->CheckDeviceOnline(networkId);
    DSoftbusAdapterImpl::GetInstance()->CloseSession(networkId);
    DSoftbusAdapterImpl::GetInstance()->HandleSessionData(networkId, circleBuffer);
    DSoftbusAdapterImpl::GetInstance()->OpenSessionLocked(networkId);
    DSoftbusAdapterImpl::GetInstance()->OnConnectedLocked(networkId);
    return true;
}

bool OpenSessionFuzzTest(const uint8_t* data, size_t size)
{
    if ((data == nullptr) || (size < 1)) {
        return false;
    }

    std::string networkId = GetStringFromData(STR_LEN);
    DSoftbusAdapterImpl::GetInstance()->OpenSession(networkId);
    DSoftbusAdapterImpl::GetInstance()->FindConnection(networkId);
    DSoftbusAdapterImpl::GetInstance()->CloseSession(networkId);
    DSoftbusAdapterImpl::GetInstance()->CloseAllSessions();
    return true;
}


bool SendPacketFuzzTest(const uint8_t* data, size_t size)
{
    if ((data == nullptr) || (size < 1)) {
        return false;
    }

    Parcel parcel;
    NetPacket packet(MessageId::DSOFTBUS_START_COOPERATE);
    std::string networkId = GetStringFromData(STR_LEN);
    DSoftbusAdapterImpl::GetInstance()->SendPacket(networkId, packet);
    DSoftbusAdapterImpl::GetInstance()->SendParcel(networkId, parcel);
    DSoftbusAdapterImpl::GetInstance()->BroadcastPacket(packet);
    DSoftbusAdapterImpl::GetInstance()->HandlePacket(networkId, packet);
    return true;
}

bool InitSocketFuzzTest(const uint8_t* data, size_t size)
{
    if ((data == nullptr) || (size < 1)) {
        return false;
    }

    int32_t socket = GetData<int32_t>();
    uint32_t dataLen = GetData<uint32_t>();
    std::string networkId = GetStringFromData(STR_LEN);
    int32_t *g_data = new int32_t(socket);

    char name[DEVICE_NAME_SIZE_MAX] { SERVER_SESSION_NAME };
    char pkgName[PKG_NAME_SIZE_MAX] { FI_PKG_NAME };
    SocketInfo info {
        .name = name,
        .pkgName = pkgName,
        .dataType = DATA_TYPE_BYTES
    };

    DSoftbusAdapterImpl::GetInstance()->InitSocket(info, socket, socket);
    DSoftbusAdapterImpl::GetInstance()->ConfigTcpAlive(socket);
    DSoftbusAdapterImpl::GetInstance()->OnShutdown(socket, SHUTDOWN_REASON_UNKNOWN);
    DSoftbusAdapterImpl::GetInstance()->OnBytes(socket, g_data, dataLen);
    DSoftbusAdapterImpl::GetInstance()->HandleRawData(networkId, g_data, dataLen);
    return true;
}

bool DDMAdapterFuzzTest(const uint8_t* data, size_t size)
{
    if ((data == nullptr) || (size < 1)) {
        return false;
    }

    SetPermission(SYSTEM_CORE, g_cores, sizeof(g_cores) / sizeof(g_cores[0]));
    DDMAdapter ddmAdapter;
    ddmAdapter.Enable();
    std::string networkId = GetStringFromData(STR_LEN);
    ddmAdapter.CheckSameAccountToLocal(networkId);
    ddmAdapter.Disable();
    RemovePermission();
    return true;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    /* Run your code on data */
    if (data == nullptr) {
        return 0;
    }
    SetGlobalFuzzData(data, size);

    OHOS::Msdp::DeviceStatus::EnableFuzzTest(data, size);
    OHOS::Msdp::DeviceStatus::AddObserverFuzzTest(data, size);
    OHOS::Msdp::DeviceStatus::CheckDeviceOnlineFuzzTest(data, size);
    OHOS::Msdp::DeviceStatus::OpenSessionFuzzTest(data, size);
    OHOS::Msdp::DeviceStatus::SendPacketFuzzTest(data, size);
    OHOS::Msdp::DeviceStatus::InitSocketFuzzTest(data, size);
    OHOS::Msdp::DeviceStatus::DDMAdapterFuzzTest(data, size);
    return 0;
}
} // namespace DeviceStatus
} // namespace Msdp
} // namespace OHOS