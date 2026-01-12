
#include "MainScene.h"

#include <glfw/glfw3.h>

#include "BeAssetRegistry.h"
#include "BeCamera.h"
#include "BeInput.h"
#include "BeMaterial.h"
#include "BeModel.h"
#include "BeRenderer.h"
#include "BeShader.h"
#include "BeTexture.h"
#include "BeWindow.h"
#include "basic-render-pipeline/BeBackbufferPass.h"
#include "basic-render-pipeline/BeBloomPass.h"
#include "basic-render-pipeline/BeFullscreenEffectPass.h"
#include "basic-render-pipeline/BeGeometryPass.h"
#include "basic-render-pipeline/BeLightingPass.h"
#include "basic-render-pipeline/BeShadowPass.h"

MainScene::MainScene(
    const std::shared_ptr<BeRenderer>& renderer,
    const std::shared_ptr<BeWindow>& window,
    const std::shared_ptr<BeInput>& input
)
    : _renderer(renderer)
    , _window(window)
    , _input(input)
{}

auto MainScene::Prepare() -> void {
    _submissionBuffer = std::make_shared<BeBRPSubmissionBuffer>();
    
    _camera = std::make_unique<BeCamera>(); 
    _camera->Width = _window->GetWidth();
    _camera->Height = _window->GetHeight();
    _camera->NearPlane = 0.1f;
    _camera->FarPlane = 200.0f;
    
    const auto device = _renderer->GetDevice();
    
    BeTexture::Create("white")
    .SetSize(1, 1)
    .SetBindFlags(D3D11_BIND_SHADER_RESOURCE)
    .SetFormat(DXGI_FORMAT_R8G8B8A8_UNORM)
    .FillWithColor(glm::vec4(1.f))
    .AddToRegistry()
    .BuildNoReturn(device);

    BeTexture::Create("black")
    .SetSize(1, 1)
    .SetBindFlags(D3D11_BIND_SHADER_RESOURCE)
    .SetFormat(DXGI_FORMAT_R8G8B8A8_UNORM)
    .FillWithColor(glm::vec4(0.f, 0.f, 0.f, 1.f))
    .AddToRegistry()
    .BuildNoReturn(device);

    BeAssetRegistry::InjectRenderer(_renderer);
    BeAssetRegistry::IndexShaderFiles({ 
        "assets/shaders/objectMaterial.hlsl", 
        "assets/shaders/standard.hlsl",
        "assets/shaders/checkerboard.hlsl",
        "assets/shaders/fullscreen-vertex.hlsl", 
        "assets/shaders/directionalLight.hlsl", 
        "assets/shaders/pointLight.hlsl", 
        "assets/shaders/emissive-add.hlsl",
        "assets/shaders/BeBloomAdd.hlsl", 
        "assets/shaders/BeBloomBright.hlsl", 
        "assets/shaders/BeBloomKawase.hlsl", 
        "assets/shaders/tonemapper.hlsl", 
        "assets/shaders/backbuffer.hlsl", 
    });
    
    const auto standardShader = BeAssetRegistry::GetShader("standard");
    const auto checkerboardShader = BeAssetRegistry::GetShader("checkerboard");
    
    _cube = BeModel::Create("assets/cube.glb", checkerboardShader, *_renderer);
    _cube->Materials[0]->SetTexture("DiffuseTexture", 
        BeTexture::Create("Checkerboard")
        .LoadFromFile("assets/checkerboard.png")
        .AddToRegistry()
        .Build(device)
    ); 
    
    _emissiveCube = BeModel::Create("assets/cube.glb", standardShader, *_renderer);
    _emissiveCube->Materials[0]->SetFloat3("EmissiveColor", glm::vec3(0.99f, 0.8f, 0.6f) * 1.7f);
    
    _anvil = BeModel::Create("assets/anvil/anvil.fbx", standardShader, *_renderer);
    _anvil->Materials[0]->SetFloat3("SpecularColor", glm::vec3(1.0f));
    _anvil->Materials[0]->SetSampler("InputSampler", BeAssetRegistry::GetSampler("point-clamp"));

    _sakura = BeModel::Create("assets/sakura/scene.gltf", standardShader, *_renderer);
    _sakura->Materials[0]->SetSampler("InputSampler", BeAssetRegistry::GetSampler("linear-wrap"));
    
    _sakura2 = BeModel::Create("assets/stylized_sakura_tree.glb", standardShader, *_renderer);
    //_sakura2->Materials[0]
    
    const std::vector<std::shared_ptr<BeModel>> models {
        _cube, _anvil, _sakura, _sakura2, _emissiveCube
    };
    _renderer->RegisterModels(models);

    _renderer->UniformData.AmbientColor = glm::vec3(0.1f);

    const uint32_t screenWidth = _window->GetWidth();
    const uint32_t screenHeight = _window->GetHeight();
    
    BeTexture::Create("DepthStencil")
    .SetBindFlags(D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE)
    .SetFormat(DXGI_FORMAT_R32_TYPELESS)
    .SetSize(screenWidth, screenHeight)
    .AddToRegistry()
    .Build(device);

    BeTexture::Create("BaseColor")
    .SetBindFlags(D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE)
    .SetFormat(DXGI_FORMAT_R11G11B10_FLOAT)
    .SetSize(screenWidth, screenHeight)
    .AddToRegistry()
    .Build(device);

    BeTexture::Create("WorldNormal")
    .SetBindFlags(D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE)
    .SetFormat(DXGI_FORMAT_R16G16B16A16_FLOAT)
    .SetSize(screenWidth, screenHeight)
    .AddToRegistry()
    .Build(device);

    BeTexture::Create("Specular-Shininess")
    .SetBindFlags(D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE)
    .SetFormat(DXGI_FORMAT_R8G8B8A8_UNORM)
    .SetSize(screenWidth, screenHeight)
    .AddToRegistry()
    .Build(device);

    BeTexture::Create("Emissive")
    .SetBindFlags(D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE)
    .SetFormat(DXGI_FORMAT_R11G11B10_FLOAT)
    .SetSize(screenWidth, screenHeight)
    .AddToRegistry()
    .Build(device);
    
    BeTexture::Create("HDR-Input")
    .SetBindFlags(D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE)
    .SetFormat(DXGI_FORMAT_R11G11B10_FLOAT)
    .SetSize(screenWidth, screenHeight)
    .AddToRegistry()
    .Build(device);

    for (int mip = 0; mip < 5; ++mip) {
        const float multiplier = glm::pow(0.5f, mip);
        const uint32_t mipWidth = screenWidth * multiplier;
        const uint32_t mipHeight = screenHeight * multiplier;

        BeTexture::Create("Bloom_Mip" + std::to_string(mip))
        .SetBindFlags(D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE)
        .SetFormat(DXGI_FORMAT_R11G11B10_FLOAT)
        .SetSize(mipWidth, mipHeight)
        .AddToRegistry()
        .Build(device);
    }

    BeTexture::Create("BloomOutput")
    .SetBindFlags(D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE)
    .SetFormat(DXGI_FORMAT_R11G11B10_FLOAT)
    .SetSize(screenWidth, screenHeight)
    .AddToRegistry()
    .Build(device);

    BeTexture::Create("TonemapperOutput")
    .SetBindFlags(D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE)
    .SetFormat(DXGI_FORMAT_R11G11B10_FLOAT)
    .SetSize(screenWidth, screenHeight)
    .AddToRegistry()
    .Build(device);
}

