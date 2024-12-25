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

//! Implementation of container containing records of devices.

#![allow(dead_code)]
#![allow(unused_variables)]

use std::cmp::{ Ord, Ordering };
use std::cell::RefCell;
use std::collections::{ BTreeSet, HashMap };
use std::ffi::{ CString, c_char };
use std::rc::{ Rc, Weak };
use hilog_rust::{ hilog, warn, HiLogLabel, LogType };
use fusion_utils_rust::{ call_debug_enter };
use crate::interfaces::{ IDevice, IHotplugHandler, IDeviceManager, IDeviceObserver };
use crate::device::Device;

const LOG_LABEL: HiLogLabel = HiLogLabel {
    log_type: LogType::LogCore,
    domain: 0xD002220,
    tag: "DeviceCollection"
};

struct WeakHolder<T: ?Sized> {
    inner: Weak<RefCell<T>>,
}

impl<T: ?Sized> WeakHolder<T> {
    pub fn new(value: &Rc<RefCell<T>>) -> Self {
        Self {
            inner: Rc::downgrade(value),
        }
    }
}

impl<T: ?Sized> Ord for WeakHolder<T> {
    fn cmp(&self, other: &Self) -> Ordering {
        self.inner.as_ptr().cmp(&other.inner.as_ptr())
    }
}

impl<T: ?Sized> PartialOrd for WeakHolder<T> {
    fn partial_cmp(&self, other: &Self) -> Option<Ordering> {
        Some(self.cmp(other))
    }
}

impl<T: ?Sized> PartialEq for WeakHolder<T> {
    fn eq(&self, other: &Self) -> bool {
        self.inner.ptr_eq(&other.inner)
    }
}

impl<T: ?Sized> Eq for WeakHolder<T> {}

impl<T: ?Sized> From<Weak<RefCell<T>>> for WeakHolder<T> {
    // Required method
    fn from(value: Weak<RefCell<T>>) -> Self {
        Self {
            inner: value,
        }
    }
}

#[derive(Default)]
pub struct DeviceCollection {
    devices: HashMap<i32, Rc<dyn IDevice>>,
    observers: BTreeSet<WeakHolder<dyn IDeviceObserver>>,
}

impl DeviceCollection {
    fn on_device_added(&self, dev: &Rc<Device>) {

    }
}

impl IHotplugHandler for DeviceCollection {
    fn add_device(&mut self, node_name: &str) {
        let dev = Rc::new(
            match Device::new(node_name) {
                Ok(dev) => { dev },
                Err(_) => {
                    return;
                }
            }
        );

        match self.devices.insert(dev.id(), dev.clone()) {
            Some(v) => {
                warn!(LOG_LABEL, "Device {} is updated", dev.id());
            }
            None => {
                self.on_device_added(&dev);
            }
        }
    }

    fn remove_device(&mut self, node_name: &str) {

    }
}

impl IDeviceManager for DeviceCollection {
    fn device(&self, id: i32) -> Option<Rc<dyn IDevice>> {
        None
    }

    fn add_observer(&mut self, observer: &Rc<RefCell<dyn IDeviceObserver>>) {
        if !self.observers.insert(WeakHolder::new(observer)) {
            warn!(LOG_LABEL, "Observer is duplicate");
        }
    }

    fn remove_observer(&mut self, observer: &Rc<RefCell<dyn IDeviceObserver>>) {
        let holder = WeakHolder::new(observer);
        if !self.observers.remove(&holder) {
            warn!(LOG_LABEL, "Not present");
        }
    }

    fn retrigger_hotplug(&self, observer: &Rc<RefCell<dyn IDeviceObserver>>) {
        call_debug_enter!("DeviceCollection::retrigger_hotplug");
        for (id, dev) in &self.devices {
            observer.borrow_mut().on_device_added(dev);
        }
    }
}
