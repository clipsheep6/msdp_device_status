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
use std::ffi::{ CString, c_char };
use std::fs::DirEntry;
use std::fmt::{ Display, Formatter, Error };
use std::mem::MaybeUninit;
use std::rc::Rc;
use hilog_rust::{ debug, error, hilog, HiLogLabel, LogType };
use fusion_data_rust::{ DEV_INPUT_PATH };
use fusion_utils_rust::{ call_debug_enter, last_error };
use crate::interfaces::IHotplugHandler;

const LOG_LABEL: HiLogLabel = HiLogLabel {
    log_type: LogType::LogCore,
    domain: 0xD002220,
    tag: "Enumerator"
};

/// TODO: add documentation.
#[derive(Default)]
pub struct Enumerator {
    hotplug: Option<Rc<RefCell<dyn IHotplugHandler>>>,
}

impl Enumerator {
    pub fn set_hotplug_handler(&mut self, hotplug: Rc<RefCell<dyn IHotplugHandler>>) {
        call_debug_enter!("Enumerator::set_hotplug_handler");
        self.hotplug.replace(hotplug);
    }

    pub fn scan_devices(&self) {
        call_debug_enter!("Enumerator::scan_devices");
        match std::fs::read_dir(DEV_INPUT_PATH) {
            Ok(read_dir) => {
                for dent in read_dir {
                    match &dent {
                        Ok(dir_entry) => {
                            unsafe { self.check_dir_entry(dir_entry) };
                        }
                        Err(err) => {
                            error!(LOG_LABEL, "error: {}", err);
                        }
                    }
                }
            }
            Err(err) => {
                error!(LOG_LABEL, "read_dir fail:{}", last_error());
            }
        }
    }

    unsafe fn check_dir_entry(&self, dir_entry: &DirEntry) {
        call_debug_enter!("Enumerator::check_dir_entry");
        let file_name = dir_entry.file_name();
        let node_name = match file_name.to_str() {
            Some(node_name) => { node_name },
            None => {
                error!(LOG_LABEL, "Not valid Unicode: {:?}", dir_entry.file_name());
                return;
            }
        };
        let dev_path = String::from(DEV_INPUT_PATH) + node_name;
        let mut state_buf = MaybeUninit::<libc::stat>::zeroed();

        if libc::stat(dev_path.as_ptr() as *const c_char, state_buf.as_mut_ptr()) != 0 {
            error!(LOG_LABEL, "stat({}) fail: {}", dev_path, last_error());
            return;
        }
        if (state_buf.assume_init_ref().st_mode & libc::S_IFMT) != libc::S_IFCHR {
            debug!(LOG_LABEL, "Not character device: {}", dev_path);
            return;
        }
        self.add_device(node_name);
    }

    fn add_device(&self, node_name: &str) {
    }
}

impl Display for Enumerator {
    fn fmt(&self, f: &mut Formatter<'_>) -> Result<(), Error>
    {
        writeln!(f, "\nEnumerator {{")?;
        writeln!(f, "}}")?;
        Ok(())
    }
}
