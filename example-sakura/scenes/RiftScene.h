#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <umbrellas/common.hpp>
#include <umbrellas/include-glm.h>

#include "DeliverySystem.h"
#include "FullScene.h"

class ShipCameraController;
class DeliverySystem;
class BeMaterial;
class BeImGuiPass;

struct RiftSceneSettings {
    struct {
        float NearPlane = 0.5f;
        float FarPlane = 300.0f;
    } Camera;

    struct {
        //glm::vec3 Color = HexColor("#0A122E");
        //glm::vec3 Color = HexColor("#222222");
        //glm::vec3 Color = HexColor("#FF7F11");
        glm::vec3 Color = HexColor("#000000");
        //glm::vec3 Color = HexColor("#2E4372");
    } Ambient;

    struct { 
        float Size = 180.0f;
        int Cells = 45;
        float SpikeAmplitude = 75.0f;
        glm::vec3 Color = HexColor("#F28123");
    } Terrain;

    struct {
        float PixelSize = 4.0f;
        float DitherSpread = 1.0f;
        float FogStart = 100.0f;
        float FogEnd = 200.0f;
        glm::vec3 FogColor = HexColor("#2E4372");
        bool Enabled = true;
        std::vector<glm::vec3> Palette = {
            HexColor("#2E4372"),
            HexColor("#E89128"),
            HexColor("#F7F052"),
            HexColor("#D34E24"),
            HexColor("#8C3318"),
            HexColor("#1F2C47"),
        };
    } Posterize;

    struct {
        glm::vec3 Direction = { -0.5f, -1.0f, -0.5f };
        glm::vec3 Color = glm::vec3(1.0f);
        float Power = 1.0f;
        float ShadowCameraDistance = 50.0f;
        float ShadowMapWorldSize = 30.0f;
        float ShadowNearPlane = 0.1f;
        float ShadowFarPlane = 200.0f;
    } Sun;

    struct {
        DeliverySystem::Config Config = {
            .StationProps = { "station-sphere", "station-cube", "station-shard" },
            .StationCount = 12,
            .MapRadius = 1500.0f,
            .AltitudeMin = 30.0f,
            .AltitudeMax = 150.0f,
            .MinSeparation = 250.0f,
            .VisitRadius = 35.0f,
            .StationScale = 10.0f,
        };
        struct {
            float ScreenMargin = 0.88f;
            float MinRadius = 1.5f;
            float MaxRadius = 10.0f;
            float SizeFar = 800.0f;
            float SizeNear = 120.0f;
            float FadeNear = 80.0f;
            float FadeFar = 200.0f;
        } Marker;
    } Delivery;
};

class RiftScene : public FullScene {
    hide
    std::unique_ptr<ShipCameraController> _shipCameraController;
    std::unique_ptr<DeliverySystem> _delivery;
    std::array<entt::entity, 9> _terrainTiles;
    std::shared_ptr<BeMaterial> _posterizeMaterial;
    std::shared_ptr<BeMaterial> _hudMaterial;

    expose
    RiftSceneSettings Settings;

    explicit RiftScene(Game* game);
    ~RiftScene() override;

    auto Prepare() -> void override;
    auto Tick(float deltaTime) -> void override;

    protect
    auto DefineAssets() -> void override;
    auto DefineSettings() -> void override;
    auto DefineScene() -> void override;
    auto DefinePasses() -> void override;
};
