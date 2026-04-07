#pragma once

#include <memory>
#include <umbrellas/access-modifiers.hpp>

#include "BaseScene.h"

class BeInput;
class BeWindow;
class BeRenderer;
class BeSceneManager;

struct ImFont;

class MenuScene : public BaseScene {
    hide
    ImFont* _titleFont = nullptr;

    expose
    explicit MenuScene(Game* game);
    ~MenuScene() override = default;

    auto Prepare() -> void override {}
    auto OnLoad() -> void override;
    auto Tick(float deltaTime) -> void override;

    auto RunUI() -> void;
};
