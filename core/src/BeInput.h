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

    // internal ////////////////////////////////////////////////////////////////////////////////////////////////////////
    hide
    auto UpdateKeyStates() -> void;
    auto UpdateMouseButtonStates() -> void;
    auto UpdateMousePosition() -> void;
};
