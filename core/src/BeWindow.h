#pragma once

#include <string>
#include <umbrellas/access-modifiers.hpp>

struct GLFWwindow;

enum class BeWindowMode {
    Windowed,
    Fullscreen,
    BorderlessFullscreen
};

class BeWindow {
    hide
    GLFWwindow* _window;
    void* _nativeHandle;
    int _width;
    int _height;
    std::string _title;
    BeWindowMode _mode;

    expose
    explicit BeWindow(
        int width,
        int height,
        const std::string& title = "Window",
        BeWindowMode mode = BeWindowMode::Windowed
    );
    ~BeWindow();

    BeWindow(const BeWindow&) = delete;
    BeWindow& operator=(const BeWindow&) = delete;
    BeWindow(BeWindow&&) noexcept;
    BeWindow& operator=(BeWindow&&) noexcept;

    expose
    auto PollEvents() -> void;
    auto RequestClose() -> void;

    [[nodiscard]] auto ShouldClose() const -> bool;
    [[nodiscard]] auto GetNativeHandle() const -> void* { return _nativeHandle; }
    [[nodiscard]] auto GetGlfwWindow() const -> GLFWwindow*;
    [[nodiscard]] auto GetWidth() const -> int { return _width; }
    [[nodiscard]] auto GetHeight() const -> int { return _height; }
    [[nodiscard]] auto GetTitle() const -> const std::string& { return _title; }
    [[nodiscard]] auto GetWindowMode() const -> BeWindowMode { return _mode; }

    hide auto SetupErrorCallback() -> void;

    // Platform-specific: returns HWND on Windows, NSWindow* on macOS
    // This is intentionally void* to avoid platform headers in this header.
    [[nodiscard]] auto GetHwnd() const -> void* { return _nativeHandle; }
};
