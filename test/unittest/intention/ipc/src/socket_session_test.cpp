/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "socket_session_test.h"

#include "ipc_skeleton.h"
#include "message_parcel.h"

#include "devicestatus_define.h"
#include "i_context.h"
#include "i_plugin.h"
#include "socket_client.h"
#include "socket_params.h"
#include "socket_session_manager.h"
#include "socket_server.h"
#include "tunnel_client.h"

namespace OHOS {
namespace Msdp {
namespace DeviceStatus {
using namespace testing::ext;
namespace {
constexpr size_t BUF_CMD_SIZE { 512 };
std::shared_ptr<TunnelClient> g_tunnel {nullptr};
std::unique_ptr<SocketClient> g_client {nullptr};
std::shared_ptr<SocketConnection> g_socket { nullptr };
std::shared_ptr<SocketServer> g_socketServer { nullptr };
std::shared_ptr<SocketSession> g_session { nullptr };
std::shared_ptr<SocketSession> g_sessionOne { nullptr };
std::shared_ptr<SocketSessionManager> g_socketSessionManager { nullptr };
IContext *g_context { nullptr };
Intention g_intention { Intention::UNKNOWN_INTENTION };
constexpr int32_t TIME_WAIT_FOR_OP_MS { 20 };
} // namespace

void SocketSessionTest::SetUpTestCase() {}

void SocketSessionTest::SetUp()
{
    g_tunnel = std::make_shared<TunnelClient>();
    g_client = std::make_unique<SocketClient>(g_tunnel);
    g_socketServer = std::make_unique<SocketServer>(g_context);
    g_socketSessionManager = std::make_shared<SocketSessionManager>();
    int32_t moduleType = 1;
    int32_t tokenType = 1;
    int32_t uid = IPCSkeleton::GetCallingUid();
    int32_t pid = IPCSkeleton::GetCallingPid();
    int32_t sockFds[2] { 0, -1 };
    g_session = std::make_shared<SocketSession>("test", moduleType, tokenType, sockFds[0], uid, pid);
    g_sessionOne = std::make_shared<SocketSession>("test1", moduleType, tokenType, sockFds[1], uid, pid);
}
void SocketSessionTest::TearDown()
{
    g_tunnel = nullptr;
    g_client = nullptr;
    g_socket = nullptr;
    g_socketSessionManager = nullptr;
    g_session = nullptr;
    g_socketServer = nullptr;
    g_sessionOne = nullptr;
    std::this_thread::sleep_for(std::chrono::milliseconds(TIME_WAIT_FOR_OP_MS));
}

/**
 * @tc.name: SocketSessionTest1
 * @tc.desc: Drag Drawing
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(SocketSessionTest, SocketSessionTest1, TestSize.Level0)
{
    CALL_TEST_DEBUG;
    int32_t ret = g_client->Connect();
    EXPECT_TRUE(ret);
    ret = g_client->Connect();
    EXPECT_TRUE(ret);
    g_client->Stop();
    g_client->OnDisconnected();
}

/**
 * @tc.name: SocketSessionTest2
 * @tc.desc: Drag Drawing
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(SocketSessionTest, SocketSessionTest2, TestSize.Level0)
{
    CALL_TEST_DEBUG;
    int32_t ret = g_client->Socket();
    MessageId msgId { MessageId::INVALID };
    NetPacket pkt(msgId);
    g_client->OnPacket(pkt);
    EXPECT_GT(ret, -1);
}

/**
 * @tc.name: SocketSessionTest3
 * @tc.desc: Drag Drawing
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(SocketSessionTest, SocketSessionTest3, TestSize.Level0)
{
    CALL_TEST_DEBUG;
    g_client->Reconnect();
    int32_t ret = g_client->Connect();
    EXPECT_TRUE(ret);
    g_client->Reconnect();
    g_client->OnDisconnected();
}

/**
 * @tc.name: SocketSessionTest4
 * @tc.desc: Drag Drawing
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(SocketSessionTest, SocketSessionTest4, TestSize.Level0)
{
    CALL_TEST_DEBUG;
    MessageParcel data;
    AllocSocketPairParam param;
    bool ret = param.Unmarshalling(data);
    EXPECT_FALSE(ret);
}

/**
 * @tc.name: SocketSessionTest5
 * @tc.desc: Drag Drawing
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(SocketSessionTest, SocketSessionTest5, TestSize.Level0)
{
    CALL_TEST_DEBUG;
    MessageParcel data;
    AllocSocketPairParam param;
    bool ret = param.Marshalling(data);
    EXPECT_TRUE(ret);
}

/**
 * @tc.name: SocketSessionTest6
 * @tc.desc: Drag Drawing
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(SocketSessionTest, SocketSessionTest6, TestSize.Level0)
{
    CALL_TEST_DEBUG;
    MessageParcel data;
    AllocSocketPairReply replyData(1, 1);
    bool ret = replyData.Marshalling(data);
    EXPECT_TRUE(ret);
}

/**
 * @tc.name: SocketSessionTest7
 * @tc.desc: Drag Drawing
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(SocketSessionTest, SocketSessionTest7, TestSize.Level0)
{
    CALL_TEST_DEBUG;
    MessageParcel data;
    AllocSocketPairReply replyData(1, 1);
    bool ret = replyData.Unmarshalling(data);
    EXPECT_FALSE(ret);
}

/**
 * @tc.name: SocketSessionTest8
 * @tc.desc: Drag Drawing
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(SocketSessionTest, SocketSessionTest8, TestSize.Level0)
{
    CALL_TEST_DEBUG;
    CallingContext context {
        .intention = g_intention,
        .tokenId = IPCSkeleton::GetCallingTokenID(),
        .uid = IPCSkeleton::GetCallingUid(),
        .pid = IPCSkeleton::GetCallingPid(),
    };
    MessageParcel datas;
    MessageParcel reply;
    int32_t ret = g_socketServer->Enable(context, datas, reply);
    EXPECT_EQ(ret, RET_ERR);
}

/**
 * @tc.name: SocketSessionTest9
 * @tc.desc: Drag Drawing
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(SocketSessionTest, SocketSessionTest9, TestSize.Level0)
{
    CALL_TEST_DEBUG;
    CallingContext context {
        .intention = g_intention,
        .tokenId = IPCSkeleton::GetCallingTokenID(),
        .uid = IPCSkeleton::GetCallingUid(),
        .pid = IPCSkeleton::GetCallingPid(),
    };
    MessageParcel reply;
    MessageParcel datas;
    int32_t ret = g_socketServer->Disable(context, datas, reply);
    EXPECT_EQ(ret, RET_ERR);
}

/**
 * @tc.name: SocketSessionTest10
 * @tc.desc: Drag Drawing
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(SocketSessionTest, SocketSessionTest10, TestSize.Level0)
{
    CALL_TEST_DEBUG;
    CallingContext context {
        .intention = g_intention,
        .tokenId = IPCSkeleton::GetCallingTokenID(),
        .uid = IPCSkeleton::GetCallingUid(),
        .pid = IPCSkeleton::GetCallingPid(),
    };
    MessageParcel reply;
    MessageParcel datas;
    int32_t ret = g_socketServer->Start(context, datas, reply);
    EXPECT_EQ(ret, RET_ERR);
}

/**
 * @tc.name: SocketSessionTest11
 * @tc.desc: Drag Drawing
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(SocketSessionTest, SocketSessionTest11, TestSize.Level0)
{
    CALL_TEST_DEBUG;
    
    CallingContext context {
        .intention = g_intention,
        .tokenId = IPCSkeleton::GetCallingTokenID(),
        .uid = IPCSkeleton::GetCallingUid(),
        .pid = IPCSkeleton::GetCallingPid(),
    };
    MessageParcel reply;
    MessageParcel datas;
    int32_t ret = g_socketServer->Stop(context, datas, reply);
    EXPECT_EQ(ret, RET_ERR);
}

/**
 * @tc.name: SocketSessionTest12
 * @tc.desc: Drag Drawing
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(SocketSessionTest, SocketSessionTest12, TestSize.Level0)
{
    CALL_TEST_DEBUG;
    CallingContext context {
        .intention = g_intention,
        .tokenId = IPCSkeleton::GetCallingTokenID(),
        .uid = IPCSkeleton::GetCallingUid(),
        .pid = IPCSkeleton::GetCallingPid(),
    };
    MessageParcel reply;
    MessageParcel datas;
    int32_t ret = g_socketServer->AddWatch(context, 1, datas, reply);
    EXPECT_EQ(ret, RET_ERR);
}

/**
 * @tc.name: SocketSessionTest13
 * @tc.desc: Drag Drawing
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(SocketSessionTest, SocketSessionTest13, TestSize.Level0)
{
    CALL_TEST_DEBUG;
    CallingContext context {
        .intention = g_intention,
        .tokenId = IPCSkeleton::GetCallingTokenID(),
        .uid = IPCSkeleton::GetCallingUid(),
        .pid = IPCSkeleton::GetCallingPid(),
    };
    MessageParcel reply;
    MessageParcel datas;
    int32_t ret = g_socketServer->RemoveWatch(context, 1, datas, reply);
    EXPECT_EQ(ret, RET_ERR);
}

/**
 * @tc.name: SocketSessionTest14
 * @tc.desc: Drag Drawing
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(SocketSessionTest, SocketSessionTest14, TestSize.Level0)
{
    CALL_TEST_DEBUG;
    CallingContext context {
        .intention = g_intention,
        .tokenId = IPCSkeleton::GetCallingTokenID(),
        .uid = IPCSkeleton::GetCallingUid(),
        .pid = IPCSkeleton::GetCallingPid(),
    };
    MessageParcel reply;
    MessageParcel datas;
    int32_t ret = g_socketServer->SetParam(context, 1, datas, reply);
    EXPECT_EQ(ret, RET_ERR);
}

/**
 * @tc.name: SocketSessionTest15
 * @tc.desc: Drag Drawing
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(SocketSessionTest, SocketSessionTest15, TestSize.Level0)
{
    CALL_TEST_DEBUG;
    CallingContext context {
        .intention = g_intention,
        .tokenId = IPCSkeleton::GetCallingTokenID(),
        .uid = IPCSkeleton::GetCallingUid(),
        .pid = IPCSkeleton::GetCallingPid(),
    };
    MessageParcel reply;
    MessageParcel datas;
    int32_t ret = g_socketServer->GetParam(context, 1, datas, reply);
    EXPECT_EQ(ret, RET_ERR);
}

/**
 * @tc.name: SocketSessionTest16
 * @tc.desc: Drag Drawing
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(SocketSessionTest, SocketSessionTest16, TestSize.Level0)
{
    CALL_TEST_DEBUG;
    CallingContext context {
        .intention = g_intention,
        .tokenId = IPCSkeleton::GetCallingTokenID(),
        .uid = IPCSkeleton::GetCallingUid(),
        .pid = IPCSkeleton::GetCallingPid(),
    };
    MessageParcel reply;
    MessageParcel datas;
    int32_t ret = g_socketServer->Control(context, 1, datas, reply);
    EXPECT_EQ(ret, RET_ERR);
}

/**
 * @tc.name: SocketSessionTest17
 * @tc.desc: Drag Drawing
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(SocketSessionTest, SocketSessionTest17, TestSize.Level0)
{
    CALL_TEST_DEBUG;
    CallingContext context {
        .intention = g_intention,
        .tokenId = IPCSkeleton::GetCallingTokenID(),
        .uid = IPCSkeleton::GetCallingUid(),
        .pid = IPCSkeleton::GetCallingPid(),
    };
    MessageParcel reply;
    MessageParcel datas;
    int32_t ret = g_socketServer->Control(context, -1, datas, reply);
    EXPECT_EQ(ret, RET_ERR);
}

/**
 * @tc.name: SocketSessionTest18
 * @tc.desc: Drag Drawing
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(SocketSessionTest, SocketSessionTest18, TestSize.Level0)
{
    CALL_TEST_DEBUG;
    g_socketSessionManager->GetAppMgr();
    int32_t ret = g_socketSessionManager->Enable();
    EXPECT_EQ(ret, RET_OK);
}

/**
 * @tc.name: SocketSessionTest19
 * @tc.desc: Drag Drawing
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(SocketSessionTest, SocketSessionTest19, TestSize.Level0)
{
    CALL_TEST_DEBUG;
    int32_t clientFd { -1 };
    int32_t ret = g_socketSessionManager->AllocSocketFd("", 1, 1, 1, 1, clientFd);
    EXPECT_EQ(ret, RET_ERR);
}

/**
 * @tc.name: SocketSessionTest20
 * @tc.desc: Drag Drawing
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(SocketSessionTest, SocketSessionTest20, TestSize.Level0)
{
    CALL_TEST_DEBUG;
    int32_t pid { 1 };
    g_socketSessionManager->FindSessionByPid(pid);
    g_socketSessionManager->DispatchOne();
    int32_t fd { 1 };
    g_socketSessionManager->ReleaseSession(fd);
    g_socketSessionManager->FindSession(fd);
    g_socketSessionManager->DumpSession("");
    g_socketSessionManager->RemoveSessionDeletedCallback(pid);
    int32_t sockFd { -1 };
    int32_t bufSize { -1 };
    int32_t ret = g_socketSessionManager->SetBufferSize(sockFd, bufSize);
    EXPECT_EQ(ret, RET_OK);
}

/**
 * @tc.name: SocketSessionTest21
 * @tc.desc: Drag Drawing
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(SocketSessionTest, SocketSessionTest21, TestSize.Level0)
{
    CALL_TEST_DEBUG;
    int32_t fd { 1 };
    bool ret = g_socketSessionManager->AddSession(g_session);
    g_socketSessionManager->NotifySessionDeleted(g_session);
    EXPECT_FALSE(ret);
    g_socketSessionManager->ReleaseSession(fd);
}

/**
 * @tc.name: SocketSessionTest22
 * @tc.desc: Drag Drawing
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(SocketSessionTest, SocketSessionTest22, TestSize.Level0)
{
    CALL_TEST_DEBUG;
    MessageId msgId { MessageId::INVALID };
    NetPacket pkt(msgId);
    bool ret = g_session->SendMsg(pkt);
    EXPECT_FALSE(ret);
}

/**
 * @tc.name: SocketSessionTest23
 * @tc.desc: Drag Drawing
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(SocketSessionTest, SocketSessionTest23, TestSize.Level0)
{
    CALL_TEST_DEBUG;
    char buf[BUF_CMD_SIZE] = { 0 };
    size_t size = 1;
    ASSERT_NO_FATAL_FAILURE(g_session->SendMsg(buf, size));
}

/**
 * @tc.name: SocketSessionTest24
 * @tc.desc: Drag Drawing
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(SocketSessionTest, SocketSessionTest24, TestSize.Level0)
{
    CALL_TEST_DEBUG;
    char buf[BUF_CMD_SIZE] = { 0 };
    size_t size = 1;
    struct epoll_event ev {};
    ev.events = 0;
    ev.events |= EPOLLIN;
    g_session->Dispatch(ev);
    g_socketSessionManager->Dispatch(ev);

    ev.events = 0;
    ev.events |= EPOLLHUP;
    g_session->Dispatch(ev);
    g_socketSessionManager->Dispatch(ev);
    bool ret = g_session->SendMsg(buf, size);
    g_session->ToString();
    EXPECT_FALSE(ret);
}

/**
 * @tc.name: SocketSessionTest25
 * @tc.desc: Drag Drawing
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(SocketSessionTest, SocketSessionTest25, TestSize.Level0)
{
    CALL_TEST_DEBUG;
    g_socketSessionManager->AddSessionDeletedCallback(1, nullptr);
    g_socketSessionManager->AddSessionDeletedCallback(1, [](SocketSessionPtr ptr){});
    bool ret = g_socketSessionManager->AddSession(nullptr);
    EXPECT_FALSE(ret);
}

/**
 * @tc.name: SocketSessionTest26
 * @tc.desc: Drag Drawing
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(SocketSessionTest, SocketSessionTest26, TestSize.Level0)
{
    CALL_TEST_DEBUG;
    int32_t fd = 12;
    int32_t ret = g_client->Connect();
    EXPECT_TRUE(ret);
    g_client->socket_->OnReadable(fd);
    fd = 1;
    g_client->socket_->OnReadable(fd);
    g_client->socket_->OnException(fd);
    g_client->OnDisconnected();
}

/**
 * @tc.name: SocketSessionTest27
 * @tc.desc: Drag Drawing
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(SocketSessionTest, SocketSessionTest27, TestSize.Level0)
{
    CALL_TEST_DEBUG;
    char buf[BUF_CMD_SIZE] = { 0 };
    size_t size = 0;
    bool ret = g_session->SendMsg(buf, size);
    EXPECT_FALSE(ret);
}

/**
 * @tc.name: SocketSessionTest28
 * @tc.desc: Drag Drawing
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(SocketSessionTest, SocketSessionTest28, TestSize.Level0)
{
    CALL_TEST_DEBUG;
    int32_t fd = g_session->GetFd();
    bool ret = g_socketSessionManager->AddSession(g_session);
    EXPECT_FALSE(ret);
    g_socketSessionManager->ReleaseSession(fd);
}

/**
 * @tc.name: SocketSessionTest29
 * @tc.desc: Drag Drawing
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(SocketSessionTest, SocketSessionTest29, TestSize.Level0)
{
    CALL_TEST_DEBUG;
    g_socketSessionManager->callbacks_.emplace(g_session->GetPid(), [](SocketSessionPtr ptr){});
    g_socketSessionManager->NotifySessionDeleted(g_session);
    int32_t ARG_101 = 101;
    for (size_t i = 0; i < ARG_101; i++) {
        g_socketSessionManager->sessions_.emplace(i, nullptr);
    }
    bool ret = g_socketSessionManager->AddSession(g_session);
    EXPECT_FALSE(ret);
    g_socketSessionManager->sessions_.clear();
    g_socketSessionManager->callbacks_.clear();
}

/**
 * @tc.name: SocketSessionTest30
 * @tc.desc: Drag Drawing
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(SocketSessionTest, SocketSessionTest30, TestSize.Level0)
{
    CALL_TEST_DEBUG;
    char buf[BUF_CMD_SIZE] = { 0 };
    size_t size = 1;
    bool ret = g_sessionOne->SendMsg(buf, size);
    EXPECT_FALSE(ret);
}

/**
 * @tc.name: SocketSessionTest31
 * @tc.desc: Drag Drawing
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(SocketSessionTest, SocketSessionTest31, TestSize.Level0)
{
    CALL_TEST_DEBUG;
    int32_t pid = IPCSkeleton::GetCallingPid();
    ASSERT_NO_FATAL_FAILURE(g_socketSessionManager->ReleaseSessionByPid(pid));
}

/**
 * @tc.name: SocketSessionTest32
 * @tc.desc: Drag Drawing
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(SocketSessionTest, SocketSessionTest32, TestSize.Level0)
{
    CALL_TEST_DEBUG;
    IEpollEventSource *epollEventSource = g_session.get();
    ASSERT_NO_FATAL_FAILURE(g_socketSessionManager->OnEpollIn(*epollEventSource));
    ASSERT_NO_FATAL_FAILURE(g_socketSessionManager->DeleteCollaborationServiceByName());
}
} // namespace DeviceStatus
} // namespace Msdp
} // namespace OHOS
