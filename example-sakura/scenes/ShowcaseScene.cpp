#include "ShowcaseScene.h"

#include <glfw/glfw3.h>

#include "OrbitCameraController.h"
#include "FreeCameraController.h"
#include "BeCamera.h"
#include "BeInput.h"
#include "BeMaterial.h"
#include "BeMeshPrimitives.h"
#include "BeProp.h"
#include "BeRenderer.h"
#include "BeAssetRegistry.h"
#include "BeTexture.h"
#include "BeWindow.h"
#include "Components.h"
#include "Game.h"
#include "basic-render-pipeline/BeBackbufferPass.h"
#include "basic-render-pipeline/BeBloomPass.h"
#include "basic-render-pipeline/BeFullscreenEffectPass.h"
#include "basic-render-pipeline/BeGeometryPass.h"
#include "basic-render-pipeline/BeLightingPass.h"
#include "basic-render-pipeline/BeShadowPass.h"
#include "scenes/BeSceneManager.h"

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
    
    CreateTargetTextures();
    LoadModels();
    CreateObjects();
    
    GameIns->Renderer->UniformData.AmbientColor = glm::vec3(0.1f);
}

auto ShowcaseScene::CreateTargetTextures() -> void {
    const uint32_t screenWidth = GameIns->Window->GetWidth();
    const uint32_t screenHeight = GameIns->Window->GetHeight();
    
    
    BeTexture::Create("S_DepthStencil")
    .SetUsage(SenTextureUsage::DepthStencil | SenTextureUsage::ShaderResource)
    .SetFormat(SenFormat::Depth32)
    .SetSize(screenWidth, screenHeight)
    .AddToRegistry()
    .Build();

    BeTexture::Create("S_BaseColor")
    .SetUsage(SenTextureUsage::RenderTarget | SenTextureUsage::ShaderResource)
    .SetFormat(SenFormat::R11G11B10_Float)
    .SetSize(screenWidth, screenHeight)
    .AddToRegistry()
    .Build();

    BeTexture::Create("S_WorldNormal")
    .SetUsage(SenTextureUsage::RenderTarget | SenTextureUsage::ShaderResource)
    .SetFormat(SenFormat::RGBA16_Float)
    .SetSize(screenWidth, screenHeight)
    .AddToRegistry()
    .Build();

    BeTexture::Create("S_Specular-Shininess")
    .SetUsage(SenTextureUsage::RenderTarget | SenTextureUsage::ShaderResource)
    .SetFormat(SenFormat::RGBA8_Unorm)
    .SetSize(screenWidth, screenHeight)
    .AddToRegistry()
    .Build();

    BeTexture::Create("S_Emissive")
    .SetUsage(SenTextureUsage::RenderTarget | SenTextureUsage::ShaderResource)
    .SetFormat(SenFormat::R11G11B10_Float)
    .SetSize(screenWidth, screenHeight)
    .AddToRegistry()
    .Build();
}

