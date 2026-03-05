#include "BeGeometryPass.h"

#include <cassert>
#include <scope_guard/scope_guard.hpp>
#include <umbrellas/include-glm.h>

#include "BeAssetRegistry.h"
#include "BeBRPSubmissionBuffer.h"
#include "BeMaterial.h"
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
    pipeline->BindVertexBuffer(submissionBuffer->GetSharedVertexBuffer(), sizeof(BeFullVertex));
    pipeline->BindIndexBuffer(submissionBuffer->GetSharedIndexBuffer());
    SCOPE_EXIT {
        pipeline->ClearVertexBuffer();
        pipeline->ClearIndexBuffer();
    };

    
    // Draw all objects
    const auto& entries = SubmissionBuffer.lock()->GetGeometryEntries();
    for (const auto& entry : entries) {
        const auto shader = entry.Prop->Shader;
        assert(shader);

        pipeline->BindShader(shader, BeShaderType::All);
        SCOPE_EXIT { pipeline->Clear(); };

        _objectMaterial->SetMatrix("Model", entry.ModelMatrix);
        _objectMaterial->SetMatrix("ProjectionView", _renderer->UniformData.ProjectionView);
        _objectMaterial->SetFloat3("ViewerPosition", _renderer->UniformData.CameraPosition);
        _objectMaterial->UpdateGPUBuffers(context);
        pipeline->BindMaterialAutomatic(_objectMaterial);

        const auto& meshSlices = submissionBuffer->GetMeshSlices(entry.Prop->Mesh.get());
        for (size_t i = 0; i < meshSlices.size(); ++i) {
            const auto& meshSlice = meshSlices[i];
            const auto& propSlice = entry.Prop->Slices[i];

            if (propSlice.TwoSided) {
                context->RSSetState(_renderer->GetRasterizerCullNone().Get());
            }

            pipeline->BindMaterialAutomatic(propSlice.Material);
            pipeline->DrawIndexed(meshSlice.IndexCount, meshSlice.StartIndexLocation, meshSlice.BaseVertexLocation);

            if (propSlice.TwoSided) {
                context->RSSetState(_renderer->GetRasterizerCullBack().Get());
            }
        }
    }
}
