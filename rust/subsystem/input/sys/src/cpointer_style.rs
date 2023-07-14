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

//! rust input pointer style

use crate::input_binding::{ CPointerStyle, CPointerStyleColor };

impl Default for CPointerStyle {
    fn default() -> Self {
        Self::new()
    }
}

impl CPointerStyle {
    /// Create a CPointerStyle object
    pub fn new() -> Self {
        Self {
            size: -1, 
            color: { CPointerStyleColor {
                r: 0,
                g: 0,
                b: 0,
            } },
            id: 0,
        }
    }
}