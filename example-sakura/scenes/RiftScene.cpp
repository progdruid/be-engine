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
#include "StationUI.h"
#include "RiftSettings.h"
#include "RiftTerrain.h"
#include "BeCamera.h"
#include "imgui/BeImGuiPass.h"
#include "imgui/imgui.h"
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

RiftScene::RiftScene(Game* game) : FullScene(game) {
    RiftStore::Bootstrap();
}

RiftScene::~RiftScene() {
    RiftStore::Shutdown();
}

void RiftScene::Prepare() {
    FullScene::Prepare();

    _camera->Position = glm::vec3(0.0f, RiftStore::Get().Camera.SpawnHeight, 0.0f);

    _shipCameraController = std::make_unique<ShipCameraController>(_camera.get());
    _hudMaterial->SetFloat1("AimRadius", RiftStore::Get().Ship.AimRadius);
}

auto RiftScene::EnterPlayMode() -> void {
    RiftStore::Get().Delivery.Seed = std::random_device{}();
    _delivery = std::make_unique<DeliverySystem>(_registry, _assetRegistry);
    _delivery->GenerateStations();
}

auto RiftScene::SetStationUiOpen(bool open) -> void {
    _stationUiOpen = open;
    _shipCameraController->SetControlsEnabled(!open);
    _gameIns->Input->SetMouseCapture(!open);
}

auto RiftScene::ExitPlayMode() -> void {
    const auto stations = _registry.view<StationComponent>();
    _registry.destroy(stations.begin(), stations.end());
    const auto docks = _registry.view<DockComponent>();
    _registry.destroy(docks.begin(), docks.end());
    SetStationUiOpen(false);
    _delivery.reset();
    _hudMaterial->SetFloat1("TargetState", 0.0f);
}

auto RiftScene::DefineSettings() -> void {
    const auto& settings = RiftStore::Get();
    _camera->NearPlane = settings.Camera.NearPlane;
    _camera->FarPlane = settings.Camera.FarPlane;
    _machine->UniformMaterial->SetFloat3("AmbientColor", settings.Ambient.Color);
}

auto RiftScene::DefineAssets() -> void {
    const auto& settings = RiftStore::Get();
    auto phongShader = BeShaderLibrary::GetShader("standard-phong");

    auto box = BeProp::FromMesh(BeMeshPrimitives::Cube(), phongShader, "geometry-main");
    box->Materials[0]->SetFloat3("DiffuseColor", glm::vec3(1.0f));
    _assetRegistry.AddProp("box", box);
    _machine->RegisterMesh(box->Mesh);

    auto floor = BeProp::FromMesh(
        RiftTerrain::BuildMesh(settings.Terrain.Size, settings.Terrain.Cells, settings.Terrain.SpikeAmplitude),
        phongShader, "geometry-main"
    );
    floor->Materials[0]->SetFloat3("DiffuseColor", settings.Terrain.Color);
    _assetRegistry.AddProp("floor", floor);
    _machine->RegisterMesh(floor->Mesh);

    auto stationShader = BeShaderLibrary::GetShader("station");
    for (const auto& kind : settings.Delivery.Kinds) {
        auto prop = _machine->LoadProp(kind.Path, stationShader, BeSRMLightingModel::Phong);
        for (const auto& material : prop->Materials) {
            material->SetFloat1("EmissiveMix", kind.EmissiveMix);
        }
        _assetRegistry.AddProp(kind.Prop, prop);
    }

    auto ringShader = BeShaderLibrary::GetShader("dock-ring");
    auto ring = BeProp::FromMesh(BeMeshPrimitives::Plane(), ringShader, "geometry-main");
    _assetRegistry.AddProp("dock-ring", ring);
    _machine->RegisterMesh(ring->Mesh);

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

    const auto& sun = RiftStore::Get().Sun;
    CreateEntity(_registry
        ,NameComponent { .Name = "Sun" }
        ,SunLightComponent {
            .Direction = sun.Direction,
            .Color = sun.Color,
            .Power = sun.Power,
            .CastsShadows = false,
            .ShadowCameraDistance = sun.ShadowCameraDistance,
            .ShadowMapWorldSize = sun.ShadowMapWorldSize,
            .ShadowNearPlane = sun.ShadowNearPlane,
            .ShadowFarPlane = sun.ShadowFarPlane,
        }
    );
}

