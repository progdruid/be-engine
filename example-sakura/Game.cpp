
#include "Game.h"

#include <filesystem>

#include <umbrellas/include-glfw.h>
#include <sen-rhi/SenBackend.h>

#include "BeShaderLibrary.h"
#include "BeWindow.h"
#include "BeInput.h"
#include "BeRenderer.h"

#include "scenes/BeSceneManager.h"
#include "scenes/ShowcaseScene.h"
#include "scenes/MenuScene.h"
#include "scenes/SakuraScene.h"
#include "scenes/RiftScene.h"
#include "scenes/OldScene.h"

Game::Game() = default;
Game::~Game() = default;


auto Game::Run() -> int {
    Width = 1920;
    Height = 1080;
    
    Window = std::make_shared<BeWindow>(0, 0, "be: example sakura", BeWindowMode::Fullscreen);
    Renderer = std::make_shared<BeRenderer>(Window->GetReportedPixelWidth(), Window->GetReportedPixelHeight(), static_cast<void*>(Window->GetGlfwWindow()));
    Renderer->LaunchDevice(SenPresentMode::Immediate);
    Width = Renderer->GetSwapchainPixelWidth();
    Height = Renderer->GetSwapchainPixelHeight();

    Input = std::make_unique<BeInput>(Window->GetGlfwWindow());

    SetupScenes();

    MainLoop();

    BeShaderLibrary::Shutdown();

    return 0;
}

auto Game::SetupScenes() -> void {
    SceneManager = std::make_unique<BeSceneManager>();

    auto menuScene = std::make_unique<MenuScene>(this);
    auto mainScene = std::make_unique<SakuraScene>(this);
    auto showcase  = std::make_unique<ShowcaseScene>(this);
    auto rift      = std::make_unique<RiftScene>(this);
    auto oldScene  = std::make_unique<OldScene>(this);

    SceneManager->RegisterScene("menu", std::move(menuScene));
    SceneManager->RegisterScene("sakura", std::move(mainScene));
    SceneManager->RegisterScene("showcase", std::move(showcase));
    SceneManager->RegisterScene("rift", std::move(rift));
    SceneManager->RegisterScene("old", std::move(oldScene));

    SceneManager->GetScene<BaseScene>("menu")->Prepare();
    SceneManager->GetScene<BaseScene>("sakura")->Prepare();
    SceneManager->GetScene<BaseScene>("showcase")->Prepare();
    SceneManager->GetScene<BaseScene>("rift")->Prepare();
    SceneManager->GetScene<BaseScene>("old")->Prepare();

    SceneManager->RequestSceneChange("menu");
    SceneManager->ApplyPendingSceneChange();
}

auto Game::MainLoop() -> void {
    double lastTime = glfwGetTime();

    while (!Window->ShouldClose()) {
        Window->PollEvents();
        Input->Update();

        if (Input->GetKeyDown(GLFW_KEY_F6)) {
            const std::filesystem::path changed[] = { "shaders/backbuffer.hlsl" };
            SenBackend::ReloadSources(changed);
        }

        const double now = glfwGetTime();
        const float dt = static_cast<float>(now - lastTime);
        lastTime = now;

        const auto activeScene = SceneManager->GetActiveScene<BaseScene>();
        if (activeScene)
            activeScene->Tick(dt);

        Renderer->Render();
        SceneManager->ApplyPendingSceneChange();
    }
}
