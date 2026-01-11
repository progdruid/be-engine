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
    context->OMSetRenderTargets(0, Utils::NullRTVs, sunLight.ShadowMap.lock()->GetDSV().Get());
    context->ClearDepthStencilView(sunLight.ShadowMap.lock()->GetDSV().Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
    SCOPE_EXIT { context->OMSetRenderTargets(0, nullptr, nullptr); };

    // Set vertex and index buffers
    uint32_t stride = sizeof(BeFullVertex);
    uint32_t offset = 0;
    context->IASetVertexBuffers(0, 1, _renderer->GetShaderVertexBuffer().GetAddressOf(), &stride, &offset);
    context->IASetIndexBuffer(_renderer->GetShaderIndexBuffer().Get(), DXGI_FORMAT_R32_UINT, 0);
    SCOPE_EXIT {
        context->IASetVertexBuffers(0, 1, Utils::NullBuffers, &stride, &offset);
        context->IASetIndexBuffer(nullptr, DXGI_FORMAT_R32_UINT, 0);
    };

    const auto& entries = submissionBuffer.GetGeometryEntries();
    for (const auto& entry : entries) {
        if (!entry.CastShadows)
            continue;

        pipeline->BindShader(entry.Model->Shader, BeShaderType::Vertex | BeShaderType::Tesselation);
        
        _objectMaterial->SetMatrix("Model", entry.ModelMatrix);
        _objectMaterial->SetMatrix("ProjectionView", sunLight.ShadowViewProjection);
        _objectMaterial->SetFloat3("ViewerPosition", glm::vec3(0.f));
        _objectMaterial->UpdateGPUBuffers(context);
        pipeline->BindMaterialAutomatic(_objectMaterial);
        
        const auto & drawSlices = _renderer->GetDrawSlicesForModel(entry.Model);
        for (const auto& slice : drawSlices) {
            pipeline->BindMaterialAutomatic(slice.Material);
            context->DrawIndexed(slice.IndexCount, slice.StartIndexLocation, slice.BaseVertexLocation);
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
    uint32_t stride = sizeof(BeFullVertex);
    uint32_t offset = 0;
    context->IASetVertexBuffers(0, 1, _renderer->GetShaderVertexBuffer().GetAddressOf(), &stride, &offset);
    context->IASetIndexBuffer(_renderer->GetShaderIndexBuffer().Get(), DXGI_FORMAT_R32_UINT, 0);
    SCOPE_EXIT {
        context->IASetVertexBuffers(0, 1, Utils::NullBuffers, &stride, &offset);
        context->IASetIndexBuffer(nullptr, DXGI_FORMAT_R32_UINT, 0);
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
        auto cubemapDSV = pointLight.ShadowMap.lock()->GetCubemapDSV(face);
        context->ClearDepthStencilView(cubemapDSV.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
        context->OMSetRenderTargets(0, nullptr, cubemapDSV.Get());

        const glm::mat4x4 faceViewProj = CalculatePointLightFaceViewProjection(pointLight, face);

        // for each object
        const auto& entries = submissionBuffer.GetGeometryEntries();
        for (const auto& entry : entries) {
            if (!entry.CastShadows)
                continue;

            pipeline->BindShader(entry.Model->Shader, BeShaderType::Vertex | BeShaderType::Tesselation);
            
            _objectMaterial->SetMatrix("Model", entry.ModelMatrix);
            _objectMaterial->SetMatrix("ProjectionView", faceViewProj);
            _objectMaterial->SetFloat3("ViewerPosition", pointLight.Position);
            _objectMaterial->UpdateGPUBuffers(context);
            pipeline->BindMaterialAutomatic(_objectMaterial);
            
            // draw
            const auto& drawSlices = _renderer->GetDrawSlicesForModel(entry.Model);
            for (const auto& slice : drawSlices) {
                pipeline->BindMaterialAutomatic(slice.Material);
                context->DrawIndexed(slice.IndexCount, slice.StartIndexLocation, slice.BaseVertexLocation);
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