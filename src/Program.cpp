#include "Program.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include <cstdio>
#include <cassert>
#include "gtc/matrix_transform.hpp"

#include "Renderer.h"

static auto errorCallback(int code, const char* desc) -> void {
    (void)std::fprintf(stderr, "GLFW error %d: %s\n", code, desc);
}

Program::Program() = default;
Program::~Program() = default;

auto Program::run() -> int {

    int width = 1920, height = 1080;
    
    glfwSetErrorCallback(errorCallback);
    if (!glfwInit()) return -1;

    // No client API: Direct3D will render
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    window = glfwCreateWindow(width, height, "Best Engine Ever", nullptr, nullptr);
    if (!window) { terminate(); return -1; }

    const HWND hwnd = glfwGetWin32Window(window);
    assert(hwnd != nullptr);
    
    Renderer renderer(hwnd, width, height);
    if (!renderer.isActive()) { terminate(); return -1; }

    glm::vec3 cameraPos = {20.0f, 20.0f, 0.0f};
    glm::vec3 cameraDirection = {-1.0f, -1.0f, 1.0f};
    glm::float32 fov = 45.0f;
    const float radius = cameraPos.x; // Distance from origin

    
    
    // Main loop
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        {
            // Rotating camera around the origin
            static float angle = 0.0f;
            angle += 0.01f; // Adjust speed as needed
            cameraPos = glm::vec3(
                radius * sin(angle),
                cameraPos.y, // Keep Y fixed for a simple orbit
                radius * cos(angle)
            );
            cameraDirection = glm::normalize(-cameraPos); // Always look at the origin
        }
        glm::mat4x4 view = glm::lookAtLH(cameraPos, cameraPos + cameraDirection, {0.0f, 1.0f, 0.0f});
        glm::mat4x4 projection = glm::perspectiveFovLH(glm::radians(fov), static_cast<float>(width), static_cast<float>(height), 0.1f, 100.0f);
        glm::mat4x4 projectionView = projection * view;
        renderer.setProjectionView(projectionView);
        
        renderer.render();
    }
    
    return 0;
}

auto Program::terminate() -> void {
    glfwDestroyWindow(window);
    glfwTerminate();
}
