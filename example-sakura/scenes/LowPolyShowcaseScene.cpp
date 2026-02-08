#include "LowPolyShowcaseScene.h"

#include <glfw/glfw3.h>

#include "BeAssetRegistry.h"
#include "BeCamera.h"
#include "BeInput.h"
#include "BeMaterial.h"
#include "BeModel.h"
#include "BeRenderer.h"
#include "BeTexture.h"
#include "BeWindow.h"
#include "Components.h"
#include "Game.h"
#include "basic-render-pipeline/BeBackbufferPass.h"
#include "basic-render-pipeline/BeBloomPass.h"
#include "basic-render-pipeline/BeFullscreenEffectPass.h"
#include "basic-render-pipeline/BeGeometryPass.h"
#include "basic-render-pipeline/BeLightingPass.h"
#include "basic-render-pipeline/BeShadowPass.h"

LowPolyShowcaseScene::LowPolyShowcaseScene(Game* game) : BaseScene(game) {}
LowPolyShowcaseScene::~LowPolyShowcaseScene() = default;

void LowPolyShowcaseScene::Prepare() {

    // Showcase camera (default)
    _showcaseCamera = std::make_shared<BeCamera>();
    _showcaseCamera->Width = GameIns->Window->GetWidth();
    _showcaseCamera->Height = GameIns->Window->GetHeight();
    _showcaseCamera->NearPlane = 0.1f;
    _showcaseCamera->FarPlane = 200.0f;

    // Free control camera
    _freeCamera = std::make_shared<BeCamera>();
    _freeCamera->Width = GameIns->Window->GetWidth();
    _freeCamera->Height = GameIns->Window->GetHeight();
    _freeCamera->NearPlane = 0.1f;
    _freeCamera->FarPlane = 200.0f;
    _freeCamera->Position = {0, 0, -5};
    
    BeAssetRegistry::IndexShaderFiles({ 
        "assets/shaders/objectMaterial.hlsl", 
        "assets/shaders/standard.hlsl",
        "assets/shaders/checkerboard.hlsl",
        "assets/shaders/fullscreen-vertex.hlsl", 
        "assets/shaders/directionalLight.hlsl", 
        "assets/shaders/pointLight.hlsl", 
        "assets/shaders/emissive-add.hlsl",
        "assets/shaders/BeBloomAdd.hlsl", 
        "assets/shaders/BeBloomBright.hlsl", 
        "assets/shaders/BeBloomKawase.hlsl", 
        "assets/shaders/tonemapper.hlsl", 
        "assets/shaders/backbuffer.hlsl", 
    });
    
    CreateTargetTextures();
    LoadModels();
    CreateObjects();
    
    GameIns->Renderer->UniformData.AmbientColor = glm::vec3(0.1);
}

auto LowPolyShowcaseScene::CreateTargetTextures() -> void {
    const uint32_t screenWidth = GameIns->Window->GetWidth();
    const uint32_t screenHeight = GameIns->Window->GetHeight();
    const auto device = GameIns->Renderer->GetDevice();
    
    BeTexture::Create("LP_DepthStencil")
    .SetBindFlags(D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE)
    .SetFormat(DXGI_FORMAT_R32_TYPELESS)
    .SetSize(screenWidth, screenHeight)
    .AddToRegistry()
    .Build(device);

    BeTexture::Create("LP_BaseColor")
    .SetBindFlags(D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE)
    .SetFormat(DXGI_FORMAT_R11G11B10_FLOAT)
    .SetSize(screenWidth, screenHeight)
    .AddToRegistry()
    .Build(device);

    BeTexture::Create("LP_WorldNormal")
    .SetBindFlags(D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE)
    .SetFormat(DXGI_FORMAT_R16G16B16A16_FLOAT)
    .SetSize(screenWidth, screenHeight)
    .AddToRegistry()
    .Build(device);

    BeTexture::Create("LP_Specular-Shininess")
    .SetBindFlags(D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE)
    .SetFormat(DXGI_FORMAT_R8G8B8A8_UNORM)
    .SetSize(screenWidth, screenHeight)
    .AddToRegistry()
    .Build(device);

    BeTexture::Create("LP_Emissive")
    .SetBindFlags(D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE)
    .SetFormat(DXGI_FORMAT_R11G11B10_FLOAT)
    .SetSize(screenWidth, screenHeight)
    .AddToRegistry()
    .Build(device);
}


