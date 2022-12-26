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

#ifndef INPUT_MANAGER_H
#define INPUT_MANAGER_H

#include <list>
#include <map>
#include <memory>

#include "event_handler.h"
#include "nocopyable.h"

#include "cooperation_message.h"
#include "error_multimodal.h"
#include "i_anr_observer.h"
#include "i_input_device_cooperate_listener.h"
#include "i_input_device_listener.h"
#include "i_input_event_consumer.h"
#include "i_input_event_filter.h"
#include "input_device.h"
#include "key_option.h"
#include "window_info.h"

namespace OHOS {
namespace MMI {
class InputManager {
public:
    /**
     * @brief Obtains an <b>InputManager</b> instance.
     * @return Returns the pointer to the <b>InputManager</b> instance.
     * @since 9
     */
    static InputManager *GetInstance();
    virtual ~InputManager() = default;

    int32_t GetDisplayBindInfo(DisplayBindInfos &infos);
    int32_t SetDisplayBind(int32_t deviceId, int32_t displayId, std::string &msg);

    /**
     * @brief Updates the screen and window information.
     * @param displayGroupInfo Indicates the logical screen information.
     * @since 9
     */
    void UpdateDisplayInfo(const DisplayGroupInfo &displayGroupInfo);

    int32_t AddInputEventFilter(std::shared_ptr<IInputEventFilter> filter, int32_t priority);
    int32_t RemoveInputEventFilter(int32_t filterId);

    /**
     * @brief Sets a consumer for the window input event of the current process.
     * @param inputEventConsumer Indicates the consumer to set. The window input event of the current process
     * will be called back to the consumer object for processing.
     * @since 9
     */
    void SetWindowInputEventConsumer(std::shared_ptr<IInputEventConsumer> inputEventConsumer);

    /**
     * @brief Sets a window input event consumer that runs on the specified thread.
     * @param inputEventConsumer Indicates the consumer to set.
     * @param eventHandler Indicates the thread running the consumer.
     * @since 9
     */
    void SetWindowInputEventConsumer(std::shared_ptr<IInputEventConsumer> inputEventConsumer,
        std::shared_ptr<AppExecFwk::EventHandler> eventHandler);

    /**
     * @brief Subscribes to the key input event that meets a specific condition. When such an event occurs,
     * the <b>callback</b> specified is invoked to process the event.
     * @param keyOption Indicates the condition of the key input event.
     * @param callback Indicates the callback.
     * @return Returns the subscription ID, which uniquely identifies a subscription in the process.
     * If the value is greater than or equal to <b>0</b>,
     * the subscription is successful. Otherwise, the subscription fails.
     * @since 9
     */
    int32_t SubscribeKeyEvent(std::shared_ptr<KeyOption> keyOption,
        std::function<void(std::shared_ptr<KeyEvent>)> callback);

    /**
     * @brief Unsubscribes from a key input event.
     * @param subscriberId Indicates the subscription ID, which is the return value of <b>SubscribeKeyEvent</b>.
     * @return void
     * @since 9
     */
    void UnsubscribeKeyEvent(int32_t subscriberId);

    /**
     * @brief Adds an input event monitor. After such a monitor is added,
     * an input event is copied and distributed to the monitor while being distributed to the original target.
     * @param monitor Indicates the input event monitor. After an input event is generated,
     * the functions of the monitor object will be called.
     * @return Returns the monitor ID, which uniquely identifies a monitor in the process.
     * If the value is greater than or equal to <b>0</b>, the monitor is successfully added. Otherwise,
     * the monitor fails to be added.
     * @since 9
     */
    int32_t AddMonitor(std::function<void(std::shared_ptr<KeyEvent>)> monitor);

    /**
     * @brief Adds an input event monitor. After such a monitor is added,
     * an input event is copied and distributed to the monitor while being distributed to the original target.
     * @param monitor Indicates the input event monitor. After an input event is generated,
     * the functions of the monitor object will be called.
     * @return Returns the monitor ID, which uniquely identifies a monitor in the process.
     * If the value is greater than or equal to <b>0</b>, the monitor is successfully added. Otherwise,
     * the monitor fails to be added.
     * @since 9
     */
    int32_t AddMonitor(std::function<void(std::shared_ptr<PointerEvent>)> monitor);

    /**
     * @brief Adds an input event monitor. After such a monitor is added,
     * an input event is copied and distributed to the monitor while being distributed to the original target.
     * @param monitor Indicates the input event monitor. After an input event is generated,
     * the functions of the monitor object will be called.
     * @return Returns the monitor ID, which uniquely identifies a monitor in the process.
     * If the value is greater than or equal to <b>0</b>, the monitor is successfully added. Otherwise,
     * the monitor fails to be added.
     * @since 9
     */
    int32_t AddMonitor(std::shared_ptr<IInputEventConsumer> monitor);

