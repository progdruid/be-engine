#include "BeStandardGeometryPass.h"

#include <sen-rhi/SenBackend.h>

#include "BePass.h"
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
) 
: _srm(srm)
, _colorTargets(std::move(colorTargets))
, _depthTarget(std::move(depthTarget)) {}

auto BeStandardGeometryPass::Initialise() -> void {}

auto BeStandardGeometryPass::Render(SenCommandBuffer& cmd) -> void {
    const auto uniformMat = _srm->UniformMaterial.lock();
    const auto& entries = _srm->GetGeometryEntries();

    std::vector<SenFormat> colorFormats;
    colorFormats.reserve(_colorTargets.size());
    for (const auto& tex : _colorTargets) {
        colorFormats.push_back(tex->Format);
    }

    cmd.SetBindGroup(uniformMat->GetBindGroup(), 0);

    BePass pass(cmd);
    pass.AddColorTargets(_colorTargets);
    pass.SetDepthTarget(_depthTarget);
    pass.SetViewport(_renderer->GetViewport());
    pass.Begin();

    cmd.SetVertexBuffer(_srm->GetSharedVertexBuffer());
    cmd.SetIndexBuffer (_srm->GetSharedIndexBuffer());

    for (const auto& entry : entries) {
        be_assert(entry.Prop->Shader);

        const auto& scheme = entry.Prop->Shader->GetMaterialScheme("geometry-object");
        const auto  mat = _srm->AcquireNewObjectMaterial(scheme);
        mat->SetMatrix("Model", entry.ModelMatrix);
        mat->SetMatrix("ProjectionView", uniformMat->GetMatrix("CameraProjectionView"));
        mat->SetFloat3("ViewerPosition", uniformMat->GetFloat3("CameraPosition"));
        cmd.SetBindGroup(mat->GetBindGroup(), 1);

        const auto& meshSlices = _srm->GetMeshSlices(entry.Prop->Mesh.get());
        for (size_t j = 0; j < meshSlices.size(); ++j) {
            const auto& meshSlice = meshSlices[j];
            const auto& propSlice = entry.Prop->Slices[j];

            const auto pipeline = BePipelineBuilder::Start(*entry.Prop->Shader)
                .SetCullMode(propSlice.TwoSided ? SenCullMode::None : SenCullMode::Back)
                .SetColorFormats(colorFormats)
                .SetDepthFormat(_depthTarget->Format)
                .Build()
            ;
            cmd.SetPipeline(pipeline);

            cmd.SetBindGroup(propSlice.Material->GetBindGroup(), 2);
            cmd.DrawIndexed(meshSlice.IndexCount, meshSlice.StartIndexLocation, meshSlice.BaseVertexLocation);
        }
    }

    pass.End();
}
