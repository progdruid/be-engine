#include "BeShadowPass.h"

#include <format>
#include <umbrellas/include-glm.h>
#include <scope_guard/scope_guard.hpp>
#include <sen-rhi/SenBackend.h>

#include "BeAssetRegistry.h"
#include "BeBRPSubmissionBuffer.h"
#include "BeMaterial.h"
#include "BePipelineBuilder.h"
#include "BeRenderer.h"
#include "BeShader.h"
#include "BeTexture.h"

auto BeShadowPass::Initialise() -> void {
    auto objectScheme = BeAssetRegistry::GetMaterialScheme("object-material-for-geometry-pass");
}

auto BeShadowPass::Render() -> void {
    auto& submissionBuffer = *SubmissionBuffer.lock();

    const auto& sunLights = submissionBuffer.GetSunLightEntries();
    for (size_t i = 0; i < sunLights.size(); ++i) {
        if (!sunLights[i].CastsShadows)
            continue;
        RenderDirectionalShadows(sunLights[i], submissionBuffer);
    }

    const auto& pointLights = submissionBuffer.GetPointLightEntries();
    for (size_t i = 0; i < pointLights.size(); i++) {
        if (!pointLights[i].CastsShadows)
            continue;
        RenderPointLightShadows(pointLights[i], submissionBuffer);
    }
}

auto BeShadowPass::RenderDirectionalShadows(
    const BeBRPSunLightEntry& sunLight,
    BeBRPSubmissionBuffer& submissionBuffer
) -> void {
    auto& cmd = _renderer->GetCommandBuffer();
    const auto uniformMat = submissionBuffer.UniformMaterial.lock();
    const auto& entries = submissionBuffer.GetGeometryEntries();
    
    cmd.SetBindGroup(uniformMat->GetBindGroup(), 0);

    cmd.BeginPass({
        .DepthAttachment = SenDepthAttachment{ sunLight.ShadowMap.lock()->Handle },
        .Viewport = { 0, 0, (float)sunLight.ShadowMapResolution, (float)sunLight.ShadowMapResolution, 0, 1 },
    });
    SCOPE_EXIT { cmd.EndPass(); };

    cmd.SetVertexBuffer(submissionBuffer.GetSharedVertexBuffer(), sizeof(BeFullVertex));
    cmd.SetIndexBuffer(submissionBuffer.GetSharedIndexBuffer());
    SCOPE_EXIT {
        cmd.ClearVertexBuffer();
        cmd.ClearIndexBuffer();
    };

    for (const auto& entry : entries) {
        if (!entry.CastShadows)
            continue;

        auto mat = submissionBuffer.AcquireNewObjectMaterial();
        mat->SetMatrix("Model", entry.ModelMatrix);
        mat->SetMatrix("ProjectionView", sunLight.ShadowViewProjection);
        mat->SetFloat3("ViewerPosition", glm::vec3(0.f));
        cmd.SetBindGroup(mat->GetBindGroup(), 1);

        const auto& meshSlices = submissionBuffer.GetMeshSlices(entry.Prop->Mesh.get());
        for (size_t j = 0; j < meshSlices.size(); ++j) {
            const auto& meshSlice = meshSlices[j];
            auto& propSlice = entry.Prop->Slices[j];

            auto pipeline = 
                BePipelineBuilder::Start(*entry.Prop->Shader)
                .SetCullMode(propSlice.TwoSided ? SenCullMode::None : SenCullMode::Back)
                .SetDepthFormat(sunLight.ShadowMap.lock()->Format)
                .Build();
            cmd.SetPipeline(pipeline);

            cmd.SetBindGroup(propSlice.Material->GetBindGroup(), 2);
            cmd.DrawIndexed(meshSlice.IndexCount, meshSlice.StartIndexLocation, meshSlice.BaseVertexLocation);
        }
    }
}

