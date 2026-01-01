#pragma once

#include <string>
#include <umbrellas/access-modifiers.hpp>

// ReSharper disable once CppInconsistentNaming
struct GLFWwindow;
// ReSharper disable once CppInconsistentNaming
struct HWND__;
// ReSharper disable once CppInconsistentNaming
using HWND = HWND__*;

class BeWindow {
    //fields////////////////////////////////////////////////////////////////////////////////////////////////////////////
    hide
    GLFWwindow* _window;
    HWND _hwnd;
    int _width;
    int _height;
    std::string _title;

    //lifetime//////////////////////////////////////////////////////////////////////////////////////////////////////////
    expose
    explicit BeWindow(
        int width,
        int height,
        const std::string& title = "Window"
    );
    ~BeWindow();

    // no copy
    BeWindow(const BeWindow&) = delete;
    BeWindow& operator=(const BeWindow&) = delete;

    // move allowed
    BeWindow(BeWindow&&) noexcept;
    BeWindow& operator=(BeWindow&&) noexcept;


    //interface/////////////////////////////////////////////////////////////////////////////////////////////////////////
    expose
    auto PollEvents() -> void;

    [[nodiscard]] auto ShouldClose() const -> bool;
    [[nodiscard]] auto GetHwnd() const -> HWND;
    [[nodiscard]] auto GetGlfwWindow() const -> GLFWwindow*;
    [[nodiscard]] auto GetWidth() const -> int { return _width; }
    [[nodiscard]] auto GetHeight() const -> int { return _height; }
    [[nodiscard]] auto GetTitle() const -> const std::string& { return _title; }

    hide auto SetupErrorCallback() -> void;
};
