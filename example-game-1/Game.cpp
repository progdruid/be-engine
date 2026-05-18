
#include "Game.h"

#include <umbrellas/include-glfw.h>

#include "BeAssetRegistry.h"
#include "BeWindow.h"
#include "BeInput.h"
#include "BeRenderer.h"
#include "scenes/BeSceneManager.h"
#include "scenes/MenuScene.h"
#include "scenes/MainScene.h"

Game::Game() = default;
Game::~Game() = default;


auto Game::Run() -> int {
    Width = 1920;
    Height = 1080;

    Window = std::make_shared<BeWindow>(Width, Height, "be: example game 1");
    Width = Window->GetFramebufferWidth();
    Height = Window->GetFramebufferHeight();
    Renderer = std::make_shared<BeRenderer>(Width, Height, static_cast<void*>(Window->GetGlfwWindow()));
    Renderer->LaunchDevice();
    
    Input = std::make_unique<BeInput>(Window->GetGlfwWindow());
    
    SetupScenes();

    MainLoop();

    BeAssetRegistry::Shutdown();
    
    return 0;
}

auto Game::SetupScenes() -> void {
    SceneManager = std::make_unique<BeSceneManager>();

    auto menuScene = std::make_unique<MenuScene>(this);
    auto mainScene = std::make_unique<MainScene>(this);

    SceneManager->RegisterScene("menu", std::move(menuScene));
    SceneManager->RegisterScene("main", std::move(mainScene));

    SceneManager->GetScene<MenuScene>("menu")->Prepare();
    SceneManager->GetScene<MainScene>("main")->Prepare();

    SceneManager->RequestSceneChange("menu");
    SceneManager->ApplyPendingSceneChange();
}

auto Game::MainLoop() -> void {
    double lastTime = glfwGetTime();

    while (!Window->ShouldClose()) {
        Window->PollEvents();
        Input->Update();

        const double now = glfwGetTime();
        const float dt = static_cast<float>(now - lastTime);
        lastTime = now;

        const auto activeScene = SceneManager->GetActiveScene<BaseScene>();
        if (activeScene) {
            activeScene->Tick(dt);
        }

        SceneManager->ApplyPendingSceneChange();
        Renderer->Render();
    }
}
