#include "BeGeometryPass.h"

#include <cassert>
#include <scope_guard/scope_guard.hpp>
#include <umbrellas/include-glm.h>

#include "BeAssetRegistry.h"
#include "BeMaterial.h"
#include "BeModel.h"
#include "BePipeline.h"
#include "BeRenderer.h"
#include "BeTexture.h"
#include "Utils.h"

BeGeometryPass::BeGeometryPass() = default;
BeGeometryPass::~BeGeometryPass() = default;

auto BeGeometryPass::Initialise() -> void {
    auto objectScheme = BeAssetRegistry::GetMaterialScheme("object-material-for-geometry-pass");
    _objectMaterial = BeMaterial::Create("object", objectScheme, true, *_renderer);
}

auto BeGeometryPass::Render() -> void {
    const auto context = _renderer->GetContext();
    const auto pipeline = _renderer->GetPipeline();
    
    
    // Clear and set render targets
    const auto depthResource    = OutputDepthTexture.lock();
    const auto gbufferResource0 = OutputTexture0.lock();
    const auto gbufferResource1 = OutputTexture1.lock();
    const auto gbufferResource2 = OutputTexture2.lock();
    
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
    const auto& entries = _renderer->GetDrawEntries();
    for (const auto& entry : entries) {
        const auto shader = entry.Model->Shader;
        assert(shader);
        
        pipeline->BindShader(shader, BeShaderType::All);
        SCOPE_EXIT { pipeline->Clear(); };

        const glm::mat4x4 modelMatrix =
            glm::translate(glm::mat4(1.0f), entry.Position) *
            glm::mat4_cast(entry.Rotation) *
            glm::scale(glm::mat4(1.0f), entry.Scale);
        
        _objectMaterial->SetMatrix("Model", modelMatrix);
        _objectMaterial->SetMatrix("ProjectionView", _renderer->UniformData.ProjectionView);
        _objectMaterial->SetFloat3("ViewerPosition", _renderer->UniformData.CameraPosition);
        _objectMaterial->UpdateGPUBuffers(context);
        pipeline->BindMaterialAutomatic(_objectMaterial);

        const auto & drawSlices = _renderer->GetDrawSlicesForModel(entry.Model);
        for (const auto& slice : drawSlices) {
            pipeline->BindMaterialAutomatic(slice.Material);
            context->DrawIndexed(slice.IndexCount, slice.StartIndexLocation, slice.BaseVertexLocation);
        }
    }
}