auto LowPolyShowcaseScene::LoadModels() -> void {
    auto device = GameIns->Renderer->GetDevice();
    
    auto standardShader = BeAssetRegistry::GetShader("standard");
    
    
    auto ramen = BeModel::Create("assets/ramen/scene.gltf", standardShader, *GameIns->Renderer);
    BeAssetRegistry::AddModel("ramen", ramen);
    
    auto stillLife = BeModel::Create("assets/still-life/scene.gltf", standardShader, *GameIns->Renderer);
    BeAssetRegistry::AddModel("still-life", stillLife);
    
    auto fiestaTea = BeModel::Create("assets/fiesta_tea/scene.gltf", standardShader, *GameIns->Renderer);
    BeAssetRegistry::AddModel("fiesta-tea", fiestaTea);
    
    auto honeydew_melons = BeModel::Create("assets/honeydew_melons/scene.gltf", standardShader, *GameIns->Renderer);
    BeAssetRegistry::AddModel("honeydew_melons", honeydew_melons);
    
    auto hunger_games = BeModel::Create("assets/hunger_games/scene.gltf", standardShader, *GameIns->Renderer);
    BeAssetRegistry::AddModel("hunger_games", hunger_games);
    
    auto pickles = BeModel::Create("assets/pickles/scene.gltf", standardShader, *GameIns->Renderer);
    BeAssetRegistry::AddModel("pickles", pickles);
    
    auto watermelons = BeModel::Create("assets/watermelons/scene.gltf", standardShader, *GameIns->Renderer);
    BeAssetRegistry::AddModel("watermelons", watermelons);
    
    auto apfel = BeModel::Create("assets/apfel/scene.gltf", standardShader, *GameIns->Renderer);
    BeAssetRegistry::AddModel("apfel", apfel);
    
    auto eggplant = BeModel::Create("assets/eggplant/scene.gltf", standardShader, *GameIns->Renderer);
    BeAssetRegistry::AddModel("eggplant", eggplant);
    
    auto tomatoes = BeModel::Create("assets/tomatoes/scene.gltf", standardShader, *GameIns->Renderer);
    BeAssetRegistry::AddModel("tomatoes", tomatoes);
    
    
    auto skycube = BeModel::Create("assets/cube.glb", standardShader, *GameIns->Renderer);
    skycube->Materials[0]->SetFloat3("DiffuseColor", HexColor("#FAC8CD"));
    BeAssetRegistry::AddModel("skycube", skycube);
    
    auto models = std::vector<std::shared_ptr<BeModel>> {
        skycube, 
        ramen,          //1
        stillLife,      //2
        fiestaTea,      //3
        honeydew_melons,//4
        hunger_games,   //5
        pickles,        //6
        watermelons,    //7
        apfel,          //8
        eggplant,       //9
        tomatoes        //0
    };
    GameIns->Renderer->RegisterModels(models);
}

auto LowPolyShowcaseScene::CreateObjects() -> void {
    
    auto device = GameIns->Renderer->GetDevice();
    
    CreateEntity(_registry
        ,NameComponent { .Name = "showcased-object" }
        ,TransformComponent { }
        ,RenderComponent { .Model = BeAssetRegistry::GetModel("ramen").lock(), .CastShadows = false }
    );
    
    CreateEntity(_registry
        ,NameComponent { .Name = "skycube" }
        ,TransformComponent { .Position = glm::vec3(50, -50, -50), .Rotation = glm::quat(), .Scale = glm::vec3(100) }
        ,RenderComponent { .Model = BeAssetRegistry::GetModel("skycube").lock(), .CastShadows = false }
    );
    
    CreateEntity(_registry
        ,NameComponent { .Name = "Moon" }
        ,SunLightComponent {
            .Direction = { -1, -1, -1 },
            .Color = glm::vec3(0.7f, 0.7f, 0.99),
            .Power = (1.0f / 0.7f) * 0.7f,
            .CastsShadows = true,
            .ShadowMapResolution = 4096,
            .ShadowCameraDistance = 100.0f,
            .ShadowMapWorldSize = 60.0f,
            .ShadowNearPlane = 0.1f,
            .ShadowFarPlane = 400.0f,
            .ShadowMap = BeTexture::Create("SunLightShadowMap")
                .SetBindFlags(D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE)
                .SetFormat(DXGI_FORMAT_R32_TYPELESS)
                .SetSize(4096, 4096)
                .AddToRegistry()
                .Build(device)
        }
    );
}


