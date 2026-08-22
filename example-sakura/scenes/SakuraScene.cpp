
#include "SakuraScene.h"

#include <filesystem>

#include <umbrellas/include-glfw.h>

#include <imgui/imgui.h>

#include "BeRenderPass.h"
#include "imgui/BeImGuiPass.h"
#include "OrbitCameraController.h"
#include "FreeCameraController.h"
#include "BeAssetRegistry.h"
#include "BeShaderLibrary.h"
#include "lua/BeLua.h"
#include "BeCamera.h"
#include "BeInput.h"
#include "BeMaterial.h"
#include "BeMeshPrimitives.h"
#include "BeProp.h"
#include "BeTexture.h"
#include "BeShader.h"
#include "Game.h"
#include "scenes/BeSceneManager.h"
#include "standard-render-machine/BeStandardRenderMachine.h"

SakuraScene::SakuraScene(Game* game) : FullScene(game) {}
SakuraScene::~SakuraScene() = default;

void SakuraScene::Prepare() {
    SetWatchFile(
        Settings.SceneFile.Paths[Settings.SceneFile.CurrentSceneIndex], 
        [this] -> void { Reload(); }
    );

    FullScene::Prepare();

    _orbitCameraController = std::make_unique<OrbitCameraController>(_camera.get());
    _freeCameraController = std::make_unique<FreeCameraController>(_camera.get());
}

auto SakuraScene::DefineAssets() -> void {
    _machine->ClearTargets();
    _machine->ClearMeshes();
    
    _machine->DeclareGBufferTarget("Sakura_Albedo_RGB",      SenFormat::R11G11B10_Float);
    _machine->DeclareGBufferTarget("Sakura_WorldNormal_XYZ", SenFormat::RGBA16_Float);
    _machine->DeclareGBufferTarget("Sakura_ORM_RGB",         SenFormat::RGBA8_Unorm);
    _machine->DeclareGBufferTarget("Sakura_Emissive_RGB",    SenFormat::R11G11B10_Float);
    _machine->DeclareDepthTarget  ("Sakura_Depth",           SenFormat::Depth32);
    _machine->DeclareTextureTarget("Sakura_HDR",             SenFormat::R11G11B10_Float);
    _machine->DeclareTextureTarget("Sakura_Bloom",           SenFormat::R11G11B10_Float);
    _machine->DeclareTextureTarget("Sakura_DoF",             SenFormat::R11G11B10_Float);
    _machine->DeclareTextureTarget("Sakura_Tonemapper",      SenFormat::R11G11B10_Float);
    _machine->DeclareTextureTarget("Sakura_FXAA",            SenFormat::R11G11B10_Float);
    
    const auto pbrShader = BeShaderLibrary::GetShader("standard-pbr");
    const auto phongShader = BeShaderLibrary::GetShader("standard-phong");
    const auto checkerboardShader = BeShaderLibrary::GetShader("checkerboard");
    
    const auto cube = BeProp::FromMesh(BeMeshPrimitives::Cube(), checkerboardShader, "geometry-main");
    cube->Materials[0]->SetTexture("DiffuseTexture",
        BeTexture::Create("Sakura_Checkerboard")
        .LoadFromFile("assets/checkerboard.png")
        .AddToRegistry(_assetRegistry)
        .Build()
    );
    _machine->RegisterMesh(cube->Mesh);
    
    const auto emissiveCube = BeProp::FromMesh(BeMeshPrimitives::Cube(), pbrShader, "geometry-main");
    emissiveCube->Materials[0]->SetFloat3("EmissiveColor", glm::vec3(1.f) * 4.0f);
    _machine->RegisterMesh(emissiveCube->Mesh);
    
    const auto moon = BeProp::FromMesh(BeMeshPrimitives::Cube(), pbrShader, "geometry-main");
    moon->Materials[0]->SetFloat3("EmissiveColor", glm::vec3(0.7f, 0.7f, 0.99f) * 10.0f);
    _machine->RegisterMesh(moon->Mesh);
    
    const auto testSphere = BeProp::FromMesh(BeMeshPrimitives::Sphere(), pbrShader, "geometry-main");
    testSphere->Materials[0]->SetFloat3("BaseColor", glm::vec3(0.8f, 0.3f, 0.1f));
    testSphere->Materials[0]->SetFloat1("Metallic", 0.8f);
    testSphere->Materials[0]->SetFloat1("Roughness", 0.5f);
    _machine->RegisterMesh(testSphere->Mesh);
    
    const auto anvil = _machine->LoadProp("assets/anvil/scene.gltf", pbrShader);
    anvil->Materials[0]->SetSampler("InputSampler", BeShaderLibrary::GetSampler("point-clamp"));
    
    const auto sakura = _machine->LoadProp("assets/sakura/scene.gltf", pbrShader);
    sakura->Materials[0]->SetSampler("InputSampler", BeShaderLibrary::GetSampler("linear-wrap"));
    
    const auto sakura2 = _machine->LoadProp("assets/stylized_sakura_tree.glb", pbrShader);
    
    const auto axe = _machine->LoadProp("assets/pixel_molten_axe/scene.gltf",phongShader,BeSRMLightingModel::Phong);
    for (const auto& material : axe->Materials) {
        material->SetSampler("InputSampler", BeShaderLibrary::GetSampler("point-clamp"));
        material->SetFloat3("EmissiveColor", glm::vec3(6.0f));
    }
    
    const auto katana = _machine->LoadProp("assets/cyberpunk_katana/scene.gltf", pbrShader);
    for (const auto& material : katana->Materials) {
        material->SetFloat3("EmissiveColor", glm::vec3(6.0f));
        material->SetFloat1("Metallic", 0.01f);
    }
    
    const auto rustySphere = _machine->LoadProp("assets/rusty-sphere/scene.gltf", pbrShader);
    const auto compass = _machine->LoadProp("assets/stylized_magic_compass/scene.gltf", pbrShader);
    const auto book = _machine->LoadProp("assets/book/scene.gltf", pbrShader);
    
    _assetRegistry.AddProp("cube", cube);
    _assetRegistry.AddProp("emissiveCube", emissiveCube);
    _assetRegistry.AddProp("moon", moon);
    _assetRegistry.AddProp("testSphere", testSphere);
    _assetRegistry.AddProp("anvil", anvil);
    _assetRegistry.AddProp("sakura", sakura);
    _assetRegistry.AddProp("sakura2", sakura2);
    _assetRegistry.AddProp("axe", axe);
    _assetRegistry.AddProp("katana", katana);
    _assetRegistry.AddProp("rusty-sphere", rustySphere);
    _assetRegistry.AddProp("compass", compass);
    _assetRegistry.AddProp("book", book);
    
    _machine->BakeMeshes();
}

