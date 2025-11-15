#include "BeLightingPass.h"

#include <scope_guard.hpp>
#include <gtc/type_ptr.inl>

#include "BeRenderer.h"
#include "BeShader.h"
#include "Utils.h"


BeLightingPass::BeLightingPass() = default;
BeLightingPass::~BeLightingPass() = default;

auto BeLightingPass::Initialise() -> void {
    const auto device = _renderer->GetDevice();

    // Additive blending for lights
    D3D11_BLEND_DESC lightingBlendDesc = {};
    lightingBlendDesc.RenderTarget[0].BlendEnable = TRUE;
    lightingBlendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
    lightingBlendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
    lightingBlendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    lightingBlendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    lightingBlendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
    lightingBlendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    lightingBlendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    Utils::Check << device->CreateBlendState(&lightingBlendDesc, _lightingBlendState.GetAddressOf());

    
    D3D11_BUFFER_DESC directionalLightBufferDescriptor = {};
    directionalLightBufferDescriptor.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    directionalLightBufferDescriptor.Usage = D3D11_USAGE_DYNAMIC;
    directionalLightBufferDescriptor.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    directionalLightBufferDescriptor.ByteWidth = sizeof(BeDirectionalLightLightingBufferGPU);
    Utils::Check << device->CreateBuffer(&directionalLightBufferDescriptor, nullptr, &_directionalLightBuffer);
    
    D3D11_BUFFER_DESC pointLightBufferDescriptor = {};
    pointLightBufferDescriptor.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    pointLightBufferDescriptor.Usage = D3D11_USAGE_DYNAMIC;
    pointLightBufferDescriptor.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    pointLightBufferDescriptor.ByteWidth = sizeof(BePointLightLightingBufferGPU);
    Utils::Check << device->CreateBuffer(&pointLightBufferDescriptor, nullptr, &_pointLightBuffer);

    _directionalLightShader = BeShader::Create( device.Get(), "assets/shaders/directionalLight");
    _pointLightShader = BeShader::Create(device.Get(), "assets/shaders/pointLight" );
}

auto BeLightingPass::Render() -> void {
    const auto context = _renderer->GetContext();
    const BeDirectionalLight& directionalLight = *_renderer->GetContextDataPointer<BeDirectionalLight>(InputDirectionalLightName);
    const std::vector<BePointLight>& pointLights = *_renderer->GetContextDataPointer<std::vector<BePointLight>>(InputPointLightsName);
    
    BeRenderResource* depthResource     = _renderer->GetRenderResource(InputDepthTextureName);
    BeRenderResource* gbufferResource0  = _renderer->GetRenderResource(InputTexture0Name);
    BeRenderResource* gbufferResource1  = _renderer->GetRenderResource(InputTexture1Name);
    BeRenderResource* gbufferResource2  = _renderer->GetRenderResource(InputTexture2Name);
    BeRenderResource* lightingResource  = _renderer->GetRenderResource(OutputTextureName);
    BeRenderResource* directionalLightShadowMapResource  = _renderer->GetRenderResource(directionalLight.ShadowMapTextureName);
    
    context->ClearRenderTargetView(lightingResource->RTV.Get(), glm::value_ptr(glm::vec4(0.0f)));
    context->OMSetRenderTargets(1, lightingResource->RTV.GetAddressOf(), nullptr);
    context->OMSetBlendState(_lightingBlendState.Get(), nullptr, 0xFFFFFFFF);
    SCOPE_EXIT {
        context->OMSetRenderTargets(1, Utils::NullRTVs, nullptr);
        context->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
    };

    context->VSSetShader(_renderer->GetFullscreenVertexShader().Get(), nullptr, 0);
    SCOPE_EXIT { context->VSSetShader(nullptr, nullptr, 0); };

    context->PSSetShaderResources(0, 1, depthResource->SRV.GetAddressOf());
    context->PSSetShaderResources(1, 1, gbufferResource0->SRV.GetAddressOf());
    context->PSSetShaderResources(2, 1, gbufferResource1->SRV.GetAddressOf());
    context->PSSetShaderResources(3, 1, gbufferResource2->SRV.GetAddressOf());
    SCOPE_EXIT { context->PSSetShaderResources(0, 4, Utils::NullSRVs); };
    
    context->PSSetSamplers(0, 1, _renderer->GetPointSampler().GetAddressOf());
    SCOPE_EXIT { context->PSSetSamplers(0, 1, Utils::NullSamplers); };

    {
        context->PSSetShaderResources(4, 1, directionalLightShadowMapResource->SRV.GetAddressOf());
        SCOPE_EXIT { context->PSSetShaderResources(4, 1, Utils::NullSRVs); };
        context->PSSetShader(_directionalLightShader->PixelShader.Get(), nullptr, 0);

        BeDirectionalLightLightingBufferGPU directionalLightBuffer(directionalLight);
        D3D11_MAPPED_SUBRESOURCE directionalLightMappedResource;
        Utils::Check << context->Map(_directionalLightBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &directionalLightMappedResource);
        memcpy(directionalLightMappedResource.pData, &directionalLightBuffer, sizeof(BeDirectionalLightLightingBufferGPU));
        context->Unmap(_directionalLightBuffer.Get(), 0);
        context->PSSetConstantBuffers(1, 1, _directionalLightBuffer.GetAddressOf());

        context->IASetInputLayout(nullptr);
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
        context->Draw(4, 0);
    }

    {
        context->PSSetShader(_pointLightShader->PixelShader.Get(), nullptr, 0);
        for (const auto& pointLightData : pointLights) {
            const auto& shadowCubemap = *_renderer->GetRenderResource(pointLightData.ShadowMapTextureName);
            context->PSSetShaderResources(4, 1, shadowCubemap.SRV.GetAddressOf());

            BePointLightLightingBufferGPU pointLightBuffer(pointLightData);
            D3D11_MAPPED_SUBRESOURCE pointLightMappedResource;
            Utils::Check << context->Map(_pointLightBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &pointLightMappedResource);
            memcpy(pointLightMappedResource.pData, &pointLightBuffer, sizeof(BePointLightLightingBufferGPU));
            context->Unmap(_pointLightBuffer.Get(), 0);
            context->PSSetConstantBuffers(1, 1, _pointLightBuffer.GetAddressOf());
    
            context->IASetInputLayout(nullptr);
            context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
            context->Draw(4, 0);
        }
        context->PSSetShaderResources(4, 1, Utils::NullSRVs);
    }

    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED);
    context->PSSetShader(nullptr, nullptr, 0);
    context->PSSetConstantBuffers(1, 1, Utils::NullBuffers);
}
