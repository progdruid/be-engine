#pragma once

#include "FullScene.h"

class VideoScene : public FullScene {
    expose
    explicit VideoScene(Game* game);
    ~VideoScene() override;

    auto Tick(float deltaTime) -> void override;

    protect
    auto DefineAssets() -> void override;
    auto DefinePasses() -> void override;
};
