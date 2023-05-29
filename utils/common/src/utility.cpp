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

#include "utility.h"

#include <grp.h>
#include <pwd.h>
#include <unistd.h>

#include <cerrno>
#include <limits>
#include <regex>
#include <sstream>

#include <sys/stat.h>
#include <sys/types.h>

#include "securec.h"

#include "devicestatus_define.h"

namespace OHOS {
namespace Msdp {
namespace DeviceStatus {
namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL { LOG_CORE, MSDP_DOMAIN_ID, "Utility" };
}

size_t Utility::CopyNulstr(char *dest, size_t size, const char *src)
{
    CHKPR(dest, 0);
    CHKPR(src, 0);

    size_t len = strlen(src);
    if (len >= size) {
        if (size > 1) {
            len = size - 1;
        } else {
            len = 0;
        }
    }
    if (len > 0) {
        errno_t ret = memcpy_s(dest, size, src, len);
        if (ret != EOK) {
            FI_HILOGW("memcpy_s:bounds checking failed");
        }
    }
    if (size > 0) {
        dest[len] = '\0';
    }
    return len;
}

bool Utility::StartWith(const char *str, const char *prefix)
{
    size_t prefixlen = strlen(prefix);
    return (prefixlen > 0 ? (strncmp(str, prefix, strlen(prefix)) == 0) : false);
}

bool Utility::StartWith(const std::string &str, const std::string &prefix)
{
    if (str.size() < prefix.size()) {
        return false;
    }
    return (str.compare(0, prefix.size(), prefix) == 0);
}

void Utility::RemoveTrailingChars(char *path, char c)
{
    CHKPV(path);
    size_t len = strlen(path);
    while (len > 0 && path[len-1] == c) {
        path[--len] = '\0';
    }
}

void Utility::RemoveTrailingChars(std::string &path, const std::string &toRemoved)
{
    while (!path.empty() && (toRemoved.find(path.back()) != std::string::npos)) {
        path.pop_back();
    }
}

void Utility::RemoveSpace(std::string &str)
{
    str.erase(remove_if(str.begin(), str.end(), [](unsigned char c) { return std::isspace(c);}), str.end());
}

bool Utility::IsInteger(const std::string &target)
{
    std::regex pattern("^\\s*-?(0|([1-9]\\d*))\\s*$");
    return std::regex_match(target, pattern);
}

bool Utility::DoesFileExist(const char *path)
{
    return (access(path, F_OK) == 0);
}

ssize_t Utility::GetFileSize(const char *path)
{
    struct stat buf {};
    ssize_t sz { 0 };

    if (stat(path, &buf) == 0) {
        if (S_ISREG(buf.st_mode)) {
            sz = buf.st_size;
        } else {
            FI_HILOGE("Not regular file:\'%{public}s\'", path);
        }
    } else {
        FI_HILOGE("stat(\'%{public}s\') failed:%{public}s", path, strerror(errno));
    }
    return sz;
}

void Utility::ShowFileAttributes(const char *path)
{
    CALL_DEBUG_ENTER;
    FI_HILOGD("======================= File Attributes ========================");
    FI_HILOGD("%{public}20s:%{public}s", "FILE NAME", path);

    struct stat buf {};
    if (stat(path, &buf) != 0) {
        FI_HILOGE("stat(\'%{public}s\') failed:%{public}s", path, strerror(errno));
    } else {
        if (S_ISDIR(buf.st_mode)) {
            FI_HILOGD("%{public}20s: directory", "TYPE");
        } else if (S_ISCHR(buf.st_mode)) {
            FI_HILOGD("%{public}20s: character special file", "TYPE");
        } else if (S_ISREG(buf.st_mode)) {
            FI_HILOGD("%{public}20s: regular file", "TYPE");
        }

        std::ostringstream ss;
        if (buf.st_mode & S_IRUSR) {
            ss << "U+R ";
        }
        if (buf.st_mode & S_IWUSR) {
            ss << "U+W ";
        }
        if (buf.st_mode & S_IXUSR) {
            ss << "U+X ";
        }
        if (buf.st_mode & S_IRGRP) {
            ss << "G+R ";
        }
        if (buf.st_mode & S_IWGRP) {
            ss << "G+W ";
        }
        if (buf.st_mode & S_IXGRP) {
            ss << "G+X ";
        }
        if (buf.st_mode & S_IROTH) {
            ss << "O+R ";
        }
        if (buf.st_mode & S_IWOTH) {
            ss << "O+W ";
        }
        if (buf.st_mode & S_IXOTH) {
            ss << "O+X ";
        }
        FI_HILOGD("%{public}20s:%{public}s", "PERMISSIONS", ss.str().c_str());
    }
}

void Utility::ShowUserAndGroup()
{
    CALL_DEBUG_ENTER;
    static constexpr size_t BUFSIZE { 1024 };
    char buffer[BUFSIZE];
    uid_t uid;
    gid_t gid;
    struct passwd buf, *pbuf;
    struct group grp, *pgrp;

    FI_HILOGD("======================= Users and Groups =======================");
    uid = getuid();
    if (getpwuid_r(uid, &buf, buffer, sizeof(buffer), &pbuf) != 0) {
        FI_HILOGE("getpwuid_r failed:%{public}s", strerror(errno));
    } else {
        FI_HILOGD("%{public}20s:%{public}10u%{public}20s", "USER", uid, buf.pw_name);
    }

    gid = getgid();
    if (getgrgid_r(gid, &grp, buffer, sizeof(buffer), &pgrp) != 0) {
        FI_HILOGE("getgrgid_r failed:%{public}s", strerror(errno));
    } else {
        FI_HILOGD("%{public}20s:%{public}10u%{public}20s", "GROUP", gid, grp.gr_name);
    }

    uid = geteuid();
    if (getpwuid_r(uid, &buf, buffer, sizeof(buffer), &pbuf) != 0) {
        FI_HILOGE("getpwuid_r failed:%{public}s", strerror(errno));
    } else {
        FI_HILOGD("%{public}20s:%{public}10u%{public}20s", "EFFECTIVE USER", uid, buf.pw_name);
    }

    gid = getegid();
    if (getgrgid_r(gid, &grp, buffer, sizeof(buffer), &pgrp) != 0) {
        FI_HILOGE("getgrgid_r failed:%{public}s", strerror(errno));
    } else {
        FI_HILOGD("%{public}20s:%{public}10u%{public}20s", "EFFECTIVE GROUP", gid, grp.gr_name);
    }

    gid_t groups[NGROUPS_MAX + 1];
    int32_t ngrps = getgroups(sizeof(groups), groups);
    for (int32_t i = 0; i < ngrps; ++i) {
        if (getgrgid_r(groups[i], &grp, buffer, sizeof(buffer), &pgrp) != 0) {
            FI_HILOGE("getgrgid_r failed:%{public}s", strerror(errno));
        } else {
            FI_HILOGD("%{public}20s:%{public}10u%{public}20s", "SUPPLEMENTARY GROUP", groups[i], grp.gr_name);
        }
    }
}
} // namespace DeviceStatus
} // namespace Msdp
} // namespace OHOS
