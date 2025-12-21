#pragma once

#include "BaseScene.h"

class BeSceneManager;

class MenuScene : public BaseScene {
public:
    MenuScene(BeSceneManager* sceneManager);
    ~MenuScene() override = default;

    auto Prepare() -> void override {}
    auto Tick(float deltaTime) -> void override {}
    auto OnLoad() -> void override;
};