auto ShowcaseScene::LoadModels() -> void {
    
    
    auto standardShader = BeAssetRegistry::GetShader("standard");
    
    
    auto ramen = BeProp::Create("assets/ramen/scene.gltf", standardShader, *GameIns->Renderer);
    BeAssetRegistry::AddProp("ramen", ramen);

    auto stillLife = BeProp::Create("assets/still-life/scene.gltf", standardShader, *GameIns->Renderer);
    BeAssetRegistry::AddProp("still-life", stillLife);

    auto fiestaTea = BeProp::Create("assets/fiesta_tea/scene.gltf", standardShader, *GameIns->Renderer);
    BeAssetRegistry::AddProp("fiesta-tea", fiestaTea);

    auto honeydew_melons = BeProp::Create("assets/honeydew_melons/scene.gltf", standardShader, *GameIns->Renderer);
    BeAssetRegistry::AddProp("honeydew_melons", honeydew_melons);

    auto hunger_games = BeProp::Create("assets/hunger_games/scene.gltf", standardShader, *GameIns->Renderer);
    BeAssetRegistry::AddProp("hunger_games", hunger_games);

    auto pickles = BeProp::Create("assets/pickles/scene.gltf", standardShader, *GameIns->Renderer);
    BeAssetRegistry::AddProp("pickles", pickles);

    auto watermelons = BeProp::Create("assets/watermelons/scene.gltf", standardShader, *GameIns->Renderer);
    BeAssetRegistry::AddProp("watermelons", watermelons);

    auto apfel = BeProp::Create("assets/apfel/scene.gltf", standardShader, *GameIns->Renderer);
    BeAssetRegistry::AddProp("apfel", apfel);

    auto eggplant = BeProp::Create("assets/eggplant/scene.gltf", standardShader, *GameIns->Renderer);
    BeAssetRegistry::AddProp("eggplant", eggplant);

    auto tomatoes = BeProp::Create("assets/tomatoes/scene.gltf", standardShader, *GameIns->Renderer);
    BeAssetRegistry::AddProp("tomatoes", tomatoes);


    auto skycube = BeProp::FromMesh(BeMeshPrimitives::Cube(), standardShader, *GameIns->Renderer);
    skycube->Materials[0]->SetFloat3("DiffuseColor", HexColor("#FAC8CD"));
    skycube->Slices[0].TwoSided = true;
    BeAssetRegistry::AddProp("skycube", skycube);
    

    GameIns->SubmissionBuffer->RegisterMesh(skycube->Mesh);
    GameIns->SubmissionBuffer->RegisterMesh(ramen->Mesh);
    GameIns->SubmissionBuffer->RegisterMesh(stillLife->Mesh);
    GameIns->SubmissionBuffer->RegisterMesh(fiestaTea->Mesh);
    GameIns->SubmissionBuffer->RegisterMesh(honeydew_melons->Mesh);
    GameIns->SubmissionBuffer->RegisterMesh(hunger_games->Mesh);
    GameIns->SubmissionBuffer->RegisterMesh(pickles->Mesh);
    GameIns->SubmissionBuffer->RegisterMesh(watermelons->Mesh);
    GameIns->SubmissionBuffer->RegisterMesh(apfel->Mesh);
    GameIns->SubmissionBuffer->RegisterMesh(eggplant->Mesh);
    GameIns->SubmissionBuffer->RegisterMesh(tomatoes->Mesh);
}

auto ShowcaseScene::CreateObjects() -> void {
    
    
    
    CreateEntity(_registry
        ,NameComponent { .Name = "showcased-object" }
        ,TransformComponent { }
        ,RenderComponent { .Prop = BeAssetRegistry::GetProp("ramen").lock(), .CastShadows = false }
    );
    
    CreateEntity(_registry
        ,NameComponent { .Name = "skycube" }
        ,TransformComponent { .Position = glm::vec3(0, 0, 0), .Rotation = glm::quat(), .Scale = glm::vec3(100) }
        ,RenderComponent { .Prop = BeAssetRegistry::GetProp("skycube").lock(), .CastShadows = false }
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
            .ShadowMap = BeTexture::Create("ShowcaseScene_SunLightShadowMap")
                .SetUsage(SenTextureUsage::DepthStencil | SenTextureUsage::ShaderResource)
                .SetFormat(SenFormat::Depth32)
                .SetSize(4096, 4096)
                .AddToRegistry()
                .Build()
        }
    );
}


void ShowcaseScene::OnLoad() {
    LoadPasses();
}

auto ShowcaseScene::LoadPasses() -> void {
    
    
    GameIns->Renderer->ClearPasses();

    const auto shadowPass = new BeShadowPass();
    GameIns->Renderer->AddRenderPass(shadowPass);
    shadowPass->SubmissionBuffer = GameIns->SubmissionBuffer;

    const auto geometryPass = new BeGeometryPass();
    GameIns->Renderer->AddRenderPass(geometryPass);
    geometryPass->SubmissionBuffer = GameIns->SubmissionBuffer;
    geometryPass->OutputDepthTexture = BeAssetRegistry::GetTexture("S_DepthStencil");
    geometryPass->OutputTexture0 = BeAssetRegistry::GetTexture("S_BaseColor");
    geometryPass->OutputTexture1 = BeAssetRegistry::GetTexture("S_WorldNormal");
    geometryPass->OutputTexture2 = BeAssetRegistry::GetTexture("S_Specular-Shininess");
    geometryPass->OutputTexture3 = BeAssetRegistry::GetTexture("S_Emissive");

    const auto backbufferPass = new BeBackbufferPass();
    GameIns->Renderer->AddRenderPass(backbufferPass);
    backbufferPass->InputTexture = BeAssetRegistry::GetTexture("S_BaseColor");
    backbufferPass->ClearColor = {0.f / 255.f, 23.f / 255.f, 31.f / 255.f};
    
    GameIns->Renderer->InitialisePasses();
}



