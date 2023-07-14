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

use crate::{
    input_binding, input_binding::CPointerStyle
};
use fusion_data_rust::FusionResult;
use std::ffi::{ c_char, CString };
use hilog_rust::{debug, error, hilog, HiLogLabel, LogType};
const LOG_LABEL: HiLogLabel = HiLogLabel {
    log_type: LogType::LogCore,
    domain: 0xD002220,
    tag: "RustPointerStyle"
};

/// struct PointerStyle.
pub struct PointerStyle {
    inner: CPointerStyle,
}

impl Default for PointerStyle {
    fn default() -> Self {
        Self::new()
    }
}

impl PointerStyle {
    /// TODO: add documentation.
    pub fn new() -> Self {
        Self {
            inner: CPointerStyle::default()
        }
    }

    /// TODO: add documentation.
    pub fn pointer_style(&mut self) -> FusionResult<i32> {
        // SAFETY:  no `None` here, cause `CPointerStyle` is valid
        unsafe {
            match input_binding::CGetPointerStyle(&mut self.inner) {
                0 => {
                    debug!(LOG_LABEL, "get pointer style success");
                    Ok(0)
                }
                _ => {
                    error!(LOG_LABEL, "get pointer style failed");
                    Err(-1)
                }
            }
        }
    }
}