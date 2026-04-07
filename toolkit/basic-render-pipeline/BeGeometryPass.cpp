#include "BeGeometryPass.h"

#include <cassert>
#include <scope_guard/scope_guard.hpp>
#include <umbrellas/include-glm.h>
#include <sen-rhi/SenBackend.h>

#include "BeAssetRegistry.h"
#include "BeBRPSubmissionBuffer.h"
#include "BeMaterial.h"
#include "BePipelineBuilder.h"
#include "BeRenderer.h"
#include "BeShader.h"
#include "BeTexture.h"
#include "BeTimer.h"

BeGeometryPass::BeGeometryPass() = default;
BeGeometryPass::~BeGeometryPass() = default;

auto BeGeometryPass::Initialise() -> void {}

auto BeGeometryPass::Render() -> void
{
    auto& cmd = _renderer->GetCommandBuffer();
    const auto submissionBuffer = SubmissionBuffer.lock();
    const auto uniformMat = submissionBuffer->UniformMaterial.lock();
    const auto& entries = submissionBuffer->GetGeometryEntries();

    auto output0 = OutputTexture0.lock();
    auto output1 = OutputTexture1.lock();
    auto output2 = OutputTexture2.lock();
    auto output3 = OutputTexture3.lock();
    auto outputDepth = OutputDepthTexture.lock();
    
    cmd.SetBindGroup(uniformMat->GetBindGroup(), 0);

    cmd.BeginPass({
        .ColorAttachments = {
            { output0->Handle, 0, -1, SenLoadOp::Clear, {0, 0, 0, 0} },
            { output1->Handle, 0, -1, SenLoadOp::Clear, {0, 0, 0, 0} },
            { output2->Handle, 0, -1, SenLoadOp::Clear, {0, 0, 0, 0} },
            { output3->Handle, 0, -1, SenLoadOp::Clear, {0, 0, 0, 0} },
        },
        .DepthAttachment = SenDepthAttachment{ outputDepth->Handle },
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

        auto mat = submissionBuffer->AcquireNewObjectMaterial();
        mat->SetMatrix("Model", entry.ModelMatrix);
        mat->SetMatrix("ProjectionView", uniformMat->GetMatrix("CameraProjectionView"));
        mat->SetFloat3("ViewerPosition", uniformMat->GetFloat3("CameraPosition"));
        cmd.SetBindGroup(mat->GetBindGroup(), 1);

        const auto& meshSlices = submissionBuffer->GetMeshSlices(entry.Prop->Mesh.get());
        for (size_t j = 0; j < meshSlices.size(); ++j) {
            const auto& meshSlice = meshSlices[j];
            auto& propSlice = entry.Prop->Slices[j];

            auto pipeline = 
                BePipelineBuilder::Start(*shader)
                .SetCullMode(propSlice.TwoSided ? SenCullMode::None : SenCullMode::Back)
                .SetColorFormats({output0->Format, output1->Format, output2->Format, output3->Format, })
                .SetDepthFormat(outputDepth->Format)
                .Build();
            cmd.SetPipeline(pipeline);
            
            cmd.SetBindGroup(propSlice.Material->GetBindGroup(), 2);
            cmd.DrawIndexed(meshSlice.IndexCount, meshSlice.StartIndexLocation, meshSlice.BaseVertexLocation);
        }
    }
}
