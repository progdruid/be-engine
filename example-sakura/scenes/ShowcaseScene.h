#pragma once

#include <string>
#include <umbrellas/common.hpp>
#include <umbrellas/include-glm.h>

#include "FullScene.h"
#include "Components.h"

class OrbitCameraController;
class FreeCameraController;
class BeStandardRenderMachine;
class BeMaterial;

class ShowcaseScene : public FullScene {
    hide
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
    auto Tick(float deltaTime) -> void override;

    protect
    auto DefineSettings() -> void override;
    auto DefineAssets() -> void override;
    auto DefineScene() -> void override;
    auto DefinePasses() -> void override;

    hide
    auto LoadModels(BeStandardRenderMachine& machine) -> void;
    auto CreateObjects() -> void;
    auto ChangeShowcase (
        const std::string& modelName,
        const std::string& hxcolor,
        const TransformComponent& adjustedTransform
    ) -> void;
};
