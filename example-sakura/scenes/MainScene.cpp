
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
#include "basic-pipeline/BeBackbufferPass.h"
#include "basic-pipeline/BeBloomPass.h"
#include "basic-pipeline/BeFullscreenEffectPass.h"
#include "basic-pipeline/BeGeometryPass.h"
#include "basic-pipeline/BeLightingPass.h"
#include "basic-pipeline/BeShadowPass.h"

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
        "assets/shaders/standard.beshade",
        "assets/shaders/objectMaterial.beshade", 
        "assets/shaders/fullscreen-vertex.beshade", 
        "assets/shaders/directionalLight.beshade", 
        "assets/shaders/pointLight.beshade", 
        "assets/shaders/BeBloomAdd.beshade", 
        "assets/shaders/BeBloomBright.beshade", 
        "assets/shaders/BeBloomKawase.beshade", 
        "assets/shaders/tonemapper.beshade", 
        "assets/shaders/backbuffer.beshade", 
    });
    
    const auto standardShader = BeAssetRegistry::GetShader("standard");
    
    _cube = BeModel::Create("assets/cube.glb", standardShader, *_renderer);
    _cube->Materials[0]->SetFloat3("DiffuseColor", glm::vec3(0.28, 0.39, 1.0));
    _anvil = BeModel::Create("assets/anvil/anvil.fbx", standardShader, *_renderer);
    _anvil->Materials[0]->SetFloat3("SpecularColor", glm::vec3(1.0f));

    const std::vector<std::shared_ptr<BeModel>> models {
        _cube, _anvil
    };
    _renderer->RegisterModels(models);

    _renderer->UniformData.AmbientColor = glm::vec3(0.1f);

    _directionalLight = std::make_shared<BeDirectionalLight>();
    _directionalLight->Direction = glm::normalize(glm::vec3(-0.8f, -1.0f, -0.8f));
    _directionalLight->Color = glm::vec3(0.7f, 0.7f, 0.99);
    _directionalLight->Power = (1.0f / 0.7f) * 0.7f;
    _directionalLight->CastsShadows = true;
    _directionalLight->ShadowMapResolution = 4096;
    _directionalLight->ShadowCameraDistance = 100.0f;
    _directionalLight->ShadowMapWorldSize = 60.0f;
    _directionalLight->ShadowNearPlane = 0.1f;
    _directionalLight->ShadowFarPlane = 400.0f;
    _directionalLight->ShadowMap =
    BeTexture::Create("DirectionalLightShadowMap")
    .SetBindFlags(D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE)
    .SetFormat(DXGI_FORMAT_R32_TYPELESS)
    .SetSize(_directionalLight->ShadowMapResolution, _directionalLight->ShadowMapResolution)
    .AddToRegistry()
    .Build(device);
    _directionalLight->CalculateMatrix();

    for (uint32_t i = 0; i < 4; ++i) {
        BePointLight pointLight;

        pointLight.Radius = 20.0f;
        pointLight.Color = glm::vec3(0.99f, 0.8f, 0.6f);
        pointLight.Power = (1.0f / 0.7f) * 2.7f;
        pointLight.CastsShadows = true;
        pointLight.ShadowMapResolution = 2048;
        pointLight.ShadowNearPlane = 0.1f;

        pointLight.ShadowMap =
            BeTexture::Create("PointLight" + std::to_string(i) + "_ShadowMap")
            .SetBindFlags(D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE)
            .SetFormat(DXGI_FORMAT_R32_TYPELESS)
            .SetCubemap(true)
            .SetSize(pointLight.ShadowMapResolution, pointLight.ShadowMapResolution)
            .AddToRegistry()
            .Build(device);

        _pointLights.push_back(pointLight);
    }


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
    
    _renderer->ClearPasses();

    const auto shadowPass = new BeShadowPass();
    _renderer->AddRenderPass(shadowPass);
    shadowPass->DirectionalLight = _directionalLight;
    shadowPass->PointLights = _pointLights;

    const auto geometryPass = new BeGeometryPass();
    _renderer->AddRenderPass(geometryPass);
    geometryPass->OutputDepthTexture = BeAssetRegistry::GetTexture("DepthStencil");
    geometryPass->OutputTexture0 = BeAssetRegistry::GetTexture("BaseColor");
    geometryPass->OutputTexture1 = BeAssetRegistry::GetTexture("WorldNormal");
    geometryPass->OutputTexture2 = BeAssetRegistry::GetTexture("Specular-Shininess");

    const auto lightingPass = new BeLightingPass();
    _renderer->AddRenderPass(lightingPass);
    lightingPass->DirectionalLight = _directionalLight;
    lightingPass->PointLights = _pointLights;
    lightingPass->InputDepthTexture = BeAssetRegistry::GetTexture("DepthStencil");
    lightingPass->InputTexture0 = BeAssetRegistry::GetTexture("BaseColor");
    lightingPass->InputTexture1 = BeAssetRegistry::GetTexture("WorldNormal");
    lightingPass->InputTexture2 = BeAssetRegistry::GetTexture("Specular-Shininess");
    lightingPass->OutputTexture = BeAssetRegistry::GetTexture("HDR-Input");

    BeTexture::Create("BloomDirtTexture")
    .LoadFromFile("assets/bloom-dirt-mask.png")
    .AddToRegistry()
    .BuildNoReturn(_renderer->GetDevice());
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

    const auto tonemapperShader = BeShader::Create("assets/shaders/tonemapper.beshade", *_renderer);
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
    
    auto entity = _registry.create();
    _registry.emplace<NameComponent>(entity, "Cube");
    _registry.emplace<TransformComponent>(entity, glm::vec3(0, 0, 0), glm::quat(glm::vec3(0, 0, 0)), glm::vec3(2.f));
    _registry.emplace<RenderComponent>(entity, _cube, true);
    
    entity = _registry.create();
    _registry.emplace<NameComponent>(entity, "Anvil1");
    _registry.emplace<TransformComponent>(entity, glm::vec3(7, 0, 5), glm::quat(glm::vec3(0, glm::radians(90.f), 0)), glm::vec3(0.2f));
    _registry.emplace<RenderComponent>(entity, _anvil, true);
    
    //entity = _registry.create();
    //_registry.emplace<NameComponent>(entity, "Pagoda");
    //_registry.emplace<TransformComponent>(entity, glm::vec3(0, 0, 8), glm::quat(glm::vec3(0, 0, 0)), glm::vec3(0.2f));
    //_registry.emplace<RenderComponent>(entity, _pagoda, true);
    
}


auto MainScene::Tick(float deltaTime) -> void {
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

        for (int i = 0; i < _pointLights.size(); ++i) {
            constexpr float radius = 13.0f;

            const auto add = glm::two_pi<float>() * (static_cast<float>(i) / static_cast<float>(_pointLights.size()));
            const auto rad = radius * (0.7f + 0.3f * ((i + 1) % 2));
            const auto pos = glm::vec3(cos(angle + add) * rad, 4.0f + 4.0f * (i % 2), sin(angle + add) * rad);

            _pointLights[i].Position = pos;
        }
    }

    const auto renderView = _registry.view<TransformComponent, RenderComponent>();
    renderView.each([this](auto& transform, auto& render) {
        _renderer->SubmitDrawEntry(BeRenderer::DrawEntry{
            .Position = transform.Position,
            .Rotation = transform.Rotation,
            .Scale = transform.Scale,
            .Model = render.Model,
            .CastShadows = render.CastShadows,
        });
    });
}




