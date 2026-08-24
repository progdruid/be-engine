#include "RiftScene.h"

#include <algorithm>
#include <cmath>
#include <random>
#include <string>

#include <umbrellas/include-glfw.h>
#include <umbrellas/include-glm.h>

#include "BeMesh.h"

#include "ShipCameraController.h"
#include "DeliverySystem.h"
#include "RiftTerrain.h"
#include "BeCamera.h"
#include "imgui/BeImGuiPass.h"
#include "BeInput.h"
#include "BeMaterial.h"
#include "BeMeshPrimitives.h"
#include "BeProp.h"
#include "BeRenderer.h"
#include "BeTexture.h"
#include "Components.h"
#include "Game.h"
#include "scenes/BeSceneManager.h"
#include "BeShaderLibrary.h"
#include "BeShader.h"
#include "standard-render-machine/BeStandardRenderMachine.h"

RiftScene::RiftScene(Game* game) : FullScene(game) {}
RiftScene::~RiftScene() = default;

void RiftScene::Prepare() {
    FullScene::Prepare();

    _camera->Position = glm::vec3(0.0f, Settings.Camera.SpawnHeight, 0.0f);

    _shipCameraController = std::make_unique<ShipCameraController>(_camera.get());
    _hudMaterial->SetFloat1("AimRadius", _shipCameraController->AimRadius);
}

auto RiftScene::EnterPlayMode() -> void {
    auto deliveryConfig = Settings.Delivery.Config;
    deliveryConfig.TerrainSize = Settings.Terrain.Size;
    deliveryConfig.TerrainSpikeAmplitude = Settings.Terrain.SpikeAmplitude;
    deliveryConfig.Seed = std::random_device{}();
    _delivery = std::make_unique<DeliverySystem>(_registry, _assetRegistry, deliveryConfig);
    _delivery->GenerateStations();
    _delivery->Begin(_camera->Position);
}

auto RiftScene::ExitPlayMode() -> void {
    const auto stations = _registry.view<StationComponent>();
    _registry.destroy(stations.begin(), stations.end());
    _delivery.reset();
    _hudMaterial->SetFloat1("TargetState", 0.0f);
}

auto RiftScene::DefineSettings() -> void {
    _camera->NearPlane = Settings.Camera.NearPlane;
    _camera->FarPlane = Settings.Camera.FarPlane;
    _machine->UniformMaterial->SetFloat3("AmbientColor", Settings.Ambient.Color);
}

auto RiftScene::DefineAssets() -> void {
    auto phongShader = BeShaderLibrary::GetShader("standard-phong");

    auto box = BeProp::FromMesh(BeMeshPrimitives::Cube(), phongShader, "geometry-main");
    box->Materials[0]->SetFloat3("DiffuseColor", glm::vec3(1.0f));
    _assetRegistry.AddProp("box", box);
    _machine->RegisterMesh(box->Mesh);

    auto floor = BeProp::FromMesh(
        RiftTerrain::BuildMesh(Settings.Terrain.Size, Settings.Terrain.Cells, Settings.Terrain.SpikeAmplitude),
        phongShader, "geometry-main"
    );
    floor->Materials[0]->SetFloat3("DiffuseColor", Settings.Terrain.Color);
    _assetRegistry.AddProp("floor", floor);
    _machine->RegisterMesh(floor->Mesh);

    auto stationShader = BeShaderLibrary::GetShader("station");
    for (const auto& kind : Settings.Delivery.Config.StationKinds) {
        auto prop = _machine->LoadProp(kind.Path, stationShader, BeSRMLightingModel::Phong);
        for (const auto& material : prop->Materials) {
            material->SetFloat1("EmissiveMix", kind.EmissiveMix);
        }
        _assetRegistry.AddProp(kind.Prop, prop);
    }

    _machine->BakeMeshes();

    _machine->DeclareGBufferTarget("Rift_BaseColor",         SenFormat::R11G11B10_Float);
    _machine->DeclareGBufferTarget("Rift_WorldNormal",       SenFormat::RGBA16_Float);
    _machine->DeclareGBufferTarget("Rift_SpecularShininess", SenFormat::RGBA8_Unorm);
    _machine->DeclareGBufferTarget("Rift_Emissive",          SenFormat::R11G11B10_Float);
    _machine->DeclareDepthTarget  ("Rift_Depth",             SenFormat::Depth32);
    _machine->DeclareTextureTarget("Rift_HDR",               SenFormat::R11G11B10_Float);
    _machine->DeclareTextureTarget("Rift_Post",              SenFormat::R11G11B10_Float);
    _machine->DeclareTextureTarget("Rift_UI",                SenFormat::RGBA8_Unorm);
}

