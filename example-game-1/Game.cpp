
#include "Game.h"

#define NOMINMAX

#include <cstdio>
#include <cassert>
#include <numbers>
#include <umbrellas/include-glm.h>

#include "BeWindow.h"
#include "BeAssetRegistry.h"
#include "BeInput.h"
#include "BeRenderer.h"
#include "BeCamera.h"
#include "passes/BeBackbufferPass.h"
#include "passes/BeGeometryPass.h"
#include "passes/BeLightingPass.h"
#include "BeMaterial.h"
#include "BeShader.h"
#include "BeModel.h"
#include "passes/BeBloomPass.h"
#include "passes/BeFullscreenEffectPass.h"
#include "passes/BeShadowPass.h"
#include "ImGuiPass.h"

Game::Game() = default;
Game::~Game() = default;


auto Game::Run() -> int {
    _width = 1920;
    _height = 1080;

    _window = std::make_unique<BeWindow>(_width, _height, "be: example game 1");
    _assetRegistry = std::make_shared<BeAssetRegistry>();
    const HWND hwnd = _window->getHWND();

    _renderer = std::make_unique<BeRenderer>(_width, _height, hwnd, _assetRegistry);
    _renderer->LaunchDevice();

    LoadAssets();
    SetupScene();
    SetupRenderPasses();
    SetupCamera(_width, _height);

    MainLoop();

    return 0;
}

auto Game::LoadAssets() -> void {
    const auto device = _renderer->GetDevice();

    BeShader::StandardShaderIncludePath = "standardShaders/";
    
    // Create builtin textures
    BeTexture::Create("white")
    .SetSize(1, 1)
    .SetBindFlags(D3D11_BIND_SHADER_RESOURCE)
    .SetFormat(DXGI_FORMAT_R8G8B8A8_UNORM)
    .FillWithColor(glm::vec4(1.f))
    .AddToRegistry(_assetRegistry)
    .BuildNoReturn(device);
    
    BeTexture::Create("black")
    .SetSize(1, 1)
    .SetBindFlags(D3D11_BIND_SHADER_RESOURCE)
    .SetFormat(DXGI_FORMAT_R8G8B8A8_UNORM)
    .FillWithColor(glm::vec4(0.f, 0.f, 0.f, 1.f))
    .AddToRegistry(_assetRegistry)
    .BuildNoReturn(device);

    
    // Create shader
    const auto standardShader = BeShader::Create(device.Get(), "assets/shaders/standard");
    _assetRegistry->AddShader("standard", standardShader);

    // Create tessellated shader
    const auto tessellatedShader = BeShader::Create(device.Get(), "assets/shaders/tessellated");
    _assetRegistry->AddShader("tessellated", tessellatedShader);

    // Create models
    _plane = CreatePlane(64);
    _witchItems = BeModel::Create("assets/witch_items.glb", standardShader, _assetRegistry, device);
    _livingCube = BeModel::Create("assets/cube.glb", tessellatedShader, _assetRegistry, device);
    _livingCube->Materials[0]->SetFloat3("DiffuseColor", glm::vec3(0.28, 0.39, 1.0));
    _macintosh = BeModel::Create("assets/model.fbx", standardShader, _assetRegistry, device);
    _pagoda = BeModel::Create("assets/pagoda.glb", standardShader, _assetRegistry, device);
    _disks = BeModel::Create("assets/floppy-disks.glb", standardShader, _assetRegistry, device);
    _anvil = BeModel::Create("assets/anvil/anvil.fbx", standardShader, _assetRegistry, device);

    _anvil->DrawSlices[0].Material->SetFloat3("SpecularColor", glm::vec3(1.0f));
}

