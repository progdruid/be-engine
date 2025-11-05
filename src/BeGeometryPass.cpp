#include "BeGeometryPass.h"

#include <gtc/type_ptr.inl>

#include "BeRenderer.h"
#include "Utils.h"

BeGeometryPass::BeGeometryPass() = default;
BeGeometryPass::~BeGeometryPass() = default;

auto BeGeometryPass::Initialise() -> void {
    //material buffer
    D3D11_BUFFER_DESC materialBufferDescriptor = {};
    materialBufferDescriptor.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    materialBufferDescriptor.Usage = D3D11_USAGE_DYNAMIC;
    materialBufferDescriptor.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    materialBufferDescriptor.ByteWidth = sizeof(MaterialBufferGPU);
    Utils::Check << _renderer->GetDevice()->CreateBuffer(&materialBufferDescriptor, nullptr, &_materialBuffer);

    //depth stencil state
    constexpr D3D11_DEPTH_STENCIL_DESC depthStencilStateDescriptor = {
        .DepthEnable = true,
        .DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL,
        .DepthFunc = D3D11_COMPARISON_LESS,
        .StencilEnable = false,
    };
    Utils::Check << _renderer->GetDevice()->CreateDepthStencilState(&depthStencilStateDescriptor, _depthStencilState.GetAddressOf());
    _renderer->GetContext()->OMSetDepthStencilState(_depthStencilState.Get(), 1);

    //
    _whiteFallbackTexture.CreateSRV(_renderer->GetDevice());
}

auto BeGeometryPass::Render() -> void {
    const auto context = _renderer->GetContext();

    const BeRenderResource* depthResource = _renderer->GetRenderResource(OutputDepthTextureName);
    const BeRenderResource* gbufferResource0 = _renderer->GetRenderResource(OutputTexture0Name);
    const BeRenderResource* gbufferResource1 = _renderer->GetRenderResource(OutputTexture1Name);
    const BeRenderResource* gbufferResource2 = _renderer->GetRenderResource(OutputTexture2Name);
    
    // Clear and set render targets
    context->ClearDepthStencilView(depthResource->DSV.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
    context->ClearRenderTargetView(gbufferResource0->RTV.Get(), glm::value_ptr(glm::vec4(0.0f)));
    context->ClearRenderTargetView(gbufferResource1->RTV.Get(), glm::value_ptr(glm::vec4(0.0f)));
    context->ClearRenderTargetView(gbufferResource2->RTV.Get(), glm::value_ptr(glm::vec4(0.0f)));

    ID3D11RenderTargetView* gbufferRTVs[3] = {
        gbufferResource0->RTV.Get(),
        gbufferResource1->RTV.Get(),
        gbufferResource2->RTV.Get()
    };
    context->OMSetRenderTargets(3, gbufferRTVs, depthResource->DSV.Get());

    // Set vertex and index buffers
    uint32_t stride = sizeof(BeFullVertex);
    uint32_t offset = 0;
    context->IASetVertexBuffers(0, 1, _renderer->GetShaderVertexBuffer().GetAddressOf(), &stride, &offset);
    context->IASetIndexBuffer(_renderer->GetShaderIndexBuffer().Get(), DXGI_FORMAT_R32_UINT, 0);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // Set default sampler - temporary,  should be overridden by materials if needed
    context->PSSetSamplers(0, 1, _renderer->GetPointSampler().GetAddressOf());

    // Draw all objects
    const auto& objects = _renderer->GetObjects();
    for (const auto& object : objects) {
        object.Shader->Bind(context.Get());
    
        for (const auto& slice : object.DrawSlices) {
        
            glm::mat4x4 modelMatrix =
                glm::translate(glm::mat4(1.0f), object.Position) *
                glm::mat4_cast(object.Rotation) *
                glm::scale(glm::mat4(1.0f), object.Scale);
            MaterialBufferGPU materialData(modelMatrix, slice.Material);
            D3D11_MAPPED_SUBRESOURCE materialMappedResource;
            Utils::Check << context->Map(_materialBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &materialMappedResource);
            memcpy(materialMappedResource.pData, &materialData, sizeof(MaterialBufferGPU));
            context->Unmap(_materialBuffer.Get(), 0);
            context->VSSetConstantBuffers(1, 1, _materialBuffer.GetAddressOf());
            context->PSSetConstantBuffers(1, 1, _materialBuffer.GetAddressOf());
        
            ID3D11ShaderResourceView* materialResources[2] = {
                slice.Material.DiffuseTexture ? slice.Material.DiffuseTexture->SRV.Get() : _whiteFallbackTexture.SRV.Get(),
                slice.Material.SpecularTexture ? slice.Material.SpecularTexture->SRV.Get() : _whiteFallbackTexture.SRV.Get(),
            };
            context->PSSetShaderResources(0, 2, materialResources);

            context->DrawIndexed(slice.IndexCount, slice.StartIndexLocation, slice.BaseVertexLocation);
        
            ID3D11ShaderResourceView* emptyResources[2] = { nullptr, nullptr };
            context->PSSetShaderResources(0, 2, emptyResources);
        }
    }
    { 
        ID3D11ShaderResourceView* emptyResources[2] = { nullptr, nullptr };
        context->PSSetShaderResources(0, 2, emptyResources); // clean material textures
        ID3D11Buffer* emptyBuffers[1] = { nullptr };
        context->VSSetConstantBuffers(1, 1, emptyBuffers); // clean material buffer
        ID3D11SamplerState* emptySamplers[1] = { nullptr };
        context->PSSetSamplers(0, 1, emptySamplers); // clean samplers
        ID3D11RenderTargetView* emptyTargets[3] = { nullptr, nullptr, nullptr };
        context->OMSetRenderTargets(3, emptyTargets, nullptr); // clean render targets
    }
}