auto SakuraScene::DefineSettings() -> void {
    const auto data = _sceneLua->Call("makeData");
    const auto settings = data["Settings"];

    const auto skybox = settings["skybox"];
    Settings.Skybox.Enabled = skybox["enabled"].GetOr(Settings.Skybox.Enabled);
    Settings.Skybox.HdrPath = skybox["hdrPath"].GetOr(Settings.Skybox.HdrPath);

    const auto bloom = settings["bloom"];
    Settings.Bloom.MipCount = bloom["mipCount"].GetOr(Settings.Bloom.MipCount);
    Settings.Bloom.DirtTexturePath = bloom["dirtTexturePath"].GetOr(Settings.Bloom.DirtTexturePath);

    const auto depthOfField = settings["depthOfField"];
    Settings.DepthOfField.Enabled = depthOfField["enabled"].GetOr(Settings.DepthOfField.Enabled);
    Settings.DepthOfField.MinFocalDistance = depthOfField["minFocalDistance"].GetOr(Settings.DepthOfField.MinFocalDistance);
    Settings.DepthOfField.FocusSpeed = depthOfField["focusSpeed"].GetOr(Settings.DepthOfField.FocusSpeed);

    Settings.Background.ClearColor = settings["background"]["clearColor"].GetOr(Settings.Background.ClearColor);

    ApplyBaseSettings(settings);
}

auto SakuraScene::DefineScene() -> void {
    const auto objects = _sceneLua->Call("makeData")["Objects"];
    ApplyBaseScene(objects);
}