auto Game::SetupScene() -> void {
    const std::vector<BeRenderer::ObjectEntry> objects = {
        {
            .Name = "Macintosh",
            .Position = {0, 0, -6.9},
            .Model = _macintosh,
        },
        {
            .Name = "Terrain",
            .Position = {0, 0, 0},
            .Scale = glm::vec3(1.f),
            .Model = _plane,
            .CastShadows = false,
        },
        {
            .Name = "Tessellated Cube",
            .Position = {0, 10, 0},
            .Scale = glm::vec3(2.f),
            .Model = _livingCube,
        },
        {
            .Name = "Pagoda",
            .Position = {0, 0, 8},
            .Scale = glm::vec3(0.2f),
            .Model = _pagoda,
        },
        {
            .Name = "Witch Items",
            .Position = {-3, 2, 5},
            .Scale = glm::vec3(3.f),
            .Model = _witchItems,
        },
        {
            .Name = "Anvil",
            .Position = {7, 0, 5},
            .Rotation = glm::quat(glm::vec3(0, glm::radians(90.f), 0)),
            .Scale = glm::vec3(0.2f),
            .Model = _anvil,
        },
        {
            .Name = "Anvil1",
            .Position = {-7, 0, -3},
            .Rotation = glm::quat(glm::vec3(0, glm::radians(-90.f), 0)),
            .Scale = glm::vec3(0.2f),
            .Model = _anvil,
        },
        {
            .Name = "Anvil2",
            .Position = {-17, -10, -3},
            .Rotation = glm::quat(glm::vec3(0, glm::radians(-90.f), 0)),
            .Scale = glm::vec3(1.0f),
            .Model = _anvil,
        },
        {
            .Name = "Disks",
            .Position = {7.5f, 1, -4},
            .Rotation = glm::quat(glm::vec3(0, glm::radians(150.f), 0)),
            .Model = _disks,
        },
    };

    _renderer->SetObjects(objects);

    _renderer->UniformData.AmbientColor = glm::vec3(0.1f);
        
    // Setup directional light
    _directionalLight = std::make_unique<BeDirectionalLight>();
    _directionalLight->Direction = glm::normalize(glm::vec3(-0.8f, -1.0f, -0.8f));
    _directionalLight->Color = glm::vec3(0.7f, 0.7f, 0.99);
    _directionalLight->Power = (1.0f / 0.7f) * 0.7f;
    _directionalLight->ShadowMapResolution = 4096.0f;
    _directionalLight->ShadowCameraDistance = 100.0f;
    _directionalLight->ShadowMapWorldSize = 60.0f;
    _directionalLight->ShadowNearPlane = 0.1f;
    _directionalLight->ShadowFarPlane = 400.0f;
    _directionalLight->ShadowMapTextureName = "DirectionalLightShadowMap";
    _directionalLight->CalculateMatrix();
    
    // Setup point lights
    const auto device = _renderer->GetDevice();
    for (auto i = 0; i < 4; i++) {
        BePointLight pointLight = {};
        pointLight.Radius = 20.0f;
        pointLight.Color = glm::vec3(0.99f, 0.8f, 0.6f);
        pointLight.Power = (1.0f / 0.7f) * 2.7f;
        pointLight.CastsShadows = true;
        pointLight.ShadowMapResolution = 2048.0f;
        pointLight.ShadowNearPlane = 0.1f;
        pointLight.ShadowMapTextureName = "PointLight" + std::to_string(i) + "_ShadowMap";
        
        _pointLights.push_back(pointLight);
    }
}

