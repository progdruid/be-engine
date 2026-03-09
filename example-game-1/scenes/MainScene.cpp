
#include "MainScene.h"

#include <glfw/glfw3.h>

#include "BeAssetRegistry.h"
#include "BeCamera.h"
#include "BeInput.h"
#include "BeMaterial.h"
#include "BeMesh.h"
#include "BeMeshPrimitives.h"
#include "BeProp.h"
#include "BeRenderer.h"
#include "BeShader.h"
#include "BeTexture.h"
#include "BeWindow.h"
#include "Game.h"
#include "basic-render-pipeline/BeBackbufferPass.h"
#include "basic-render-pipeline/BeBloomPass.h"
#include "basic-render-pipeline/BeFullscreenEffectPass.h"
#include "basic-render-pipeline/BeGeometryPass.h"
#include "basic-render-pipeline/BeLightingPass.h"
#include "basic-render-pipeline/BeShadowPass.h"

MainScene::MainScene(Game* game) : BaseScene(game){}

auto MainScene::Prepare() -> void {

    _camera = std::make_unique<BeCamera>(); 
    _camera->Width = GameIns->Window->GetWidth();
    _camera->Height = GameIns->Window->GetHeight();
    _camera->NearPlane = 0.1f;
    _camera->FarPlane = 200.0f;
    
    
    
    BeTexture::Create("white")
    .SetSize(1, 1)
    .SetUsage(SenTextureUsage::ShaderResource)
    .SetFormat(SenFormat::RGBA8_Unorm)
    .FillWithColor(glm::vec4(1.f))
    .AddToRegistry()
    .BuildNoReturn();

    BeTexture::Create("black")
    .SetSize(1, 1)
    .SetUsage(SenTextureUsage::ShaderResource)
    .SetFormat(SenFormat::RGBA8_Unorm)
    .FillWithColor(glm::vec4(0.f, 0.f, 0.f, 1.f))
    .AddToRegistry()
    .BuildNoReturn();

    BeAssetRegistry::InjectRenderer(GameIns->Renderer);
    BeAssetRegistry::IndexShaderFiles({ 
        "assets/shaders/standard.hlsl", 
        "assets/shaders/tessellated.hlsl", 
        "assets/shaders/terrain.hlsl", 
        "assets/shaders/objectMaterial.hlsl", 
        "assets/shaders/fullscreen-vertex.hlsl", 
        "assets/shaders/directionalLight.hlsl", 
        "assets/shaders/pointLight.hlsl", 
        "assets/shaders/BeBloomAdd.hlsl", 
        "assets/shaders/BeBloomBright.hlsl", 
        "assets/shaders/BeBloomKawase.hlsl", 
        "assets/shaders/tonemapper.hlsl", 
        "assets/shaders/backbuffer.hlsl", 
    });
    
    const auto standardShader = BeAssetRegistry::GetShader("standard");
    const auto tessellatedShader = BeAssetRegistry::GetShader("tessellated");
    
    {
        auto planeMesh = BeMeshPrimitives::Plane(63);
        const auto terrainShader = BeAssetRegistry::GetShader("terrain");
        _plane = BeProp::FromMesh(std::move(planeMesh), terrainShader, *GameIns->Renderer);
        _plane->Slices[0].Material->SetFloat("TerrainScale", 200.0f);
        _plane->Slices[0].Material->SetFloat("HeightScale", 100.0f);
    }
    _witchItems = BeProp::Create("assets/witch_items.glb", standardShader, *GameIns->Renderer);
    _cube = BeProp::Create("assets/cube.glb", tessellatedShader, *GameIns->Renderer);
    _cube->Materials[0]->SetFloat3("DiffuseColor", glm::vec3(0.28, 0.39, 1.0));
    _macintosh = BeProp::Create("assets/model.fbx", standardShader, *GameIns->Renderer);
    _pagoda = BeProp::Create("assets/pagoda.glb", standardShader, *GameIns->Renderer);
    _disks = BeProp::Create("assets/floppy-disks.glb", standardShader, *GameIns->Renderer);
    _anvil = BeProp::Create("assets/anvil/anvil.fbx", standardShader, *GameIns->Renderer);
    _anvil->Slices[0].Material->SetFloat3("SpecularColor", glm::vec3(1.0f));

    GameIns->SubmissionBuffer->RegisterMesh(_plane->Mesh);
    GameIns->SubmissionBuffer->RegisterMesh(_witchItems->Mesh);
    GameIns->SubmissionBuffer->RegisterMesh(_cube->Mesh);
    GameIns->SubmissionBuffer->RegisterMesh(_macintosh->Mesh);
    GameIns->SubmissionBuffer->RegisterMesh(_pagoda->Mesh);
    GameIns->SubmissionBuffer->RegisterMesh(_disks->Mesh);
    GameIns->SubmissionBuffer->RegisterMesh(_anvil->Mesh);

    GameIns->Renderer->UniformData.AmbientColor = glm::vec3(0.1f);

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
    .SetUsage(SenTextureUsage::DepthStencil | SenTextureUsage::ShaderResource)
    .SetFormat(SenFormat::Depth32)
    .SetSize(_directionalLight->ShadowMapResolution, _directionalLight->ShadowMapResolution)
    .AddToRegistry()
    .Build();
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
            .SetUsage(SenTextureUsage::DepthStencil | SenTextureUsage::ShaderResource)
            .SetFormat(SenFormat::Depth32)
            .SetCubemap(true)
            .SetSize(pointLight.ShadowMapResolution, pointLight.ShadowMapResolution)
            .AddToRegistry()
            .Build();

        _pointLights.push_back(pointLight);
    }


    const uint32_t screenWidth = GameIns->Window->GetWidth();
    const uint32_t screenHeight = GameIns->Window->GetHeight();
    
    BeTexture::Create("DepthStencil")
    .SetUsage(SenTextureUsage::DepthStencil | SenTextureUsage::ShaderResource)
    .SetFormat(SenFormat::Depth32)
    .SetSize(screenWidth, screenHeight)
    .AddToRegistry()
    .Build();

    BeTexture::Create("BaseColor")
    .SetUsage(SenTextureUsage::RenderTarget | SenTextureUsage::ShaderResource)
    .SetFormat(SenFormat::R11G11B10_Float)
    .SetSize(screenWidth, screenHeight)
    .AddToRegistry()
    .Build();

    BeTexture::Create("WorldNormal")
    .SetUsage(SenTextureUsage::RenderTarget | SenTextureUsage::ShaderResource)
    .SetFormat(SenFormat::RGBA16_Float)
    .SetSize(screenWidth, screenHeight)
    .AddToRegistry()
    .Build();

    BeTexture::Create("Specular-Shininess")
    .SetUsage(SenTextureUsage::RenderTarget | SenTextureUsage::ShaderResource)
    .SetFormat(SenFormat::RGBA8_Unorm)
    .SetSize(screenWidth, screenHeight)
    .AddToRegistry()
    .Build();

    BeTexture::Create("HDR-Input")
    .SetUsage(SenTextureUsage::RenderTarget | SenTextureUsage::ShaderResource)
    .SetFormat(SenFormat::R11G11B10_Float)
    .SetSize(screenWidth, screenHeight)
    .AddToRegistry()
    .Build();

    for (int mip = 0; mip < 5; ++mip) {
        const float multiplier = glm::pow(0.5f, mip);
        const uint32_t mipWidth = screenWidth * multiplier;
        const uint32_t mipHeight = screenHeight * multiplier;

        BeTexture::Create("Bloom_Mip" + std::to_string(mip))
        .SetUsage(SenTextureUsage::RenderTarget | SenTextureUsage::ShaderResource)
        .SetFormat(SenFormat::R11G11B10_Float)
        .SetSize(mipWidth, mipHeight)
        .AddToRegistry()
        .Build();
    }

    BeTexture::Create("BloomOutput")
    .SetUsage(SenTextureUsage::RenderTarget | SenTextureUsage::ShaderResource)
    .SetFormat(SenFormat::R11G11B10_Float)
    .SetSize(screenWidth, screenHeight)
    .AddToRegistry()
    .Build();

    BeTexture::Create("TonemapperOutput")
    .SetUsage(SenTextureUsage::RenderTarget | SenTextureUsage::ShaderResource)
    .SetFormat(SenFormat::R11G11B10_Float)
    .SetSize(screenWidth, screenHeight)
    .AddToRegistry()
    .Build();
}

