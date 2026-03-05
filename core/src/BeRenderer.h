#pragma once

#include <d3d11.h>
#include <dxgi1_2.h>
#include <vector>
#include <wrl/client.h>
#include <memory>
#include <unordered_map>
#include <umbrellas/access-modifiers.hpp>
#include <umbrellas/include-glm.h>

#include "BeBuffers.h"
#include <sen-rhi/SenBuffer.h>

class BeWindow;
class BePipeline;
class BeRenderPass;
class BeShader;
using Microsoft::WRL::ComPtr;


class BeRenderer {

    // static part /////////////////////////////////////////////////////////////////////////////////////////////////////
    hide static auto GetBestAdapter() -> ComPtr<IDXGIAdapter1>;
    
    
    // fields //////////////////////////////////////////////////////////////////////////////////////////////////////////
    expose BeUniformData UniformData;

    hide
    uint32_t _width;
    uint32_t _height;
    HWND _hwnd;
    
    // dx11 core components
    ComPtr<ID3D11Device> _device;
    ComPtr<ID3D11DeviceContext> _context;
    ComPtr<IDXGIDevice> _dxgiDevice;
    ComPtr<IDXGIAdapter> _adapter;
    ComPtr<IDXGIFactory2> _factory;
    ComPtr<IDXGISwapChain1> _swapchain;
    ComPtr<ID3D11RenderTargetView> _backbufferTarget;
    std::shared_ptr<BePipeline> _pipeline = nullptr;

    SenBuffer _uniformBuffer;
    ComPtr<ID3D11DepthStencilState> _defaultDepthStencilState;
    ComPtr<ID3D11RasterizerState> _rasterizerCullBack;
    ComPtr<ID3D11RasterizerState> _rasterizerCullNone;
    
    std::vector<BeRenderPass*> _passes;

    // lifetime ////////////////////////////////////////////////////////////////////////////////////////////////////////
    expose
    explicit BeRenderer(
        uint32_t width,
        uint32_t height,
        HWND window
    );
    ~BeRenderer();
    
    auto LaunchDevice () -> void;


    // public interface ////////////////////////////////////////////////////////////////////////////////////////////////
    
    expose
    auto AddRenderPass(BeRenderPass* renderPass) -> void;
    auto ClearPasses() -> void;
    auto InitialisePasses() const -> void;
    auto Render() -> void;

    [[nodiscard]] auto GetDevice() const -> ComPtr<ID3D11Device> { return _device; }
    [[nodiscard]] auto GetContext() const -> ComPtr<ID3D11DeviceContext> { return _context; }
    [[nodiscard]] auto GetPipeline() const -> std::shared_ptr<BePipeline> { return _pipeline; }
    [[nodiscard]] auto GetBackbufferTarget() const -> ComPtr<ID3D11RenderTargetView> { return _backbufferTarget; }

    [[nodiscard]] auto GetWidth () const -> uint32_t { return _width; }
    [[nodiscard]] auto GetHeight () const -> uint32_t { return _height; }
    [[nodiscard]] auto GetRasterizerCullBack () const -> ComPtr<ID3D11RasterizerState> { return _rasterizerCullBack; }
    [[nodiscard]] auto GetRasterizerCullNone () const -> ComPtr<ID3D11RasterizerState> { return _rasterizerCullNone; }
};
