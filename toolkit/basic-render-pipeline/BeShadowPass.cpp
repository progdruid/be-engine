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

auto BeShadowPass::Initialise() -> void {
    auto objectScheme = BeAssetRegistry::GetMaterialScheme("object-material-for-geometry-pass");
    _objectMaterial = BeMaterial::Create("object", objectScheme, true, *_renderer);
}

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

    const auto& entries = submissionBuffer.GetGeometryEntries();
    for (const auto& entry : entries) {
        if (!entry.CastShadows)
            continue;

        const auto shader = entry.Prop->Shader;

        if (!_objectBindings.contains(shader.get())) {
            _objectBindings[shader.get()].Make(_objectMaterial, shader);
        }

        _objectMaterial->SetMatrix("Model", entry.ModelMatrix);
        _objectMaterial->SetMatrix("ProjectionView", sunLight.ShadowViewProjection);
        _objectMaterial->SetFloat3("ViewerPosition", glm::vec3(0.f));
        cmd.SetBindGroup(_objectBindings[shader.get()].Resolve(), 1);

        const auto& meshSlices = submissionBuffer.GetMeshSlices(entry.Prop->Mesh.get());
        for (size_t i = 0; i < meshSlices.size(); ++i) {
            const auto& meshSlice = meshSlices[i];
            auto& propSlice = entry.Prop->Slices[i];

            PipelineKey key{ shader.get(), propSlice.TwoSided };
            if (!_shaderPipelines.contains(key)) {
                auto pipelineDesc = shader->CreatePipelineDesc();
                pipelineDesc.RasterizerState.CullMode = propSlice.TwoSided ? SenCullMode::None : SenCullMode::Back;
                pipelineDesc.BindGroupLayouts = {
                    _renderer->GetUniformBindGroupLayout(),          // set 0: renderer uniforms
                    _objectBindings[shader.get()].GetLayout(),       // set 1: object material
                    propSlice.Binding.GetLayout(),                   // set 2: surface material
                };
                _shaderPipelines[key] = SenBackend::CreatePipeline(pipelineDesc);
            }
            cmd.SetPipeline(_shaderPipelines[key]);

            cmd.SetBindGroup(propSlice.Binding.Resolve(), 2);
            cmd.DrawIndexed(meshSlice.IndexCount, meshSlice.StartIndexLocation, meshSlice.BaseVertexLocation);
        }
    }
}

auto BeShadowPass::RenderPointLightShadows(
    const BeBRPPointLightEntry& pointLight,
    const BeBRPSubmissionBuffer& submissionBuffer
) -> void {
    auto& cmd = _renderer->GetCommandBuffer();

    cmd.SetVertexBuffer(submissionBuffer.GetSharedVertexBuffer(), sizeof(BeFullVertex));
    cmd.SetIndexBuffer(submissionBuffer.GetSharedIndexBuffer());
    SCOPE_EXIT {
        cmd.ClearVertexBuffer();
        cmd.ClearIndexBuffer();
    };

    auto shadowMapPtr = pointLight.ShadowMap.lock();

    for (int face = 0; face < 6; face++) {
        cmd.BeginPass({
            .DepthAttachment = SenDepthAttachment{
                shadowMapPtr->Handle,
                static_cast<int8_t>(face),
                SenLoadOp::Clear
            },
            .Viewport = { 0, 0, (float)pointLight.ShadowMapResolution, (float)pointLight.ShadowMapResolution, 0, 1 },
        });
        SCOPE_EXIT { cmd.EndPass(); };

        const glm::mat4x4 faceViewProj = CalculatePointLightFaceViewProjection(pointLight, face);

        const auto& entries = submissionBuffer.GetGeometryEntries();
        for (const auto& entry : entries) {
            if (!entry.CastShadows)
                continue;

            const auto shader = entry.Prop->Shader;

            if (!_objectBindings.contains(shader.get())) {
                _objectBindings[shader.get()].Make(_objectMaterial, shader);
            }

            _objectMaterial->SetMatrix("Model", entry.ModelMatrix);
            _objectMaterial->SetMatrix("ProjectionView", faceViewProj);
            _objectMaterial->SetFloat3("ViewerPosition", pointLight.Position);
            cmd.SetBindGroup(_objectBindings[shader.get()].Resolve(), 1);

            const auto& meshSlices = submissionBuffer.GetMeshSlices(entry.Prop->Mesh.get());
            for (size_t i = 0; i < meshSlices.size(); ++i) {
                const auto& meshSlice = meshSlices[i];
                auto& propSlice = entry.Prop->Slices[i];

                PipelineKey key{ shader.get(), propSlice.TwoSided };
                if (!_shaderPipelines.contains(key)) {
                    auto pipelineDesc = shader->CreatePipelineDesc();
                    pipelineDesc.RasterizerState.CullMode = propSlice.TwoSided ? SenCullMode::None : SenCullMode::Back;
                    pipelineDesc.BindGroupLayouts = {
                        _renderer->GetUniformBindGroupLayout(),          // set 0: renderer uniforms
                        _objectBindings[shader.get()].GetLayout(),       // set 1: object material
                        propSlice.Binding.GetLayout(),                   // set 2: surface material
                    };
                    _shaderPipelines[key] = SenBackend::CreatePipeline(pipelineDesc);
                }
                cmd.SetPipeline(_shaderPipelines[key]);

                cmd.SetBindGroup(propSlice.Binding.Resolve(), 2);
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
