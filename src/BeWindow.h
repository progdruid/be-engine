#pragma once

#include <string>

// Forward declarations to avoid exposing GLFW in public headers
struct GLFWwindow;
struct HWND__;
using HWND = HWND__*;

class BeWindow {
private:
    //fields///////////////////////////////////////////////////////////////////////////////////////
    GLFWwindow* _window;
    HWND _hwnd;
    int _width;
    int _height;
    std::string _title;

public:
    //initialisation///////////////////////////////////////////////////////////////////////////////////////
    explicit BeWindow(int width, int height, const std::string& title = "Window");
    ~BeWindow();

    // Prevent copying
    BeWindow(const BeWindow&) = delete;
    BeWindow& operator=(const BeWindow&) = delete;

    // Allow moving
    BeWindow(BeWindow&&) noexcept;
    BeWindow& operator=(BeWindow&&) noexcept;

    //public interface///////////////////////////////////////////////////////////////////////////////////////

    /// Poll window events (call once per frame)
    auto pollEvents() -> void;

    /// Check if window should close
    [[nodiscard]] auto shouldClose() const -> bool;

    /// Get the native Windows HWND handle for DirectX
    [[nodiscard]] auto getHWND() const -> HWND;

    /// Get the GLFW window pointer (for internal use like BeInput)
    [[nodiscard]] auto getGLFWWindow() const -> GLFWwindow*;

    /// Get window width
    [[nodiscard]] auto getWidth() const -> int { return _width; }

    /// Get window height
    [[nodiscard]] auto getHeight() const -> int { return _height; }

    /// Get window title
    [[nodiscard]] auto getTitle() const -> const std::string& { return _title; }

private:
    //private logic/////////////////////////////////////////////////////////////////////////////////////////////////////
    auto setupErrorCallback() -> void;
};
