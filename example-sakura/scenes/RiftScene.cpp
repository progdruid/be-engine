#include "RiftScene.h"

#include <algorithm>
#include <cmath>
#include <string>

#include <umbrellas/include-glfw.h>
#include <umbrellas/include-glm.h>

#include "BeMesh.h"

#include "ShipCameraController.h"
#include "DeliverySystem.h"
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

static auto MakeSpikyTerrain(float size, int cells, float spikeAmp) -> std::shared_ptr<BeMesh> {
    const float half = size * 0.5f;
    const float cell = size / cells;

    auto hash = [](int x, int z) -> float {
        uint32_t h = static_cast<uint32_t>(x * 374761393 + z * 668265263);
        h = (h ^ (h >> 13)) * 1274126177u;
        h ^= h >> 16;
        return (h & 0xFFFF) / 65535.0f;
    };
    auto smootherstep = [](float t) -> float {
        return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
    };
    auto valueNoise = [&](float u, float v, int freq) -> float {
        float px = u * freq;
        float pz = v * freq;
        int x0 = static_cast<int>(std::floor(px));
        int z0 = static_cast<int>(std::floor(pz));
        float tx = smootherstep(px - x0);
        float tz = smootherstep(pz - z0);
        int seed = freq * 131;
        int x1 = (x0 + 1) % freq;
        int z1 = (z0 + 1) % freq;
        x0 = x0 % freq;
        z0 = z0 % freq;
        float c00 = hash(x0 + seed, z0);
        float c10 = hash(x1 + seed, z0);
        float c01 = hash(x0 + seed, z1);
        float c11 = hash(x1 + seed, z1);
        float a = c00 + (c10 - c00) * tx;
        float b = c01 + (c11 - c01) * tx;
        return a + (b - a) * tz;
    };
    auto ridgedFbm = [&](float u, float v) -> float {
        float sum = 0.0f;
        float amp = 1.0f;
        float norm = 0.0f;
        int freq = 2;
        for (int octave = 0; octave < 5; ++octave) {
            float n = valueNoise(u, v, freq);
            float ridge = 1.0f - std::fabs(2.0f * n - 1.0f);
            ridge *= ridge;
            sum += amp * ridge;
            norm += amp;
            amp *= 0.55f;
            freq *= 2;
        }
        return sum / norm;
    };
    auto heightAt = [&](int gx, int gz) -> float {
        float u = static_cast<float>(gx) / cells;
        float v = static_cast<float>(gz) / cells;
        float e = ridgedFbm(u, v);
        return std::pow(e, 2.0f) * spikeAmp;
    };
    auto vpos = [&](int gx, int gz) -> glm::vec3 {
        return glm::vec3(-(gx * cell - half), heightAt(gx, gz), gz * cell - half);
    };

    auto mesh = std::make_shared<BeMesh>();
    auto pushTri = [&](glm::vec3 a, glm::vec3 b, glm::vec3 c) {
        glm::vec3 normal = glm::normalize(glm::cross(b - a, c - a));
        uint32_t base = static_cast<uint32_t>(mesh->Vertices.size());
        for (const auto& p : { a, b, c }) {
            BeFullVertex v{};
            v.Position = p;
            v.Normal = normal;
            mesh->Vertices.push_back(v);
        }
        mesh->Indices.push_back(base + 0);
        mesh->Indices.push_back(base + 1);
        mesh->Indices.push_back(base + 2);
    };

    for (int gz = 0; gz < cells; ++gz) {
        for (int gx = 0; gx < cells; ++gx) {
            glm::vec3 topLeft = vpos(gx, gz);
            glm::vec3 topRight = vpos(gx + 1, gz);
            glm::vec3 bottomLeft = vpos(gx, gz + 1);
            glm::vec3 bottomRight = vpos(gx + 1, gz + 1);
            pushTri(topLeft, topRight, bottomLeft);
            pushTri(topRight, bottomRight, bottomLeft);
        }
    }

    mesh->Slices.push_back({
        .IndexCount = static_cast<uint32_t>(mesh->Indices.size()),
        .StartIndexLocation = 0,
        .BaseVertexLocation = 0,
    });

    return mesh;
}

RiftScene::RiftScene(Game* game) : FullScene(game) {}
RiftScene::~RiftScene() = default;

void RiftScene::Prepare() {
    FullScene::Prepare();

    _shipCameraController = std::make_unique<ShipCameraController>(_camera.get());
    _hudMaterial->SetFloat1("AimRadius", _shipCameraController->AimRadius);

    _delivery = std::make_unique<DeliverySystem>(_registry, _assetRegistry, Settings.Delivery.Config);
    _delivery->GenerateStations();
    _delivery->Begin(_camera->Position);
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
        MakeSpikyTerrain(Settings.Terrain.Size, Settings.Terrain.Cells, Settings.Terrain.SpikeAmplitude),
        phongShader, "geometry-main"
    );
    floor->Materials[0]->SetFloat3("DiffuseColor", Settings.Terrain.Color);
    _assetRegistry.AddProp("floor", floor);
    _machine->RegisterMesh(floor->Mesh);

    auto whiteTexture = BeShaderLibrary::GetDefaultTexture("white").lock();
    auto makeStation = [&](const std::string& name, std::shared_ptr<BeMesh> mesh, glm::vec3 color) {
        auto prop = BeProp::FromMesh(mesh, phongShader, "geometry-main");
        prop->Materials[0]->SetFloat3("DiffuseColor", color * 0.35f);
        prop->Materials[0]->SetFloat3("EmissiveColor", color);
        prop->Materials[0]->SetTexture("EmissiveTexture", whiteTexture);
        _assetRegistry.AddProp(name, prop);
        _machine->RegisterMesh(prop->Mesh);
    };
    makeStation("station-sphere", BeMeshPrimitives::Sphere(), HexColor("#F7F052"));
    makeStation("station-cube", BeMeshPrimitives::Cube(), HexColor("#E89128"));
    makeStation("station-shard", BeMeshPrimitives::Sphere(4, 6), HexColor("#D34E24"));

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

    auto imguiPass = std::make_unique<BeImGuiPass>(_gameIns->Window);
    imguiPass->SetUICallback([this]() { _shipCameraController->DrawDebugUI(); });
    _machine->AddPass(std::move(imguiPass));

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

    _shipCameraController->Update(deltaTime, _gameIns->Input.get());
    _hudMaterial->SetFloat2("AimOffset", _shipCameraController->GetAim());

    _delivery->Update(_camera->Position);

    const float screenW = static_cast<float>(_gameIns->Renderer->GetSwapchainPixelWidth());
    const float screenH = static_cast<float>(_gameIns->Renderer->GetSwapchainPixelHeight());
    const auto& marker = Settings.Delivery.Marker;
    float targetState = 0.0f;
    glm::vec2 targetPixel = { screenW * 0.5f, screenH * 0.5f };
    glm::vec2 targetDir = { 0.0f, 1.0f };
    float targetRadius = marker.MinRadius;
    float targetAlpha = 1.0f;
    if (_delivery->HasTarget()) {
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
