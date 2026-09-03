#include "VideoScene.h"

#include <umbrellas/include-glfw.h>

#include "BeCamera.h"
#include "BeInput.h"
#include "FreeCameraController.h"
#include "OrbitCameraController.h"
#include "standard-game/BeStandardGame.h"
#include "lua/BeLua.h"
#include "scenes/BeSceneManager.h"
#include "standard-render-machine/BeStandardRenderMachine.h"

VideoScene::VideoScene(BeStandardGame* game) : BeStandardFullScene(game) {}
VideoScene::~VideoScene() = default;

auto VideoScene::Prepare() -> void {
    SetWatchFile("assets/lua-scenes/video_scene.lua", [this] -> void { Reload(); });

    BeStandardFullScene::Prepare();

    _freeCameraController = std::make_unique<FreeCameraController>(_camera.get());
    _orbitCameraController = std::make_unique<OrbitCameraController>(_camera.get(), glm::vec3(0.f), 30.f, 30.f);
    _camera->Position = { 0.f, 10.f, -24.f };
    _camera->LookIn(glm::normalize(glm::vec3(0.f, -8.f, 24.f)));
}

auto VideoScene::DefineAssets() -> void {
    _machine->ClearTargets();
    _machine->ClearMeshes();

    _machine->DeclareGBufferTarget("Video_Albedo_RGB",      SenFormat::R11G11B10_Float);
    _machine->DeclareGBufferTarget("Video_WorldNormal_XYZ", SenFormat::RGBA16_Float);
    _machine->DeclareGBufferTarget("Video_ORM_RGB",         SenFormat::RGBA8_Unorm);
    _machine->DeclareGBufferTarget("Video_Emissive_RGB",    SenFormat::R11G11B10_Float);
    _machine->DeclareDepthTarget  ("Video_Depth",           SenFormat::Depth32);
    _machine->DeclareTextureTarget("Video_HDR",             SenFormat::R11G11B10_Float);
    _machine->DeclareTextureTarget("Video_Bloom",           SenFormat::R11G11B10_Float);
    _machine->DeclareTextureTarget("Video_Tonemapped",      SenFormat::R11G11B10_Float);

    _assetRegistry.ClearTextures();
    _assetRegistry.ClearProps();
    ApplyLuaAssets(_sceneLua->Call("makeData")["Assets"]);
    
    _machine->BakeMeshes();
}

auto VideoScene::DefineSettings() -> void {
    const auto settings = _sceneLua->Call("makeData")["Settings"];
    ApplyLuaSettings(settings);
}

auto VideoScene::DefineScene() -> void {
    _registry.clear();
    
    const auto objects = _sceneLua->Call("makeData")["Objects"];
    ApplyLuaScene(objects);
}

auto VideoScene::DefinePasses() -> void {
    _machine->ClearPasses();

    _machine->AddGeometryPass();
    _machine->AddLightingPass("Video_HDR");
    _machine->AddBloomPass(5, "Video_HDR", "Video_Bloom");
    _machine->AddTonemapperPass("Video_Bloom", "Video_Tonemapped");
    _machine->AddBackbufferPass("Video_Tonemapped");

    _machine->InitialisePasses();
}

auto VideoScene::Tick(float deltaTime) -> void {
    if (_game->Input->GetKeyDown(GLFW_KEY_ESCAPE)) {
        _game->Input->SetMouseCapture(false);
        _game->SceneManager->RequestSceneChange("menu");
        return;
    }

    if (_game->Input->GetKeyDown(GLFW_KEY_C)) {
        _useOrbitCamera = !_useOrbitCamera;
    }

    if (_useOrbitCamera) {
        _game->Input->SetMouseCapture(false);
        _orbitCameraController->Update(deltaTime, _game->Input.get());
    } else {
        _freeCameraController->Update(deltaTime, _game->Input.get());
    }

    BeStandardFullScene::Tick(deltaTime);
}
