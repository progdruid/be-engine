#pragma once

#include <memory>
#include <string>
#include <vector>
#include <umbrellas/include-glm.h>
#include <umbrellas/common.hpp>

#include "FullScene.h"
#include "Components.h"

struct BeProp;
struct BeMesh;
class BeMaterial;
class BeTexture;

class OldScene : public FullScene {
    hide
    std::shared_ptr<BeProp> _plane, _witchItems, _cube, _macintosh, _pagoda, _disks, _anvil;

    expose
    explicit OldScene(Game* game);
    ~OldScene() override;

    auto Tick(float deltaTime) -> void override;

    protect
    auto DefineAssets() -> void override;
    auto DefineSettings() -> void override;
    auto DefineScene() -> void override;
    auto DefinePasses() -> void override;
};
