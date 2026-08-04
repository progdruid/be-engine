
#include "SakuraScene.h"

#include <umbrellas/include-glfw.h>

#include <imgui/imgui.h>

#include "BeRenderPass.h"
#include "imgui/BeImGuiPass.h"
#include "OrbitCameraController.h"
#include "FreeCameraController.h"
#include "RigCameraController.h"
#include "RailGizmo.h"
#include "BeAssetRegistry.h"
#include "BeShaderLibrary.h"
#include "lua/BeLua.h"
#include "BeCamera.h"
#include "BeInput.h"
#include "BeMaterial.h"
#include "BeMeshPrimitives.h"
#include "BeProp.h"
#include "BeTexture.h"
#include "BeRenderer.h"
#include "BeShader.h"
#include "Components.h"
#include "Game.h"
#include "scenes/BeSceneManager.h"
#include "standard-render-machine/BeStandardRenderMachine.h"

SakuraScene::SakuraScene(Game* game) : BaseScene(game) {}
SakuraScene::~SakuraScene() = default;

auto SakuraScene::Prepare() -> void {
    const uint32_t screenWidth  = GameIns->Renderer->GetSwapchainPixelWidth();
    const uint32_t screenHeight = GameIns->Renderer->GetSwapchainPixelHeight();

    _machine = std::make_unique<BeStandardRenderMachine>(GameIns->Renderer, _assetRegistry, screenWidth, screenHeight);

    LoadProps();

    // srm textures
    _machine->DeclareGBufferTarget("Sakura_Albedo_RGB",      SenFormat::R11G11B10_Float);
    _machine->DeclareGBufferTarget("Sakura_WorldNormal_XYZ", SenFormat::RGBA16_Float);
    _machine->DeclareGBufferTarget("Sakura_ORM_RGB",         SenFormat::RGBA8_Unorm);
    _machine->DeclareGBufferTarget("Sakura_Emissive_RGB",    SenFormat::R11G11B10_Float);
    _machine->DeclareDepth        ("Sakura_Depth",           SenFormat::Depth32);
    _machine->DeclareTexture      ("Sakura_HDR",             SenFormat::R11G11B10_Float);
    _machine->DeclareTexture      ("Sakura_Bloom",           SenFormat::R11G11B10_Float);
    _machine->DeclareTexture      ("Sakura_DoF",             SenFormat::R11G11B10_Float);
    _machine->DeclareTexture      ("Sakura_Tonemapper",      SenFormat::R11G11B10_Float);
    _machine->DeclareTexture      ("Sakura_FXAA",            SenFormat::R11G11B10_Float);

    // camera
    _camera = std::make_unique<BeCamera>();
    _camera->Width = GameIns->Renderer->GetSwapchainPixelWidth();
    _camera->Height = GameIns->Renderer->GetSwapchainPixelHeight();
    _orbitCameraController = std::make_unique<OrbitCameraController>(_camera.get());
    _freeCameraController = std::make_unique<FreeCameraController>(_camera.get());

    LoadSceneFile();
    const auto currentPath = Settings.SceneFile.Paths[Settings.SceneFile.CurrentSceneIndex];
    _sceneLastWriteTime = std::filesystem::last_write_time(currentPath);

    RebuildPasses();


    // rig
    _rigCameraController = std::make_unique<RigCameraController>(_camera.get());
    BeCameraShot& flythrough = _rigCameraController->AddShot("flythrough");
    flythrough.SetPathRail(BeRail()
        .Knot({ 12.0f, 2.0f,  0.0f})
        .Knot({  6.0f, 1.3f,  7.0f})
        .Knot({  1.5f, 2.6f,  5.5f})
        .Knot({ -6.0f, 4.0f,  6.0f})
        .Knot({-13.0f, 9.0f, -2.0f})
        .Knot({ -5.0f,12.0f,-10.0f})
        .Knot({  4.0f, 6.0f,-11.0f})
        .Knot({ 10.0f, 2.5f, -5.0f})
        .Close()
        .Finalize());

    const BeRail& path = flythrough.GetPathRail();
    flythrough.PathWarp()
        .Key( 0.0f, path.IndexToDistance(0.0f), BeTrackInterp::Linear)
        .Key(22.0f, path.IndexToDistance(8.0f), BeTrackInterp::Linear);

    flythrough.AimAt()
        .Key( 0.0f, {  0.0f,  1.0f,   0.0f}, BeTrackInterp::Linear)
        .Key( 12.0f, {  0.0f,  1.0f,   0.0f}, BeTrackInterp::EaseInOut)
        .Key(18.0f, {100.0f,150.0f, 100.0f}, BeTrackInterp::EaseInOut)
        .Key(22.0f, {  0.0f,  1.0f,   0.0f}, BeTrackInterp::EaseInOut);
}  
 