auto Game::SetupRenderPasses() -> void {
    const auto device = _renderer->GetDevice();
    
    // Create render resources
    BeTexture::Create(_directionalLight->ShadowMapTextureName)
    .SetBindFlags(D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE)
    .SetFormat(DXGI_FORMAT_R32_TYPELESS)
    .SetSize(static_cast<uint32_t>(_directionalLight->ShadowMapResolution), static_cast<uint32_t>(_directionalLight->ShadowMapResolution))
    .AddToRegistry(_assetRegistry)
    .Build(device);

    for (const auto & pointLight : _pointLights) {
        BeTexture::Create(pointLight.ShadowMapTextureName)
        .SetBindFlags(D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE)
        .SetFormat(DXGI_FORMAT_R32_TYPELESS)
        .SetCubemap(true)
        .SetSize(static_cast<uint32_t>(pointLight.ShadowMapResolution), static_cast<uint32_t>(pointLight.ShadowMapResolution))
        .AddToRegistry(_assetRegistry)
        .Build(device);
    }
    
    BeTexture::Create("DepthStencil")
    .SetBindFlags(D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE)
    .SetFormat(DXGI_FORMAT_R32_TYPELESS)
    .SetSize(_width, _height)
    .AddToRegistry(_assetRegistry)
    .Build(device);

    BeTexture::Create("BaseColor")
    .SetBindFlags(D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE)
    .SetFormat(DXGI_FORMAT_R11G11B10_FLOAT)
    .SetSize(_width, _height)
    .AddToRegistry(_assetRegistry)
    .Build(device);

    BeTexture::Create("WorldNormal")
    .SetBindFlags(D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE)
    .SetFormat(DXGI_FORMAT_R16G16B16A16_FLOAT)
    .SetSize(_width, _height)
    .AddToRegistry(_assetRegistry)
    .Build(device);

    BeTexture::Create("Specular-Shininess")
    .SetBindFlags(D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE)
    .SetFormat(DXGI_FORMAT_R8G8B8A8_UNORM)
    .SetSize(_width, _height)
    .AddToRegistry(_assetRegistry)
    .Build(device);

    BeTexture::Create("HDR-Input")
    .SetBindFlags(D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE)
    .SetFormat(DXGI_FORMAT_R11G11B10_FLOAT)
    .SetSize(_width, _height)
    .AddToRegistry(_assetRegistry)
    .Build(device);

    for (int mip = 0; mip < 5; ++mip) {
        const float multiplier = glm::pow(0.5f, mip);
        const uint32_t mipWidth = _width * multiplier;
        const uint32_t mipHeight = _height * multiplier;

        BeTexture::Create("Bloom_Mip" + std::to_string(mip))
        .SetBindFlags(D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE)
        .SetFormat(DXGI_FORMAT_R11G11B10_FLOAT)
        .SetSize(mipWidth, mipHeight)
        .AddToRegistry(_assetRegistry)
        .Build(device);
    }

    BeTexture::Create("BloomOutput")
    .SetBindFlags(D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE)
    .SetFormat(DXGI_FORMAT_R11G11B10_FLOAT)
    .SetSize(_width, _height)
    .AddToRegistry(_assetRegistry)
    .Build(device);

    BeTexture::Create("TonemapperOutput")
    .SetBindFlags(D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE)
    .SetFormat(DXGI_FORMAT_R11G11B10_FLOAT)
    .SetSize(_width, _height)
    .AddToRegistry(_assetRegistry)
    .Build(device);


    // Shadow pass
    const auto shadowPass = new BeShadowPass();
    _renderer->AddRenderPass(shadowPass);
    shadowPass->DirectionalLight = _directionalLight.get();
    shadowPass->PointLights = &_pointLights;

    // Geometry pass
    const auto geometryPass = new BeGeometryPass();
    _renderer->AddRenderPass(geometryPass);
    geometryPass->OutputDepthTextureName = "DepthStencil";
    geometryPass->OutputTexture0Name = "BaseColor";
    geometryPass->OutputTexture1Name = "WorldNormal";
    geometryPass->OutputTexture2Name = "Specular-Shininess";

    // Lighting pass
    const auto lightingPass = new BeLightingPass();
    _renderer->AddRenderPass(lightingPass);
    lightingPass->DirectionalLight = _directionalLight.get();
    lightingPass->PointLights = &_pointLights;
    lightingPass->InputDepthTextureName = "DepthStencil";
    lightingPass->InputTexture0Name = "BaseColor";
    lightingPass->InputTexture1Name = "WorldNormal";
    lightingPass->InputTexture2Name = "Specular-Shininess";
    lightingPass->OutputTextureName = "HDR-Input";

    // Bloom Pass
    const auto bloomPass = new BeBloomPass();
    _renderer->AddRenderPass(bloomPass);
    bloomPass->AssetRegistry = _assetRegistry;
    bloomPass->InputHDRTextureName = "HDR-Input";
    bloomPass->BloomMipTextureName = "Bloom_Mip";
    bloomPass->BloomMipCount = 5;
    BeTexture::Create("BloomDirtTexture")
    .LoadFromFile("assets/bloom-dirt-mask.png")
    .AddToRegistry(_assetRegistry)
    .BuildNoReturn(device);
    bloomPass->DirtTextureName = "BloomDirtTexture";
    bloomPass->OutputTextureName = "BloomOutput";
    
    // Tonemapper pass
    const auto tonemapperShader = BeShader::Create(_renderer->GetDevice().Get(), "assets/shaders/tonemapper");
    const auto tonemapperPass = new BeFullscreenEffectPass();
    _renderer->AddRenderPass(tonemapperPass);
    tonemapperPass->InputTextureNames = {"BloomOutput"};
    tonemapperPass->OutputTextureNames = {"TonemapperOutput"};
    tonemapperPass->Shader = tonemapperShader;

    // Backbuffer pass
    const auto backbufferPass = new BeBackbufferPass();
    _renderer->AddRenderPass(backbufferPass);
    backbufferPass->InputTextureName = "TonemapperOutput";
    backbufferPass->ClearColor = {0.f / 255.f, 23.f / 255.f, 31.f / 255.f}; // black

    // ImGui pass
    const auto imguiPass = new ImGuiPass(_window->getGLFWWindow());
    _renderer->AddRenderPass(imguiPass);
    imguiPass->DirectionalLight = _directionalLight.get();
    imguiPass->PointLights = &_pointLights;
    imguiPass->TerrainMaterial = _plane->Materials[0];
    imguiPass->LivingCubeMaterial = _livingCube->Materials[0];

    _renderer->InitialisePasses();
    
    imguiPass->BloomMaterial = bloomPass->GetBrightMaterial();
}

