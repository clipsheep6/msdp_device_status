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


use std::rc::Rc;
use std::cell::RefCell;
use fusion_data_rust::FusionResult;

/// Interface for handling hotplug events.
pub trait IHotplugHandler {
    fn add_device(&mut self, node_name: &str);
    fn remove_device(&mut self, node_name: &str);
}

/// TODO: add documentation.
pub enum KeyboardType {
    Unknown,
    AlphabeticKeyboard,
    DigitalKeyboard,
    HandWrittenPen,
    RemoteControl,
}

pub trait IDevice {
    fn open(&mut self) -> FusionResult<i32>;
    fn close(&mut self);

    fn id(&self) -> i32;
    fn dev_path(&self) -> &str;
    fn sys_path(&self) -> &str;
    fn name(&self) -> &str;
    fn bus(&self) -> i32;
    fn version(&self) -> i32;
    fn product(&self) -> i32;
    fn vendor(&self) -> i32;
    fn phys(&self) -> &str;
    fn uniq(&self) -> &str;
    fn keyboard_type(&self) -> KeyboardType;
    fn is_pointer_device(&mut self) -> bool;
    fn is_keyboard(&mut self) -> bool;
}

/// TODO: add documentation.
pub trait IDeviceObserver {
    fn on_device_added(&mut self, dev: &Rc<RefCell<dyn IDevice>>);
    fn on_device_removed(&mut self, dev: &Rc<RefCell<dyn IDevice>>);
}

/// Interface of device manager.
pub trait IDeviceManager {
    fn device(&self, id: i32) -> Option<Rc<RefCell<dyn IDevice>>>;
    fn add_observer(&mut self, observer: &Rc<RefCell<dyn IDeviceObserver>>);
    fn remove_observer(&mut self, observer: &Rc<RefCell<dyn IDeviceObserver>>);
    fn retrigger_hotplug(&self, observer: &Rc<RefCell<dyn IDeviceObserver>>);
}
