#include "Game.h"

#include <cstdio>
#include <cassert>
#include <umbrellas/include-glm.h>

#include "BeWindow.h"
#include "BeAssetRegistry.h"
#include "BeInput.h"
#include "BeRenderer.h"
#include "BeCamera.h"
#include "passes/BeComposerPass.h"
#include "passes/BeGeometryPass.h"
#include "passes/BeLightingPass.h"
#include "BeMaterial.h"
#include "BeShader.h"
#include "BeTexture.h"
#include "BeModel.h"
#include "passes/BeFullscreenEffectPass.h"
#include "passes/BeShadowPass.h"

Game::Game() = default;
Game::~Game() = default;


auto Game::Run() -> int {
    constexpr int width = 1920, height = 1080;

    _window = std::make_unique<BeWindow>(width, height, "be: example game 1");

    const HWND hwnd = _window->getHWND();
    _renderer = std::make_unique<BeRenderer>(hwnd, width, height);
    _renderer->LaunchDevice();

    LoadAssets();
    SetupScene();
    SetupRenderPasses();
    SetupCamera(width, height);
    MainLoop();
    
    return 0;
}

auto Game::LoadAssets() -> void {
    _assetRegistry = std::make_unique<BeAssetRegistry>();
    const auto device = _renderer->GetDevice();

    BeShader::StandardShaderIncludePath = "standardShaders/";
    
    // Create builtin textures
    const auto whiteTexture = BeTexture::CreateFromColor(glm::vec4(1.f), device);
    const auto blackTexture = BeTexture::CreateFromColor(glm::vec4(0.f), device);
    _assetRegistry->AddTexture("white", whiteTexture);
    _assetRegistry->AddTexture("black", blackTexture);

    // Create shader
    const auto standardShader = BeShader::Create(device.Get(), "assets/shaders/standard");
    _assetRegistry->AddShader("standard", standardShader);

    // Create models
    const auto planeMaterial = BeMaterial::Create("PlaneMaterial", standardShader, *_assetRegistry, device);
    _plane = CreatePlane(1024);
    _witchItems = BeModel::Create("assets/witch_items.glb", standardShader, *_assetRegistry, device);
    _cube = BeModel::Create("assets/cube.glb", standardShader, *_assetRegistry, device);
    _macintosh = BeModel::Create("assets/model.fbx", standardShader, *_assetRegistry, device);
    _pagoda = BeModel::Create("assets/pagoda.glb", standardShader, *_assetRegistry, device);
    _disks = BeModel::Create("assets/floppy-disks.glb", standardShader, *_assetRegistry, device);
    _anvil = BeModel::Create("assets/anvil/anvil.fbx", standardShader, *_assetRegistry, device);

    _anvil->DrawSlices[0].Material->SetFloat3("SpecularColor0", glm::vec3(1.0f));
    _anvil->DrawSlices[0].Material->SetFloat3("SpecularColor1", glm::vec3(1.0f) * 3.f);
    _anvil->DrawSlices[0].Material->SetFloat("Shininess1", 512.f/2048.f);
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
            .Position = {15, 1, 0},
            .Scale = glm::vec3(1.f),
            .Model = _plane,
        },
        {
            .Name = "Plane",
            .Position = {50, 0, -50},
            .Scale = glm::vec3(100.f, 0.1f, 100.f),
            .Model = _cube,
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
    _directionalLight->ShadowFarPlane = 300.0f;
    _directionalLight->ShadowMapTextureName = "DirectionalLightShadowMap";
    _directionalLight->CalculateMatrix();
    _renderer->SetContextDataPointer("DirectionalLight", _directionalLight.get());

    // Setup point lights
    const auto device = _renderer->GetDevice();
    for (auto i = 0; i < 4; i++) {
        BePointLight pointLight = {};
        pointLight.Radius = 20.0f;
        pointLight.Color = glm::vec3(0.99f, 0.99f, 0.6);
        pointLight.Power = (1.0f / 0.7f) * 2.2f;
        pointLight.CastsShadows = true;
        pointLight.ShadowMapResolution = 2048.0f;
        pointLight.ShadowNearPlane = 0.1f;
        pointLight.ShadowMapTextureName = "PointLight" + std::to_string(i) + "_ShadowMap";
        
        _pointLights.push_back(pointLight);
    }
    _renderer->SetContextDataPointer("PointLights", &_pointLights);
}

