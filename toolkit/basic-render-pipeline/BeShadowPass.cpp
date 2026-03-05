#include "BeShadowPass.h"

#include <umbrellas/include-glm.h>
#include <scope_guard/scope_guard.hpp>

#include "BeAssetRegistry.h"
#include "BeBRPSubmissionBuffer.h"
#include "BeMaterial.h"
#include "BePipeline.h"
#include "BeRenderer.h"
#include "BeTexture.h"
#include <sen-rhi/dx11/SenDx11Backend.h>

auto BeShadowPass::Initialise() -> void {
    auto objectScheme = BeAssetRegistry::GetMaterialScheme("object-material-for-geometry-pass");
    _objectMaterial = BeMaterial::Create("object", objectScheme, true, *_renderer);
}

auto BeShadowPass::Render() -> void {
    const auto context = _renderer->GetContext();

    UINT numViewports = 1;
    D3D11_VIEWPORT previousViewport = {};
    context->RSGetViewports(&numViewports,  &previousViewport);
    SCOPE_EXIT { context->RSSetViewports(1, &previousViewport); };

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
) const -> void {
    const auto& context = _renderer->GetContext();
    const auto& pipeline = _renderer->GetPipeline();

    // sort out viewport
    D3D11_VIEWPORT viewport = {};
    viewport.Width = sunLight.ShadowMapResolution;
    viewport.Height = sunLight.ShadowMapResolution;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    context->RSSetViewports(1, &viewport);

    // sort out render target
    pipeline->BindTargets({}, sunLight.ShadowMap.lock().get(), true);
    SCOPE_EXIT { pipeline->ClearTargets(); };

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

        pipeline->BindShader(entry.Prop->Shader, BeShaderType::Vertex | BeShaderType::Tesselation);

        _objectMaterial->SetMatrix("Model", entry.ModelMatrix);
        _objectMaterial->SetMatrix("ProjectionView", sunLight.ShadowViewProjection);
        _objectMaterial->SetFloat3("ViewerPosition", glm::vec3(0.f));
        _objectMaterial->UpdateGPUBuffers();
        pipeline->BindMaterialAutomatic(_objectMaterial);

        const auto& meshSlices = submissionBuffer.GetMeshSlices(entry.Prop->Mesh.get());
        for (size_t i = 0; i < meshSlices.size(); ++i) {
            const auto& meshSlice = meshSlices[i];
            const auto& propSlice = entry.Prop->Slices[i];

            if (propSlice.TwoSided) {
                context->RSSetState(_renderer->GetRasterizerCullNone().Get());
            }

            pipeline->BindMaterialAutomatic(propSlice.Material);
            context->DrawIndexed(meshSlice.IndexCount, meshSlice.StartIndexLocation, meshSlice.BaseVertexLocation);

            if (propSlice.TwoSided) {
                context->RSSetState(_renderer->GetRasterizerCullBack().Get());
            }
        }

        pipeline->Clear();
    }
}

auto BeShadowPass::RenderPointLightShadows(
    const BeBRPPointLightEntry& pointLight,
    const BeBRPSubmissionBuffer& submissionBuffer
) const -> void {
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

    // sort out viewport
    D3D11_VIEWPORT viewport = {};
    viewport.Width = pointLight.ShadowMapResolution;
    viewport.Height = pointLight.ShadowMapResolution;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    context->RSSetViewports(1, &viewport);

    // render each face
    for (int face = 0; face < 6; face++) {
        // sort out render target
        auto shadowMapPtr = pointLight.ShadowMap.lock();
        auto cubemapDSV = SenDx11Backend::Get().LookupTexture(shadowMapPtr->Handle).CubemapDSVs[face].Get();
        context->ClearDepthStencilView(cubemapDSV, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
        context->OMSetRenderTargets(0, nullptr, cubemapDSV);

        const glm::mat4x4 faceViewProj = CalculatePointLightFaceViewProjection(pointLight, face);

        // for each object
        const auto& entries = submissionBuffer.GetGeometryEntries();
        for (const auto& entry : entries) {
            if (!entry.CastShadows)
                continue;

            pipeline->BindShader(entry.Prop->Shader, BeShaderType::Vertex | BeShaderType::Tesselation);

            _objectMaterial->SetMatrix("Model", entry.ModelMatrix);
            _objectMaterial->SetMatrix("ProjectionView", faceViewProj);
            _objectMaterial->SetFloat3("ViewerPosition", pointLight.Position);
            _objectMaterial->UpdateGPUBuffers();
            pipeline->BindMaterialAutomatic(_objectMaterial);

            // draw
            const auto& meshSlices = submissionBuffer.GetMeshSlices(entry.Prop->Mesh.get());
            for (size_t i = 0; i < meshSlices.size(); ++i) {
                const auto& meshSlice = meshSlices[i];
                const auto& propSlice = entry.Prop->Slices[i];

                if (propSlice.TwoSided) {
                    context->RSSetState(_renderer->GetRasterizerCullNone().Get());
                }

                pipeline->BindMaterialAutomatic(propSlice.Material);
                context->DrawIndexed(meshSlice.IndexCount, meshSlice.StartIndexLocation, meshSlice.BaseVertexLocation);

                if (propSlice.TwoSided) {
                    context->RSSetState(_renderer->GetRasterizerCullBack().Get());
                }
            }

            pipeline->Clear();
        }
    }

    context->OMSetRenderTargets(0, nullptr, nullptr);
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