void LowPolyShowcaseScene::OnLoad() {
    LoadPasses();
}

auto LowPolyShowcaseScene::LoadPasses() -> void {
    const auto& device = GameIns->Renderer->GetDevice();
    
    GameIns->Renderer->ClearPasses();

    const auto shadowPass = new BeShadowPass();
    GameIns->Renderer->AddRenderPass(shadowPass);
    shadowPass->SubmissionBuffer = GameIns->SubmissionBuffer;

    const auto geometryPass = new BeGeometryPass();
    GameIns->Renderer->AddRenderPass(geometryPass);
    geometryPass->SubmissionBuffer = GameIns->SubmissionBuffer;
    geometryPass->OutputDepthTexture = BeAssetRegistry::GetTexture("LP_DepthStencil");
    geometryPass->OutputTexture0 = BeAssetRegistry::GetTexture("LP_BaseColor");
    geometryPass->OutputTexture1 = BeAssetRegistry::GetTexture("LP_WorldNormal");
    geometryPass->OutputTexture2 = BeAssetRegistry::GetTexture("LP_Specular-Shininess");
    geometryPass->OutputTexture3 = BeAssetRegistry::GetTexture("LP_Emissive");

    const auto backbufferPass = new BeBackbufferPass();
    GameIns->Renderer->AddRenderPass(backbufferPass);
    backbufferPass->InputTexture = BeAssetRegistry::GetTexture("LP_BaseColor");
    backbufferPass->ClearColor = {0.f / 255.f, 23.f / 255.f, 31.f / 255.f};
    
    GameIns->Renderer->InitialisePasses();
}



