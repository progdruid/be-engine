#include "BeShadowPass.h"

#include <umbrellas/include-glm.h>
#include <scope_guard/scope_guard.hpp>

#include "BeAssetRegistry.h"
#include "BeRenderer.h"

auto BeShadowPass::Initialise() -> void {
    D3D11_BUFFER_DESC objectBufferDescriptor = {};
    objectBufferDescriptor.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    objectBufferDescriptor.Usage = D3D11_USAGE_DYNAMIC;
    objectBufferDescriptor.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    objectBufferDescriptor.ByteWidth = sizeof(BeObjectBufferGPU);
    Utils::Check << _renderer->GetDevice()->CreateBuffer(&objectBufferDescriptor, nullptr, &_objectBuffer);
    
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


    for (size_t i = 0; i < PointLights->size(); i++) {
        if (!(*PointLights)[i].CastsShadows)
            continue;

        Utils::BeDebugAnnotation pointLightAnnotation(context, "Point Light Shadows " + std::to_string(i));
        RenderPointLightShadows((*PointLights)[i]);
    }
}

auto BeShadowPass::RenderDirectionalShadows() -> void {
    const auto context = _renderer->GetContext();
    const auto registry = _renderer->GetAssetRegistry().lock();

    const auto directionalShadowMap = registry->GetTexture(DirectionalLight->ShadowMapTextureName).lock();

    // sort out viewport
    D3D11_VIEWPORT viewport = {};
    viewport.Width = DirectionalLight->ShadowMapResolution;
    viewport.Height = DirectionalLight->ShadowMapResolution;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    context->RSSetViewports(1, &viewport);

    // sort out render target
    context->OMSetRenderTargets(0, Utils::NullRTVs, directionalShadowMap->GetDSV().Get());
    context->ClearDepthStencilView(directionalShadowMap->GetDSV().Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
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

    const auto& objects = _renderer->GetObjects();
    for (const auto& object : objects) {
        const auto& shader = object.Model->Shader;

        shader->Bind(context.Get(), BeShaderType::Vertex | BeShaderType::Tesselation);
        SCOPE_EXIT { BeShader::Unbind(context.Get(), BeShaderType::Vertex | BeShaderType::Tesselation); };

        context->IASetPrimitiveTopology(
            HasAny(shader->GetShaderType(), BeShaderType::Tesselation)
            ? D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST
            : D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST
        );
        
        const glm::mat4x4 modelMatrix =
            glm::translate(glm::mat4(1.0f), object.Position) *
            glm::mat4_cast(object.Rotation) *
            glm::scale(glm::mat4(1.0f), object.Scale);
        
        BeObjectBufferGPU objectData(modelMatrix, DirectionalLight->ViewProjection, glm::vec3(0.f));
        D3D11_MAPPED_SUBRESOURCE objectMappedResource;
        Utils::Check << context->Map(_objectBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &objectMappedResource);
        memcpy(objectMappedResource.pData, &objectData, sizeof(BeObjectBufferGPU));
        context->Unmap(_objectBuffer.Get(), 0);
        context->VSSetConstantBuffers(1, 1, _objectBuffer.GetAddressOf());
        //context->PSSetConstantBuffers(1, 1, _objectBuffer.GetAddressOf());
        if (HasAny(shader->GetShaderType(), BeShaderType::Tesselation)) {
            context->HSSetConstantBuffers(1, 1, _objectBuffer.GetAddressOf());
            context->DSSetConstantBuffers(1, 1, _objectBuffer.GetAddressOf());
        }

        for (const auto& slice : object.DrawSlices) {
            slice.Material->UpdateGPUBuffers(context);
            const auto& materialBuffer = slice.Material->GetBuffer();
            context->VSSetConstantBuffers(2, 1, materialBuffer.GetAddressOf());
            //context->PSSetConstantBuffers(2, 1, materialBuffer.GetAddressOf());
            if (HasAny(shader->GetShaderType(), BeShaderType::Tesselation)) {
                context->HSSetConstantBuffers(2, 1, materialBuffer.GetAddressOf());
                context->DSSetConstantBuffers(2, 1, materialBuffer.GetAddressOf());
            }

            context->DrawIndexed(slice.IndexCount, slice.StartIndexLocation, slice.BaseVertexLocation);
        }
    }
    context->VSSetConstantBuffers(1, 2, Utils::NullBuffers);
    context->HSSetConstantBuffers(1, 2, Utils::NullBuffers);
    context->DSSetConstantBuffers(1, 2, Utils::NullBuffers);
    
}

auto BeShadowPass::RenderPointLightShadows(const BePointLight& pointLight) -> void {

    // get what we need
    const auto context = _renderer->GetContext();
    const auto registry = _renderer->GetAssetRegistry().lock();
    const auto pointShadowMap = registry->GetTexture(pointLight.ShadowMapTextureName).lock();
    
    // sort out shader
    //_pointShadowShader->Bind(context.Get(), BeShaderType::Vertex);
    //SCOPE_EXIT { BeShader::Unbind(context.Get(), BeShaderType::Vertex); };
    
    // sort out vertex and index buffers
    uint32_t stride = sizeof(BeFullVertex);
    uint32_t offset = 0;
    context->IASetVertexBuffers(0, 1, _renderer->GetShaderVertexBuffer().GetAddressOf(), &stride, &offset);
    context->IASetIndexBuffer(_renderer->GetShaderIndexBuffer().Get(), DXGI_FORMAT_R32_UINT, 0);
    //context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
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
            const auto& shader = object.Model->Shader;
            
            shader->Bind(context.Get(), BeShaderType::Vertex | BeShaderType::Tesselation);
            SCOPE_EXIT { BeShader::Unbind(context.Get(), BeShaderType::Vertex | BeShaderType::Tesselation); };

            context->IASetPrimitiveTopology(
                HasAny(shader->GetShaderType(), BeShaderType::Tesselation)
                ? D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST
                : D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST
            );
            
            // model matrix
            const glm::mat4x4 modelMatrix =
                glm::translate(glm::mat4(1.0f), object.Position) *
                glm::mat4_cast(object.Rotation) *
                glm::scale(glm::mat4(1.0f), object.Scale);

            BeObjectBufferGPU objectData(modelMatrix, faceViewProj, pointLight.Position);
            D3D11_MAPPED_SUBRESOURCE objectMappedResource;
            Utils::Check << context->Map(_objectBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &objectMappedResource);
            memcpy(objectMappedResource.pData, &objectData, sizeof(BeObjectBufferGPU));
            context->Unmap(_objectBuffer.Get(), 0);
            context->VSSetConstantBuffers(1, 1, _objectBuffer.GetAddressOf());
            //context->PSSetConstantBuffers(1, 1, _objectBuffer.GetAddressOf());
            if (HasAny(shader->GetShaderType(), BeShaderType::Tesselation)) {
                context->HSSetConstantBuffers(1, 1, _objectBuffer.GetAddressOf());
                context->DSSetConstantBuffers(1, 1, _objectBuffer.GetAddressOf());
            }
            
            // draw
            for (const auto& slice : object.DrawSlices) {
                slice.Material->UpdateGPUBuffers(context);
                const auto& materialBuffer = slice.Material->GetBuffer();
                context->VSSetConstantBuffers(2, 1, materialBuffer.GetAddressOf());
                //context->PSSetConstantBuffers(2, 1, materialBuffer.GetAddressOf());
                if (HasAny(shader->GetShaderType(), BeShaderType::Tesselation)) {
                    context->HSSetConstantBuffers(2, 1, materialBuffer.GetAddressOf());
                    context->DSSetConstantBuffers(2, 1, materialBuffer.GetAddressOf());
                }

                context->DrawIndexed(slice.IndexCount, slice.StartIndexLocation, slice.BaseVertexLocation);
            }
        }
    }
    // clean up
    context->OMSetRenderTargets(0, nullptr, nullptr);
    context->VSSetConstantBuffers(1, 2, Utils::NullBuffers);
    context->HSSetConstantBuffers(1, 2, Utils::NullBuffers);
    context->DSSetConstantBuffers(1, 2, Utils::NullBuffers);
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