auto BeShadowPass::RenderPointLightShadows(
    const BeBRPPointLightEntry& pointLight,
    BeBRPSubmissionBuffer& submissionBuffer
) -> void {
    auto& cmd = _renderer->GetCommandBuffer();
    const auto uniformMat = submissionBuffer.UniformMaterial.lock();
    const auto& entries = submissionBuffer.GetGeometryEntries();
    auto shadowMapPtr = pointLight.ShadowMap.lock();
    
    cmd.SetVertexBuffer(submissionBuffer.GetSharedVertexBuffer(), sizeof(BeFullVertex));
    cmd.SetIndexBuffer(submissionBuffer.GetSharedIndexBuffer());
    SCOPE_EXIT {
        cmd.ClearVertexBuffer();
        cmd.ClearIndexBuffer();
    };

    cmd.SetBindGroup(uniformMat->GetBindGroup(), 0);

    for (int face = 0; face < 6; face++) {
        const glm::mat4x4 faceViewProj = CalculatePointLightFaceViewProjection(pointLight, face);
        
        cmd.BeginPass({
            .DepthAttachment = SenDepthAttachment{
                .Texture = shadowMapPtr->Handle,
                .CubemapFace = static_cast<int8_t>(face),
                .LoadOp = SenLoadOp::Clear
            },
            .Viewport = SenViewport { 
                .Width = (float)pointLight.ShadowMapResolution, 
                .Height = (float)pointLight.ShadowMapResolution, 
            },
        });
        SCOPE_EXIT { cmd.EndPass(); };

        for (const auto& entry : entries) {
            if (!entry.CastShadows)
                continue;

            const auto shader = entry.Prop->Shader;

            auto mat = submissionBuffer.AcquireNewObjectMaterial();
            mat->SetMatrix("Model", entry.ModelMatrix);
            mat->SetMatrix("ProjectionView", faceViewProj);
            mat->SetFloat3("ViewerPosition", pointLight.Position);
            cmd.SetBindGroup(mat->GetBindGroup(), 1);

            const auto& meshSlices = submissionBuffer.GetMeshSlices(entry.Prop->Mesh.get());
            for (size_t j = 0; j < meshSlices.size(); ++j) {
                const auto& meshSlice = meshSlices[j];
                auto& propSlice = entry.Prop->Slices[j];

                auto pipeline = 
                    BePipelineBuilder::Start(*entry.Prop->Shader)
                    .SetCullMode(propSlice.TwoSided ? SenCullMode::None : SenCullMode::Back)
                    .SetDepthFormat(shadowMapPtr->Format)
                    .Build();
                cmd.SetPipeline(pipeline);

                cmd.SetBindGroup(propSlice.Material->GetBindGroup(), 2);
                cmd.DrawIndexed(meshSlice.IndexCount, meshSlice.StartIndexLocation, meshSlice.BaseVertexLocation);
            }
        }
    }
}

auto BeShadowPass::CalculatePointLightFaceViewProjection(
    const BeBRPPointLightEntry& pointLight,
    const int faceIndex
) const -> glm::mat4 {
    static constexpr std::array<glm::vec3, 6> Forwards = {
        glm::vec3(1, 0, 0),
        glm::vec3(-1, 0, 0),
        glm::vec3(0, 1, 0),
        glm::vec3(0, -1, 0),
        glm::vec3(0, 0, 1),
        glm::vec3(0, 0, -1),
    };

    static constexpr std::array<glm::vec3, 6> Ups = {
        glm::vec3(0, 1, 0),
        glm::vec3(0, 1, 0),
        glm::vec3(0, 0, -1),
        glm::vec3(0, 0, 1),
        glm::vec3(0, 1, 0),
        glm::vec3(0, 1, 0),
    };

    glm::mat4x4 proj = glm::perspectiveLH_ZO(
      glm::radians(90.0f),
      1.0f,
      pointLight.ShadowNearPlane,
      pointLight.Radius
    );

    glm::vec3 lookAtPoint = pointLight.Position + Forwards[faceIndex];
    glm::mat4x4 view = glm::lookAtLH(pointLight.Position, lookAtPoint, Ups[faceIndex]);

    return proj * view;
}
