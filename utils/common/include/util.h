/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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
#ifndef OHOS_MSDP_DEVICE_STATUS_UTIL_H
#define OHOS_MSDP_DEVICE_STATUS_UTIL_H

#include <limits>
#include <string>
#include <vector>

#include <sys/types.h>

namespace OHOS {
namespace Msdp {
namespace DeviceStatus {
int32_t GetPid();
const char* GetProgramName();
int64_t GetMillisTime();

uint64_t GetThisThreadId();

void SetThreadName(const std::string &name);
void GetTimeStamp(std::string &startTime);

template<typename T>
bool Add(T op1, T op2, T minVal, T maxVal, T &res)
{
    if (op1 >= 0) {
        if (op2 > maxVal - op1) {
            return false;
        }
    } else {
        if (op2 < minVal - op1) {
            return false;
        }
    }
    res = op1 + op2;
    return true;
}

inline bool AddInt32(int32_t op1, int32_t op2, int32_t &res)
{
    return Add(op1, op2, std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::max(), res);
}

inline bool AddInt64(int64_t op1, int64_t op2, int64_t &res)
{
    return Add(op1, op2, std::numeric_limits<int64_t>::min(), std::numeric_limits<int64_t>::max(), res);
}

inline bool AddLong(long op1, long op2, long &res)
{
    return Add(op1, op2, std::numeric_limits<long>::min(), std::numeric_limits<long>::max(), res);
}

inline bool AddLongLong(long long op1, long long op2, long long &res)
{
    return Add(op1, op2, std::numeric_limits<long long>::min(), std::numeric_limits<long long>::max(), res);
}

template<typename T>
bool SubUnsigned(T op1, T op2, T &res)
{
    if (op1 < op2) {
        return false;
    }
    res = op1 - op2;
    return true;
}

inline bool SubUnsignedInt(unsigned int op1, unsigned int op2, unsigned int &res)
{
    return SubUnsigned(op1, op2, res);
}

inline bool SubUnsignedLong(unsigned long op1, unsigned long op2, unsigned long &res)
{
    return SubUnsigned(op1, op2, res);
}

template<typename T>
bool SubSigned(T op1, T op2, T minVal, T maxVal, T &res)
{
    if (op1 < 0 && op2 > 0 && minVal + op2 > op1) {
        return false;
    }
    if (op1 >= 0 && op2 < 0 && maxVal + op2 < op1 ) {
        return false;
    }
    res = op1 - op2;
    return true;
}

inline bool SubSignedInt32(int32_t op1, int32_t op2, int32_t &res)
{
    return SubSigned(op1, op2, std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::max(), res);
}

inline bool SubSignedInt64(int64_t op1, int64_t op2, int64_t &res)
{
    return SubSigned(op1, op2, std::numeric_limits<int64_t>::min(), std::numeric_limits<int64_t>::max(), res);
}

inline bool SubSignedLong(long op1, long op2, long &res)
{
    return SubSigned(op1, op2, std::numeric_limits<long>::min(), std::numeric_limits<long>::max(), res);
}

template<typename T>
bool Multiply(T op1, T op2, T minVal, T maxVal, T &res)
{
    if (op1 > 0 && op2 > 0 && maxVal / op1 < op2) {
        return false;
    }
    if (op1 < 0 && op2 < 0 && maxVal / (- op1) < (- op2)) {
        return false;
    }
    if (op1 < 0 && op2 > 0 && minVal / op1 < op2) {
        return false;
    }
    if (op1 > 0 && op2 < 0 && minVal / op2 < op1) {
        return false;
    }
    res = op1 * op2;
    return true;
}

inline bool MultiplyInt32(int32_t op1, int32_t op2, int32_t& res) {
    return Multiply(op1, op2, std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::max(), res);
}

inline bool MultiplyInt64(int64_t op1, int64_t op2, int64_t& res) {
    return Multiply(op1, op2, std::numeric_limits<int64_t>::min(), std::numeric_limits<int64_t>::max(), res);
}

inline bool MultiplyLong(long op1, long op2, long& res) {
    return Multiply(op1, op2, std::numeric_limits<long>::min(), std::numeric_limits<long>::max(), res);
}

size_t StringSplit(const std::string &str, const std::string &sep, std::vector<std::string> &vecList);

std::string StringPrintf(const char *format, ...);
bool CheckFileExtendName(const std::string &filePath, const std::string &checkExtension);
int32_t GetFileSize(const std::string &filePath);
bool IsValidPath(const std::string &rootDir, const std::string &filePath);
bool IsValidSvgPath(const std::string &filePath);
bool IsFileExists(const std::string &fileName);
bool IsValidSvgFile(const std::string &filePath);
bool IsNum(const std::string &str);
int32_t ChangeNumber(int32_t num);
} // namespace DeviceStatus
} // namespace Msdp
} // namespace OHOS
#endif // OHOS_MSDP_DEVICE_STATUS_UTIL_H
