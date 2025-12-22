#pragma once

#include <string>

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

    // no copy
    BeWindow(const BeWindow&) = delete;
    BeWindow& operator=(const BeWindow&) = delete;

    // move allowed
    BeWindow(BeWindow&&) noexcept;
    BeWindow& operator=(BeWindow&&) noexcept;

    //public interface///////////////////////////////////////////////////////////////////////////////////////

    auto PollEvents() -> void;

    [[nodiscard]] auto ShouldClose() const -> bool;
    [[nodiscard]] auto GetHwnd() const -> HWND;
    [[nodiscard]] auto GetGlfwWindow() const -> GLFWwindow*;
    [[nodiscard]] auto GetWidth() const -> int { return _width; }
    [[nodiscard]] auto GetHeight() const -> int { return _height; }
    [[nodiscard]] auto GetTitle() const -> const std::string& { return _title; }

private:
    //private logic/////////////////////////////////////////////////////////////////////////////////////////////////////
    auto SetupErrorCallback() -> void;
};
