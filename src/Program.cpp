#include "Program.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include <cstdio>
#include <cassert>

#define GLM_FORCE_LEFT_HANDED
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <gtc/matrix_transform.hpp>

#include "BeAssetImporter.h"
#include "BeInput.h"
#include "BeRenderer.h"
#include "BeCamera.h"
#include "BeComposerPass.h"
#include "BeGeometryPass.h"
#include "BeLightingPass.h"
#include "BeShader.h"
#include "CustomFullscreenEffectPass.h"
#include "ShadowPass.h"


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
    BeRenderer renderer(hwnd, width, height);
    renderer.LaunchDevice();
    const auto device = renderer.GetDevice();

    BeShader standardShader(
        device.Get(),
        "assets/shaders/standard",
        BeShaderType::Vertex | BeShaderType::Pixel,
        {
            {.Name = "Position", .Attribute = BeVertexElementDescriptor::BeVertexSemantic::Position},
            {.Name = "Normal", .Attribute = BeVertexElementDescriptor::BeVertexSemantic::Normal},
            {.Name = "UV", .Attribute = BeVertexElementDescriptor::BeVertexSemantic::TexCoord0},
        }
    );

    BeAssetImporter importer(device);
    auto witchItems = importer.LoadModel("assets/witch_items.glb");
    auto cube = importer.LoadModel("assets/cube.glb");
    auto macintosh = importer.LoadModel("assets/model.fbx");
    auto pagoda = importer.LoadModel("assets/pagoda.glb");
    auto disks = importer.LoadModel("assets/floppy-disks.glb");
    auto anvil = importer.LoadModel("assets/anvil/anvil.fbx");
    anvil->DrawSlices[0].Material.SpecularColor = glm::vec4(1.0f);
    anvil->DrawSlices[0].Material.SuperSpecularColor = glm::vec4(1.0f) * 3.f;
    anvil->DrawSlices[0].Material.SuperShininess = 512.f;
    

    const std::vector<BeRenderer::ObjectEntry> objects = {
        {
            .Name = "Macintosh",
            .Position = {0, 0, -7},
            .Model = macintosh.get(),
            .Shader = &standardShader,
        },
        {
            .Name = "Plane",
            .Position = {50, 0, -50},
            .Scale = glm::vec3(100.f, 0.1f, 100.f),
            .Model = cube.get(),
            .Shader = &standardShader,
        },
        {
            .Name = "Pagoda",
            .Position = {0, 0, 8},
            .Scale = glm::vec3(0.2f),
            .Model = pagoda.get(),
            .Shader = &standardShader,
        },
        {
            .Name = "Witch Items",
            .Position = {-3, 2, 5},
            .Scale = glm::vec3(3.f),
            .Model = witchItems.get(),
            .Shader = &standardShader,
        },
        {
            .Name = "Anvil",
            .Position = {7, 0, 5},
            .Rotation = glm::quat(glm::vec3(0, glm::radians(90.f), 0)),
            .Scale = glm::vec3(0.2f),
            .Model = anvil.get(),
            .Shader = &standardShader,
        },
        {
            .Name = "Anvil1",
            .Position = {-7, 0, -3},
            .Rotation = glm::quat(glm::vec3(0, glm::radians(-90.f), 0)),
            .Scale = glm::vec3(0.2f),
            .Model = anvil.get(),
            .Shader = &standardShader,
        },
        {
            .Name = "Anvil2",
            .Position = {-17, -10, -3},
            .Rotation = glm::quat(glm::vec3(0, glm::radians(-90.f), 0)),
            .Scale = glm::vec3(1.0f),
            .Model = anvil.get(),
            .Shader = &standardShader,
        },
        {
            .Name = "Disks",
            .Position = {7.5f, 1, -4},
            .Rotation = glm::quat(glm::vec3(0, glm::radians(150.f), 0)),
            .Model = disks.get(),
            .Shader = &standardShader,
        },
    };

    renderer.SetObjects(objects);

    renderer.UniformData.NearFarPlane = {0.1f, 100.0f};
    renderer.UniformData.AmbientColor = glm::vec3(0.1f);

    renderer.CreateRenderResource("DirectionalLightShadowMap", false, BeRenderResource::BeResourceDescriptor {
        .Format = DXGI_FORMAT_R32_FLOAT,
        .BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE,
        .CustomWidth = 4096,
        .CustomHeight = 4096,
    });
    renderer.CreateRenderResource("DepthStencil", true, BeRenderResource::BeResourceDescriptor {
        .Format = DXGI_FORMAT_R24G8_TYPELESS,
        .BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE,
    });
    renderer.CreateRenderResource("BaseColor", true, BeRenderResource::BeResourceDescriptor {
        .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
        .BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE,
    });
    renderer.CreateRenderResource("WorldNormal", true, BeRenderResource::BeResourceDescriptor {
        .Format = DXGI_FORMAT_R16G16B16A16_FLOAT,
        .BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE,
    });
    renderer.CreateRenderResource("Specular-Shininess", true, BeRenderResource::BeResourceDescriptor {
        .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
        .BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE,
    });
    renderer.CreateRenderResource("Lighting", true, BeRenderResource::BeResourceDescriptor {
        .Format = DXGI_FORMAT_R11G11B10_FLOAT,
        .BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE,
    });
    renderer.CreateRenderResource("PPOutput", true, BeRenderResource::BeResourceDescriptor {
        .Format = DXGI_FORMAT_R11G11B10_FLOAT,
        .BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE,
    });

    BeDirectionalLight directionalLight;
    directionalLight.Direction = glm::normalize(glm::vec3(-0.8f, -1.0f, -0.8f));
    directionalLight.Color = glm::vec3(0.7f, 0.7f, 0.99); 
    directionalLight.Power = (1.0f / 0.7f) * 0.7f;
    directionalLight.ShadowMapResolution = 4096.0f;
    directionalLight.ShadowCameraDistance = 100.0f;
    directionalLight.ShadowMapWorldSize = 60.0f;
    directionalLight.ShadowNearPlane = 0.1f;
    directionalLight.ShadowFarPlane = 200.0f;
    directionalLight.ShadowMapTextureName = "DirectionalLightShadowMap";
    directionalLight.CalculateMatrix();
    renderer.SetContextDataPointer("DirectionalLight", &directionalLight);

    std::vector<BePointLight> pointLights;
    for (auto i = 0; i < 3; i++) {
        BePointLight pointLight = {};
        pointLight.Radius = 20.0f;
        pointLight.Color = glm::vec3(0.99f, 0.99f, 0.6);
        pointLight.Power = (1.0f / 0.7f) * 2.2f;
        pointLight.CastsShadows = true;
        pointLight.ShadowMapResolution = 2048.0f;
        pointLight.ShadowNearPlane = 0.1f;
        pointLight.ShadowMapTextureName = "PointLight" + std::to_string(i) + "_ShadowMap";
        renderer.CreateRenderResource(pointLight.ShadowMapTextureName, false, {
            .IsCubemap = true,
            .Format = DXGI_FORMAT_R32_FLOAT,
            .BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE,
            .CustomWidth = 2048,
            .CustomHeight = 2048,
        });
        pointLights.push_back(pointLight);
    }
    renderer.SetContextDataPointer("PointLights", &pointLights);
    
    // shadow pass
    auto shadowPass = new ShadowPass();
    renderer.AddRenderPass(shadowPass);
    shadowPass->InputDirectionalLightName = "DirectionalLight";
    shadowPass->InputPointLightsName = "PointLights";
    
    // geometry pass
    auto geometryPass = new BeGeometryPass();
    renderer.AddRenderPass(geometryPass);
    geometryPass->OutputDepthTextureName = "DepthStencil";
    geometryPass->OutputTexture0Name = "BaseColor";
    geometryPass->OutputTexture1Name = "WorldNormal";
    geometryPass->OutputTexture2Name = "Specular-Shininess";

    // lighting pass
    auto lightingPass = new BeLightingPass();
    renderer.AddRenderPass(lightingPass);
    lightingPass->InputDirectionalLightName = "DirectionalLight";
    lightingPass->InputPointLightsName = "PointLights";
    lightingPass->InputDepthTextureName = "DepthStencil";
    lightingPass->InputTexture0Name = "BaseColor";
    lightingPass->InputTexture1Name = "WorldNormal";
    lightingPass->InputTexture2Name = "Specular-Shininess";
    lightingPass->OutputTextureName = "Lighting";

    // Cel shader pass
    auto effectShader = std::make_unique<BeShader>(
        device.Get(),
        "assets/shaders/glitchPass",
        BeShaderType::Pixel,
        std::vector<BeVertexElementDescriptor>{}
    );
    auto effectPass = new CustomFullscreenEffectPass();
    renderer.AddRenderPass(effectPass);
    effectPass->InputTextureNames = {"DepthStencil", "Lighting", "WorldNormal"};
    //effectPass->InputTextureNames = {"Lighting"};
    effectPass->OutputTextureNames = {"PPOutput"};
    effectPass->Shader = effectShader.get();

    // composer pass
    auto composerPass = new BeComposerPass();
    renderer.AddRenderPass(composerPass);
    composerPass->InputDepthTextureName = "DepthStencil";
    composerPass->InputTexture0Name = "BaseColor";
    composerPass->InputTexture1Name = "WorldNormal";
    composerPass->InputTexture2Name = "Specular-Shininess";
    composerPass->InputLightTextureName = "PPOutput";
    composerPass->ClearColor = {0.f / 255.f, 23.f / 255.f, 31.f / 255.f}; // black
    //composerPass->ClearColor = {53.f / 255.f, 144.f / 255.f, 243.f / 255.f}; // blue
    //composerPass->ClearColor = {255.f / 255.f, 205.f / 255.f, 27.f / 255.f}; // gold

    
    renderer.InitialisePasses();
    
    BeInput input(window);
    BeCamera cam;
    cam.Width = static_cast<float>(width);
    cam.Height = static_cast<float>(height);
    cam.NearPlane = 0.1f;
    cam.FarPlane = 100.0f;

    double lastTime = glfwGetTime();
    
    // Main loop
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        input.update();

        // Delta time
        double now = glfwGetTime();
        float dt = static_cast<float>(now - lastTime);
        lastTime = now;

        // Keyboard movement
        constexpr float moveSpeed = 5.0f;
        float speed = moveSpeed * dt;
        if (input.getKey(GLFW_KEY_LEFT_SHIFT)) speed *= 2.0f;
        if (input.getKey(GLFW_KEY_W)) cam.Position += cam.getFront() * speed;
        if (input.getKey(GLFW_KEY_S)) cam.Position -= cam.getFront() * speed;
        if (input.getKey(GLFW_KEY_D)) cam.Position -= cam.getRight() * speed;
        if (input.getKey(GLFW_KEY_A)) cam.Position += cam.getRight() * speed;
        if (input.getKey(GLFW_KEY_E)) cam.Position += glm::vec3(0, 1, 0) * speed;
        if (input.getKey(GLFW_KEY_Q)) cam.Position -= glm::vec3(0, 1, 0) * speed;

        // Mouse look (right mouse button)
        bool captureMouse = false;
        if (input.getMouseButton(GLFW_MOUSE_BUTTON_RIGHT)) {
            constexpr float mouseSens = 0.1f;
            
            captureMouse = true;
            const glm::vec2 mouseDelta = input.getMouseDelta();
            cam.Yaw   -= mouseDelta.x * mouseSens;
            cam.Pitch -= mouseDelta.y * mouseSens;
            cam.Pitch = glm::clamp(cam.Pitch, -89.0f, 89.0f);
        }
        input.setMouseCapture(captureMouse);

        // Mouse scroll (zoom)
        const glm::vec2 scrollDelta = input.getScrollDelta();
        if (scrollDelta.y != 0.0f) {
            cam.Fov -= scrollDelta.y;
            cam.Fov = glm::clamp(cam.Fov, 20.0f, 90.0f);
        }

        // Update camera matrices
        cam.updateMatrices();

        // Apply camera to renderer
        renderer.UniformData.ProjectionView = cam.getProjectionMatrix() * cam.getViewMatrix();
        renderer.UniformData.CameraPosition = cam.Position;

        {
            static float angle = 0.0f;
            angle += dt * glm::radians(15.0f); // 15 degrees per second
            if (angle > glm::two_pi<float>()) angle -= glm::two_pi<float>();
            constexpr float radius = 13.0f;
            for (int i = 0; i < pointLights.size(); ++i) {
                float add = glm::two_pi<float>() * (static_cast<float>(i) / static_cast<float>(pointLights.size()));
                float rad = radius * (0.7f + 0.3f * ((i + 1) % 2));
                auto& light = pointLights[i];
                light.Position = glm::vec3(cos(angle + add) * rad, 4.0f + 4.0f * (i % 2), sin(angle + add) * rad);
            }
        }
        
        renderer.Render();
    }
    
    return 0;
}

auto Program::terminate() -> void {
    glfwDestroyWindow(window);
    glfwTerminate();
}
