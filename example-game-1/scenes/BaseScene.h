#pragma once

#include <scenes/BeScene.h>

class BeSceneManager;

class BaseScene : public BeScene {
protected:
    BeSceneManager* _sceneManager = nullptr;

public:
    BaseScene(BeSceneManager* sceneManager = nullptr) : _sceneManager(sceneManager) {}
    virtual ~BaseScene() = default;

    virtual auto Prepare() -> void {}
    virtual auto Tick(float deltaTime) -> void {}
};
