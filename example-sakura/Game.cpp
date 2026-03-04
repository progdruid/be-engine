
#include "Game.h"

#include <glfw/glfw3.h>

#include "BeAssetRegistry.h"
#include "BeWindow.h"
#include "BeInput.h"
#include "BeRenderer.h"
#include "BeTexture.h"
#include "basic-render-pipeline/BeBRPSubmissionBuffer.h"

#include "scenes/BeSceneManager.h"
#include "scenes/ShowcaseScene.h"
#include "scenes/MenuScene.h"
#include "scenes/SakuraScene.h"

Game::Game() = default;
Game::~Game() = default;


auto Game::Run() -> int {
    Width = 1920;
    Height = 1080;
    
    //Window = std::make_shared<BeWindow>(Width, Height, "be: example game 1");
    Window = std::make_shared<BeWindow>(0, 0, "be: example sakura", BeWindowMode::BorderlessFullscreen);
    Width = Window->GetWidth();
    Height = Window->GetHeight();
    Renderer = std::make_shared<BeRenderer>(Width, Height, Window->GetHwnd());
    Renderer->LaunchDevice();
    const auto device = Renderer->GetDevice();
    
    SubmissionBuffer = std::make_shared<BeBRPSubmissionBuffer>();
    SubmissionBuffer->Init(device);
    Input = std::make_unique<BeInput>(Window->GetGlfwWindow());
    
    
    BeAssetRegistry::InjectRenderer(Renderer);
    
    BeTexture::Create("white")
    .SetSize(1, 1)
    .SetUsage(SenTextureUsage::ShaderResource)
    .SetFormat(SenFormat::RGBA8_Unorm)
    .FillWithColor(glm::vec4(1.f))
    .AddToRegistry()
    .BuildNoReturn(device);
    BeTexture::Create("black")
    .SetSize(1, 1)
    .SetUsage(SenTextureUsage::ShaderResource)
    .SetFormat(SenFormat::RGBA8_Unorm)
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
    auto mainScene = std::make_unique<SakuraScene>(this);
    auto showcase  = std::make_unique<ShowcaseScene>(this); 
    
    SceneManager->RegisterScene("menu", std::move(menuScene));
    SceneManager->RegisterScene("sakura", std::move(mainScene));
    SceneManager->RegisterScene("showcase", std::move(showcase));
    
    SceneManager->GetScene<BaseScene>("menu")->Prepare();
    SceneManager->GetScene<BaseScene>("sakura")->Prepare();
    SceneManager->GetScene<BaseScene>("showcase")->Prepare();
    
    SubmissionBuffer->BakeMeshes();

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
