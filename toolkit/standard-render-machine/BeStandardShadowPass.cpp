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
    _srm->EnsureShadowArrays();

    const auto directionalArray = _srm->GetDirectionalShadowArray();
    uint32_t directionalSlice = 0;
    for (const auto& sunLight : _srm->GetSunLightEntries()) {
        if (!sunLight.CastsShadows) continue;
        if (directionalSlice >= _srm->Settings.Shadow.MaxDirectional) break;
        RenderDirectionalShadows(cmd, sunLight, directionalArray, directionalSlice);
        ++directionalSlice;
    }

    const auto pointArray = _srm->GetPointShadowArray();
    uint32_t pointSlice = 0;
    for (const auto& pointLight : _srm->GetPointLightEntries()) {
        if (!pointLight.CastsShadows) continue;
        if (pointSlice >= _srm->Settings.Shadow.MaxPoint) break;
        RenderPointLightShadows(cmd, pointLight, pointArray, pointSlice);
        ++pointSlice;
    }
}

auto BeStandardShadowPass::RenderDirectionalShadows(
    SenCommandBuffer& cmd, 
    const BeSRMSunLightEntry& sunLight, 
    const std::shared_ptr<BeTexture>& shadowArray, 
    uint32_t slice
) const -> void {
    
    const auto uniformMat = _srm->UniformMaterial;
    const auto& entries = _srm->GetGeometryEntries();
    const float resolution = static_cast<float>(_srm->Settings.Shadow.DirectionalResolution);

    cmd.SetVertexBuffer(_srm->GetSharedVertexBuffer());
    cmd.SetIndexBuffer(_srm->GetSharedIndexBuffer());
    cmd.SetBindGroup(uniformMat->GetBindGroup(), 0);
    
    BePass pass(cmd);
    pass.SetDepthTarget(shadowArray, SenLoadOp::Clear, 1.0f, static_cast<int16_t>(slice));
    pass.SetViewport({ 0, 0, resolution, resolution, 0, 1 });
    pass.Begin();

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
                .SetDepthFormat(shadowArray->Format)
                .Build()
            ;
            
            cmd.SetPipeline(pipeline);
            cmd.SetBindGroup(propSlice.Material->GetBindGroup(), 2);
            cmd.DrawIndexed(meshSlice.IndexCount, meshSlice.StartIndexLocation, meshSlice.BaseVertexLocation);
        }
    }
    pass.End();
}

auto BeStandardShadowPass::RenderPointLightShadows(
    SenCommandBuffer& cmd, 
    const BeSRMPointLightEntry& pointLight, 
    const std::shared_ptr<BeTexture>& shadowArray, 
    uint32_t slice
) const -> void {
    
    const auto  uniformMat = _srm->UniformMaterial;
    const auto& entries = _srm->GetGeometryEntries();
    const float resolution = static_cast<float>(_srm->Settings.Shadow.PointResolution);

    cmd.SetVertexBuffer(_srm->GetSharedVertexBuffer());
    cmd.SetIndexBuffer(_srm->GetSharedIndexBuffer());
    cmd.SetBindGroup(uniformMat->GetBindGroup(), 0);

    for (int face = 0; face < 6; ++face) {
        const glm::mat4 faceViewProj = CalculatePointLightFaceViewProjection(pointLight, face);
        const int16_t layer = static_cast<int16_t>(slice * 6 + face);

        BePass pass(cmd);
        pass.SetDepthTarget(shadowArray, SenLoadOp::Clear, 1.0f, layer);
        pass.SetViewport({ 0, 0, resolution, resolution, 0, 1 });
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
                    .SetDepthFormat(shadowArray->Format)
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
