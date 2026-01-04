#include "BeInput.h"

#include <glfw/glfw3.h>

#include "BeWindow.h"

auto BeInput::ScrollCallbackInternal(GLFWwindow* window, double xOffset, double yOffset) -> void {
    auto* input = static_cast<BeInput*>(glfwGetWindowUserPointer(window));
    if (!input) return;

    input->_scrollDelta.x += static_cast<float>(xOffset);
    input->_scrollDelta.y += static_cast<float>(yOffset);
}


BeInput::BeInput(GLFWwindow* windowHandle)
    : _window(windowHandle)
    , _mousePosition(0.0f)
    , _previousMousePosition(0.0f)
    , _scrollDelta(0.0f)
    , _isMouseCaptured(false)
    , _currentGamepadLeftStick(0.0f)
    , _currentGamepadRightStick(0.0f)
    , _currentGamepadLeftTrigger(0.0f)
    , _currentGamepadRightTrigger(0.0f)
    , _previousGamepadLeftStick(0.0f)
    , _previousGamepadRightStick(0.0f)
    , _previousGamepadLeftTrigger(0.0f)
    , _previousGamepadRightTrigger(0.0f)
{
    glfwSetWindowUserPointer(_window, this);
    glfwSetScrollCallback(_window, ScrollCallbackInternal);

    double xpos, ypos;
    glfwGetCursorPos(_window, &xpos, &ypos);
    _mousePosition = glm::vec2(static_cast<float>(xpos), static_cast<float>(ypos));
    _previousMousePosition = _mousePosition;
}



auto BeInput::Update() -> void {
    _previousKeys = _currentKeys;
    _previousMouseButtons = _currentMouseButtons;
    _previousMousePosition = _mousePosition;
    _previousGamepadButtons = _currentGamepadButtons;
    _previousGamepadLeftStick = _currentGamepadLeftStick;
    _previousGamepadRightStick = _currentGamepadRightStick;
    _previousGamepadLeftTrigger = _currentGamepadLeftTrigger;
    _previousGamepadRightTrigger = _currentGamepadRightTrigger;

    UpdateKeyStates();
    UpdateMouseButtonStates();
    UpdateMousePosition();
    UpdateGamepadStates();

    _scrollDelta = glm::vec2(0.0f);
}

auto BeInput::GetKey(int key) const -> bool {
    const auto it = _currentKeys.find(key);
    if (it == _currentKeys.end()) return false;
    return it->second;
}

auto BeInput::GetKeyDown(int key) const -> bool {
    const auto currentIt = _currentKeys.find(key);
    const auto previousIt = _previousKeys.find(key);

    const bool currentPressed = (currentIt != _currentKeys.end()) && currentIt->second;
    const bool previousPressed = (previousIt != _previousKeys.end()) && previousIt->second;

    return currentPressed && !previousPressed;
}

auto BeInput::GetKeyUp(int key) const -> bool {
    const auto currentIt = _currentKeys.find(key);
    const auto previousIt = _previousKeys.find(key);

    const bool currentPressed = (currentIt != _currentKeys.end()) && currentIt->second;
    const bool previousPressed = (previousIt != _previousKeys.end()) && previousIt->second;

    return !currentPressed && previousPressed;
}

auto BeInput::GetMouseButton(int button) const -> bool {
    const auto it = _currentMouseButtons.find(button);
    if (it == _currentMouseButtons.end()) return false;
    return it->second;
}

auto BeInput::GetMouseButtonDown(int button) const -> bool {
    const auto currentIt = _currentMouseButtons.find(button);
    const auto previousIt = _previousMouseButtons.find(button);

    const bool currentPressed = (currentIt != _currentMouseButtons.end()) && currentIt->second;
    const bool previousPressed = (previousIt != _previousMouseButtons.end()) && previousIt->second;

    return currentPressed && !previousPressed;
}

auto BeInput::GetMouseButtonUp(int button) const -> bool {
    const auto currentIt = _currentMouseButtons.find(button);
    const auto previousIt = _previousMouseButtons.find(button);

    const bool currentPressed = (currentIt != _currentMouseButtons.end()) && currentIt->second;
    const bool previousPressed = (previousIt != _previousMouseButtons.end()) && previousIt->second;

    return !currentPressed && previousPressed;
}

auto BeInput::GetMousePosition() const -> glm::vec2 {
    return _mousePosition;
}

auto BeInput::GetMouseDelta() const -> glm::vec2 {
    return _mousePosition - _previousMousePosition;
}

auto BeInput::GetScrollDelta() const -> glm::vec2 {
    return _scrollDelta;
}

auto BeInput::SetMouseCapture(bool capture) -> void {
    if (capture == _isMouseCaptured) return;

    _isMouseCaptured = capture;
    const int cursorMode = capture ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL;
    glfwSetInputMode(_window, GLFW_CURSOR, cursorMode);
}

auto BeInput::IsMouseCaptured() const -> bool {
    return _isMouseCaptured;
}



auto BeInput::UpdateKeyStates() -> void {
    constexpr int keysToCheck[] = {
        GLFW_KEY_W, GLFW_KEY_A, GLFW_KEY_S, GLFW_KEY_D,
        GLFW_KEY_Q, GLFW_KEY_E,
        GLFW_KEY_SPACE, GLFW_KEY_LEFT_SHIFT, GLFW_KEY_LEFT_CONTROL,
        GLFW_KEY_ESCAPE,
        GLFW_KEY_UP, GLFW_KEY_DOWN, GLFW_KEY_LEFT, GLFW_KEY_RIGHT,
        GLFW_KEY_TAB, GLFW_KEY_ENTER,
        GLFW_KEY_0, GLFW_KEY_1, GLFW_KEY_2, GLFW_KEY_3, GLFW_KEY_4,
        GLFW_KEY_5, GLFW_KEY_6, GLFW_KEY_7, GLFW_KEY_8, GLFW_KEY_9,
    };

    for (const int key : keysToCheck) {
        const int state = glfwGetKey(_window, key);
        _currentKeys[key] = (state == GLFW_PRESS || state == GLFW_REPEAT);
    }
}