void LowPolyShowcaseScene::Tick(float deltaTime) {

    // Toggle between cameras with [C]
    if (GameIns->Input->GetKeyDown(GLFW_KEY_C)) {
        _useShowcaseCamera = !_useShowcaseCamera;
        
        _freeCamera->Position = _showcaseCamera->Position;
        _freeCamera->Yaw = _showcaseCamera->Yaw;
        _freeCamera->Pitch = _showcaseCamera->Pitch;
    }
    
    if (GameIns->Input->GetKeyDown(GLFW_KEY_1)) {
        ChangeShowcase("ramen", "#FAC8CD", TransformComponent());
    }
    else if (GameIns->Input->GetKeyDown(GLFW_KEY_2)) {
        ChangeShowcase("still-life", "#D0D0C4", TransformComponent { .Position = { 0.f, 1.f, 0.f }, .Scale = glm::vec3(4.f) } ); 
    }
    else if (GameIns->Input->GetKeyDown(GLFW_KEY_3)) {
        ChangeShowcase("fiesta-tea", "#61636D", TransformComponent { .Position = { 0, -1, 0}, .Scale = glm::vec3(2.f) });
    }
    else if (GameIns->Input->GetKeyDown(GLFW_KEY_4)) {
        ChangeShowcase("honeydew_melons", "#855C36", TransformComponent { .Position = {0.f, 1.f, 0.f} });
    }
    else if (GameIns->Input->GetKeyDown(GLFW_KEY_5)) {
        ChangeShowcase("hunger_games", "#39708E", TransformComponent { .Position = {0.f, 3.f, 0.f}, .Scale = glm::vec3(2.f) });
    }
    else if (GameIns->Input->GetKeyDown(GLFW_KEY_6)) {
        ChangeShowcase("pickles", "#FEC693", TransformComponent { .Position = {0.f, 1.f, 0.f}, .Scale = glm::vec3(24.f) });
    }
    else if (GameIns->Input->GetKeyDown(GLFW_KEY_7)) {
        ChangeShowcase("watermelons", "#A3A17B", TransformComponent { .Position = {0.f, 1.f, 0.f}, .Scale = glm::vec3(90.f) });
    }
    else if (GameIns->Input->GetKeyDown(GLFW_KEY_8)) {
        ChangeShowcase("apfel", "#73615E", TransformComponent { .Position = {0.f, 1.f, 0.f}, .Scale = glm::vec3(2.f) });
    }
    else if (GameIns->Input->GetKeyDown(GLFW_KEY_9)) {
        ChangeShowcase("eggplant", "#E1D5F2", TransformComponent { .Position = {0.f, 1.f, 0.f}, .Scale = glm::vec3(0.2f) });
    }
    else if (GameIns->Input->GetKeyDown(GLFW_KEY_0)) {
        ChangeShowcase("tomatoes", "#E4FDE1", TransformComponent { .Position = {0.f, 1.f, 0.f}, .Scale = glm::vec3(2.f) });
    }

    // Update the appropriate camera
    if (_useShowcaseCamera) {
        UpdateShowcaseCamera(deltaTime);
    } else {
        UpdateFreeCamera(deltaTime);
    }

    static const auto GeometryView = _registry.view<TransformComponent, RenderComponent>();
    static const auto SunView = _registry.view<SunLightComponent>();

    GameIns->SubmissionBuffer->ClearEntries();
    for (const auto [entity, transform, render] : GeometryView.each()) {
        auto entry = BeBRPGeometryEntry();
        entry.Model = render.Model;
        entry.CastShadows = render.CastShadows;
        entry.ModelMatrix = BeBRPGeometryEntry::CalculateModelMatrix(
            transform.Position,
            transform.Rotation,
            transform.Scale
        );

        GameIns->SubmissionBuffer->SubmitGeometry(entry);
    }

    for (const auto [entity, sunLight] : SunView.each()) {
        auto entry = BeBRPSunLightEntry();
        entry.Direction = sunLight.Direction;
        entry.Color = sunLight.Color;
        entry.Power = sunLight.Power;
        entry.CastsShadows = sunLight.CastsShadows;
        entry.ShadowMapResolution = sunLight.ShadowMapResolution;
        entry.ShadowMap = sunLight.ShadowMap;
        entry.ShadowViewProjection = BeBRPSunLightEntry::CalculateViewProj(
            entry.Direction,
            sunLight.ShadowCameraDistance,
            sunLight.ShadowMapWorldSize,
            sunLight.ShadowNearPlane,
            sunLight.ShadowFarPlane
        );

        GameIns->SubmissionBuffer->SubmitSunLight(entry);
    }
}

auto LowPolyShowcaseScene::ChangeShowcase(
    const std::string& modelName, 
    const std::string& hxcolor,
    const TransformComponent& adjustedTransform
) -> void {
    
    auto view = _registry.view<NameComponent, TransformComponent, RenderComponent>();
    for (auto [entity, name, transform, render] : view.each()) {
        if (name.Name == "showcased-object") {
            transform = adjustedTransform;            
            render.Model = BeAssetRegistry::GetModel(modelName).lock();
        }
        if (name.Name == "skycube") {
            render.Model->Materials[0]->SetFloat3("DiffuseColor", HexColor(hxcolor.c_str()));
        }
    }
}

