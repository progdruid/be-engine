#include "BeStandardGeometryPass.h"

#include <Sen.h>

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
        _objectMaterial->SetMatrix("Model", entry.ModelMatrix);
        // todo: cmooooon, stuuuupid
        _objectMaterial->SetMatrix("ProjectionView", uniformMat->GetMatrix("CameraProjectionView"));
        _objectMaterial->SetFloat3("ViewerPosition", uniformMat->GetFloat3("CameraPosition"));

        const auto& meshSlices = _srm->GetMeshSlices(entry.Prop->Mesh.get());
        for (const auto& surface : entry.Prop->Surfaces) {
            const auto& meshSlice = meshSlices[surface.MeshSlice];

            pass.SetState(_srm->GetDrawState(surface));
            pass.Bind("frame", *uniformMat);
            pass.Bind("geometry-object", *_objectMaterial);
            pass.Bind("geometry-main", *surface.Material);

            pass.DrawIndexed(meshSlice.IndexCount, meshSlice.StartIndexLocation, meshSlice.BaseVertexLocation);
        }
    }

    pass.End();
}
