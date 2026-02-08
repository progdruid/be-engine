#pragma once
#include <umbrellas/access-modifiers.hpp>

#include "BaseScene.h"
#include "entt/entt.hpp"

struct BeModel;
class BeInput;
class BeCamera;
class BeWindow;
class BeRenderer;
class BeBRPSubmissionBuffer;

class MainScene : public BaseScene {
    hide
    entt::registry _registry;
    std::shared_ptr<BeCamera> _camera;
    
    std::shared_ptr<BeModel> _cube, _anvil, _sakura, _sakura2, _emissiveCube;
    
    expose
    explicit MainScene(Game* game);
    ~MainScene() override = default;

    auto Prepare() -> void override;
    auto OnLoad() -> void override;
    auto Tick(float deltaTime) -> void override;
};
