#pragma once

#include <d3d11.h>
#include <dxgi1_2.h>
#include <glm.hpp>
#include <gtc/quaternion.hpp>
#include <vector>
#include <wrl/client.h>

#include "BeModel.h"
#include "BeBuffers.h"

class BeShader;
class ModelAsset;
using Microsoft::WRL::ComPtr;


class Renderer {
public:
    struct ObjectEntry {
        std::string Name;
        glm::vec3 Position = {0.f, 0.f, 0.f};
        glm::quat Rotation = glm::quat(glm::vec3(0, 0, 0));
        glm::vec3 Scale = {1.f, 1.f, 1.f};
        BeModel* Model;
        std::vector<BeModel::BeDrawSlice> DrawSlices;
        BeShader* Shader;
    };

public:
    explicit Renderer(HWND windowHandle, uint32_t width, uint32_t height);
    ~Renderer() = default;

public:
    glm::vec3 ClearColor;
    
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
    ComPtr<ID3D11Buffer> _materialBuffer;
    ComPtr<ID3D11SamplerState> _pointSampler;

    // geometry buffers
    ComPtr<ID3D11Buffer> _sharedVertexBuffer;
    ComPtr<ID3D11Buffer> _sharedIndexBuffer;
    
    // scene objects
    std::vector<ObjectEntry> _objects;

    BeTexture _whiteFallbackTexture {glm::vec4(1.0f)};

public:
    [[nodiscard]] auto GetDevice() const -> ComPtr<ID3D11Device> { return _device; }

    auto LaunchDevice () -> void;
    auto PushObjects (const std::vector<ObjectEntry>& objects) -> void;
    auto Render() -> void;

private:
    void TerminateRenderer();
};
