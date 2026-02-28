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

BeGeometryPass::BeGeometryPass() = default;
BeGeometryPass::~BeGeometryPass() = default;

auto BeGeometryPass::Initialise() -> void {
    auto objectScheme = BeAssetRegistry::GetMaterialScheme("object-material-for-geometry-pass");
    _objectMaterial = BeMaterial::Create("object", objectScheme, true, *_renderer);
}

auto BeGeometryPass::Render() -> void {
    const auto pipeline = _renderer->GetPipeline();
    const auto submissionBuffer = SubmissionBuffer.lock();

    pipeline->BindTargets({ OutputTexture0, OutputTexture1, OutputTexture2, OutputTexture3 }, OutputDepthTexture.lock().get(), true);
    SCOPE_EXIT { pipeline->ClearTargets(); };

    pipeline->BindMeshBuffers(*submissionBuffer);
    SCOPE_EXIT { pipeline->UnbindMeshBuffers(); };

    const auto& entries = SubmissionBuffer.lock()->GetGeometryEntries();
    for (const auto& entry : entries) {
        const auto shader = entry.Model->Shader;
        assert(shader);

        pipeline->BindShader(shader, BeShaderType::All);
        SCOPE_EXIT { pipeline->Clear(); };

        _objectMaterial->SetMatrix("Model", entry.ModelMatrix);
        _objectMaterial->SetMatrix("ProjectionView", _renderer->UniformData.ProjectionView);
        _objectMaterial->SetFloat3("ViewerPosition", _renderer->UniformData.CameraPosition);
        pipeline->UpdateMaterialBuffers(_objectMaterial);
        pipeline->BindMaterialAutomatic(_objectMaterial);

        const auto& drawSlices = submissionBuffer->GetDrawSlicesForModel(entry.Model);
        for (const auto& slice : drawSlices) {
            if (slice.TwoSided)
                pipeline->SetCullMode(BeCullMode::None);

            pipeline->BindMaterialAutomatic(slice.Material);
            pipeline->DrawSlice(slice);

            if (slice.TwoSided)
                pipeline->SetCullMode(BeCullMode::Back);
        }
    }
}
