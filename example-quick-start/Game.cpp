
#include "Game.h"

#define NOMINMAX

#include <cassert>
#include <glfw/glfw3.h>
#include <umbrellas/include-glm.h>

#include "BeAssetRegistry.h"
#include "BeCamera.h"
#include "BeInput.h"
#include "BeModel.h"
#include "BeRenderer.h"
#include "BeShader.h"
#include "BeTexture.h"
#include "BeWindow.h"
#include "passes/BeBackbufferPass.h"
#include "passes/BeGeometryPass.h"

Game::Game() = default;
Game::~Game() = default;


auto Game::Run() -> int {
    BeShader::StandardShaderIncludePath = "standardShaders/";
    
    _width = 1920;
    _height = 1080;

    _window = std::make_shared<BeWindow>(_width, _height, "be: example game 1");
    _assetRegistry = std::make_shared<BeAssetRegistry>();
    _input = std::make_unique<BeInput>(_window);
    _renderer = std::make_shared<BeRenderer>(_width, _height, _window, _assetRegistry);
    _renderer->LaunchDevice();

    const auto device = _renderer->GetDevice();
    
    // Create builtin textures
    BeTexture::Create("white")
    .SetSize(1, 1)
    .SetBindFlags(D3D11_BIND_SHADER_RESOURCE)
    .SetFormat(DXGI_FORMAT_R8G8B8A8_UNORM)
    .FillWithColor(glm::vec4(1.f))
    .AddToRegistry(_assetRegistry)
    .BuildNoReturn(device);
    
    BeTexture::Create("black")
    .SetSize(1, 1)
    .SetBindFlags(D3D11_BIND_SHADER_RESOURCE)
    .SetFormat(DXGI_FORMAT_R8G8B8A8_UNORM)
    .FillWithColor(glm::vec4(0.f, 0.f, 0.f, 1.f))
    .AddToRegistry(_assetRegistry)
    .BuildNoReturn(device);
    
    // load shader
    const auto standardShader = BeShader::Create("assets/shaders/standard", *_renderer);
    _assetRegistry->AddShader("standard", standardShader);
    
    // load model
    const auto model = BeModel::Create("assets/model.fbx", standardShader, *_renderer);
    _renderer->SetModels({ model });

    // create entity
    const auto entity = _registry.create();
    _registry.emplace<NameComponent>(entity, "Model");
    _registry.emplace<TransformComponent>(entity, glm::vec3(0, 0, 0), glm::quat(glm::vec3(0, 0, 0)), glm::vec3(1.f));
    _registry.emplace<RenderComponent>(entity, model, true);

    // setup camera
    _camera = std::make_unique<BeCamera>(_renderer, _window);
    _camera->NearPlane = 0.1f;
    _camera->FarPlane = 200.0f;

    // setup render passes
    SetupRenderPasses();

    MainLoop();

    return 0;
}


auto Game::SetupRenderPasses() -> void {
    const auto device = _renderer->GetDevice();
    
    // Create render resources
    BeTexture::Create("DepthStencil")
    .SetBindFlags(D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE)
    .SetFormat(DXGI_FORMAT_R32_TYPELESS)
    .SetSize(_width, _height)
    .AddToRegistry(_assetRegistry)
    .Build(device);

    BeTexture::Create("BaseColor")
    .SetBindFlags(D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE)
    .SetFormat(DXGI_FORMAT_R11G11B10_FLOAT)
    .SetSize(_width, _height)
    .AddToRegistry(_assetRegistry)
    .Build(device);

    BeTexture::Create("WorldNormal")
    .SetBindFlags(D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE)
    .SetFormat(DXGI_FORMAT_R16G16B16A16_FLOAT)
    .SetSize(_width, _height)
    .AddToRegistry(_assetRegistry)
    .Build(device);

    BeTexture::Create("Specular-Shininess")
    .SetBindFlags(D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE)
    .SetFormat(DXGI_FORMAT_R8G8B8A8_UNORM)
    .SetSize(_width, _height)
    .AddToRegistry(_assetRegistry)
    .Build(device);

    // Geometry pass
    const auto geometryPass = new BeGeometryPass();
    _renderer->AddRenderPass(geometryPass);
    geometryPass->OutputDepthTextureName = "DepthStencil";
    geometryPass->OutputTexture0Name = "BaseColor";
    geometryPass->OutputTexture1Name = "WorldNormal";
    geometryPass->OutputTexture2Name = "Specular-Shininess";

    // Backbuffer pass
    const auto backbufferPass = new BeBackbufferPass();
    _renderer->AddRenderPass(backbufferPass);
    backbufferPass->InputTextureName = "BaseColor";
    backbufferPass->ClearColor = {0.f / 255.f, 23.f / 255.f, 31.f / 255.f}; // black

    _renderer->InitialisePasses();
}

auto Game::MainLoop() -> void {
    double lastTime = glfwGetTime();

    while (!_window->ShouldClose()) {
        _window->PollEvents();
        _input->Update();

        double now = glfwGetTime();
        float dt = static_cast<float>(now - lastTime);
        lastTime = now;

        _renderer->UniformData.Time = now;

        constexpr float moveSpeed = 5.0f;
        float speed = moveSpeed * dt;
        if (_input->GetKey(GLFW_KEY_LEFT_SHIFT)) speed *= 2.0f;
        if (_input->GetKey(GLFW_KEY_W)) _camera->Position += _camera->GetFront() * speed;
        if (_input->GetKey(GLFW_KEY_S)) _camera->Position -= _camera->GetFront() * speed;
        if (_input->GetKey(GLFW_KEY_D)) _camera->Position -= _camera->GetRight() * speed;
        if (_input->GetKey(GLFW_KEY_A)) _camera->Position += _camera->GetRight() * speed;
        if (_input->GetKey(GLFW_KEY_E)) _camera->Position += glm::vec3(0, 1, 0) * speed;
        if (_input->GetKey(GLFW_KEY_Q)) _camera->Position -= glm::vec3(0, 1, 0) * speed;

        bool captureMouse = false;
        if (_input->GetMouseButton(GLFW_MOUSE_BUTTON_RIGHT)) {
            constexpr float mouseSens = 0.1f;

            captureMouse = true;
            const glm::vec2 mouseDelta = _input->GetMouseDelta();
            _camera->Yaw   -= mouseDelta.x * mouseSens;
            _camera->Pitch -= mouseDelta.y * mouseSens;
            _camera->Pitch = glm::clamp(_camera->Pitch, -89.0f, 89.0f);
        }
        _input->SetMouseCapture(captureMouse);

        const glm::vec2 scrollDelta = _input->GetScrollDelta();
        if (scrollDelta.y != 0.0f) {
            _camera->Fov -= scrollDelta.y;
            _camera->Fov = glm::clamp(_camera->Fov, 20.0f, 90.0f);
        }

        _camera->Update();

        
        const auto renderView = _registry.view<TransformComponent, RenderComponent>();
        renderView.each([this](auto& transform, auto& render) {
            _renderer->SubmitDrawEntry(BeRenderer::DrawEntry{
                .Position = transform.Position,
                .Rotation = transform.Rotation,
                .Scale = transform.Scale,
                .Model = render.Model,
                .CastShadows = render.CastShadows,
            });
        });

        _renderer->Render();
    }
}