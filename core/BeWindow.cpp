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
        glfwSetWindowPos(_window, 0, 0);
    }
    else if (mode == BeWindowMode::Windowed) {
        _window = glfwCreateWindow(desiredWidth, desiredHeight, title.c_str(), nullptr, nullptr);
        be_assert(_window, "Failed to create GLFW window for Windowed mode");
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
    , _title(std::move(other._title))
    , _mode(other._mode)
{
    other._window = nullptr;
}

BeWindow& BeWindow::operator=(BeWindow&& other) noexcept {
    if (this != &other) {
        _title = std::move(other._title);
        _mode = other._mode;

        if (_window) {
            glfwDestroyWindow(_window);
        }

        _window = other._window;
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

auto BeWindow::GetReportedLogicalWidth() const -> int {
    int w = 0, h = 0;
    glfwGetWindowSize(_window, &w, &h);
    return w;
}

auto BeWindow::GetReportedLogicalHeight() const -> int {
    int w = 0, h = 0;
    glfwGetWindowSize(_window, &w, &h);
    return h;
}

auto BeWindow::GetReportedPixelWidth() const -> int {
    int w = 0, h = 0;
    glfwGetFramebufferSize(_window, &w, &h);
    return w;
}

auto BeWindow::GetReportedPixelHeight() const -> int {
    int w = 0, h = 0;
    glfwGetFramebufferSize(_window, &w, &h);
    return h;
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
    glfwGetWindowSize(_window, &windowWidth, &windowHeight);
    glfwGetWindowContentScale(_window, &scaleX, &scaleY);
    glfwGetFramebufferSize(_window, &framebufferWidth, &framebufferHeight);
    const int computedFramebufferWidth = static_cast<int>(static_cast<float>(windowWidth) * scaleX);
    const int computedFramebufferHeight = static_cast<int>(static_cast<float>(windowHeight) * scaleY);
    std::fprintf(stderr,
        "[BeWindow] logicalSize=%dx%d  framebuffer=%dx%d  computedFramebuffer=%dx%d  contentScale=%.2fx%.2f\n",
        windowWidth, windowHeight, framebufferWidth, framebufferHeight, computedFramebufferWidth, computedFramebufferHeight, scaleX, scaleY
    );
}
