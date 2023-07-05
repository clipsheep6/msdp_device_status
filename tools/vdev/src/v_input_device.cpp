/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "v_input_device.h"

#include <cstring>
#include <fstream>
#include <regex>
#include <sstream>

#include <fcntl.h>
#include <securec.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "devicestatus_define.h"
#include "devicestatus_errors.h"
#include "fi_log.h"
#include "napi_constants.h"
#include "utility.h"
#include "virtual_device_defines.h"

namespace OHOS {
namespace Msdp {
namespace DeviceStatus {
struct Range {
    size_t start = 0;
    size_t end = 0;
};

namespace {
constexpr ::OHOS::HiviewDFX::HiLogLabel LABEL { LOG_CORE, MSDP_DOMAIN_ID, "VInputDevice" };

const struct Range KEY_BLOCKS[] { { KEY_ESC, BTN_MISC },
    { KEY_OK, BTN_DPAD_UP },
    { KEY_ALS_TOGGLE, BTN_TRIGGER_HAPPY } };
} // namespace

VInputDevice::VInputDevice(const std::string &node) : devPath_(node) {}

VInputDevice::~VInputDevice()
{
    Close();
}

int32_t VInputDevice::Open()
{
    CALL_DEBUG_ENTER;
    char buf[PATH_MAX] {};
    if (realpath(devPath_.c_str(), buf) == nullptr) {
        FI_HILOGE("Not real path: %{public}s", devPath_.c_str());
        return RET_ERR;
    }

    int32_t nRetries = 6;
    for (;;) {
        Utility::ShowUserAndGroup();
        Utility::ShowFileAttributes(buf);
        fd_ = open(buf, O_RDWR | O_NONBLOCK | O_CLOEXEC);
        if (fd_ < 0) {
            FI_HILOGE("Unable to open device \'%{public}s\': %{public}s", buf, strerror(errno));
            if (nRetries-- > 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_TIME));
                FI_HILOGI("Retry opening device \'%{public}s\'", buf);
            } else {
                return RET_ERR;
            }
        } else {
            FI_HILOGD("Opening \'%{public}s\' successfully", buf);
            break;
        }
    }
    QueryDeviceInfo();
    QuerySupportedEvents();
    UpdateCapability();
    return RET_OK;
}

void VInputDevice::Close()
{
    CALL_DEBUG_ENTER;
    if (fd_ >= 0) {
        if (close(fd_) != 0) {
            FI_HILOGE("close error: %{public}s", strerror(errno));
        }
        fd_ = -1;
    }
}

bool VInputDevice::QueryAbsInfo(size_t abs, struct input_absinfo &absInfo)
{
    CALL_DEBUG_ENTER;
    errno_t ret = memset_s(&absInfo, sizeof(absInfo), 0, sizeof(absInfo));
    if (ret != EOK) {
        FI_HILOGE("Call memset_s failed");
        return false;
    }
    return (ioctl(fd_, EVIOCGABS(abs), &absInfo) >= 0);
}

int32_t VInputDevice::SendEvent(uint16_t type, uint16_t code, int32_t value)
{
    CALL_DEBUG_ENTER;
    if (!IsActive()) {
        FI_HILOGE("No active device");
        return RET_ERR;
    }
    struct input_event event {
        .type = type,
        .code = code,
        .value = value
    };
    struct timeval tv;
    if (gettimeofday(&tv, nullptr) != 0) {
        FI_HILOGE("Failed to get current time");
        return RET_ERR;
    }
    event.input_event_sec = tv.tv_sec;
    event.input_event_usec = tv.tv_usec;
    ssize_t ret = ::write(fd_, &event, sizeof(struct input_event));
    if (ret < 0) {
        FI_HILOGE("Failed to send event: %{public}s", strerror(errno));
        return RET_ERR;
    }
    return RET_OK;
}

void VInputDevice::QueryDeviceInfo()
{
    CALL_DEBUG_ENTER;
    char buffer[PATH_MAX] {};

    int32_t rc = ioctl(fd_, EVIOCGNAME(sizeof(buffer) - 1), &buffer);
    if (rc < 0) {
        FI_HILOGE("Could not get device name: %{public}s", strerror(errno));
    } else {
        name_.assign(buffer);
    }

    rc = ioctl(fd_, EVIOCGID, &inputId_);
    if (rc < 0) {
        FI_HILOGE("Could not get device input id: %{public}s", strerror(errno));
    }
    errno_t ret = memset_s(buffer, sizeof(buffer), 0, sizeof(buffer));
    if (ret != EOK) {
        FI_HILOGE("Call memset_s failed");
        return;
    }
    rc = ioctl(fd_, EVIOCGPHYS(sizeof(buffer) - 1), &buffer);
    if (rc < 0) {
        FI_HILOGE("Could not get location: %{public}s", strerror(errno));
    } else {
        phys_.assign(buffer);
    }
    ret = memset_s(buffer, sizeof(buffer), 0, sizeof(buffer));
    if (ret != EOK) {
        FI_HILOGE("Call memset_s failed");
        return;
    }
    rc = ioctl(fd_, EVIOCGUNIQ(sizeof(buffer) - 1), &buffer);
    if (rc < 0) {
        FI_HILOGE("Could not get uniq: %{public}s", strerror(errno));
    } else {
        uniq_.assign(buffer);
    }
}

void VInputDevice::QuerySupportedEvents()
{
    CALL_DEBUG_ENTER;
    int32_t rc = ioctl(fd_, EVIOCGBIT(0, sizeof(evBitmask_)), evBitmask_);
    if (rc < 0) {
        FI_HILOGE("Could not get events mask: %{public}s", strerror(errno));
    }
    rc = ioctl(fd_, EVIOCGBIT(EV_KEY, sizeof(keyBitmask_)), keyBitmask_);
    if (rc < 0) {
        FI_HILOGE("Could not get key events mask: %{public}s", strerror(errno));
    }
    rc = ioctl(fd_, EVIOCGBIT(EV_ABS, sizeof(absBitmask_)), absBitmask_);
    if (rc < 0) {
        FI_HILOGE("Could not get abs events mask: %{public}s", strerror(errno));
    }
    rc = ioctl(fd_, EVIOCGBIT(EV_REL, sizeof(relBitmask_)), relBitmask_);
    if (rc < 0) {
        FI_HILOGE("Could not get rel events mask: %{public}s", strerror(errno));
    }
    rc = ioctl(fd_, EVIOCGBIT(EV_MSC, sizeof(mscBitmask_)), mscBitmask_);
    if (rc < 0) {
        FI_HILOGE("Could not get msc events mask: %{public}s", strerror(errno));
    }
    rc = ioctl(fd_, EVIOCGBIT(EV_LED, sizeof(ledBitmask_)), ledBitmask_);
    if (rc < 0) {
        FI_HILOGE("Could not get led events mask: %{public}s", strerror(errno));
    }
    rc = ioctl(fd_, EVIOCGBIT(EV_REP, sizeof(repBitmask_)), repBitmask_);
    if (rc < 0) {
        FI_HILOGE("Could not get rep events mask: %{public}s", strerror(errno));
    }
    rc = ioctl(fd_, EVIOCGPROP(sizeof(propBitmask_)), propBitmask_);
    if (rc < 0) {
        FI_HILOGE("Could not get properties mask: %{public}s", strerror(errno));
    }
}

void VInputDevice::UpdateCapability()
{
    CALL_DEBUG_ENTER;
    CheckPointers();
    CheckKeys();
}

bool VInputDevice::HasMouseButton() const
{
    for (size_t button = BTN_MOUSE; button < BTN_JOYSTICK; ++button) {
        if (TestBit(button, keyBitmask_)) {
            return true;
        }
    }
    return false;
}

bool VInputDevice::HasJoystickAxesOrButtons() const
{
    if (!TestBit(BTN_JOYSTICK - 1, keyBitmask_)) {
        for (size_t button = BTN_JOYSTICK; button < BTN_DIGI; ++button) {
            if (TestBit(button, keyBitmask_)) {
                return true;
            }
        }
        for (size_t button = BTN_TRIGGER_HAPPY1; button <= BTN_TRIGGER_HAPPY40; ++button) {
            if (TestBit(button, keyBitmask_)) {
                return true;
            }
        }
        for (size_t button = BTN_DPAD_UP; button <= BTN_DPAD_RIGHT; ++button) {
            if (TestBit(button, keyBitmask_)) {
                return true;
            }
        }
    }
    for (size_t axis = ABS_RX; axis < ABS_PRESSURE; ++axis) {
        if (TestBit(axis, absBitmask_)) {
            return true;
        }
    }
    return false;
}

void VInputDevice::CheckPointers()
{
    CALL_DEBUG_ENTER;
    bool hasAbsCoords { TestBit(ABS_X, absBitmask_) && TestBit(ABS_Y, absBitmask_) };
    bool hasMtCoords { TestBit(ABS_MT_POSITION_X, absBitmask_) && TestBit(ABS_MT_POSITION_Y, absBitmask_) };
    bool isDirect { TestBit(INPUT_PROP_DIRECT, propBitmask_) };
    bool hasTouch { TestBit(BTN_TOUCH, keyBitmask_) };
    bool hasRelCoords { TestBit(REL_X, relBitmask_) && TestBit(REL_Y, relBitmask_) };
    bool stylusOrPen { TestBit(BTN_STYLUS, keyBitmask_) || TestBit(BTN_TOOL_PEN, keyBitmask_) };
    bool fingerButNoPen { TestBit(BTN_TOOL_FINGER, keyBitmask_) && !TestBit(BTN_TOOL_PEN, keyBitmask_) };
    bool hasMouseBtn { HasMouseButton() };
    bool hasJoystickFeature { HasJoystickAxesOrButtons() };

    if (hasAbsCoords) {
        if (stylusOrPen) {
            caps_.set(DEVICE_CAP_TABLET_TOOL);
            FI_HILOGD("This is tablet tool");
        } else if (fingerButNoPen && !isDirect) {
            caps_.set(DEVICE_CAP_POINTER);
            FI_HILOGD("This is touchpad");
        } else if (hasMouseBtn) {
            caps_.set(DEVICE_CAP_POINTER);
            FI_HILOGD("This is mouse");
        } else if (hasTouch || isDirect) {
            caps_.set(DEVICE_CAP_TOUCH);
            FI_HILOGD("This is touch device");
        } else if (hasJoystickFeature) {
            caps_.set(DEVICE_CAP_JOYSTICK);
            FI_HILOGD("This is joystick");
        }
    } else if (hasJoystickFeature) {
        caps_.set(DEVICE_CAP_JOYSTICK);
        FI_HILOGD("This is joystick");
    }
    if (hasMtCoords) {
        if (stylusOrPen) {
            caps_.set(DEVICE_CAP_TABLET_TOOL);
            FI_HILOGD("This is tablet tool");
        } else if (fingerButNoPen && !isDirect) {
            caps_.set(DEVICE_CAP_POINTER);
            FI_HILOGD("This is touchpad");
        } else if (hasTouch || isDirect) {
            caps_.set(DEVICE_CAP_TOUCH);
            FI_HILOGD("This is touch device");
        }
    }
    if (!caps_.test(DEVICE_CAP_TABLET_TOOL) &&
        !caps_.test(DEVICE_CAP_POINTER) &&
        !caps_.test(DEVICE_CAP_JOYSTICK) &&
        hasMouseBtn && (hasRelCoords || !hasAbsCoords)) {
        caps_.set(DEVICE_CAP_POINTER);
        FI_HILOGD("This is mouse");
    }
}

void VInputDevice::CheckKeys()
{
    CALL_DEBUG_ENTER;
    if (!TestBit(EV_KEY, evBitmask_)) {
        FI_HILOGD("No EV_KEY capability");
        return;
    }
    for (size_t block = 0U; block < (sizeof(KEY_BLOCKS) / sizeof(struct Range)); ++block) {
        for (size_t key = KEY_BLOCKS[block].start; key < KEY_BLOCKS[block].end; ++key) {
            if (TestBit(key, keyBitmask_)) {
                FI_HILOGD("Found key %{public}zx", key);
                caps_.set(DEVICE_CAP_KEYBOARD);
                return;
            }
        }
    }
}
} // namespace DeviceStatus
} // namespace Msdp
} // namespace