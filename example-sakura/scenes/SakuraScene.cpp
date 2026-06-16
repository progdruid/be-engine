
#include "SakuraScene.h"

#include <slang.h>
#include <umbrellas/include-glfw.h>

#include "BeRenderPass.h"
#include "OrbitCameraController.h"
#include "FreeCameraController.h"
#include "RigCameraController.h"
#include "RailGizmo.h"
#include "BeAssetRegistry.h"
#include "LuaSceneLoader.h"
#include "BeCamera.h"
#include "BeInput.h"
#include "BeMaterial.h"
#include "BeMeshPrimitives.h"
#include "BeProp.h"
#include "BeShader.h"
#include "BeTexture.h"
#include "BeWindow.h"
#include "BeRenderer.h"
#include "Components.h"
#include "Game.h"
#include "scenes/BeSceneManager.h"
#include "standard-render-machine/BeStandardRenderMachine.h"

SakuraScene::SakuraScene(Game* game) : BaseScene(game) {}
SakuraScene::~SakuraScene() = default;

auto SakuraScene::Prepare() -> void {
    BeAssetRegistry::IndexShaderFiles({
        "assets/shaders/uniform-material.hlsl",
        "assets/shaders/objectMaterial.hlsl",
        "assets/shaders/standard-pbr.hlsl",
        "assets/shaders/standard-phong.hlsl",
        "assets/shaders/checkerboard.hlsl",
        "assets/shaders/fullscreen-vertex.hlsl",
        "assets/shaders/directionalLight.hlsl",
        "assets/shaders/pointLight.hlsl",
        "assets/shaders/emissive-add.hlsl",
        "assets/shaders/bloom-add.hlsl",
        "assets/shaders/bloom-bright.hlsl",
        "assets/shaders/bloom-downsample.hlsl",
        "assets/shaders/bloom-upsample.hlsl",
        "assets/shaders/tonemapper.hlsl",
        "assets/shaders/backbuffer.hlsl",
        "assets/shaders/fxaa.hlsl",
        "assets/shaders/dof.hlsl",
    });

    const auto standardShader    = BeAssetRegistry::GetShader("standard-pbr");
    const auto phongShader       = BeAssetRegistry::GetShader("standard-phong");
    const auto checkerboardShader = BeAssetRegistry::GetShader("checkerboard");

    const uint32_t screenWidth  = GameIns->Renderer->GetSwapchainPixelWidth();
    const uint32_t screenHeight = GameIns->Renderer->GetSwapchainPixelHeight();

    _machine = std::make_unique<BeStandardRenderMachine>(GameIns->Renderer, screenWidth, screenHeight);

    _cube = BeProp::FromMesh(BeMeshPrimitives::Cube(), checkerboardShader, "geometry-main");
    _cube->Materials[0]->SetTexture("DiffuseTexture",
        BeTexture::Create("Sakura_Checkerboard")
        .LoadFromFile("assets/checkerboard.png")
        .AddToRegistry()
        .Build()
    );

    _emissiveCube = BeProp::FromMesh(BeMeshPrimitives::Cube(), standardShader, "geometry-main");
    _emissiveCube->Materials[0]->SetFloat3("EmissiveColor", glm::vec3(0.99f, 0.8f, 0.6f) * 1.7f);

    _moon = BeProp::FromMesh(BeMeshPrimitives::Cube(), standardShader, "geometry-main");
    _moon->Materials[0]->SetFloat3("EmissiveColor", glm::vec3(0.7f, 0.7f, 0.99f) * 2.1f);

    _anvil = _machine->LoadProp("assets/anvil/scene.gltf", standardShader);
    _anvil->Materials[0]->SetSampler("InputSampler", BeAssetRegistry::GetSampler("point-clamp"));

    _sakura = _machine->LoadProp("assets/sakura/scene.gltf", standardShader);
    _sakura->Materials[0]->SetSampler("InputSampler", BeAssetRegistry::GetSampler("linear-wrap"));

    _sakura2 = _machine->LoadProp("assets/stylized_sakura_tree.glb", standardShader);

    _testSphere = BeProp::FromMesh(BeMeshPrimitives::Sphere(), standardShader, "geometry-main");
    _testSphere->Materials[0]->SetFloat3("BaseColor", glm::vec3(0.8f, 0.3f, 0.1f));
    _testSphere->Materials[0]->SetFloat("Metallic", 0.8f);
    _testSphere->Materials[0]->SetFloat("Roughness", 0.5f);

    _axe = _machine->LoadProp("assets/pixel_molten_axe/scene.gltf",phongShader,BeSRMLightingModel::Phong);
    for (const auto& material : _axe->Materials) {
        material->SetSampler("InputSampler", BeAssetRegistry::GetSampler("point-clamp"));
        material->SetFloat3("EmissiveColor", glm::vec3(1.5f));
    }

    _katana = _machine->LoadProp("assets/cyberpunk_katana/scene.gltf", standardShader);
    for (const auto& material : _katana->Materials) {
        material->SetFloat3("EmissiveColor", glm::vec3(2.5f));
        material->SetFloat("Metallic", 0.01f);
    }
    
    _rustySphere = _machine->LoadProp("assets/rusty-sphere/scene.gltf", standardShader);
    // for (const auto& material : _rustySphere->Materials) {
    //     material->SetFloat("Metallic", 0.9f);
    // }
    
    BeAssetRegistry::AddProp("cube", _cube);
    BeAssetRegistry::AddProp("emissiveCube", _emissiveCube);
    BeAssetRegistry::AddProp("moon", _moon);
    BeAssetRegistry::AddProp("anvil", _anvil);
    BeAssetRegistry::AddProp("sakura", _sakura);
    BeAssetRegistry::AddProp("sakura2", _sakura2);
    BeAssetRegistry::AddProp("testSphere", _testSphere);
    BeAssetRegistry::AddProp("axe", _axe);
    BeAssetRegistry::AddProp("katana", _katana);
    BeAssetRegistry::AddProp("rusty-sphere", _rustySphere);

    _uniformMaterial = BeMaterial::Create("uniform-material", false);
    _uniformMaterial->SetFloat3("AmbientColor", glm::vec3(0.1f));

    _machine->RegisterMesh(_cube->Mesh);
    _machine->RegisterMesh(_emissiveCube->Mesh);
    _machine->RegisterMesh(_moon->Mesh);
    _machine->RegisterMesh(_testSphere->Mesh);
    _machine->BakeMeshes();

    _machine->DeclareGBufferTarget("Sakura_Albedo_RGB",         SenFormat::R11G11B10_Float);
    _machine->DeclareGBufferTarget("Sakura_WorldNormal_XYZ",    SenFormat::RGBA16_Float);
    _machine->DeclareGBufferTarget("Sakura_ORM_RGB",            SenFormat::RGBA8_Unorm);
    _machine->DeclareGBufferTarget("Sakura_Emissive_RGB",       SenFormat::R11G11B10_Float);
    _machine->DeclareDepth        ("Sakura_Depth",              SenFormat::Depth32);
    _machine->DeclareTexture      ("Sakura_HDR",                SenFormat::R11G11B10_Float);
    _machine->DeclareTexture      ("Sakura_Bloom",              SenFormat::R11G11B10_Float);
    _machine->DeclareTexture      ("Sakura_DoF",                SenFormat::R11G11B10_Float);
    _machine->DeclareTexture      ("Sakura_Tonemapper",         SenFormat::R11G11B10_Float);
    _machine->DeclareTexture      ("Sakura_FXAA",               SenFormat::R11G11B10_Float);

    const auto dirtTexture = BeTexture::Create("Sakura_BloomDirtTexture")
        .LoadFromFile("assets/bloom-dirt-mask.png")
        .AddToRegistry()
        .Build();

    _sceneLoader = std::make_unique<LuaSceneLoader>();
    RegisterComponentParsers(*_sceneLoader);

    _camera = std::make_unique<BeCamera>();
    _camera->Width = GameIns->Renderer->GetSwapchainPixelWidth();
    _camera->Height = GameIns->Renderer->GetSwapchainPixelHeight();
    _camera->NearPlane = 0.1f;
    _camera->FarPlane = 250.0f;
    _orbitCameraController = std::make_unique<OrbitCameraController>(_camera.get());
    _freeCameraController = std::make_unique<FreeCameraController>(_camera.get());

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

auto SakuraScene::OnLoad() -> void {
    RebuildPasses();

    constexpr auto scenePath = "assets/sakura_scene.lua";
    _registry.clear();
    _sceneLoader->Load(scenePath, _registry);
    _sceneLastWriteTime = std::filesystem::last_write_time(scenePath);
}

auto SakuraScene::RebuildPasses() -> void {
    _machine->UniformMaterial = _uniformMaterial;
    _machine->ClearPasses();
    _machine->AddShadowPass();
    _machine->AddGeometryPass();
    _machine->AddLightingPass("Sakura_HDR");
    _machine->AddBloomPass(5, "Sakura_HDR", "Sakura_Bloom", BeAssetRegistry::GetTexture("Sakura_BloomDirtTexture").lock());

    std::string tonemapperInput = "Sakura_Bloom";

    if (_dofEnabled) {
        if (!_dofMaterial) {
            _dofMaterial = BeMaterial::Create("dof-material", false);
        }
        _dofMaterial->SetTexture("ColorInput", _machine->GetRenderTexture("Sakura_Bloom"));
        _dofMaterial->SetTexture("DepthInput", _machine->GetRenderTexture("Sakura_Depth"));
        _machine->AddFullscreenPass(BeAssetRegistry::GetShader("dof"), _dofMaterial, { "Sakura_DoF" });
        tonemapperInput = "Sakura_DoF";
    }

    const auto tonemapperMaterial = BeMaterial::Create("tonemapper-material", false);
    tonemapperMaterial->SetTexture("HDRInput", _machine->GetRenderTexture(tonemapperInput));
    _machine->AddFullscreenPass(BeAssetRegistry::GetShader("tonemapper"), tonemapperMaterial, { "Sakura_Tonemapper" });

    const auto fxaaMaterial = BeMaterial::Create("fxaa-material", false);
    fxaaMaterial->SetTexture("ColorTexture", _machine->GetRenderTexture("Sakura_Tonemapper"));
    _machine->AddFullscreenPass(BeAssetRegistry::GetShader("fxaa"), fxaaMaterial, { "Sakura_FXAA" });

    _machine->AddBackbufferPass("Sakura_FXAA", { 0.f / 255.f, 23.f / 255.f, 31.f / 255.f });
    _machine->BuildPasses();
}

auto SakuraScene::Tick(float deltaTime) -> void {
    if (GameIns->Input->GetKeyDown(GLFW_KEY_ESCAPE)) {
        GameIns->Input->SetMouseCapture(false);
        GameIns->SceneManager->RequestSceneChange("menu");
    }

    constexpr auto scenePath = "assets/sakura_scene.lua";
    auto writeTime = std::filesystem::last_write_time(scenePath);
    if (writeTime > _sceneLastWriteTime) {
        _registry.clear();
        _sceneLoader->Load(scenePath, _registry);
        _sceneLastWriteTime = writeTime;
    }

    static const auto GeometryView = _registry.view<NameComponent, TransformComponent, RenderComponent>();
    static const auto SunView = _registry.view<SunLightComponent>();
    static const auto PointLightView = _registry.view<NameComponent, TransformComponent, PointLightComponent>();
    static const auto OrbitingLightView = _registry.view<NameComponent, TransformComponent, PointLightComponent>(entt::exclude<StaticTag>);

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
        _dofEnabled = !_dofEnabled;
        RebuildPasses();
    }

    if (_dofEnabled) {
        float dofFocalDistance = _dofMaterial->GetFloat("FocalDistance");
        
        if (GameIns->Input->GetKey(GLFW_KEY_LEFT_BRACKET))
            dofFocalDistance = std::max(0.5f, dofFocalDistance - 5.0f * deltaTime);
        if (GameIns->Input->GetKey(GLFW_KEY_RIGHT_BRACKET))
            dofFocalDistance += 5.0f * deltaTime;

        _dofMaterial->SetFloat("FocalDistance",  dofFocalDistance);
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

    auto& uniformMat = *_uniformMaterial;
    const auto projView = _camera->GetProjectionMatrix() * _camera->GetViewMatrix();
    uniformMat.SetMatrix("CameraProjectionView", projView);
    uniformMat.SetMatrix("CameraInverseProjectionView", glm::inverse(projView));
    uniformMat.SetFloat4("NearFarPlane", { _camera->NearPlane, _camera->FarPlane, 1.0f / _camera->NearPlane, 1.0f / _camera->FarPlane });
    uniformMat.SetFloat3("CameraPosition", _camera->Position);

    {
        static float angle = 0.0f;
        angle += deltaTime * glm::radians(15.0f);
        if (angle > glm::two_pi<float>()) {
            angle -= glm::two_pi<float>();
        }

        auto i = size_t(0);
        const auto orbitNumber = float(std::ranges::distance(OrbitingLightView));
        for (const auto [entity, name, transform, _] : OrbitingLightView.each()) {
            constexpr float radius = 13.0f;
            const auto add = glm::two_pi<float>() * (float(i) / orbitNumber);
            const auto rad = radius * (0.7f + 0.3f * ((i + 1) % 2));
            transform.Position = glm::vec3(cos(angle + add) * rad, 4.0f + 4.0f * (i % 2), sin(angle + add) * rad);
            i++;
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

    //RailGizmo::DrawRail(*_machine, _rigCameraController->GetPathRail(), _testSphere, _cube);

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
