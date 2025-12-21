#pragma once

#include <memory>
#include <umbrellas/include-glm.h>
#include <unordered_map>

struct GLFWwindow;
class BeWindow;

class BeInput {
    // Allow internal scroll callback to access private members
    static auto ScrollCallbackInternal(GLFWwindow* window, double xOffset, double yOffset) -> void;

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
    explicit BeInput(const std::shared_ptr<BeWindow>& window);
    ~BeInput() = default;

    //public interface//////////////////////////////////////////////////////////////////////////////////////////////////
    auto Update() -> void;

    // Keyboard
    [[nodiscard]] auto GetKey(int key) const -> bool;
    [[nodiscard]] auto GetKeyDown(int key) const -> bool;
    [[nodiscard]] auto GetKeyUp(int key) const -> bool;

    // Mouse buttons
    [[nodiscard]] auto GetMouseButton(int button) const -> bool;
    [[nodiscard]] auto GetMouseButtonDown(int button) const -> bool;
    [[nodiscard]] auto GetMouseButtonUp(int button) const -> bool;

    // Mouse position and movement
    [[nodiscard]] auto GetMousePosition() const -> glm::vec2;
    [[nodiscard]] auto GetMouseDelta() const -> glm::vec2;

    // Scroll wheel
    [[nodiscard]] auto GetScrollDelta() const -> glm::vec2;

    auto SetMouseCapture(bool capture) -> void;
    [[nodiscard]] auto IsMouseCaptured() const -> bool;

private:
    //private logic/////////////////////////////////////////////////////////////////////////////////////////////////////
    auto UpdateKeyStates() -> void;
    auto UpdateMouseButtonStates() -> void;
    auto UpdateMousePosition() -> void;
};
