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

use std::cell::RefCell;
use std::default::Default;
use std::ffi::{ c_int, c_char, c_void, CStr, CString };
use std::fmt::{ Display, Formatter, Error };
use std::rc::Rc;
use hilog_rust::{ debug, error, hilog, warn, HiLogLabel, LogType };
use fusion_data_rust::{ FusionResult, DEV_INPUT_PATH };
use fusion_utils_rust::{ call_debug_enter, last_error };
use crate::interfaces::IHotplugHandler;

const BUFSIZE: usize = std::mem::size_of::<libc::inotify_event>() + libc::FILENAME_MAX as usize + 1;
const MAX_EVENTS_NUM: usize = 20;
const LOG_LABEL: HiLogLabel = HiLogLabel {
    log_type: LogType::LogCore,
    domain: 0xD002220,
    tag: "Monitor"
};

/// TODO: add documentation.
#[derive(Default)]
pub struct Monitor {
    /// TODO: add documentation.
    pub inotify_fd: c_int,
    dev_input_wd: c_int,
    hotplug: Option<Rc<RefCell<dyn IHotplugHandler>>>,
}

impl Monitor {
    /// get refenrance from pointer
    /// # Safety
    /// object pointer is valid
    pub unsafe fn as_ref<'a>(object: *const Self) -> Option<&'a Self>{
        object.as_ref()
    }

    /// get mut refenrance from pointer
    /// # Safety
    /// object pointer is valid
    pub unsafe fn as_mut<'a>(object: *mut Self) -> Option<&'a mut Self>{
        object.as_mut()
    }

    /// TODO: add documentation.
    pub fn new() -> Self {
        Self {
            inotify_fd: -1,
            dev_input_wd: -1,
            hotplug: None,
        }
    }

    /// TODO: add documentation.
    pub fn set_hotplug_handler(&mut self, hotplug: Rc<RefCell<dyn IHotplugHandler>>) {
        call_debug_enter!("Monitor::set_hotplug_handler");
        self.hotplug.replace(hotplug);
    }

    /// TODO: add documentation.
    pub fn enable(&mut self) -> FusionResult<()> {
        call_debug_enter!("Monitor::enable");
        self.open_connection()?;
        match self.enable_receiving() {
            Ok(_) => {
                Ok(())
            }
            Err(err) => {
                self.disable();
                Err(err)
            }
        }
    }

    /// TODO: add documentation.
    pub fn disable(&mut self) {
        call_debug_enter!("Monitor::disable");
        if self.dev_input_wd != -1 {
            let ret = unsafe {
                libc::inotify_rm_watch(self.inotify_fd, self.dev_input_wd)
            };
            if ret != 0 {
                error!(LOG_LABEL, "inotify_rm_watch({}, {}) fail", self.inotify_fd, self.dev_input_wd);
            }
            self.dev_input_wd = -1;
        }
        if self.inotify_fd != -1 {
            unsafe {
                libc::close(self.inotify_fd);
            }
            self.inotify_fd = -1;
        }
    }

    fn open_connection(&mut self) -> FusionResult<()> {
        call_debug_enter!("Monitor::open_connection");
        if self.inotify_fd != -1 {
            debug!(LOG_LABEL, "Inotify has been initialized");
            return Ok(());
        }
        self.inotify_fd = unsafe {
            libc::inotify_init1(libc::IN_NONBLOCK | libc::IN_CLOEXEC)
        };
        if self.inotify_fd != -1 {
            Ok(())
        } else {
            error!(LOG_LABEL, "inotify_init1 fail");
            Err(-1)
        }
    }

    fn enable_receiving(&mut self) -> FusionResult<()> {
        call_debug_enter!("Monitor::enable_receiving");
        let input_path = CString::new(DEV_INPUT_PATH).unwrap();
        self.dev_input_wd = unsafe {
            libc::inotify_add_watch(self.inotify_fd, input_path.as_ptr(), libc::IN_CREATE | libc::IN_DELETE)
        };
        if self.dev_input_wd != -1 {
            Ok(())
        } else {
            error!(LOG_LABEL, "Fail to watch \'{}\', inotify_fd:{}", DEV_INPUT_PATH, self.inotify_fd);
            Err(-1)
        }
    }

    unsafe fn receive_device(&self) -> FusionResult<i32> {
        call_debug_enter!("Monitor::receive_device");
        let mut buf: [c_char; BUFSIZE] = [0; BUFSIZE];
        let num_read = libc::read(self.inotify_fd, buf.as_mut_ptr() as *mut c_void, std::mem::size_of_val(&buf));
        if num_read < 0 {
            error!(LOG_LABEL, "read() fail, inotify_fd:{}, error:{}", self.inotify_fd, last_error());
            return Err(-1);
        }
        if num_read == 0 {
            warn!(LOG_LABEL, "End of file, inotify_fd:{}", self.inotify_fd);
            return Err(-1);
        }
        debug!(LOG_LABEL, "Read {} bytes from inotify events", num_read);
        let mut p = buf.as_ptr();

        while p < buf.as_ptr().add(num_read as usize) {
            let pe = p.cast::<libc::inotify_event>();
            self.handle_inotify_event(pe);
            let event = pe.as_ref().unwrap();
            p = p.add(std::mem::size_of::<libc::inotify_event>() + event.len as usize);
        }
        Ok(0)
    }

    unsafe fn handle_inotify_event(&self, pe: *const libc::inotify_event) {
        let event = pe.as_ref().unwrap();
        if event.len == 0 {
            warn!(LOG_LABEL, "name is none");
            return;
        }
        let pname = pe.add(1) as *const c_char;
        let node_name = match CStr::from_ptr(pname).to_str() {
            Ok(node_name) => { node_name },
            Err(err) => {
                error!(LOG_LABEL, "error: {:?}", @public(err));
                return;
            }
        };

        if (event.mask & libc::IN_CREATE) == libc::IN_CREATE {
            self.add_device(node_name);
        } else if (event.mask & libc::IN_DELETE) == libc::IN_DELETE {
            self.remove_device(node_name);
        }
    }

    fn add_device(&self, node_name: &str) {
        call_debug_enter!("Monitor::add_device");
        match &self.hotplug {
            Some(hotplug) => {
                hotplug.borrow_mut().add_device(node_name);
            }
            None => {
                debug!(LOG_LABEL, "Device manager is none");
            }
        }
    }

    fn remove_device(&self, node_name: &str) {
        call_debug_enter!("Monitor::remove_device");
        match &self.hotplug {
            Some(hotplug) => {
                hotplug.borrow_mut().remove_device(node_name);
            }
            None => {
                debug!(LOG_LABEL, "Device manager is none");
            }
        }
    }
}

impl Display for Monitor {
    fn fmt(&self, f: &mut Formatter<'_>) -> Result<(), Error>
    {
        writeln!(f, "\nMonitor {{")?;
        writeln!(f, "  inotify_fd: {},", self.inotify_fd)?;
        writeln!(f, "  dev_input_wd: {},", self.dev_input_wd)?;
        writeln!(f, "}}")?;
        Ok(())
    }
}
