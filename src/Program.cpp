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


// <AI> 
struct FlyCamera {
    glm::vec3 pos{0.0f, 2.0f, 6.0f};
    float yaw{-90.0f};   // degrees, -Z forward
    float pitch{0.0f};   // degrees
    float fov{90.0f};
    float moveSpeed{5.0f};   // units/sec
    float mouseSens{0.1f};   // deg per pixel
    glm::vec3 front{0,0,-1};
    glm::vec3 up{0,1,0};
    glm::vec3 right{1,0,0};
    void updateVectors() {
        const float cy = cos(glm::radians(yaw));
        const float sy = sin(glm::radians(yaw));
        const float cp = cos(glm::radians(pitch));
        const float sp = sin(glm::radians(pitch));
        front = glm::normalize(glm::vec3(cy*cp, sp, sy*cp));
        right = glm::normalize(glm::cross(front, glm::vec3(0,1,0)));
        up    = glm::normalize(glm::cross(right, front));
    }
    void applyZoom(float yoff) {
        // scroll up (positive yoff) -> zoom in (smaller fov)
        fov -= yoff;                           // 1:1 steps feel good; scale if needed
        fov = glm::clamp(fov, 20.0f, 90.0f);   // clamp range
    }
};

static bool GMouseCaptured = false;
static double GLastX = 0.0, GLastY = 0.0;
static bool GFirstMouse = true;

// Optional: toggle capture with RMB
static void MouseButtonCb(GLFWwindow* w, int button, int action, int mods) {
    if (action == GLFW_PRESS) {
        GMouseCaptured = true;
        glfwSetInputMode(w, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        GFirstMouse = true;
    }
    else if (action == GLFW_RELEASE) {
        GMouseCaptured = false;
        glfwSetInputMode(w, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
}

// Cursor callback to accumulate yaw/pitch when captured
static void CursorPosCb(GLFWwindow* w, double xpos, double ypos) {
    if (!GMouseCaptured) { GFirstMouse = true; return; }
    if (GFirstMouse) { GLastX = xpos; GLastY = ypos; GFirstMouse = false; return; }
    double xoffset = GLastX - xpos; // invert X so moving right is +yaw;
    double yoffset = GLastY - ypos; // invert Y so moving up is +pitch
    GLastX = xpos; GLastY = ypos;

    FlyCamera* cam = static_cast<FlyCamera*>(glfwGetWindowUserPointer(w));
    cam->yaw   += static_cast<float>(xoffset) * cam->mouseSens;
    cam->pitch += static_cast<float>(yoffset) * cam->mouseSens;
    cam->pitch = glm::clamp(cam->pitch, -89.0f, 89.0f);
    cam->updateVectors();
}

static void scrollCB(GLFWwindow* w, double xoffset, double yoffset) {
    FlyCamera* cam = static_cast<FlyCamera*>(glfwGetWindowUserPointer(w));
    if (!cam) return;
    cam->applyZoom(static_cast<float>(yoffset));
}

// </AI>


static auto errorCallback(int code, const char* desc) -> void {
    (void)std::fprintf(stderr, "GLFW error %d: %s\n", code, desc);
}

Program::Program() = default;
Program::~Program() = default;


auto Program::run() -> int {

    // window
    constexpr int width = 1920, height = 1080;
    
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
        {.Name = "Normal", .Attribute = BeVertexElementDescriptor::BeVertexSemantic::Normal},
        {.Name = "Color", .Attribute = BeVertexElementDescriptor::BeVertexSemantic::Color3}
    });
    BeAssetManager::Ins->LoadShader("Textured", "assets/shaders/textured", {
        {.Name = "Position", .Attribute = BeVertexElementDescriptor::BeVertexSemantic::Position},
        {.Name = "Normal", .Attribute = BeVertexElementDescriptor::BeVertexSemantic::Normal},
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

    const std::vector<Renderer::ObjectEntry> objects = {
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
            .Rotation = glm::quat(glm::vec3(0, glm::radians(90.f), 0)),
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
    renderer.ClearColor = {0.f / 255.f, 23.f / 255.f, 31.f / 255.f}; // black
    //renderer.ClearColor = {53.f / 255.f, 144.f / 255.f, 243.f / 255.f}; // blue
    //renderer.ClearColor = {255.f / 255.f, 205.f / 255.f, 27.f / 255.f}; // gold
    renderer.UniformData.AmbientColor = glm::vec3(0.1f);
    renderer.UniformData.DirectionalLightVector = glm::normalize(glm::vec3(-1.0f, -1.0f, -1.0f));
    renderer.UniformData.DirectionalLightColor = glm::vec3(1.0f, 1.0, 1.0);
    renderer.UniformData.DirectionalLightPower = 1.0f;
    

    FlyCamera cam;
    glfwSetWindowUserPointer(window, &cam);
    glfwSetMouseButtonCallback(window, MouseButtonCb);
    glfwSetCursorPosCallback(window, CursorPosCb);
    glfwSetScrollCallback(window, scrollCB); 

    // Enable raw mouse motion when available and cursor disabled
    if (glfwRawMouseMotionSupported()) {
        glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    }

    double lastTime = glfwGetTime();
    
    // Main loop
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // Delta time
        double now = glfwGetTime();
        float dt = static_cast<float>(now - lastTime);
        lastTime = now;

        // Keyboard movement (WASD + Space/Ctrl)
        float speed = cam.moveSpeed * dt;
        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) speed *= 2.0f;
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) cam.pos += cam.front * speed;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) cam.pos -= cam.front * speed;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) cam.pos -= cam.right * speed;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) cam.pos += cam.right * speed;
        if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) cam.pos += cam.up * speed;
        if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) cam.pos -= cam.up * speed;

        // Build view/projection
        glm::mat4 view = glm::lookAtLH(cam.pos, cam.pos + cam.front, cam.up);
        glm::mat4 proj = glm::perspectiveFovLH(glm::radians(cam.fov), static_cast<float>(width), static_cast<float>(height), 0.1f, 100.0f);
        renderer.UniformData.ProjectionView = proj * view;
        renderer.UniformData.CameraPosition = cam.pos;

        renderer.Render();
    }
    
    return 0;
}

auto Program::terminate() -> void {
    glfwDestroyWindow(window);
    glfwTerminate();
}