auto SakuraScene::DefinePasses() -> void {
    _machine->ClearPasses();

    BeTexture::Create("Sakura_BloomDirtTexture")
        .LoadFromFile(Settings.Bloom.DirtTexturePath)
        .AddToRegistry(_assetRegistry)
        .Build();

    if (Settings.Skybox.Enabled) {
        const auto skyTexture = BeTexture::Create("Sakura_Sky")
            .LoadFromFileHdr(Settings.Skybox.HdrPath)
            .AddToRegistry(_assetRegistry)
            .Build();
        _machine->AddEnvironmentBakePass(skyTexture);
        _machine->BakeEnvironment();
    }

    _machine->AddShadowPass();
    _machine->AddGeometryPass();
    _machine->AddLightingPass("Sakura_HDR");
    if (Settings.Skybox.Enabled) {
        _machine->AddSkyboxPass("Sakura_HDR");
    }
    _machine->AddBloomPass(
        Settings.Bloom.MipCount,
        "Sakura_HDR",
        "Sakura_Bloom",
        _assetRegistry.GetTexture("Sakura_BloomDirtTexture").lock()
    );

    std::string tonemapperInput = "Sakura_Bloom";
    //std::string tonemapperInput = "Sakura_HDR";

    if (Settings.DepthOfField.Enabled) {
        if (!_dofMaterial) {
            const auto& dofScheme = BeShaderLibrary::GetShader("dof")->GetMaterialScheme("main");
            _dofMaterial = BeMaterial::Create(dofScheme);
        }
        _dofMaterial->SetTexture("ColorInput", _machine->GetRenderTexture("Sakura_Bloom"));
        _dofMaterial->SetTexture("DepthInput", _machine->GetRenderTexture("Sakura_Depth"));
        _machine->AddFullscreenPass(BeShaderLibrary::GetShader("dof"), _dofMaterial, { "Sakura_DoF" });
        tonemapperInput = "Sakura_DoF";
    }

    _machine->AddTonemapperPass(tonemapperInput, "Sakura_Tonemapper");

    const auto& fxaaScheme = BeShaderLibrary::GetShader("fxaa")->GetMaterialScheme("main");
    const auto fxaaMaterial = BeMaterial::Create(fxaaScheme);
    fxaaMaterial->SetTexture("ColorTexture", _machine->GetRenderTexture("Sakura_Tonemapper"));
    _machine->AddFullscreenPass(BeShaderLibrary::GetShader("fxaa"), fxaaMaterial, { "Sakura_FXAA" });

    _machine->AddBackbufferPass("Sakura_FXAA", Settings.Background.ClearColor);

    auto imguiPass = std::make_unique<BeImGuiPass>(_gameIns->Window);
    imguiPass->SetUICallback([this]() {
        ImGui::Begin("controls");
        ImGui::Text("%.0f fps (%.2f ms)", _fpsCounter.GetFps(), _fpsCounter.GetFrameMs());
        ImGui::SeparatorText("scene");
        for (size_t i = 0; i < Settings.SceneFile.Paths.size(); ++i) {
            const auto label = std::filesystem::path(Settings.SceneFile.Paths[i]).stem().string();
            ImGui::BeginDisabled(i == Settings.SceneFile.CurrentSceneIndex);
            if (ImGui::Button(label.c_str())) {
                _pendingSceneIndex = static_cast<int>(i);
            }
            ImGui::EndDisabled();
        }
        ImGui::End();
    });
    _machine->AddPass(std::move(imguiPass));

    _machine->InitialisePasses();
}

auto SakuraScene::Tick(float deltaTime) -> void {
    if (_gameIns->Input->GetKeyDown(GLFW_KEY_ESCAPE)) {
        _gameIns->Input->SetMouseCapture(false);
        _gameIns->SceneManager->RequestSceneChange("menu");
        return;
    }

    if (_pendingSceneIndex >= 0) {
        Settings.SceneFile.CurrentSceneIndex = static_cast<uint8_t>(_pendingSceneIndex);
        _pendingSceneIndex = -1;
        SetWatchFile(
            Settings.SceneFile.Paths[Settings.SceneFile.CurrentSceneIndex],
            [this] -> void { Reload(); }
        );
        Reload();
    }

    _fpsCounter.Tick(deltaTime);

    if (_gameIns->Input->GetKeyDown(GLFW_KEY_C)) {
        _cameraMode = (_cameraMode + 1) % 2;
    }

    if (Settings.DepthOfField.Enabled) {
        const float focusStep = Settings.DepthOfField.FocusSpeed * deltaTime;
        float dofFocalDistance = _dofMaterial->GetFloat("FocalDistance");

        if (_gameIns->Input->GetKey(GLFW_KEY_LEFT_BRACKET))
            dofFocalDistance = std::max(Settings.DepthOfField.MinFocalDistance, dofFocalDistance - focusStep);
        if (_gameIns->Input->GetKey(GLFW_KEY_RIGHT_BRACKET))
            dofFocalDistance += focusStep;

        _dofMaterial->SetFloat1("FocalDistance",  dofFocalDistance);
    }

    if (_cameraMode == 0) {
        _freeCameraController->Update(deltaTime, _gameIns->Input.get());
    } else if (_cameraMode == 1) {
        _gameIns->Input->SetMouseCapture(false);
        _orbitCameraController->Update(deltaTime, _gameIns->Input.get());
    }

    FullScene::Tick(deltaTime);
}