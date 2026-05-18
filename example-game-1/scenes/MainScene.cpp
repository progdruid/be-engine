
#include "MainScene.h"

#include <umbrellas/include-glfw.h>

#include "BeAssetRegistry.h"
#include "BeCamera.h"
#include "BeInput.h"
#include "BeMaterial.h"
#include "BeMeshPrimitives.h"
#include "BeProp.h"
#include "BeRenderer.h"
#include "BeTexture.h"
#include "BeWindow.h"
#include "Game.h"
#include "standard-render-machine/BeStandardRenderMachine.h"

MainScene::MainScene(Game* game) : BaseScene(game) {}
MainScene::~MainScene() = default;

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
        "assets/shaders/uniform-material.hlsl",
        "assets/shaders/standard.hlsl",
        "assets/shaders/tessellated.hlsl",
        "assets/shaders/terrain.hlsl",
        "assets/shaders/objectMaterial.hlsl",
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

    const auto standardShader   = BeAssetRegistry::GetShader("standard");
    const auto tessellatedShader = BeAssetRegistry::GetShader("tessellated");

    {
        auto planeMesh = BeMeshPrimitives::Plane(63);
        const auto terrainShader = BeAssetRegistry::GetShader("terrain");
        _plane = BeProp::FromMesh(std::move(planeMesh), terrainShader);
        _plane->Slices[0].Material->SetFloat("TerrainScale", 200.0f);
        _plane->Slices[0].Material->SetFloat("HeightScale", 100.0f);
    }
    _witchItems = BeProp::Create("assets/witch_items.glb",    standardShader,    *GameIns->Renderer);
    _cube       = BeProp::Create("assets/cube.glb",           tessellatedShader, *GameIns->Renderer);
    _cube->Materials[0]->SetFloat3("BaseColor", glm::vec3(0.28f, 0.39f, 1.0f));
    _macintosh  = BeProp::Create("assets/model.fbx",          standardShader,    *GameIns->Renderer);
    _pagoda     = BeProp::Create("assets/pagoda.glb",         standardShader,    *GameIns->Renderer);
    _disks      = BeProp::Create("assets/floppy-disks.glb",   standardShader,    *GameIns->Renderer);
    _anvil      = BeProp::Create("assets/anvil/anvil.fbx",    standardShader,    *GameIns->Renderer);
    _anvil->Slices[0].Material->SetFloat3("SpecularColor", glm::vec3(1.0f));

    _uniformMaterial = BeMaterial::Create("uniform-material", false);
    _uniformMaterial->SetFloat3("AmbientColor", glm::vec3(0.1f));

    const uint32_t screenWidth  = GameIns->Window->GetWidth();
    const uint32_t screenHeight = GameIns->Window->GetHeight();

    _machine = std::make_unique<BeStandardRenderMachine>(GameIns->Renderer, screenWidth, screenHeight);

    _machine->RegisterMesh(_plane->Mesh);
    _machine->RegisterMesh(_witchItems->Mesh);
    _machine->RegisterMesh(_cube->Mesh);
    _machine->RegisterMesh(_macintosh->Mesh);
    _machine->RegisterMesh(_pagoda->Mesh);
    _machine->RegisterMesh(_disks->Mesh);
    _machine->RegisterMesh(_anvil->Mesh);
    _machine->BakeMeshes();

    _machine->DeclareGBufferTarget("BaseColor",         SenFormat::R11G11B10_Float);
    _machine->DeclareGBufferTarget("WorldNormal",       SenFormat::RGBA16_Float);
    _machine->DeclareGBufferTarget("SpecularShininess", SenFormat::RGBA8_Unorm);
    _machine->DeclareGBufferTarget("Emissive",          SenFormat::R11G11B10_Float);
    _machine->DeclareDepth        ("Depth",             SenFormat::Depth32);
    _machine->DeclareTexture      ("HDR",               SenFormat::R11G11B10_Float);
    _machine->DeclareTexture      ("BloomOutput",       SenFormat::R11G11B10_Float);
    _machine->DeclareTexture      ("TonemapperOutput",  SenFormat::R11G11B10_Float);

    _machine->AddShadowPass();
    _machine->AddGeometryPass();
    _machine->AddLightingPass("HDR");

    const auto dirtTexture = BeTexture::Create("MainScene_BloomDirt")
        .LoadFromFile("assets/bloom-dirt-mask.png")
        .Build();
    _machine->AddBloomPass(5, "HDR", "BloomOutput", dirtTexture);

    const auto tonemapperMaterial = BeMaterial::Create("main-tonemapper-material", false);
    tonemapperMaterial->SetTexture("HDRInput", _machine->GetRenderTexture("BloomOutput"));
    _machine->AddFullscreenPass(BeAssetRegistry::GetShader("tonemapper"), tonemapperMaterial, { "TonemapperOutput" });

    _machine->AddBackbufferPass("TonemapperOutput", { 0.f / 255.f, 23.f / 255.f, 31.f / 255.f });
}

