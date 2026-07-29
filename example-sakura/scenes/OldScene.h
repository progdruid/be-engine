#pragma once

#include <memory>
#include <string>
#include <vector>
#include <umbrellas/include-glm.h>
#include <umbrellas/common.hpp>

#include "entt/entt.hpp"
#include "BaseScene.h"
#include "BeAssetRegistry.h"
#include "Components.h"

class BeCamera;
struct BeProp;
struct BeMesh;
class BeMaterial;
class BeTexture;
class BeStandardRenderMachine;

class OldScene : public BaseScene {
    hide
    BeAssetRegistry _assetRegistry;
    entt::registry _registry;
    std::shared_ptr<BeCamera> _camera;
    float _elapsedTime = 0.0f;

    std::shared_ptr<BeProp> _plane, _witchItems, _cube, _macintosh, _pagoda, _disks, _anvil;
    std::unique_ptr<BeStandardRenderMachine> _machine;

    expose
    explicit OldScene(Game* game);
    ~OldScene() override;

    auto Prepare() -> void override;
    auto LoadPasses() -> void;
    auto OnLoad() -> void override;
    auto Tick(float deltaTime) -> void override;
};
