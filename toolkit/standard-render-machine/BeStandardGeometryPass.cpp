#include "BeStandardGeometryPass.h"

#include <scope_guard/scope_guard.hpp>
#include <sen-rhi/SenBackend.h>

#include "BeMaterial.h"
#include "BePipelineBuilder.h"
#include "BeRenderer.h"
#include "BeShader.h"
#include "BeTexture.h"
#include "standard-render-machine/BeStandardRenderMachine.h"

BeStandardGeometryPass::BeStandardGeometryPass(
    BeStandardRenderMachine* srm,
    std::vector<std::shared_ptr<BeTexture>> colorTargets,
    std::shared_ptr<BeTexture> depthTarget
) : _srm(srm), _colorTargets(std::move(colorTargets)), _depthTarget(std::move(depthTarget)) {}

auto BeStandardGeometryPass::Render() -> void {
    auto& cmd = _renderer->GetCommandBuffer();
    const auto uniformMat = _srm->UniformMaterial.lock();
    const auto& entries = _srm->GetGeometryEntries();

    std::vector<SenColorAttachment> colorAttachments;
    colorAttachments.reserve(_colorTargets.size());
    std::vector<SenFormat> colorFormats;
    colorFormats.reserve(_colorTargets.size());
    for (const auto& tex : _colorTargets) {
        colorAttachments.push_back({ tex->Handle, 0, -1, SenLoadOp::Clear, {0, 0, 0, 0} });
        colorFormats.push_back(tex->Format);
    }

    cmd.SetBindGroup(uniformMat->GetBindGroup(), 0);

    cmd.BeginPass({
        .ColorAttachments = colorAttachments,
        .DepthAttachment  = SenDepthAttachment{ _depthTarget->Handle },
        .Viewport = { 0, 0, float(_renderer->GetWidth()), float(_renderer->GetHeight()), 0, 1 },
    });
    SCOPE_EXIT { cmd.EndPass(); };

    cmd.SetVertexBuffer(_srm->GetSharedVertexBuffer(), sizeof(BeFullVertex));
    cmd.SetIndexBuffer(_srm->GetSharedIndexBuffer());
    SCOPE_EXIT { cmd.ClearVertexBuffer(); cmd.ClearIndexBuffer(); };

    for (const auto& entry : entries) {
        be_assert(entry.Prop->Shader);

        auto mat = _srm->AcquireNewObjectMaterial();
        mat->SetMatrix("Model", entry.ModelMatrix);
        mat->SetMatrix("ProjectionView", uniformMat->GetMatrix("CameraProjectionView"));
        mat->SetFloat3("ViewerPosition", uniformMat->GetFloat3("CameraPosition"));
        cmd.SetBindGroup(mat->GetBindGroup(), 1);

        const auto& meshSlices = _srm->GetMeshSlices(entry.Prop->Mesh.get());
        for (size_t j = 0; j < meshSlices.size(); ++j) {
            const auto& meshSlice = meshSlices[j];
            auto& propSlice = entry.Prop->Slices[j];

            auto pipeline = BePipelineBuilder::Start(*entry.Prop->Shader)
                .SetCullMode(propSlice.TwoSided ? SenCullMode::None : SenCullMode::Back)
                .SetColorFormats(colorFormats)
                .SetDepthFormat(_depthTarget->Format)
                .Build();
            cmd.SetPipeline(pipeline);

            cmd.SetBindGroup(propSlice.Material->GetBindGroup(), 2);
            cmd.DrawIndexed(meshSlice.IndexCount, meshSlice.StartIndexLocation, meshSlice.BaseVertexLocation);
        }
    }
}