auto MainScene::OnLoad() -> void {
    _registry.clear();

    _machine->UniformMaterial = _uniformMaterial;
    _machine->Build();

    CreateEntity(_registry
        ,NameComponent { .Name = "Macintosh" }
        ,TransformComponent { .Position = glm::vec3(0, 0, -6.9f), .Rotation = glm::quat(glm::vec3(0, 0, 0)), .Scale = glm::vec3(1.f) }
        ,RenderComponent { .Prop = _macintosh, .CastShadows = true }
    );
    CreateEntity(_registry
        ,NameComponent { .Name = "Plane" }
        ,TransformComponent { .Position = glm::vec3(0, 0, 0), .Rotation = glm::quat(glm::vec3(0, 0, 0)), .Scale = glm::vec3(1.f) }
        ,RenderComponent { .Prop = _plane, .CastShadows = false }
    );
    CreateEntity(_registry
        ,NameComponent { .Name = "LivingCube" }
        ,TransformComponent { .Position = glm::vec3(0, 10, 0), .Rotation = glm::quat(glm::vec3(0, 0, 0)), .Scale = glm::vec3(2.f) }
        ,RenderComponent { .Prop = _cube, .CastShadows = true }
    );
    CreateEntity(_registry
        ,NameComponent { .Name = "Pagoda" }
        ,TransformComponent { .Position = glm::vec3(0, 0, 8), .Rotation = glm::quat(glm::vec3(0, 0, 0)), .Scale = glm::vec3(0.2f) }
        ,RenderComponent { .Prop = _pagoda, .CastShadows = true }
    );
    CreateEntity(_registry
        ,NameComponent { .Name = "WitchItems" }
        ,TransformComponent { .Position = glm::vec3(-3, 2, 5), .Rotation = glm::quat(glm::vec3(0, 0, 0)), .Scale = glm::vec3(3.f) }
        ,RenderComponent { .Prop = _witchItems, .CastShadows = true }
    );
    CreateEntity(_registry
        ,NameComponent { .Name = "Anvil1" }
        ,TransformComponent { .Position = glm::vec3(7, 0, 5), .Rotation = glm::quat(glm::vec3(0, glm::radians(90.f), 0)), .Scale = glm::vec3(0.2f) }
        ,RenderComponent { .Prop = _anvil, .CastShadows = true }
    );
    CreateEntity(_registry
        ,NameComponent { .Name = "Anvil2" }
        ,TransformComponent { .Position = glm::vec3(-7, 0, -3), .Rotation = glm::quat(glm::vec3(0, glm::radians(-90.f), 0)), .Scale = glm::vec3(0.2f) }
        ,RenderComponent { .Prop = _anvil, .CastShadows = true }
    );
    CreateEntity(_registry
        ,NameComponent { .Name = "Anvil3" }
        ,TransformComponent { .Position = glm::vec3(-17, -10, -3), .Rotation = glm::quat(glm::vec3(0, glm::radians(-90.f), 0)), .Scale = glm::vec3(1.0f) }
        ,RenderComponent { .Prop = _anvil, .CastShadows = true }
    );
    CreateEntity(_registry
        ,NameComponent { .Name = "Disks" }
        ,TransformComponent { .Position = glm::vec3(7.5f, 1, -4), .Rotation = glm::quat(glm::vec3(0, glm::radians(150.f), 0)), .Scale = glm::vec3(1.f) }
        ,RenderComponent { .Prop = _disks, .CastShadows = true }
    );

    CreateEntity(_registry
        ,NameComponent { .Name = "Sun" }
        ,SunLightComponent {
            .Direction = glm::normalize(glm::vec3(-0.8f, -1.0f, -0.8f)),
            .Color = glm::vec3(0.7f, 0.7f, 0.99f),
            .Power = (1.0f / 0.7f) * 0.7f,
            .CastsShadows = true,
            .ShadowMapResolution = 4096,
            .ShadowCameraDistance = 100.0f,
            .ShadowMapWorldSize = 60.0f,
            .ShadowNearPlane = 0.1f,
            .ShadowFarPlane = 400.0f,
            .ShadowMap = BeTexture::Create("MainScene_SunShadowMap")
                .SetUsage(SenTextureUsage::DepthStencil | SenTextureUsage::ShaderResource)
                .SetFormat(SenFormat::Depth32)
                .SetSize(4096, 4096)
                .AddToRegistry()
                .Build()
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
                .CastsShadows = true,
                .ShadowMapResolution = 2048,
                .ShadowNearPlane = 0.1f,
                .ShadowMap =
                    BeTexture::Create("MainScene_PointLight" + std::to_string(i) + "_ShadowMap")
                    .SetUsage(SenTextureUsage::DepthStencil | SenTextureUsage::ShaderResource)
                    .SetFormat(SenFormat::Depth32)
                    .SetCubemap(true)
                    .SetSize(2048, 2048)
                    .AddToRegistry()
                    .Build()
            }
        );
    }
}

