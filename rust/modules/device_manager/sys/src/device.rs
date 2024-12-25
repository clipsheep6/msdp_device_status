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

//! TODO: add documentation.

#![allow(dead_code)]
#![allow(unused_variables)]

use std::os::fd::RawFd;
use std::ffi::{ c_char, c_int, c_long, CStr, CString };
use std::fmt::{ Display, Formatter, Error };
use std::mem::MaybeUninit;
use std::time::Duration;
use hilog_rust::{ debug, error, hilog, info, warn, HiLogLabel, LogType };
use fusion_data_rust::{ FusionResult, FusionErrorCode, DEV_INPUT_PATH };
use fusion_utils_rust::{ call_debug_enter, errno, last_error };
use crate::interfaces::{ IDevice, KeyboardType };
use crate::linux_input::{ eviocgname, eviocgphys, eviocguniq, eviocgprop, eviocgbit, eviocgid };

const DEFAULT_WAIT_TIME: u64 = 500;
const LOG_LABEL: HiLogLabel = HiLogLabel {
    log_type: LogType::LogCore,
    domain: 0xD002220,
    tag: "FusionDevice"
};

const BITS_PER_UINT8: usize = 8usize;
const EV_KEY: usize = 0x01;
const EV_REL: usize = 0x02;
const EV_ABS: usize = 0x03;
const SPLIT_SYMBOL: &str = "|";
const EMPTYSTR: &str = "" ;
const RET_ERR: i32 = -1 ;
const RET_OK: i32 = 0 ;

const fn bit_offset(bit: usize) -> usize
{
    bit % BITS_PER_UINT8
}

const fn byte_offset(bit: usize) -> usize
{
    bit / BITS_PER_UINT8
}

const fn test_bit(bit: usize, array: &[u8]) -> bool
{
    if byte_offset(bit) < array.len() {
        (array[byte_offset(bit)] & (1u8 << bit_offset(bit))) == (1u8 << bit_offset(bit))
    } else {
        false
    }
}

fn set_bit(bit: usize, array: &mut [u8])
{
    if byte_offset(bit) < array.len() {
        array[byte_offset(bit)] |= 1u8 << bit_offset(bit);
    }
}

const fn nbytes(nbits: usize) -> usize
{
    (nbits + BITS_PER_UINT8 - 1) / BITS_PER_UINT8
}

/// DeviceCapability
pub enum DeviceCapability {
    /// Keyboard
    Keyboard,
    /// Pointer
    Pointer,
    /// Touch
    Touch,
    /// TabletTool
    TabletTool,
    /// TabletPad
    TabletPad,
    /// Gesture
    Gesture,
    /// Switch
    Switch,
    /// Joystick
    Joystick,
}

pub const N_DEVICE_CAPABILITIES: usize = 8;

impl From<DeviceCapability> for usize {
    fn from(value: DeviceCapability) -> Self {
        match value {
            DeviceCapability::Keyboard => { 0usize },
            DeviceCapability::Pointer => { 1usize },
            DeviceCapability::Touch => { 2usize },
            DeviceCapability::TabletTool => { 3usize },
            DeviceCapability::TabletPad => { 4usize },
            DeviceCapability::Gesture => { 5usize },
            DeviceCapability::Switch => { 6usize },
            DeviceCapability::Joystick => { 7usize },
        }
    }
}

impl TryFrom<usize> for DeviceCapability {
    type Error = FusionErrorCode;

