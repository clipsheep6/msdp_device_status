/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
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

#ifndef DEVICESTATUS_PROTO_H
#define DEVICESTATUS_PROTO_H

#include <sys/types.h>

inline constexpr int32_t MAX_EVENT_SIZE { 100 };
inline constexpr int32_t STREAM_BUF_READ_FAIL { 1 };
inline constexpr int32_t STREAM_BUF_WRITE_FAIL { 2 };
inline constexpr int32_t MAX_VECTOR_SIZE { 10 };
inline constexpr int32_t MEM_OUT_OF_BOUNDS { 3 };
inline constexpr int32_t MEMCPY_SEC_FUN_FAIL { 4 };
inline constexpr int32_t PARAM_INPUT_INVALID { 5 };
inline constexpr int32_t MAX_STREAM_BUF_SIZE { 1024 };
inline constexpr size_t MAX_PACKET_BUF_SIZE { MAX_STREAM_BUF_SIZE };
inline constexpr int32_t ONCE_PROCESS_NETPACKET_LIMIT { 100 };
inline constexpr int32_t INVALID_FD { 6 };
inline constexpr int32_t INVALID_PID { 7 };
inline constexpr int32_t SESSION_NOT_FOUND { 8 };
inline constexpr int32_t EPOLL_MODIFY_FAIL { 9 };
inline constexpr int32_t ADD_SESSION_FAIL { 11 };
inline constexpr size_t MAX_SESSION_ALARM { 100 };
inline constexpr int32_t MAX_RECV_LIMIT { 13 };
inline constexpr int32_t SERVICE_NOT_RUNNING { 14 };
inline constexpr int32_t CONNECT_MODULE_TYPE_FI_CLIENT { 0 };
inline constexpr int64_t CLIENT_RECONNECT_COOLING_TIME { 800 };
inline constexpr int32_t SEND_RETRY_LIMIT { 32 };
inline constexpr useconds_t SEND_RETRY_SLEEP_TIME { 10000 };

enum class MessageId : int32_t {
    INVALID,

    COORDINATION_ADD_LISTENER,
    COORDINATION_MESSAGE,
    COORDINATION_GET_STATE,
    HOT_AREA_ADD_LISTENER,
    MOUSE_LOCATION_ADD_LISTENER,

    DSOFTBUS_START_COOPERATE,
    DSOFTBUS_START_COOPERATE_RESPONSE,
    DSOFTBUS_START_COOPERATE_FINISHED,
    DSOFTBUS_STOP_COOPERATE,
    DSOFTBUS_COME_BACK,
    DSOFTBUS_RELAY_COOPERATE,
    DSOFTBUS_RELAY_COOPERATE_FINISHED,
    DSOFTBUS_INPUT_POINTER_EVENT,
    DSOFTBUS_INPUT_KEY_EVENT,
    DSOFTBUS_SUBSCRIBE_MOUSE_LOCATION,
    DSOFTBUS_UNSUBSCRIBE_MOUSE_LOCATION,
    DSOFTBUS_REPLY_SUBSCRIBE_MOUSE_LOCATION,
    DSOFTBUS_REPLY_UNSUBSCRIBE_MOUSE_LOCATION,
    DSOFTBUS_MOUSE_LOCATION,

    DRAG_NOTIFY_RESULT,
    DRAG_STATE_LISTENER,
    DRAG_NOTIFY_HIDE_ICON,
    DRAG_STYLE_LISTENER,
    DSOFTBUS_INPUT_DEV_HOT_PLUG,
    DSOFTBUS_INPUT_DEV_SYNC,
    DSOFTBUS_HEART_BEAT_PACKET,
    MAX_MESSAGE_ID,
    ADD_SELECTED_PIXELMAP_RESULT,
    DSOFTBUS_COME_BACK_WITH_OPTIONS,
    DSOFTBUS_COOPERATE_WITH_OPTIONS
};

enum TokenType : int32_t {
    TOKEN_INVALID = -1,
    TOKEN_HAP = 0,
    TOKEN_NATIVE,
    TOKEN_SHELL
};
#endif // DEVICESTATUS_PROTO_H
