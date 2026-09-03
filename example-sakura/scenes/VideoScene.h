#pragma once

#include <memory>

#include <umbrellas/include-glm.h>

#include "standard-game/BeStandardFullScene.h"

class FreeCameraController;
class OrbitCameraController;

class VideoScene : public BeStandardFullScene {
    hide
    std::unique_ptr<FreeCameraController> _freeCameraController;
    std::unique_ptr<OrbitCameraController> _orbitCameraController;
    bool _useOrbitCamera = true;

    expose
    explicit VideoScene(BeStandardGame* game);
    ~VideoScene() override;

    auto Prepare() -> void override;
    auto Tick(float deltaTime) -> void override;

    protect
    auto DefineAssets() -> void override;
    auto DefineSettings() -> void override;
    auto DefineScene() -> void override;
    auto DefinePasses() -> void override;
};
