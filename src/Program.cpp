#include "Program.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include <cstdio>
#include <cassert>

#include <gtc/matrix_transform.hpp>

#include "BeAssetManager.h"
#include "Renderer.h"

static auto errorCallback(int code, const char* desc) -> void {
    (void)std::fprintf(stderr, "GLFW error %d: %s\n", code, desc);
}

Program::Program() = default;
Program::~Program() = default;

auto Program::run() -> int {

    // window
    int width = 1920, height = 1080;
    
    glfwSetErrorCallback(errorCallback);
    if (!glfwInit()) return -1;

    // No client API: Direct3D will render
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    window = glfwCreateWindow(width, height, "Best Engine Ever", nullptr, nullptr);
    if (!window) { terminate(); return -1; }

    const HWND hwnd = glfwGetWin32Window(window);
    assert(hwnd != nullptr);

    
    // engine
    Renderer renderer(hwnd, width, height);
    renderer.LaunchDevice();
    
    const auto device = renderer.GetDevice();
    BeAssetManager::Ins = std::make_unique<BeAssetManager>(device);

    
    // load assets
    BeAssetManager::Ins->LoadShader("Color", "assets/shaders/default", {
        {.Name = "Position", .Attribute = BeVertexElementDescriptor::BeVertexSemantic::Position},
        {.Name = "Color", .Attribute = BeVertexElementDescriptor::BeVertexSemantic::Color3}
    });
    BeAssetManager::Ins->LoadShader("Textured", "assets/shaders/textured", {
        {.Name = "Position", .Attribute = BeVertexElementDescriptor::BeVertexSemantic::Position},
        {.Name = "UV", .Attribute = BeVertexElementDescriptor::BeVertexSemantic::TexCoord0},
    });
    
    // Compile shaders
    const auto colorShader = &BeAssetManager::Ins->GetShader("Color");
    const auto texturedShader = &BeAssetManager::Ins->GetShader("Textured");

    BeAssetManager::Ins->LoadModel("macintosh", "assets/model.fbx");
    BeAssetManager::Ins->LoadModel("disks", "assets/floppy-disks.glb");
    BeAssetManager::Ins->LoadModel("pagoda", "assets/pagoda.glb");
    BeAssetManager::Ins->LoadModel("witch_items", "assets/witch_items.glb");
    BeAssetManager::Ins->LoadModel("anvil", "assets/lowpoly_pixelart_anvil.glb");
    BeAssetManager::Ins->LoadModel("rock", "assets/lowpoly_rock_1/scene.gltf");
    
    std::vector<Renderer::ObjectEntry> objects = {
        {
            .Name = "Macintosh",
            .Position = {0, 0, -7},
            .Model = &BeAssetManager::Ins->GetModel("macintosh"),
            .Shader = colorShader,
        },
        {
            .Name = "Disks",
            .Position = {7.5f, 1, -4},
            .Rotation = glm::quat(glm::vec3(0, glm::radians(150.f), 0)),
            .Model = &BeAssetManager::Ins->GetModel("disks"),
            .Shader = colorShader,
        },
        {
            .Name = "Pagoda",
            .Position = {0, 0, 8},
            .Scale = glm::vec3(0.2f),
            .Model = &BeAssetManager::Ins->GetModel("pagoda"),
            .Shader = texturedShader,
        },
        {
            .Name = "Witch Items",
            .Position = {-3, 0, 5},
            .Scale = glm::vec3(3.f),
            .Model = &BeAssetManager::Ins->GetModel("witch_items"),
            .Shader = texturedShader,
        },
        {
            .Name = "Anvil",
            .Position = {7, 0, 5},
            //.Rotation = glm::quat(glm::vec3(glm::radians(-90.f), glm::radians(180.f), 0)),
            .Scale = glm::vec3(0.2f),
            .Model = &BeAssetManager::Ins->GetModel("anvil"),
            .Shader = texturedShader,
        },
        {
            .Name = "Rock",
            .Position = {0, 0, 0},
            .Rotation = glm::quat(glm::vec3(glm::radians(-90.f), 0, 0)),
            .Scale = glm::vec3(1.5f),
            .Model = &BeAssetManager::Ins->GetModel("rock"),
            .Shader = texturedShader,
        },
    };

    renderer.PushObjects(objects);

    
    
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
        renderer.SetProjectionView(projectionView);
        
        renderer.Render();
    }
    
    return 0;
}

auto Program::terminate() -> void {
    glfwDestroyWindow(window);
    glfwTerminate();
}
