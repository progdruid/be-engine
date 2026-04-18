#include "ShowcaseScene.h"

#include <glfw/glfw3.h>

#include "BeRenderPass.h"
#include "OrbitCameraController.h"
#include "FreeCameraController.h"
#include "BeCamera.h"
#include "BeInput.h"
#include "BeMaterial.h"
#include "BeMeshPrimitives.h"
#include "BeProp.h"
#include "BeRenderer.h"
#include "BeTexture.h"
#include "BeWindow.h"
#include "Components.h"
#include "Game.h"
#include "scenes/BeSceneManager.h"
#include "BeAssetRegistry.h"
#include "standard-render-machine/BeStandardRenderMachine.h"

ShowcaseScene::ShowcaseScene(Game* game) : BaseScene(game) {}
ShowcaseScene::~ShowcaseScene() = default;

void ShowcaseScene::Prepare() {
    _camera = std::make_shared<BeCamera>();
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
        "assets/shaders/fullscreen-vertex.hlsl",
        "assets/shaders/backbuffer.hlsl",
        "assets/shaders/fxaa.hlsl",
    });

    _uniformMaterial = BeMaterial::Create("uniform-material", false);
    _uniformMaterial->SetFloat3("AmbientColor", glm::vec3(0.1f));

    LoadModels();
    CreateObjects();

    const uint32_t screenWidth  = GameIns->Window->GetWidth();
    const uint32_t screenHeight = GameIns->Window->GetHeight();

    _machine = std::make_unique<BeStandardRenderMachine>(GameIns->Renderer, screenWidth, screenHeight);

    for (const auto& name : { 
        "skycube", "ramen", "still-life", "fiesta-tea",
        "honeydew_melons", "hunger_games", "pickles",
        "watermelons", "apfel", "eggplant", "tomatoes" 
    }) {
        if (const auto prop = BeAssetRegistry::GetProp(name).lock()) {
            _machine->RegisterMesh(prop->Mesh);
        }
    }
    _machine->BakeMeshes();

    _machine->DeclareGBufferTarget("Showcase_BaseColor",         SenFormat::R11G11B10_Float);
    _machine->DeclareGBufferTarget("Showcase_WorldNormal",       SenFormat::RGBA16_Float);
    _machine->DeclareGBufferTarget("Showcase_SpecularShininess", SenFormat::RGBA8_Unorm);
    _machine->DeclareGBufferTarget("Showcase_Emissive",          SenFormat::R11G11B10_Float);
    _machine->DeclareDepth        ("Showcase_Depth",             SenFormat::Depth32);
    _machine->DeclareTexture      ("Showcase_FXAAOutput",        SenFormat::R11G11B10_Float);

    _machine->AddGeometryPass();

    const auto fxaaMaterial = BeMaterial::Create("fxaa-material", false);
    fxaaMaterial->SetTexture("ColorTexture", _machine->GetRenderTexture("Showcase_BaseColor"));
    _machine->AddFullscreenPass(BeAssetRegistry::GetShader("fxaa"), fxaaMaterial, { "Showcase_FXAAOutput" });

    _machine->AddBackbufferPass("Showcase_FXAAOutput", { 0.f / 255.f, 23.f / 255.f, 31.f / 255.f });
}

auto ShowcaseScene::LoadModels() -> void {
    auto standardShader = BeAssetRegistry::GetShader("standard");

    BeAssetRegistry::AddProp("ramen",           BeProp::Create("assets/ramen/scene.gltf",           standardShader, *GameIns->Renderer));
    BeAssetRegistry::AddProp("still-life",      BeProp::Create("assets/still-life/scene.gltf",      standardShader, *GameIns->Renderer));
    BeAssetRegistry::AddProp("fiesta-tea",      BeProp::Create("assets/fiesta_tea/scene.gltf",      standardShader, *GameIns->Renderer));
    BeAssetRegistry::AddProp("honeydew_melons", BeProp::Create("assets/honeydew_melons/scene.gltf", standardShader, *GameIns->Renderer));
    BeAssetRegistry::AddProp("hunger_games",    BeProp::Create("assets/hunger_games/scene.gltf",    standardShader, *GameIns->Renderer));
    BeAssetRegistry::AddProp("pickles",         BeProp::Create("assets/pickles/scene.gltf",         standardShader, *GameIns->Renderer));
    BeAssetRegistry::AddProp("watermelons",     BeProp::Create("assets/watermelons/scene.gltf",     standardShader, *GameIns->Renderer));
    BeAssetRegistry::AddProp("apfel",           BeProp::Create("assets/apfel/scene.gltf",           standardShader, *GameIns->Renderer));
    BeAssetRegistry::AddProp("eggplant",        BeProp::Create("assets/eggplant/scene.gltf",        standardShader, *GameIns->Renderer));
    BeAssetRegistry::AddProp("tomatoes",        BeProp::Create("assets/tomatoes/scene.gltf",        standardShader, *GameIns->Renderer));

    auto skycube = BeProp::FromMesh(BeMeshPrimitives::Cube(), standardShader);
    skycube->Materials[0]->SetFloat3("BaseColor", HexColor("#FAC8CD"));
    skycube->Slices[0].TwoSided = true;
    BeAssetRegistry::AddProp("skycube", skycube);
}

