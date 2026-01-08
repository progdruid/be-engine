
#include "Game.h"

#include <glfw/glfw3.h>

#include "BeWindow.h"
#include "BeInput.h"
#include "BeRenderer.h"

#include "scenes/BeSceneManager.h"
#include "scenes/MenuScene.h"
#include "scenes/MainScene.h"

Game::Game() = default;
Game::~Game() = default;


auto Game::Run() -> int {
    _width = 1920;
    _height = 1080;
    
    _window = std::make_shared<BeWindow>(_width, _height, "be: example game 1");
    _renderer = std::make_shared<BeRenderer>(_width, _height, _window->GetHwnd());
    _renderer->LaunchDevice();
    
    _input = std::make_unique<BeInput>(_window->GetGlfwWindow());
    
    SetupScenes();

    MainLoop();

    return 0;
}

auto Game::SetupScenes() -> void {
    _sceneManager = std::make_unique<BeSceneManager>();

    auto menuScene = std::make_unique<MenuScene>(_sceneManager.get(), _renderer, _window, _input);
    auto mainScene = std::make_unique<MainScene>(_renderer, _window, _input);

    _sceneManager->RegisterScene("menu", std::move(menuScene));
    _sceneManager->RegisterScene("main", std::move(mainScene));

    _sceneManager->GetScene<MenuScene>("menu")->Prepare();
    _sceneManager->GetScene<MainScene>("main")->Prepare();

    _renderer->BakeModels();

    _sceneManager->RequestSceneChange("menu");
    _sceneManager->ApplyPendingSceneChange();
}

auto Game::MainLoop() -> void {
    double lastTime = glfwGetTime();

    while (!_window->ShouldClose()) {
        _window->PollEvents();
        _input->Update();

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
