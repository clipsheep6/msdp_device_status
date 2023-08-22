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

#include "fusion_security.h"

#include <cstdlib>
#include <cstring>
#include <iostream>

#include "accesstoken_kit.h"
#include "nativetoken_kit.h"
#include "nocopyable.h"
#include "softbus_bus_center.h"
#include "token_setproc.h"

#include "devicestatus_define.h"

using namespace ::OHOS;
using namespace ::OHOS::Security::AccessToken;

namespace {
constexpr HiviewDFX::HiLogLabel LABEL { LOG_CORE, Msdp::MSDP_DOMAIN_ID, "FusionSecurity" };
} // namespace

static void SetAceessTokenPermission(const std::string &processName, const char** perms, size_t permCount)
{
    if (perms == nullptr || permCount == 0) {
        return;
    }
    uint64_t tokenId;
    NativeTokenInfoParams infoInstance = {
        .dcapsNum = 0,
        .permsNum = permCount,
        .aclsNum = 0,
        .dcaps = nullptr,
        .perms = perms,
        .acls = nullptr,
        .processName = processName.c_str(),
        .aplStr = "system_basic",
    };
    tokenId = GetAccessTokenId(&infoInstance);
    SetSelfTokenID(tokenId);
    OHOS::Security::AccessToken::AccessTokenKit::ReloadNativeTokenInfo();
}

void GetAccessToken()
{
    const char* perms[] {
        "ohos.permission.CAPTURE_SCREEN",
        "ohos.permission.DISTRIBUTED_DATASYNC",
    };
    SetAceessTokenPermission("fusion_device_profile_test", perms, 2);
}

struct CString : public CIString {
    std::string str;

    CString(const char *s);
    DISALLOW_MOVE(CString);
    CString(const CString &other);
    ~CString() = default;
    CString& operator=(const CString &other) = delete;

    static CIString* Clone(CIString *target);
    static void Destruct(CIString *target);
    static const char* GetData(CIString *target);
};

CString::CString(const char *s)
    : str(s != nullptr ? s : std::string())
{
    clone = &CString::Clone;
    destruct = &CString::Destruct;
    data = &CString::GetData;
}

CString::CString(const CString &other)
    : str(other.str)
{
    clone = &CString::Clone;
    destruct = &CString::Destruct;
    data = &CString::GetData;
}

CIString* CString::Clone(CIString *target)
{
    CString *t = static_cast<CString *>(target);
    CHKPP(t);
    return new CString(*t);
}

void CString::Destruct(CIString *target)
{
    CString *t = static_cast<CString *>(target);
    CHKPV(t);
    delete t;
}

const char* CString::GetData(CIString *target)
{
    CString *t = static_cast<CString *>(target);
    CHKPP(t);
    return t->str.c_str();
}

CIString* GetLocalNetworkId()
{
    CALL_DEBUG_ENTER;
    NodeBasicInfo node;
    int32_t ret = GetLocalNodeDeviceInfo(FI_PKG_NAME, &node);
    if (ret != RET_OK) {
        return nullptr;
    }
    return new CString(node.networkId);
}
