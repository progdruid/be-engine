#include "BeRenderer.h"

#include <d3dcompiler.h>
#include <glm.hpp>
#include <gtc/quaternion.hpp>
#include <gtc/type_ptr.hpp>

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

    
    // Create RTV
    {
        ComPtr<ID3D11Texture2D> backBuffer;
        Utils::Check
        << _swapchain->GetBuffer(0, IID_PPV_ARGS(&backBuffer))
        << _device->CreateRenderTargetView(backBuffer.Get(), nullptr, &_backbufferTarget);
    
        D3D11_TEXTURE2D_DESC depthStencilDescriptor = {};
        depthStencilDescriptor.Width = _width;
        depthStencilDescriptor.Height = _height;
        depthStencilDescriptor.MipLevels = 1;
        depthStencilDescriptor.ArraySize = 1;
        depthStencilDescriptor.Format = DXGI_FORMAT_R24G8_TYPELESS; // 24-bit depth, 8-bit stencil
        depthStencilDescriptor.SampleDesc.Count = 1;
        depthStencilDescriptor.SampleDesc.Quality = 0;
        depthStencilDescriptor.Usage = D3D11_USAGE_DEFAULT;
        depthStencilDescriptor.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
        depthStencilDescriptor.CPUAccessFlags = 0;
        depthStencilDescriptor.MiscFlags = 0;
        
        D3D11_SHADER_RESOURCE_VIEW_DESC depthSRVDescriptor = {};
        depthSRVDescriptor.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS; // depth in R
        depthSRVDescriptor.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        depthSRVDescriptor.Texture2D.MostDetailedMip = 0;
        depthSRVDescriptor.Texture2D.MipLevels = 1;
        
        D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
        dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
        dsvDesc.Texture2D.MipSlice = 0;
        
        ComPtr<ID3D11Texture2D> depthStencilBuffer;
        //Utils::Check
        Utils::Check << _device->CreateTexture2D(&depthStencilDescriptor,nullptr, depthStencilBuffer.GetAddressOf());
        Utils::Check << _device->CreateShaderResourceView(depthStencilBuffer.Get(), &depthSRVDescriptor, _depthStencilResource.GetAddressOf());
        Utils::Check << _device->CreateDepthStencilView(depthStencilBuffer.Get(), &dsvDesc, _depthStencilView.GetAddressOf());

        D3D11_DEPTH_STENCIL_DESC depthStencilStateDescriptor = {
            .DepthEnable = true,
            .DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL,
            .DepthFunc = D3D11_COMPARISON_LESS,
            .StencilEnable = false,
        };
        
        Utils::Check << _device->CreateDepthStencilState(&depthStencilStateDescriptor, _depthStencilState.GetAddressOf());
        _context->OMSetDepthStencilState(_depthStencilState.Get(), 1);


        constexpr DXGI_FORMAT formats[3] = {
            DXGI_FORMAT_R8G8B8A8_UNORM,     // rgb: diffuse, a: unused
            DXGI_FORMAT_R16G16B16A16_FLOAT, // rgb: normal, a: unused
            DXGI_FORMAT_R8G8B8A8_UNORM,     // rgb: specular, a: shininess
        };
        

        D3D11_TEXTURE2D_DESC textureDesc = {};
        textureDesc.Width = _width;
        textureDesc.Height = _height;
        textureDesc.MipLevels = 1;
        textureDesc.ArraySize = 1;
        textureDesc.SampleDesc.Count = 1;
        textureDesc.SampleDesc.Quality = 0;
        textureDesc.Usage = D3D11_USAGE_DEFAULT;
        textureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
        textureDesc.CPUAccessFlags = 0;
        textureDesc.MiscFlags = 0;
        
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;

        for (int i = 0; i < 3; ++i) {
            textureDesc.Format = formats[i];
            srvDesc.Format = formats[i];
            ID3D11Texture2D* texture = nullptr;
            Utils::Check
            << _device->CreateTexture2D(&textureDesc, nullptr, &texture)
            << _device->CreateRenderTargetView(texture, nullptr, &_gbufferTargets[i])
            << _device->CreateShaderResourceView(texture, &srvDesc, &_gbufferResources[i]);
        }

        D3D11_TEXTURE2D_DESC lightingTextureDesc = {};
        lightingTextureDesc.Width = _width;
        lightingTextureDesc.Height = _height;
        lightingTextureDesc.MipLevels = 1;
        lightingTextureDesc.ArraySize = 1;
        lightingTextureDesc.Format = DXGI_FORMAT_R11G11B10_FLOAT;
        lightingTextureDesc.SampleDesc.Count = 1;
        lightingTextureDesc.SampleDesc.Quality = 0;
        lightingTextureDesc.Usage = D3D11_USAGE_DEFAULT;
        lightingTextureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

        ID3D11Texture2D* lightingTexture = nullptr;
        Utils::Check
        << _device->CreateTexture2D(&lightingTextureDesc, nullptr, &lightingTexture)
        << _device->CreateRenderTargetView(lightingTexture, nullptr, &_lightingTarget)
        << _device->CreateShaderResourceView(lightingTexture, nullptr, &_lightingResource);

        // Additive blending for lights
        D3D11_BLEND_DESC lightingBlendDesc = {};
        lightingBlendDesc.RenderTarget[0].BlendEnable = TRUE;
        lightingBlendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
        lightingBlendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
        lightingBlendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
        lightingBlendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
        lightingBlendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
        lightingBlendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        lightingBlendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        
        Utils::Check << _device->CreateBlendState(&lightingBlendDesc, _lightingBlendState.GetAddressOf());
    }
    
    D3D11_BUFFER_DESC uniformBufferDescriptor = {};
    uniformBufferDescriptor.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    uniformBufferDescriptor.Usage = D3D11_USAGE_DYNAMIC;
    uniformBufferDescriptor.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    uniformBufferDescriptor.ByteWidth = sizeof(UniformBufferGPU);
    Utils::Check << _device->CreateBuffer(&uniformBufferDescriptor, nullptr, &_uniformBuffer);
    
    D3D11_BUFFER_DESC materialBufferDescriptor = {};
    materialBufferDescriptor.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    materialBufferDescriptor.Usage = D3D11_USAGE_DYNAMIC;
    materialBufferDescriptor.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    materialBufferDescriptor.ByteWidth = sizeof(MaterialBufferGPU);
    Utils::Check << _device->CreateBuffer(&materialBufferDescriptor, nullptr, &_materialBuffer);

    D3D11_BUFFER_DESC directionalLightBufferDescriptor = {};
    directionalLightBufferDescriptor.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    directionalLightBufferDescriptor.Usage = D3D11_USAGE_DYNAMIC;
    directionalLightBufferDescriptor.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    directionalLightBufferDescriptor.ByteWidth = sizeof(DirectionalLightBufferGPU);
    Utils::Check << _device->CreateBuffer(&directionalLightBufferDescriptor, nullptr, &_directionalLightBuffer);
    
    D3D11_BUFFER_DESC pointLightBufferDescriptor = {};
    pointLightBufferDescriptor.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    pointLightBufferDescriptor.Usage = D3D11_USAGE_DYNAMIC;
    pointLightBufferDescriptor.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    pointLightBufferDescriptor.ByteWidth = sizeof(PointLightBufferGPU);
    Utils::Check << _device->CreateBuffer(&pointLightBufferDescriptor, nullptr, &_pointLightBuffer);
    
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

    _directionalLightShader = std::make_unique<BeShader>(
        _device.Get(),
        "assets/shaders/directionalLight",
        std::vector<BeVertexElementDescriptor>{}
    );

    _pointLightShader = std::make_unique<BeShader>(
        _device.Get(),
        "assets/shaders/pointLight",
        std::vector<BeVertexElementDescriptor>{}
    );
    
    _whiteFallbackTexture.CreateSRV(_device);
}

