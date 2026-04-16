#include "BeStandardShadowPass.h"

#include <format>
#include <scope_guard/scope_guard.hpp>
#include <umbrellas/include-glm.h>
#include <sen-rhi/SenBackend.h>

#include "BeAssetRegistry.h"
#include "BeMaterial.h"
#include "BePipelineBuilder.h"
#include "BeRenderer.h"
#include "BeShader.h"
#include "BeTexture.h"
#include "standard-render-machine/BeStandardRenderMachine.h"

BeStandardShadowPass::BeStandardShadowPass(BeStandardRenderMachine* srm) : _srm(srm) {}

auto BeStandardShadowPass::Render() -> void {
    for (const auto& sunLight : _srm->GetSunLightEntries()) {
        if (sunLight.CastsShadows)
            RenderDirectionalShadows(sunLight);
    }
    for (const auto& pointLight : _srm->GetPointLightEntries()) {
        if (pointLight.CastsShadows)
            RenderPointLightShadows(pointLight);
    }
}

auto BeStandardShadowPass::RenderDirectionalShadows(const BeSRMSunLightEntry& sunLight) -> void {
    auto& cmd = _renderer->GetCommandBuffer();
    const auto uniformMat = _srm->UniformMaterial.lock();
    const auto& entries = _srm->GetGeometryEntries();

    cmd.SetBindGroup(uniformMat->GetBindGroup(), 0);

    cmd.BeginPass({
        .DepthAttachment = SenDepthAttachment{ sunLight.ShadowMap.lock()->Handle },
        .Viewport = { 0, 0, (float)sunLight.ShadowMapResolution, (float)sunLight.ShadowMapResolution, 0, 1 },
    });
    SCOPE_EXIT { cmd.EndPass(); };

    cmd.SetVertexBuffer(_srm->GetSharedVertexBuffer(), sizeof(BeFullVertex));
    cmd.SetIndexBuffer(_srm->GetSharedIndexBuffer());
    SCOPE_EXIT { cmd.ClearVertexBuffer(); cmd.ClearIndexBuffer(); };

    for (const auto& entry : entries) {
        if (!entry.CastShadows)
            continue;

        auto mat = _srm->AcquireNewObjectMaterial();
        mat->SetMatrix("Model", entry.ModelMatrix);
        mat->SetMatrix("ProjectionView", sunLight.ShadowViewProjection);
        mat->SetFloat3("ViewerPosition", glm::vec3(0.f));
        cmd.SetBindGroup(mat->GetBindGroup(), 1);

        const auto& meshSlices = _srm->GetMeshSlices(entry.Prop->Mesh.get());
        for (size_t j = 0; j < meshSlices.size(); ++j) {
            const auto& meshSlice = meshSlices[j];
            auto& propSlice = entry.Prop->Slices[j];
#
            auto pipeline = BePipelineBuilder::Start(*entry.Prop->Shader)
                .SetCullMode(propSlice.TwoSided ? SenCullMode::None : SenCullMode::Back)
                .SetDepthFormat(sunLight.ShadowMap.lock()->Format)
                .Build();
            
            cmd.SetPipeline(pipeline);
            cmd.SetBindGroup(propSlice.Material->GetBindGroup(), 2);
            cmd.DrawIndexed(meshSlice.IndexCount, meshSlice.StartIndexLocation, meshSlice.BaseVertexLocation);
        }
    }
}

auto BeStandardShadowPass::RenderPointLightShadows(const BeSRMPointLightEntry& pointLight) -> void {
    auto& cmd = _renderer->GetCommandBuffer();
    const auto uniformMat = _srm->UniformMaterial.lock();
    const auto& entries = _srm->GetGeometryEntries();
    auto shadowMap = pointLight.ShadowMap.lock();

    cmd.SetVertexBuffer(_srm->GetSharedVertexBuffer(), sizeof(BeFullVertex));
    cmd.SetIndexBuffer(_srm->GetSharedIndexBuffer());
    SCOPE_EXIT { cmd.ClearVertexBuffer(); cmd.ClearIndexBuffer(); };

    cmd.SetBindGroup(uniformMat->GetBindGroup(), 0);

    for (int face = 0; face < 6; ++face) {
        const glm::mat4 faceViewProj = CalculatePointLightFaceViewProjection(pointLight, face);

        cmd.BeginPass({
            .DepthAttachment = SenDepthAttachment{
                .Texture = shadowMap->Handle,
                .CubemapFace = static_cast<int8_t>(face),
                .LoadOp = SenLoadOp::Clear,
            },
            .Viewport = SenViewport{
                .Width  = (float)pointLight.ShadowMapResolution,
                .Height = (float)pointLight.ShadowMapResolution,
            },
        });
        SCOPE_EXIT { cmd.EndPass(); };

        for (const auto& entry : entries) {
            if (!entry.CastShadows)
                continue;

            auto mat = _srm->AcquireNewObjectMaterial();
            mat->SetMatrix("Model", entry.ModelMatrix);
            mat->SetMatrix("ProjectionView", faceViewProj);
            mat->SetFloat3("ViewerPosition", pointLight.Position);
            cmd.SetBindGroup(mat->GetBindGroup(), 1);

            const auto& meshSlices = _srm->GetMeshSlices(entry.Prop->Mesh.get());
            for (size_t j = 0; j < meshSlices.size(); ++j) {
                const auto& meshSlice = meshSlices[j];
                auto& propSlice = entry.Prop->Slices[j];

                auto pipeline = BePipelineBuilder::Start(*entry.Prop->Shader)
                    .SetCullMode(propSlice.TwoSided ? SenCullMode::None : SenCullMode::Back)
                    .SetDepthFormat(shadowMap->Format)
                    .Build();
                cmd.SetPipeline(pipeline);
                cmd.SetBindGroup(propSlice.Material->GetBindGroup(), 2);
                cmd.DrawIndexed(meshSlice.IndexCount, meshSlice.StartIndexLocation, meshSlice.BaseVertexLocation);
            }
        }
    }
}

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