auto SakuraScene::LoadProps() -> void {
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
    _assetRegistry.AddProp("cube", cube);
    _machine->RegisterMesh(cube->Mesh);


    const auto emissiveCube = BeProp::FromMesh(BeMeshPrimitives::Cube(), pbrShader, "geometry-main");
    emissiveCube->Materials[0]->SetFloat3("EmissiveColor", glm::vec3(1.f) * 4.0f);
    _assetRegistry.AddProp("emissiveCube", emissiveCube);
    _machine->RegisterMesh(emissiveCube->Mesh);

    
    const auto moon = BeProp::FromMesh(BeMeshPrimitives::Cube(), pbrShader, "geometry-main");
    moon->Materials[0]->SetFloat3("EmissiveColor", glm::vec3(0.7f, 0.7f, 0.99f) * 10.0f);
    _assetRegistry.AddProp("moon", moon);
    _machine->RegisterMesh(moon->Mesh);
    
    
    const auto testSphere = BeProp::FromMesh(BeMeshPrimitives::Sphere(), pbrShader, "geometry-main");
    testSphere->Materials[0]->SetFloat3("BaseColor", glm::vec3(0.8f, 0.3f, 0.1f));
    testSphere->Materials[0]->SetFloat1("Metallic", 0.8f);
    testSphere->Materials[0]->SetFloat1("Roughness", 0.5f);
    _assetRegistry.AddProp("testSphere", testSphere);
    _machine->RegisterMesh(testSphere->Mesh);

    
    const auto anvil = _machine->LoadProp("assets/anvil/scene.gltf", pbrShader);
    anvil->Materials[0]->SetSampler("InputSampler", BeShaderLibrary::GetSampler("point-clamp"));
    _assetRegistry.AddProp("anvil", anvil);

    
    const auto sakura = _machine->LoadProp("assets/sakura/scene.gltf", pbrShader);
    sakura->Materials[0]->SetSampler("InputSampler", BeShaderLibrary::GetSampler("linear-wrap"));
    _assetRegistry.AddProp("sakura", sakura);

    
    const auto sakura2 = _machine->LoadProp("assets/stylized_sakura_tree.glb", pbrShader);
    _assetRegistry.AddProp("sakura2", sakura2);
    
    
    const auto axe = _machine->LoadProp("assets/pixel_molten_axe/scene.gltf",phongShader,BeSRMLightingModel::Phong);
    for (const auto& material : axe->Materials) {
        material->SetSampler("InputSampler", BeShaderLibrary::GetSampler("point-clamp"));
        material->SetFloat3("EmissiveColor", glm::vec3(6.0f));
    }
    _assetRegistry.AddProp("axe", axe);

    
    const auto katana = _machine->LoadProp("assets/cyberpunk_katana/scene.gltf", pbrShader);
    for (const auto& material : katana->Materials) {
        material->SetFloat3("EmissiveColor", glm::vec3(6.0f));
        material->SetFloat1("Metallic", 0.01f);
    }
    _assetRegistry.AddProp("katana", katana);
    
    
    const auto rustySphere = _machine->LoadProp("assets/rusty-sphere/scene.gltf", pbrShader);
    // for (const auto& material : rustySphere->Materials) {
    //     material->SetFloat("Metallic", 0.9f);
    // }
    _assetRegistry.AddProp("rusty-sphere", rustySphere);
    
    
    const auto compass = _machine->LoadProp("assets/stylized_magic_compass/scene.gltf", pbrShader);
    _assetRegistry.AddProp("compass", compass);
    
    const auto book = _machine->LoadProp("assets/book/scene.gltf", pbrShader);
    _assetRegistry.AddProp("book", book);
    
    
    _machine->BakeMeshes();
}


auto SakuraScene::OnLoad() -> void {
    _machine->Activate();
}

