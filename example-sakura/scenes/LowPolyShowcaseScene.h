#pragma once

#include <umbrellas/access-modifiers.hpp>
#include <umbrellas/include-glm.h>

#include "BaseScene.h"
#include "entt/entt.hpp"

struct TransformComponent;
class BeInput;
class BeCamera;
class BeWindow;
class BeRenderer;
class BeBRPSubmissionBuffer;

class LowPolyShowcaseScene : public BaseScene {
    hide
    entt::registry _registry;
    std::shared_ptr<BeCamera> _showcaseCamera;
    std::shared_ptr<BeCamera> _freeCamera;
    bool _useShowcaseCamera = true;
    
    expose
    explicit LowPolyShowcaseScene(Game* game);
    ~LowPolyShowcaseScene() override;
    
    auto Prepare() -> void override;
    auto CreateTargetTextures() -> void;
    auto LoadModels () -> void;
    auto CreateObjects() -> void;
    
    auto OnLoad() -> void override;
    auto LoadPasses() -> void;
    
    auto Tick(float deltaTime) -> void override;
    auto ChangeShowcase (
        const std::string& modelName, 
        const std::string& hxcolor, 
        const TransformComponent& adjustedTransform
    ) -> void;
    auto UpdateFreeCamera(float deltaTime) -> void;
    auto UpdateShowcaseCamera(float deltaTime) -> void;
};
