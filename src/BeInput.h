#pragma once

#include <GLFW/glfw3.h>
#include <glm.hpp>
#include <unordered_map>

class BeInput {
    // Allow internal scroll callback to access private members
    static auto scrollCallbackInternal(GLFWwindow* window, double xOffset, double yOffset) -> void;

private:
    //fields////////////////////////////////////////////////////////////////////////////////////////////////////////////
    GLFWwindow* _window;

    // Key states
    std::unordered_map<int, bool> _currentKeys;
    std::unordered_map<int, bool> _previousKeys;

    // Mouse button states
    std::unordered_map<int, bool> _currentMouseButtons;
    std::unordered_map<int, bool> _previousMouseButtons;

    // Mouse position
    glm::vec2 _mousePosition;
    glm::vec2 _previousMousePosition;

    // Scroll
    glm::vec2 _scrollDelta;

    // Mouse capture
    bool _isMouseCaptured;

public:
    //initialisation////////////////////////////////////////////////////////////////////////////////////////////////////
    explicit BeInput(GLFWwindow* window);
    ~BeInput() = default;

    //public interface//////////////////////////////////////////////////////////////////////////////////////////////////
    auto update() -> void;

    // Keyboard
    [[nodiscard]] auto getKey(int key) const -> bool;
    [[nodiscard]] auto getKeyDown(int key) const -> bool;
    [[nodiscard]] auto getKeyUp(int key) const -> bool;

    // Mouse buttons
    [[nodiscard]] auto getMouseButton(int button) const -> bool;
    [[nodiscard]] auto getMouseButtonDown(int button) const -> bool;
    [[nodiscard]] auto getMouseButtonUp(int button) const -> bool;

    // Mouse position and movement
    [[nodiscard]] auto getMousePosition() const -> glm::vec2;
    [[nodiscard]] auto getMouseDelta() const -> glm::vec2;

    // Scroll wheel
    [[nodiscard]] auto getScrollDelta() const -> glm::vec2;

    auto setMouseCapture(bool capture) -> void;
    [[nodiscard]] auto isMouseCaptured() const -> bool;

private:
    //private logic/////////////////////////////////////////////////////////////////////////////////////////////////////
    auto updateKeyStates() -> void;
    auto updateMouseButtonStates() -> void;
    auto updateMousePosition() -> void;
};