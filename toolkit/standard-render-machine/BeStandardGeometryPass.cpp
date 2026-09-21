#include "BeStandardGeometryPass.h"

#include <sen-rhi/Sen.h>

#include "BePass.h"
#include "BeMaterial.h"
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
    _objectMaterial = BeMaterial::Create(BeShaderLibrary::GetMaterialScheme("object-material-for-geometry-pass"));
}

auto BeStandardGeometryPass::Render(BeRenderer& renderer, SenCommandList cmd) -> void {
    const auto uniformMat = _srm->UniformMaterial;
    const auto& entries = _srm->GetGeometryEntries();

    BePass pass(cmd);
    pass.AddColorTargets(_colorTargets);
    pass.SetDepthTarget(_depthTarget);
    pass.SetViewport(_colorTargets[0]->GetViewport());
    pass.SetVertexBuffer(_srm->GetSharedVertexBuffer());
    pass.SetIndexBuffer (_srm->GetSharedIndexBuffer());
    pass.Begin();

    for (const auto& entry : entries) {
        be_assert(entry.Prop->State.IsValid());

        _objectMaterial->SetMatrix("Model", entry.ModelMatrix);
        _objectMaterial->SetMatrix("ProjectionView", uniformMat->GetMatrix("CameraProjectionView"));
        _objectMaterial->SetFloat3("ViewerPosition", uniformMat->GetFloat3("CameraPosition"));

        const auto& meshSlices = _srm->GetMeshSlices(entry.Prop->Mesh.get());
        for (size_t j = 0; j < meshSlices.size(); ++j) {
            const auto& meshSlice = meshSlices[j];
            const auto& propSlice = entry.Prop->Slices[j];

            pass.SetState(entry.Prop->State);
            pass.OverrideCull(propSlice.TwoSided ? SenCull::None : SenCull::Back);
            pass.Bind("frame", *uniformMat);
            pass.Bind("geometry-object", *_objectMaterial);
            pass.Bind("geometry-main", *propSlice.Material);

            pass.DrawIndexed(meshSlice.IndexCount, meshSlice.StartIndexLocation, meshSlice.BaseVertexLocation);
        }
    }

    pass.End();
}
