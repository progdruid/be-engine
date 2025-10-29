#include "BeLightingPass.h"

#include <gtc/type_ptr.inl>

#include "BeRenderer.h"
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
    directionalLightBufferDescriptor.ByteWidth = sizeof(DirectionalLightBufferGPU);
    Utils::Check << device->CreateBuffer(&directionalLightBufferDescriptor, nullptr, &_directionalLightBuffer);
    
    D3D11_BUFFER_DESC pointLightBufferDescriptor = {};
    pointLightBufferDescriptor.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    pointLightBufferDescriptor.Usage = D3D11_USAGE_DYNAMIC;
    pointLightBufferDescriptor.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    pointLightBufferDescriptor.ByteWidth = sizeof(PointLightBufferGPU);
    Utils::Check << device->CreateBuffer(&pointLightBufferDescriptor, nullptr, &_pointLightBuffer);

    _directionalLightShader = std::make_unique<BeShader>(
        device.Get(),
        "assets/shaders/directionalLight",
        std::vector<BeVertexElementDescriptor>{}
    );

    _pointLightShader = std::make_unique<BeShader>(
        device.Get(),
        "assets/shaders/pointLight",
        std::vector<BeVertexElementDescriptor>{}
    );
}

auto BeLightingPass::Render() -> void {
    const auto context = _renderer->GetContext();
    
    BeRenderResource* depthResource    = _renderer->GetRenderResource(InputDepthTextureName);
    BeRenderResource* gbufferResource0 = _renderer->GetRenderResource(InputTexture0Name);
    BeRenderResource* gbufferResource1 = _renderer->GetRenderResource(InputTexture1Name);
    BeRenderResource* gbufferResource2 = _renderer->GetRenderResource(InputTexture2Name);
    BeRenderResource* lightingResource = _renderer->GetRenderResource(OutputTextureName);
    
    context->ClearRenderTargetView(lightingResource->RTV.Get(), glm::value_ptr(glm::vec4(0.0f)));
    context->OMSetRenderTargets(1, lightingResource->RTV.GetAddressOf(), nullptr);
    context->OMSetBlendState(_lightingBlendState.Get(), nullptr, 0xFFFFFFFF);

    context->VSSetShader(_renderer->GetFullscreenVertexShader().Get(), nullptr, 0);

    context->PSSetShaderResources(0, 1, depthResource->SRV.GetAddressOf());
    context->PSSetShaderResources(1, 1, gbufferResource0->SRV.GetAddressOf());
    context->PSSetShaderResources(2, 1, gbufferResource1->SRV.GetAddressOf());
    context->PSSetShaderResources(3, 1, gbufferResource2->SRV.GetAddressOf());

    context->PSSetSamplers(0, 1, _renderer->GetPointSampler().GetAddressOf());

    {
        DirectionalLightBufferGPU directionalLightBuffer(DirectionalLightData);
        D3D11_MAPPED_SUBRESOURCE directionalLightMappedResource;
        Utils::Check << context->Map(_directionalLightBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &directionalLightMappedResource);
        memcpy(directionalLightMappedResource.pData, &directionalLightBuffer, sizeof(DirectionalLightBufferGPU));
        context->Unmap(_directionalLightBuffer.Get(), 0);
        context->PSSetConstantBuffers(1, 1, _directionalLightBuffer.GetAddressOf());

        context->PSSetShader(_directionalLightShader->PixelShader.Get(), nullptr, 0);

        context->IASetInputLayout(nullptr);
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
        context->Draw(4, 0);
    }

    context->PSSetShader(_pointLightShader->PixelShader.Get(), nullptr, 0);
    for (const auto& pointLightData : PointLights) {
        PointLightBufferGPU pointLightBuffer(pointLightData);
        D3D11_MAPPED_SUBRESOURCE pointLightMappedResource;
        Utils::Check << context->Map(_pointLightBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &pointLightMappedResource);
        memcpy(pointLightMappedResource.pData, &pointLightBuffer, sizeof(PointLightBufferGPU));
        context->Unmap(_pointLightBuffer.Get(), 0);
        context->PSSetConstantBuffers(1, 1, _pointLightBuffer.GetAddressOf());
    
        context->IASetInputLayout(nullptr);
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
        context->Draw(4, 0);
    }

    {
        ID3D11ShaderResourceView* emptyResources[4] = { nullptr, nullptr, nullptr, nullptr };
        context->PSSetShaderResources(0, 4, emptyResources);
        ID3D11Buffer* emptyBuffers[1] = { nullptr };
        context->PSSetConstantBuffers(1, 1, emptyBuffers);
        ID3D11SamplerState* emptySamplers[1] = { nullptr };
        context->PSSetSamplers(0, 1, emptySamplers);
        ID3D11RenderTargetView* emptyTargets[1] = { nullptr };
        context->OMSetRenderTargets(1, emptyTargets, nullptr);
        context->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
    }
}
