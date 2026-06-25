#include "ShowcaseScene.h"

#include <umbrellas/include-glfw.h>

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
#include "BeShader.h"
#include "standard-render-machine/BeStandardRenderMachine.h"

// brace(0.85) -ease-in-> peak(1.2) at t=0.5 -ease-out-> settle(1.0) at t=1.0
// scale crosses 1.0 on the way up at ~t=0.33, which is when the swap fires
static auto ExpandCurve(float t) -> float {
    constexpr float peak = 1.1f;
    t = glm::clamp(t, 0.f, 1.f);
    if (t < 0.5f) {
        float s = t / 0.5f;
        return glm::mix(0.85f, peak, s * s * s);
    } else {
        float s = (t - 0.5f) / 0.5f;
        return glm::mix(peak, 1.0f, 1.f - (1.f - s) * (1.f - s) * (1.f - s));
    }
}

ShowcaseScene::ShowcaseScene(Game* game) : BaseScene(game) {}
ShowcaseScene::~ShowcaseScene() = default;

void ShowcaseScene::Prepare() {
    _camera = std::make_shared<BeCamera>();
    _camera->Width = GameIns->Renderer->GetSwapchainPixelWidth();
    _camera->Height = GameIns->Renderer->GetSwapchainPixelHeight();
    _camera->NearPlane = 0.1f;
    _camera->FarPlane = 200.0f;
    _orbitCameraController = std::make_unique<OrbitCameraController>(_camera.get());
    _freeCameraController = std::make_unique<FreeCameraController>(_camera.get());

    _assetRegistry.IndexShaderFiles({
        "assets/shaders/uniform-material.hlsl",
        "assets/shaders/objectMaterial.hlsl",
        "assets/shaders/standard-pbr.hlsl",
        "assets/shaders/standard-phong.hlsl",
        "assets/shaders/fullscreen-vertex.hlsl",
        "assets/shaders/backbuffer.hlsl",
        "assets/shaders/fxaa.hlsl",
        "assets/shaders/pixelation.hlsl",
        "assets/shaders/test-compute.hlsl",
    });

    const auto& uniformScheme = _assetRegistry.GetMaterialScheme("uniform-material");
    _uniformMaterial = BeMaterial::Create(uniformScheme, false);
    _uniformMaterial->SetFloat3("AmbientColor", glm::vec3(0.1f));

    const uint32_t screenWidth  = GameIns->Renderer->GetSwapchainPixelWidth();
    const uint32_t screenHeight = GameIns->Renderer->GetSwapchainPixelHeight();

    _machine = std::make_unique<BeStandardRenderMachine>(GameIns->Renderer, _assetRegistry, screenWidth, screenHeight);

    LoadModels(*_machine);
    CreateObjects();

    _machine->BakeMeshes();

    _machine->DeclareGBufferTarget("Showcase_BaseColor",         SenFormat::R11G11B10_Float);
    _machine->DeclareGBufferTarget("Showcase_WorldNormal",       SenFormat::RGBA16_Float);
    _machine->DeclareGBufferTarget("Showcase_SpecularShininess", SenFormat::RGBA8_Unorm);
    _machine->DeclareGBufferTarget("Showcase_Emissive",          SenFormat::R11G11B10_Float);
    _machine->DeclareDepth        ("Showcase_Depth",             SenFormat::Depth32);
    _machine->DeclareTexture      ("Showcase_FXAAOutput",        SenFormat::R11G11B10_Float);
    _machine->DeclareTexture      ("Showcase_PixelOutput",       SenFormat::R11G11B10_Float);
}

auto ShowcaseScene::LoadModels(BeStandardRenderMachine& machine) -> void {
    auto standardShader = _assetRegistry.GetShader("standard-pbr");

    _assetRegistry.AddProp("ramen",           machine.LoadProp("assets/ramen/scene.gltf",            standardShader));
    _assetRegistry.AddProp("still-life",      machine.LoadProp("assets/still-life/scene.gltf",       standardShader));
    _assetRegistry.AddProp("fiesta-tea",      machine.LoadProp("assets/fiesta_tea/scene.gltf",       standardShader));
    _assetRegistry.AddProp("honeydew_melons", machine.LoadProp("assets/honeydew_melons/scene.gltf",  standardShader));
    _assetRegistry.AddProp("hunger_games",    machine.LoadProp("assets/hunger_games/scene.gltf",     standardShader));
    _assetRegistry.AddProp("pickles",         machine.LoadProp("assets/pickles/scene.gltf",          standardShader));
    _assetRegistry.AddProp("watermelons",     machine.LoadProp("assets/watermelons/scene.gltf",      standardShader));
    _assetRegistry.AddProp("apfel",           machine.LoadProp("assets/apfel/scene.gltf",            standardShader));
    _assetRegistry.AddProp("eggplant",        machine.LoadProp("assets/eggplant/scene.gltf",         standardShader));
    _assetRegistry.AddProp("tomatoes",        machine.LoadProp("assets/tomatoes/scene.gltf",         standardShader));
    
    const auto flowerPot = machine.LoadProp("assets/pixel-flower-pot/scene.gltf", standardShader);
    _assetRegistry.AddProp("flower-pot", flowerPot);
    for (const auto& m : flowerPot->Materials) {
        m->SetSampler("InputSampler", BeAssetRegistry::GetSampler("point-clamp"));
    }
    
    const auto headset = machine.LoadProp("assets/headset/scene.gltf", standardShader);
    _assetRegistry.AddProp("headset", headset);
    for (const auto& m : headset->Materials) {
        m->SetSampler("InputSampler", BeAssetRegistry::GetSampler("point-clamp"));
    }

    const auto skycube = BeProp::FromMesh(BeMeshPrimitives::Cube(), standardShader, "geometry-main");
    skycube->Materials[0]->SetFloat3("BaseColor", HexColor("#FAC8CD"));
    skycube->Slices[0].TwoSided = true;
    _assetRegistry.AddProp("skycube", skycube);
    machine.RegisterMesh(skycube->Mesh);
}