auto Game::SetupCamera(int width, int height) -> void {
    _camera = std::make_unique<BeCamera>();
    _camera->Width = static_cast<float>(width);
    _camera->Height = static_cast<float>(height);
    _camera->NearPlane = 0.1f;
    _camera->FarPlane = 200.0f;

    _renderer->UniformData.NearFarPlane = {_camera->NearPlane, _camera->FarPlane};
    
    _input = std::make_unique<BeInput>(_window->getGLFWWindow());
}

auto Game::MainLoop() -> void {
    double lastTime = glfwGetTime();

    while (!_window->shouldClose()) {
        _window->pollEvents();
        _input->update();

        double now = glfwGetTime();
        float dt = static_cast<float>(now - lastTime);
        lastTime = now;

        _renderer->UniformData.Time = now;

        constexpr float moveSpeed = 5.0f;
        float speed = moveSpeed * dt;
        if (_input->getKey(GLFW_KEY_LEFT_SHIFT)) speed *= 2.0f;
        if (_input->getKey(GLFW_KEY_W)) _camera->Position += _camera->getFront() * speed;
        if (_input->getKey(GLFW_KEY_S)) _camera->Position -= _camera->getFront() * speed;
        if (_input->getKey(GLFW_KEY_D)) _camera->Position -= _camera->getRight() * speed;
        if (_input->getKey(GLFW_KEY_A)) _camera->Position += _camera->getRight() * speed;
        if (_input->getKey(GLFW_KEY_E)) _camera->Position += glm::vec3(0, 1, 0) * speed;
        if (_input->getKey(GLFW_KEY_Q)) _camera->Position -= glm::vec3(0, 1, 0) * speed;

        bool captureMouse = false;
        if (_input->getMouseButton(GLFW_MOUSE_BUTTON_RIGHT)) {
            constexpr float mouseSens = 0.1f;

            captureMouse = true;
            const glm::vec2 mouseDelta = _input->getMouseDelta();
            _camera->Yaw   -= mouseDelta.x * mouseSens;
            _camera->Pitch -= mouseDelta.y * mouseSens;
            _camera->Pitch = glm::clamp(_camera->Pitch, -89.0f, 89.0f);
        }
        _input->setMouseCapture(captureMouse);

        const glm::vec2 scrollDelta = _input->getScrollDelta();
        if (scrollDelta.y != 0.0f) {
            _camera->Fov -= scrollDelta.y;
            _camera->Fov = glm::clamp(_camera->Fov, 20.0f, 90.0f);
        }

        _camera->updateMatrices();

        _renderer->UniformData.ProjectionView = _camera->getProjectionMatrix() * _camera->getViewMatrix();
        _renderer->UniformData.CameraPosition = _camera->Position;

        {
            static float angle = 0.0f;
            angle += dt * glm::radians(15.0f); // 15 degrees per second
            if (angle > glm::two_pi<float>()) angle -= glm::two_pi<float>();
            constexpr float radius = 13.0f;
            for (int i = 0; i < _pointLights.size(); ++i) {
                float add = glm::two_pi<float>() * (static_cast<float>(i) / static_cast<float>(_pointLights.size()));
                float rad = radius * (0.7f + 0.3f * ((i + 1) % 2));
                auto& light = _pointLights[i];
                light.Position = glm::vec3(cos(angle + add) * rad, 4.0f + 4.0f * (i % 2), sin(angle + add) * rad);
            }
        }

        _renderer->Render();
    }
}

