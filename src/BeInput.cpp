#include "BeInput.h"

auto BeInput::scrollCallbackInternal(GLFWwindow* window, double xOffset, double yOffset) -> void {
    auto* input = static_cast<BeInput*>(glfwGetWindowUserPointer(window));
    if (!input) return;

    input->_scrollDelta.x += static_cast<float>(xOffset);
    input->_scrollDelta.y += static_cast<float>(yOffset);
}

//initialisation////////////////////////////////////////////////////////////////////////////////////////////////////

BeInput::BeInput(GLFWwindow* window)
    : _window(window)
    , _mousePosition(0.0f)
    , _previousMousePosition(0.0f)
    , _scrollDelta(0.0f)
    , _isMouseCaptured(false) {

    glfwSetWindowUserPointer(window, this);
    glfwSetScrollCallback(window, scrollCallbackInternal);

    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    _mousePosition = glm::vec2(static_cast<float>(xpos), static_cast<float>(ypos));
    _previousMousePosition = _mousePosition;
}

//public interface//////////////////////////////////////////////////////////////////////////////////////////////////

auto BeInput::update() -> void {
    _previousKeys = _currentKeys;
    _previousMouseButtons = _currentMouseButtons;
    _previousMousePosition = _mousePosition;

    updateKeyStates();
    updateMouseButtonStates();
    updateMousePosition();

    _scrollDelta = glm::vec2(0.0f);
}

auto BeInput::getKey(int key) const -> bool {
    const auto it = _currentKeys.find(key);
    if (it == _currentKeys.end()) return false;
    return it->second;
}

auto BeInput::getKeyDown(int key) const -> bool {
    const auto currentIt = _currentKeys.find(key);
    const auto previousIt = _previousKeys.find(key);

    const bool currentPressed = (currentIt != _currentKeys.end()) && currentIt->second;
    const bool previousPressed = (previousIt != _previousKeys.end()) && previousIt->second;

    return currentPressed && !previousPressed;
}

auto BeInput::getKeyUp(int key) const -> bool {
    const auto currentIt = _currentKeys.find(key);
    const auto previousIt = _previousKeys.find(key);

    const bool currentPressed = (currentIt != _currentKeys.end()) && currentIt->second;
    const bool previousPressed = (previousIt != _previousKeys.end()) && previousIt->second;

    return !currentPressed && previousPressed;
}

auto BeInput::getMouseButton(int button) const -> bool {
    const auto it = _currentMouseButtons.find(button);
    if (it == _currentMouseButtons.end()) return false;
    return it->second;
}

auto BeInput::getMouseButtonDown(int button) const -> bool {
    const auto currentIt = _currentMouseButtons.find(button);
    const auto previousIt = _previousMouseButtons.find(button);

    const bool currentPressed = (currentIt != _currentMouseButtons.end()) && currentIt->second;
    const bool previousPressed = (previousIt != _previousMouseButtons.end()) && previousIt->second;

    return currentPressed && !previousPressed;
}

auto BeInput::getMouseButtonUp(int button) const -> bool {
    const auto currentIt = _currentMouseButtons.find(button);
    const auto previousIt = _previousMouseButtons.find(button);

    const bool currentPressed = (currentIt != _currentMouseButtons.end()) && currentIt->second;
    const bool previousPressed = (previousIt != _previousMouseButtons.end()) && previousIt->second;

    return !currentPressed && previousPressed;
}

auto BeInput::getMousePosition() const -> glm::vec2 {
    return _mousePosition;
}

auto BeInput::getMouseDelta() const -> glm::vec2 {
    return _mousePosition - _previousMousePosition;
}

auto BeInput::getScrollDelta() const -> glm::vec2 {
    return _scrollDelta;
}

auto BeInput::setMouseCapture(bool capture) -> void {
    if (capture == _isMouseCaptured) return;

    _isMouseCaptured = capture;
    const int cursorMode = capture ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL;
    glfwSetInputMode(_window, GLFW_CURSOR, cursorMode);
}

auto BeInput::isMouseCaptured() const -> bool {
    return _isMouseCaptured;
}

//private logic/////////////////////////////////////////////////////////////////////////////////////////////////////

auto BeInput::updateKeyStates() -> void {
    const int keysToCheck[] = {
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

auto BeInput::updateMouseButtonStates() -> void {
    const int buttonsToCheck[] = {
        GLFW_MOUSE_BUTTON_LEFT,
        GLFW_MOUSE_BUTTON_RIGHT,
        GLFW_MOUSE_BUTTON_MIDDLE,
    };

    for (const int button : buttonsToCheck) {
        const int state = glfwGetMouseButton(_window, button);
        _currentMouseButtons[button] = (state == GLFW_PRESS);
    }
}

auto BeInput::updateMousePosition() -> void {
    double xpos, ypos;
    glfwGetCursorPos(_window, &xpos, &ypos);
    _mousePosition = glm::vec2(static_cast<float>(xpos), static_cast<float>(ypos));
}