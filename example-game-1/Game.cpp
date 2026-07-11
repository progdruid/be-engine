
#include "Game.h"

#include <umbrellas/include-glfw.h>

#include "BeAssetRegistry.h"
#include "BeShaderLibrary.h"
#include "BeWindow.h"
#include "BeInput.h"
#include "BeRenderer.h"
#include "BeTexture.h"
#include "scenes/BeSceneManager.h"
#include "scenes/MenuScene.h"
#include "scenes/MainScene.h"

Game::Game() = default;
Game::~Game() = default;


auto Game::Run() -> int {
    Width = 1920;
    Height = 1080;

    Window = std::make_shared<BeWindow>(Width, Height, "be: example game 1");
    Width = Window->GetReportedPixelWidth();
    Height = Window->GetReportedPixelHeight();
    Renderer = std::make_shared<BeRenderer>(Width, Height, static_cast<void*>(Window->GetGlfwWindow()));
    Renderer->LaunchDevice();
    
    Input = std::make_unique<BeInput>(Window->GetGlfwWindow());

    BeShaderLibrary::RegisterDefaultTexture("white",
        BeTexture::Create("white")
        .SetSize(1, 1)
        .SetUsage(SenTextureUsage::ShaderResource)
        .SetFormat(SenFormat::RGBA8_Unorm)
        .FillWithColor(glm::vec4(1.f))
        .Build()
    );
    BeShaderLibrary::RegisterDefaultTexture("black",
        BeTexture::Create("black")
        .SetSize(1, 1)
        .SetUsage(SenTextureUsage::ShaderResource)
        .SetFormat(SenFormat::RGBA8_Unorm)
        .FillWithColor(glm::vec4(0.f, 0.f, 0.f, 1.f))
        .Build()
    );
    BeShaderLibrary::RegisterDefaultTexture("storage-black",
        BeTexture::Create("storage-black")
        .SetSize(1, 1)
        .SetUsage(SenTextureUsage::ShaderResource | SenTextureUsage::Storage)
        .SetFormat(SenFormat::RGBA8_Unorm)
        .FillWithColor(glm::vec4(0.f, 0.f, 0.f, 1.f))
        .Build()
    );
    BeShaderLibrary::RegisterDefaultTexture("black-cube",
        BeTexture::Create("black-cube")
        .SetSize(1, 1)
        .SetCubemap(true)
        .SetUsage(SenTextureUsage::ShaderResource)
        .SetFormat(SenFormat::RGBA8_Unorm)
        .FillWithColor(glm::vec4(0.f, 0.f, 0.f, 1.f))
        .Build()
    );
    BeShaderLibrary::RegisterDefaultTexture("default-orm",
        BeTexture::Create("default-orm")
        .SetSize(1, 1)
        .SetUsage(SenTextureUsage::ShaderResource)
        .SetFormat(SenFormat::RGBA8_Unorm)
        .FillWithColor(glm::vec4(0.f, 1.f, 1.f, 1.f))
        .Build()
    );
    BeShaderLibrary::RegisterDefaultTexture("flat-normal",
        BeTexture::Create("flat-normal")
        .SetSize(1, 1)
        .SetUsage(SenTextureUsage::ShaderResource)
        .SetFormat(SenFormat::RGBA8_Unorm)
        .FillWithColor(glm::vec4(0.5f, 0.5f, 1.0f, 1.0f))
        .Build()
    );

    SetupScenes();

    MainLoop();

    Renderer->ClearPasses();
    BeShaderLibrary::Shutdown();

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

        Renderer->Render();
        SceneManager->ApplyPendingSceneChange();
    }
}