    /**
     * @brief Removes a monitor.
     * @param monitorId Indicates the monitor ID, which is the return value of <b>AddMonitor</b>.
     * @return void
     * @since 9
     */
    void RemoveMonitor(int32_t monitorId);

    /**
     * @brief Marks that a monitor has consumed a touchscreen input event. After being consumed,
     * the touchscreen input event will not be distributed to the original target.
     * @param monitorId Indicates the monitor ID.
     * @param eventId Indicates the ID of the consumed touchscreen input event.
     * @return void
     * @since 9
     */
    void MarkConsumed(int32_t monitorId, int32_t eventId);

    /**
     * @brief Moves the cursor to the specified position.
     * @param offsetX Indicates the offset on the X axis.
     * @param offsetY Indicates the offset on the Y axis.
     * @return void
     * @since 9
     */
    void MoveMouse(int32_t offsetX, int32_t offsetY);

    /**
     * @brief Adds an input event interceptor. After such an interceptor is added,
     * an input event will be distributed to the interceptor instead of the original target and monitor.
     * @param interceptor Indicates the input event interceptor. After an input event is generated,
     * the functions of the interceptor object will be called.
     * @return Returns the interceptor ID, which uniquely identifies an interceptor in the process.
     * If the value is greater than or equal to <b>0</b>,the interceptor is successfully added. Otherwise,
     * the interceptor fails to be added.
     * @since 9
     */
    int32_t AddInterceptor(std::shared_ptr<IInputEventConsumer> interceptor);
    int32_t AddInterceptor(std::function<void(std::shared_ptr<KeyEvent>)> interceptor);
    int32_t AddInterceptor(std::shared_ptr<IInputEventConsumer> interceptor, int32_t priority, uint32_t deviceTags);

    /**
     * @brief Removes an interceptor.
     * @param interceptorId Indicates the interceptor ID, which is the return value of <b>AddInterceptor</b>.
     * @return void
     * @since 9
     */
    void RemoveInterceptor(int32_t interceptorId);

    /**
     * @brief Simulates a key input event. This event will be distributed and
     * processed in the same way as the event reported by the input device.
     * @param keyEvent Indicates the key input event to simulate.
     * @return void
     * @since 9
     */
    void SimulateInputEvent(std::shared_ptr<KeyEvent> keyEvent);

    /**
     * @brief Simulates a touchpad input event, touchscreen input event, or mouse device input event.
     * This event will be distributed and processed in the same way as the event reported by the input device.
     * @param pointerEvent Indicates the touchpad input event, touchscreen input event,
     * or mouse device input event to simulate.
     * @return void
     * @since 9
     */
    void SimulateInputEvent(std::shared_ptr<PointerEvent> pointerEvent);

    /**
     * @brief Starts listening for an input device event.
     * @param type Indicates the type of the input device event, which is <b>change</b>.
     * @param listener Indicates the listener for the input device event.
     * @return Returns <b>0</b> if success; returns a non-0 value otherwise.
     * @since 9
     */
    int32_t RegisterDevListener(std::string type, std::shared_ptr<IInputDeviceListener> listener);

    /**
     * @brief Stops listening for an input device event.
     * @param type Indicates the type of the input device event, which is <b>change</b>.
     * @param listener Indicates the listener for the input device event.
     * @return Returns <b>0</b> if success; returns a non-0 value otherwise.
     * @since 9
     */
    int32_t UnregisterDevListener(std::string type, std::shared_ptr<IInputDeviceListener> listener = nullptr);

    /**
     * @brief Obtains the information about an input device.
     * @param callback Indicates the callback used to receive the reported data.
     * @return Returns <b>0</b> if success; returns a non-0 value otherwise.
     * @since 9
     */
    int32_t GetDeviceIds(std::function<void(std::vector<int32_t>&)> callback);

    /**
     * @brief Obtains the information about an input device.
     * @param deviceId Indicates the ID of the input device whose information is to be obtained.
     * @param callback Indicates the callback used to receive the reported data.
     * @return Returns <b>0</b> if success; returns a non-0 value otherwise.
     * @since 9
     */
    int32_t GetDevice(int32_t deviceId, std::function<void(std::shared_ptr<InputDevice>)> callback);

    /**
     * @brief Checks whether the specified key codes of an input device are supported.
     * @param deviceId Indicates the ID of the input device.
     * @param keyCodes Indicates the key codes of the input device.
     * @param callback Indicates the callback used to receive the reported data.
     * @return Returns <b>0</b> if success; returns a non-0 value otherwise.
     * @since 9
     */
    int32_t SupportKeys(int32_t deviceId, std::vector<int32_t> keyCodes,
        std::function<void(std::vector<bool>&)> callback);

