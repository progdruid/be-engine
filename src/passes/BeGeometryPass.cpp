#include "BeGeometryPass.h"

#include <cassert>
#include <scope_guard/scope_guard.hpp>
#include <umbrellas/include-glm.h>

#include "BeAssetRegistry.h"
#include "BeModel.h"
#include "BePipeline.h"
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
    const auto pipeline = _renderer->GetPipeline();
    const auto registry = _renderer->GetAssetRegistry().lock();
    
    
    // Clear and set render targets
    const auto depthResource    = registry->GetTexture(OutputDepthTextureName).lock();
    const auto gbufferResource0 = registry->GetTexture(OutputTexture0Name).lock();
    const auto gbufferResource1 = registry->GetTexture(OutputTexture1Name).lock();
    const auto gbufferResource2 = registry->GetTexture(OutputTexture2Name).lock();
    
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

    
    // Draw all objects
    const auto& objects = _renderer->GetObjects();
    for (const auto& object : objects) {
        const auto shader = object.Model->Shader;
        assert(shader);
        
        pipeline->BindShader(shader, BeShaderType::All);
        SCOPE_EXIT { pipeline->Clear(); };

        glm::mat4x4 modelMatrix =
            glm::translate(glm::mat4(1.0f), object.Position) *
            glm::mat4_cast(object.Rotation) *
            glm::scale(glm::mat4(1.0f), object.Scale);
        
        BeObjectBufferGPU objectData(modelMatrix, _renderer->UniformData.ProjectionView, _renderer->UniformData.CameraPosition);
        D3D11_MAPPED_SUBRESOURCE objectMappedResource;
        Utils::Check << context->Map(_objectBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &objectMappedResource);
        memcpy(objectMappedResource.pData, &objectData, sizeof(BeObjectBufferGPU));
        context->Unmap(_objectBuffer.Get(), 0);
        context->VSSetConstantBuffers(1, 1, _objectBuffer.GetAddressOf());
        context->PSSetConstantBuffers(1, 1, _objectBuffer.GetAddressOf());
        if (HasAny(shader->ShaderType, BeShaderType::Tesselation)) {
            context->HSSetConstantBuffers(1, 1, _objectBuffer.GetAddressOf());
            context->DSSetConstantBuffers(1, 1, _objectBuffer.GetAddressOf());
        }
        
        for (const auto& slice : object.DrawSlices) {
            pipeline->BindMaterial(slice.Material);
            context->DrawIndexed(slice.IndexCount, slice.StartIndexLocation, slice.BaseVertexLocation);
        }
    }
}
