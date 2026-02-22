#pragma once

#include <umbrellas/access-modifiers.hpp>
#include <umbrellas/include-glm.h>

#include "BaseScene.h"
#include "entt/entt.hpp"

struct TransformComponent;
class BeInput;
class BeCamera;
class OrbitCameraController;
class FreeCameraController;
class BeWindow;
class BeRenderer;
class BeBRPSubmissionBuffer;

class ShowcaseScene : public BaseScene {
    hide
    entt::registry _registry;
    std::shared_ptr<BeCamera> _camera;
    std::unique_ptr<OrbitCameraController> _orbitCameraController;
    std::unique_ptr<FreeCameraController> _freeCameraController;
    bool _useOrbitCamera = true;

    expose
    explicit ShowcaseScene(Game* game);
    ~ShowcaseScene() override;

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
};
