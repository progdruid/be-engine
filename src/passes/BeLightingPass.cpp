#include "BeLightingPass.h"

#include <scope_guard/scope_guard.hpp>
#include <umbrellas/include-glm.h>

#include "BeAssetRegistry.h"
#include "BeRenderer.h"
#include "BeShader.h"
#include "Utils.h"


BeLightingPass::BeLightingPass() = default;
BeLightingPass::~BeLightingPass() = default;

void BeLightingPass::Initialise() {
    const auto device = _renderer->GetDevice();

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

    const auto registry = _renderer->GetAssetRegistry().lock();
    
    _directionalLightShader = BeShader::Create( device.Get(), "assets/shaders/directionalLight");
    _directionalLightMaterial = BeMaterial::Create("DirectionalLightMaterial", true, _directionalLightShader, _renderer->GetAssetRegistry().lock(), _renderer->GetDevice());
    _directionalLightMaterial->SetTexture("Depth", registry->GetTexture(InputDepthTextureName).lock());
    _directionalLightMaterial->SetTexture("Diffuse", registry->GetTexture(InputTexture0Name).lock());
    _directionalLightMaterial->SetTexture("WorldNormal", registry->GetTexture(InputTexture1Name).lock());
    _directionalLightMaterial->SetTexture("Specular_Shininess", registry->GetTexture(InputTexture2Name).lock());
    
    _pointLightShader = BeShader::Create(device.Get(), "assets/shaders/pointLight" );
    _pointLightMaterial = BeMaterial::Create("PointLightMaterial", true, _pointLightShader, _renderer->GetAssetRegistry().lock(), _renderer->GetDevice());
    _pointLightMaterial->SetTexture("Depth", registry->GetTexture(InputDepthTextureName).lock());
    _pointLightMaterial->SetTexture("Diffuse", registry->GetTexture(InputTexture0Name).lock());
    _pointLightMaterial->SetTexture("WorldNormal", registry->GetTexture(InputTexture1Name).lock());
    _pointLightMaterial->SetTexture("Specular_Shininess", registry->GetTexture(InputTexture2Name).lock());
}

auto BeLightingPass::Render() -> void {
    const auto context = _renderer->GetContext();
    const auto registry = _renderer->GetAssetRegistry().lock();
    
    const auto lightingResource  = registry->GetTexture(OutputTextureName).lock();
    context->ClearRenderTargetView(lightingResource->GetRTV().Get(), glm::value_ptr(glm::vec4(0.0f)));
    context->OMSetRenderTargets(1, lightingResource->GetRTV().GetAddressOf(), nullptr);
    context->OMSetBlendState(_lightingBlendState.Get(), nullptr, 0xFFFFFFFF);
    SCOPE_EXIT {
        context->OMSetRenderTargets(1, Utils::NullRTVs, nullptr);
        context->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
    };

    context->PSSetSamplers(0, 1, _renderer->GetPointSampler().GetAddressOf());
    SCOPE_EXIT { context->PSSetSamplers(0, 1, Utils::NullSamplers); };

    {
        const auto data = DirectionalLight.lock();

        _directionalLightShader->Bind(context.Get(), BeShaderType::Vertex | BeShaderType::Pixel);
        
        _directionalLightMaterial->SetFloat("HasShadowMap", data->CastsShadows ? 1.0f : 0.0f);
        _directionalLightMaterial->SetFloat3("Direction", data->Direction);
        _directionalLightMaterial->SetFloat3("Color", data->Color);
        _directionalLightMaterial->SetFloat("Power", data->Power);
        _directionalLightMaterial->SetMatrix("ProjectionView", data->ViewProjection);
        _directionalLightMaterial->SetFloat("TexelSize", 1.0f / data->ShadowMapResolution);
        _directionalLightMaterial->SetTexture("ShadowMap", data->ShadowMap);
        
        BeMaterial::BindMaterial_Temporary(_directionalLightMaterial, context, BeShaderType::Pixel);
        
        context->IASetInputLayout(nullptr);
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
        context->Draw(4, 0);

        BeMaterial::UnbindMaterial_Temporary(_directionalLightMaterial, context, BeShaderType::Pixel);
        context->PSSetConstantBuffers(2, 1, Utils::NullBuffers);
    }

    {
        _pointLightShader->Bind(context.Get(), BeShaderType::Vertex | BeShaderType::Pixel);
        for (const auto& pointLight : PointLights) {

            _pointLightMaterial->SetFloat3("Position", pointLight.Position);
            _pointLightMaterial->SetFloat("Radius", pointLight.Radius);
            _pointLightMaterial->SetFloat3("Color", pointLight.Color);
            _pointLightMaterial->SetFloat("Power", pointLight.Power);
            _pointLightMaterial->SetFloat("HasShadowMap", pointLight.CastsShadows ? 1.0f : 0.0f);
            _pointLightMaterial->SetFloat("ShadowMapResolution", pointLight.ShadowMapResolution);
            _pointLightMaterial->SetFloat("ShadowNearPlane", pointLight.ShadowNearPlane);
            _pointLightMaterial->SetTexture("PointLightShadowMap", pointLight.ShadowMap);
            
            _pointLightMaterial->UpdateGPUBuffers(context.Get());
            context->PSSetConstantBuffers(2, 1, _pointLightMaterial->GetBuffer().GetAddressOf());
            BeMaterial::BindMaterial_Temporary(_pointLightMaterial, context, BeShaderType::Pixel);
    
            context->IASetInputLayout(nullptr);
            context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
            context->Draw(4, 0);
        }

        context->PSSetConstantBuffers(2, 1, Utils::NullBuffers);
        BeMaterial::UnbindMaterial_Temporary(_pointLightMaterial, context, BeShaderType::Pixel);
    }

    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED);
    BeShader::Unbind(context.Get(), BeShaderType::Pixel);
}
