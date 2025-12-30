#pragma once

#include <memory>
#include <umbrellas/access-modifiers.hpp>

#include "BaseScene.h"

class BeInput;
class BeWindow;
class BeAssetRegistry;
class BeRenderer;
class BeSceneManager;

class MenuScene : public BaseScene {
    hide
    std::shared_ptr<BeRenderer> _renderer;
    std::shared_ptr<BeAssetRegistry> _assetRegistry;
    std::shared_ptr<BeWindow> _window;
    std::shared_ptr<BeInput> _input;
    
    expose
    explicit MenuScene(
        BeSceneManager* sceneManager,
        const std::shared_ptr<BeRenderer>& renderer,
        const std::shared_ptr<BeAssetRegistry>& assetRegistry,
        const std::shared_ptr<BeWindow>& window,
        const std::shared_ptr<BeInput>& input
    );
    ~MenuScene() override = default;

    auto Prepare() -> void override {}
    auto OnLoad() -> void override;
    auto Tick(float deltaTime) -> void override {}

    auto RunUI() -> void;
};
