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

//! data definitions.

extern crate hilog_rust;
extern crate ipc_rust;
extern crate fusion_utils_rust;

mod errors;
mod ipc;
mod plugin_manager;

pub use errors::{ FusionErrorCode, FusionResult };
pub use ipc::basic::{ BasicParamID, AllocSocketPairParam };
pub use ipc::coordination::{ GeneralCoordinationParam, StartCoordinationParam,
    StopCoordinationParam, GetCoordinationStateParam };
pub use ipc::default::{ CallingContext, DefaultReply };
pub use ipc::drag::{ CDragData, DragData };
pub use plugin_manager::{ Intention, IPlugin };
