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

use fusion_utils_rust::call_debug_enter;
use crate::interfaces::IDeviceObserver;

/// TODO: add documentation.
#[derive(Default)]
pub struct DeviceObserver {    
}

impl IDeviceObserver for DeviceObserver {
    fn on_device_added(&mut self, dev: &Rc<Device>) {
        call_debug_enter!("DeviceObserver::on_device_added");
    }

    fn on_device_removed(&mut self, dev: &Rc<dyn IDevice>) {
        call_debug_enter!("DeviceObserver::on_device_removed");
    }
}