    fn try_from(value: usize) -> Result<Self, Self::Error> {
        match value {
            _ if usize::from(DeviceCapability::Keyboard) == value => { Ok(DeviceCapability::Keyboard) },
            _ if usize::from(DeviceCapability::Pointer) == value => { Ok(DeviceCapability::Pointer) },
            _ if usize::from(DeviceCapability::Touch) == value => { Ok(DeviceCapability::Touch) },
            _ if usize::from(DeviceCapability::TabletTool) == value => { Ok(DeviceCapability::TabletTool) },
            _ if usize::from(DeviceCapability::TabletPad) == value => { Ok(DeviceCapability::TabletPad) },
            _ if usize::from(DeviceCapability::Gesture) == value => { Ok(DeviceCapability::Gesture) },
            _ if usize::from(DeviceCapability::Switch) == value => { Ok(DeviceCapability::Switch) },
            _ if usize::from(DeviceCapability::Joystick) == value => { Ok(DeviceCapability::Joystick) },
            _ => { Err(FusionErrorCode::Fail) },
        }
    }
}

/// TODO: add documentation.
pub struct Device {
    fd: RawFd,
    dev_id: i32,
    bus: i32,
    version: i32,
    product: i32,
    vendor: i32,
    dev_path: String,
    sys_path: String,
    name: String,
    phys: String,
    uniq: String,
    caps: [u8; nbytes(N_DEVICE_CAPABILITIES)],
    ev_bitmask: [u8; nbytes(libc::EV_MAX as usize)],
    key_bitmask: [u8; nbytes(libc::KEY_MAX as usize)],
    abs_bitmask: [u8; nbytes(libc::ABS_MAX as usize)],
    rel_bitmask: [u8; nbytes(libc::REL_MAX as usize)],
    prop_bitmask: [u8; nbytes(libc::INPUT_PROP_MAX as usize)],
}

impl Device {
    fn is_character_device(dev_path: &str) -> bool {
        false
    }

    fn parse_device_id(node_name: &str) -> FusionResult<i32> {
        call_debug_enter!("Device::parse_device_id");
        Err(-1)
    }

    fn check_node_path(node_name: &str) -> FusionResult<String> {
        let dev_path = String::from(DEV_INPUT_PATH) + node_name;
        if Self::is_character_device(&dev_path) {
            Ok(dev_path)
        } else {
            warn!(LOG_LABEL, "Not character device: {}", dev_path);
            Err(-1)
        }
    }

    fn check_system_path(node_name: &str) -> FusionResult<String> {
        Err(-1)
    }

    /// TODO: add documentation.
    pub fn new(node_name: &str) -> FusionResult<Self> {
        let dev_id = Self::parse_device_id(node_name)?;
        let dev_path = Self::check_node_path(node_name)?;
        let sys_path = Self::check_system_path(node_name)?;

        Ok(Self {
            fd: -1,
            dev_id,
            bus: 0,
            version: 0,
            product: 0,
            vendor: 0,
            dev_path,
            sys_path,
            name: String::default(),
            phys: String::default(),
            uniq: String::default(),
            caps: [0; nbytes(N_DEVICE_CAPABILITIES)],
            ev_bitmask: [0; nbytes(libc::EV_MAX as usize)],
            key_bitmask: [0; nbytes(libc::KEY_MAX as usize)],
            abs_bitmask: [0; nbytes(libc::ABS_MAX as usize)],
            rel_bitmask: [0; nbytes(libc::REL_MAX as usize)],
            prop_bitmask: [0; nbytes(libc::INPUT_PROP_MAX as usize)],
        })
    }

