#pragma once
#include <filesystem>
#include <string>
#include <umbrellas/common.hpp>
#include <umbrellas/include-glm.h>
#include <entt/entt.hpp>

#include "BaseScene.h"
#include "BeAssetRegistry.h"
#include "standard-render-machine/BeStandardRenderMachine.h"

struct BeProp;
class BeInput;
class BeCamera;
class OrbitCameraController;
class BeFreeCameraController;
class RigCameraController;
class BeWindow;
class BeRenderer;
class BeMaterial;

struct SakuraSceneSettings {
    BeSRMSettings SRM = {
        .Shadow = {
            .Bias = 16.f / 100000.f,
        },
        .Bloom = {
            .Threshold = 2.5f,
            .Knee = 0.7f,
            .Intensity = 0.7f,
            .Clamp = 4.0f,
        },
        .Tonemapper = {
            .Exposure = 0.25f,
            .Contrast = 1.70f,
        },
    };

    struct {
        std::string Path = "assets/sakura_scene.lua";
    } SceneFile;

    struct {
        float NearPlane = 0.1f;
        float FarPlane = 250.0f;
    } Camera;

    struct {
        glm::vec3 Color = glm::vec3(0.0f);
    } Ambient;

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

    struct {
        glm::vec3 ClearColor = { 0.f / 255.f, 23.f / 255.f, 31.f / 255.f };
    } Background;
};

class SakuraScene : public BaseScene {
    expose
    SakuraSceneSettings Settings;

    hide
    BeAssetRegistry _assetRegistry;
    entt::registry _registry;
    std::shared_ptr<BeCamera> _camera;
    std::unique_ptr<OrbitCameraController> _orbitCameraController;
    std::unique_ptr<BeFreeCameraController> _freeCameraController;
    std::unique_ptr<RigCameraController> _rigCameraController;
    int _cameraMode = 0;   // 0 = free, 1 = orbit, 2 = rig

    std::shared_ptr<BeMaterial> _dofMaterial;
    std::unique_ptr<BeStandardRenderMachine> _machine;
    std::filesystem::file_time_type _sceneLastWriteTime{};

    float _time = 0.0f;
    
    expose
    explicit SakuraScene(Game* game);
    ~SakuraScene() override;

    expose auto Prepare() -> void override;
    hide auto LoadProps() -> void;
    
    expose auto OnLoad() -> void override;
    hide auto RebuildPasses() -> void;
    hide auto LoadSceneFile() -> void;

    expose auto Tick(float deltaTime) -> void override;
};
