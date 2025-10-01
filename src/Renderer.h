#pragma once

#include <d3d11.h>
#include <dxgi1_2.h>
#include <glm.hpp>
#include <gtc/quaternion.hpp>
#include <memory>
#include <vector>
#include <wrl/client.h>

#include "BeModel.h"

class BeShader;
class ModelAsset;
using Microsoft::WRL::ComPtr;

class Renderer {
    struct alignas(16) UniformBufferData {
        glm::mat4x4 ProjectionView;
        glm::vec3 CameraPosition = {0.f, 0.f, 0.f}; float __pad0;

        glm::vec3 AmbientColor = {0.f, 0.f, 0.f}; float __pad1;
        glm::vec3 DirectionalLightVector = glm::normalize(glm::vec3(-1.f, -1.f, -1.f)); float __pad2;
        glm::vec3 DirectionalLightColor = {1.f, 1.f, 1.f}; float __pad3;
    };

    struct alignas(16) ObjectBufferData {
        glm::mat4x4 Model;
    };

public:
    struct ObjectEntry {
        std::string Name;
        glm::vec3 Position = {0.f, 0.f, 0.f};
        glm::quat Rotation = glm::quat(glm::vec3(0, 0, 0));
        glm::vec3 Scale = {1.f, 1.f, 1.f};
        BeModel* Model;
        std::vector<BeModel::BeMeshInstruction> MeshInstructions;
        BeShader* Shader;
    };

public:
    explicit Renderer(HWND windowHandle, uint32_t width, uint32_t height);
    ~Renderer() = default;

public:
    glm::vec3 ClearColor = {53.f / 255.f, 144.f / 255.f, 243.f / 255.f}; // blue
    //glm::vec3 ClearColor = {255.f / 255.f, 205.f / 255.f, 27.f / 255.f}; // gold

    UniformBufferData UniformData;

private:

    // window
    HWND _windowHandle;
    uint32_t _width;
    uint32_t _height;

    // dx11 core components
    ComPtr<ID3D11Device> _device;
    ComPtr<ID3D11DeviceContext> _context;
    ComPtr<IDXGIDevice> _dxgiDevice;
    ComPtr<IDXGIAdapter> _adapter;
    ComPtr<IDXGIFactory2> _factory;
    ComPtr<IDXGISwapChain1> _swapchain;

    // targets
    ComPtr<ID3D11RenderTargetView> _renderTarget;
    ComPtr<ID3D11DepthStencilView> _depthStencilView;
    ComPtr<ID3D11DepthStencilState> _depthStencilState;

    // constant buffers
    ComPtr<ID3D11Buffer> _uniformBuffer;
    ComPtr<ID3D11Buffer> _objectBuffer;
    ComPtr<ID3D11SamplerState> _pointSampler;

    // geometry buffers
    ComPtr<ID3D11Buffer> _sharedVertexBuffer;
    ComPtr<ID3D11Buffer> _sharedIndexBuffer;
    
    // scene objects
    std::vector<ObjectEntry> _objects;

public:
    [[nodiscard]] auto GetDevice() const -> ComPtr<ID3D11Device> { return _device; }

    auto LaunchDevice () -> void;
    auto PushObjects (const std::vector<ObjectEntry>& objects) -> void;
    auto Render() -> void;

private:
    void TerminateRenderer();
};
