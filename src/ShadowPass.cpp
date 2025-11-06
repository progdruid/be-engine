#include "ShadowPass.h"

#include "BeRenderer.h"
#include "scope_guard.hpp"

auto ShadowPass::Initialise() -> void {
    D3D11_BUFFER_DESC shadowBufferDescriptor = {};
    shadowBufferDescriptor.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    shadowBufferDescriptor.Usage = D3D11_USAGE_DYNAMIC;
    shadowBufferDescriptor.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    shadowBufferDescriptor.ByteWidth = sizeof(BeDirectionalLightShadowBufferGPU);
    Utils::Check << _renderer->GetDevice()->CreateBuffer(&shadowBufferDescriptor, nullptr, &_shadowPassConstantBuffer);
    
    _shadowShader = std::make_unique<BeShader>(
        _renderer->GetDevice().Get(),
        "assets/shaders/shadow",
        BeShaderType::Vertex,
        std::vector<BeVertexElementDescriptor> {
            {.Name = "Position", .Attribute = BeVertexElementDescriptor::BeVertexSemantic::Position},
        }
    );
}

auto ShadowPass::Render() -> void {
    const auto context = _renderer->GetContext();

    const auto* directionalLight = _renderer->GetContextDataPointer<BeDirectionalLight>(InputDirectionalLightName);
    
    UINT numViewports = 1;
    D3D11_VIEWPORT previousViewport = {};
    context->RSGetViewports(&numViewports,  &previousViewport);
    
    D3D11_VIEWPORT viewport = {};
    viewport.Width = directionalLight->ShadowMapResolution;
    viewport.Height = directionalLight->ShadowMapResolution;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    context->RSSetViewports(1, &viewport);
    SCOPE_EXIT { context->RSSetViewports(1, &previousViewport); };

    const auto directionalShadowMap = _renderer->GetRenderResource(directionalLight->ShadowMapTextureName);

    ID3D11RenderTargetView* nullRTV = nullptr;
    context->OMSetRenderTargets(1, &nullRTV, directionalShadowMap->DSV.Get());
    context->ClearDepthStencilView(directionalShadowMap->DSV.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
    SCOPE_EXIT { context->OMSetRenderTargets(0, nullptr, nullptr); };
    
    _shadowShader->Bind(context.Get());
    SCOPE_EXIT { context->VSSetShader(nullptr, nullptr, 0); };

    // Set vertex and index buffers
    uint32_t stride = sizeof(BeFullVertex);
    uint32_t offset = 0;
    context->IASetVertexBuffers(0, 1, _renderer->GetShaderVertexBuffer().GetAddressOf(), &stride, &offset);
    context->IASetIndexBuffer(_renderer->GetShaderIndexBuffer().Get(), DXGI_FORMAT_R32_UINT, 0);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    SCOPE_EXIT {
        ID3D11Buffer* nullBuffer = nullptr;
        context->IASetVertexBuffers(0, 1, &nullBuffer, &stride, &offset);
        context->IASetIndexBuffer(nullptr, DXGI_FORMAT_R32_UINT, 0);
    };
    
    const auto& objects = _renderer->GetObjects();
    for (const auto& object : objects) {
        glm::mat4x4 modelMatrix =
            glm::translate(glm::mat4(1.0f), object.Position) *
            glm::mat4_cast(object.Rotation) *
            glm::scale(glm::mat4(1.0f), object.Scale);
        
        const BeDirectionalLightShadowBufferGPU shadowPassDataGPU(*directionalLight, modelMatrix);
        D3D11_MAPPED_SUBRESOURCE mappedResource;
        Utils::Check << context->Map(_shadowPassConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
        memcpy(mappedResource.pData, &shadowPassDataGPU, sizeof(BeDirectionalLightShadowBufferGPU));
        context->Unmap(_shadowPassConstantBuffer.Get(), 0);
        context->VSSetConstantBuffers(1, 1, _shadowPassConstantBuffer.GetAddressOf());
        
        for (const auto& slice : object.DrawSlices) {
            context->DrawIndexed(slice.IndexCount, slice.StartIndexLocation, slice.BaseVertexLocation);
        }
    }
    ID3D11Buffer* nullBuffer = nullptr;
    context->VSSetConstantBuffers(1, 1, &nullBuffer);
}
