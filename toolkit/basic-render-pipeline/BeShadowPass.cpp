#include "BeShadowPass.h"

#include <umbrellas/include-glm.h>
#include <scope_guard/scope_guard.hpp>
#include <sen-rhi/SenBackend.h>

#include "BeAssetRegistry.h"
#include "BeBRPSubmissionBuffer.h"
#include "BeMaterial.h"
#include "BePipeline.h"
#include "BeRenderer.h"
#include "BeShader.h"
#include "BeTexture.h"
#include "Utils.h"

auto BeShadowPass::Initialise() -> void {
    auto objectScheme = BeAssetRegistry::GetMaterialScheme("object-material-for-geometry-pass");
    _objectMaterial = BeMaterial::Create("object", objectScheme, true, *_renderer);
}

auto BeShadowPass::Render() -> void {
    const auto context = _renderer->GetContext();
    const auto& submissionBuffer = *SubmissionBuffer.lock();

    const auto& sunLights = submissionBuffer.GetSunLightEntries();
    for (size_t i = 0; i < sunLights.size(); ++i) {
        if (!sunLights[i].CastsShadows)
            continue;

        Utils::BeDebugAnnotation directionalLightAnnotation(context, "Directional Light Shadows " + std::to_string(i));
        RenderDirectionalShadows(sunLights[0], submissionBuffer);
    }

    const auto& pointLights = submissionBuffer.GetPointLightEntries();
    for (size_t i = 0; i < pointLights.size(); i++) {
        if (!pointLights[i].CastsShadows)
            continue;

        Utils::BeDebugAnnotation pointLightAnnotation(context, "Point Light Shadows " + std::to_string(i));
        RenderPointLightShadows(pointLights[i], submissionBuffer);
    }
}

auto BeShadowPass::RenderDirectionalShadows(
    const BeBRPSunLightEntry& sunLight,
    const BeBRPSubmissionBuffer& submissionBuffer
) -> void {
    const auto& context = _renderer->GetContext();
    const auto& pipeline = _renderer->GetPipeline();

    // Begin pass with depth-only target
    SenBackend::BeginPass({
        .DepthAttachment = SenDepthAttachment{ sunLight.ShadowMap.lock()->Handle },
        .Viewport = { 0, 0, (float)sunLight.ShadowMapResolution, (float)sunLight.ShadowMapResolution, 0, 1 },
    });
    SCOPE_EXIT { SenBackend::EndPass(); };

    // Set vertex and index buffers
    pipeline->BindVertexBuffer(submissionBuffer.GetSharedVertexBuffer(), sizeof(BeFullVertex));
    pipeline->BindIndexBuffer(submissionBuffer.GetSharedIndexBuffer());
    SCOPE_EXIT {
        pipeline->ClearVertexBuffer();
        pipeline->ClearIndexBuffer();
    };

    const auto& entries = submissionBuffer.GetGeometryEntries();
    for (const auto& entry : entries) {
        if (!entry.CastShadows)
            continue;

        const auto shader = entry.Prop->Shader;
        // Get or create pipeline for this shader
        if (!_shaderPipelines.contains(shader.get())) {
            auto pipelineDesc = shader->CreatePipelineDesc();
            _shaderPipelines[shader.get()] = SenBackend::CreatePipeline(pipelineDesc);
            _objectBindings[shader.get()].Make(_objectMaterial, shader);
        }
        pipeline->BindPipeline(_shaderPipelines[shader.get()]);

        _objectMaterial->SetMatrix("Model", entry.ModelMatrix);
        _objectMaterial->SetMatrix("ProjectionView", sunLight.ShadowViewProjection);
        _objectMaterial->SetFloat3("ViewerPosition", glm::vec3(0.f));
        pipeline->SetBindGroup(_objectBindings[shader.get()].Resolve(), 1);

        const auto& meshSlices = submissionBuffer.GetMeshSlices(entry.Prop->Mesh.get());
        for (size_t i = 0; i < meshSlices.size(); ++i) {
            const auto& meshSlice = meshSlices[i];
            auto& propSlice = entry.Prop->Slices[i];

            if (propSlice.TwoSided) {
                context->RSSetState(_renderer->GetRasterizerCullNone().Get());
            }

            pipeline->SetBindGroup(propSlice.Binding.Resolve(), 2);
            context->DrawIndexed(meshSlice.IndexCount, meshSlice.StartIndexLocation, meshSlice.BaseVertexLocation);

            if (propSlice.TwoSided) {
                context->RSSetState(_renderer->GetRasterizerCullBack().Get());
            }
        }
    }
}

auto BeShadowPass::RenderPointLightShadows(
    const BeBRPPointLightEntry& pointLight,
    const BeBRPSubmissionBuffer& submissionBuffer
) -> void {
    // get what we need
    const auto& context = _renderer->GetContext();
    const auto& pipeline = _renderer->GetPipeline();

    // sort out vertex and index buffers
    pipeline->BindVertexBuffer(submissionBuffer.GetSharedVertexBuffer(), sizeof(BeFullVertex));
    pipeline->BindIndexBuffer(submissionBuffer.GetSharedIndexBuffer());
    SCOPE_EXIT {
        pipeline->ClearVertexBuffer();
        pipeline->ClearIndexBuffer();
    };

    auto shadowMapPtr = pointLight.ShadowMap.lock();

    // render each face
    for (int face = 0; face < 6; face++) {
        // Begin pass for this cubemap face
        SenBackend::BeginPass({
            .DepthAttachment = SenDepthAttachment{
                shadowMapPtr->Handle,
                static_cast<int8_t>(face),
                SenLoadOp::Clear
            },
            .Viewport = { 0, 0, (float)pointLight.ShadowMapResolution, (float)pointLight.ShadowMapResolution, 0, 1 },
        });
        SCOPE_EXIT { SenBackend::EndPass(); };

        const glm::mat4x4 faceViewProj = CalculatePointLightFaceViewProjection(pointLight, face);

        // for each object
        const auto& entries = submissionBuffer.GetGeometryEntries();
        for (const auto& entry : entries) {
            if (!entry.CastShadows)
                continue;

            const auto shader = entry.Prop->Shader;
            // Get or create pipeline for this shader
            if (!_shaderPipelines.contains(shader.get())) {
                auto pipelineDesc = shader->CreatePipelineDesc();
                _shaderPipelines[shader.get()] = SenBackend::CreatePipeline(pipelineDesc);
                _objectBindings[shader.get()].Make(_objectMaterial, shader);
            }
            pipeline->BindPipeline(_shaderPipelines[shader.get()]);

            _objectMaterial->SetMatrix("Model", entry.ModelMatrix);
            _objectMaterial->SetMatrix("ProjectionView", faceViewProj);
            _objectMaterial->SetFloat3("ViewerPosition", pointLight.Position);
            pipeline->SetBindGroup(_objectBindings[shader.get()].Resolve(), 1);

            // draw
            const auto& meshSlices = submissionBuffer.GetMeshSlices(entry.Prop->Mesh.get());
            for (size_t i = 0; i < meshSlices.size(); ++i) {
                const auto& meshSlice = meshSlices[i];
                auto& propSlice = entry.Prop->Slices[i];

                if (propSlice.TwoSided) {
                    context->RSSetState(_renderer->GetRasterizerCullNone().Get());
                }

                pipeline->SetBindGroup(propSlice.Binding.Resolve(), 2);
                context->DrawIndexed(meshSlice.IndexCount, meshSlice.StartIndexLocation, meshSlice.BaseVertexLocation);

                if (propSlice.TwoSided) {
                    context->RSSetState(_renderer->GetRasterizerCullBack().Get());
                }
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
