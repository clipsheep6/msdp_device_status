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

#include "util.h"

#include <unistd.h>

#include <string>

#include "securec.h"
#include <sys/prctl.h>
#include <sys/stat.h>
#include <sys/syscall.h>

#include "devicestatus_hilog_wrapper.h"
#include "fi_log.h"

namespace OHOS {
namespace Msdp {
namespace DeviceStatus {
namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, MSDP_DOMAIN_ID, "Util" };
constexpr size_t BUF_TID_SIZE = 10;
constexpr size_t PROGRAM_NAME_SIZE = 256;
constexpr size_t BUF_CMD_SIZE = 512;
constexpr uint32_t BASE_YEAR = 1900;
constexpr uint32_t BASE_MON = 1;
constexpr uint32_t MS_NS = 1000000;
constexpr int32_t FILE_SIZE_MAX = 0x5000;
const std::string SVG_PATH = "/system/etc/device_status/drag_icon/";
} // namespace

int32_t GetPid()
{
    return static_cast<int32_t>(getpid());
}

static std::string GetThisThreadIdOfString()
{
    thread_local std::string threadLocalId;
    if (threadLocalId.empty()) {
        long tid = syscall(SYS_gettid);
        char buf[BUF_TID_SIZE] = {};
        const int32_t ret = sprintf_s(buf, BUF_TID_SIZE, "%06d", tid);
        if (ret < 0) {
            FI_HILOGE("call sprintf_s failed, ret:%{public}d", ret);
            return threadLocalId;
        }
        buf[BUF_TID_SIZE - 1] = '\0';
        threadLocalId = buf;
    }

    return threadLocalId;
}

uint64_t GetThisThreadId()
{
    std::string stid = GetThisThreadIdOfString();
    auto tid = std::stoull(stid);
    return tid;
}

int64_t GetMillisTime()
{
    auto timeNow = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now());
    auto tmp = std::chrono::duration_cast<std::chrono::milliseconds>(timeNow.time_since_epoch());
    return tmp.count();
}

void GetTimeStamp(std::string &startTime)
{
    timespec curTime;
    clock_gettime(CLOCK_REALTIME, &curTime);
    struct tm *timeinfo = localtime(&(curTime.tv_sec));
    if (timeinfo == nullptr) {
        DEV_HILOGE(SERVICE, "get localtime failed");
        return;
    }
    startTime.append(std::to_string(timeinfo->tm_year + BASE_YEAR)).append("-")
        .append(std::to_string(timeinfo->tm_mon + BASE_MON)).append("-").append(std::to_string(timeinfo->tm_mday))
        .append(" ").append(std::to_string(timeinfo->tm_hour)).append(":").append(std::to_string(timeinfo->tm_min))
        .append(":").append(std::to_string(timeinfo->tm_sec)).append(".")
        .append(std::to_string(curTime.tv_nsec / MS_NS));
}

void SetThreadName(const std::string &name)
{
    prctl(PR_SET_NAME, name.c_str());
}

static size_t StringToken(std::string &str, const std::string &sep, std::string &token)
{
    token = "";
    if (str.empty()) {
        return str.npos;
    }
    size_t pos = str.npos;
    size_t tmp = 0;
    for (auto &item : sep) {
        tmp = str.find(item);
        if (str.npos != tmp) {
            pos = (std::min)(pos, tmp);
        }
    }
    if (str.npos != pos) {
        token = str.substr(0, pos);
        if (str.npos != pos + 1) {
            str = str.substr(pos + 1, str.npos);
        }
        if (pos == 0) {
            return StringToken(str, sep, token);
        }
    } else {
        token = str;
        str = "";
    }
    return token.size();
}

size_t StringSplit(const std::string &str, const std::string &sep, std::vector<std::string> &vecList)
{
    size_t size;
    auto strs = str;
    std::string token;
    while (str.npos != (size = StringToken(strs, sep, token))) {
        vecList.push_back(token);
    }
    return vecList.size();
}

std::string StringPrintf(const char *format, ...)
{
    char space[1024];

    va_list ap;
    va_start(ap, format);
    std::string result;
    int32_t ret = vsnprintf_s(space, sizeof(space), sizeof(space) - 1, format, ap);
    if (ret >= RET_OK && (size_t)ret < sizeof(space)) {
        result = space;
    } else {
        FI_HILOGE("The buffer is overflow");
    }
    va_end(ap);
    return result;
}

