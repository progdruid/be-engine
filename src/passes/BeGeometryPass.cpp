#include "BeGeometryPass.h"

#include <cassert>
#include <scope_guard/scope_guard.hpp>
#include <umbrellas/include-glm.h>

#include "BeRenderer.h"
#include "Utils.h"

BeGeometryPass::BeGeometryPass() = default;
BeGeometryPass::~BeGeometryPass() = default;

auto BeGeometryPass::Initialise() -> void {
    //material buffer
    D3D11_BUFFER_DESC objectBufferDescriptor = {};
    objectBufferDescriptor.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    objectBufferDescriptor.Usage = D3D11_USAGE_DYNAMIC;
    objectBufferDescriptor.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    objectBufferDescriptor.ByteWidth = sizeof(BeObjectBufferGPU);
    Utils::Check << _renderer->GetDevice()->CreateBuffer(&objectBufferDescriptor, nullptr, &_objectBuffer);
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
    SCOPE_EXIT {
        ID3D11RenderTargetView* emptyTargets[3] = { nullptr, nullptr, nullptr };
        context->OMSetRenderTargets(3, emptyTargets, nullptr);
    };

    // Set vertex and index buffers
    uint32_t stride = sizeof(BeFullVertex);
    uint32_t offset = 0;
    context->IASetVertexBuffers(0, 1, _renderer->GetShaderVertexBuffer().GetAddressOf(), &stride, &offset);
    context->IASetIndexBuffer(_renderer->GetShaderIndexBuffer().Get(), DXGI_FORMAT_R32_UINT, 0);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    SCOPE_EXIT {
        ID3D11Buffer* emptyBuffers[1] = { nullptr };
        context->IASetVertexBuffers(0, 1, emptyBuffers, &stride, &offset);
        context->IASetIndexBuffer(nullptr, DXGI_FORMAT_R32_UINT, 0);
    };

    // Set default sampler - temporary,  should be overridden by materials if needed
    context->PSSetSamplers(0, 1, _renderer->GetPointSampler().GetAddressOf());
    SCOPE_EXIT {
        ID3D11SamplerState* emptySamplers[1] = { nullptr };
        context->PSSetSamplers(0, 1, emptySamplers);
    };

    // Draw all objects
    const auto& objects = _renderer->GetObjects();
    for (const auto& object : objects) {
        const auto shader = object.Model->Shader;
        assert(shader);
        shader->Bind(context.Get());

        glm::mat4x4 modelMatrix =
            glm::translate(glm::mat4(1.0f), object.Position) *
            glm::mat4_cast(object.Rotation) *
            glm::scale(glm::mat4(1.0f), object.Scale);
        
        BeObjectBufferGPU objectData(modelMatrix);
        D3D11_MAPPED_SUBRESOURCE objectMappedResource;
        Utils::Check << context->Map(_objectBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &objectMappedResource);
        memcpy(objectMappedResource.pData, &objectData, sizeof(BeObjectBufferGPU));
        context->Unmap(_objectBuffer.Get(), 0);
        context->VSSetConstantBuffers(1, 1, _objectBuffer.GetAddressOf());
        context->PSSetConstantBuffers(1, 1, _objectBuffer.GetAddressOf());
        
        for (const auto& slice : object.DrawSlices) {
            slice.Material->UpdateGPUBuffers(context);
            const auto& materialBuffer = slice.Material->GetBuffer();
            context->VSSetConstantBuffers(2, 1, materialBuffer.GetAddressOf());
            context->PSSetConstantBuffers(2, 1, materialBuffer.GetAddressOf());

            const auto& textureSlots = slice.Material->GetTexturePairs();
            for (const auto& [texture, slot] : textureSlots | std::views::values) {
                context->PSSetShaderResources(slot, 1, texture->SRV.GetAddressOf());
            } 

            context->DrawIndexed(slice.IndexCount, slice.StartIndexLocation, slice.BaseVertexLocation);
        
            context->PSSetShaderResources(0, 2, Utils::NullSRVs);
        }
    }
    context->PSSetShaderResources(0, 2, Utils::NullSRVs); // clean material textures
    context->VSSetConstantBuffers(1, 2, Utils::NullBuffers); // clean object and material buffers
}
