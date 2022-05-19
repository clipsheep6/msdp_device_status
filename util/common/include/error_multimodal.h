/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

#ifndef ERROR_MULTIMODAL_H
#define ERROR_MULTIMODAL_H

#include <errors.h>

namespace OHOS {
namespace MMI {
namespace {
constexpr int32_t ERROR_UNSUPPORT = -2;
constexpr int32_t ARGV_VALID = 2;
}

enum MmiModuleType {
    MODULE_CLIENT = 0x00,
    MODULE_EVENT_SIMULATE = 0x01,
    MODULE_SERVER = 0x02,
    MODULE_UTIL = 0x03,
    MODULE_VIRTUAL_DEVICE = 0x04,
    MODULE_NAPI = 0x05
};
// Error code for client
constexpr ErrCode CLIENT_ERR_OFFSET = ErrCodeOffset(SUBSYS_MULTIMODAINPUT, MODULE_CLIENT);

enum {
    MSG_HANDLER_INIT_FAIL = CLIENT_ERR_OFFSET,  // ��Ϣ�����ʼ��ʧ��
    START_CLI_FAIL,                             // �ͻ�������ʧ��
    EVENT_CONSUM_FAIL,                          // �¼�����ʧ��
    UNKNOW_TOUCH_TYPE,                          // �ͻ��˴���Touchʱ��ʱ���յ��˿ͻ��˷�����λ������
    STRCPY_S_CALLBACK_FAIL,                     // strcpy_s���ش���
    CHECK_PERMISSION_FAIL,                      // APL��Ȩʧ��
};

// Error code for event simulate
constexpr ErrCode EVENT_SIMULATE_ERR_OFFSET = ErrCodeOffset(SUBSYS_MULTIMODAINPUT, MODULE_EVENT_SIMULATE);

enum {
    FILE_OPEN_FAIL = EVENT_SIMULATE_ERR_OFFSET, // �ļ���ʧ��
    STREAM_BUF_READ_FAIL,                       // �������ȡʧ��
    EVENT_REG_FAIL,                             // �¼�ע��ʧ��
    PARAM_INPUT_FAIL,                           // ע��Я����������
    EVENT_DATA_LEN_INPUT_FAIL,                  // ע���¼������ݳ��ȴ���
    TOUCH_CMD_INPUT_FAIL,                       // ע���touch������Ч
    STRSET_SEC_FUN_FAIL,                        // strset��ȫ��������
    DRIVE_PATH_INVALID,                         // ��Ч�������б��ļ�·��
    CMD_PATH_INVALID,                           // ��Ч��ָ��ļ�·��
    CMD_STR_INVALID,                            // ��Чָ���ַ���
};
// Error code for server
constexpr ErrCode SERVER_ERR_OFFSET = ErrCodeOffset(SUBSYS_MULTIMODAINPUT, MODULE_SERVER);

enum {
    MSG_SEND_FAIL = SERVER_ERR_OFFSET,          // ������Ϣʧ��
    UNKNOWN_EVENT,                              // δ֪���¼�
    ERROR_NULL_POINTER,                         // ��ָ��
    WINDOWS_MSG_INIT_FAIL,                      // ���ڹ�������ʼ��ʧ��
    SVR_MSG_HANDLER_INIT_FAIL,                  // ������Ϣ�����ʼ��ʧ��
    INPUT_EVENT_HANDLER_INIT_FAIL,              // �����¼������ʼ��ʧ��
    LIBINPUT_INIT_FAIL,                         // libinput��ʼ��ʧ��
    LIBINPUT_START_FAIL,                        // libinput����ʧ��
    LIBMMI_SVR_START_FAIL,                      // ��ģ��������ʧ��
    LOG_CONFIG_FAIL,                            // log4z����ʧ��
    LOG_START_FAIL,                             // log4z����ʧ��
    PARAM_INPUT_INVALID,                        // ��Ч���������
    INVALID_PARAM,                              // ��Ч�Ĳ���
    SENIOR_INPUT_DEV_INIT_FAIL,                 // �߼������豸��ʼ��ʧ��
    LIBINPUT_DEV_EMPTY,                         // libinput�豸Ϊ��
    REG_EVENT_DISP_FAIL,                        // ע���¼��ɷ�ʧ��
    KEY_EVENT_DISP_FAIL,                        // �����¼��ɷ�ʧ��
    INVAILD_COORDINATE,                         // ��Ч�����꣨�������δ�ҵ����ڣ�
    ILLEGAL_DEV_ID,                             // �Ƿ����豸fd
    DEV_REG_FAIL,                               // �豸ע��ʧ��
    FD_FIND_FAIL,                               // ����fdʧ��
    CONN_BREAK,                                 // ���ӶϿ�
    SOCKET_BUF_FULL,                            // socket ��������
    WAITING_QUEUE_FULL,                         // �ȴ�������
    APP_NOT_RESP,                               // Ӧ�ó�����Ӧ
    MEMCPY_SEC_FUN_FAIL,                        // memcpy��ȫ��������
    LIBINPUT_DEV_NULLPTR,                       // libinput DeviceΪ��
    TOUCH_ID_NO_FIND,                           // δ�ҵ�touchid
    JOYSTICK_EVENT_DISP_FAIL,                   // ҡ�˻��ֱ��¼��ɷ�ʧ��
    TOUCH_EVENT_DISP_FAIL,                      // �������¼��ɷ�ʧ��
    POINT_REG_EVENT_DISP_FAIL,                  // ���ע���¼��ɷ�ʧ��
    POINT_EVENT_DISP_FAIL,                      // ����¼��ɷ�ʧ��
    KEY_EVENT_PKG_FAIL,                         // �����¼���װʧ��
    POINT_EVENT_PKG_FAIL,                       // ����¼���װʧ��
    JOYSTICK_AXIS_EVENT_PKG_FAIL,               // ҡ���ֱ����¼���װʧ��
    JOYSTICK_KEY_EVENT_PKG_FAIL,                // ҡ���ֱ����¼���װʧ��
    SPRINTF_S_SEC_FUN_FAIL,                     // sprintf_s��ȫ��������
    SPCL_REG_EVENT_DISP_FAIL,                   // ����ע���¼��ɷ�ʧ��.
    TABLETPAD_KEY_EVENT_PKG_FAIL,               // ���ذ���¼���װʧ��
    TABLETPAD_KEY_EVENT_DISP_FAIL,              // ���ذ���¼��ɷ�ʧ��
    TABLETPAD_EVENT_PKG_FAIL,                   // ���ذ����¼���װʧ��
    TABLETPAD_EVENT_DISP_FAIL,                  // ���ذ����¼��ɷ�ʧ��
    TABLETTOOL_EVENT_PKG_FAIL,                  // ���ر��¼���װʧ��
    TABLETTOOL_EVENT_DISP_FAIL,                 // ���ر��¼��ɷ�ʧ��
    MULTIDEVICE_SAME_EVENT_MARK,                // ���豸��ͬ�¼����ر�־
    GESTURE_EVENT_PKG_FAIL,                     // GESTURE_SWIPE�¼���װʧ��
    STAT_CALL_FAIL,                             // stat��������ʧ��
    REG_EVENT_PKG_FAIL,                         // ע���¼���װʧ��
    GESTURE_EVENT_DISP_FAIL,                    // gesture swipe�¼��ɷ�ʧ��
    DEV_PARAM_PKG_FAIL,                         // �豸�Ų�����װʧ��
    DEV_ADD_EVENT_PKG_FAIL,                     // �����豸�¼���װʧ��
    DEV_REMOVE_EVENT_PKG_FAIL,                  // ɾ���豸�¼���װʧ��
    ADD_DEVICE_INFO_CALL_FAIL,                  // ����AddDeviceInfo����ʧ��
    TOUCH_EVENT_PKG_FAIL,                       // �������¼���װʧ��
    UNKNOWN_EVENT_PKG_FAIL,                     // δʶ���¼���װʧ��
    MEMSET_SEC_FUN_FAIL,                        // memset��ȫ��������
    DEVICEID_PARAM_PKG_FAIL,                    // �豸�Ų�����װʧ��
    MALLOC_FAIL,                                // mallocʧ��
    SEC_STRCPY_FAIL,                            // ��ȫ����strcpy����
    SASERVICE_INIT_FAIL,                        // SA_Service��ʼ������
    SASERVICE_START_FAIL,                       // SA_Service��������
    SASERVICE_STOP_FAIL,                        // SA_Serviceֹͣ����
    INVALID_RETURN_VALUE,                       // ��Ч�ķ���ֵ
    EPOLL_CTL_FAIL,                             // epoll_ctl����
    EXP_SO_LIBY_INIT_FAIL,                      // ����չģ���ʼ������
    SASERVICE_PERMISSION_FAIL,                  // SA_ServiceȨ�޲���
    SASERVICE_IPC_CALL_FAIL,                    // SA_Service����ʧ��
    STREAMBUFF_OVER_FLOW,                       // ������д�����
    ADD_SESSION_FAIL,                           // ����session����
    MAKE_SHARED_FAIL,                           // make_shared����
    CLEAR_DEAD_SESSION_FAIL,                    // ������Ч��session����
    INIT_SIGNAL_HANDLER_FAIL,                   // ��ʼ��ȫ���źŴ�����ʧ��
    FCNTL_FAIL,                                 // fcntl �������ô���
    PACKET_WRITE_FAIL,                          // д�����ݴ���
    PACKET_READ_FAIL,                           // ��ȡ���ݴ���
    POINTER_DRAW_INIT_FAIL,                     // ��ʼ�������ʧ��
};
// Error code for util
constexpr ErrCode UTIL_ERR_OFFSET = ErrCodeOffset(SUBSYS_MULTIMODAINPUT, MODULE_UTIL);

enum {
    NON_STD_EVENT = UTIL_ERR_OFFSET,            // �Ǳ�׼���¼�
    UNPROC_MSG,                                 // δ�������Ϣ
    UNKNOWN_MSG_ID,                             // δ֪��ϢID
    UNKNOWN_DEV,                                // δ֪�豸
    FILE_READ_FAIL,                             // �ļ���ȡʧ��
    FILE_WRITE_FAIL,                            // �ļ�д��ʧ��
    API_PARAM_TYPE_FAIL,                        // api�������ʹ���
    API_OUT_OF_RANGE,                           // api����ֵ�������巶Χ
    FOCUS_ID_OBTAIN_FAIL,                       // ��ȡfocus_idʧ��
    SOCKET_PATH_INVALID,                        // ��Ч��Socket�ļ�·��
    SOCKET_CREATE_FAIL,                         // Socket����ʧ��
    SOCKET_BIND_FAIL,                           // ����Socketʧ��
    SOCKET_LISTEN_FAIL,                         // ����Socketʧ��
    EPOLL_CREATE_FAIL,                          // EPOLL����ʧ��
    EPOLL_MODIFY_FAIL,                          // �޸�EPOLLʧ��
    STREAM_BUF_WRITE_FAIL,                      // ������д��ʧ��
    SESSION_ADD_FAIL,                           // ����Sessionʧ��
    VAL_NOT_EXP,                                // ֵ������Ԥ��
    MEM_NOT_ENOUGH,                             // û���㹻���ڴ�
    MEM_OUT_OF_BOUNDS,                          // �ڴ�Խ��
    CONN_FAIL,                                  // ��������ʧ��
    SESSION_NOT_FOUND,                          // û���ҵ�session
    FD_ACCEPT_FAIL,                             // ��������ʱfd��Ч
    PID_OBTAIN_FAIL,                            // ��ȡPIDʧ��
    FD_OBTAIN_FAIL,                             // ��ȡFDʧ��
    INVALID_MONITOR_MON                         // ��������ʧ��
};
// Error code for virtual deviceparam
constexpr ErrCode VIRTUAL_DEVICE_ERR_OFFSET = ErrCodeOffset(SUBSYS_MULTIMODAINPUT, MODULE_VIRTUAL_DEVICE);

// Error code for napi
constexpr ErrCode NAPI_ERR_OFFSET = ErrCodeOffset(SUBSYS_MULTIMODAINPUT, MODULE_NAPI);

enum {
    CALL_NAPI_API_ERR = NAPI_ERR_OFFSET
};

enum REGISTER {
    MMI_STANDARD_EVENT_SUCCESS = 1,
    MMI_STANDARD_EVENT_EXIST = 2,
    MMI_STANDARD_EVENT_INVALID_PARAM = -1,
    MMI_STANDARD_EVENT_NOT_EXIST = 3,
};
enum EXCEPTIONTEST {
    SERVICESELFCHECK = 1001,
    MULTIMODALINPUT_EXCEPTION_INJECTION = 1002,
};

enum MMI_SERVICE_STATUS {
    MMI_SERVICE_INVALID = -1, // ��ģ���񲻴��ڣ���ģ��������쳣
    MMI_SERVICE_RUNNING,     // ��ģ������������
};
} // namespace MMI
} // namespace OHOS
#endif // ERROR_MULTIMODAL_H