auto BeRenderer::PushObjects(const std::vector<ObjectEntry>& objects) -> void {
    _objects = objects;
    
    size_t totalVerticesNumber = 0;
    size_t totalIndicesNumber = 0;
    size_t totalDrawSlices = 0;
    for (const auto& object : _objects) {
        totalVerticesNumber += object.Model->FullVertices.size();
        totalIndicesNumber += object.Model->Indices.size();
        totalDrawSlices += object.Model->DrawSlices.size();
    }
    
    std::vector<BeFullVertex> fullVertices;
    std::vector<uint32_t> indices;
    fullVertices.reserve(totalVerticesNumber);
    indices.reserve(totalIndicesNumber);
    for (auto& object : _objects) {
        fullVertices.insert(fullVertices.end(), object.Model->FullVertices.begin(), object.Model->FullVertices.end());
        indices.insert(indices.end(), object.Model->Indices.begin(), object.Model->Indices.end());
        for (BeModel::BeDrawSlice slice : object.Model->DrawSlices) {
            slice.BaseVertexLocation += static_cast<int32_t>(fullVertices.size() - object.Model->FullVertices.size());
            slice.StartIndexLocation += static_cast<uint32_t>(indices.size() - object.Model->Indices.size());
            object.DrawSlices.push_back(slice);
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

auto BeRenderer::Render() -> void {

    // --------- Setup phase ---------
    
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
    UniformBufferGPU uniformDataGpu(UniformData);
    D3D11_MAPPED_SUBRESOURCE uniformMappedResource;
    Utils::Check << _context->Map(_uniformBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &uniformMappedResource);
    memcpy(uniformMappedResource.pData, &uniformDataGpu, sizeof(UniformBufferGPU));
    _context->Unmap(_uniformBuffer.Get(), 0);
    _context->VSSetConstantBuffers(0, 1, _uniformBuffer.GetAddressOf());
    _context->PSSetConstantBuffers(0, 1, _uniformBuffer.GetAddressOf());

    
    
    // ---------- Geometry pass ----------
    
    // Clear and set render targets
    for (const auto& _gbufferRTV : _gbufferTargets) {
        _context->ClearRenderTargetView(_gbufferRTV, glm::value_ptr(glm::vec4(0.0f)));
    }
    _context->ClearDepthStencilView(_depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
    _context->OMSetRenderTargets(3, _gbufferTargets, _depthStencilView.Get());
    
    // Set vertex and index buffers
    uint32_t stride = sizeof(BeFullVertex);
    uint32_t offset = 0;
    _context->IASetVertexBuffers(0, 1, _sharedVertexBuffer.GetAddressOf(), &stride, &offset);
    _context->IASetIndexBuffer(_sharedIndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
    _context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // Set default sampler - temporary,  should be overridden by materials if needed
    _context->PSSetSamplers(0, 1, _pointSampler.GetAddressOf());

    // Draw all objects
    for (const auto& object : _objects) {
        object.Shader->Bind(_context.Get());
        
        for (const auto& slice : object.DrawSlices) {
            
            glm::mat4x4 modelMatrix =
                glm::translate(glm::mat4(1.0f), object.Position) *
                glm::mat4_cast(object.Rotation) *
                glm::scale(glm::mat4(1.0f), object.Scale);
            MaterialBufferGPU materialData(modelMatrix, slice.Material);
            D3D11_MAPPED_SUBRESOURCE materialMappedResource;
            Utils::Check << _context->Map(_materialBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &materialMappedResource);
            memcpy(materialMappedResource.pData, &materialData, sizeof(MaterialBufferGPU));
            _context->Unmap(_materialBuffer.Get(), 0);
            _context->VSSetConstantBuffers(1, 1, _materialBuffer.GetAddressOf());
            _context->PSSetConstantBuffers(1, 1, _materialBuffer.GetAddressOf());
            
            ID3D11ShaderResourceView* materialResources[2] = {
                slice.Material.DiffuseTexture ? slice.Material.DiffuseTexture->SRV.Get() : _whiteFallbackTexture.SRV.Get(),
                slice.Material.SpecularTexture ? slice.Material.SpecularTexture->SRV.Get() : _whiteFallbackTexture.SRV.Get(),
            };
            _context->PSSetShaderResources(0, 2, materialResources);

            _context->DrawIndexed(slice.IndexCount, slice.StartIndexLocation, slice.BaseVertexLocation);
            
            ID3D11ShaderResourceView* emptyResources[2] = { nullptr, nullptr };
            _context->PSSetShaderResources(0, 2, emptyResources);
        }
    }
    { 
        ID3D11ShaderResourceView* emptyResources[2] = { nullptr, nullptr };
        _context->PSSetShaderResources(0, 2, emptyResources); // clean material textures
        ID3D11Buffer* emptyBuffers[1] = { nullptr };
        _context->VSSetConstantBuffers(1, 1, emptyBuffers); // clean material buffer
        ID3D11SamplerState* emptySamplers[1] = { nullptr };
        _context->PSSetSamplers(0, 1, emptySamplers); // clean samplers
        ID3D11RenderTargetView* emptyTargets[3] = { nullptr, nullptr, nullptr };
        _context->OMSetRenderTargets(3, emptyTargets, nullptr); // clean render targets
    }
    // --------- End of pass ---------
    
    
    // --------- Lighting pass ---------
    _context->ClearRenderTargetView(_lightingTarget.Get(), glm::value_ptr(glm::vec4(0.0f)));
    _context->OMSetRenderTargets(1, _lightingTarget.GetAddressOf(), nullptr);
    _context->OMSetBlendState(_lightingBlendState.Get(), nullptr, 0xFFFFFFFF);
    
    _context->VSSetShader(_fullscreenShader->VertexShader.Get(), nullptr, 0);
    
    _context->PSSetShaderResources(0, 1, _depthStencilResource.GetAddressOf());
    _context->PSSetShaderResources(1, 3, _gbufferResources);
    
    _context->PSSetSamplers(0, 1, _pointSampler.GetAddressOf());

    {
        DirectionalLightBufferGPU directionalLightBuffer(DirectionalLightData);
        D3D11_MAPPED_SUBRESOURCE directionalLightMappedResource;
        Utils::Check << _context->Map(_directionalLightBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &directionalLightMappedResource);
        memcpy(directionalLightMappedResource.pData, &directionalLightBuffer, sizeof(DirectionalLightBufferGPU));
        _context->Unmap(_directionalLightBuffer.Get(), 0);
        _context->PSSetConstantBuffers(1, 1, _directionalLightBuffer.GetAddressOf());
    
        _context->PSSetShader(_directionalLightShader->PixelShader.Get(), nullptr, 0);
    
        _context->IASetInputLayout(nullptr);
        _context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
        _context->Draw(4, 0);
    }

    {
        PointLightBufferGPU pointLightBuffer(PointLightData);
        D3D11_MAPPED_SUBRESOURCE pointLightMappedResource;
        Utils::Check << _context->Map(_pointLightBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &pointLightMappedResource);
        memcpy(pointLightMappedResource.pData, &pointLightBuffer, sizeof(PointLightBufferGPU));
        _context->Unmap(_pointLightBuffer.Get(), 0);
        _context->PSSetConstantBuffers(1, 1, _pointLightBuffer.GetAddressOf());
    
        _context->PSSetShader(_pointLightShader->PixelShader.Get(), nullptr, 0);
    
        _context->IASetInputLayout(nullptr);
        _context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
        _context->Draw(4, 0);
    }
    
    {
        ID3D11ShaderResourceView* emptyResources[4] = { nullptr, nullptr, nullptr, nullptr };
        _context->PSSetShaderResources(0, 4, emptyResources);
        ID3D11Buffer* emptyBuffers[1] = { nullptr };
        _context->PSSetConstantBuffers(1, 1, emptyBuffers);
        ID3D11SamplerState* emptySamplers[1] = { nullptr };
        _context->PSSetSamplers(0, 1, emptySamplers);
        ID3D11RenderTargetView* emptyTargets[1] = { nullptr };
        _context->OMSetRenderTargets(1, emptyTargets, nullptr);
        _context->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
    }
    // --------- End of pass ---------

    
    // --------- Fullscreen pass ---------
    auto fullClearColor = glm::vec4(ClearColor, 1.0f);
    _context->ClearRenderTargetView(_backbufferTarget.Get(), reinterpret_cast<FLOAT*>(&fullClearColor));
    _context->OMSetRenderTargets(1, _backbufferTarget.GetAddressOf(), nullptr);

    _context->PSSetShaderResources(0, 1, _depthStencilResource.GetAddressOf());
    _context->PSSetShaderResources(1, 3, _gbufferResources);
    _context->PSSetShaderResources(4, 1, _lightingResource.GetAddressOf());

    _context->PSSetSamplers(0, 1, _pointSampler.GetAddressOf());
    
    _context->VSSetShader(_fullscreenShader->VertexShader.Get(), nullptr, 0);
    _context->PSSetShader(_fullscreenShader->PixelShader.Get(), nullptr, 0);

    _context->IASetInputLayout(nullptr);
    _context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    _context->Draw(4, 0);
    { 
        ID3D11ShaderResourceView* emptyResources[] = { nullptr, nullptr, nullptr, nullptr };
        _context->PSSetShaderResources(0, 4, emptyResources);
        ID3D11SamplerState* emptySamplers[] = { nullptr };
        _context->PSSetSamplers(0, 1, emptySamplers);
        ID3D11RenderTargetView* emptyTargets[] = { nullptr };
        _context->OMSetRenderTargets(1, emptyTargets, nullptr);
    }
    // --------- End of pass ---------

    ID3D11Buffer* emptyBuffers[1] = { nullptr };
    _context->VSSetConstantBuffers(1, 1, emptyBuffers);
    _context->PSSetConstantBuffers(1, 1, emptyBuffers);
    
    _swapchain->Present(1, 0);
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