    /**
     * @brief Sets whether the pointer icon is visible.
     * @param visible Indicates whether the pointer icon is visible. The value <b>true</b> indicates that
     * the pointer icon is visible, and the value <b>false</b> indicates the opposite.
     * @return Returns <b>0</b> if success; returns a non-0 value otherwise.
     * @since 9
     */
    int32_t SetPointerVisible(bool visible);

    /**
     * @brief Checks whether the pointer icon is visible.
     * @return Returns <b>true</b> if the pointer icon is visible; returns <b>false</b> otherwise.
     * @since 9
     */
    bool IsPointerVisible();

    /**
     * @brief Sets the mouse pointer style.
     * @param windowId Indicates the ID of the window for which the mouse pointer style is set.
     * @param pointerStyle Indicates the ID of the mouse pointer style.
     * @return Returns <b>0</b> if the operation is successful; returns an error code otherwise.
     * @since 9
     */
    int32_t SetPointerStyle(int32_t windowId, int32_t pointerStyle);

    /**
     * @brief Obtains the mouse pointer style.
     * @param windowId Indicates the ID of the window for which the mouse pointer style is obtained.
     * @param pointerStyle Indicates the ID of the mouse pointer style.
     * @return Returns <b>0</b> if the operation is successful; returns an error code otherwise.
     * @since 9
     */
    int32_t GetPointerStyle(int32_t windowId, int32_t &pointerStyle);

    /**
     * @brief Sets the mouse pointer speed, which ranges from 1 to 11.
     * @param speed Indicates the mouse pointer speed to set.
     * @return Returns <b>RET_OK</b> if success; returns <b>RET_ERR</b> otherwise.
     * @since 9
     */
    int32_t SetPointerSpeed(int32_t speed);

    /**
     * @brief Obtains the mouse pointer speed.
     * @param speed Indicates the mouse pointer speed to get.
     * @return Returns the mouse pointer speed if the operation is successful; returns <b>RET_ERR</b> otherwise.
     * @since 9
     */
    int32_t GetPointerSpeed(int32_t &speed);

    /**
     * @brief Queries the keyboard type.
     * @param deviceId Indicates the keyboard device ID.
     * @param callback Callback used to return the keyboard type.
     * @return Returns <b>0</b> if success; returns a non-0 value otherwise.
     * @since 9
     */
    int32_t GetKeyboardType(int32_t deviceId, std::function<void(int32_t)> callback);

    /**
     * @brief Sets the observer for events indicating that the application does not respond.
     * @param observer Indicates the observer for events indicating that the application does not respond.
     * @return void
     * @since 9
     */
    void SetAnrObserver(std::shared_ptr<IAnrObserver> observer);

    /**
     * @brief Sets the screen ID corresponding to the specified input device.
     * @param dhid Indicates the ID of the input device.
     * @param screenId Indicates the screen ID corresponding to the input device.
     * @return Returns <b>0</b> if success; returns a non-0 value otherwise.
     */
    int32_t SetInputDevice(const std::string &dhid, const std::string &screenId);

    /**
     * @brief Obtains the enablement status of the specified function key on the keyboard.
     * @param funcKey Indicates the function key. Currently, the following function keys are supported:
     * NUM_LOCK_FUNCTION_KEY
     * CAPS_LOCK_FUNCTION_KEY
     * SCROLL_LOCK_FUNCTION_KEY
     * @return Returns <b>true</b> if the function key is enabled;
     * returns <b>false</b> otherwise.
     */
    bool GetFunctionKeyState(int32_t funcKey);

    /**
     * @brief Sets the enablement status of the specified function key on the keyboard.
     * @param funcKey Indicates the function key. Currently, the following function keys are supported:
     * NUM_LOCK_FUNCTION_KEY
     * CAPS_LOCK_FUNCTION_KEY
     * SCROLL_LOCK_FUNCTION_KEY
     * @param isEnable Indicates the enablement status to set.
     * @return Returns <b>0</b> if success; returns a non-0 value otherwise.
     */
    int32_t SetFunctionKeyState(int32_t funcKey, bool enable);

    /**
     * @brief Sets the absolute coordinate of mouse.
     * @param x Specifies the x coordinate of the mouse to be set.
     * @param y Specifies the y coordinate of the mouse to be set.
     * @return void
     */
    void SetPointerLocation(int32_t x, int32_t y);

    /**
     * @brief 进入捕获模式
     * @param windowId 窗口id.
     * @return 进入捕获模式成功或失败.
     * @since 9
     */
    int32_t EnterCaptureMode(int32_t windowId);

    /**
     * @brief 退出捕获模式
     * @param windowId 窗口id.
     * @return 退出捕获模式成功或失败.
     * @since 9
     */
    int32_t LeaveCaptureMode(int32_t windowId);

private:
    InputManager() = default;
    DISALLOW_COPY_AND_MOVE(InputManager);
    static InputManager *instance_;
};
} // namespace MMI
} // namespace OHOS
#endif // INPUT_MANAGER_H