auto RiftScene::DefinePasses() -> void {
    _machine->ClearPasses();
    _machine->AddShadowPass();
    _machine->AddGeometryPass();
    _machine->AddLightingPass("Rift_HDR");

    const auto& posterize = RiftStore::Get().Posterize;
    const auto& posterizeScheme = BeShaderLibrary::GetShader("posterize")->GetMaterialScheme("main");
    _posterizeMaterial = BeMaterial::Create(posterizeScheme);
    _posterizeMaterial->SetTexture("ColorTexture", _machine->GetRenderTexture("Rift_HDR"));
    _posterizeMaterial->SetTexture("DepthTexture", _machine->GetRenderTexture("Rift_Depth"));
    _posterizeMaterial->SetFloat1("PixelSize", posterize.PixelSize);
    _posterizeMaterial->SetFloat1("DitherSpread", posterize.DitherSpread);
    _posterizeMaterial->SetFloat1("FogStart", posterize.FogStart);
    _posterizeMaterial->SetFloat1("FogEnd", posterize.FogEnd);
    _posterizeMaterial->SetFloat3("FogColor", posterize.FogColor);
    _posterizeMaterial->SetFloat3Array("Palette", posterize.Palette);
    _posterizeMaterial->SetFloat1("PaletteCount", static_cast<float>(posterize.Palette.size()));
    _posterizeMaterial->SetFloat1("Enabled", posterize.Enabled ? 1.0f : 0.0f);
    _posterizeMaterial->SetTexture("UITexture", _machine->GetRenderTexture("Rift_UI"));

    const uint32_t screenWidth  = _gameIns->Renderer->GetSwapchainPixelWidth();
    const uint32_t screenHeight = _gameIns->Renderer->GetSwapchainPixelHeight();
    const auto& hudScheme = BeShaderLibrary::GetShader("ship-hud")->GetMaterialScheme("main");
    _hudMaterial = BeMaterial::Create(hudScheme);
    _hudMaterial->SetFloat2("ScreenSize", { static_cast<float>(screenWidth), static_cast<float>(screenHeight) });
    _hudMaterial->SetFloat1("PixelSize", posterize.PixelSize);
    _machine->AddFullscreenPass(BeShaderLibrary::GetShader("ship-hud"), _hudMaterial, { "Rift_UI" });

    _machine->AddFullscreenPass(BeShaderLibrary::GetShader("posterize"), _posterizeMaterial, { "Rift_Post" });

    _machine->AddBackbufferPass("Rift_Post");

    auto imguiPass = std::make_unique<BeImGuiPass>(_gameIns->Window);
    imguiPass->SetUICallback([this]() {
        ImGui::PushFont(_riftFont);
        _shipCameraController->DrawDebugUI();
        if (_stationUiOpen && _delivery) StationUI::Draw(*_delivery, _camera->Position);
        ImGui::PopFont();
    });
    _machine->AddPass(std::move(imguiPass));

    _machine->InitialisePasses();

    _riftFont = ImGui::GetIO().Fonts->AddFontFromFileTTF("assets/rift/b612-mono/B612Mono-Regular.ttf", 20.0f);
}

void RiftScene::Tick(float deltaTime) {
    if (_gameIns->Input->GetKeyDown(GLFW_KEY_ESCAPE)) {
        _gameIns->Input->SetMouseCapture(false);
        _gameIns->SceneManager->RequestSceneChange("menu");
        return;
    }
    

    if (_gameIns->Input->GetKeyDown(GLFW_KEY_ENTER)) {
        bool& enabled = RiftStore::Get().Posterize.Enabled;
        enabled = !enabled;
        _posterizeMaterial->SetFloat1("Enabled", enabled ? 1.0f : 0.0f);
    }

    if (_gameIns->Input->GetKeyDown(GLFW_KEY_P)) {
        if (_delivery) ExitPlayMode();
        else EnterPlayMode();
    }

    _shipCameraController->Update(deltaTime, _gameIns->Input.get());
    _hudMaterial->SetFloat2("AimOffset", _shipCameraController->GetAim());

    const glm::vec3 worldUp = { 0.0f, 1.0f, 0.0f };
    const glm::vec2 upScreen = { glm::dot(worldUp, _camera->GetRight()), glm::dot(worldUp, _camera->GetUp()) };
    glm::vec2 horizonDir = { upScreen.y, -upScreen.x };
    const float horizonLen = glm::length(horizonDir);
    horizonDir = horizonLen > 1e-3f ? horizonDir / horizonLen : glm::vec2(1.0f, 0.0f);
    _hudMaterial->SetFloat2("HorizonDir", { horizonDir.x, -horizonDir.y });

    if (_delivery) {
        const auto dock = _delivery->CheckDock(_camera->Position);
        _shipCameraController->SetInDock(dock.Hit);

        if (_shipCameraController->HasJustEnteredDock()) {
            _shipCameraController->Capture(dock.Anchor);
            _delivery->NotifyDocked(dock);
        }

        if (_shipCameraController->IsCaptured() && _gameIns->Input->GetKeyDown(GLFW_KEY_S)) {
            SetStationUiOpen(!_stationUiOpen);
        }

        if (_gameIns->Input->GetKeyDown(GLFW_KEY_C)) {
            _shipCameraController->Uncapture();
            _delivery->NotifyUndocked();
            SetStationUiOpen(false);
        }
    }

    const float screenW = static_cast<float>(_gameIns->Renderer->GetSwapchainPixelWidth());
    const float screenH = static_cast<float>(_gameIns->Renderer->GetSwapchainPixelHeight());
    const auto& marker = RiftStore::Get().Delivery.Marker;
    float targetState = 0.0f;
    glm::vec2 targetPixel = { screenW * 0.5f, screenH * 0.5f };
    glm::vec2 targetDir = { 0.0f, 1.0f };
    float targetRadius = marker.MinRadius;
    float targetAlpha = 1.0f;
    if (_delivery && _delivery->HasContract() && !_delivery->CanComplete()) {
        const glm::vec3 targetWorld = _delivery->GetTargetPosition(_camera->Position);
        const glm::vec4 clip = _camera->GetProjectionMatrix() * _camera->GetViewMatrix()
            * glm::vec4(targetWorld, 1.0f);
        const bool behind = clip.w <= 1e-4f;
        glm::vec2 ndc = glm::vec2(clip.x, clip.y) / clip.w;
        if (behind) ndc = -ndc;
        const bool onScreen = !behind && std::abs(ndc.x) <= 1.0f && std::abs(ndc.y) <= 1.0f;
        if (onScreen) {
            targetState = 1.0f;
            targetPixel = { (ndc.x * 0.5f + 0.5f) * screenW, (0.5f - ndc.y * 0.5f) * screenH };
            const float distance = glm::length(targetWorld - _camera->Position);
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

    const float tileSize = RiftStore::Get().Terrain.Size;
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