auto MainScene::Tick(float deltaTime) -> void {
    constexpr float moveSpeed = 5.0f;
    float speed = moveSpeed * deltaTime;
    if (GameIns->Input->GetKey(GLFW_KEY_LEFT_SHIFT) || (GameIns->Input->IsGamepadConnected() && GameIns->Input->GetGamepadButton(GLFW_GAMEPAD_BUTTON_LEFT_BUMPER))) {
        speed *= 2.0f;
    }
    if (GameIns->Input->GetKey(GLFW_KEY_W)) { _camera->Position += _camera->GetFront() * speed; }
    if (GameIns->Input->GetKey(GLFW_KEY_S)) { _camera->Position -= _camera->GetFront() * speed; }
    if (GameIns->Input->GetKey(GLFW_KEY_D)) { _camera->Position -= _camera->GetRight() * speed; }
    if (GameIns->Input->GetKey(GLFW_KEY_A)) { _camera->Position += _camera->GetRight() * speed; }
    if (GameIns->Input->GetKey(GLFW_KEY_E)) { _camera->Position += glm::vec3(0, 1, 0) * speed; }
    if (GameIns->Input->GetKey(GLFW_KEY_Q)) { _camera->Position -= glm::vec3(0, 1, 0) * speed; }

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
        auto& uniformMat = *_uniformMaterial;
        const auto projView = _camera->GetProjectionMatrix() * _camera->GetViewMatrix();
        uniformMat.SetMatrix("CameraProjectionView", projView);
        uniformMat.SetMatrix("CameraInverseProjectionView", glm::inverse(projView));
        uniformMat.SetFloat4("NearFarPlane", { _camera->NearPlane, _camera->FarPlane, 1.0f / _camera->NearPlane, 1.0f / _camera->FarPlane });
        uniformMat.SetFloat3("CameraPosition", _camera->Position);
    }

    static const auto GeometryView   = _registry.view<NameComponent, TransformComponent, RenderComponent>();
    static const auto SunView        = _registry.view<SunLightComponent>();
    static const auto PointLightView = _registry.view<NameComponent, TransformComponent, PointLightComponent>();

    {
        static float angle = 0.0f;
        angle += deltaTime * glm::radians(15.0f);
        if (angle > glm::two_pi<float>()) {
            angle -= glm::two_pi<float>();
        }

        auto i = size_t(0);
        for (const auto [entity, name, transform, _] : PointLightView.each()) {
            constexpr float radius = 13.0f;
            const auto add = glm::two_pi<float>() * (float(i) / float(PointLightView.size_hint()));
            const auto rad = radius * (0.7f + 0.3f * ((i + 1) % 2));
            transform.Position = glm::vec3(cos(angle + add) * rad, 4.0f + 4.0f * (i % 2), sin(angle + add) * rad);
            i++;
        }
    }

    _machine->ClearFrame();

    for (const auto [entity, name, transform, render] : GeometryView.each()) {
        _machine->AddGeometry({
            .Name = name.Name,
            .ModelMatrix = BeSRMGeometryEntry::CalculateModelMatrix(transform.Position, transform.Rotation, transform.Scale),
            .Prop = render.Prop,
            .CastShadows = render.CastShadows,
        });
    }

    for (const auto [entity, sunLight] : SunView.each()) {
        _machine->AddSunLight({
            .Direction = sunLight.Direction,
            .Color = sunLight.Color,
            .Power = sunLight.Power,
            .CastsShadows = sunLight.CastsShadows,
            .ShadowViewProjection = BeSRMSunLightEntry::CalculateViewProj(
                sunLight.Direction,
                sunLight.ShadowCameraDistance,
                sunLight.ShadowMapWorldSize,
                sunLight.ShadowNearPlane,
                sunLight.ShadowFarPlane
            ),
            .ShadowMapResolution = sunLight.ShadowMapResolution,
            .ShadowMap = sunLight.ShadowMap,
        });
    }

    for (const auto [entity, name, transform, pointLight] : PointLightView.each()) {
        _machine->AddPointLight({
            .Name = name.Name,
            .Position = transform.Position,
            .Radius = pointLight.Radius,
            .Color = pointLight.Color,
            .Power = pointLight.Power,
            .CastsShadows = pointLight.CastsShadows,
            .ShadowMapResolution = pointLight.ShadowMapResolution,
            .ShadowNearPlane = pointLight.ShadowNearPlane,
            .ShadowMap = pointLight.ShadowMap,
        });
    }
}
