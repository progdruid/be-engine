#include "BeWindow.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <glfw/glfw3.h>
#include <glfw/glfw3native.h>

#include <cstdio>
#include <cassert>
#include <stdexcept>

namespace {
    auto errorCallback(int code, const char* desc) -> void {
        (void)std::fprintf(stderr, "GLFW error %d: %s\n", code, desc);
    }
}

BeWindow::BeWindow(int width, int height, const std::string& title, bool fullscreen)
    : _window(nullptr), _hwnd(nullptr), _width(width), _height(height), _title(title), _fullscreen(fullscreen) {

    SetupErrorCallback();

    if (!glfwInit()) {
        throw std::runtime_error("Failed to initialize GLFW");
    }

    // No client API, using DX11 not OpenGL, yknow
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    GLFWmonitor* monitor = nullptr;
    if (fullscreen) {
        monitor = glfwGetPrimaryMonitor();
        if (monitor) {
            const GLFWvidmode* mode = glfwGetVideoMode(monitor);
            if (mode) {
                // Use monitor resolution if width/height are 0 or use provided dimensions
                _width = (width > 0) ? width : mode->width;
                _height = (height > 0) ? height : mode->height;
            }
        }
    }

    _window = glfwCreateWindow(_width, _height, title.c_str(), monitor, nullptr);
    if (!_window) {
        glfwTerminate();
        throw std::runtime_error("Failed to create GLFW window");
    }

    _hwnd = glfwGetWin32Window(_window);
    assert(_hwnd != nullptr);
}

BeWindow::~BeWindow() {
    if (_window) {
        glfwDestroyWindow(_window);
    }
    glfwTerminate();
}

BeWindow::BeWindow(BeWindow&& other) noexcept
    : _window(other._window), _hwnd(other._hwnd), _width(other._width),
      _height(other._height), _title(std::move(other._title)), _fullscreen(other._fullscreen) {
    other._window = nullptr;
    other._hwnd = nullptr;
}

BeWindow& BeWindow::operator=(BeWindow&& other) noexcept {
    if (this != &other) {
        _height = other._height;
        _title = std::move(other._title);
        _fullscreen = other._fullscreen;

        if (_window) {
            glfwDestroyWindow(_window);
        }

        _window = other._window;
        _hwnd = other._hwnd;
        _width = other._width;
        other._window = nullptr;
        other._hwnd = nullptr;
    }
    return *this;
}

auto BeWindow::PollEvents() -> void {
    glfwPollEvents();
}

auto BeWindow::RequestClose() -> void {
    glfwSetWindowShouldClose(_window, GLFW_TRUE);
}

auto BeWindow::ShouldClose() const -> bool {
    return glfwWindowShouldClose(_window);
}

auto BeWindow::GetHwnd() const -> HWND {
    return _hwnd;
}

auto BeWindow::GetGlfwWindow() const -> GLFWwindow* {
    return _window;
}

auto BeWindow::SetupErrorCallback() -> void {
    glfwSetErrorCallback(errorCallback);
}
