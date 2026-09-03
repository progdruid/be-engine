#pragma once

#include <umbrellas/common.hpp>
#include <scenes/BeScene.h>

class BeStandardGame;

class BeStandardBaseScene : public BeScene {

    protect
    BeStandardGame* _game = nullptr;

    expose
    explicit BeStandardBaseScene(BeStandardGame* game);
    ~BeStandardBaseScene() override;

    virtual auto Prepare() -> void {}
    virtual auto Tick(float deltaTime) -> void {}
    virtual auto Render() -> void {}
};
