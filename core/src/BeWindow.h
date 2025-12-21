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

    auto pollEvents() -> void;

    [[nodiscard]] auto shouldClose() const -> bool;
    [[nodiscard]] auto getHWND() const -> HWND;
    [[nodiscard]] auto getGLFWWindow() const -> GLFWwindow*;
    [[nodiscard]] auto getWidth() const -> int { return _width; }
    [[nodiscard]] auto getHeight() const -> int { return _height; }
    [[nodiscard]] auto getTitle() const -> const std::string& { return _title; }

private:
    //private logic/////////////////////////////////////////////////////////////////////////////////////////////////////
    auto setupErrorCallback() -> void;
};
