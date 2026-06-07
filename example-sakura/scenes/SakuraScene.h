#pragma once
#include <filesystem>
#include <umbrellas/access-modifiers.hpp>

#include "BaseScene.h"
#include "entt/entt.hpp"

struct BeProp;
class BeInput;
class BeCamera;
class OrbitCameraController;
class FreeCameraController;
class BeWindow;
class BeRenderer;
class BeStandardRenderMachine;
class BeMaterial;
class LuaSceneLoader;

class SakuraScene : public BaseScene {
    hide
    entt::registry _registry;
    std::shared_ptr<BeCamera> _camera;
    std::unique_ptr<OrbitCameraController> _orbitCameraController;
    std::unique_ptr<FreeCameraController> _freeCameraController;
    bool _useOrbitCamera = false;

    std::shared_ptr<BeProp> _cube, _anvil, _sakura, _sakura2, _emissiveCube, _moon, _testSphere, _axe, _katana, _rustySphere;
    std::shared_ptr<BeMaterial> _uniformMaterial;
    std::shared_ptr<BeMaterial> _dofMaterial; bool _dofEnabled = false;
    std::unique_ptr<BeStandardRenderMachine> _machine;
    std::unique_ptr<LuaSceneLoader> _sceneLoader;
    std::filesystem::file_time_type _sceneLastWriteTime{};
    
    expose
    explicit SakuraScene(Game* game);
    ~SakuraScene() override;

    auto Prepare() -> void override;
    auto OnLoad() -> void override;
    auto Tick(float deltaTime) -> void override;

    hide
    auto RebuildPasses() -> void;
};