auto MainScene::OnLoad() -> void {
    const auto& device = _renderer->GetDevice();
    
    _renderer->ClearPasses();

    const auto shadowPass = new BeShadowPass();
    _renderer->AddRenderPass(shadowPass);
    shadowPass->SubmissionBuffer = _submissionBuffer;

    const auto geometryPass = new BeGeometryPass();
    _renderer->AddRenderPass(geometryPass);
    geometryPass->SubmissionBuffer = _submissionBuffer;
    geometryPass->OutputDepthTexture = BeAssetRegistry::GetTexture("DepthStencil");
    geometryPass->OutputTexture0 = BeAssetRegistry::GetTexture("BaseColor");
    geometryPass->OutputTexture1 = BeAssetRegistry::GetTexture("WorldNormal");
    geometryPass->OutputTexture2 = BeAssetRegistry::GetTexture("Specular-Shininess");
    geometryPass->OutputTexture3 = BeAssetRegistry::GetTexture("Emissive");

    const auto lightingPass = new BeLightingPass();
    _renderer->AddRenderPass(lightingPass);
    lightingPass->SubmissionBuffer = _submissionBuffer;
    lightingPass->InputDepthTexture = BeAssetRegistry::GetTexture("DepthStencil");
    lightingPass->InputTexture0 = BeAssetRegistry::GetTexture("BaseColor");
    lightingPass->InputTexture1 = BeAssetRegistry::GetTexture("WorldNormal");
    lightingPass->InputTexture2 = BeAssetRegistry::GetTexture("Specular-Shininess");
    lightingPass->InputTexture3 = BeAssetRegistry::GetTexture("Emissive");
    lightingPass->OutputTexture = BeAssetRegistry::GetTexture("HDR-Input");

    BeTexture::Create("BloomDirtTexture")
    .LoadFromFile("assets/bloom-dirt-mask.png")
    .AddToRegistry()
    .BuildNoReturn(device);
    const auto bloomPass = new BeBloomPass();
    _renderer->AddRenderPass(bloomPass);
    bloomPass->InputHDRTexture = BeAssetRegistry::GetTexture("HDR-Input");
    bloomPass->BloomMipTextures = {
        BeAssetRegistry::GetTexture("Bloom_Mip0"),
        BeAssetRegistry::GetTexture("Bloom_Mip1"),
        BeAssetRegistry::GetTexture("Bloom_Mip2"),
        BeAssetRegistry::GetTexture("Bloom_Mip3"),
        BeAssetRegistry::GetTexture("Bloom_Mip4"),
    };
    bloomPass->BloomMipCount = 5;
    bloomPass->DirtTexture = BeAssetRegistry::GetTexture("BloomDirtTexture");
    bloomPass->OutputTexture = BeAssetRegistry::GetTexture("BloomOutput");

    const auto tonemapperShader = BeAssetRegistry::GetShader("tonemapper");
    const auto& tonemapperScheme = BeAssetRegistry::GetMaterialScheme("tonemapper-material");
    const auto tonemapperMaterial = BeMaterial::Create("TonemapperMaterial", tonemapperScheme, false, *_renderer);
    tonemapperMaterial->SetTexture("HDRInput", BeAssetRegistry::GetTexture("BloomOutput").lock());
    const auto tonemapperPass = new BeFullscreenEffectPass();
    _renderer->AddRenderPass(tonemapperPass);
    tonemapperPass->OutputTextureNames = {"TonemapperOutput"};
    tonemapperPass->Shader = tonemapperShader;
    tonemapperPass->Material = tonemapperMaterial;

    const auto backbufferPass = new BeBackbufferPass();
    _renderer->AddRenderPass(backbufferPass);
    backbufferPass->InputTexture = BeAssetRegistry::GetTexture("TonemapperOutput");
    backbufferPass->ClearColor = {0.f / 255.f, 23.f / 255.f, 31.f / 255.f};
    
    _renderer->InitialisePasses();
    
    
    CreateEntity(_registry
        ,NameComponent { .Name = "Cube" }
        ,TransformComponent { .Position = glm::vec3(15, -30, -15), .Rotation = glm::quat(), .Scale = glm::vec3(30) }
        ,RenderComponent { .Model = _cube, .CastShadows = true }
    );

    CreateEntity(_registry
        ,NameComponent { .Name = "Anvil1" }
        ,RenderComponent { .Model = _anvil }
        ,TransformComponent { 
            .Position = {0, 0, 0}, 
            .Rotation = glm::quat(glm::vec3(0, 45_rad, 0)), 
            .Scale = glm::vec3(0.2f), 
        }
    );
    
    //CreateEntity(_registry
    //    ,NameComponent { .Name = "Sakura" }
    //    ,RenderComponent { .Model = _sakura }
    //    ,TransformComponent { 
    //        .Position = {-7.5, 0, 0}, 
    //        .Rotation = glm::quat(glm::vec3(0, 45.0_rad, 0)), 
    //        .Scale = glm::vec3(0.7f), 
    //    }
    //);
    
    CreateEntity(_registry
        ,NameComponent { .Name = "Sakura2" }
        ,RenderComponent { .Model = _sakura2 }
        ,TransformComponent { 
            .Position = {-3.f, -5.5, 2}, 
            .Rotation = glm::quat(glm::vec3(0, 45.0_rad, 0)), 
            .Scale = glm::vec3(5.0f), 
        }
    );

    CreateEntity(_registry
        ,NameComponent { .Name = "Moon" }
        ,SunLightComponent {
            .Direction = { -1, -1, -1 },
            .Color = glm::vec3(0.7f, 0.7f, 0.99),
            .Power = (1.0f / 0.7f) * 0.7f,
            .CastsShadows = true,
            .ShadowMapResolution = 4096,
            .ShadowCameraDistance = 100.0f,
            .ShadowMapWorldSize = 60.0f,
            .ShadowNearPlane = 0.1f,
            .ShadowFarPlane = 400.0f,
            .ShadowMap = BeTexture::Create("SunLightShadowMap")
                .SetBindFlags(D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE)
                .SetFormat(DXGI_FORMAT_R32_TYPELESS)
                .SetSize(4096, 4096)
                .AddToRegistry()
                .Build(device)
        }
    );

    for (uint32_t i = 0; i < 4; ++i) {
        CreateEntity(_registry
            ,NameComponent { .Name = "PointLight_" + std::to_string(i) }
            ,TransformComponent { .Scale = glm::vec3(0.5f) }
            ,PointLightComponent {
                .Radius = 20.0f,
                .Color = glm::vec3(0.99f, 0.8f, 0.6f),
                .Power = (1.0f / 0.7f) * 2.7f,
                .CastsShadows = false,
                .ShadowMapResolution = 2048,
                .ShadowNearPlane = 0.1f,
                .ShadowMap = 
                    BeTexture::Create("PointLight" + std::to_string(i) + "_ShadowMap")
                    .SetBindFlags(D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE)
                    .SetFormat(DXGI_FORMAT_R32_TYPELESS)
                    .SetCubemap(true)
                    .SetSize(2048, 2048)
                    .AddToRegistry()
                    .Build(device)
            }
            ,RenderComponent {
                .Model = _emissiveCube,
                .CastShadows = false,
            }
        );
    }
    
    srand(time(0));
    for (uint32_t i = 0; i < 100; ++i) {
        auto randFloat = [](float min, float max) -> float {
            float random = float(rand()) / float(RAND_MAX);
            return min + random * (max - min);
        };
        
        auto x = randFloat(-50.f, 50.f);
        auto y = randFloat(30.f, 60.f);
        auto z = randFloat(-50.f, 50.f);
        
        CreateEntity(_registry
            ,NameComponent { .Name = "Star_" + std::to_string(i) }
            ,TransformComponent { .Position = glm::vec3(x, y, z), .Scale = glm::vec3(0.1f) }
            ,RenderComponent {
                .Model = _emissiveCube,
                .CastShadows = false,
            }
        );
    }
}


