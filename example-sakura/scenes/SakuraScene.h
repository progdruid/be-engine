#pragma once
#include <array>
#include <string>
#include <umbrellas/common.hpp>
#include <umbrellas/include-glm.h>

#include "FullScene.h"
#include "FpsCounter.h"

struct BeProp;
class OrbitCameraController;
class FreeCameraController;
class RigCameraController;
class BeMaterial;

struct SakuraSceneSettings {
    struct {
        uint8_t CurrentSceneIndex = 0;
        std::array<std::string, 4> Paths = {
            "assets/lua-scenes/sakura_scene.lua",
            "assets/lua-scenes/sakura_scene_1.lua",
            "assets/lua-scenes/sakura_scene_2.lua",
            "assets/lua-scenes/spinning_lights_500_scene.lua",
        };
    } SceneFile;

    struct {
        bool Enabled = true;
        std::string HdrPath = "assets/moonrise_puresky.hdr";
    } Skybox;

    struct {
        uint32_t MipCount = 5;
        std::string DirtTexturePath = "assets/bloom-dirt-mask.png";
    } Bloom;

    struct {
        bool Enabled = false;
        float MinFocalDistance = 0.5f;
        float FocusSpeed = 5.0f;
    } DepthOfField;
};

class SakuraScene : public FullScene {
    expose
    SakuraSceneSettings Settings;

    hide
    std::unique_ptr<OrbitCameraController> _orbitCameraController;
    std::unique_ptr<FreeCameraController> _freeCameraController;
    int _cameraMode = 0;

    std::shared_ptr<BeMaterial> _dofMaterial;
    int _pendingSceneIndex = -1;

    FpsCounter _fpsCounter;

    expose
    explicit SakuraScene(Game* game);
    ~SakuraScene() override;
    
    expose auto Prepare() -> void override;
    expose auto Tick(float deltaTime) -> void override;

    protect
    auto DefineAssets() -> void override;
    auto DefineSettings() -> void override;
    auto DefineScene() -> void override;
    auto DefinePasses() -> void override;

    hide
};
