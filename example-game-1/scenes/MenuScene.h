#pragma once

#include <umbrellas/common.hpp>

#include "BaseScene.h"

class BeInput;
class BeWindow;
class BeRenderer;
class BeSceneManager;
class BeImGuiPass;

class MenuScene : public BaseScene {
    hide
    std::unique_ptr<BeImGuiPass> _imguiPass;

    expose
    explicit MenuScene(Game* game);
    ~MenuScene() override;

    auto Prepare() -> void override {}
    auto OnLoad() -> void override;
    auto Tick(float deltaTime) -> void override {}

    auto RunUI() -> void;
};
