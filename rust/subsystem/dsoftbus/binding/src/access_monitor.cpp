#include "access_monitor.h"
#include "accesstoken_kit.h"
#include "nativetoken_kit.h"
#include "token_setproc.h"

using namespace OHOS::Security::AccessToken;
using OHOS::Security::AccessToken::AccessTokenID;

void SetAceessTokenPermission(const char* processName,
    const char** perms, const int32_t permCount)
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
        .processName = processName,
        .aplStr = "system_basic",
    };
    tokenId = GetAccessTokenId(&infoInstance);
    SetSelfTokenID(tokenId);
    OHOS::Security::AccessToken::AccessTokenKit::ReloadNativeTokenInfo();
}

void GetAccessToken()
{
    const char** perms = new const char *[1];
    perms[0] = "ohos.permission.DISTRIBUTED_DATASYNC";
    const char* name = "rust_main";
    SetAceessTokenPermission(name, perms, 1);
}



