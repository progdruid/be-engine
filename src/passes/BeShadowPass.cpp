#include "BeShadowPass.h"

#include <umbrellas/include-glm.h>
#include <scope_guard/scope_guard.hpp>

#include "BeRenderer.h"

auto BeShadowPass::Initialise() -> void {
    D3D11_BUFFER_DESC shadowBufferDescriptor = {};
    shadowBufferDescriptor.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    shadowBufferDescriptor.Usage = D3D11_USAGE_DYNAMIC;
    shadowBufferDescriptor.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    shadowBufferDescriptor.ByteWidth = sizeof(BeShadowpassBufferGPU);
    Utils::Check << _renderer->GetDevice()->CreateBuffer(&shadowBufferDescriptor, nullptr, &_shadowConstantBuffer);
    
    _directionalShadowShader = BeShader::Create(_renderer->GetDevice().Get(), "assets/shaders/directionalShadow");
    _pointShadowShader = BeShader::Create(_renderer->GetDevice().Get(), "assets/shaders/pointShadow");
}

auto BeShadowPass::Render() -> void {
    const auto context = _renderer->GetContext();

    UINT numViewports = 1;
    D3D11_VIEWPORT previousViewport = {};
    context->RSGetViewports(&numViewports,  &previousViewport);
    SCOPE_EXIT { context->RSSetViewports(1, &previousViewport); };

    {
        Utils::BeDebugAnnotation directionalLightAnnotation(context, "Directional Light Shadows");
        RenderDirectionalShadows();
    }

    const std::vector<BePointLight>& pointLights = *_renderer->GetContextDataPointer<std::vector<BePointLight>>(InputPointLightsName);

    for (size_t i = 0; i < pointLights.size(); i++) {
        if (!pointLights[i].CastsShadows)
            continue;

        Utils::BeDebugAnnotation pointLightAnnotation(context, "Point Light Shadows " + std::to_string(i));
        RenderPointLightShadows(pointLights[i]);
    }
}

auto BeShadowPass::RenderDirectionalShadows() -> void {
    const auto context = _renderer->GetContext();
    const auto& directionalLight = *_renderer->GetContextDataPointer<BeDirectionalLight>(InputDirectionalLightName);
    const auto directionalShadowMap = _renderer->GetRenderResource(directionalLight.ShadowMapTextureName);

    // sort out viewport
    D3D11_VIEWPORT viewport = {};
    viewport.Width = directionalLight.ShadowMapResolution;
    viewport.Height = directionalLight.ShadowMapResolution;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    context->RSSetViewports(1, &viewport);

    // sort out render target
    context->OMSetRenderTargets(0, Utils::NullRTVs, directionalShadowMap->DSV.Get());
    context->ClearDepthStencilView(directionalShadowMap->DSV.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
    SCOPE_EXIT { context->OMSetRenderTargets(0, nullptr, nullptr); };

    // sort out shader
    _directionalShadowShader->Bind(context.Get(), BeShaderType::Vertex);
    SCOPE_EXIT { BeShader::Unbind(context.Get(), BeShaderType::Vertex); };

    // Set vertex and index buffers
    uint32_t stride = sizeof(BeFullVertex);
    uint32_t offset = 0;
    context->IASetVertexBuffers(0, 1, _renderer->GetShaderVertexBuffer().GetAddressOf(), &stride, &offset);
    context->IASetIndexBuffer(_renderer->GetShaderIndexBuffer().Get(), DXGI_FORMAT_R32_UINT, 0);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    SCOPE_EXIT {
        context->IASetVertexBuffers(0, 1, Utils::NullBuffers, &stride, &offset);
        context->IASetIndexBuffer(nullptr, DXGI_FORMAT_R32_UINT, 0);
    };
    
    const auto& objects = _renderer->GetObjects();
    for (const auto& object : objects) {
        const glm::mat4x4 modelMatrix =
            glm::translate(glm::mat4(1.0f), object.Position) *
            glm::mat4_cast(object.Rotation) *
            glm::scale(glm::mat4(1.0f), object.Scale);
        
        BeShadowpassBufferGPU shadowpassBufferGPU;
        shadowpassBufferGPU.LightProjectionView = directionalLight.ViewProjection;
        shadowpassBufferGPU.ObjectModel = modelMatrix;
        shadowpassBufferGPU.LightPosition = glm::vec3(0.0f);
        D3D11_MAPPED_SUBRESOURCE mappedResource;
        Utils::Check << context->Map(_shadowConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
        memcpy(mappedResource.pData, &shadowpassBufferGPU, sizeof(BeShadowpassBufferGPU));
        context->Unmap(_shadowConstantBuffer.Get(), 0);
        context->VSSetConstantBuffers(1, 1, _shadowConstantBuffer.GetAddressOf());

        for (const auto& slice : object.DrawSlices) {
            context->DrawIndexed(slice.IndexCount, slice.StartIndexLocation, slice.BaseVertexLocation);
        }
    }
    context->VSSetConstantBuffers(1, 1, Utils::NullBuffers);
}

auto BeShadowPass::RenderPointLightShadows(const BePointLight& pointLight) -> void {

    // get what we need
    const auto context = _renderer->GetContext();
    const auto pointShadowMap = _renderer->GetRenderResource(pointLight.ShadowMapTextureName);
    
    // sort out shader
    _pointShadowShader->Bind(context.Get(), BeShaderType::Vertex);
    SCOPE_EXIT { BeShader::Unbind(context.Get(), BeShaderType::Vertex); };
    
    // sort out vertex and index buffers
    uint32_t stride = sizeof(BeFullVertex);
    uint32_t offset = 0;
    context->IASetVertexBuffers(0, 1, _renderer->GetShaderVertexBuffer().GetAddressOf(), &stride, &offset);
    context->IASetIndexBuffer(_renderer->GetShaderIndexBuffer().Get(), DXGI_FORMAT_R32_UINT, 0);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
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
        auto cubemapDSV = pointShadowMap->GetCubemapDSV(face);
        context->ClearDepthStencilView(cubemapDSV.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
        context->OMSetRenderTargets(0, nullptr, cubemapDSV.Get());

        const glm::mat4x4 faceViewProj = CalculatePointLightFaceViewProjection(pointLight, face);

        // for each object
        const auto& objects = _renderer->GetObjects();
        for (const auto& object : objects) {

            // model matrix
            const glm::mat4x4 modelMatrix =
                glm::translate(glm::mat4(1.0f), object.Position) *
                glm::mat4_cast(object.Rotation) *
                glm::scale(glm::mat4(1.0f), object.Scale);

            // sort out point light constant buffer
            BeShadowpassBufferGPU shadowpassBufferGPU;
            shadowpassBufferGPU.LightProjectionView = faceViewProj;
            shadowpassBufferGPU.ObjectModel = modelMatrix;
            shadowpassBufferGPU.LightPosition = pointLight.Position;
            D3D11_MAPPED_SUBRESOURCE mappedResource;
            Utils::Check << context->Map(_shadowConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
            memcpy(mappedResource.pData, &shadowpassBufferGPU, sizeof(BeShadowpassBufferGPU));
            context->Unmap(_shadowConstantBuffer.Get(), 0);
            context->VSSetConstantBuffers(1, 1, _shadowConstantBuffer.GetAddressOf());
            
            // draw
            for (const auto& slice : object.DrawSlices) {
                context->DrawIndexed(slice.IndexCount, slice.StartIndexLocation, slice.BaseVertexLocation);
            }
        }
    }
    // clean up
    context->OMSetRenderTargets(0, nullptr, nullptr);
    context->VSSetConstantBuffers(1, 1, Utils::NullBuffers);
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