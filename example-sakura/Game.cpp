
#include "Game.h"

#include <glfw/glfw3.h>

#include "BeAssetRegistry.h"
#include "BeWindow.h"
#include "BeInput.h"
#include "BeRenderer.h"
#include "BeTexture.h"
#include "basic-render-pipeline/BeBRPSubmissionBuffer.h"

#include "scenes/BeSceneManager.h"
#include "scenes/LowPolyShowcaseScene.h"
#include "scenes/MenuScene.h"
#include "scenes/MainScene.h"

Game::Game() = default;
Game::~Game() = default;


auto Game::Run() -> int {
    Width = 1920;
    Height = 1080;
    
    Window = std::make_shared<BeWindow>(Width, Height, "be: example game 1");
    Renderer = std::make_shared<BeRenderer>(Width, Height, Window->GetHwnd());
    Renderer->LaunchDevice();
    SubmissionBuffer = std::make_shared<BeBRPSubmissionBuffer>();
    
    Input = std::make_unique<BeInput>(Window->GetGlfwWindow());
    
    const auto device = Renderer->GetDevice();
    
    BeAssetRegistry::InjectRenderer(Renderer);
    
    BeTexture::Create("white")
    .SetSize(1, 1)
    .SetBindFlags(D3D11_BIND_SHADER_RESOURCE)
    .SetFormat(DXGI_FORMAT_R8G8B8A8_UNORM)
    .FillWithColor(glm::vec4(1.f))
    .AddToRegistry()
    .BuildNoReturn(device);
    BeTexture::Create("black")
    .SetSize(1, 1)
    .SetBindFlags(D3D11_BIND_SHADER_RESOURCE)
    .SetFormat(DXGI_FORMAT_R8G8B8A8_UNORM)
    .FillWithColor(glm::vec4(0.f, 0.f, 0.f, 1.f))
    .AddToRegistry()
    .BuildNoReturn(device);
    
    SetupScenes();

    MainLoop();

    return 0;
}

auto Game::SetupScenes() -> void {
    SceneManager = std::make_unique<BeSceneManager>();

    auto menuScene = std::make_unique<MenuScene>(this);
    auto mainScene = std::make_unique<MainScene>(this);
    auto showcase  = std::make_unique<LowPolyShowcaseScene>(this); 
    
    SceneManager->RegisterScene("menu", std::move(menuScene));
    SceneManager->RegisterScene("main", std::move(mainScene));
    SceneManager->RegisterScene("showcase", std::move(showcase));
    
    SceneManager->GetScene<BaseScene>("menu")->Prepare();
    SceneManager->GetScene<BaseScene>("main")->Prepare();
    SceneManager->GetScene<BaseScene>("showcase")->Prepare();
    
    Renderer->BakeModels();

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

        Renderer->UniformData.Time = now;

        const auto activeScene = SceneManager->GetActiveScene<BaseScene>();
        if (activeScene) {
            activeScene->Tick(dt);
        }

        Renderer->Render();
        SceneManager->ApplyPendingSceneChange();
    }
}
