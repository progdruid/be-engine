
#include "OldScene.h"

#include <umbrellas/include-glfw.h>

#include "BeAssetRegistry.h"
#include "BeCamera.h"
#include "BeInput.h"
#include "BeMaterial.h"
#include "BeMeshPrimitives.h"
#include "BeProp.h"
#include "BeRenderer.h"
#include "BeShader.h"
#include "BeShaderLibrary.h"
#include "BeTexture.h"
#include "BeWindow.h"
#include "Game.h"
#include "scenes/BeSceneManager.h"
#include "standard-render-machine/BeStandardRenderMachine.h"

OldScene::OldScene(Game* game) : FullScene(game) {}
OldScene::~OldScene() = default;

auto OldScene::DefinePasses() -> void {
    _machine->ClearPasses();

    _machine->AddShadowPass();
    _machine->AddGeometryPass();
    _machine->AddLightingPass("Old_HDR");
    _machine->AddBloomPass(5, "Old_HDR", "Old_BloomOutput", _assetRegistry.GetTexture("Old_BloomDirt").lock());

    _machine->AddTonemapperPass("Old_BloomOutput", "Old_TonemapperOutput");

    _machine->AddBackbufferPass("Old_TonemapperOutput", { 0.f / 255.f, 23.f / 255.f, 31.f / 255.f });
    _machine->InitialisePasses();
}

auto OldScene::DefineSettings() -> void {
    _camera->NearPlane = 0.1f;
    _camera->FarPlane = 200.0f;
    _machine->UniformMaterial->SetFloat3("AmbientColor", glm::vec3(0.0f));
}

auto OldScene::DefineAssets() -> void {
    const auto standardShader    = BeShaderLibrary::GetShader("standard-phong");
    const auto tessellatedShader = BeShaderLibrary::GetShader("tessellated");
    const auto terrainShader     = BeShaderLibrary::GetShader("terrain");

    {
        auto planeMesh = BeMeshPrimitives::Plane(63);
        _plane = BeProp::FromMesh(std::move(planeMesh), terrainShader, "geometry-main");
        _plane->Slices[0].Material->SetFloat1("TerrainScale", 200.0f);
        _plane->Slices[0].Material->SetFloat1("HeightScale", 100.0f);
    }
    _cube = BeProp::FromMesh(BeMeshPrimitives::Cube(), tessellatedShader, "geometry-main");
    _cube->Slices[0].Material->SetFloat3("DiffuseColor", glm::vec3(0.28f, 0.39f, 1.0f));

    _witchItems = _machine->LoadProp("assets/witch_items.glb",          standardShader, BeSRMLightingModel::Phong);
    _macintosh  = _machine->LoadProp("assets/model.fbx",                standardShader, BeSRMLightingModel::Phong);
    _pagoda     = _machine->LoadProp("assets/pagoda.glb",               standardShader, BeSRMLightingModel::Phong);
    _disks      = _machine->LoadProp("assets/floppy-disks.glb",         standardShader, BeSRMLightingModel::Phong);
    _anvil      = _machine->LoadProp("assets/anvil-lowpoly/anvil.fbx",  standardShader, BeSRMLightingModel::Phong);
    _anvil->Materials[0]->SetFloat3("SpecularColor", glm::vec3(1.0f));

    for (const auto& prop : { _anvil, _pagoda, _witchItems }) {
        for (const auto& material : prop->Materials) {
            material->SetSampler("InputSampler", BeShaderLibrary::GetSampler("point-clamp"));
        }
    }

    _machine->RegisterMesh(_plane->Mesh);
    _machine->RegisterMesh(_cube->Mesh);
    _machine->BakeMeshes();

    _machine->DeclareGBufferTarget("Old_BaseColor",         SenFormat::R11G11B10_Float);
    _machine->DeclareGBufferTarget("Old_WorldNormal",       SenFormat::RGBA16_Float);
    _machine->DeclareGBufferTarget("Old_SpecularShininess", SenFormat::RGBA8_Unorm);
    _machine->DeclareGBufferTarget("Old_Emissive",          SenFormat::R11G11B10_Float);
    _machine->DeclareDepthTarget        ("Old_Depth",             SenFormat::Depth32);
    _machine->DeclareTextureTarget      ("Old_HDR",               SenFormat::R11G11B10_Float);
    _machine->DeclareTextureTarget      ("Old_BloomOutput",       SenFormat::R11G11B10_Float);
    _machine->DeclareTextureTarget      ("Old_TonemapperOutput",  SenFormat::R11G11B10_Float);

    _assetRegistry.AddTexture("Old_BloomDirt",
        BeTexture::Create("Old_BloomDirt")
        .LoadFromFile("assets/bloom-dirt-mask.png")
        .Build()
    );
}

