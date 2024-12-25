/*
 * Copyright (c) 2023-2024 Huawei Device Co., Ltd.
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

#include "drag_server.h"

#include "tokenid_kit.h"

#include "accesstoken_kit.h"
#include "drag_params.h"
#include "devicestatus_define.h"

#undef LOG_TAG
#define LOG_TAG "DragServer"

namespace OHOS {
namespace Msdp {
namespace DeviceStatus {

#ifdef OHOS_BUILD_UNIVERSAL_DRAG
DragServer::DragServer(IContext *env)
    : env_(env), universalDragWrapper_(env)
#else
DragServer::DragServer(IContext *env)
    : env_(env)
#endif // OHOS_BUILD_UNIVERSAL_DRAG
{
#ifdef OHOS_BUILD_UNIVERSAL_DRAG
    bool ret = universalDragWrapper_.InitUniversalDrag();
    if (!ret) {
        FI_HILOGW("Init universal drag failed");
    }
#endif // OHOS_BUILD_UNIVERSAL_DRAG
}

int32_t DragServer::Enable(CallingContext &context, MessageParcel &data, MessageParcel &reply)
{
    CALL_DEBUG_ENTER;
    return RET_ERR;
}

int32_t DragServer::Disable(CallingContext &context, MessageParcel &data, MessageParcel &reply)
{
    CALL_DEBUG_ENTER;
    return RET_ERR;
}

int32_t DragServer::Start(CallingContext &context, MessageParcel &data, MessageParcel &reply)
{
    CALL_DEBUG_ENTER;
    DragData dragData {};
    StartDragParam param { dragData };

    if (!param.Unmarshalling(data)) {
        FI_HILOGE("Failed to unmarshalling param");
        return RET_ERR;
    }
    CHKPR(env_, RET_ERR);
    auto session = env_->GetSocketSessionManager().FindSessionByPid(context.pid);
    CHKPR(session, RET_ERR);
    session->SetProgramName(GetPackageName(context.tokenId));
    return env_->GetDragManager().StartDrag(dragData, context.pid);
}

int32_t DragServer::Stop(CallingContext &context, MessageParcel &data, MessageParcel &reply)
{
    CALL_DEBUG_ENTER;
    StopDragParam param {};

    if (!param.Unmarshalling(data)) {
        FI_HILOGE("Failed to unmarshalling param");
        return RET_ERR;
    }
    CHKPR(env_, RET_ERR);
    return env_->GetDragManager().StopDrag(param.dropResult_, GetPackageName(context.tokenId), context.pid);
}

int32_t DragServer::WithOptionsStart(CallingContext &context, MessageParcel &data, MessageParcel &reply)
{
    CALL_DEBUG_ENTER;
    return RET_ERR;   
}

int32_t DragServer::AddWatch(CallingContext &context, uint32_t id, MessageParcel &data, MessageParcel &reply)
{
    CALL_DEBUG_ENTER;
    switch (id) {
        case DragRequestID::ADD_DRAG_LISTENER: {
            return AddListener(context, data);
        }
        case DragRequestID::ADD_SUBSCRIPT_LISTENER: {
            FI_HILOGD("Add subscript listener, from:%{public}d", context.pid);
            return env_->GetDragManager().AddSubscriptListener(context.pid);
        }
        default: {
            FI_HILOGE("Unexpected request ID (%{public}u)", id);
            return RET_ERR;
        }
    }
}

int32_t DragServer::RemoveWatch(CallingContext &context, uint32_t id, MessageParcel &data, MessageParcel &reply)
{
    CALL_DEBUG_ENTER;
    switch (id) {
        case DragRequestID::REMOVE_DRAG_LISTENER: {
            return RemoveListener(context, data);
        }
        case DragRequestID::REMOVE_SUBSCRIPT_LISTENER: {
            FI_HILOGD("Remove subscript listener, from:%{public}d", context.pid);
            return env_->GetDragManager().RemoveSubscriptListener(context.pid);
        }
        default: {
            FI_HILOGE("Unexpected request ID (%{public}u)", id);
            return RET_ERR;
        }
    }
}

int32_t DragServer::SetParam(CallingContext &context, uint32_t id, MessageParcel &data, MessageParcel &reply)
{
    CALL_DEBUG_ENTER;
    switch (id) {
        case DragRequestID::SET_DRAG_WINDOW_VISIBLE: {
            return SetDragWindowVisible(context, data, reply);
        }
        case DragRequestID::UPDATE_DRAG_STYLE: {
            return UpdateDragStyle(context, data, reply);
        }
        case DragRequestID::UPDATE_SHADOW_PIC: {
            return UpdateShadowPic(context, data, reply);
        }
        case DragRequestID::UPDATE_PREVIEW_STYLE: {
            return UpdatePreviewStyle(context, data, reply);
        }
        case DragRequestID::UPDATE_PREVIEW_STYLE_WITH_ANIMATION: {
            return UpdatePreviewAnimation(context, data, reply);
        }
        case DragRequestID::SET_DRAG_WINDOW_SCREEN_ID: {
            return SetDragWindowScreenId(context, data, reply);
        }
        case DragRequestID::ADD_SELECTED_PIXELMAP: {
            return AddSelectedPixelMap(context, data, reply);
        }
        case DragRequestID::SET_DRAG_SWITCH_STATE: {
            return SetDragSwitchState(context, data, reply);
        }
        case DragRequestID::SET_APP_DRAG_SWITCH_STATE: {
            return SetAppDragSwitchState(context, data, reply);
        }
        case DragRequestID::SET_DRAGGABLE_STATE: {
            return SetDraggableState(context, data, reply);
        }
        default: {
            FI_HILOGE("Unexpected request ID (%{public}u)", id);
            return RET_ERR;
        }
    }
}

int32_t DragServer::GetParam(CallingContext &context, uint32_t id, MessageParcel &data, MessageParcel &reply)
{
    CALL_DEBUG_ENTER;
    switch (id) {
        case DragRequestID::GET_DRAG_TARGET_PID: {
            FI_HILOGI("Get drag target pid, from:%{public}d", context.pid);
            return GetDragTargetPid(context, data, reply);
        }
        case DragRequestID::GET_UDKEY: {
            FI_HILOGI("Get udkey, from:%{public}d", context.pid);
            return GetUdKey(context, data, reply);
        }
        case DragRequestID::GET_SHADOW_OFFSET: {
            FI_HILOGD("Get shadow offset, from:%{public}d", context.pid);
            return GetShadowOffset(context, data, reply);
        }
        case DragRequestID::GET_DRAG_DATA: {
            FI_HILOGD("Get drag data, from:%{public}d", context.pid);
            return GetDragData(context, data, reply);
        }
        case DragRequestID::GET_DRAG_STATE: {
            FI_HILOGD("Get drag state, from:%{public}d", context.pid);
            return GetDragState(context, data, reply);
        }
        case DragRequestID::GET_DRAG_SUMMARY: {
            FI_HILOGD("Get drag summary, from:%{public}d", context.pid);
            return GetDragSummary(context, data, reply);
        }
        case DragRequestID::GET_DRAG_ACTION: {
            FI_HILOGI("Get drag action, from:%{public}d", context.pid);
            return GetDragAction(context, data, reply);
        }
        case DragRequestID::GET_EXTRA_INFO: {
            FI_HILOGI("Get extra info, from:%{public}d", context.pid);
            return GetExtraInfo(context, data, reply);
        }
        default: {
            FI_HILOGE("Unexpected request ID (%{public}u)", id);
            return RET_ERR;
        }
    }
}

int32_t DragServer::Control(CallingContext &context, uint32_t id, MessageParcel &data, MessageParcel &reply)
{
    CALL_DEBUG_ENTER;
    switch (id) {
        case DragRequestID::ADD_PRIVILEGE: {
            FI_HILOGI("Add privilege, from:%{public}d", context.pid);
            return env_->GetDragManager().AddPrivilege(context.tokenId);
        }
        case DragRequestID::ENTER_TEXT_EDITOR_AREA: {
            FI_HILOGI("Enter text editor area, from:%{public}d", context.pid);
            return EnterTextEditorArea(context, data, reply);
        }
        case DragRequestID::ROTATE_DRAG_WINDOW_SYNC: {
            FI_HILOGI("Rotate drag window sync, from:%{public}d", context.pid);
            return RotateDragWindowSync(context, data, reply);
        }
        case DragRequestID::ERASE_MOUSE_ICON: {
            FI_HILOGI("Erase mouse, from:%{public}d", context.pid);
            return env_->GetDragManager().EraseMouseIcon();
        }
        case DragRequestID::SET_MOUSE_DRAG_MONITOR_STATE: {
            FI_HILOGI("Set mouse drag monitor state, from:%{public}d", context.pid);
            return SetMouseDragMonitorState(context, data, reply);
        }
        default: {
            FI_HILOGE("Unexpected request ID (%{public}u)", id);
            return RET_ERR;
        }
    }
}

int32_t DragServer::AddListener(CallingContext &context, MessageParcel &data)
{
    AddDraglistenerParam param {};
    if (!param.Unmarshalling(data)) {
        FI_HILOGE("AddDraglistenerParam::Unmarshalling fail");
        return RET_ERR;
    }

    if (param.isJsCaller_ && !IsSystemHAPCalling(context)) {
        FI_HILOGE("The caller is not system hap");
        return COMMON_NOT_SYSTEM_APP;
    }
    FI_HILOGI("Add drag listener, from:%{public}d", context.pid);
    return env_->GetDragManager().AddListener(context.pid);
}

int32_t DragServer::RemoveListener(CallingContext &context, MessageParcel &data)
{
    RemoveDraglistenerParam param {};
    if (!param.Unmarshalling(data)) {
        FI_HILOGE("RemoveDraglistenerParam::Unmarshalling fail");
        return RET_ERR;
    }

    if (param.isJsCaller_ && !IsSystemHAPCalling(context)) {
        FI_HILOGE("The caller is not system hap");
        return COMMON_NOT_SYSTEM_APP;
    }
    FI_HILOGD("Remove drag listener, from:%{public}d", context.pid);
    return env_->GetDragManager().RemoveListener(context.pid);
}

int32_t DragServer::SetDragWindowVisible(CallingContext &context, MessageParcel &data, MessageParcel &reply)
{
    SetDragWindowVisibleParam param {};

    if (!param.Unmarshalling(data)) {
        FI_HILOGE("SetDragWindowVisibleParam::Unmarshalling fail");
        return RET_ERR;
    }
    FI_HILOGI("SetDragWindowVisible(%{public}d, %{public}d)", param.visible_, param.isForce_);
    return env_->GetDragManager().OnSetDragWindowVisible(param.visible_, param.isForce_);
}

int32_t DragServer::UpdateDragStyle(CallingContext &context, MessageParcel &data, MessageParcel &reply)
{
    UpdateDragStyleParam param {};

    if (!param.Unmarshalling(data)) {
        FI_HILOGE("UpdateDragStyleParam::Unmarshalling fail");
        return RET_ERR;
    }
    FI_HILOGI("UpdateDragStyle(%{public}d)", static_cast<int32_t>(param.cursorStyle_));
    return env_->GetDragManager().UpdateDragStyle(param.cursorStyle_, context.pid, context.tokenId, param.eventId_);
}

int32_t DragServer::UpdateShadowPic(CallingContext &context, MessageParcel &data, MessageParcel &reply)
{
    UpdateShadowPicParam param {};

    if (!param.Unmarshalling(data)) {
        FI_HILOGE("UpdateShadowPicParam::Unmarshalling fail");
        return RET_ERR;
    }
    FI_HILOGD("Updata shadow pic");
    return env_->GetDragManager().UpdateShadowPic(param.shadowInfo_);
}

int32_t DragServer::AddSelectedPixelMap(CallingContext &context, MessageParcel &data, MessageParcel &reply)
{
    AddSelectedPixelMapParam param {};

    if (!param.Unmarshalling(data)) {
        FI_HILOGE("AddSelectedPixelMap::Unmarshalling fail");
        return RET_ERR;
    }
    return env_->GetDragManager().AddSelectedPixelMap(param.pixelMap_);
}

int32_t DragServer::UpdatePreviewStyle(CallingContext &context, MessageParcel &data, MessageParcel &reply)
{
    UpdatePreviewStyleParam param {};

    if (!param.Unmarshalling(data)) {
        FI_HILOGE("UpdatePreviewStyleParam::Unmarshalling fail");
        return RET_ERR;
    }
    return env_->GetDragManager().UpdatePreviewStyle(param.previewStyle_);
}

int32_t DragServer::UpdatePreviewAnimation(CallingContext &context, MessageParcel &data, MessageParcel &reply)
{
    UpdatePreviewAnimationParam param {};

    if (!param.Unmarshalling(data)) {
        FI_HILOGE("UpdatePreviewAnimationParam::Unmarshalling fail");
        return RET_ERR;
    }
    return env_->GetDragManager().UpdatePreviewStyleWithAnimation(param.previewStyle_, param.previewAnimation_);
}

int32_t DragServer::RotateDragWindowSync(CallingContext &context, MessageParcel &data, MessageParcel &reply)
{
    RotateDragWindowSyncParam param {};

    if (!param.Unmarshalling(data)) {
        FI_HILOGE("RotateDragWindowSync::Unmarshalling fail");
        return RET_ERR;
    }
    return env_->GetDragManager().RotateDragWindowSync(param.rsTransaction_);
}

int32_t DragServer::SetDragWindowScreenId(CallingContext &context, MessageParcel &data, MessageParcel &reply)
{
    SetDragWindowScreenIdParam param {};

    if (!param.Unmarshalling(data)) {
        FI_HILOGE("SetDragWindowScreenId::Unmarshalling fail");
        return RET_ERR;
    }
    env_->GetDragManager().SetDragWindowScreenId(param.displayId_, param.screenId_);
    return RET_OK;
}

int32_t DragServer::GetDragTargetPid(CallingContext &context, MessageParcel &data, MessageParcel &reply)
{
    int32_t targetPid = env_->GetDragManager().GetDragTargetPid();
    GetDragTargetPidReply targetPidReply { targetPid };

    if (!targetPidReply.Marshalling(reply)) {
        FI_HILOGE("GetDragTargetPidReply::Marshalling fail");
        return RET_ERR;
    }
    return RET_OK;
}

int32_t DragServer::GetUdKey(CallingContext &context, MessageParcel &data, MessageParcel &reply)
{
    std::string udKey;

    int32_t ret = env_->GetDragManager().GetUdKey(udKey);
    if (ret != RET_OK) {
        FI_HILOGE("IDragManager::GetUdKey fail, error:%{public}d", ret);
        return ret;
    }
    GetUdKeyReply udKeyReply { std::move(udKey) };

    if (!udKeyReply.Marshalling(reply)) {
        FI_HILOGE("GetUdKeyReply::Marshalling fail");
        return RET_ERR;
    }
    return RET_OK;
}

int32_t DragServer::GetShadowOffset(CallingContext &context, MessageParcel &data, MessageParcel &reply)
{
    ShadowOffset shadowOffset {};

    int32_t ret = env_->GetDragManager().OnGetShadowOffset(shadowOffset);
    if (ret != RET_OK) {
        FI_HILOGE("IDragManager::GetShadowOffset fail, error:%{public}d", ret);
        return ret;
    }
    GetShadowOffsetReply shadowOffsetReply { shadowOffset };

    if (!shadowOffsetReply.Marshalling(reply)) {
        FI_HILOGE("GetShadowOffsetReply::Marshalling fail");
        return RET_ERR;
    }
    return RET_OK;
}

int32_t DragServer::GetDragData(CallingContext &context, MessageParcel &data, MessageParcel &reply)
{
    DragData dragData {};

    int32_t ret = env_->GetDragManager().GetDragData(dragData);
    if (ret != RET_OK) {
        FI_HILOGE("IDragManager::GetDragData fail, error:%{public}d", ret);
        return ret;
    }
    GetDragDataReply dragDataReply { static_cast<const DragData&>(dragData) };

    if (!dragDataReply.Marshalling(reply)) {
        FI_HILOGE("GetDragDataReply::Marshalling fail");
        return RET_ERR;
    }
    return RET_OK;
}

int32_t DragServer::GetDragState(CallingContext &context, MessageParcel &data, MessageParcel &reply)
{
    DragState dragState {};

    int32_t ret = env_->GetDragManager().GetDragState(dragState);
    if (ret != RET_OK) {
        FI_HILOGE("IDragManager::GetDragState fail, error:%{public}d", ret);
        return ret;
    }
    GetDragStateReply dragStateReply { dragState };

    if (!dragStateReply.Marshalling(reply)) {
        FI_HILOGE("GetDragStateReply::Marshalling fail");
        return RET_ERR;
    }
    return RET_OK;
}

int32_t DragServer::GetDragSummary(CallingContext &context, MessageParcel &data, MessageParcel &reply)
{
    GetDragSummaryParam param {};

    if (!param.Unmarshalling(data)) {
        FI_HILOGE("GetDragSummary::Unmarshalling fail");
        return RET_ERR;
    }
    if (param.isJsCaller_ && !IsSystemHAPCalling(context)) {
        FI_HILOGE("The caller is not system hap");
        return COMMON_NOT_SYSTEM_APP;
    }
    std::map<std::string, int64_t> summaries;

    int32_t ret = env_->GetDragManager().GetDragSummary(summaries);
    if (ret != RET_OK) {
        FI_HILOGE("IDragManager::GetDragSummary fail, error:%{public}d", ret);
        return ret;
    }
    GetDragSummaryReply summaryReply { std::move(summaries) };

    if (!summaryReply.Marshalling(reply)) {
        FI_HILOGE("GetDragSummaryReply::Marshalling fail");
        return RET_ERR;
    }
    return RET_OK;
}

int32_t DragServer::SetDragSwitchState(CallingContext &context, MessageParcel &data, MessageParcel &reply)
{
    SetDragSwitchStateParam param {};

    if (!param.Unmarshalling(data)) {
        FI_HILOGE("SetDragSwitchStateParam::Unmarshalling fail");
        return RET_ERR;
    }
    if (param.isJsCaller_ && !IsSystemHAPCalling(context)) {
        FI_HILOGE("The caller is not system hap");
        return COMMON_NOT_SYSTEM_APP;
    }
#ifdef OHOS_BUILD_UNIVERSAL_DRAG
    universalDragWrapper_.SetDragSwitchState(param.enable_);
#endif // OHOS_BUILD_UNIVERSAL_DRAG
    return RET_OK;
}

int32_t DragServer::SetAppDragSwitchState(CallingContext &context, MessageParcel &data, MessageParcel &reply)
{
    SetAppDragSwitchStateParam param {};

    if (!param.Unmarshalling(data)) {
        FI_HILOGE("SetAppDragSwitchStateParam::Unmarshalling fail");
        return RET_ERR;
    }
    if (param.isJsCaller_ && !IsSystemHAPCalling(context)) {
        FI_HILOGE("The caller is not system hap");
        return COMMON_NOT_SYSTEM_APP;
    }
#ifdef OHOS_BUILD_UNIVERSAL_DRAG
    universalDragWrapper_.SetAppDragSwitchState(param.pkgName_, param.enable_);
#endif // OHOS_BUILD_UNIVERSAL_DRAG
    return RET_OK;
}

int32_t DragServer::GetDragAction(CallingContext &context, MessageParcel &data, MessageParcel &reply)
{
    DragAction dragAction {};

    int32_t ret = env_->GetDragManager().GetDragAction(dragAction);
    if (ret != RET_OK) {
        FI_HILOGE("IDragManager::GetDragSummary fail, error:%{public}d", ret);
        return ret;
    }
    GetDragActionReply dragActionReply { dragAction };

    if (!dragActionReply.Marshalling(reply)) {
        FI_HILOGE("GetDragActionReply::Marshalling fail");
        return RET_ERR;
    }
    return RET_OK;
}

int32_t DragServer::GetExtraInfo(CallingContext &context, MessageParcel &data, MessageParcel &reply)
{
    std::string extraInfo;

    int32_t ret = env_->GetDragManager().GetExtraInfo(extraInfo);
    if (ret != RET_OK) {
        FI_HILOGE("IDragManager::GetExtraInfo fail, error:%{public}d", ret);
        return ret;
    }
    GetExtraInfoReply extraInfoReply { std::move(extraInfo) };

    if (!extraInfoReply.Marshalling(reply)) {
        FI_HILOGE("GetExtraInfoReply::Marshalling fail");
        return RET_ERR;
    }
    return RET_OK;
}

int32_t DragServer::EnterTextEditorArea(CallingContext &context, MessageParcel &data, MessageParcel &reply)
{
    EnterTextEditorAreaParam param {};

    if (!param.Unmarshalling(data)) {
        FI_HILOGE("EnterTextEditorAreaParam::Unmarshalling fail");
        return RET_ERR;
    }
    return env_->GetDragManager().EnterTextEditorArea(param.state);
}

int32_t DragServer::SetMouseDragMonitorState(CallingContext &context, MessageParcel &data, MessageParcel &reply)
{
    SetMouseDragMonitorStateParam param {};

    if (!param.Unmarshalling(data)) {
        FI_HILOGE("SetMouseDragMonitorStateParam::Unmarshalling fail");
        return RET_ERR;
    }
    return env_->GetDragManager().SetMouseDragMonitorState(param.state);
}

std::string DragServer::GetPackageName(Security::AccessToken::AccessTokenID tokenId)
{
    std::string packageName = std::string();
    int32_t tokenType = Security::AccessToken::AccessTokenKit::GetTokenTypeFlag(tokenId);
    switch (tokenType) {
        case Security::AccessToken::ATokenTypeEnum::TOKEN_HAP: {
            Security::AccessToken::HapTokenInfo hapInfo;
            if (Security::AccessToken::AccessTokenKit::GetHapTokenInfo(tokenId, hapInfo) != 0) {
                FI_HILOGE("Get hap token info failed");
            } else {
                packageName = hapInfo.bundleName;
            }
            break;
        }
        case Security::AccessToken::ATokenTypeEnum::TOKEN_NATIVE:
        case Security::AccessToken::ATokenTypeEnum::TOKEN_SHELL: {
            Security::AccessToken::NativeTokenInfo tokenInfo;
            if (Security::AccessToken::AccessTokenKit::GetNativeTokenInfo(tokenId, tokenInfo) != 0) {
                FI_HILOGE("Get native token info failed");
            } else {
                packageName = tokenInfo.processName;
            }
            break;
        }
        default: {
            FI_HILOGW("token type not match");
            break;
        }
    }
    return packageName;
}

bool DragServer::IsSystemServiceCalling(CallingContext &context)
{
    auto flag = Security::AccessToken::AccessTokenKit::GetTokenTypeFlag(context.tokenId);
    if ((flag == Security::AccessToken::ATokenTypeEnum::TOKEN_NATIVE) ||
        (flag == Security::AccessToken::ATokenTypeEnum::TOKEN_SHELL)) {
        FI_HILOGI("system service calling, flag:%{public}u", flag);
        return true;
    }
    return false;
}

bool DragServer::IsSystemHAPCalling(CallingContext &context)
{
    if (IsSystemServiceCalling(context)) {
        return true;
    }
    return Security::AccessToken::TokenIdKit::IsSystemAppByFullTokenID(context.fullTokenId);
}

int32_t DragServer::SetDraggableState(CallingContext &context, MessageParcel &data, MessageParcel &reply)
{
    SetDraggableStateParam param {};

    if (!param.Unmarshalling(data)) {
        FI_HILOGE("SetDraggableStateParam::Unmarshalling fail");
        return RET_ERR;
    }
#ifdef OHOS_BUILD_UNIVERSAL_DRAG
    universalDragWrapper_.SetDragableState(param.state_);
#endif // OHOS_BUILD_UNIVERSAL_DRAG
    return RET_OK;
}
} // namespace DeviceStatus
} // namespace Msdp
} // namespace OHOS