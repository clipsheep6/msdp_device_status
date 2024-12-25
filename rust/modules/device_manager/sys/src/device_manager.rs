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

//! Implementation of device manager.

#![allow(dead_code)]
#![allow(unused_variables)]

use std::cell::RefCell;
use std::ffi::{ CString, c_char };
use std::rc::Rc;
use hilog_rust::{ hilog, HiLogLabel, LogType };
use fusion_utils_rust::call_debug_enter;
use crate::interfaces::{ IDeviceManager, IDevice, IDeviceObserver };
use crate::enumerator::Enumerator;
use crate::monitor::Monitor;
use crate::device_collection::DeviceCollection;

const LOG_LABEL: HiLogLabel = HiLogLabel {
    log_type: LogType::LogCore,
    domain: 0xD002220,
    tag: "DeviceManager"
};

/// TODO: add documentation.
#[derive(Default)]
pub struct DeviceManager {
    enumerator: Enumerator,
    monitor: Monitor,
    devices: Rc<RefCell<DeviceCollection>>,
}

impl DeviceManager {
    /// TODO: add documentation.
    pub fn init(&mut self) {
        self.enumerator.set_hotplug_handler(self.devices.clone());
        self.monitor.set_hotplug_handler(self.devices.clone());
    }
}

impl IDeviceManager for DeviceManager {
    fn device(&self, id: i32) -> Option<Rc<dyn IDevice>> {
        self.devices.borrow().device(id)
    }

    fn add_observer(&mut self, observer: &Rc<RefCell<dyn IDeviceObserver>>) {
        call_debug_enter!("DeviceManager::add_observer");
    }

    fn remove_observer(&mut self, observer: &Rc<RefCell<dyn IDeviceObserver>>) {
        call_debug_enter!("DeviceManager::remove_observer");
    }

    fn retrigger_hotplug(&self, observer: &Rc<RefCell<dyn IDeviceObserver>>) {
        call_debug_enter!("DeviceManager::retrigger_hotplug");
    }
}