auto RiftScene::DefineScene() -> void {
    _registry.clear();

    CreateEntity(_registry
        ,NameComponent { .Name = "box-left" }
        ,TransformComponent { .Position = { -1.5f, 0.5f, 0.f } }
        ,RenderComponent { .Prop = _assetRegistry.GetProp("box").lock(), .CastShadows = true }
    );
    CreateEntity(_registry
        ,NameComponent { .Name = "box-right" }
        ,TransformComponent { .Position = { 1.5f, 0.5f, 0.f } }
        ,RenderComponent { .Prop = _assetRegistry.GetProp("box").lock(), .CastShadows = true }
    );

    for (int tile = 0; tile < 9; ++tile) {
        _terrainTiles[tile] = CreateEntity(_registry
            ,NameComponent { .Name = "terrain-" + std::to_string(tile) }
            ,TransformComponent { }
            ,RenderComponent { .Prop = _assetRegistry.GetProp("floor").lock(), .CastShadows = true }
        );
    }

    CreateEntity(_registry
        ,NameComponent { .Name = "Sun" }
        ,SunLightComponent {
            .Direction = Settings.Sun.Direction,
            .Color = Settings.Sun.Color,
            .Power = Settings.Sun.Power,
            .CastsShadows = false,
            .ShadowCameraDistance = Settings.Sun.ShadowCameraDistance,
            .ShadowMapWorldSize = Settings.Sun.ShadowMapWorldSize,
            .ShadowNearPlane = Settings.Sun.ShadowNearPlane,
            .ShadowFarPlane = Settings.Sun.ShadowFarPlane,
        }
    );
}

auto RiftScene::DefinePasses() -> void {
    _machine->ClearPasses();
    _machine->AddShadowPass();
    _machine->AddGeometryPass();
    _machine->AddLightingPass("Rift_HDR");

    const auto& posterizeScheme = BeShaderLibrary::GetShader("posterize")->GetMaterialScheme("main");
    _posterizeMaterial = BeMaterial::Create(posterizeScheme);
    _posterizeMaterial->SetTexture("ColorTexture", _machine->GetRenderTexture("Rift_HDR"));
    _posterizeMaterial->SetTexture("DepthTexture", _machine->GetRenderTexture("Rift_Depth"));
    _posterizeMaterial->SetFloat1("PixelSize", Settings.Posterize.PixelSize);
    _posterizeMaterial->SetFloat1("DitherSpread", Settings.Posterize.DitherSpread);
    _posterizeMaterial->SetFloat1("FogStart", Settings.Posterize.FogStart);
    _posterizeMaterial->SetFloat1("FogEnd", Settings.Posterize.FogEnd);
    _posterizeMaterial->SetFloat3("FogColor", Settings.Posterize.FogColor);
    _posterizeMaterial->SetFloat3Array("Palette", Settings.Posterize.Palette);
    _posterizeMaterial->SetFloat1("PaletteCount", static_cast<float>(Settings.Posterize.Palette.size()));
    _posterizeMaterial->SetFloat1("Enabled", Settings.Posterize.Enabled ? 1.0f : 0.0f);
    _posterizeMaterial->SetTexture("UITexture", _machine->GetRenderTexture("Rift_UI"));

    const uint32_t screenWidth  = _gameIns->Renderer->GetSwapchainPixelWidth();
    const uint32_t screenHeight = _gameIns->Renderer->GetSwapchainPixelHeight();
    const auto& hudScheme = BeShaderLibrary::GetShader("ship-hud")->GetMaterialScheme("main");
    _hudMaterial = BeMaterial::Create(hudScheme);
    _hudMaterial->SetFloat2("ScreenSize", { static_cast<float>(screenWidth), static_cast<float>(screenHeight) });
    _hudMaterial->SetFloat1("PixelSize", Settings.Posterize.PixelSize);
    _machine->AddFullscreenPass(BeShaderLibrary::GetShader("ship-hud"), _hudMaterial, { "Rift_UI" });

    _machine->AddFullscreenPass(BeShaderLibrary::GetShader("posterize"), _posterizeMaterial, { "Rift_Post" });

    _machine->AddBackbufferPass("Rift_Post");

    //auto imguiPass = std::make_unique<BeImGuiPass>(_gameIns->Window);
    //imguiPass->SetUICallback([this]() { _shipCameraController->DrawDebugUI(); });
    //_machine->AddPass(std::move(imguiPass));

    _machine->InitialisePasses();
}

