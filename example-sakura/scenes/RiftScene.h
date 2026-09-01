#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <umbrellas/common.hpp>

#include "DeliverySystem.h"
#include "FullScene.h"
#include "MetaSystem.h"

class ShipCameraController;
class DeliverySystem;
class RiftTerrain;
class BeMaterial;
class BeImGuiPass;
struct ImFont;

class RiftScene : public FullScene {
    hide
    std::unique_ptr<RiftTerrain> _terrain;
    std::unique_ptr<ShipCameraController> _shipCameraController;
    std::unique_ptr<DeliverySystem> _delivery;
    std::array<entt::entity, 9> _terrainTiles;
    std::shared_ptr<BeMaterial> _posterizeMaterial;
    std::shared_ptr<BeMaterial> _hudMaterial;
    ImFont* _riftFont = nullptr;
    bool _stationUiOpen = false;
    bool _dying = false;
    bool _showDebug = false;
    MetaSystem _meta;

    expose
    explicit RiftScene(Game* game);
    ~RiftScene() override;

    auto Prepare() -> void override;
    auto Tick(float deltaTime) -> void override;

    hide
    auto EnterPlayMode() -> void;
    auto ExitPlayMode() -> void;
    auto SetStationUiOpen(bool open) -> void;
    auto DeathSequence() -> BeCoroutine;

    protect
    auto DefineAssets() -> void override;
    auto DefineSettings() -> void override;
    auto DefineScene() -> void override;
    auto DefinePasses() -> void override;
};
