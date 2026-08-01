#include "BeStandardGeometryPass.h"

#include <sen-rhi/SenBackend.h>

#include "BePass.h"
#include "BeMaterial.h"
#include "BePipelineBuilder.h"
#include "BeRenderer.h"
#include "BeShader.h"
#include "BeShaderLibrary.h"
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

auto BeStandardGeometryPass::Initialise(BeRenderer& renderer) -> void {
    _objectMaterial = BeMaterial::Create(BeShaderLibrary::GetMaterialScheme("object-material-for-geometry-pass"), true);
}

auto BeStandardGeometryPass::Render(BeRenderer& renderer, SenCommandBuffer& cmd) -> void {
    const auto uniformMat = _srm->UniformMaterial;
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
    pass.SetViewport(renderer.GetViewport());
    pass.Begin();

    cmd.SetVertexBuffer(_srm->GetSharedVertexBuffer());
    cmd.SetIndexBuffer (_srm->GetSharedIndexBuffer());

    for (const auto& entry : entries) {
        be_assert(entry.Prop->Shader);

        _objectMaterial->SetMatrix("Model", entry.ModelMatrix);
        _objectMaterial->SetMatrix("ProjectionView", uniformMat->GetMatrix("CameraProjectionView"));
        _objectMaterial->SetFloat3("ViewerPosition", uniformMat->GetFloat3("CameraPosition"));
        cmd.SetBindGroup(_objectMaterial->GetBindGroup(), 1);

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