auto OldScene::DefineScene() -> void {
    _registry.clear();

    CreateEntity(_registry
        ,NameComponent { .Name = "Macintosh" }
        ,TransformComponent { .Position = glm::vec3(0, 0, -6.9f), .Rotation = glm::quat(glm::vec3(0, 0, 0)), .Scale = glm::vec3(1.f) }
        ,RenderComponent { .Prop = _macintosh, .CastShadows = true }
    );
    CreateEntity(_registry
        ,NameComponent { .Name = "Plane" }
        ,TransformComponent { .Position = glm::vec3(0, 0, 0), .Rotation = glm::quat(glm::vec3(0, 0, 0)), .Scale = glm::vec3(1.f) }
        ,RenderComponent { .Prop = _plane, .CastShadows = false }
    );
    CreateEntity(_registry
        ,NameComponent { .Name = "LivingCube" }
        ,TransformComponent { .Position = glm::vec3(0, 10, 0), .Rotation = glm::quat(glm::vec3(0, 0, 0)), .Scale = glm::vec3(2.f) }
        ,RenderComponent { .Prop = _cube, .CastShadows = true }
    );
    CreateEntity(_registry
        ,NameComponent { .Name = "Pagoda" }
        ,TransformComponent { .Position = glm::vec3(0, 0, 8), .Rotation = glm::quat(glm::vec3(0, 0, 0)), .Scale = glm::vec3(0.2f) }
        ,RenderComponent { .Prop = _pagoda, .CastShadows = true }
    );
    CreateEntity(_registry
        ,NameComponent { .Name = "WitchItems" }
        ,TransformComponent { .Position = glm::vec3(-3, 2, 5), .Rotation = glm::quat(glm::vec3(0, 0, 0)), .Scale = glm::vec3(3.f) }
        ,RenderComponent { .Prop = _witchItems, .CastShadows = true }
    );
    CreateEntity(_registry
        ,NameComponent { .Name = "Anvil1" }
        ,TransformComponent { .Position = glm::vec3(7, 0, 5), .Rotation = glm::quat(glm::vec3(0, glm::radians(90.f), 0)), .Scale = glm::vec3(0.2f) }
        ,RenderComponent { .Prop = _anvil, .CastShadows = true }
    );
    CreateEntity(_registry
        ,NameComponent { .Name = "Anvil2" }
        ,TransformComponent { .Position = glm::vec3(-7, 0, -3), .Rotation = glm::quat(glm::vec3(0, glm::radians(-90.f), 0)), .Scale = glm::vec3(0.2f) }
        ,RenderComponent { .Prop = _anvil, .CastShadows = true }
    );
    CreateEntity(_registry
        ,NameComponent { .Name = "Anvil3" }
        ,TransformComponent { .Position = glm::vec3(-17, -10, -3), .Rotation = glm::quat(glm::vec3(0, glm::radians(-90.f), 0)), .Scale = glm::vec3(1.0f) }
        ,RenderComponent { .Prop = _anvil, .CastShadows = true }
    );
    CreateEntity(_registry
        ,NameComponent { .Name = "Disks" }
        ,TransformComponent { .Position = glm::vec3(7.5f, 1, -4), .Rotation = glm::quat(glm::vec3(0, glm::radians(150.f), 0)), .Scale = glm::vec3(1.f) }
        ,RenderComponent { .Prop = _disks, .CastShadows = true }
    );

    CreateEntity(_registry
        ,NameComponent { .Name = "Sun" }
        ,SunLightComponent {
            .Direction = glm::normalize(glm::vec3(-0.8f, -1.0f, -0.8f)),
            .Color = glm::vec3(0.7f, 0.7f, 0.99f),
            .Power = (1.0f / 0.7f) * 0.7f,
            .CastsShadows = true,
            .ShadowCameraDistance = 100.0f,
            .ShadowMapWorldSize = 60.0f,
            .ShadowNearPlane = 0.1f,
            .ShadowFarPlane = 400.0f,
        }
    );

    for (uint32_t i = 0; i < 4; ++i) {
        CreateEntity(_registry
            ,NameComponent { .Name = "PointLight_" + std::to_string(i) }
            ,TransformComponent { .Scale = glm::vec3(0.5f) }
            ,PointLightComponent {
                .Radius = 20.0f,
                .Color = glm::vec3(0.99f, 0.8f, 0.6f),
                .Power = (1.0f / 0.7f) * 2.7f,
                .CastsShadows = true,
                .ShadowNearPlane = 0.1f,
            }
        );
    }
}