void RiftScene::Tick(float deltaTime) {
    if (_gameIns->Input->GetKeyDown(GLFW_KEY_ESCAPE)) {
        _gameIns->Input->SetMouseCapture(false);
        _gameIns->SceneManager->RequestSceneChange("menu");
        return;
    }
    

    if (_gameIns->Input->GetKeyDown(GLFW_KEY_ENTER)) {
        Settings.Posterize.Enabled = !Settings.Posterize.Enabled;
        _posterizeMaterial->SetFloat1("Enabled", Settings.Posterize.Enabled ? 1.0f : 0.0f);
    }

    if (_gameIns->Input->GetKeyDown(GLFW_KEY_P)) {
        if (_delivery) ExitPlayMode();
        else EnterPlayMode();
    }

    if (_gameIns->Input->GetKeyDown(GLFW_KEY_T) && _delivery) {
        _delivery->TargetNearest(_camera->Position);
    }

    _shipCameraController->Update(deltaTime, _gameIns->Input.get());
    _hudMaterial->SetFloat2("AimOffset", _shipCameraController->GetAim());

    const glm::vec3 worldUp = { 0.0f, 1.0f, 0.0f };
    const glm::vec2 upScreen = { glm::dot(worldUp, _camera->GetRight()), glm::dot(worldUp, _camera->GetUp()) };
    glm::vec2 horizonDir = { upScreen.y, -upScreen.x };
    const float horizonLen = glm::length(horizonDir);
    horizonDir = horizonLen > 1e-3f ? horizonDir / horizonLen : glm::vec2(1.0f, 0.0f);
    _hudMaterial->SetFloat2("HorizonDir", { horizonDir.x, -horizonDir.y });

    if (_delivery) _delivery->Update(_camera->Position);

    const float screenW = static_cast<float>(_gameIns->Renderer->GetSwapchainPixelWidth());
    const float screenH = static_cast<float>(_gameIns->Renderer->GetSwapchainPixelHeight());
    const auto& marker = Settings.Delivery.Marker;
    float targetState = 0.0f;
    glm::vec2 targetPixel = { screenW * 0.5f, screenH * 0.5f };
    glm::vec2 targetDir = { 0.0f, 1.0f };
    float targetRadius = marker.MinRadius;
    float targetAlpha = 1.0f;
    if (_delivery && _delivery->HasTarget()) {
        const glm::vec4 clip = _camera->GetProjectionMatrix() * _camera->GetViewMatrix()
            * glm::vec4(_delivery->TargetPosition(), 1.0f);
        const bool behind = clip.w <= 1e-4f;
        glm::vec2 ndc = glm::vec2(clip.x, clip.y) / clip.w;
        if (behind) ndc = -ndc;
        const bool onScreen = !behind && std::abs(ndc.x) <= 1.0f && std::abs(ndc.y) <= 1.0f;
        if (onScreen) {
            targetState = 1.0f;
            targetPixel = { (ndc.x * 0.5f + 0.5f) * screenW, (0.5f - ndc.y * 0.5f) * screenH };
            const float distance = glm::length(_delivery->TargetPosition() - _camera->Position);
            targetRadius = glm::mix(marker.MinRadius, marker.MaxRadius, glm::smoothstep(marker.SizeFar, marker.SizeNear, distance));
            targetAlpha = glm::smoothstep(marker.FadeNear, marker.FadeFar, distance);
        } else {
            targetState = 2.0f;
            const float extent = std::max(std::max(std::abs(ndc.x), std::abs(ndc.y)), 1e-4f);
            const glm::vec2 marked = (ndc / extent) * marker.ScreenMargin;
            targetPixel = { (marked.x * 0.5f + 0.5f) * screenW, (0.5f - marked.y * 0.5f) * screenH };
            targetDir = glm::normalize(glm::vec2(ndc.x, -ndc.y));
        }
    }
    _hudMaterial->SetFloat2("TargetPos", targetPixel);
    _hudMaterial->SetFloat2("TargetDir", targetDir);
    _hudMaterial->SetFloat1("TargetState", targetState);
    _hudMaterial->SetFloat1("TargetRingRadius", targetRadius);
    _hudMaterial->SetFloat1("TargetAlpha", targetAlpha);

    const float tileSize = Settings.Terrain.Size;
    const int centerX = static_cast<int>(std::round(_camera->Position.x / tileSize));
    const int centerZ = static_cast<int>(std::round(_camera->Position.z / tileSize));
    int tileIndex = 0;
    for (int j = -1; j <= 1; ++j) {
        for (int i = -1; i <= 1; ++i) {
            auto& tileTransform = _registry.get<TransformComponent>(_terrainTiles[tileIndex++]);
            tileTransform.Position = glm::vec3((centerX + i) * tileSize, 0.0f, (centerZ + j) * tileSize);
        }
    }

    FullScene::Tick(deltaTime);
}
