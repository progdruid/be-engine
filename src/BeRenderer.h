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
using Microsoft::WRL::ComPtr;


class BeRenderer {
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
    explicit BeRenderer(HWND windowHandle, uint32_t width, uint32_t height);
    ~BeRenderer() = default;

public:
    glm::vec3 ClearColor;
    
    UniformData UniformData;
    DirectionalLightData DirectionalLightData;
    std::vector<PointLightData> PointLights;

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
    ComPtr<ID3D11RenderTargetView> _backbufferTarget;
    ComPtr<ID3D11ShaderResourceView> _depthStencilResource;
    ComPtr<ID3D11DepthStencilView> _depthStencilView;
    ComPtr<ID3D11DepthStencilState> _depthStencilState;
    ID3D11RenderTargetView* _gbufferTargets[3];
    ID3D11ShaderResourceView* _gbufferResources[3];
    ComPtr<ID3D11RenderTargetView> _lightingTarget;
    ComPtr<ID3D11BlendState> _lightingBlendState;
    ComPtr<ID3D11ShaderResourceView> _lightingResource;
    
    // constant buffers
    ComPtr<ID3D11Buffer> _uniformBuffer;
    ComPtr<ID3D11Buffer> _materialBuffer;
    ComPtr<ID3D11Buffer> _directionalLightBuffer;
    ComPtr<ID3D11Buffer> _pointLightBuffer;
    //ComPtr<ID3D11Buffer> _spotLightBuffer;
    ComPtr<ID3D11SamplerState> _pointSampler;
    
    // shaders
    std::unique_ptr<BeShader> _directionalLightShader;
    std::unique_ptr<BeShader> _pointLightShader;
    std::unique_ptr<BeShader> _fullscreenShader = nullptr;

    // geometry pass
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