auto BeInput::UpdateMouseButtonStates() -> void {
    constexpr int buttonsToCheck[] = {
        GLFW_MOUSE_BUTTON_LEFT,
        GLFW_MOUSE_BUTTON_RIGHT,
        GLFW_MOUSE_BUTTON_MIDDLE,
    };

    for (const int button : buttonsToCheck) {
        const int state = glfwGetMouseButton(_window, button);
        _currentMouseButtons[button] = (state == GLFW_PRESS);
    }
}

auto BeInput::UpdateMousePosition() -> void {
    double xpos, ypos;
    glfwGetCursorPos(_window, &xpos, &ypos);
    _mousePosition = glm::vec2(static_cast<float>(xpos), static_cast<float>(ypos));
}

auto BeInput::UpdateGamepadStates() -> void {
    if (!IsGamepadConnected()) return;

    GLFWgamepadstate state;
    if (!glfwGetGamepadState(GLFW_JOYSTICK_1, &state)) {
        return;
    }

    // Update button states
    constexpr int buttonsToCheck[] = {
        GLFW_GAMEPAD_BUTTON_A,
        GLFW_GAMEPAD_BUTTON_B,
        GLFW_GAMEPAD_BUTTON_X,
        GLFW_GAMEPAD_BUTTON_Y,
        GLFW_GAMEPAD_BUTTON_LEFT_BUMPER,
        GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER,
        GLFW_GAMEPAD_BUTTON_BACK,
        GLFW_GAMEPAD_BUTTON_START,
        GLFW_GAMEPAD_BUTTON_LEFT_THUMB,
        GLFW_GAMEPAD_BUTTON_RIGHT_THUMB,
        GLFW_GAMEPAD_BUTTON_DPAD_UP,
        GLFW_GAMEPAD_BUTTON_DPAD_RIGHT,
        GLFW_GAMEPAD_BUTTON_DPAD_DOWN,
        GLFW_GAMEPAD_BUTTON_DPAD_LEFT,
    };

    for (const int button : buttonsToCheck) {
        _currentGamepadButtons[button] = (state.buttons[button] == GLFW_PRESS);
    }

    // Apply dead zone and update analog sticks
    constexpr float stickDeadZone = 0.15f;
    auto applyDeadZone = [stickDeadZone](float value) -> float {
        if (glm::abs(value) < stickDeadZone) return 0.0f;
        return value;
    };

    _currentGamepadLeftStick.x = applyDeadZone(state.axes[GLFW_GAMEPAD_AXIS_LEFT_X]);
    _currentGamepadLeftStick.y = -applyDeadZone(state.axes[GLFW_GAMEPAD_AXIS_LEFT_Y]);  // Negate Y for standard conventions

    _currentGamepadRightStick.x = applyDeadZone(state.axes[GLFW_GAMEPAD_AXIS_RIGHT_X]);
    _currentGamepadRightStick.y = -applyDeadZone(state.axes[GLFW_GAMEPAD_AXIS_RIGHT_Y]);

    // Update triggers (range 0-1)
    constexpr float triggerDeadZone = 0.1f;
    auto applyTriggerDeadZone = [triggerDeadZone](float value) -> float {
        if (value < triggerDeadZone) return 0.0f;
        return value;
    };

    _currentGamepadLeftTrigger = applyTriggerDeadZone(state.axes[GLFW_GAMEPAD_AXIS_LEFT_TRIGGER]);
    _currentGamepadRightTrigger = applyTriggerDeadZone(state.axes[GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER]);
}

auto BeInput::GetGamepadButton(int button) const -> bool {
    const auto it = _currentGamepadButtons.find(button);
    if (it == _currentGamepadButtons.end()) return false;
    return it->second;
}

auto BeInput::GetGamepadButtonDown(int button) const -> bool {
    const auto currentIt = _currentGamepadButtons.find(button);
    const auto previousIt = _previousGamepadButtons.find(button);

    const bool currentPressed = (currentIt != _currentGamepadButtons.end()) && currentIt->second;
    const bool previousPressed = (previousIt != _previousGamepadButtons.end()) && previousIt->second;

    return currentPressed && !previousPressed;
}

auto BeInput::GetGamepadButtonUp(int button) const -> bool {
    const auto currentIt = _currentGamepadButtons.find(button);
    const auto previousIt = _previousGamepadButtons.find(button);

    const bool currentPressed = (currentIt != _currentGamepadButtons.end()) && currentIt->second;
    const bool previousPressed = (previousIt != _previousGamepadButtons.end()) && previousIt->second;

    return !currentPressed && previousPressed;
}

auto BeInput::GetGamepadLeftStick() const -> glm::vec2 {
    return _currentGamepadLeftStick;
}

auto BeInput::GetGamepadRightStick() const -> glm::vec2 {
    return _currentGamepadRightStick;
}

auto BeInput::GetGamepadLeftTrigger() const -> float {
    return _currentGamepadLeftTrigger;
}

auto BeInput::GetGamepadRightTrigger() const -> float {
    return _currentGamepadRightTrigger;
}

auto BeInput::IsGamepadConnected() const -> bool {
    return glfwJoystickPresent(GLFW_JOYSTICK_1) && glfwJoystickIsGamepad(GLFW_JOYSTICK_1);
}