#pragma once

#include <memory>
#include <umbrellas/common.hpp>

#include "standard-game/BeStandardBaseScene.h"
#include "BePassSequence.h"

class BeInput;
class BeWindow;
class BeRenderer;
class BeSceneManager;

struct ImFont;

class MenuScene : public BeStandardBaseScene {
    hide
    BePassSequence _sequence;
    ImFont* _bodyFont = nullptr;
    ImFont* _titleFont = nullptr;

    expose
    explicit MenuScene(BeStandardGame* game);
    ~MenuScene() override;

    auto Prepare() -> void override {}
    auto OnLoad() -> void override;
    auto Tick(float deltaTime) -> void override;
    auto Render() -> void override;

    auto RunUI() -> void;
};
