
#include "Game.h"

#define NOMINMAX

#include "BeWindow.h"
#include "BeAssetRegistry.h"
#include "BeInput.h"
#include "BeRenderer.h"
#include "BeCamera.h"

#include <scenes/BeSceneManager.h>
#include "scenes/MenuScene.h"
#include "scenes/MainScene.h"

Game::Game() = default;
Game::~Game() = default;


auto Game::Run() -> int {
    _width = 1920;
    _height = 1080;

    _window = std::make_unique<BeWindow>(_width, _height, "be: example game 1");
    _assetRegistry = std::make_shared<BeAssetRegistry>();
    const HWND hwnd = _window->getHWND();

    _renderer = std::make_shared<BeRenderer>(_width, _height, hwnd, _assetRegistry);
    _renderer->LaunchDevice();

    SetupCamera(_width, _height);
    SetupScenes();

    _sceneManager->RequestSceneChange("menu");
    _sceneManager->ApplyPendingSceneChange();
    MainLoop();

    return 0;
}
auto Game::SetupScenes() -> void {
    _sceneManager = std::make_unique<BeSceneManager>();

    auto menuScene = std::make_unique<MenuScene>(_sceneManager.get());
    auto mainScene = std::make_unique<MainScene>(_renderer, _assetRegistry, _camera, _input, _width, _height);

    _sceneManager->RegisterScene("menu", std::move(menuScene));
    _sceneManager->RegisterScene("main", std::move(mainScene));

    _sceneManager->GetScene<MenuScene>("menu")->Prepare();
    _sceneManager->GetScene<MainScene>("main")->Prepare();

    _renderer->BakeModels();
}

auto Game::SetupCamera(int width, int height) -> void {
    _camera = std::make_unique<BeCamera>();
    _camera->Width = static_cast<float>(width);
    _camera->Height = static_cast<float>(height);
    _camera->NearPlane = 0.1f;
    _camera->FarPlane = 200.0f;

    _renderer->UniformData.NearFarPlane = {_camera->NearPlane, _camera->FarPlane};
    
    _input = std::make_unique<BeInput>(_window->getGLFWWindow());
}

auto Game::MainLoop() -> void {
    double lastTime = glfwGetTime();

    while (!_window->shouldClose()) {
        _window->pollEvents();
        _input->update();

        const double now = glfwGetTime();
        const float dt = static_cast<float>(now - lastTime);
        lastTime = now;

        _renderer->UniformData.Time = now;

        const auto activeScene = _sceneManager->GetActiveScene<BaseScene>();
        if (activeScene) {
            activeScene->Tick(dt);
        }

        _sceneManager->ApplyPendingSceneChange();
        _renderer->Render();
    }
}