auto SakuraScene::LoadSceneFile() -> void {
    const auto currentPath = Settings.SceneFile.Paths[Settings.SceneFile.CurrentSceneIndex];
    
    BeLuaState lua;
    if (!lua.DoFile(currentPath)) {
        return;
    }

    const auto data = lua.Call("makeData");

    const auto settings = data["Settings"];
    const auto srm = settings["srm"];
    Settings.SRM.Shadow.Bias = srm["shadow"]["bias"].GetOr(Settings.SRM.Shadow.Bias);
    Settings.SRM.IBL.MaxSampleRadiance = srm["ibl"]["maxSampleRadiance"].GetOr(Settings.SRM.IBL.MaxSampleRadiance);
    Settings.SRM.Skybox.ClampRadiance = srm["skybox"]["clampRadiance"].GetOr(Settings.SRM.Skybox.ClampRadiance);

    const auto srmBloom = srm["bloom"];
    Settings.SRM.Bloom.Threshold = srmBloom["threshold"].GetOr(Settings.SRM.Bloom.Threshold);
    Settings.SRM.Bloom.Knee = srmBloom["knee"].GetOr(Settings.SRM.Bloom.Knee);
    Settings.SRM.Bloom.Intensity = srmBloom["intensity"].GetOr(Settings.SRM.Bloom.Intensity);
    Settings.SRM.Bloom.Clamp = srmBloom["clamp"].GetOr(Settings.SRM.Bloom.Clamp);
    Settings.SRM.Bloom.UpsampleRadius = srmBloom["upsampleRadius"].GetOr(Settings.SRM.Bloom.UpsampleRadius);

    const auto tonemapper = srm["tonemapper"];
    Settings.SRM.Tonemapper.Exposure = tonemapper["exposure"].GetOr(Settings.SRM.Tonemapper.Exposure);
    Settings.SRM.Tonemapper.Contrast = tonemapper["contrast"].GetOr(Settings.SRM.Tonemapper.Contrast);

    const auto camera = settings["camera"];
    Settings.Camera.NearPlane = camera["nearPlane"].GetOr(Settings.Camera.NearPlane);
    Settings.Camera.FarPlane = camera["farPlane"].GetOr(Settings.Camera.FarPlane);

    Settings.Ambient.Color = settings["ambient"]["color"].GetOr(Settings.Ambient.Color);

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
    
    be_assert(_camera, "LoadSceneFile: camera must be created before the scene file is loaded");
    _machine->Settings = Settings.SRM;
    _camera->NearPlane = Settings.Camera.NearPlane;
    _camera->FarPlane = Settings.Camera.FarPlane;
    _machine->UniformMaterial->SetFloat3("AmbientColor", Settings.Ambient.Color);

    _registry.clear();

    for (const auto& [name, entityTable] : data["Objects"].Pairs()) {
        const auto entity = _registry.create();
        _registry.emplace<NameComponent>(entity, NameComponent{ .Name = name });

        if (const auto table = entityTable["transform"]; table.Exists()) {
            TransformComponent comp;
            comp.Position = table["position"].GetOr(comp.Position);
            comp.Rotation = table["rotation"].GetOr(comp.Rotation);
            comp.Scale = table["scale"].GetOr(comp.Scale);
            _registry.emplace<TransformComponent>(entity, comp);
        }

        if (const auto table = entityTable["render"]; table.Exists()) {
            const auto propName = table["prop"].Get<std::string>();
            if (propName) {
                if (const auto prop = _assetRegistry.GetProp(*propName).lock()) {
                    _registry.emplace<RenderComponent>(entity, RenderComponent{
                        .Prop = prop,
                        .CastShadows = table["castShadows"].GetOr(true),
                    });
                }
            }
        }

        if (const auto table = entityTable["circling"]; table.Exists()) {
            CirclingComponent comp;
            comp.Origin = table["origin"].GetOr(comp.Origin);
            comp.Axis = glm::normalize(table["axis"].GetOr(comp.Axis));
            comp.Radius = table["radius"].GetOr(comp.Radius);
            comp.Speed = table["speed"].GetOr(comp.Speed);
            comp.Phase = table["phase"].GetOr(comp.Phase);
            comp.Rotate = table["rotate"].GetOr(comp.Rotate);
            _registry.emplace<CirclingComponent>(entity, comp);
        }

        if (entityTable["static"].Exists()) {
            _registry.emplace<StaticTag>(entity);
        }
        
        if (const auto table = entityTable["sunLight"]; table.Exists()) {
            SunLightComponent comp;
            comp.Direction = glm::normalize(table["direction"].GetOr(comp.Direction));
            comp.Color = table["color"].GetOr(comp.Color);
            comp.Power = table["power"].GetOr(comp.Power);
            comp.CastsShadows = table["castsShadows"].GetOr(comp.CastsShadows);
            comp.ShadowMapResolution = table["shadowMapResolution"].GetOr(comp.ShadowMapResolution);
            comp.ShadowCameraDistance = table["shadowCameraDistance"].GetOr(comp.ShadowCameraDistance);
            comp.ShadowMapWorldSize = table["shadowMapWorldSize"].GetOr(comp.ShadowMapWorldSize);
            comp.ShadowNearPlane = table["shadowNearPlane"].GetOr(comp.ShadowNearPlane);
            comp.ShadowFarPlane = table["shadowFarPlane"].GetOr(comp.ShadowFarPlane);

            if (comp.CastsShadows) {
                comp.ShadowMap = BeTexture::Create(name + "_ShadowMap")
                    .SetUsage(SenTextureUsage::DepthStencil | SenTextureUsage::ShaderResource)
                    .SetFormat(SenFormat::Depth32)
                    .SetSize(comp.ShadowMapResolution, comp.ShadowMapResolution)
                    .Build();
            }
            _registry.emplace<SunLightComponent>(entity, comp);
        }

        if (const auto table = entityTable["pointLight"]; table.Exists()) {
            PointLightComponent comp;
            comp.Radius = table["radius"].GetOr(comp.Radius);
            comp.Color = table["color"].GetOr(comp.Color);
            comp.Power = table["power"].GetOr(comp.Power);
            comp.CastsShadows = table["castsShadows"].GetOr(comp.CastsShadows);
            comp.ShadowMapResolution = table["shadowMapResolution"].GetOr(comp.ShadowMapResolution);
            comp.ShadowNearPlane = table["shadowNearPlane"].GetOr(comp.ShadowNearPlane);

            if (comp.CastsShadows) {
                comp.ShadowMap = BeTexture::Create(name + "_ShadowMap")
                    .SetUsage(SenTextureUsage::DepthStencil | SenTextureUsage::ShaderResource)
                    .SetFormat(SenFormat::Depth32)
                    .SetCubemap(true)
                    .SetSize(comp.ShadowMapResolution, comp.ShadowMapResolution)
                    .Build();
            }
            _registry.emplace<PointLightComponent>(entity, comp);
        }
    } // end of for loop
}