void ShowcaseScene::Tick(float deltaTime) {

    if (GameIns->Input->GetKeyDown(GLFW_KEY_ESCAPE)) {
        GameIns->SceneManager->RequestSceneChange("menu");
    }
    
    if (GameIns->Input->GetKeyDown(GLFW_KEY_1)) {
        ChangeShowcase("ramen", "#FAC8CD", TransformComponent());
    }
    else if (GameIns->Input->GetKeyDown(GLFW_KEY_2)) {
        ChangeShowcase("still-life", "#D0D0C4", TransformComponent { .Position = { 0.f, 1.f, 0.f }, .Scale = glm::vec3(4.f) } ); 
    }
    else if (GameIns->Input->GetKeyDown(GLFW_KEY_3)) {
        ChangeShowcase("fiesta-tea", "#61636D", TransformComponent { .Position = { 0, -1, 0}, .Scale = glm::vec3(2.f) });
    }
    else if (GameIns->Input->GetKeyDown(GLFW_KEY_4)) {
        ChangeShowcase("honeydew_melons", "#855C36", TransformComponent { .Position = {0.f, 1.f, 0.f} });
    }
    else if (GameIns->Input->GetKeyDown(GLFW_KEY_5)) {
        ChangeShowcase("hunger_games", "#39708E", TransformComponent { .Position = {0.f, 3.f, 0.f}, .Scale = glm::vec3(2.f) });
    }
    else if (GameIns->Input->GetKeyDown(GLFW_KEY_6)) {
        ChangeShowcase("pickles", "#FEC693", TransformComponent { .Position = {0.f, 1.f, 0.f}, .Scale = glm::vec3(24.f) });
    }
    else if (GameIns->Input->GetKeyDown(GLFW_KEY_7)) {
        ChangeShowcase("watermelons", "#A3A17B", TransformComponent { .Position = {0.f, 1.f, 0.f}, .Scale = glm::vec3(90.f) });
    }
    else if (GameIns->Input->GetKeyDown(GLFW_KEY_8)) {
        ChangeShowcase("apfel", "#73615E", TransformComponent { .Position = {0.f, 1.f, 0.f}, .Scale = glm::vec3(2.f) });
    }
    else if (GameIns->Input->GetKeyDown(GLFW_KEY_9)) {
        ChangeShowcase("eggplant", "#E1D5F2", TransformComponent { .Position = {0.f, 1.f, 0.f}, .Scale = glm::vec3(0.2f) });
    }
    else if (GameIns->Input->GetKeyDown(GLFW_KEY_0)) {
        ChangeShowcase("tomatoes", "#E4FDE1", TransformComponent { .Position = {0.f, 1.f, 0.f}, .Scale = glm::vec3(2.f) });
    }

    // Toggle between cameras with [C]
    if (GameIns->Input->GetKeyDown(GLFW_KEY_C)) {
        _useOrbitCamera = !_useOrbitCamera;
    }
    
    // Update the appropriate camera controller
    if (_useOrbitCamera) {
        GameIns->Input->SetMouseCapture(false);
        _orbitCameraController->Update(deltaTime, GameIns->Input.get());
    } else {
        _freeCameraController->Update(deltaTime, GameIns->Input.get());
    }

    GameIns->Renderer->UniformData.NearFarPlane = {_camera->NearPlane, _camera->FarPlane};
    GameIns->Renderer->UniformData.ProjectionView = _camera->GetProjectionMatrix() * _camera->GetViewMatrix();
    GameIns->Renderer->UniformData.CameraPosition = _camera->Position;

    static const auto GeometryView = _registry.view<TransformComponent, RenderComponent>();
    static const auto SunView = _registry.view<SunLightComponent>();

    GameIns->SubmissionBuffer->ClearEntries();
    for (const auto [entity, transform, render] : GeometryView.each()) {
        auto entry = BeBRPGeometryEntry();
        entry.Prop = render.Prop;
        entry.CastShadows = render.CastShadows;
        entry.ModelMatrix = BeBRPGeometryEntry::CalculateModelMatrix(
            transform.Position,
            transform.Rotation,
            transform.Scale
        );

        GameIns->SubmissionBuffer->SubmitGeometry(entry);
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

        GameIns->SubmissionBuffer->SubmitSunLight(entry);
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
            render.Prop->Materials[0]->SetFloat3("DiffuseColor", HexColor(hxcolor.c_str()));
        }
    }
}