static std::string GetFileName(const std::string &strPath)
{
    size_t nPos = strPath.find_last_of('/');
    if (strPath.npos == nPos) {
        nPos = strPath.find_last_of('\\');
    }
    if (strPath.npos == nPos) {
        return strPath;
    }

    return strPath.substr(nPos + 1, strPath.npos);
}

const char* GetProgramName()
{
    static char programName[PROGRAM_NAME_SIZE] = {};
    if (programName[0] != '\0') {
        return programName;
    }

    char buf[BUF_CMD_SIZE] = { 0 };
    if (sprintf_s(buf, BUF_CMD_SIZE, "/proc/%d/cmdline", static_cast<int32_t>(getpid())) == -1) {
        FI_HILOGE("GetProcessInfo sprintf_s cmdline error");
        return "";
    }
    FILE *fp = fopen(buf, "rb");
    if (fp == nullptr) {
        FI_HILOGE("The fp is nullptr, filename:%{public}s", buf);
        return "";
    }
    static constexpr size_t bufLineSize = 512;
    char bufLine[bufLineSize] = { 0 };
    if ((fgets(bufLine, bufLineSize, fp) == nullptr)) {
        FI_HILOGE("fgets failed");
        if (fclose(fp) != 0) {
            FI_HILOGW("Close file failed");
        }
        fp = nullptr;
        return "";
    }
    if (fclose(fp) != 0) {
        FI_HILOGW("Close file:%{public}s failed", buf);
    }
    fp = nullptr;

    std::string tempName(bufLine);
    tempName = GetFileName(tempName);
    if (tempName.empty()) {
        FI_HILOGE("tempName is empty");
        return "";
    }
    size_t copySize = std::min(tempName.size(), PROGRAM_NAME_SIZE - 1);
    if (copySize == 0) {
        FI_HILOGE("The copySize is 0");
        return "";
    }
    errno_t ret = memcpy_s(programName, PROGRAM_NAME_SIZE, tempName.c_str(), copySize);
    if (ret != EOK) {
        FI_HILOGE("memcpy_s failed");
        return "";
    }
    FI_HILOGI("Get program name success, programName:%{public}s", programName);

    return programName;
}

bool CheckFileExtendName(const std::string &filePath, const std::string &checkExtension)
{
    std::string::size_type pos = filePath.find_last_of('.');
    if (pos == std::string::npos) {
        FI_HILOGE("File is not found extension");
        return false;
    }
    return (filePath.substr(pos + 1, filePath.npos) == checkExtension);
}

int32_t GetFileSize(const std::string &filePath)
{
    struct stat statbuf = { 0 };
    if (stat(filePath.c_str(), &statbuf) != 0) {
        FI_HILOGE("Get file size error, errno:%{public}d", errno);
        return RET_ERR;
    }
    return statbuf.st_size;
}

bool IsValidPath(const std::string &rootDir, const std::string &filePath)
{
    return (filePath.compare(0, rootDir.size(), rootDir) == 0);
}

bool IsValidSvgPath(const std::string &filePath)
{
    return IsValidPath(SVG_PATH, filePath);
}

bool IsFileExists(const std::string &fileName)
{
    return (access(fileName.c_str(), F_OK) == 0);
}

bool IsValidSvgFile(const std::string &filePath)
{
    CALL_DEBUG_ENTER;
    if (filePath.empty()) {
        FI_HILOGE("FilePath is empty");
        return false;
    }
    char realPath[PATH_MAX] = {};
    if (realpath(filePath.c_str(), realPath) == nullptr) {
        FI_HILOGE("Realpath return nullptr, realPath:%{public}s", realPath);
        return false;
    }
    if (!IsValidSvgPath(realPath)) {
        FI_HILOGE("File path invalid");
        return false;
    }
    if (!IsFileExists(realPath)) {
        FI_HILOGE("File not exist");
        return false;
    }
    if (!CheckFileExtendName(realPath, "svg")) {
        FI_HILOGE("Unable to parse files other than svg format");
        return false;
    }
    int32_t fileSize = GetFileSize(realPath);
    if ((fileSize <= 0) || (fileSize > FILE_SIZE_MAX)) {
        FI_HILOGE("File size out of read range");
        return false;
    }
    return true;
}

bool IsNum(const std::string &str)
{
    std::istringstream sin(str);
    double num;
    return (sin >> num) && sin.eof();
}

int32_t ChangeNumber(int32_t num)
{
    if (num < 0) {
        num = ~(num - 1);
    } else if (num > 0) {
        num = ~num + 1;
    }
    FI_HILOGD("Change number succeed, num:%{public}d", num);
    return num;
}
} // namespace DeviceStatus
} // namespace Msdp
} // namespace OHOS