#pragma once
#include <filesystem>
#include <glm/vec3.hpp>
#include <umbrellas/common.hpp>

#include "BaseScene.h"
#include "BeAssetRegistry.h"
#include "entt/entt.hpp"

struct BeProp;
class BeInput;
class BeCamera;
class OrbitCameraController;
class FreeCameraController;
class RigCameraController;
class BeWindow;
class BeRenderer;
class BeStandardRenderMachine;
class BeMaterial;

class SakuraScene : public BaseScene {
    hide
    BeAssetRegistry _assetRegistry;
    entt::registry _registry;
    std::shared_ptr<BeCamera> _camera;
    std::unique_ptr<OrbitCameraController> _orbitCameraController;
    std::unique_ptr<FreeCameraController> _freeCameraController;
    std::unique_ptr<RigCameraController> _rigCameraController;
    int _cameraMode = 0;   // 0 = free, 1 = orbit, 2 = rig

    std::shared_ptr<BeMaterial> _uniformMaterial;
    std::shared_ptr<BeMaterial> _dofMaterial; bool _dofEnabled = false;
    std::unique_ptr<BeStandardRenderMachine> _machine;
    std::filesystem::file_time_type _sceneLastWriteTime{};
    std::string _scenePath = "assets/sakura_scene.lua";

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
