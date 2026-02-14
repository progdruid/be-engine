#pragma once

#include <scenes/BeScene.h>

class Game;

class BaseScene : public BeScene {

    protect Game* GameIns = nullptr;

    expose
    explicit BaseScene(Game* game) : GameIns(game) {}
    ~BaseScene() override = default;

    virtual auto Prepare() -> void {}
    virtual auto Tick(float deltaTime) -> void {}
};
