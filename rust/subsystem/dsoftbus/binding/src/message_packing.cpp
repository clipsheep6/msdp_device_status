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

#include "message_packing.h"

#include "message_packing_internal.h"

namespace {
constexpr ::OHOS::HiviewDFX::HiLogLabel LABEL { LOG_CORE, ::OHOS::Msdp::MSDP_DOMAIN_ID, "MessagePacking" };
} // namespace

CJsonStruct* CGetCJsonObj() {
    CALL_DEBUG_ENTER;
    auto cJsonObj = new (std::nothrow) CJsonStruct(OHOS::DelayedSingleton<CJson>::GetInstance()->cJsonObj);
    return cJsonObj;
}

void CSaveHandleCb(const HandleCb callback) {
    CALL_DEBUG_ENTER;
    OHOS::DelayedSingleton<HandRecvMsg>::GetInstance()->cb = callback;
}

HandleCb CGetHandleCb() {
    CALL_DEBUG_ENTER;
    return OHOS::DelayedSingleton<HandRecvMsg>::GetInstance()->cb;
}

bool CAddNumber(CJsonStruct* cJsonObj, int32_t value, const char* str) {
    CALL_DEBUG_ENTER;
    if (cJsonObj == nullptr) {
        return false;
    }

    cJSON_AddItemToObject(cJsonObj->cJsonObj, str, cJSON_CreateNumber(value));
    return true;
}

bool CAddbool(CJsonStruct* cJsonObj, bool value, const char* str) {
    CALL_DEBUG_ENTER;
    if (cJsonObj == nullptr) {
        return false;
    }

    cJSON_AddItemToObject(cJsonObj->cJsonObj, str, cJSON_CreateBool(value));
    return true;
}

bool CAddString(CJsonStruct* cJsonObj, const char* value, const char* str) {
    CALL_DEBUG_ENTER;
    if (cJsonObj == nullptr) {
        return false;
    }

    cJSON_AddItemToObject(cJsonObj->cJsonObj, str, cJSON_CreateString(value));
    return true;
}

bool CJsonPrint(CJsonStruct* cJsonObj, char* msg) {
    CALL_DEBUG_ENTER;
    if (cJsonObj == nullptr) {
        return false;
    }

    msg = cJSON_Print(cJsonObj->cJsonObj);
    return true; 
}

bool CJsonDelete(CJsonStruct* cJsonObj) {
    CALL_DEBUG_ENTER;
    if (cJsonObj == nullptr) {
        return false;
    }

    cJSON_Delete(cJsonObj->cJsonObj);
    return true;
}

void CJsonFree(char* str) {
    CALL_DEBUG_ENTER;
    cJSON_free((void*)str);
}

void CParse(const char* message, CJsonStruct* cJsonObj) {
    CALL_DEBUG_ENTER;
    cJsonObj->cJsonObj = cJSON_Parse(message);
    
}

bool CIsJsonObj(CJsonStruct* cJsonObj) {
    CALL_DEBUG_ENTER;
    return cJSON_IsObject(cJsonObj->cJsonObj);
}

bool CIsNumber(CJsonStruct* cJsonObj) {
    CALL_DEBUG_ENTER;
    return cJSON_IsNumber(cJsonObj->cJsonObj);
}

bool CIsBool(CJsonStruct* cJsonObj) {
    CALL_DEBUG_ENTER;
    return cJSON_IsBool(cJsonObj->cJsonObj);
}

bool CIsString(CJsonStruct* cJsonObj) {
    CALL_DEBUG_ENTER;
    return cJSON_IsString(cJsonObj->cJsonObj);
}

bool CIsTrue(CJsonStruct* cJsonObj) {
    CALL_DEBUG_ENTER;
    return cJSON_IsTrue(cJsonObj->cJsonObj);
}

int32_t CGetValueInt(CJsonStruct* cJsonObj) {
    CALL_DEBUG_ENTER;
    return cJsonObj->cJsonObj->valueint;
}

// bool CGetValueBool(CJsonStruct* cJsonObj) {
//     CALL_DEBUG_ENTER;
//     return cJsonObj->cJsonObj->valuebool;
// }

const char* CGetValueString(CJsonStruct* cJsonObj) {
    return cJsonObj->cJsonObj->valuestring;
}

bool CGetObjectItemCaseSensitive(CJsonStruct* cJsonObj, const char* type, CJsonStruct* comType) {
    CALL_DEBUG_ENTER;
    if (cJsonObj == nullptr) {
        return false;
    }
    comType->cJsonObj = cJSON_GetObjectItemCaseSensitive(cJsonObj->cJsonObj, type);
    return true;
}