#include "BeShadowPass.h"

#include <umbrellas/include-glm.h>
#include <scope_guard/scope_guard.hpp>

#include "BeAssetRegistry.h"
#include "BeBRPSubmissionBuffer.h"
#include "BeMaterial.h"
#include "BeModel.h"
#include "BePipeline.h"
#include "BeRenderer.h"
#include "BeTexture.h"

auto BeShadowPass::Initialise() -> void {
    auto objectScheme = BeAssetRegistry::GetMaterialScheme("object-material-for-geometry-pass");
    _objectMaterial = BeMaterial::Create("object", objectScheme, true, *_renderer);
}

auto BeShadowPass::Render() -> void {
    const auto& pipeline = _renderer->GetPipeline();
    const auto previousViewport = pipeline->GetViewport();
    SCOPE_EXIT { pipeline->SetViewport(previousViewport); };

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
) const -> void {
    const auto& pipeline = _renderer->GetPipeline();

    BeViewport viewport;
    viewport.Width = static_cast<float>(sunLight.ShadowMapResolution);
    viewport.Height = static_cast<float>(sunLight.ShadowMapResolution);
    pipeline->SetViewport(viewport);

    auto* shadowMap = sunLight.ShadowMap.lock().get();
    pipeline->ClearDepthTarget(shadowMap);
    pipeline->SetDepthOnlyTarget(shadowMap);
    SCOPE_EXIT { pipeline->ClearTargets(); };

    pipeline->BindMeshBuffers(submissionBuffer);
    SCOPE_EXIT { pipeline->UnbindMeshBuffers(); };

    const auto& entries = submissionBuffer.GetGeometryEntries();
    for (const auto& entry : entries) {
        if (!entry.CastShadows)
            continue;

        pipeline->BindShader(entry.Model->Shader, BeShaderType::Vertex | BeShaderType::Tesselation);

        _objectMaterial->SetMatrix("Model", entry.ModelMatrix);
        _objectMaterial->SetMatrix("ProjectionView", sunLight.ShadowViewProjection);
        _objectMaterial->SetFloat3("ViewerPosition", glm::vec3(0.f));
        pipeline->UpdateMaterialBuffers(_objectMaterial);
        pipeline->BindMaterialAutomatic(_objectMaterial);

        const auto& drawSlices = submissionBuffer.GetDrawSlicesForModel(entry.Model);
        for (const auto& slice : drawSlices) {
            if (slice.TwoSided)
                pipeline->SetCullMode(BeCullMode::None);

            pipeline->BindMaterialAutomatic(slice.Material);
            pipeline->DrawIndexed(slice.IndexCount, slice.StartIndexLocation, slice.BaseVertexLocation);

            if (slice.TwoSided)
                pipeline->SetCullMode(BeCullMode::Back);
        }

        pipeline->Clear();
    }
}

auto BeShadowPass::RenderPointLightShadows(
    const BeBRPPointLightEntry& pointLight,
    const BeBRPSubmissionBuffer& submissionBuffer
) const -> void {
    const auto& pipeline = _renderer->GetPipeline();

    pipeline->BindMeshBuffers(submissionBuffer);
    SCOPE_EXIT { pipeline->UnbindMeshBuffers(); };

    BeViewport viewport;
    viewport.Width = static_cast<float>(pointLight.ShadowMapResolution);
    viewport.Height = static_cast<float>(pointLight.ShadowMapResolution);
    pipeline->SetViewport(viewport);

    auto* shadowMap = pointLight.ShadowMap.lock().get();

    for (int face = 0; face < 6; face++) {
        pipeline->ClearDepthTarget(shadowMap);
        pipeline->SetCubemapDepthTarget(shadowMap, face);

        const glm::mat4x4 faceViewProj = CalculatePointLightFaceViewProjection(pointLight, face);

        const auto& entries = submissionBuffer.GetGeometryEntries();
        for (const auto& entry : entries) {
            if (!entry.CastShadows)
                continue;

            pipeline->BindShader(entry.Model->Shader, BeShaderType::Vertex | BeShaderType::Tesselation);

            _objectMaterial->SetMatrix("Model", entry.ModelMatrix);
            _objectMaterial->SetMatrix("ProjectionView", faceViewProj);
            _objectMaterial->SetFloat3("ViewerPosition", pointLight.Position);
            pipeline->UpdateMaterialBuffers(_objectMaterial);
            pipeline->BindMaterialAutomatic(_objectMaterial);

            const auto& drawSlices = submissionBuffer.GetDrawSlicesForModel(entry.Model);
            for (const auto& slice : drawSlices) {
                if (slice.TwoSided)
                    pipeline->SetCullMode(BeCullMode::None);

                pipeline->BindMaterialAutomatic(slice.Material);
                pipeline->DrawIndexed(slice.IndexCount, slice.StartIndexLocation, slice.BaseVertexLocation);

                if (slice.TwoSided)
                    pipeline->SetCullMode(BeCullMode::Back);
            }

            pipeline->Clear();
        }
    }

    pipeline->ClearTargets();
}

auto BeShadowPass::CalculatePointLightFaceViewProjection(
    const BeBRPPointLightEntry& pointLight,
    const int faceIndex
) const -> glm::mat4 {
    static constexpr std::array<glm::vec3, 6> Forwards = {
        glm::vec3(1, 0, 0),  glm::vec3(-1, 0, 0),
        glm::vec3(0, 1, 0),  glm::vec3(0, -1, 0),
        glm::vec3(0, 0, 1),  glm::vec3(0, 0, -1),
    };

    static constexpr std::array<glm::vec3, 6> Ups = {
        glm::vec3(0, 1, 0),  glm::vec3(0, 1, 0),
        glm::vec3(0, 0, -1), glm::vec3(0, 0, 1),
        glm::vec3(0, 1, 0),  glm::vec3(0, 1, 0),
    };

    glm::mat4x4 proj = glm::perspectiveLH_ZO(glm::radians(90.0f), 1.0f, pointLight.ShadowNearPlane, pointLight.Radius);
    glm::vec3 lookAtPoint = pointLight.Position + Forwards[faceIndex];
    glm::mat4x4 view = glm::lookAtLH(pointLight.Position, lookAtPoint, Ups[faceIndex]);

    return proj * view;
}
