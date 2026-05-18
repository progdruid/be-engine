
#include "SakuraScene.h"

#include <umbrellas/include-glfw.h>

#include "BeRenderPass.h"
#include "OrbitCameraController.h"
#include "FreeCameraController.h"
#include "BeAssetRegistry.h"
#include "BeCamera.h"
#include "BeInput.h"
#include "BeMaterial.h"
#include "BeMeshPrimitives.h"
#include "BeProp.h"
#include "BeRenderer.h"
#include "BeShader.h"
#include "BeTexture.h"
#include "BeWindow.h"
#include "Components.h"
#include "Game.h"
#include "scenes/BeSceneManager.h"
#include "standard-render-machine/BeStandardRenderMachine.h"

SakuraScene::SakuraScene(Game* game) : BaseScene(game) {}
SakuraScene::~SakuraScene() = default;

auto SakuraScene::Prepare() -> void {
    _camera = std::make_unique<BeCamera>();
    _camera->Width = GameIns->Window->GetWidth();
    _camera->Height = GameIns->Window->GetHeight();
    _camera->NearPlane = 0.1f;
    _camera->FarPlane = 200.0f;
    _orbitCameraController = std::make_unique<OrbitCameraController>(_camera.get());
    _freeCameraController = std::make_unique<FreeCameraController>(_camera.get());

    BeAssetRegistry::IndexShaderFiles({
        "assets/shaders/uniform-material.hlsl",
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
        "assets/shaders/fxaa.hlsl",
    });

    const auto standardShader    = BeAssetRegistry::GetShader("standard");
    const auto checkerboardShader = BeAssetRegistry::GetShader("checkerboard");

    _cube = BeProp::FromMesh(BeMeshPrimitives::Cube(), checkerboardShader);
    _cube->Materials[0]->SetTexture("DiffuseTexture",
        BeTexture::Create("Sakura_Checkerboard")
        .LoadFromFile("assets/checkerboard.png")
        .AddToRegistry()
        .Build()
    );

    _emissiveCube = BeProp::FromMesh(BeMeshPrimitives::Cube(), standardShader);
    _emissiveCube->Materials[0]->SetFloat3("EmissiveColor", glm::vec3(0.99f, 0.8f, 0.6f) * 1.7f);

    _moon = BeProp::FromMesh(BeMeshPrimitives::Cube(), standardShader);
    _moon->Materials[0]->SetFloat3("EmissiveColor", glm::vec3(0.7f, 0.7f, 0.99f) * 2.1f);

    _anvil = BeProp::Create("assets/anvil/scene.gltf", standardShader, *GameIns->Renderer);
    _anvil->Materials[0]->SetSampler("InputSampler", BeAssetRegistry::GetSampler("point-clamp"));

    _sakura = BeProp::Create("assets/sakura/scene.gltf", standardShader, *GameIns->Renderer);
    _sakura->Materials[0]->SetSampler("InputSampler", BeAssetRegistry::GetSampler("linear-wrap"));

    _sakura2 = BeProp::Create("assets/stylized_sakura_tree.glb", standardShader, *GameIns->Renderer);

    _testSphere = BeProp::FromMesh(BeMeshPrimitives::Sphere(), standardShader);
    _testSphere->Materials[0]->SetFloat3("BaseColor", glm::vec3(0.8f, 0.3f, 0.1f));
    _testSphere->Materials[0]->SetFloat("Metallic", 0.0f);
    _testSphere->Materials[0]->SetFloat("Roughness", 0.5f);

    _uniformMaterial = BeMaterial::Create("uniform-material", false);
    _uniformMaterial->SetFloat3("AmbientColor", glm::vec3(0.1f));

    const uint32_t screenWidth  = GameIns->Window->GetFramebufferWidth();
    const uint32_t screenHeight = GameIns->Window->GetFramebufferHeight();

    _machine = std::make_unique<BeStandardRenderMachine>(GameIns->Renderer, screenWidth, screenHeight);

    _machine->RegisterMesh(_cube->Mesh);
    _machine->RegisterMesh(_anvil->Mesh);
    _machine->RegisterMesh(_sakura->Mesh);
    _machine->RegisterMesh(_sakura2->Mesh);
    _machine->RegisterMesh(_emissiveCube->Mesh);
    _machine->RegisterMesh(_moon->Mesh);
    _machine->RegisterMesh(_testSphere->Mesh);
    _machine->BakeMeshes();

    _machine->DeclareGBufferTarget("Sakura_Albedo_RGB", SenFormat::R11G11B10_Float);
    _machine->DeclareGBufferTarget("Sakura_WorldNormal_XYZ", SenFormat::RGBA16_Float);
    _machine->DeclareGBufferTarget("Sakura_ORM_RGB",            SenFormat::RGBA8_Unorm);
    _machine->DeclareGBufferTarget("Sakura_Emissive_RGB",              SenFormat::R11G11B10_Float);
    _machine->DeclareDepth        ("Sakura_Depth",                     SenFormat::Depth32);
    _machine->DeclareTexture      ("Sakura_HDR",                       SenFormat::R11G11B10_Float);
    _machine->DeclareTexture      ("Sakura_Bloom",                     SenFormat::R11G11B10_Float);
    _machine->DeclareTexture      ("Sakura_Tonemapper",                SenFormat::R11G11B10_Float);
    _machine->DeclareTexture      ("Sakura_FXAA",                      SenFormat::R11G11B10_Float);

    _machine->AddShadowPass();
    _machine->AddGeometryPass();
    _machine->AddLightingPass("Sakura_HDR");

    const auto dirtTexture = BeTexture::Create("Sakura_BloomDirtTexture")
        .LoadFromFile("assets/bloom-dirt-mask.png")
        .Build();
    _machine->AddBloomPass(5, "Sakura_HDR", "Sakura_Bloom", dirtTexture);

    const auto tonemapperMaterial = BeMaterial::Create("tonemapper-material", false);
    tonemapperMaterial->SetTexture("HDRInput", _machine->GetRenderTexture("Sakura_Bloom"));
    //tonemapperMaterial->SetTexture("HDRInput", _machine->GetRenderTexture("Sakura_HDR"));
    _machine->AddFullscreenPass(BeAssetRegistry::GetShader("tonemapper"), tonemapperMaterial, { "Sakura_Tonemapper" });

    const auto fxaaMaterial = BeMaterial::Create("fxaa-material", false);
    fxaaMaterial->SetTexture("ColorTexture", _machine->GetRenderTexture("Sakura_Tonemapper"));
    _machine->AddFullscreenPass(BeAssetRegistry::GetShader("fxaa"), fxaaMaterial, { "Sakura_FXAA" });

    _machine->AddBackbufferPass("Sakura_FXAA", { 0.f / 255.f, 23.f / 255.f, 31.f / 255.f });
}