auto MainScene::OnLoad() -> void {
    
    GameIns->Renderer->ClearPasses();

    const auto shadowPass = new BeShadowPass();
    GameIns->Renderer->AddRenderPass(shadowPass);
    shadowPass->SubmissionBuffer = GameIns->SubmissionBuffer;

    const auto geometryPass = new BeGeometryPass();
    GameIns->Renderer->AddRenderPass(geometryPass);
    geometryPass->SubmissionBuffer = GameIns->SubmissionBuffer;
    geometryPass->OutputDepthTexture = BeAssetRegistry::GetTexture("DepthStencil");
    geometryPass->OutputTexture0 = BeAssetRegistry::GetTexture("BaseColor");
    geometryPass->OutputTexture1 = BeAssetRegistry::GetTexture("WorldNormal");
    geometryPass->OutputTexture2 = BeAssetRegistry::GetTexture("Specular-Shininess");

    const auto lightingPass = new BeLightingPass();
    GameIns->Renderer->AddRenderPass(lightingPass);
    lightingPass->SubmissionBuffer = GameIns->SubmissionBuffer;
    lightingPass->InputDepthTexture = BeAssetRegistry::GetTexture("DepthStencil");
    lightingPass->InputTexture0 = BeAssetRegistry::GetTexture("BaseColor");
    lightingPass->InputTexture1 = BeAssetRegistry::GetTexture("WorldNormal");
    lightingPass->InputTexture2 = BeAssetRegistry::GetTexture("Specular-Shininess");
    lightingPass->OutputTexture = BeAssetRegistry::GetTexture("HDR-Input");

    BeTexture::Create("BloomDirtTexture")
    .LoadFromFile("assets/bloom-dirt-mask.png")
    .AddToRegistry()
    .BuildNoReturn();
    const auto bloomPass = new BeBloomPass();
    GameIns->Renderer->AddRenderPass(bloomPass);
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

    const auto tonemapperShader = BeShader::Create("assets/shaders/tonemapper.hlsl", *GameIns->Renderer);
    const auto& tonemapperScheme = BeAssetRegistry::GetMaterialScheme("tonemapper-material");
    const auto tonemapperMaterial = BeMaterial::Create("TonemapperMaterial", tonemapperScheme, false, *GameIns->Renderer);
    tonemapperMaterial->SetTexture("HDRInput", BeAssetRegistry::GetTexture("BloomOutput").lock());
    const auto tonemapperPass = new BeFullscreenEffectPass();
    GameIns->Renderer->AddRenderPass(tonemapperPass);
    tonemapperPass->OutputTextures = { BeAssetRegistry::GetTexture("TonemapperOutput") };
    tonemapperPass->Shader = tonemapperShader;
    tonemapperPass->Material = tonemapperMaterial;

    const auto backbufferPass = new BeBackbufferPass();
    GameIns->Renderer->AddRenderPass(backbufferPass);
    backbufferPass->InputTexture = BeAssetRegistry::GetTexture("TonemapperOutput");
    backbufferPass->ClearColor = {0.f / 255.f, 23.f / 255.f, 31.f / 255.f};
    
    GameIns->Renderer->InitialisePasses();
    
    auto entity = _registry.create();
    _registry.emplace<NameComponent>(entity, "Macintosh");
    _registry.emplace<TransformComponent>(entity, glm::vec3(0, 0, -6.9), glm::quat(glm::vec3(0, 0, 0)), glm::vec3(1.f));
    _registry.emplace<RenderComponent>(entity, _macintosh, true);

    entity = _registry.create();
    _registry.emplace<NameComponent>(entity, "Plane");
    _registry.emplace<TransformComponent>(entity, glm::vec3(0, 0, 0), glm::quat(glm::vec3(0, 0, 0)), glm::vec3(1.f));
    _registry.emplace<RenderComponent>(entity, _plane, false);
    
    entity = _registry.create();
    _registry.emplace<NameComponent>(entity, "LivingCube");
    _registry.emplace<TransformComponent>(entity, glm::vec3(0, 10, 0), glm::quat(glm::vec3(0, 0, 0)), glm::vec3(2.f));
    _registry.emplace<RenderComponent>(entity, _cube, true);
    
    entity = _registry.create();
    _registry.emplace<NameComponent>(entity, "Pagoda");
    _registry.emplace<TransformComponent>(entity, glm::vec3(0, 0, 8), glm::quat(glm::vec3(0, 0, 0)), glm::vec3(0.2f));
    _registry.emplace<RenderComponent>(entity, _pagoda, true);
    
    entity = _registry.create();
    _registry.emplace<NameComponent>(entity, "WitchItems");
    _registry.emplace<TransformComponent>(entity, glm::vec3(-3, 2, 5), glm::quat(glm::vec3(0, 0, 0)), glm::vec3(3.f));
    _registry.emplace<RenderComponent>(entity, _witchItems, true);
    
    entity = _registry.create();
    _registry.emplace<NameComponent>(entity, "Anvil1");
    _registry.emplace<TransformComponent>(entity, glm::vec3(7, 0, 5), glm::quat(glm::vec3(0, glm::radians(90.f), 0)), glm::vec3(0.2f));
    _registry.emplace<RenderComponent>(entity, _anvil, true);
    
    entity = _registry.create();
    _registry.emplace<NameComponent>(entity, "Anvil2");
    _registry.emplace<TransformComponent>(entity, glm::vec3(-7, 0, -3), glm::quat(glm::vec3(0, glm::radians(-90.f), 0)), glm::vec3(0.2f));
    _registry.emplace<RenderComponent>(entity, _anvil, true);
    
    entity = _registry.create();
    _registry.emplace<NameComponent>(entity, "Anvil3");
    _registry.emplace<TransformComponent>(entity, glm::vec3(-17, -10, -3), glm::quat(glm::vec3(0, glm::radians(-90.f), 0)), glm::vec3(1.0f));
    _registry.emplace<RenderComponent>(entity, _anvil, true);
    
    entity = _registry.create();
    _registry.emplace<NameComponent>(entity, "Disks");
    _registry.emplace<TransformComponent>(entity, glm::vec3(7.5f, 1, -4), glm::quat(glm::vec3(0, glm::radians(150.f), 0)), glm::vec3(1.f));
    _registry.emplace<RenderComponent>(entity, _disks, true);

    
}


