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

BeWindow::BeWindow(int width, int height, const std::string& title)
    : _window(nullptr), _hwnd(nullptr), _width(width), _height(height), _title(title) {

    SetupErrorCallback();

    if (!glfwInit()) {
        throw std::runtime_error("Failed to initialize GLFW");
    }

    // No client API, using DX11 not OpenGL, yknow
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    _window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
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
      _height(other._height), _title(std::move(other._title)) {
    other._window = nullptr;
    other._hwnd = nullptr;
}

BeWindow& BeWindow::operator=(BeWindow&& other) noexcept {
    if (this != &other) {
        _height = other._height;
        _title = std::move(other._title);

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
