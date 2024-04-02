// Copyright (C) 2023 Huawei Device Co., Ltd.
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

//! Proxy for general functionality of service.

#![allow(dead_code)]

use std::ffi::{c_char, CString};
use std::fs::File;

use fusion_data_rust::{AllocSocketPairParam, BasicParamID, Intention};
use fusion_ipc_client_rust::FusionIpcClient;
use fusion_utils_rust::{call_debug_enter, FusionErrorCode, FusionResult};
use hilog_rust::{debug, hilog, HiLogLabel, LogType};
use ipc::parcel::MsgParcel;

const LOG_LABEL: HiLogLabel = HiLogLabel {
    log_type: LogType::LogCore,
    domain: 0xD002220,
    tag: "FusionBasicClient",
};

/// Definition of proxy for general functionality of service.
#[derive(Default)]
pub struct FusionBasicClient(i32);

impl FusionBasicClient {
    /// Request connection of service via socket.
    pub fn alloc_socket_pair(
        &self,
        param: &AllocSocketPairParam,
        ipc_client: &FusionIpcClient,
    ) -> FusionResult<(File, i32)> {
        call_debug_enter!("FusionBasicClient::alloc_socket_pair");
        let mut reply_parcel = MsgParcel::new();

        debug!(LOG_LABEL, "Call ipc_client::start()");
        ipc_client.control(
            Intention::Basic,
            u32::from(BasicParamID::AllocSocketPair),
            param,
            &mut reply_parcel,
        )?;

        let file = reply_parcel.read_file().or(Err(FusionErrorCode::Fail))?;
        let token_type = reply_parcel.read().or(Err(FusionErrorCode::Fail))?;

        Ok((file, token_type))
    }
}