auto MainScene::Tick(float deltaTime) -> void {
    constexpr float moveSpeed = 5.0f;
    float speed = moveSpeed * deltaTime;
    if (GameIns->Input->GetKey(GLFW_KEY_LEFT_SHIFT) || (GameIns->Input->IsGamepadConnected() && GameIns->Input->GetGamepadButton(GLFW_GAMEPAD_BUTTON_LEFT_BUMPER))) speed *= 2.0f;
    if (GameIns->Input->GetKey(GLFW_KEY_W)) _camera->Position += _camera->GetFront() * speed;
    if (GameIns->Input->GetKey(GLFW_KEY_S)) _camera->Position -= _camera->GetFront() * speed;
    if (GameIns->Input->GetKey(GLFW_KEY_D)) _camera->Position -= _camera->GetRight() * speed;
    if (GameIns->Input->GetKey(GLFW_KEY_A)) _camera->Position += _camera->GetRight() * speed;
    if (GameIns->Input->GetKey(GLFW_KEY_E)) _camera->Position += glm::vec3(0, 1, 0) * speed;
    if (GameIns->Input->GetKey(GLFW_KEY_Q)) _camera->Position -= glm::vec3(0, 1, 0) * speed;

    // Gamepad movement
    if (GameIns->Input->IsGamepadConnected()) {
        const glm::vec2 leftStick = GameIns->Input->GetGamepadLeftStick();
        _camera->Position += _camera->GetFront() * (leftStick.y * speed);
        _camera->Position -= _camera->GetRight() * (leftStick.x * speed);

        const float verticalInput = GameIns->Input->GetGamepadRightTrigger() - GameIns->Input->GetGamepadLeftTrigger();
        _camera->Position += glm::vec3(0, 1, 0) * (verticalInput * speed);
    }

    bool captureMouse = false;
    if (GameIns->Input->GetMouseButton(GLFW_MOUSE_BUTTON_RIGHT)) {
        constexpr float mouseSens = 0.1f;

        captureMouse = true;
        const glm::vec2 mouseDelta = GameIns->Input->GetMouseDelta();
        _camera->Yaw   -= mouseDelta.x * mouseSens;
        _camera->Pitch -= mouseDelta.y * mouseSens;
        _camera->Pitch = glm::clamp(_camera->Pitch, -89.0f, 89.0f);
    }
    GameIns->Input->SetMouseCapture(captureMouse);

    // Gamepad camera look
    if (GameIns->Input->IsGamepadConnected()) {
        const glm::vec2 rightStick = GameIns->Input->GetGamepadRightStick();
        constexpr float gamepadCameraSens = 100.0f;

        _camera->Yaw   -= rightStick.x * gamepadCameraSens * deltaTime;
        _camera->Pitch += rightStick.y * gamepadCameraSens * deltaTime;
        _camera->Pitch = glm::clamp(_camera->Pitch, -89.0f, 89.0f);
    }

    const glm::vec2 scrollDelta = GameIns->Input->GetScrollDelta();
    if (scrollDelta.y != 0.0f) {
        _camera->Fov -= scrollDelta.y;
        _camera->Fov = glm::clamp(_camera->Fov, 20.0f, 90.0f);
    }

    {
        _camera->Update();
        GameIns->Renderer->UniformData.NearFarPlane = {_camera->NearPlane, _camera->FarPlane};
        GameIns->Renderer->UniformData.ProjectionView = _camera->GetProjectionMatrix() * _camera->GetViewMatrix();
        GameIns->Renderer->UniformData.CameraPosition = _camera->Position;
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
        //GameIns->SubmissionBuffer->SubmitGeometry(BeBRPGeometryEntry{
        //    .Position = transform.Position,
        //    .Rotation = transform.Rotation,
        //    .Scale = transform.Scale,
        //    .Prop = render.Prop,
        //    .CastShadows = render.CastShadows,
        //});
    });
}