    fn query_device_info(&mut self) {
        call_debug_enter!("Device::query_device_info");
        type Buffer = [c_char; libc::PATH_MAX as usize];
        let mut buffer = MaybeUninit::<Buffer>::zeroed();
        let mut zeroed_buffer = unsafe { buffer.assume_init_mut() };
        let mut rc = unsafe {
            libc::ioctl(self.fd, eviocgname(std::mem::size_of_val(zeroed_buffer) - 1usize) as c_long,
                zeroed_buffer.as_mut_ptr())
        };
        if rc < 0 {
            error!(LOG_LABEL, "DeviceError in query_device_info(): Getting device name failed");
        } else {
            match unsafe { CStr::from_ptr(zeroed_buffer.as_ptr()) }.to_str() {
                Ok(s) => {
                    self.name = s.to_string();
                }
                Err(err) => {
                    error!(LOG_LABEL,
                        "DeviceError in query_device_info(): Converting device name to string type failed");
                }
            }
        }

        let mut input_id=libc::input_id{ bustype: 0, vendor: 0, product: 0, version: 0 };
        rc = unsafe {
            libc::ioctl(self.fd, eviocgid() as c_long, &mut input_id)
        };
        if rc < 0 {
            error!(LOG_LABEL, "Getting input id fail");
        } else {
            self.bus = input_id.bustype as i32;
            self.product = input_id.product as i32;
            self.vendor = input_id.vendor as i32;
            self.version = input_id.version as i32;
        }

        buffer = MaybeUninit::<Buffer>::zeroed();
        zeroed_buffer = unsafe { buffer.assume_init_mut() };
        rc = unsafe {
            libc::ioctl(self.fd, eviocgphys(std::mem::size_of_val(zeroed_buffer) - 1usize) as c_long,
                zeroed_buffer.as_mut_ptr())
        };
        if rc < 0 {
            error!(LOG_LABEL, "DeviceError in query_device_info(): Getting device location failed");
        } else {
            match unsafe { CStr::from_ptr(zeroed_buffer.as_ptr()) }.to_str() {
                Ok(s) => {
                    self.phys = s.to_string();
                }
                Err(err) => {
                    error!(LOG_LABEL,
                        "DeviceError in query_device_info(): Converting device location to string type failed");
                }
            }
        }

        buffer = MaybeUninit::<Buffer>::zeroed();
        zeroed_buffer = unsafe { buffer.assume_init_mut() };
        rc = unsafe {
            libc::ioctl(self.fd, eviocguniq(std::mem::size_of_val(zeroed_buffer) - 1usize) as c_int,
                zeroed_buffer.as_mut_ptr())
        };
        if rc < 0 {
            error!(LOG_LABEL, "DeviceError in query_device_info(): Getting device uniq failed");
        } else {
            match unsafe { CStr::from_ptr(zeroed_buffer.as_ptr()) }.to_str() {
                Ok(s) => {
                    self.uniq = s.to_string();
                }
                Err(err) => {
                    error!(LOG_LABEL,
                        "DeviceError in query_device_info(): Converting device uniq to string type failed");
                }
            }
        }
    }

    /// TODO: add documentation.
    fn query_supported_events(&mut self) {
        call_debug_enter!("Device::query_supported_events");
        let mut rc = unsafe {
            libc::ioctl(self.fd, eviocgbit(0, std::mem::size_of_val(&self.ev_bitmask)) as c_long, &self.ev_bitmask)
        };
        if rc < 0 {
            error!(LOG_LABEL, "Getting events mask fail");
        }
        rc = unsafe {
            libc::ioctl(self.fd, eviocgbit(EV_KEY, std::mem::size_of_val(&self.key_bitmask)) as c_long, &self.key_bitmask)
        };
        if rc < 0 {
            error!(LOG_LABEL, "Getting key mask fail");
        }
        rc = unsafe {
            libc::ioctl(self.fd, eviocgbit(EV_ABS, std::mem::size_of_val(&self.abs_bitmask)) as c_long, &self.abs_bitmask)
        };
        if rc < 0 {
            error!(LOG_LABEL, "Getting abs mask fail");
        }
        rc = unsafe {
            libc::ioctl(self.fd, eviocgbit(EV_REL, std::mem::size_of_val(&self.rel_bitmask)) as c_long, &self.rel_bitmask)
        };
        if rc < 0 {
            error!(LOG_LABEL, "Getting rel mask fail");
        }
        rc = unsafe {
            libc::ioctl(self.fd, eviocgprop(std::mem::size_of_val(&self.prop_bitmask)) as c_long, &self.prop_bitmask)
        };
        if rc < 0 {
            error!(LOG_LABEL, "Getting properties mask fail");
        }
    }

    fn has_event_type(&self, event_type: usize) -> bool {
        test_bit(event_type, &self.ev_bitmask)
    }

