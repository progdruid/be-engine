#pragma once

#include <string>
#include <umbrellas/access-modifiers.hpp>
#include <umbrellas/include-glm.h>

#include "BaseScene.h"
#include "Components.h"
#include "BeAssetRegistry.h"
#include "entt/entt.hpp"
class BeInput;
class BeCamera;
class OrbitCameraController;
class FreeCameraController;
class BeWindow;
class BeRenderer;
class BeStandardRenderMachine;
class BeMaterial;

class ShowcaseScene : public BaseScene {
    hide
    BeAssetRegistry _assetRegistry;
    entt::registry _registry;
    std::shared_ptr<BeCamera> _camera;
    std::shared_ptr<BeMaterial> _uniformMaterial;
    std::unique_ptr<BeStandardRenderMachine> _machine;
    std::unique_ptr<OrbitCameraController> _orbitCameraController;
    std::unique_ptr<FreeCameraController> _freeCameraController;
    bool _useOrbitCamera = true;
    bool _animatedTransitions = true;
    bool _pixelationEnabled = true;
    bool _pixelEdgesEnabled = true;
    float _pixelSize = 8.0f;
    entt::entity _showcasedEntity = entt::null;
    TransformComponent _showcasedTransform = {};

    enum class PopState { Idle, Bracing, Expanding };
    PopState _popState = PopState::Idle;
    float _expandTime = 0.f;
    int _heldKey = -1;
    bool _swapDone = false;
    std::string _pendingModel;
    std::string _pendingColor;
    TransformComponent _pendingTransform = {};
    float _braceTime = 0.f;
    static constexpr float _braceScale = 0.85f;
    static constexpr float _braceDuration = 0.15f;
    static constexpr float _expandDuration = 0.15f;

    expose
    explicit ShowcaseScene(Game* game);
    ~ShowcaseScene() override;

    auto Prepare() -> void override;
    auto CreateTargetTextures() -> void;
    auto LoadModels (BeStandardRenderMachine& machine) -> void;
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