auto ShowcaseScene::CreateObjects() -> void {
    _showcasedEntity = CreateEntity(_registry
        ,NameComponent { .Name = "showcased-object" }
        ,TransformComponent { }
        ,RenderComponent { .Prop = _assetRegistry.GetProp("ramen").lock(), .CastShadows = true }
    );
    CreateEntity(_registry
        ,NameComponent { .Name = "skycube" }
        ,TransformComponent { .Position = glm::vec3(0), .Rotation = glm::quat(), .Scale = glm::vec3(100) }
        ,RenderComponent { .Prop = _assetRegistry.GetProp("skycube").lock(), .CastShadows = true }
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
                .Build()
            ,
        }
    );
}

void ShowcaseScene::OnLoad() {
    _machine->UniformMaterial = _uniformMaterial;
    LoadPasses();
}

void ShowcaseScene::LoadPasses() {
    _machine->ClearPasses();
    _machine->AddGeometryPass();

    const auto& fxaaScheme = _assetRegistry.GetShader("fxaa").lock()->GetMaterialScheme("main");
    const auto fxaaMaterial = BeMaterial::Create(fxaaScheme, false);
    fxaaMaterial->SetTexture("ColorTexture", _machine->GetRenderTexture("Showcase_BaseColor"));
    _machine->AddFullscreenPass(_assetRegistry.GetShader("fxaa"), fxaaMaterial, { "Showcase_FXAAOutput" });

    const char* backbufferInput = "Showcase_FXAAOutput";
    if (_pixelationEnabled) {
        const auto& pixelScheme = _assetRegistry.GetShader("pixelation").lock()->GetMaterialScheme("main");
        const auto pixelMaterial = BeMaterial::Create(pixelScheme, false);
        pixelMaterial->SetTexture("ColorTexture", _machine->GetRenderTexture("Showcase_FXAAOutput"));
        pixelMaterial->SetTexture("DepthTexture", _machine->GetRenderTexture("Showcase_Depth"));
        pixelMaterial->SetFloat1("PixelSize", _pixelSize);
        pixelMaterial->SetFloat1("EdgeEnabled", _pixelEdgesEnabled ? 1.0f : 0.0f);
        _machine->AddFullscreenPass(_assetRegistry.GetShader("pixelation"), pixelMaterial, { "Showcase_PixelOutput" });
        backbufferInput = "Showcase_PixelOutput";
    }

    _machine->AddBackbufferPass(backbufferInput, { 0.f / 255.f, 23.f / 255.f, 31.f / 255.f });
    _machine->BuildPasses();
}

