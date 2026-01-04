#include "BeShadowPass.h"

#include <umbrellas/include-glm.h>
#include <scope_guard/scope_guard.hpp>

#include "BeAssetRegistry.h"
#include "BeModel.h"
#include "BePipeline.h"
#include "BeRenderer.h"
#include "BeTexture.h"

auto BeShadowPass::Initialise() -> void {
    D3D11_BUFFER_DESC objectBufferDescriptor = {};
    objectBufferDescriptor.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    objectBufferDescriptor.Usage = D3D11_USAGE_DYNAMIC;
    objectBufferDescriptor.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    objectBufferDescriptor.ByteWidth = sizeof(BeObjectBufferGPU);
    Utils::Check << _renderer->GetDevice()->CreateBuffer(&objectBufferDescriptor, nullptr, &_objectBuffer);
}

auto BeShadowPass::Render() -> void {
    const auto context = _renderer->GetContext();

    UINT numViewports = 1;
    D3D11_VIEWPORT previousViewport = {};
    context->RSGetViewports(&numViewports,  &previousViewport);
    SCOPE_EXIT { context->RSSetViewports(1, &previousViewport); };

    if (DirectionalLight.lock()->CastsShadows) {
        Utils::BeDebugAnnotation directionalLightAnnotation(context, "Directional Light Shadows");
        RenderDirectionalShadows();
    }

    for (size_t i = 0; i < PointLights.size(); i++) {
        if (!PointLights[i].CastsShadows)
            continue;

        Utils::BeDebugAnnotation pointLightAnnotation(context, "Point Light Shadows " + std::to_string(i));
        RenderPointLightShadows(PointLights[i]);
    }
}

auto BeShadowPass::RenderDirectionalShadows() -> void {
    const auto context = _renderer->GetContext();
    const auto& pipeline = _renderer->GetPipeline();
    const auto directionalLight = DirectionalLight.lock();

    // sort out viewport
    D3D11_VIEWPORT viewport = {};
    viewport.Width = directionalLight->ShadowMapResolution;
    viewport.Height = directionalLight->ShadowMapResolution;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    context->RSSetViewports(1, &viewport);

    // sort out render target
    context->OMSetRenderTargets(0, Utils::NullRTVs, directionalLight->ShadowMap->GetDSV().Get());
    context->ClearDepthStencilView(directionalLight->ShadowMap->GetDSV().Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
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

    const auto& entries = _renderer->GetDrawEntries();
    for (const auto& entry : entries) {
        if (!entry.CastShadows)
            continue;

        pipeline->BindShader(entry.Model->Shader, BeShaderType::Vertex | BeShaderType::Tesselation);
        
        const glm::mat4x4 modelMatrix =
            glm::translate(glm::mat4(1.0f), entry.Position) *
            glm::mat4_cast(entry.Rotation) *
            glm::scale(glm::mat4(1.0f), entry.Scale);
        
        BeObjectBufferGPU objectData(modelMatrix, directionalLight->ViewProjection, glm::vec3(0.f));
        D3D11_MAPPED_SUBRESOURCE objectMappedResource;
        Utils::Check << context->Map(_objectBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &objectMappedResource);
        memcpy(objectMappedResource.pData, &objectData, sizeof(BeObjectBufferGPU));
        context->Unmap(_objectBuffer.Get(), 0);
        context->VSSetConstantBuffers(1, 1, _objectBuffer.GetAddressOf());
        if (HasAny(entry.Model->Shader->ShaderType, BeShaderType::Tesselation)) {
            context->HSSetConstantBuffers(1, 1, _objectBuffer.GetAddressOf());
            context->DSSetConstantBuffers(1, 1, _objectBuffer.GetAddressOf());
        }

        const auto & drawSlices = _renderer->GetDrawSlicesForModel(entry.Model);
        for (const auto& slice : drawSlices) {
            pipeline->BindMaterialAutomatic(slice.Material);
            context->DrawIndexed(slice.IndexCount, slice.StartIndexLocation, slice.BaseVertexLocation);
        }

        pipeline->Clear();
    }
    
    context->VSSetConstantBuffers(1, 2, Utils::NullBuffers);
    context->HSSetConstantBuffers(1, 2, Utils::NullBuffers);
    context->DSSetConstantBuffers(1, 2, Utils::NullBuffers);
    
}

auto BeShadowPass::RenderPointLightShadows(const BePointLight& pointLight) -> void {

    // get what we need
    const auto context = _renderer->GetContext();
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
        auto cubemapDSV = pointLight.ShadowMap->GetCubemapDSV(face);
        context->ClearDepthStencilView(cubemapDSV.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
        context->OMSetRenderTargets(0, nullptr, cubemapDSV.Get());

        const glm::mat4x4 faceViewProj = CalculatePointLightFaceViewProjection(pointLight, face);

        // for each object
        const auto& entries = _renderer->GetDrawEntries();
        for (const auto& entry : entries) {
            if (!entry.CastShadows)
                continue;

            pipeline->BindShader(entry.Model->Shader, BeShaderType::Vertex | BeShaderType::Tesselation);
            
            // model matrix
            const glm::mat4x4 modelMatrix =
                glm::translate(glm::mat4(1.0f), entry.Position) *
                glm::mat4_cast(entry.Rotation) *
                glm::scale(glm::mat4(1.0f), entry.Scale);

            BeObjectBufferGPU objectData(modelMatrix, faceViewProj, pointLight.Position);
            D3D11_MAPPED_SUBRESOURCE objectMappedResource;
            Utils::Check << context->Map(_objectBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &objectMappedResource);
            memcpy(objectMappedResource.pData, &objectData, sizeof(BeObjectBufferGPU));
            context->Unmap(_objectBuffer.Get(), 0);
            context->VSSetConstantBuffers(1, 1, _objectBuffer.GetAddressOf());
            if (HasAny(entry.Model->Shader->ShaderType, BeShaderType::Tesselation)) {
                context->HSSetConstantBuffers(1, 1, _objectBuffer.GetAddressOf());
                context->DSSetConstantBuffers(1, 1, _objectBuffer.GetAddressOf());
            }
            
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

auto BeShadowPass::CalculatePointLightFaceViewProjection(const BePointLight& pointLight, const int faceIndex) -> glm::mat4 {
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