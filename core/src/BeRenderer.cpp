#include "BeRenderer.h"

#include <cassert>
#include <scope_guard/scope_guard.hpp>
#include <dxgi1_6.h>

#include "BeRenderPass.h"
#include "BeShader.h"
#include "Utils.h"
#include <sen-rhi/SenShaderCompiler.h>
#include "sen-rhi/SenBackend.h"

auto BeRenderer::GetBestAdapter() -> ComPtr<IDXGIAdapter1> {
    ComPtr<IDXGIFactory6> f6;
    Utils::Check << CreateDXGIFactory1(IID_PPV_ARGS(&f6));

    uint32_t adapterIndex = 0;
    bool outOfAdapters = false;
    while (!outOfAdapters)
    {
        SCOPE_EXIT { adapterIndex++; };
        
        ComPtr<IDXGIAdapter1> adapter;
        HRESULT hr = f6->EnumAdapterByGpuPreference(
            adapterIndex,
            DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
            IID_PPV_ARGS(&adapter)
        );
        if (FAILED(hr)) {
            outOfAdapters = true;
            continue;
        }
        
        DXGI_ADAPTER_DESC1 desc{};
        Utils::Check << adapter->GetDesc1(&desc);

        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) continue;

        return adapter; // first high-perf adapter
    }
    return nullptr;
}

BeRenderer::BeRenderer(
    uint32_t width,
    uint32_t height,
    HWND window
)
    : _width(width)
    , _height(height)
    , _hwnd(window)
{}

BeRenderer::~BeRenderer() {
    SenBackend::Shutdown();
}

auto BeRenderer::LaunchDevice() -> void {

    UINT deviceFlags = 0;
#if defined(_DEBUG)
    deviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    
    D3D_FEATURE_LEVEL featureLevelOut{};
    static constexpr D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0
    };
    
    const ComPtr<IDXGIAdapter1> adapter = GetBestAdapter();
    
    // Device and context
    Utils::Check << D3D11CreateDevice(
        adapter.Get(),
        D3D_DRIVER_TYPE_UNKNOWN,
        nullptr,
        deviceFlags,
        featureLevels,
        std::size(featureLevels),
        D3D11_SDK_VERSION,
        &_device,
        nullptr,
        &_context
    );

    
    // DXGI interfaces
    Utils::Check
    << _device.As(&_dxgiDevice)
    << _dxgiDevice->GetAdapter(&_adapter)
    << _adapter->GetParent(IID_PPV_ARGS(&_factory));
    
    // Swap chain
    DXGI_SWAP_CHAIN_DESC1 scDesc = {
        .Width = _width,
        .Height = _height,
        .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
        .SampleDesc = { .Count = 1 },
        .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
        .BufferCount = 2,
        .Scaling = DXGI_SCALING_STRETCH,
        .SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
        .AlphaMode = DXGI_ALPHA_MODE_IGNORE,
    };

    const auto& hwnd = _hwnd;
    Utils::Check << _factory->CreateSwapChainForHwnd(_device.Get(), hwnd, &scDesc, nullptr, nullptr, &_swapchain);
    Utils::Check << _factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER);

    SenShaderCompiler::Launch();
    SenBackend::Init(_device, _context);

    _commandBuffer = SenCommandBuffer(_context);

    ComPtr<ID3D11Texture2D> backBuffer;
    Utils::Check
    << _swapchain->GetBuffer(0, IID_PPV_ARGS(&backBuffer))
    << _device->CreateRenderTargetView(backBuffer.Get(), nullptr, &_backbufferTarget);

    _backbufferTexture = SenBackend::RegisterBackbuffer(_backbufferTarget);

    _uniformBuffer = SenBackend::CreateBuffer({
        .Usage  = SenBufferUsage::Constant,
        .Access = SenBufferAccess::Dynamic,
        .Size   = sizeof(BeUniformBufferGPU),
    });

    _uniformBindGroupLayout = SenBackend::CreateBindGroupLayout({ 
        .BufferEntries = {SenBindGroupLayoutEntry { .Slot = 0}} 
    });

    _uniformBindGroup = SenBackend::CreateBindGroup({
        .Layout = _uniformBindGroupLayout,
        .ConstantBuffers = {_uniformBuffer },
    });

    D3D11_DEPTH_STENCIL_DESC depthStencilStateDescriptor = {};
    depthStencilStateDescriptor.DepthEnable = true;
    depthStencilStateDescriptor.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    depthStencilStateDescriptor.DepthFunc = D3D11_COMPARISON_LESS;
    depthStencilStateDescriptor.StencilEnable = false;
    Utils::Check << _device->CreateDepthStencilState(&depthStencilStateDescriptor, _defaultDepthStencilState.GetAddressOf());
    _context->OMSetDepthStencilState(_defaultDepthStencilState.Get(), 1);

    D3D11_RASTERIZER_DESC rasterDesc = {};
    rasterDesc.FillMode = D3D11_FILL_SOLID;
    rasterDesc.CullMode = D3D11_CULL_BACK;
    rasterDesc.FrontCounterClockwise = FALSE;
    rasterDesc.DepthClipEnable = TRUE;
    Utils::Check << _device->CreateRasterizerState(&rasterDesc, _rasterizerCullBack.GetAddressOf());

    rasterDesc.CullMode = D3D11_CULL_NONE;
    Utils::Check << _device->CreateRasterizerState(&rasterDesc, _rasterizerCullNone.GetAddressOf());

    _context->RSSetState(_rasterizerCullBack.Get());
}

auto BeRenderer::AddRenderPass(BeRenderPass* renderPass) -> void {
    _passes.push_back(renderPass);
    renderPass->InjectRenderer(this);
}

auto BeRenderer::ClearPasses() -> void {
    for (auto pass : _passes) {
        delete pass;
    }
    _passes.clear();
}

auto BeRenderer::InitialisePasses() const -> void {
    for (const auto& pass : _passes)
        pass->Initialise();
}

auto BeRenderer::Render() -> void {
    Utils::BeDebugAnnotation frameAnnotation(_context, "Frame");

    const BeUniformBufferGPU uniformDataGpu(UniformData);
    SenBackend::WriteBuffer(_uniformBuffer, &uniformDataGpu, sizeof(BeUniformBufferGPU));

    _commandBuffer.SetBindGroup(_uniformBindGroup, 0);

    for (const auto& pass : _passes) {
        Utils::BeDebugAnnotation passAnnotation(_context, std::string(pass->GetPassName()));
        pass->Render();
    }

    _swapchain->Present(1, 0);
}

