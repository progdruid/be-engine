#pragma once

#include <umbrellas/common.hpp>
#include <scenes/BeScene.h>

class Game;

class BaseScene : public BeScene {

    protect
    Game* _gameIns = nullptr;

    expose
    explicit BaseScene(Game* game);
    ~BaseScene() override;

    virtual auto Prepare() -> void {}
    virtual auto Tick(float deltaTime) -> void {}
    virtual auto Render() -> void {}
};
