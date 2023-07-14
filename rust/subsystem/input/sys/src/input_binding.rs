/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

//! rust input binding sys

#![allow(dead_code)]

/// struct CPointerEvent
#[repr(C)]
pub struct CPointerEvent {
    _private: [u8; 0],
}

/// struct CKeyEvent
#[repr(C)]
pub struct CKeyEvent {
    _private: [u8; 0],
}

/// struct CAxisEvent
#[repr(C)]
pub struct CAxisEvent {
    _private: [u8; 0],
}

/// struct CPointerStyleColor
#[repr(C)]
pub struct CPointerStyleColor {
    /// add struct field documentation.
    pub r: u8,
    /// add struct field documentation.
    pub g: u8,
    /// add struct field documentation.
    pub b: u8,
}

/// struct CPointerStyle
#[repr(C)]
pub struct CPointerStyle {
    /// add struct field documentation.
    pub size: i32,
    /// add struct field documentation.
    pub color: CPointerStyleColor,
    /// add struct field documentation.
    pub id: i32,
}

/// struct CExtraData
#[repr(C)]
pub struct CExtraData {
    /// add struct field documentation.
    pub appended: bool,
    /// add struct field documentation.
    pub buffer: *const u8,
    /// add struct field documentation.
    pub buffer_size: usize,
    /// add struct field documentation.
    pub source_type: i32,
    /// add struct field documentation.
    pub pointer_id: i32,
}

// Callback function type for OnPointerEventCallback() from native,
// this callback is invoked when listening for a pointer event.
pub type OnPointerEventCallback = unsafe extern "C" fn(
    event: *const CPointerEvent
);

// C interface for pointer event
extern "C" {
    pub fn CGetPointerId(event: *const CPointerEvent) -> i32;
    pub fn CGetPointerAction(event: *const CPointerEvent) -> i32;
    pub fn CGetTargetWindowId(event: *const CPointerEvent) -> i32;
    pub fn CGetSourceType(event: *const CPointerEvent) -> i32;
    pub fn CGetTargetDisplayId(event: *const CPointerEvent) -> i32;
    pub fn CGetDisplayX(event: *const CPointerEvent) -> i32;
    pub fn CGetDisplayY(event: *const CPointerEvent) -> i32;
    pub fn CPointerEventAddFlag(event: *const CPointerEvent);
    pub fn CGetDeviceId(event: *const CPointerEvent) -> i32;
    pub fn CGetWindowPid(event: *const CPointerEvent) -> i32;
}

// C interface for key event
extern "C" {
    pub fn CKeyEventAddFlag(event: *const CKeyEvent);
    pub fn CGetKeyCode(event: *const CKeyEvent) -> i32;
}

// C interface for input manager
extern "C" {
    pub fn CAddMonitor(callback: OnPointerEventCallback) -> i32;
    pub fn CGetPointerStyle(pointer_style: &mut CPointerStyle) -> i32;
    pub fn CAppendExtraData(extra_data: &CExtraData);
    pub fn CSetPointerVisible(visible: bool) -> i32;
    pub fn CEnableInputDevice(enable: bool) -> i32;
    pub fn CRemoveInputEventFilter(filterId: i32) -> i32;
    pub fn CRemoveMonitor(monitorId: i32);
    pub fn CRemoveInterceptor(interceptorId: i32);
    pub fn CSetPointerLocation(physicalX: i32, physicalY: i32);

    pub fn CDestroyPointerEvent(event: *mut CPointerEvent);
}