auto Game::SetupRenderPasses() -> void {
    // Create render resources
    _renderer->CreateRenderResource(_directionalLight->ShadowMapTextureName, false, {
        .Format = DXGI_FORMAT_R32_TYPELESS,
        .BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE,
        .CustomWidth = 4096,
        .CustomHeight = 4096,
    });
    _renderer->CreateRenderResource("DepthStencil", true, {
        .Format = DXGI_FORMAT_R24G8_TYPELESS,
        .BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE,
    });
    _renderer->CreateRenderResource("BaseColor", true, {
        .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
        .BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE,
    });
    _renderer->CreateRenderResource("WorldNormal", true, {
        .Format = DXGI_FORMAT_R16G16B16A16_FLOAT,
        .BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE,
    });
    _renderer->CreateRenderResource("Specular-Shininess", true, {
        .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
        .BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE,
    });
    _renderer->CreateRenderResource("Lighting", true, {
        .Format = DXGI_FORMAT_R11G11B10_FLOAT,
        .BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE,
    });
    _renderer->CreateRenderResource("PPOutput", true, {
        .Format = DXGI_FORMAT_R11G11B10_FLOAT,
        .BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE,
    });

    for (const auto & pointLight : _pointLights) {
        _renderer->CreateRenderResource(pointLight.ShadowMapTextureName, false, {
            .IsCubemap = true,
            .Format = DXGI_FORMAT_R32_TYPELESS,
            .BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE,
            .CustomWidth = 2048,
            .CustomHeight = 2048,
        });
    }
    
    // Shadow pass
    const auto shadowPass = new BeShadowPass();
    _renderer->AddRenderPass(shadowPass);
    shadowPass->InputDirectionalLightName = "DirectionalLight";
    shadowPass->InputPointLightsName = "PointLights";

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
    lightingPass->InputDirectionalLightName = "DirectionalLight";
    lightingPass->InputPointLightsName = "PointLights";
    lightingPass->InputDepthTextureName = "DepthStencil";
    lightingPass->InputTexture0Name = "BaseColor";
    lightingPass->InputTexture1Name = "WorldNormal";
    lightingPass->InputTexture2Name = "Specular-Shininess";
    lightingPass->OutputTextureName = "Lighting";

    // Cel shader pass
    const auto effectShader = BeShader::Create(_renderer->GetDevice().Get(), "assets/shaders/effects/usedEffect");
    const auto effectPass = new BeFullscreenEffectPass();
    _renderer->AddRenderPass(effectPass);
    effectPass->InputTextureNames = {"Lighting", "DepthStencil", "WorldNormal"};
    effectPass->OutputTextureNames = {"PPOutput"};
    effectPass->Shader = effectShader;

    // Composer pass
    const auto composerPass = new BeComposerPass();
    _renderer->AddRenderPass(composerPass);
    composerPass->InputDepthTextureName = "DepthStencil";
    composerPass->InputTexture0Name = "BaseColor";
    composerPass->InputTexture1Name = "WorldNormal";
    composerPass->InputTexture2Name = "Specular-Shininess";
    composerPass->InputLightTextureName = "PPOutput";
    composerPass->ClearColor = {0.f / 255.f, 23.f / 255.f, 31.f / 255.f}; // black

    _renderer->InitialisePasses();
}

auto Game::SetupCamera(int width, int height) -> void {
    _camera = std::make_unique<BeCamera>();
    _camera->Width = static_cast<float>(width);
    _camera->Height = static_cast<float>(height);
    _camera->NearPlane = 0.1f;
    _camera->FarPlane = 100.0f;

    _renderer->UniformData.NearFarPlane = {0.1f, 100.0f};
    
    _input = std::make_unique<BeInput>(_window->getGLFWWindow());
}

auto Game::MainLoop() -> void {
    double lastTime = glfwGetTime();

    while (!_window->shouldClose()) {
        _window->pollEvents();
        _input->update();

        // Delta time
        double now = glfwGetTime();
        float dt = static_cast<float>(now - lastTime);
        lastTime = now;

        // Keyboard movement
        constexpr float moveSpeed = 5.0f;
        float speed = moveSpeed * dt;
        if (_input->getKey(GLFW_KEY_LEFT_SHIFT)) speed *= 2.0f;
        if (_input->getKey(GLFW_KEY_W)) _camera->Position += _camera->getFront() * speed;
        if (_input->getKey(GLFW_KEY_S)) _camera->Position -= _camera->getFront() * speed;
        if (_input->getKey(GLFW_KEY_D)) _camera->Position -= _camera->getRight() * speed;
        if (_input->getKey(GLFW_KEY_A)) _camera->Position += _camera->getRight() * speed;
        if (_input->getKey(GLFW_KEY_E)) _camera->Position += glm::vec3(0, 1, 0) * speed;
        if (_input->getKey(GLFW_KEY_Q)) _camera->Position -= glm::vec3(0, 1, 0) * speed;

        // Mouse look (right mouse button)
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

        // Mouse scroll (zoom)
        const glm::vec2 scrollDelta = _input->getScrollDelta();
        if (scrollDelta.y != 0.0f) {
            _camera->Fov -= scrollDelta.y;
            _camera->Fov = glm::clamp(_camera->Fov, 20.0f, 90.0f);
        }

        // Update camera matrices
        _camera->updateMatrices();

        // Apply camera to renderer
        _renderer->UniformData.ProjectionView = _camera->getProjectionMatrix() * _camera->getViewMatrix();
        _renderer->UniformData.CameraPosition = _camera->Position;

        // Update point light positions
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
    auto material = BeMaterial::Create("TerrainMat", shader, *_assetRegistry, _renderer->GetDevice());
    material->SetFloat("TerrainScale", 5.0f);
    material->SetFloat("HeightScale", 1.0f);
    
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