auto SakuraScene::OnLoad() -> void {
    _registry.clear();

    _machine->UniformMaterial = _uniformMaterial;
    _machine->Build();

    CreateEntity(_registry
        ,NameComponent { .Name = "Cube" }
        ,TransformComponent { .Position = glm::vec3(0, -15, 0), .Rotation = glm::quat(), .Scale = glm::vec3(30) }
        ,RenderComponent { .Prop = _cube, .CastShadows = true }
    );
    CreateEntity(_registry
        ,NameComponent { .Name = "Anvil1" }
        ,RenderComponent { .Prop = _anvil }
        ,TransformComponent { .Position = {0, 0, 0}, .Rotation = glm::quat(glm::vec3(0, 0_rad, 0)), .Scale = glm::vec3(0.2f) }
    );
    CreateEntity(_registry
        ,NameComponent { .Name = "PBR_TestSphere" }
        ,RenderComponent { .Prop = _testSphere, .CastShadows = true }
        ,TransformComponent { .Position = glm::vec3(8, 1, 0), .Scale = glm::vec3(1.5f) }
    );
    CreateEntity(_registry
        ,NameComponent { .Name = "PBR_TestLight" }
        ,StaticTag {}
        ,TransformComponent { .Position = glm::vec3(8, 4, 2), .Scale = glm::vec3(0.2f) }
        ,RenderComponent { .Prop = _emissiveCube, .CastShadows = false }
        ,PointLightComponent {
            .Radius = 12.0f,
            .Color = glm::vec3(1.0f, 0.95f, 0.85f),
            .Power = 3.0f,
            .CastsShadows = false,
            .ShadowMapResolution = 512,
            .ShadowNearPlane = 0.1f,
            .ShadowMap = BeTexture::Create("Sakura_PBRTestLight_ShadowMap")
                .SetUsage(SenTextureUsage::DepthStencil | SenTextureUsage::ShaderResource)
                .SetFormat(SenFormat::Depth32)
                .SetCubemap(true)
                .SetSize(512, 512)
                .AddToRegistry()
                .Build()
        }
    );
    CreateEntity(_registry
        ,NameComponent { .Name = "Sakura2" }
        ,RenderComponent { .Prop = _sakura2 }
        ,TransformComponent { .Position = {-3.f, -5.5, 2}, .Rotation = glm::quat(glm::vec3(0, 45.0_rad, 0)), .Scale = glm::vec3(5.0f) }
    );
    CreateEntity(_registry
        ,NameComponent { .Name = "Moon" }
        ,TransformComponent { .Position = glm::vec3(100, 150, 100), .Scale = glm::vec3(6.f) }
        ,RenderComponent { .Prop = _moon, .CastShadows = false }
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
            .ShadowMap = BeTexture::Create("Sakura_SunLightShadowMap")
                .SetUsage(SenTextureUsage::DepthStencil | SenTextureUsage::ShaderResource)
                .SetFormat(SenFormat::Depth32)
                .SetSize(4096, 4096)
                .AddToRegistry()
                .Build()
        }
    );

    for (uint32_t i = 0; i < 0; ++i) {
        CreateEntity(_registry
            ,NameComponent { .Name = "PointLight_" + std::to_string(i) }
            ,TransformComponent { .Scale = glm::vec3(0.5f) }
            ,PointLightComponent {
                .Radius = 20.0f,
                .Color = glm::vec3(0.99f, 0.8f, 0.6f),
                .Power = (1.0f / 0.7f) * 1.7f,
                .CastsShadows = true,
                .ShadowMapResolution = 2048,
                .ShadowNearPlane = 0.1f,
                .ShadowMap =
                    BeTexture::Create("Sakura_PointLight" + std::to_string(i) + "_ShadowMap")
                    .SetUsage(SenTextureUsage::DepthStencil | SenTextureUsage::ShaderResource)
                    .SetFormat(SenFormat::Depth32)
                    .SetCubemap(true)
                    .SetSize(2048, 2048)
                    .AddToRegistry()
                    .Build()
            }
            ,RenderComponent { .Prop = _emissiveCube, .CastShadows = false }
        );
    }

    srand(time(0));
    for (uint32_t i = 0; i < 100; ++i) {
        auto randFloat = [](float min, float max) -> float {
            return min + float(rand()) / float(RAND_MAX) * (max - min);
        };
        CreateEntity(_registry
            ,NameComponent { .Name = "Star_" + std::to_string(i) }
            ,TransformComponent { .Position = glm::vec3(randFloat(-50.f, 50.f), randFloat(30.f, 60.f), randFloat(-50.f, 50.f)), .Scale = glm::vec3(0.1f) }
            ,RenderComponent { .Prop = _emissiveCube, .CastShadows = false }
        );
    }
}

