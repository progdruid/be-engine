#include "BeStandardGame.h"

#include <umbrellas/include-glfw.h>

#include "BeShaderLibrary.h"
#include "BeFileWatcher.h"
#include "BeInput.h"
#include "BeRenderer.h"

#include "scenes/BeSceneManager.h"
#include "BeStandardBaseScene.h"

BeStandardGame::BeStandardGame(const BeStandardGameConfig& config) {
    Width = config.Width;
    Height = config.Height;

    Window = std::make_shared<BeWindow>(Width, Height, config.Title, config.WindowMode);
    Renderer = std::make_shared<BeRenderer>(Window->GetReportedPixelWidth(), Window->GetReportedPixelHeight(), static_cast<void*>(Window->GetGlfwWindow()));
    Renderer->LaunchDevice(config.PresentMode);
    Width = Renderer->GetSwapchainPixelWidth();
    Height = Renderer->GetSwapchainPixelHeight();

    Input = std::make_shared<BeInput>(Window->GetGlfwWindow());

    SceneManager = std::make_unique<BeSceneManager>();
}

BeStandardGame::~BeStandardGame() = default;

auto BeStandardGame::Run() -> int {
    MainLoop();

    BeShaderLibrary::Shutdown();

    return 0;
}

auto BeStandardGame::MainLoop() -> void {
    double lastTime = glfwGetTime();

    while (!Window->ShouldClose()) {
        Window->PollEvents();
        Input->Update();

        const double now = glfwGetTime();
        const float dt = static_cast<float>(now - lastTime);
        lastTime = now;

        BeFileWatcher::Poll(dt);

        if (!Renderer->PollResize()) {
            continue;
        }

        const auto activeScene = SceneManager->GetActiveScene<BeStandardBaseScene>();
        if (activeScene) {
            activeScene->Tick(dt);
            activeScene->Render();
        }

        Renderer->Render();
        SceneManager->ApplyPendingSceneChange();
    }
}
