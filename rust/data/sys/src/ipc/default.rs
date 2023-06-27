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

//! IPC data definitions of DRAG module.

#![allow(dead_code)]
#![allow(unused_variables)]

use std::ffi::{ c_char, CString };
use crate::hilog_rust::{ info, hilog, HiLogLabel, LogType };
use crate::ipc_rust::{
    BorrowedMsgParcel, Serialize, Deserialize, IpcResult,
    get_calling_token_id, get_calling_uid, get_calling_pid
};
use crate::fusion_utils_rust::call_debug_enter;

const LOG_LABEL: HiLogLabel = HiLogLabel {
    log_type: LogType::LogCore,
    domain: 0xD002220,
    tag: "FusionIpcDefaultData"
};

/// struct CallingContext
pub struct CallingContext {
    calling_uid: u64,
    calling_pid: u64,
    calling_token_id: u64,
}

impl CallingContext {
    /// TODO: add documentation.
    pub fn current() -> Self {
        info!(LOG_LABEL, "in CallingContext::current(): assemble current calling context");
        Self {
            calling_uid: get_calling_uid(),
            calling_pid: get_calling_pid(),
            calling_token_id: get_calling_token_id()
        }
    }
}

/// struct DefaultReply
pub struct DefaultReply {
    /// TODO: add documentation.
    pub reply: i32,
}

impl Serialize for DefaultReply {
    fn serialize(&self, parcel: &mut BorrowedMsgParcel<'_>) -> IpcResult<()> {
        call_debug_enter!("DefaultReply::serialize");
        self.reply.serialize(parcel)?;
        Ok(())
    }
}

impl Deserialize for DefaultReply {
    fn deserialize(parcel: &BorrowedMsgParcel<'_>) -> IpcResult<Self> {
        call_debug_enter!("DefaultReply::deserialize");
        let reply = i32::deserialize(parcel)?;
        Ok(DefaultReply {
            reply
        })
    }
}
