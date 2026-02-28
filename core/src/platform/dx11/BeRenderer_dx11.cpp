#include "BeRenderer.h"
#include "BePipeline.h"
#include "BeRendererImpl.h"
#include "DxUtils.h"

#include <cassert>
#include <dxgi1_6.h>
#include <scope_guard/scope_guard.hpp>

#include "BeBuffers.h"
#include "BeWindow.h"

static auto GetBestAdapter() -> ComPtr<IDXGIAdapter1> {
    ComPtr<IDXGIFactory6> f6;
    DxUtils::Check << CreateDXGIFactory1(IID_PPV_ARGS(&f6));

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
        DxUtils::Check << adapter->GetDesc1(&desc);
        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) continue;

        return adapter;
    }
    return nullptr;
}

BeRenderer::BeRenderer(
    uint32_t width,
    uint32_t height,
    BeWindow& window
)
    : _width(width)
    , _height(height)
    , _impl(std::make_unique<BeRendererImpl>())
{
    _impl->hwnd = static_cast<HWND>(window.GetNativeHandle());
}

BeRenderer::~BeRenderer() = default;

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

    DxUtils::Check << D3D11CreateDevice(
        adapter.Get(),
        D3D_DRIVER_TYPE_UNKNOWN,
        nullptr,
        deviceFlags,
        featureLevels,
        std::size(featureLevels),
        D3D11_SDK_VERSION,
        &_impl->device,
        nullptr,
        &_impl->context
    );

    DxUtils::Check
    << _impl->device.As(&_impl->dxgiDevice)
    << _impl->dxgiDevice->GetAdapter(&_impl->adapter)
    << _impl->adapter->GetParent(IID_PPV_ARGS(&_impl->factory));

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

    const auto& hwnd = _impl->hwnd;
    DxUtils::Check << _impl->factory->CreateSwapChainForHwnd(_impl->device.Get(), hwnd, &scDesc, nullptr, nullptr, &_impl->swapchain);
    DxUtils::Check << _impl->factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER);

    _pipeline = BePipeline::Create(*this);

    ComPtr<ID3D11Texture2D> backBuffer;
    DxUtils::Check
    << _impl->swapchain->GetBuffer(0, IID_PPV_ARGS(&backBuffer))
    << _impl->device->CreateRenderTargetView(backBuffer.Get(), nullptr, &_impl->backbufferTarget);

    D3D11_BUFFER_DESC uniformBufferDescriptor = {};
    uniformBufferDescriptor.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    uniformBufferDescriptor.Usage = D3D11_USAGE_DYNAMIC;
    uniformBufferDescriptor.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    uniformBufferDescriptor.ByteWidth = sizeof(BeUniformBufferGPU);
    DxUtils::Check << _impl->device->CreateBuffer(&uniformBufferDescriptor, nullptr, &_impl->uniformBuffer);

    D3D11_DEPTH_STENCIL_DESC depthStencilStateDescriptor = {};
    depthStencilStateDescriptor.DepthEnable = true;
    depthStencilStateDescriptor.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    depthStencilStateDescriptor.DepthFunc = D3D11_COMPARISON_LESS;
    depthStencilStateDescriptor.StencilEnable = false;
    DxUtils::Check << _impl->device->CreateDepthStencilState(&depthStencilStateDescriptor, _impl->defaultDepthStencilState.GetAddressOf());
    _impl->context->OMSetDepthStencilState(_impl->defaultDepthStencilState.Get(), 1);

    D3D11_RASTERIZER_DESC rasterDesc = {};
    rasterDesc.FillMode = D3D11_FILL_SOLID;
    rasterDesc.CullMode = D3D11_CULL_BACK;
    rasterDesc.FrontCounterClockwise = FALSE;
    rasterDesc.DepthClipEnable = TRUE;
    DxUtils::Check << _impl->device->CreateRasterizerState(&rasterDesc, _impl->rasterizerCullBack.GetAddressOf());

    rasterDesc.CullMode = D3D11_CULL_NONE;
    DxUtils::Check << _impl->device->CreateRasterizerState(&rasterDesc, _impl->rasterizerCullNone.GetAddressOf());

    _impl->context->RSSetState(_impl->rasterizerCullBack.Get());
}

auto BeRenderer::Render() -> void {
    DxUtils::DebugAnnotation frameAnnotation(_impl->context, "Frame");

    D3D11_VIEWPORT viewport;
    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    viewport.Width = static_cast<FLOAT>(_width);
    viewport.Height = static_cast<FLOAT>(_height);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    _impl->context->RSSetViewports(1, &viewport);

    const BeUniformBufferGPU uniformDataGpu(UniformData);
    D3D11_MAPPED_SUBRESOURCE uniformMappedResource;
    DxUtils::Check << _impl->context->Map(_impl->uniformBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &uniformMappedResource);
    memcpy(uniformMappedResource.pData, &uniformDataGpu, sizeof(BeUniformBufferGPU));
    _impl->context->Unmap(_impl->uniformBuffer.Get(), 0);
    _impl->context->VSSetConstantBuffers(0, 1, _impl->uniformBuffer.GetAddressOf());
    _impl->context->HSSetConstantBuffers(0, 1, _impl->uniformBuffer.GetAddressOf());
    _impl->context->DSSetConstantBuffers(0, 1, _impl->uniformBuffer.GetAddressOf());
    _impl->context->PSSetConstantBuffers(0, 1, _impl->uniformBuffer.GetAddressOf());

    for (const auto& pass : _passes) {
        DxUtils::DebugAnnotation passAnnotation(_impl->context, std::string(pass->GetPassName()));
        pass->Render();
    }

    ID3D11Buffer* emptyBuffers[1] = { nullptr };
    _impl->context->VSSetConstantBuffers(0, 1, emptyBuffers);
    _impl->context->HSSetConstantBuffers(0, 1, emptyBuffers);
    _impl->context->DSSetConstantBuffers(0, 1, emptyBuffers);
    _impl->context->PSSetConstantBuffers(0, 1, emptyBuffers);

    _pipeline->ClearCache();
    _impl->swapchain->Present(1, 0);
}