auto SakuraScene::RebuildPasses() -> void {
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

    auto imguiPass = std::make_unique<BeImGuiPass>(GameIns->Window);
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

    _machine->BuildPasses();
}

auto SakuraScene::Tick(float deltaTime) -> void {
    if (GameIns->Input->GetKeyDown(GLFW_KEY_ESCAPE)) {
        GameIns->Input->SetMouseCapture(false);
        GameIns->SceneManager->RequestSceneChange("menu");
        return;
    }

    if (_pendingSceneIndex >= 0) {
        Settings.SceneFile.CurrentSceneIndex = static_cast<uint8_t>(_pendingSceneIndex);
        _pendingSceneIndex = -1;
        LoadSceneFile();
        RebuildPasses();
        _machine->Activate();
        _sceneLastWriteTime = std::filesystem::last_write_time(Settings.SceneFile.Paths[Settings.SceneFile.CurrentSceneIndex]);
    }

    const auto currentPath = Settings.SceneFile.Paths[Settings.SceneFile.CurrentSceneIndex];
    const auto writeTime = std::filesystem::last_write_time(currentPath);
    if (writeTime > _sceneLastWriteTime) {
        LoadSceneFile();
        RebuildPasses();
        _machine->Activate();
        _sceneLastWriteTime = writeTime;
    }

    static const auto GeometryView = _registry.view<NameComponent, TransformComponent, RenderComponent>();
    static const auto SunView = _registry.view<SunLightComponent>();
    static const auto PointLightView = _registry.view<NameComponent, TransformComponent, PointLightComponent>();
    static const auto CirclingView = _registry.view<TransformComponent, CirclingComponent>();

    _time += deltaTime;
    _fpsCounter.Tick(deltaTime);

    if (GameIns->Input->GetKeyDown(GLFW_KEY_C)) {
        _cameraMode = (_cameraMode + 1) % 3;
        if (_cameraMode == 2) _rigCameraController->Play("flythrough", /*loop*/ true);
    }

    if (GameIns->Input->GetKeyDown(GLFW_KEY_HOME)) _machine->SetDebugChannel(-1);  // normal output
    if (GameIns->Input->GetKeyDown(GLFW_KEY_F1))   _machine->SetDebugChannel(0);   // albedo
    if (GameIns->Input->GetKeyDown(GLFW_KEY_F2))   _machine->SetDebugChannel(1);   // world normal
    if (GameIns->Input->GetKeyDown(GLFW_KEY_F3))   _machine->SetDebugChannel(2);   // ORM
    if (GameIns->Input->GetKeyDown(GLFW_KEY_F4))   _machine->SetDebugChannel(3);   // emissive

    if (GameIns->Input->GetKeyDown(GLFW_KEY_F5)) {
        Settings.DepthOfField.Enabled = !Settings.DepthOfField.Enabled;
        RebuildPasses();
        _machine->Activate();
    }

    if (Settings.DepthOfField.Enabled) {
        const float focusStep = Settings.DepthOfField.FocusSpeed * deltaTime;
        float dofFocalDistance = _dofMaterial->GetFloat("FocalDistance");

        if (GameIns->Input->GetKey(GLFW_KEY_LEFT_BRACKET))
            dofFocalDistance = std::max(Settings.DepthOfField.MinFocalDistance, dofFocalDistance - focusStep);
        if (GameIns->Input->GetKey(GLFW_KEY_RIGHT_BRACKET))
            dofFocalDistance += focusStep;

        _dofMaterial->SetFloat1("FocalDistance",  dofFocalDistance);
    }

    if (_cameraMode == 1) {
        GameIns->Input->SetMouseCapture(false);
        _orbitCameraController->Update(deltaTime, GameIns->Input.get());
    } else if (_cameraMode == 2) {
        GameIns->Input->SetMouseCapture(false);
        _rigCameraController->Update(deltaTime);
    } else {
        _freeCameraController->Update(deltaTime, GameIns->Input.get());
    }

    auto& uniformMat = *_machine->UniformMaterial;
    const auto projView = _camera->GetProjectionMatrix() * _camera->GetViewMatrix();
    uniformMat.SetMatrix("CameraProjectionView", projView);
    uniformMat.SetMatrix("CameraInverseProjectionView", glm::inverse(projView));
    uniformMat.SetFloat4("NearFarPlane", { _camera->NearPlane, _camera->FarPlane, 1.0f / _camera->NearPlane, 1.0f / _camera->FarPlane });
    uniformMat.SetFloat3("CameraPosition", _camera->Position);

    for (const auto [_, transform, circling] : CirclingView.each()) {
        const auto reference = std::abs(circling.Axis.y) > 0.999f ? glm::vec3(0.f, 0.f, 1.f) : glm::vec3(0.f, 1.f, 0.f);
        const auto start = glm::normalize(glm::cross(circling.Axis, reference)) * circling.Radius;
        const auto angle = glm::radians(circling.Phase + circling.Speed * _time);
        const auto rotation = glm::angleAxis(angle, circling.Axis);

        transform.Position = circling.Origin + rotation * start;
        if (circling.Rotate) {
            transform.Rotation = rotation;
        }
    }

    _machine->ClearFrame();

    for (const auto [entity, name, transform, render] : GeometryView.each()) {
        _machine->AddGeometry({
            .Name = name.Name,
            .ModelMatrix = BeSRMGeometryEntry::CalculateModelMatrix(transform.Position, transform.Rotation, transform.Scale),
            .Prop = render.Prop,
            .CastShadows = render.CastShadows,
        });
    }

    // if (const auto shot = _rigCameraController->GetCurrentShot()) {
    //     RailGizmo::DrawRail(*_machine, shot->GetPathRail(), _testSphere, _cube);
    // }

    for (const auto [entity, sunLight] : SunView.each()) {
        _machine->AddSunLight({
            .Direction = sunLight.Direction,
            .Color = sunLight.Color,
            .Power = sunLight.Power,
            .CastsShadows = sunLight.CastsShadows,
            .ShadowViewProjection = BeSRMSunLightEntry::CalculateViewProj(
                sunLight.Direction,
                sunLight.ShadowCameraDistance,
                sunLight.ShadowMapWorldSize,
                sunLight.ShadowNearPlane,
                sunLight.ShadowFarPlane
            ),
            .ShadowMapResolution = sunLight.ShadowMapResolution,
            .ShadowMap = sunLight.ShadowMap,
        });
    }

    for (const auto [entity, name, transform, pointLight] : PointLightView.each()) {
        _machine->AddPointLight({
            .Name = name.Name,
            .Position = transform.Position,
            .Radius = pointLight.Radius,
            .Color = pointLight.Color,
            .Power = pointLight.Power,
            .CastsShadows = pointLight.CastsShadows,
            .ShadowMapResolution = pointLight.ShadowMapResolution,
            .ShadowNearPlane = pointLight.ShadowNearPlane,
            .ShadowMap = pointLight.ShadowMap,
        });
    }
}