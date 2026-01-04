#pragma once

#include <unordered_map>
#include <umbrellas/include-glm.h>
#include <umbrellas/access-modifiers.hpp>


// ReSharper disable once CppInconsistentNaming
struct GLFWwindow;
class BeWindow;

class BeInput {
    // Allow internal scroll callback to access private members
    hide static auto ScrollCallbackInternal(GLFWwindow* window, double xOffset, double yOffset) -> void;

    // fields //////////////////////////////////////////////////////////////////////////////////////////////////////////
    hide
    GLFWwindow* _window;

    std::unordered_map<int, bool> _currentKeys;
    std::unordered_map<int, bool> _previousKeys;

    std::unordered_map<int, bool> _currentMouseButtons;
    std::unordered_map<int, bool> _previousMouseButtons;

    glm::vec2 _mousePosition;
    glm::vec2 _previousMousePosition;

    glm::vec2 _scrollDelta;

    bool _isMouseCaptured;

    std::unordered_map<int, bool> _currentGamepadButtons;
    std::unordered_map<int, bool> _previousGamepadButtons;

    glm::vec2 _currentGamepadLeftStick;
    glm::vec2 _currentGamepadRightStick;
    float _currentGamepadLeftTrigger;
    float _currentGamepadRightTrigger;

    glm::vec2 _previousGamepadLeftStick;
    glm::vec2 _previousGamepadRightStick;
    float _previousGamepadLeftTrigger;
    float _previousGamepadRightTrigger;

    // lifetime ////////////////////////////////////////////////////////////////////////////////////////////////////////
    expose
    explicit BeInput(GLFWwindow* windowHandle);
    ~BeInput() = default;

    // interface ///////////////////////////////////////////////////////////////////////////////////////////////////////
    expose
    auto Update() -> void;

    [[nodiscard]] auto GetKey(int key) const -> bool;
    [[nodiscard]] auto GetKeyDown(int key) const -> bool;
    [[nodiscard]] auto GetKeyUp(int key) const -> bool;

    [[nodiscard]] auto GetMouseButton(int button) const -> bool;
    [[nodiscard]] auto GetMouseButtonDown(int button) const -> bool;
    [[nodiscard]] auto GetMouseButtonUp(int button) const -> bool;

    [[nodiscard]] auto GetMousePosition() const -> glm::vec2;
    [[nodiscard]] auto GetMouseDelta() const -> glm::vec2;

    [[nodiscard]] auto GetScrollDelta() const -> glm::vec2;

    auto SetMouseCapture(bool capture) -> void;
    [[nodiscard]] auto IsMouseCaptured() const -> bool;

    [[nodiscard]] auto GetGamepadButton(int button) const -> bool;
    [[nodiscard]] auto GetGamepadButtonDown(int button) const -> bool;
    [[nodiscard]] auto GetGamepadButtonUp(int button) const -> bool;

    [[nodiscard]] auto GetGamepadLeftStick() const -> glm::vec2;
    [[nodiscard]] auto GetGamepadRightStick() const -> glm::vec2;
    [[nodiscard]] auto GetGamepadLeftTrigger() const -> float;
    [[nodiscard]] auto GetGamepadRightTrigger() const -> float;

    [[nodiscard]] auto IsGamepadConnected() const -> bool;

    // internal ////////////////////////////////////////////////////////////////////////////////////////////////////////
    hide
    auto UpdateKeyStates() -> void;
    auto UpdateMouseButtonStates() -> void;
    auto UpdateMousePosition() -> void;
    auto UpdateGamepadStates() -> void;
};