void ShowcaseScene::Tick(float deltaTime) {
    if (GameIns->Input->GetKeyDown(GLFW_KEY_ESCAPE)) {
        GameIns->SceneManager->RequestSceneChange("menu");
    }

    auto startBrace = [&](int key, const char* model, const char* color, TransformComponent t) {
        if (!_animatedTransitions) {
            ChangeShowcase(model, color, t);
            return;
        }
        _pendingModel = model;
        _pendingColor = color;
        _pendingTransform = t;
        _heldKey = key;
        _popState = PopState::Bracing;
        _braceTime = 0.f;
    };

    if (GameIns->Input->GetKeyDown(GLFW_KEY_1)) {
        startBrace(GLFW_KEY_1, "ramen",          "#FAC8CD", TransformComponent());
    } else if (GameIns->Input->GetKeyDown(GLFW_KEY_2)) {
        startBrace(GLFW_KEY_2, "still-life",     "#D0D0C4", TransformComponent { .Position = { 0.f, 1.f, 0.f }, .Scale = glm::vec3(4.f) });
    } else if (GameIns->Input->GetKeyDown(GLFW_KEY_3)) {
        startBrace(GLFW_KEY_3, "headset",        "#84DCC6", TransformComponent { .Position = { 0, -0.5, 0 }, .Scale = glm::vec3(2.f) });
    } else if (GameIns->Input->GetKeyDown(GLFW_KEY_4)) {
        startBrace(GLFW_KEY_4, "honeydew_melons","#855C36", TransformComponent { .Position = { 0.f, 1.f, 0.f } });
    } else if (GameIns->Input->GetKeyDown(GLFW_KEY_5)) {
        startBrace(GLFW_KEY_5, "hunger_games",   "#39708E", TransformComponent { .Position = { 0.f, 3.f, 0.f }, .Scale = glm::vec3(2.f) });
    } else if (GameIns->Input->GetKeyDown(GLFW_KEY_6)) {
        startBrace(GLFW_KEY_6, "flower-pot",     "#E5D372", TransformComponent { .Position = { 0.f, -2.f, 0.f }, .Scale = glm::vec3(2.f) });
    } else if (GameIns->Input->GetKeyDown(GLFW_KEY_7)) {
        startBrace(GLFW_KEY_7, "watermelons",    "#A3A17B", TransformComponent { .Position = { 0.f, 1.f, 0.f }, .Scale = glm::vec3(90.f) });
    } else if (GameIns->Input->GetKeyDown(GLFW_KEY_8)) {
        startBrace(GLFW_KEY_8, "apfel",          "#73615E", TransformComponent { .Position = { 0.f, 1.f, 0.f }, .Scale = glm::vec3(2.f) });
    } else if (GameIns->Input->GetKeyDown(GLFW_KEY_9)) {
        startBrace(GLFW_KEY_9, "eggplant",       "#E1D5F2", TransformComponent { .Position = { 0.f, 1.f, 0.f }, .Scale = glm::vec3(0.2f) });
    } else if (GameIns->Input->GetKeyDown(GLFW_KEY_0)) {
        startBrace(GLFW_KEY_0, "tomatoes",       "#E4FDE1", TransformComponent { .Position = { 0.f, 1.f, 0.f }, .Scale = glm::vec3(2.f) });
    }

    if (_heldKey != -1 && GameIns->Input->GetKeyUp(_heldKey)) {
        if (_popState == PopState::Bracing) {
            _popState = PopState::Expanding;
            _expandTime = 0.f;
            _swapDone = false;
        }
        _heldKey = -1;
    }

    if (_popState == PopState::Bracing) {
        _braceTime = glm::min(_braceTime + deltaTime, _braceDuration);
    }

    if (_popState == PopState::Expanding) {
        _expandTime += deltaTime;
        if (_expandTime >= _expandDuration) {
            _popState = PopState::Idle;
        }
    }

    if (GameIns->Input->GetKeyDown(GLFW_KEY_C)) {
        _useOrbitCamera = !_useOrbitCamera;
    }

    if (GameIns->Input->GetKeyDown(GLFW_KEY_T)) {
        _animatedTransitions = !_animatedTransitions;
        _popState = PopState::Idle;
        _heldKey = -1;
    }

    if (GameIns->Input->GetKeyDown(GLFW_KEY_P)) {
        _pixelationEnabled = !_pixelationEnabled;
        LoadPasses();
    }

    if (GameIns->Input->GetKeyDown(GLFW_KEY_O) && _pixelationEnabled) {
        _pixelEdgesEnabled = !_pixelEdgesEnabled;
        LoadPasses();
    }

    if (_pixelationEnabled) {
        if (GameIns->Input->GetKeyDown(GLFW_KEY_MINUS)) {
            _pixelSize = std::max(1.0f, _pixelSize - 2.0f);
            LoadPasses();
        }
        if (GameIns->Input->GetKeyDown(GLFW_KEY_EQUAL)) {
            _pixelSize = std::min(64.0f, _pixelSize + 2.0f);
            LoadPasses();
        }
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

    {
        float scaleMult = 1.f;
        if (_animatedTransitions) {
            if (_popState == PopState::Bracing) {
                float t = _braceTime / _braceDuration;
                float eased = 1.f - (1.f - t) * (1.f - t);
                scaleMult = glm::mix(1.0f, _braceScale, eased);
            } else if (_popState == PopState::Expanding) {
                scaleMult = ExpandCurve(_expandTime / _expandDuration);
                if (!_swapDone && scaleMult >= 1.f) {
                    ChangeShowcase(_pendingModel, _pendingColor, _pendingTransform);
                    _swapDone = true;
                }
            }
        }
        auto& transform = _registry.get<TransformComponent>(_showcasedEntity);
        transform = _showcasedTransform;
        transform.Scale *= scaleMult;
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
}

auto ShowcaseScene::ChangeShowcase(
    const std::string& modelName,
    const std::string& hxcolor,
    const TransformComponent& adjustedTransform
) -> void {
    _showcasedTransform = adjustedTransform;
    _registry.get<RenderComponent>(_showcasedEntity).Prop = _assetRegistry.GetProp(modelName).lock();

    auto view = _registry.view<NameComponent, RenderComponent>();
    for (auto [entity, name, render] : view.each()) {
        if (name.Name == "skycube") {
            render.Prop->Materials[0]->SetFloat3("BaseColor", HexColor(hxcolor.c_str()));
        }
    }
}
