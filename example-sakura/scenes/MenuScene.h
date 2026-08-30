#pragma once

#include <memory>
#include <umbrellas/common.hpp>

#include "BaseScene.h"
#include "BePassSequence.h"

class BeInput;
class BeWindow;
class BeRenderer;
class BeSceneManager;

struct ImFont;

class MenuScene : public BaseScene {
    hide
    BePassSequence _sequence;
    ImFont* _bodyFont = nullptr;
    ImFont* _titleFont = nullptr;

    expose
    explicit MenuScene(Game* game);
    ~MenuScene() override;

    auto Prepare() -> void override {}
    auto OnLoad() -> void override;
    auto Tick(float deltaTime) -> void override;
    auto Render() -> void override;

    auto RunUI() -> void;
};
