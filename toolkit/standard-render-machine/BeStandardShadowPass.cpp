#include "BeStandardShadowPass.h"

#include <umbrellas/include-glm.h>
#include <sen-rhi/SenBackend.h>

#include "BeAssetRegistry.h"
#include "BePass.h"
#include "BeMaterial.h"
#include "BePipelineBuilder.h"
#include "BeRenderer.h"
#include "BeShader.h"
#include "BeShaderLibrary.h"
#include "BeTexture.h"
#include "standard-render-machine/BeStandardRenderMachine.h"

BeStandardShadowPass::BeStandardShadowPass(BeStandardRenderMachine* srm) : _srm(srm) {}

auto BeStandardShadowPass::Initialise(BeRenderer& renderer) -> void {
    _objectMaterial = BeMaterial::Create(BeShaderLibrary::GetMaterialScheme("object-material-for-geometry-pass"));
}

auto BeStandardShadowPass::Render(BeRenderer& renderer, SenCommandBuffer& cmd) -> void {
    for (const auto& sunLight : _srm->GetSunLightEntries()) {
        if (sunLight.CastsShadows) {
            RenderDirectionalShadows(cmd, sunLight);
        }
    }
    for (const auto& pointLight : _srm->GetPointLightEntries()) {
        if (pointLight.CastsShadows) {
            RenderPointLightShadows(cmd, pointLight);
        }
    }
}

auto BeStandardShadowPass::RenderDirectionalShadows(SenCommandBuffer& cmd, const BeSRMSunLightEntry& sunLight) const -> void {
    const auto uniformMat = _srm->UniformMaterial;
    const auto& entries = _srm->GetGeometryEntries();

    cmd.SetBindGroup(uniformMat->GetBindGroup(), 0);

    BePass pass(cmd);
    pass.SetDepthTarget(sunLight.ShadowMap.lock());
    pass.SetViewport({ 0, 0, (float)sunLight.ShadowMapResolution, (float)sunLight.ShadowMapResolution, 0, 1 });
    pass.Begin();

    cmd.SetVertexBuffer(_srm->GetSharedVertexBuffer());
    cmd.SetIndexBuffer(_srm->GetSharedIndexBuffer());

    for (const auto& entry : entries) {
        if (!entry.CastShadows) {
            continue;
        }

        _objectMaterial->SetMatrix("Model", entry.ModelMatrix);
        _objectMaterial->SetMatrix("ProjectionView", sunLight.ShadowViewProjection);
        _objectMaterial->SetFloat3("ViewerPosition", glm::vec3(0.f));
        cmd.SetBindGroup(_objectMaterial->GetBindGroup(), 1);

        const auto& meshSlices = _srm->GetMeshSlices(entry.Prop->Mesh.get());
        for (size_t j = 0; j < meshSlices.size(); ++j) {
            const auto& meshSlice = meshSlices[j];
            const auto& propSlice = entry.Prop->Slices[j];

            const auto pipeline = BePipelineBuilder::Start(*entry.Prop->Shader)
                .SetCullMode(propSlice.TwoSided ? SenCullMode::None : SenCullMode::Back)
                .SetDepthFormat(sunLight.ShadowMap.lock()->Format)
                .Build()
            ;
            
            cmd.SetPipeline(pipeline);
            cmd.SetBindGroup(propSlice.Material->GetBindGroup(), 2);
            cmd.DrawIndexed(meshSlice.IndexCount, meshSlice.StartIndexLocation, meshSlice.BaseVertexLocation);
        }
    }
    pass.End();
}

auto BeStandardShadowPass::RenderPointLightShadows(SenCommandBuffer& cmd, const BeSRMPointLightEntry& pointLight) const -> void {
    const auto  uniformMat = _srm->UniformMaterial;
    const auto& entries = _srm->GetGeometryEntries();
    const auto  shadowMap = pointLight.ShadowMap.lock();

    cmd.SetVertexBuffer(_srm->GetSharedVertexBuffer());
    cmd.SetIndexBuffer(_srm->GetSharedIndexBuffer());

    cmd.SetBindGroup(uniformMat->GetBindGroup(), 0);

    for (int face = 0; face < 6; ++face) {
        const glm::mat4 faceViewProj = CalculatePointLightFaceViewProjection(pointLight, face);

        BePass pass(cmd);
        pass.SetDepthTarget(shadowMap, SenLoadOp::Clear, 1.0f, static_cast<int8_t>(face));
        pass.SetViewport({ 0, 0, (float)pointLight.ShadowMapResolution, (float)pointLight.ShadowMapResolution, 0, 1 });
        pass.Begin();

        for (const auto& entry : entries) {
            if (!entry.CastShadows) {
                continue;
            }

            _objectMaterial->SetMatrix("Model", entry.ModelMatrix);
            _objectMaterial->SetMatrix("ProjectionView", faceViewProj);
            _objectMaterial->SetFloat3("ViewerPosition", pointLight.Position);
            cmd.SetBindGroup(_objectMaterial->GetBindGroup(), 1);

            const auto& meshSlices = _srm->GetMeshSlices(entry.Prop->Mesh.get());
            for (size_t j = 0; j < meshSlices.size(); ++j) {
                const auto& meshSlice = meshSlices[j];
                const auto& propSlice = entry.Prop->Slices[j];
                const auto  pipeline = BePipelineBuilder::Start(*entry.Prop->Shader)
                    .SetCullMode(propSlice.TwoSided ? SenCullMode::None : SenCullMode::Back)
                    .SetDepthFormat(shadowMap->Format)
                    .Build()
                ;
                
                cmd.SetPipeline(pipeline);
                cmd.SetBindGroup(propSlice.Material->GetBindGroup(), 2);
                cmd.DrawIndexed(meshSlice.IndexCount, meshSlice.StartIndexLocation, meshSlice.BaseVertexLocation);
            }
        }

        pass.End();
    }
}

// ReSharper disable once CppMemberFunctionMayBeStatic
auto BeStandardShadowPass::CalculatePointLightFaceViewProjection(
    const BeSRMPointLightEntry& pointLight,
    const int faceIndex
) const -> glm::mat4 {
    static constexpr std::array<glm::vec3, 6> Forwards = {
        glm::vec3( 1,  0,  0), glm::vec3(-1,  0,  0),
        glm::vec3( 0,  1,  0), glm::vec3( 0, -1,  0),
        glm::vec3( 0,  0,  1), glm::vec3( 0,  0, -1),
    };
    static constexpr std::array<glm::vec3, 6> Ups = {
        glm::vec3(0, 1, 0), glm::vec3(0,  1, 0),
        glm::vec3(0, 0,-1), glm::vec3(0,  0, 1),
        glm::vec3(0, 1, 0), glm::vec3(0,  1, 0),
    };

    const glm::mat4 proj = glm::perspectiveLH_ZO(glm::radians(90.0f), 1.0f, pointLight.ShadowNearPlane, pointLight.Radius);
    const glm::mat4 view = glm::lookAtLH(pointLight.Position, pointLight.Position + Forwards[faceIndex], Ups[faceIndex]);
    return proj * view;
}
