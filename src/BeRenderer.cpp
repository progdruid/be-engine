#include "BeRenderer.h"
#include "BeRenderPass.h"
#include "BeShader.h"
#include "Utils.h"

BeRenderer::BeRenderer(const HWND windowHandle, const uint32_t width, const uint32_t height) {
    _windowHandle = windowHandle;
    _width = width;
    _height = height;
}

auto BeRenderer::LaunchDevice() -> void {

    UINT deviceFlags = 0;
#if defined(_DEBUG)
    deviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    
    // Device and context
    Utils::Check << D3D11CreateDevice(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        deviceFlags,
        nullptr,
        0,
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

    Utils::Check << _factory->CreateSwapChainForHwnd(_device.Get(), _windowHandle, &scDesc, nullptr, nullptr, &_swapchain);
    Utils::Check << _factory->MakeWindowAssociation(_windowHandle, DXGI_MWA_NO_ALT_ENTER);

    
    ComPtr<ID3D11Texture2D> backBuffer;
    Utils::Check
    << _swapchain->GetBuffer(0, IID_PPV_ARGS(&backBuffer))
    << _device->CreateRenderTargetView(backBuffer.Get(), nullptr, &_backbufferTarget);
    
    D3D11_BUFFER_DESC uniformBufferDescriptor = {};
    uniformBufferDescriptor.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    uniformBufferDescriptor.Usage = D3D11_USAGE_DYNAMIC;
    uniformBufferDescriptor.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    uniformBufferDescriptor.ByteWidth = sizeof(UniformBufferGPU);
    Utils::Check << _device->CreateBuffer(&uniformBufferDescriptor, nullptr, &_uniformBuffer);
    
    // Create point sampler state
    D3D11_SAMPLER_DESC samplerDesc = {};
    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    samplerDesc.MinLOD = 0;
    samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
    Utils::Check << _device->CreateSamplerState(&samplerDesc, &_pointSampler);

    _fullscreenShader = std::make_unique<BeShader>(
        _device.Get(),
        "assets/shaders/fullscreen",
        std::vector<BeVertexElementDescriptor>{}
    );
}

auto BeRenderer::AddRenderPass(BeRenderPass* renderPass) -> void {
    _passes.push_back(renderPass);
    renderPass->InjectRenderer(this);
}

auto BeRenderer::InitialisePasses() -> void {
    for (const auto& pass : _passes)
        pass->Initialise();
}

auto BeRenderer::Render() -> void {
    // Set viewport
    D3D11_VIEWPORT viewport;
    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    viewport.Width = static_cast<FLOAT>(_width);
    viewport.Height = static_cast<FLOAT>(_height);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    _context->RSSetViewports(1, &viewport);

    
    // Update uniform constant buffer
    const UniformBufferGPU uniformDataGpu(UniformData);
    D3D11_MAPPED_SUBRESOURCE uniformMappedResource;
    Utils::Check << _context->Map(_uniformBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &uniformMappedResource);
    memcpy(uniformMappedResource.pData, &uniformDataGpu, sizeof(UniformBufferGPU));
    _context->Unmap(_uniformBuffer.Get(), 0);
    _context->VSSetConstantBuffers(0, 1, _uniformBuffer.GetAddressOf());
    _context->PSSetConstantBuffers(0, 1, _uniformBuffer.GetAddressOf());

    for (const auto& pass : _passes)
        pass->Render();
    
    ID3D11Buffer* emptyBuffers[1] = { nullptr };
    _context->VSSetConstantBuffers(1, 1, emptyBuffers);
    _context->PSSetConstantBuffers(1, 1, emptyBuffers);
    
    _swapchain->Present(1, 0);
}

auto BeRenderer::CreateRenderResource(
    const std::string& name,
    const bool useWindowSize,
    const BeRenderResource::BeResourceDescriptor& desc)
-> BeRenderResource* {

    BeRenderResource::BeResourceDescriptor descCopy = desc;
    if (useWindowSize) {
        descCopy.Width = _width;
        descCopy.Height = _height;
    }
    
    const auto resource = std::make_unique<BeRenderResource>(name, desc);
    resource->CreateGPUResources(_device);
    BeRenderResource* resourcePtr = resource.get();
    _renderResources.emplace(name, std::move(*resource));
    return resourcePtr;
}

auto BeRenderer::GetRenderResource(const std::string& name) -> BeRenderResource* {
    return &_renderResources.at(name);
}

auto BeRenderer::TerminateRenderer() -> void {
    _backbufferTarget.Reset();
    _swapchain.Reset();
    _factory.Reset();
    _adapter.Reset();
    _dxgiDevice.Reset();
    _context.Reset();
    _device.Reset();
}
