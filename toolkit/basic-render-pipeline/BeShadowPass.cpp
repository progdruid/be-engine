#include "BeShadowPass.h"

#include <umbrellas/include-glm.h>
#include <scope_guard/scope_guard.hpp>
#include <sen-rhi/SenBackend.h>

#include "BeAssetRegistry.h"
#include "BeBRPSubmissionBuffer.h"
#include "BeMaterial.h"
#include "BeRenderer.h"
#include "BeShader.h"
#include "BeTexture.h"

auto BeShadowPass::Initialise() -> void {}

auto BeShadowPass::Render() -> void {
    const auto& submissionBuffer = *SubmissionBuffer.lock();

    const auto& sunLights = submissionBuffer.GetSunLightEntries();
    for (size_t i = 0; i < sunLights.size(); ++i) {
        if (!sunLights[i].CastsShadows)
            continue;
        RenderDirectionalShadows(sunLights[0], submissionBuffer);
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
    const BeBRPSubmissionBuffer& submissionBuffer
) -> void {
    auto& cmd = _renderer->GetCommandBuffer();
    const auto uniformMat = submissionBuffer.UniformMaterial.lock();
    const auto& entries = submissionBuffer.GetGeometryEntries();
    auto objectScheme = BeAssetRegistry::GetMaterialScheme("object-material-for-geometry-pass");

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

        const auto shader = entry.Prop->Shader;

        if (!_objectMaterials.contains(entry.Name)) {
            _objectMaterials[entry.Name] = BeMaterial::Create("shadow_" + entry.Name, objectScheme, true, *_renderer);
        }
        auto& mat = _objectMaterials[entry.Name];
        mat->SetMatrix("Model", entry.ModelMatrix);
        mat->SetMatrix("ProjectionView", sunLight.ShadowViewProjection);
        mat->SetFloat3("ViewerPosition", glm::vec3(0.f));
        cmd.SetBindGroup(mat->GetBindGroup(), 1);

        const auto& meshSlices = submissionBuffer.GetMeshSlices(entry.Prop->Mesh.get());
        for (size_t j = 0; j < meshSlices.size(); ++j) {
            const auto& meshSlice = meshSlices[j];
            auto& propSlice = entry.Prop->Slices[j];

            PipelineKey key{ shader.get(), propSlice.TwoSided };
            if (!_shaderPipelines.contains(key)) {
                auto pipelineDesc = shader->GetPipelineDesc();
                pipelineDesc.RasterizerState.CullMode = propSlice.TwoSided ? SenCullMode::None : SenCullMode::Back;
                pipelineDesc.RenderTargetFormats = {};
                pipelineDesc.DepthStencilFormat = sunLight.ShadowMap.lock()->Format;
                _shaderPipelines[key] = SenBackend::CreatePipeline(pipelineDesc);
            }
            cmd.SetPipeline(_shaderPipelines[key]);

            cmd.SetBindGroup(propSlice.Material->GetBindGroup(), 2);
            cmd.DrawIndexed(meshSlice.IndexCount, meshSlice.StartIndexLocation, meshSlice.BaseVertexLocation);
        }
    }
}

auto BeShadowPass::RenderPointLightShadows(
    const BeBRPPointLightEntry& pointLight,
    const BeBRPSubmissionBuffer& submissionBuffer
) -> void {
    auto& cmd = _renderer->GetCommandBuffer();
    const auto uniformMat = submissionBuffer.UniformMaterial.lock();
    const auto& entries = submissionBuffer.GetGeometryEntries();
    auto shadowMapPtr = pointLight.ShadowMap.lock();
    auto objectScheme = BeAssetRegistry::GetMaterialScheme("object-material-for-geometry-pass");

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
                shadowMapPtr->Handle,
                static_cast<int8_t>(face),
                SenLoadOp::Clear
            },
            .Viewport = { 0, 0, (float)pointLight.ShadowMapResolution, (float)pointLight.ShadowMapResolution, 0, 1 },
        });
        SCOPE_EXIT { cmd.EndPass(); };

        for (const auto& entry : entries) {
            if (!entry.CastShadows)
                continue;

            const auto shader = entry.Prop->Shader;

            if (!_objectMaterials.contains(entry.Name)) {
                _objectMaterials[entry.Name] = BeMaterial::Create("shadow_" + entry.Name, objectScheme, true, *_renderer);
            }
            auto& mat = _objectMaterials[entry.Name];
            mat->SetMatrix("Model", entry.ModelMatrix);
            mat->SetMatrix("ProjectionView", faceViewProj);
            mat->SetFloat3("ViewerPosition", pointLight.Position);
            cmd.SetBindGroup(mat->GetBindGroup(), 1);

            const auto& meshSlices = submissionBuffer.GetMeshSlices(entry.Prop->Mesh.get());
            for (size_t j = 0; j < meshSlices.size(); ++j) {
                const auto& meshSlice = meshSlices[j];
                auto& propSlice = entry.Prop->Slices[j];

                PipelineKey key{ shader.get(), propSlice.TwoSided };
                if (!_shaderPipelines.contains(key)) {
                    auto pipelineDesc = shader->GetPipelineDesc();
                    pipelineDesc.RasterizerState.CullMode = propSlice.TwoSided ? SenCullMode::None : SenCullMode::Back;
                    pipelineDesc.RenderTargetFormats = {};
                    pipelineDesc.DepthStencilFormat = shadowMapPtr->Format;
                    _shaderPipelines[key] = SenBackend::CreatePipeline(pipelineDesc);
                }
                cmd.SetPipeline(_shaderPipelines[key]);

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