auto MainScene::Tick(float deltaTime) -> void {
    static const auto GeometryView = _registry.view<TransformComponent, RenderComponent>();
    static const auto SunView = _registry.view<SunLightComponent>();
    static const auto PointLightView = _registry.view<TransformComponent, PointLightComponent>();
    
    constexpr float moveSpeed = 5.0f;
    float speed = moveSpeed * deltaTime;
    if (_input->GetKey(GLFW_KEY_LEFT_SHIFT) || (_input->IsGamepadConnected() && _input->GetGamepadButton(GLFW_GAMEPAD_BUTTON_LEFT_BUMPER))) speed *= 2.0f;
    if (_input->GetKey(GLFW_KEY_W)) _camera->Position += _camera->GetFront() * speed;
    if (_input->GetKey(GLFW_KEY_S)) _camera->Position -= _camera->GetFront() * speed;
    if (_input->GetKey(GLFW_KEY_D)) _camera->Position -= _camera->GetRight() * speed;
    if (_input->GetKey(GLFW_KEY_A)) _camera->Position += _camera->GetRight() * speed;
    if (_input->GetKey(GLFW_KEY_E)) _camera->Position += glm::vec3(0, 1, 0) * speed;
    if (_input->GetKey(GLFW_KEY_Q)) _camera->Position -= glm::vec3(0, 1, 0) * speed;

    // Gamepad movement
    if (_input->IsGamepadConnected()) {
        const glm::vec2 leftStick = _input->GetGamepadLeftStick();
        _camera->Position += _camera->GetFront() * (leftStick.y * speed);
        _camera->Position -= _camera->GetRight() * (leftStick.x * speed);

        const float verticalInput = _input->GetGamepadRightTrigger() - _input->GetGamepadLeftTrigger();
        _camera->Position += glm::vec3(0, 1, 0) * (verticalInput * speed);
    }

    bool captureMouse = false;
    if (_input->GetMouseButton(GLFW_MOUSE_BUTTON_RIGHT)) {
        constexpr float mouseSens = 0.1f;

        captureMouse = true;
        const glm::vec2 mouseDelta = _input->GetMouseDelta();
        _camera->Yaw   -= mouseDelta.x * mouseSens;
        _camera->Pitch -= mouseDelta.y * mouseSens;
        _camera->Pitch = glm::clamp(_camera->Pitch, -89.0f, 89.0f);
    }
    _input->SetMouseCapture(captureMouse);

    // Gamepad camera look
    if (_input->IsGamepadConnected()) {
        const glm::vec2 rightStick = _input->GetGamepadRightStick();
        constexpr float gamepadCameraSens = 100.0f;

        _camera->Yaw   -= rightStick.x * gamepadCameraSens * deltaTime;
        _camera->Pitch += rightStick.y * gamepadCameraSens * deltaTime;
        _camera->Pitch = glm::clamp(_camera->Pitch, -89.0f, 89.0f);
    }

    const glm::vec2 scrollDelta = _input->GetScrollDelta();
    if (scrollDelta.y != 0.0f) {
        _camera->Fov -= scrollDelta.y;
        _camera->Fov = glm::clamp(_camera->Fov, 20.0f, 90.0f);
    }

    {
        _camera->Update();
        _renderer->UniformData.NearFarPlane = {_camera->NearPlane, _camera->FarPlane};
        _renderer->UniformData.ProjectionView = _camera->GetProjectionMatrix() * _camera->GetViewMatrix();
        _renderer->UniformData.CameraPosition = _camera->Position;
    }

    {
        static float angle = 0.0f;
        angle += deltaTime * glm::radians(15.0f);
        if (angle > glm::two_pi<float>())
            angle -= glm::two_pi<float>();

        auto i = size_t(0);
        for (const auto [entity, transform, _] : PointLightView.each()) {
            constexpr float radius = 13.0f;

            const auto add = glm::two_pi<float>() * (static_cast<float>(i) / static_cast<float>(PointLightView.size_hint()));
            const auto rad = radius * (0.7f + 0.3f * ((i + 1) % 2));
            const auto pos = glm::vec3(cos(angle + add) * rad, 4.0f + 4.0f * (i % 2), sin(angle + add) * rad);
            transform.Position = pos;
            
            i++;
        }
    }

    _submissionBuffer->ClearEntries();
    for (const auto [entity, transform, render] : GeometryView.each()) {
        auto entry = BeBRPGeometryEntry();
        entry.Model = render.Model;
        entry.CastShadows = render.CastShadows;
        entry.ModelMatrix = BeBRPGeometryEntry::CalculateModelMatrix(
            transform.Position,
            transform.Rotation,
            transform.Scale
        );
        
        _submissionBuffer->SubmitGeometry(entry);
    }
    
    for (const auto [entity, sunLight] : SunView.each()) {
        auto entry = BeBRPSunLightEntry();
        entry.Direction = sunLight.Direction;
        entry.Color = sunLight.Color;
        entry.Power = sunLight.Power;
        entry.CastsShadows = sunLight.CastsShadows;
        entry.ShadowMapResolution = sunLight.ShadowMapResolution;
        entry.ShadowMap = sunLight.ShadowMap;
        entry.ShadowViewProjection = BeBRPSunLightEntry::CalculateViewProj(
            entry.Direction,
            sunLight.ShadowCameraDistance,
            sunLight.ShadowMapWorldSize,
            sunLight.ShadowNearPlane,
            sunLight.ShadowFarPlane
        );
        
        _submissionBuffer->SubmitSunLight(entry);
    }
    
    for (const auto [entity, transform, pointLight] : PointLightView.each()) {
        auto entry = BeBRPPointLightEntry{
            .Position = transform.Position,
            .Radius = pointLight.Radius,
            .Color = pointLight.Color,
            .Power = pointLight.Power,
            .CastsShadows = pointLight.CastsShadows,
            .ShadowMapResolution = pointLight.ShadowMapResolution,
            .ShadowNearPlane = pointLight.ShadowNearPlane,
            .ShadowMap = pointLight.ShadowMap
        };
        _submissionBuffer->SubmitPointLight(entry);
    };
}




