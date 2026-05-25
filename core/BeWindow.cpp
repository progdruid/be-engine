#include "BeWindow.h"

#include <umbrellas/include-glfw.h>

#include <cstdio>
#include <cassert>
#include <stdexcept>

#include "umbrellas/include-libassert.h"

namespace {
    auto errorCallback(int code, const char* desc) -> void {
        (void)std::fprintf(stderr, "GLFW error %d: %s\n", code, desc);
    }
}

BeWindow::BeWindow(int desiredWidth, int desiredHeight, const std::string& title, BeWindowMode mode) : 
    _window(nullptr), 
    _width(0), 
    _height(0), 
    _framebufferWidth(0), 
    _framebufferHeight(0), 
    _title(title), 
    _mode(mode) 
{
    SetupErrorCallback();

#ifdef __linux__
    glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_X11);
#endif
    if (!glfwInit()) {
        throw std::runtime_error("Failed to initialize GLFW");
    }
    
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_AUTO_ICONIFY, GLFW_FALSE);

    if (mode == BeWindowMode::Fullscreen) {
        GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
        be_assert(primaryMonitor, "Primary monitor invalid");
        const GLFWvidmode* videoMode = glfwGetVideoMode(primaryMonitor);
        be_assert(videoMode, "Video mode invalid");
        
        _window = glfwCreateWindow(videoMode->width, videoMode->height, title.c_str(), primaryMonitor, nullptr);
        be_assert(_window, "Failed to create GLFW window for Fullscreen mode");
        glfwGetWindowSize(_window, &_width, &_height);
        glfwGetFramebufferSize(_window, &_framebufferWidth, &_framebufferHeight);
    } 
    else if (mode == BeWindowMode::BorderlessFullscreen) {
        GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
        be_assert(primaryMonitor, "Primary monitor invalid");
        const GLFWvidmode* videoMode = glfwGetVideoMode(primaryMonitor);
        be_assert(videoMode, "Video mode invalid");
        
        glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
        glfwWindowHint(GLFW_RED_BITS,     videoMode->redBits);
        glfwWindowHint(GLFW_GREEN_BITS,   videoMode->greenBits);
        glfwWindowHint(GLFW_BLUE_BITS,    videoMode->blueBits);
        glfwWindowHint(GLFW_REFRESH_RATE, videoMode->refreshRate);
        
        _window = glfwCreateWindow(videoMode->width, videoMode->height, title.c_str(), primaryMonitor, nullptr);
        be_assert(_window, "Failed to create GLFW window for Borderless Fullscreen mode");
        glfwGetWindowSize(_window, &_width, &_height);
        glfwGetFramebufferSize(_window, &_framebufferWidth, &_framebufferHeight);
        glfwSetWindowPos(_window, 0, 0);
    }
    else if (mode == BeWindowMode::Windowed) {
        _window = glfwCreateWindow(desiredWidth, desiredHeight, title.c_str(), nullptr, nullptr);
        be_assert(_window, "Failed to create GLFW window for Windowed mode");
        glfwGetWindowSize(_window, &_width, &_height);
        glfwGetFramebufferSize(_window, &_framebufferWidth, &_framebufferHeight);
    }
}

BeWindow::~BeWindow() {
    if (_window) {
        glfwDestroyWindow(_window);
    }
    glfwTerminate();
}

BeWindow::BeWindow(BeWindow&& other) noexcept
    : _window(other._window)
    , _width(other._width)
    , _height(other._height)
    , _framebufferWidth(other._framebufferWidth)
    , _framebufferHeight(other._framebufferHeight)
    , _title(std::move(other._title))
    , _mode(other._mode) 
{
    other._window = nullptr;
}

BeWindow& BeWindow::operator=(BeWindow&& other) noexcept {
    if (this != &other) {
        _height = other._height;
        _framebufferWidth = other._framebufferWidth;
        _framebufferHeight = other._framebufferHeight;
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

auto BeWindow::DebugPollMonitors() const -> void {
    int count;
    GLFWmonitor** monitors = glfwGetMonitors(&count);
    GLFWmonitor* primary = glfwGetPrimaryMonitor();

    for (int i = 0; i < count; i++) {
        GLFWmonitor* monitor = monitors[i];
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        const char* name = glfwGetMonitorName(monitor);

        int x, y;
        glfwGetMonitorPos(monitor, &x, &y);

        float xscale, yscale;
        glfwGetMonitorContentScale(monitor, &xscale, &yscale);

        bool isPrimary = (monitor == primary);

        printf("[%d]%s %s — %dx%d @ %dHz, pos=(%d,%d), scale=(%.1f,%.1f)\n",
            i,
            isPrimary ? " [PRIMARY]" : "",
            name,
            mode->width, mode->height, mode->refreshRate,
            x, y,
            xscale, yscale
        );
    }
}

auto BeWindow::DebugPollSizes() const -> void {
    glfwPollEvents();
    float scaleX = 1.f, scaleY = 1.f;
    int windowWidth = 0, windowHeight = 0;
    int framebufferWidth = 0, framebufferHeight = 0;
    int computedFramebufferWidth = 0, computedFramebufferHeight = 0;
    glfwGetWindowSize(_window, &windowWidth, &windowHeight);
    glfwGetWindowContentScale(_window, &scaleX, &scaleY);
    glfwGetFramebufferSize(_window, &framebufferWidth, &framebufferHeight);
    computedFramebufferWidth  = static_cast<int>(static_cast<float>(windowWidth)  * scaleX);
    computedFramebufferHeight = static_cast<int>(static_cast<float>(windowHeight) * scaleY);
    std::fprintf(stderr,
        "[BeWindow] logicalSize=%dx%d  framebuffer=%dx%d  computedFramebuffer=%dx%d  contentScale=%.2fx%.2f\n",
        _width, _height, _framebufferWidth, _framebufferHeight, computedFramebufferWidth, computedFramebufferHeight, scaleX, scaleY
    );
}
