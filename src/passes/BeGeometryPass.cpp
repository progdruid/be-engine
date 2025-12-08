#include "BeGeometryPass.h"

#include <cassert>
#include <scope_guard/scope_guard.hpp>
#include <umbrellas/include-glm.h>

#include "BeAssetRegistry.h"
#include "BeRenderer.h"
#include "BeTexture.h"
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
    const auto registry = _renderer->GetAssetRegistry().lock();

    const auto depthResource    = registry->GetTexture(OutputDepthTextureName).lock();
    const auto gbufferResource0 = registry->GetTexture(OutputTexture0Name).lock();
    const auto gbufferResource1 = registry->GetTexture(OutputTexture1Name).lock();
    const auto gbufferResource2 = registry->GetTexture(OutputTexture2Name).lock();
    
    // Clear and set render targets
    context->ClearDepthStencilView(depthResource->GetDSV().Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
    context->ClearRenderTargetView(gbufferResource0->GetRTV().Get(), glm::value_ptr(glm::vec4(0.0f)));
    context->ClearRenderTargetView(gbufferResource1->GetRTV().Get(), glm::value_ptr(glm::vec4(0.0f)));
    context->ClearRenderTargetView(gbufferResource2->GetRTV().Get(), glm::value_ptr(glm::vec4(0.0f)));

    ID3D11RenderTargetView* gbufferRTVs[3] = {
        gbufferResource0->GetRTV().Get(),
        gbufferResource1->GetRTV().Get(),
        gbufferResource2->GetRTV().Get()
    };
    context->OMSetRenderTargets(3, gbufferRTVs, depthResource->GetDSV().Get());
    SCOPE_EXIT { context->OMSetRenderTargets(3, Utils::NullRTVs, nullptr); };

    // Set vertex and index buffers
    uint32_t stride = sizeof(BeFullVertex);
    uint32_t offset = 0;
    context->IASetVertexBuffers(0, 1, _renderer->GetShaderVertexBuffer().GetAddressOf(), &stride, &offset);
    context->IASetIndexBuffer(_renderer->GetShaderIndexBuffer().Get(), DXGI_FORMAT_R32_UINT, 0);
    SCOPE_EXIT {
        context->IASetVertexBuffers(0, 1, Utils::NullBuffers, &stride, &offset);
        context->IASetIndexBuffer(nullptr, DXGI_FORMAT_R32_UINT, 0);
    };

    // Set the default sampler - temporary. It should be overriden by materials if needed
    context->PSSetSamplers(0, 1, _renderer->GetPointSampler().GetAddressOf());
    SCOPE_EXIT { context->PSSetSamplers(0, 1, Utils::NullSamplers); };

    // Draw all objects
    const auto& objects = _renderer->GetObjects();
    for (const auto& object : objects) {
        const auto shader = object.Model->Shader;
        assert(shader);
        shader->Bind(context.Get(), BeShaderType::All);
        SCOPE_EXIT { BeShader::Unbind(context.Get(), BeShaderType::All); };

        // Set primitive topology based on whether tessellation is enabled
        const D3D11_PRIMITIVE_TOPOLOGY topology = HasAny(shader->GetShaderType(), BeShaderType::Tesselation)
            ? D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST
            : D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        context->IASetPrimitiveTopology(topology);

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
        if (HasAny(shader->GetShaderType(), BeShaderType::Tesselation)) {
            context->HSSetConstantBuffers(1, 1, _objectBuffer.GetAddressOf());
            context->DSSetConstantBuffers(1, 1, _objectBuffer.GetAddressOf());
        }
        
        for (const auto& slice : object.DrawSlices) {
            slice.Material->UpdateGPUBuffers(context);
            const auto& materialBuffer = slice.Material->GetBuffer();
            context->VSSetConstantBuffers(2, 1, materialBuffer.GetAddressOf());
            context->PSSetConstantBuffers(2, 1, materialBuffer.GetAddressOf());
            if (HasAny(shader->GetShaderType(), BeShaderType::Tesselation)) {
                context->HSSetConstantBuffers(2, 1, materialBuffer.GetAddressOf());
                context->DSSetConstantBuffers(2, 1, materialBuffer.GetAddressOf());
            }

            const auto& textureSlots = slice.Material->GetTexturePairs();
            for (const auto& [texture, slot] : textureSlots | std::views::values) {
                context->PSSetShaderResources(slot, 1, texture->GetSRV().GetAddressOf());
            } 

            context->DrawIndexed(slice.IndexCount, slice.StartIndexLocation, slice.BaseVertexLocation);
        
            context->PSSetShaderResources(0, 2, Utils::NullSRVs);
        }
    }
    context->VSSetConstantBuffers(1, 2, Utils::NullBuffers);
    context->HSSetConstantBuffers(1, 2, Utils::NullBuffers);
    context->DSSetConstantBuffers(1, 2, Utils::NullBuffers);
    context->PSSetConstantBuffers(1, 2, Utils::NullBuffers);
    context->PSSetShaderResources(0, 2, Utils::NullSRVs); // clean material textures
    context->VSSetConstantBuffers(1, 2, Utils::NullBuffers); // clean object and material buffers
    context->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_UNDEFINED);
}