auto Game::CreatePlane(size_t verticesPerSide) -> std::shared_ptr<BeModel> {
    const auto shader = BeShader::Create(_renderer->GetDevice().Get(), "assets/shaders/terrain");
    auto material = BeMaterial::Create("TerrainMat", true, shader, _assetRegistry, _renderer->GetDevice());
    material->SetFloat("TerrainScale", 200.0f);
    material->SetFloat("HeightScale", 100.0f);
    
    auto model = std::make_shared<BeModel>();
    model->Shader = shader;
    model->Materials.push_back(material);

    const float cellSize = 1.0f / (verticesPerSide - 1);

    model->FullVertices.reserve(verticesPerSide * verticesPerSide);
    for (int y = 0; y < verticesPerSide; ++y) {
        for (int x = 0; x < verticesPerSide; ++x) {
            BeFullVertex vertex{};

            float posX = x * cellSize - 0.5f;
            float posZ = y * cellSize - 0.5f;

            vertex.Position = {-posX, 0.0f, posZ};
            vertex.Normal = {0.0f, 1.0f, 0.0f};
            vertex.Color = {1.0f, 1.0f, 1.0f, 1.0f};
            vertex.UV0 = {x * cellSize, y * cellSize};

            model->FullVertices.push_back(vertex);
        }
    }

    size_t quadsPerSide = verticesPerSide - 1;
    model->Indices.reserve(quadsPerSide * quadsPerSide * 6);

    for (int y = 0; y < quadsPerSide; ++y) {
        for (int x = 0; x < quadsPerSide; ++x) {
            uint32_t topLeft = y * verticesPerSide + x;
            uint32_t topRight = y * verticesPerSide + (x + 1);
            uint32_t bottomLeft = (y + 1) * verticesPerSide + x;
            uint32_t bottomRight = (y + 1) * verticesPerSide + (x + 1);

            model->Indices.push_back(topLeft);
            model->Indices.push_back(topRight);
            model->Indices.push_back(bottomLeft);

            model->Indices.push_back(topRight);
            model->Indices.push_back(bottomRight);
            model->Indices.push_back(bottomLeft);
        }
    }

    BeModel::BeDrawSlice slice{};
    slice.IndexCount = static_cast<uint32_t>(model->Indices.size());
    slice.StartIndexLocation = 0;
    slice.BaseVertexLocation = 0;
    slice.Material = material;
    model->DrawSlices.push_back(slice);

    return model;
}