auto ShowcaseScene::CreateObjects() -> void {
    CreateEntity(_registry
        ,NameComponent { .Name = "showcased-object" }
        ,TransformComponent { }
        ,RenderComponent { .Prop = BeAssetRegistry::GetProp("ramen").lock(), .CastShadows = true }
    );
    CreateEntity(_registry
        ,NameComponent { .Name = "skycube" }
        ,TransformComponent { .Position = glm::vec3(0), .Rotation = glm::quat(), .Scale = glm::vec3(100) }
        ,RenderComponent { .Prop = BeAssetRegistry::GetProp("skycube").lock(), .CastShadows = true }
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
            .ShadowMap = BeTexture::Create("Showcase_MoonShadowMap")
                .SetUsage(SenTextureUsage::DepthStencil | SenTextureUsage::ShaderResource)
                .SetFormat(SenFormat::Depth32)
                .SetSize(4096, 4096)
                .AddToRegistry()
                .Build()
        }
    );
}

void ShowcaseScene::OnLoad() {
    _machine->UniformMaterial = _uniformMaterial;
    _machine->Build();
}

void ShowcaseScene::Tick(float deltaTime) {
    if (GameIns->Input->GetKeyDown(GLFW_KEY_ESCAPE)) {
        GameIns->SceneManager->RequestSceneChange("menu");
    }

    if (GameIns->Input->GetKeyDown(GLFW_KEY_1)) {
        ChangeShowcase("ramen",          "#FAC8CD", TransformComponent());
    } else if (GameIns->Input->GetKeyDown(GLFW_KEY_2)) {
        ChangeShowcase("still-life",     "#D0D0C4", TransformComponent { .Position = { 0.f, 1.f, 0.f }, .Scale = glm::vec3(4.f) });
    } else if (GameIns->Input->GetKeyDown(GLFW_KEY_3)) {
        ChangeShowcase("fiesta-tea",     "#61636D", TransformComponent { .Position = { 0, -1, 0 }, .Scale = glm::vec3(2.f) });
    } else if (GameIns->Input->GetKeyDown(GLFW_KEY_4)) {
        ChangeShowcase("honeydew_melons","#855C36", TransformComponent { .Position = { 0.f, 1.f, 0.f } });
    } else if (GameIns->Input->GetKeyDown(GLFW_KEY_5)) {
        ChangeShowcase("hunger_games",   "#39708E", TransformComponent { .Position = { 0.f, 3.f, 0.f }, .Scale = glm::vec3(2.f) });
    } else if (GameIns->Input->GetKeyDown(GLFW_KEY_6)) {
        ChangeShowcase("pickles",        "#FEC693", TransformComponent { .Position = { 0.f, 1.f, 0.f }, .Scale = glm::vec3(24.f) });
    } else if (GameIns->Input->GetKeyDown(GLFW_KEY_7)) {
        ChangeShowcase("watermelons",    "#A3A17B", TransformComponent { .Position = { 0.f, 1.f, 0.f }, .Scale = glm::vec3(90.f) });
    } else if (GameIns->Input->GetKeyDown(GLFW_KEY_8)) {
        ChangeShowcase("apfel",          "#73615E", TransformComponent { .Position = { 0.f, 1.f, 0.f }, .Scale = glm::vec3(2.f) });
    } else if (GameIns->Input->GetKeyDown(GLFW_KEY_9)) {
        ChangeShowcase("eggplant",       "#E1D5F2", TransformComponent { .Position = { 0.f, 1.f, 0.f }, .Scale = glm::vec3(0.2f) });
    } else if (GameIns->Input->GetKeyDown(GLFW_KEY_0)) {
        ChangeShowcase("tomatoes",       "#E4FDE1", TransformComponent { .Position = { 0.f, 1.f, 0.f }, .Scale = glm::vec3(2.f) });
    }

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

    static const auto GeometryView = _registry.view<NameComponent, TransformComponent, RenderComponent>();
    static const auto SunView      = _registry.view<SunLightComponent>();

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
}

auto ShowcaseScene::ChangeShowcase(
    const std::string& modelName,
    const std::string& hxcolor,
    const TransformComponent& adjustedTransform
) -> void {
    auto view = _registry.view<NameComponent, TransformComponent, RenderComponent>();
    for (auto [entity, name, transform, render] : view.each()) {
        if (name.Name == "showcased-object") {
            transform = adjustedTransform;
            render.Prop = BeAssetRegistry::GetProp(modelName).lock();
        }
        if (name.Name == "skycube") {
            render.Prop->Materials[0]->SetFloat3("BaseColor", HexColor(hxcolor.c_str()));
        }
    }
}