auto LowPolyShowcaseScene::UpdateFreeCamera(float deltaTime) -> void {

    constexpr float moveSpeed = 5.0f;
    float speed = moveSpeed * deltaTime;
    if (GameIns->Input->GetKey(GLFW_KEY_LEFT_SHIFT) || (GameIns->Input->IsGamepadConnected() && GameIns->Input->GetGamepadButton(GLFW_GAMEPAD_BUTTON_LEFT_BUMPER))) speed *= 2.0f;
    if (GameIns->Input->GetKey(GLFW_KEY_W)) _freeCamera->Position += _freeCamera->GetFront() * speed;
    if (GameIns->Input->GetKey(GLFW_KEY_S)) _freeCamera->Position -= _freeCamera->GetFront() * speed;
    if (GameIns->Input->GetKey(GLFW_KEY_D)) _freeCamera->Position -= _freeCamera->GetRight() * speed;
    if (GameIns->Input->GetKey(GLFW_KEY_A)) _freeCamera->Position += _freeCamera->GetRight() * speed;
    if (GameIns->Input->GetKey(GLFW_KEY_E)) _freeCamera->Position += glm::vec3(0, 1, 0) * speed;
    if (GameIns->Input->GetKey(GLFW_KEY_Q)) _freeCamera->Position -= glm::vec3(0, 1, 0) * speed;

    // Gamepad movement
    if (GameIns->Input->IsGamepadConnected()) {
        const glm::vec2 leftStick = GameIns->Input->GetGamepadLeftStick();
        _freeCamera->Position += _freeCamera->GetFront() * (leftStick.y * speed);
        _freeCamera->Position -= _freeCamera->GetRight() * (leftStick.x * speed);

        const float verticalInput = GameIns->Input->GetGamepadRightTrigger() - GameIns->Input->GetGamepadLeftTrigger();
        _freeCamera->Position += glm::vec3(0, 1, 0) * (verticalInput * speed);
    }

    bool captureMouse = false;
    if (GameIns->Input->GetMouseButton(GLFW_MOUSE_BUTTON_RIGHT)) {
        constexpr float mouseSens = 0.1f;

        captureMouse = true;
        const glm::vec2 mouseDelta = GameIns->Input->GetMouseDelta();
        _freeCamera->Yaw   -= mouseDelta.x * mouseSens;
        _freeCamera->Pitch -= mouseDelta.y * mouseSens;
        _freeCamera->Pitch = glm::clamp(_freeCamera->Pitch, -89.0f, 89.0f);
    }
    GameIns->Input->SetMouseCapture(captureMouse);

    // Gamepad camera look
    if (GameIns->Input->IsGamepadConnected()) {
        const glm::vec2 rightStick = GameIns->Input->GetGamepadRightStick();
        constexpr float gamepadCameraSens = 100.0f;

        _freeCamera->Yaw   -= rightStick.x * gamepadCameraSens * deltaTime;
        _freeCamera->Pitch += rightStick.y * gamepadCameraSens * deltaTime;
        _freeCamera->Pitch = glm::clamp(_freeCamera->Pitch, -89.0f, 89.0f);
    }

    const glm::vec2 scrollDelta = GameIns->Input->GetScrollDelta();
    if (scrollDelta.y != 0.0f) {
        _freeCamera->Fov -= scrollDelta.y;
        _freeCamera->Fov = glm::clamp(_freeCamera->Fov, 20.0f, 90.0f);
    }

    _freeCamera->Update();
    GameIns->Renderer->UniformData.NearFarPlane = {_freeCamera->NearPlane, _freeCamera->FarPlane};
    GameIns->Renderer->UniformData.ProjectionView = _freeCamera->GetProjectionMatrix() * _freeCamera->GetViewMatrix();
    GameIns->Renderer->UniformData.CameraPosition = _freeCamera->Position;
}

auto LowPolyShowcaseScene::UpdateShowcaseCamera(float deltaTime) -> void {

    GameIns->Input->SetMouseCapture(false);

    static float orbitAngle = 0.0f;
    static float orbitPitch = glm::radians(45.f);
    static float orbitRadius = 7.0f;
    constexpr float orbitSpeed = 0.3f; // radians per second
    constexpr float scrollSpeed = 1.f;
    constexpr glm::vec3 lookTarget = { 0.f, 2.f, 0.f };

    orbitAngle += orbitSpeed * deltaTime;

    orbitRadius = glm::max(orbitRadius + scrollSpeed * -GameIns->Input->GetScrollDelta().y, 0.5f);
    
    _showcaseCamera->Position = lookTarget + glm::vec3(
        glm::cos(orbitAngle) * orbitRadius,
        glm::sin(orbitPitch) * orbitRadius,
        glm::sin(orbitAngle) * orbitRadius
    );

    // Look at origin
    const glm::vec3 lookDir = glm::normalize(lookTarget - _showcaseCamera->Position);

    _showcaseCamera->Yaw = glm::degrees(glm::atan(lookDir.z, lookDir.x));
    _showcaseCamera->Pitch = glm::degrees(glm::asin(lookDir.y));

    _showcaseCamera->Update();
    GameIns->Renderer->UniformData.NearFarPlane = {_showcaseCamera->NearPlane, _showcaseCamera->FarPlane};
    GameIns->Renderer->UniformData.ProjectionView = _showcaseCamera->GetProjectionMatrix() * _showcaseCamera->GetViewMatrix();
    GameIns->Renderer->UniformData.CameraPosition = _showcaseCamera->Position;
}
