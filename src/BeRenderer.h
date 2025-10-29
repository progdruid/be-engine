#pragma once

#include <d3d11.h>
#include <dxgi1_2.h>
#include <vector>
#include <wrl/client.h>
#include <memory>

#include "BeModel.h"
#include "BeBuffers.h"
#include "BeRenderResource.h"

class BeRenderPass;
class BeShader;
using Microsoft::WRL::ComPtr;


class BeRenderer {

public:
    explicit BeRenderer(HWND windowHandle, uint32_t width, uint32_t height);
    ~BeRenderer() = default;

public:
    UniformData UniformData;

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
    ComPtr<ID3D11RenderTargetView> _backbufferTarget;

    ComPtr<ID3D11Buffer> _uniformBuffer;
    ComPtr<ID3D11SamplerState> _pointSampler;
    std::unique_ptr<BeShader> _fullscreenShader = nullptr;
    
    std::unordered_map<std::string, BeRenderResource> _renderResources;
    std::vector<BeRenderPass*> _passes;

public:
    [[nodiscard]] auto GetDevice() const -> ComPtr<ID3D11Device> { return _device; }
    [[nodiscard]] auto GetContext() const -> ComPtr<ID3D11DeviceContext> { return _context; }
    [[nodiscard]] auto GetPointSampler() const -> ComPtr<ID3D11SamplerState> { return _pointSampler; }
    [[nodiscard]] auto GetFullscreenVertexShader() const -> ComPtr<ID3D11VertexShader> { return _fullscreenShader->VertexShader; }
    [[nodiscard]] auto GetBackbufferTarget() const -> ComPtr<ID3D11RenderTargetView> { return _backbufferTarget; }
    
    auto LaunchDevice () -> void;
    auto AddRenderPass(BeRenderPass* renderPass) -> void;
    auto InitialisePasses() -> void;
    auto Render() -> void;

    auto CreateRenderResource(
        const std::string& name,
        const bool useWindowSize,
        const BeRenderResource::BeResourceDescriptor& desc
    ) -> BeRenderResource*;
    auto GetRenderResource(const std::string& name) -> BeRenderResource*;
    
private:
    void TerminateRenderer();
};
