#include "BeRenderer.h"

#include <cassert>
#include <scope_guard/scope_guard.hpp>
#include <dxgi1_6.h>

#include "BeModel.h"
#include "BePipeline.h"
#include "BeRenderPass.h"
#include "BeShader.h"
#include "Utils.h"

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

    _pipeline = BePipeline::Create(_context);
    
    ComPtr<ID3D11Texture2D> backBuffer;
    Utils::Check
    << _swapchain->GetBuffer(0, IID_PPV_ARGS(&backBuffer))
    << _device->CreateRenderTargetView(backBuffer.Get(), nullptr, &_backbufferTarget);
    
    D3D11_BUFFER_DESC uniformBufferDescriptor = {};
    uniformBufferDescriptor.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    uniformBufferDescriptor.Usage = D3D11_USAGE_DYNAMIC;
    uniformBufferDescriptor.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    uniformBufferDescriptor.ByteWidth = sizeof(BeUniformBufferGPU);
    Utils::Check << _device->CreateBuffer(&uniformBufferDescriptor, nullptr, &_uniformBuffer);
    
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

    // Set viewport
    D3D11_VIEWPORT viewport;
    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    viewport.Width = static_cast<FLOAT>(_width);
    viewport.Height = static_cast<FLOAT>(_height);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    _context->RSSetViewports(1, &viewport);


    const BeUniformBufferGPU uniformDataGpu(UniformData);
    D3D11_MAPPED_SUBRESOURCE uniformMappedResource;
    Utils::Check << _context->Map(_uniformBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &uniformMappedResource);
    memcpy(uniformMappedResource.pData, &uniformDataGpu, sizeof(BeUniformBufferGPU));
    _context->Unmap(_uniformBuffer.Get(), 0);
    _context->VSSetConstantBuffers(0, 1, _uniformBuffer.GetAddressOf());
    _context->HSSetConstantBuffers(0, 1, _uniformBuffer.GetAddressOf());
    _context->DSSetConstantBuffers(0, 1, _uniformBuffer.GetAddressOf());
    _context->PSSetConstantBuffers(0, 1, _uniformBuffer.GetAddressOf());

    for (const auto& pass : _passes) {
        Utils::BeDebugAnnotation passAnnotation(_context, std::string(pass->GetPassName()));
        pass->Render();
    }

    ID3D11Buffer* emptyBuffers[1] = { nullptr };
    _context->VSSetConstantBuffers(0, 1, emptyBuffers);
    _context->HSSetConstantBuffers(0, 1, emptyBuffers);
    _context->DSSetConstantBuffers(0, 1, emptyBuffers);
    _context->PSSetConstantBuffers(0, 1, emptyBuffers);

    _pipeline->ClearCache();
    _drawEntries.clear();
    
    _swapchain->Present(1, 0);
}

auto BeRenderer::SetModels(const std::vector<std::shared_ptr<BeModel>>& models) -> void {
    //vbo + ibo
    size_t totalVerticesNumber = 0;
    size_t totalIndicesNumber = 0;
    size_t totalDrawSlices = 0;
    for (const auto& model : models) {
        totalVerticesNumber += model->FullVertices.size();
        totalIndicesNumber += model->Indices.size();
        totalDrawSlices += model->DrawSlices.size();
    }

    std::vector<BeFullVertex> fullVertices;
    std::vector<uint32_t> indices;
    fullVertices.reserve(totalVerticesNumber);
    indices.reserve(totalIndicesNumber);
    for (auto& model : models) {
        fullVertices.insert(fullVertices.end(), model->FullVertices.begin(), model->FullVertices.end());
        indices.insert(indices.end(), model->Indices.begin(), model->Indices.end());

        auto & drawSlices = _modelDrawSlices[model.get()];
        
        for (auto slice : model->DrawSlices) {
            slice.BaseVertexLocation += static_cast<int32_t>(fullVertices.size() - model->FullVertices.size());
            slice.StartIndexLocation += static_cast<uint32_t>(indices.size() - model->Indices.size());
            
            drawSlices.push_back(slice);
        }
    }
    
    D3D11_BUFFER_DESC vertexBufferDescriptor = {};
    vertexBufferDescriptor.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vertexBufferDescriptor.Usage = D3D11_USAGE_DEFAULT;
    vertexBufferDescriptor.ByteWidth = static_cast<UINT>(fullVertices.size() * sizeof(BeFullVertex));
    D3D11_SUBRESOURCE_DATA vertexData = {};
    vertexData.pSysMem = fullVertices.data();
    Utils::Check << _device->CreateBuffer(&vertexBufferDescriptor, &vertexData, &_sharedVertexBuffer);
    
    D3D11_BUFFER_DESC indexBufferDescriptor = {};
    indexBufferDescriptor.BindFlags = D3D11_BIND_INDEX_BUFFER;
    indexBufferDescriptor.Usage = D3D11_USAGE_DEFAULT;
    indexBufferDescriptor.ByteWidth = static_cast<UINT>(indices.size() * sizeof(uint32_t));
    D3D11_SUBRESOURCE_DATA indexData = {};
    indexData.pSysMem = indices.data();
    Utils::Check << _device->CreateBuffer(&indexBufferDescriptor, &indexData, &_sharedIndexBuffer);
}

auto BeRenderer::RegisterModels(const std::vector<std::shared_ptr<BeModel>>& models) -> void {
    _registeredModels.insert(_registeredModels.end(), models.begin(), models.end());
}

auto BeRenderer::BakeModels() -> void {
    // This forces RegisterModels to be compiled
    if (false) {
        std::vector<std::shared_ptr<BeModel>> temp;
        RegisterModels(temp);
    }
    SetModels(_registeredModels);
}