auto OldScene::Tick(float deltaTime) -> void {
    if (_gameIns->Input->GetKeyDown(GLFW_KEY_ESCAPE)) {
        _gameIns->Input->SetMouseCapture(false);
        _gameIns->SceneManager->RequestSceneChange("menu");
        return;
    }

    constexpr float moveSpeed = 5.0f;
    float speed = moveSpeed * deltaTime;
    if (_gameIns->Input->GetKey(GLFW_KEY_LEFT_SHIFT) || (_gameIns->Input->IsGamepadConnected() && _gameIns->Input->GetGamepadButton(GLFW_GAMEPAD_BUTTON_LEFT_BUMPER))) {
        speed *= 2.0f;
    }
    if (_gameIns->Input->GetKey(GLFW_KEY_W)) { _camera->Position += _camera->GetFront() * speed; }
    if (_gameIns->Input->GetKey(GLFW_KEY_S)) { _camera->Position -= _camera->GetFront() * speed; }
    if (_gameIns->Input->GetKey(GLFW_KEY_D)) { _camera->Position += _camera->GetRight() * speed; }
    if (_gameIns->Input->GetKey(GLFW_KEY_A)) { _camera->Position -= _camera->GetRight() * speed; }
    if (_gameIns->Input->GetKey(GLFW_KEY_E)) { _camera->Position += glm::vec3(0, 1, 0) * speed; }
    if (_gameIns->Input->GetKey(GLFW_KEY_Q)) { _camera->Position -= glm::vec3(0, 1, 0) * speed; }

    if (_gameIns->Input->IsGamepadConnected()) {
        const glm::vec2 leftStick = _gameIns->Input->GetGamepadLeftStick();
        _camera->Position += _camera->GetFront() * (leftStick.y * speed);
        _camera->Position += _camera->GetRight() * (leftStick.x * speed);

        const float verticalInput = _gameIns->Input->GetGamepadRightTrigger() - _gameIns->Input->GetGamepadLeftTrigger();
        _camera->Position += glm::vec3(0, 1, 0) * (verticalInput * speed);
    }

    glm::vec3 euler = _camera->GetEuler();

    bool captureMouse = false;
    if (_gameIns->Input->GetMouseButton(GLFW_MOUSE_BUTTON_RIGHT)) {
        constexpr float mouseSens = 0.1f;
        captureMouse = true;
        const glm::vec2 mouseDelta = _gameIns->Input->GetMouseDelta();
        euler.x += mouseDelta.x * mouseSens;
        euler.y -= mouseDelta.y * mouseSens;
    }
    _gameIns->Input->SetMouseCapture(captureMouse);

    if (_gameIns->Input->IsGamepadConnected()) {
        const glm::vec2 rightStick = _gameIns->Input->GetGamepadRightStick();
        constexpr float gamepadCameraSens = 100.0f;
        euler.x += rightStick.x * gamepadCameraSens * deltaTime;
        euler.y += rightStick.y * gamepadCameraSens * deltaTime;
    }

    euler.y = glm::clamp(euler.y, -89.0f, 89.0f);
    _camera->SetEuler(euler.x, euler.y);

    const glm::vec2 scrollDelta = _gameIns->Input->GetScrollDelta();
    if (scrollDelta.y != 0.0f) {
        _camera->Fov -= scrollDelta.y;
        _camera->Fov = glm::clamp(_camera->Fov, 20.0f, 90.0f);
    }

    {
        static float angle = 0.0f;
        angle += deltaTime * glm::radians(15.0f);
        if (angle > glm::two_pi<float>()) {
            angle -= glm::two_pi<float>();
        }

        const auto pointLights = _registry.view<NameComponent, TransformComponent, PointLightComponent>();
        auto i = size_t(0);
        for (const auto [entity, name, transform, _] : pointLights.each()) {
            constexpr float radius = 13.0f;
            const auto add = glm::two_pi<float>() * (float(i) / float(pointLights.size_hint()));
            const auto rad = radius * (0.7f + 0.3f * ((i + 1) % 2));
            transform.Position = glm::vec3(cos(angle + add) * rad, 4.0f + 4.0f * (i % 2), sin(angle + add) * rad);
            i++;
        }
    }

    FullScene::Tick(deltaTime);
    _machine->UniformMaterial->SetFloat1("Time", _time);
}
