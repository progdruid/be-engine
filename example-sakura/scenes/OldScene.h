#pragma once

#include <memory>
#include <string>
#include <vector>
#include <umbrellas/include-glm.h>
#include <umbrellas/common.hpp>

#include "standard-game/BeStandardFullScene.h"
#include "standard-game/Components.h"

struct BeProp;
struct BeMesh;
class BeMaterial;
class BeTexture;

class OldScene : public BeStandardFullScene {
    hide
    std::shared_ptr<BeProp> _plane, _witchItems, _cube, _macintosh, _pagoda, _disks, _anvil;

    expose
    explicit OldScene(BeStandardGame* game);
    ~OldScene() override;

    auto Tick(float deltaTime) -> void override;

    protect
    auto DefineAssets() -> void override;
    auto DefineSettings() -> void override;
    auto DefineScene() -> void override;
    auto DefinePasses() -> void override;
};
