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

#include <iostream>
#include <string>

#include "accesstoken_kit.h"
#include <gtest/gtest.h>
#include "nativetoken_kit.h"
#include "surface.h"
#include "token_setproc.h"

#include "across_ability_adapter.h"
#include "devicestatus_define.h"

namespace OHOS {
namespace Msdp {
namespace DeviceStatus {
using namespace testing::ext;
namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, MSDP_DOMAIN_ID, "TestAcrossByUT" };
} // namespace

class TestAcrossByUT : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
    void SetTokenID();
};

void TestAcrossByUT::SetUpTestCase()
{
    FI_HILOGD("SetUpTestCase");
}

void TestAcrossByUT::SetUp()
{
    FI_HILOGD("SetUp");
}

void TestAcrossByUT::TearDown()
{
    FI_HILOGD("TearDown");
}

void TestAcrossByUT::TearDownTestCase()
{
    FI_HILOGD("TearDownTestCase");
}

void TestAcrossByUT::SetTokenID()
{
    uint64_t tokenId;
    const char *perms[8];
    perms[0] = "ohos.permission.MANAGE_MISSIONS";
    perms[1] = "ohos.permission.MANAGER_ABILITY_FROM_GATEWAY";
    perms[2] = "ohos.permission.DISTRIBUTED_DATASYNC";
    perms[3] = "ohos.permission.ACCESS_SERVICE_DM";
    perms[4] = "ohos.permission.ACCESS_UDID";
    perms[5] = "ohos.permission.GET_BUNDLE_INFO_PRIVILEGED";
    perms[6] = "ohos.permission.MANAGER_ABILITY_FROM_GATEWAY";
    perms[7] = "ohos.permission.GET_BUNDLE_INFO";
    NativeTokenInfoParams infoInstance = {
        .dcapsNum = 0,
        .permsNum = 8,
        .aclsNum = 0,
        .dcaps = NULL,
        .perms = perms,
        .acls = NULL,
        .processName = "test_across_by_ut",
        .aplStr = "system_basic",
    };
    tokenId = GetAccessTokenId(&infoInstance);
    SetSelfTokenID(tokenId);
    OHOS::Security::AccessToken::AccessTokenKit::ReloadNativeTokenInfo();
}

HWTEST_F(TestAcrossByUT, test_across_by_ut, TestSize.Level1)
{
    CALL_TEST_DEBUG;
    SetTokenID();
    std::string remoteDeviceId;
    std::cout << "Input remoteDeviceId:" << std::endl;
    std::getline(std::cin, remoteDeviceId);
    std::cout << "remoteDeviceId: " << remoteDeviceId << std::endl;

    std::string localDeviceId;
    std::cout << "Input localDeviceId" << std::endl;
    std::getline(std::cin, localDeviceId);
    std::cout << "localDeviceId: " << localDeviceId << std::endl;

    std::cout << "----Start RegisterMissionListener----" << std::endl;
    if (AcrossAbilityAdapter::GetInstance()->RegisterMissionListener(remoteDeviceId) != RET_OK) {
        std::cout << "RegisterMissionListener failed" << std::endl;
    }
    std::cout << "----RegisterMissionListener success----" << std::endl;
    std::string op;
    while (true) {
        std::cout << "type <continueCustom> to ContinueCustomMission" << std::endl;
        std::cout << "type <continueNote> to ContinueNote" << std::endl;
        std::cout << "type <unReg> to UnRegisterMissionListener" << std::endl;
        getline(std::cin, op);
        if (op == "continueCustom") {
            std::string bundleName;
            std::cout << "Input bundleName to continue" << std::endl;
            getline(std::cin, bundleName);
            if (AcrossAbilityAdapter::GetInstance()->ContinueMission(bundleName, remoteDeviceId, localDeviceId) != RET_OK) {
                std::cout << "continueCustom failed" << std::endl;
            }
        }  else if (op == "UnReg") {
            std::cout << "----Start UnRegisterMissionListener----" << std::endl;
            if (AcrossAbilityAdapter::GetInstance()->UnRegisterMissionListener(remoteDeviceId) != RET_OK) {
                std::cout << "----UnRegisterMissionListener failed----" << std::endl;
            } else {
                std::cout << "----UnRegisterMissionListener success----" << std::endl;
            }
        } else {
            std::cout << "Unsupported op" << std::endl;
        }
    }
}
} // namespace DeviceStatus
} // namespace Msdp
} // namespace OHOS
