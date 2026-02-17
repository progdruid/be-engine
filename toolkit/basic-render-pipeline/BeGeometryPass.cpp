#include "BeGeometryPass.h"

#include <cassert>
#include <scope_guard/scope_guard.hpp>
#include <umbrellas/include-glm.h>

#include "BeAssetRegistry.h"
#include "BeBRPSubmissionBuffer.h"
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

auto BeGeometryPass::Render() -> void
{
    const auto context = _renderer->GetContext();
    const auto pipeline = _renderer->GetPipeline();
    const auto submissionBuffer = SubmissionBuffer.lock();

    pipeline->BindTargets({ OutputTexture0, OutputTexture1, OutputTexture2, OutputTexture3 }, OutputDepthTexture.lock().get(), true);
    SCOPE_EXIT { pipeline->ClearTargets(); };
    
    // Set vertex and index buffers
    uint32_t stride = sizeof(BeFullVertex);
    uint32_t offset = 0;
    context->IASetVertexBuffers(0, 1, submissionBuffer->GetSharedVertexBuffer().GetAddressOf(), &stride, &offset);
    context->IASetIndexBuffer(submissionBuffer->GetSharedIndexBuffer().Get(), DXGI_FORMAT_R32_UINT, 0);
    SCOPE_EXIT {
        context->IASetVertexBuffers(0, 1, Utils::NullBuffers, &stride, &offset);
        context->IASetIndexBuffer(nullptr, DXGI_FORMAT_R32_UINT, 0);
    };

    
    // Draw all objects
    const auto& entries = SubmissionBuffer.lock()->GetGeometryEntries();
    for (const auto& entry : entries) {
        const auto shader = entry.Model->Shader;
        assert(shader);
        
        pipeline->BindShader(shader, BeShaderType::All);
        SCOPE_EXIT { pipeline->Clear(); };
        
        _objectMaterial->SetMatrix("Model", entry.ModelMatrix);
        _objectMaterial->SetMatrix("ProjectionView", _renderer->UniformData.ProjectionView);
        _objectMaterial->SetFloat3("ViewerPosition", _renderer->UniformData.CameraPosition);
        _objectMaterial->UpdateGPUBuffers(context);
        pipeline->BindMaterialAutomatic(_objectMaterial);

        const auto & drawSlices = submissionBuffer->GetDrawSlicesForModel(entry.Model);
        for (const auto& slice : drawSlices) {
            if (slice.TwoSided) {
                context->RSSetState(_renderer->GetRasterizerCullNone().Get());
            }

            pipeline->BindMaterialAutomatic(slice.Material);
            pipeline->DrawSlice(slice);

            if (slice.TwoSided) {
                context->RSSetState(_renderer->GetRasterizerCullBack().Get());
            }
        }
    }
}
