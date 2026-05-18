#include "BeWindow.h"

#include <umbrellas/include-glfw.h>

#include <cstdio>
#include <cassert>
#include <stdexcept>

namespace {
    auto errorCallback(int code, const char* desc) -> void {
        (void)std::fprintf(stderr, "GLFW error %d: %s\n", code, desc);
    }
}

BeWindow::BeWindow(int width, int height, const std::string& title, BeWindowMode mode)
    : _window(nullptr), _width(width), _height(height), _title(title), _mode(mode) {

    SetupErrorCallback();

    if (!glfwInit()) {
        throw std::runtime_error("Failed to initialize GLFW");
    }

    // No client API, using DX11 not OpenGL, yknow
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    GLFWmonitor* monitor = nullptr;
    GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();

    if (mode == BeWindowMode::Fullscreen) {
        // Exclusive fullscreen
        monitor = primaryMonitor;
        if (primaryMonitor) {
            const GLFWvidmode* videoMode = glfwGetVideoMode(primaryMonitor);
            if (videoMode) {
                _width = (width > 0) ? width : videoMode->width;
                _height = (height > 0) ? height : videoMode->height;
            }
        }
    } else if (mode == BeWindowMode::BorderlessFullscreen) {
        // Borderless windowed fullscreen
        glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
        if (primaryMonitor) {
            const GLFWvidmode* videoMode = glfwGetVideoMode(primaryMonitor);
            if (videoMode) {
                _width = videoMode->width;
                _height = videoMode->height;
            }
        }
    }

    _window = glfwCreateWindow(_width, _height, title.c_str(), monitor, nullptr);
    if (!_window) {
        glfwTerminate();
        throw std::runtime_error("Failed to create GLFW window");
    }

    // Position borderless fullscreen window at (0, 0)
    if (mode == BeWindowMode::BorderlessFullscreen) {
        glfwSetWindowPos(_window, 0, 0);
    }

}

BeWindow::~BeWindow() {
    if (_window) {
        glfwDestroyWindow(_window);
    }
    glfwTerminate();
}

BeWindow::BeWindow(BeWindow&& other) noexcept
    : _window(other._window), _width(other._width),
      _height(other._height), _title(std::move(other._title)), _mode(other._mode) {
    other._window = nullptr;
}

BeWindow& BeWindow::operator=(BeWindow&& other) noexcept {
    if (this != &other) {
        _height = other._height;
        _title = std::move(other._title);
        _mode = other._mode;

        if (_window) {
            glfwDestroyWindow(_window);
        }

        _window = other._window;
        _width = other._width;
        other._window = nullptr;
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

auto BeWindow::GetGlfwWindow() const -> GLFWwindow* {
    return _window;
}

auto BeWindow::SetupErrorCallback() -> void {
    glfwSetErrorCallback(errorCallback);
}
