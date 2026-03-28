#include "BeGeometryPass.h"

#include <cassert>
#include <scope_guard/scope_guard.hpp>
#include <umbrellas/include-glm.h>
#include <sen-rhi/SenBackend.h>

#include "BeAssetRegistry.h"
#include "BeBRPSubmissionBuffer.h"
#include "BeMaterial.h"
#include "BeRenderer.h"
#include "BeShader.h"
#include "BeTexture.h"

BeGeometryPass::BeGeometryPass() = default;
BeGeometryPass::~BeGeometryPass() = default;

auto BeGeometryPass::Initialise() -> void {}

auto BeGeometryPass::Render() -> void
{
    auto& cmd = _renderer->GetCommandBuffer();
    const auto submissionBuffer = SubmissionBuffer.lock();
    const auto uniformMat = submissionBuffer->UniformMaterial.lock();
    const auto& entries = submissionBuffer->GetGeometryEntries();

    auto objectScheme = BeAssetRegistry::GetMaterialScheme("object-material-for-geometry-pass");

    cmd.SetBindGroup(uniformMat->GetBindGroup(), 0);

    cmd.BeginPass({
        .ColorAttachments = {
            { OutputTexture0.lock()->Handle, 0, -1, SenLoadOp::Clear, {0, 0, 0, 0} },
            { OutputTexture1.lock()->Handle, 0, -1, SenLoadOp::Clear, {0, 0, 0, 0} },
            { OutputTexture2.lock()->Handle, 0, -1, SenLoadOp::Clear, {0, 0, 0, 0} },
            { OutputTexture3.lock()->Handle, 0, -1, SenLoadOp::Clear, {0, 0, 0, 0} },
        },
        .DepthAttachment = SenDepthAttachment{ OutputDepthTexture.lock()->Handle },
        .Viewport = { 0, 0, float(_renderer->GetWidth()), float(_renderer->GetHeight()), 0, 1 },
    });
    SCOPE_EXIT { cmd.EndPass(); };

    cmd.SetVertexBuffer(submissionBuffer->GetSharedVertexBuffer(), sizeof(BeFullVertex));
    cmd.SetIndexBuffer(submissionBuffer->GetSharedIndexBuffer());
    SCOPE_EXIT {
        cmd.ClearVertexBuffer();
        cmd.ClearIndexBuffer();
    };

    for (const auto& entry : entries) {
        const auto shader = entry.Prop->Shader;
        be_assert(shader);

        //if (!_objectMaterials.contains(entry.Name)) {
        //    _objectMaterials[entry.Name] = BeMaterial::Create("object_" + entry.Name, objectScheme, true);
        //}
        auto mat = submissionBuffer->AcquireNewObjectMaterial();
        mat->SetMatrix("Model", entry.ModelMatrix);
        mat->SetMatrix("ProjectionView", uniformMat->GetMatrix("CameraProjectionView"));
        mat->SetFloat3("ViewerPosition", uniformMat->GetFloat3("CameraPosition"));
        cmd.SetBindGroup(mat->GetBindGroup(), 1);

        const auto& meshSlices = submissionBuffer->GetMeshSlices(entry.Prop->Mesh.get());
        for (size_t j = 0; j < meshSlices.size(); ++j) {
            const auto& meshSlice = meshSlices[j];
            auto& propSlice = entry.Prop->Slices[j];

            PipelineKey key{ shader.get(), propSlice.TwoSided };
            if (!_shaderPipelines.contains(key)) {
                auto pipelineDesc = shader->GetPipelineDesc();
                pipelineDesc.RasterizerState.CullMode = propSlice.TwoSided ? SenCullMode::None : SenCullMode::Back;
                pipelineDesc.RenderTargetFormats = {
                    OutputTexture0.lock()->Format,
                    OutputTexture1.lock()->Format,
                    OutputTexture2.lock()->Format,
                    OutputTexture3.lock()->Format,
                };
                pipelineDesc.DepthStencilFormat = OutputDepthTexture.lock()->Format;
                _shaderPipelines[key] = SenBackend::CreatePipeline(pipelineDesc);
            }
            cmd.SetPipeline(_shaderPipelines[key]);

            cmd.SetBindGroup(propSlice.Material->GetBindGroup(), 2);
            cmd.DrawIndexed(meshSlice.IndexCount, meshSlice.StartIndexLocation, meshSlice.BaseVertexLocation);
        }
    }
}