auto SakuraScene::Tick(float deltaTime) -> void {
    if (GameIns->Input->GetKeyDown(GLFW_KEY_ESCAPE)) {
        GameIns->Input->SetMouseCapture(false);
        GameIns->SceneManager->RequestSceneChange("menu");
    }

    static const auto GeometryView = _registry.view<NameComponent, TransformComponent, RenderComponent>();
    static const auto SunView = _registry.view<SunLightComponent>();
    static const auto PointLightView = _registry.view<NameComponent, TransformComponent, PointLightComponent>();
    static const auto OrbitingLightView = _registry.view<NameComponent, TransformComponent, PointLightComponent>(entt::exclude<StaticTag>);

    if (GameIns->Input->GetKeyDown(GLFW_KEY_C)) {
        _useOrbitCamera = !_useOrbitCamera;
    }

    if (_useOrbitCamera) {
        GameIns->Input->SetMouseCapture(false);
        _orbitCameraController->Update(deltaTime, GameIns->Input.get());
    } else {
        _freeCameraController->Update(deltaTime, GameIns->Input.get());
    }

    auto& uniformMat = *_uniformMaterial;
    const auto projView = _camera->GetProjectionMatrix() * _camera->GetViewMatrix();
    uniformMat.SetMatrix("CameraProjectionView", projView);
    uniformMat.SetMatrix("CameraInverseProjectionView", glm::inverse(projView));
    uniformMat.SetFloat4("NearFarPlane", { _camera->NearPlane, _camera->FarPlane, 1.0f / _camera->NearPlane, 1.0f / _camera->FarPlane });
    uniformMat.SetFloat3("CameraPosition", _camera->Position);

    {
        static float angle = 0.0f;
        angle += deltaTime * glm::radians(15.0f);
        if (angle > glm::two_pi<float>()) {
            angle -= glm::two_pi<float>();
        }

        auto i = size_t(0);
        for (const auto [entity, name, transform, _] : OrbitingLightView.each()) {
            constexpr float radius = 13.0f;
            const auto add = glm::two_pi<float>() * (float(i) / float(OrbitingLightView.size_hint()));
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