    fn has_key(&self, key: usize) -> bool {
        test_bit(key, &self.key_bitmask)
    }

    fn has_abs(&self, axis: usize) -> bool {
        test_bit(axis, &self.abs_bitmask)
    }

    fn has_rel(&self, axis: usize) -> bool {
        test_bit(axis, &self.rel_bitmask)
    }

    fn has_property(&self, prop: usize) -> bool {
        test_bit(prop, &self.prop_bitmask)
    }
}

impl IDevice for Device {
    fn open(&mut self) -> FusionResult<i32> {
        call_debug_enter!("Device::open");
        let mut n_retries: i32 = 6;

        loop {
            self.fd = unsafe {
                 libc::open(self.dev_path.as_ptr() as *const c_char, libc::O_RDWR | libc::O_NONBLOCK | libc::O_CLOEXEC)
            };
            if self.fd < 0 {
                error!(LOG_LABEL, "Unable to open device \'{}\':{}", self.dev_path, last_error());
                if (errno() != libc::ENOENT) && (n_retries > 0) {
                    n_retries -= 1;
                    const DEFAULT_WAIT_TIME: u64 = 500;
                    std::thread::sleep(Duration::from_millis(DEFAULT_WAIT_TIME));
                    info!(LOG_LABEL, "Retry opening device \'{}\'", self.dev_path);
                } else {
                    return Err(-1);
                }
            } else {
                debug!(LOG_LABEL, "Opening \'{}\' successfully", self.dev_path);
                break;
            }
        }
        Ok(0)
    }

    fn close(&mut self) {

    }

    fn id(&self) -> i32 {
        self.dev_id
    }

    fn dev_path(&self) -> &str {
        &self.dev_path
    }

    fn sys_path(&self) -> &str {
        &self.sys_path
    }

    fn name(&self) -> &str {
        &self.name
    }

    fn bus(&self) -> i32 {
        self.bus
    }

    fn version(&self) -> i32 {
        self.version
    }

    fn product(&self) -> i32 {
        self.product
    }

    fn vendor(&self) -> i32 {
        self.vendor
    }

    fn phys(&self) -> &str {
        &self.phys
    }

    fn uniq(&self) -> &str {
        &self.uniq
    }

    fn keyboard_type(&self) -> KeyboardType {
        KeyboardType::Unknown
    }

    fn is_pointer_device(&self) -> bool {
        false
    }

    fn is_keyboard(&self) -> bool {
        false
    }
}

impl Display for Device {
    fn fmt(&self, f: &mut Formatter<'_>) -> Result<(), Error>
    {
        writeln!(f, "\nDevice {{")?;
        writeln!(f, "  fd: {},", self.fd)?;
        writeln!(f, "  dev_id: {},", self.dev_id)?;
        writeln!(f, "  bus: {},", self.bus)?;
        writeln!(f, "  version: {},", self.version)?;
        writeln!(f, "  product: {},", self.product)?;
        writeln!(f, "  vendor: {},", self.vendor)?;
        writeln!(f, "  dev_path: {},", self.dev_path)?;
        writeln!(f, "  sys_path: {},", self.sys_path)?;
        writeln!(f, "  name: {},", self.name)?;
        writeln!(f, "  phys: {},", self.phys)?;
        writeln!(f, "  uniq: {},", self.uniq)?;
        writeln!(f, "  caps: {:?},", self.caps)?;
        writeln!(f, "  ev_bitmask: {:?},", self.ev_bitmask)?;
        writeln!(f, "  key_bitmask: {:?},", self.key_bitmask)?;
        writeln!(f, "  abs_bitmask: {:?},", self.abs_bitmask)?;
        writeln!(f, "  rel_bitmask: {:?},", self.rel_bitmask)?;
        writeln!(f, "  prop_bitmask: {:?},", self.prop_bitmask)?;
        writeln!(f, "}}")?;
        Ok(())
    }
}
