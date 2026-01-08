#include "BeLightingPass.h"

#include <scope_guard/scope_guard.hpp>
#include <umbrellas/include-glm.h>

#include "BeAssetRegistry.h"
#include "BeBRPSubmissionBuffer.h"
#include "BeMaterial.h"
#include "BePipeline.h"
#include "BeRenderer.h"
#include "BeShader.h"
#include "BeTexture.h"
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
    
    _directionalLightShader = BeAssetRegistry::GetShader( "directional-light").lock();
    const auto& directionalScheme = BeAssetRegistry::GetMaterialScheme("directional-light-material");;
    _directionalLightMaterial = BeMaterial::Create("DirectionalLightMaterial", directionalScheme, true, *_renderer);
    _directionalLightMaterial->SetTexture("Depth", InputDepthTexture.lock());
    _directionalLightMaterial->SetTexture("Diffuse", InputTexture0.lock());
    _directionalLightMaterial->SetTexture("WorldNormal", InputTexture1.lock());
    _directionalLightMaterial->SetTexture("Specular_Shininess", InputTexture2.lock());
    
    _pointLightShader = BeAssetRegistry::GetShader("point-light").lock();
    const auto& pointScheme = BeAssetRegistry::GetMaterialScheme("point-light-material");
    _pointLightMaterial = BeMaterial::Create("PointLightMaterial", pointScheme, true, *_renderer);
    _pointLightMaterial->SetTexture("Depth", InputDepthTexture.lock());
    _pointLightMaterial->SetTexture("Diffuse", InputTexture0.lock());
    _pointLightMaterial->SetTexture("WorldNormal", InputTexture1.lock());
    _pointLightMaterial->SetTexture("Specular_Shininess", InputTexture2.lock());
}

auto BeLightingPass::Render() -> void {
    const auto& submissionBuffer = *SubmissionBuffer.lock();
    const auto context = _renderer->GetContext();
    const auto& pipeline = _renderer->GetPipeline();
    
    const auto lightingResource  = OutputTexture.lock();
    context->ClearRenderTargetView(lightingResource->GetRTV().Get(), glm::value_ptr(glm::vec4(0.0f)));
    context->OMSetRenderTargets(1, lightingResource->GetRTV().GetAddressOf(), nullptr);
    context->OMSetBlendState(_lightingBlendState.Get(), nullptr, 0xFFFFFFFF);
    SCOPE_EXIT {
        context->OMSetRenderTargets(1, Utils::NullRTVs, nullptr);
        context->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
    };

    // directional light
    pipeline->BindShader(_directionalLightShader, BeShaderType::Vertex | BeShaderType::Pixel);
        
    const auto& sunLight = submissionBuffer.GetSunLightEntries()[0];
    _directionalLightMaterial->SetFloat("HasShadowMap", sunLight.CastsShadows ? 1.0f : 0.0f);
    _directionalLightMaterial->SetFloat3("Direction", sunLight.Direction);
    _directionalLightMaterial->SetFloat3("Color", sunLight.Color);
    _directionalLightMaterial->SetFloat("Power", sunLight.Power);
    _directionalLightMaterial->SetMatrix("ProjectionView", sunLight.ViewProjection);
    _directionalLightMaterial->SetFloat("TexelSize", 1.0f / sunLight.ShadowMapResolution);
    _directionalLightMaterial->SetTexture("ShadowMap", sunLight.ShadowMap.lock());
    pipeline->BindMaterialAutomatic(_directionalLightMaterial);
    
    context->Draw(4, 0);
    _directionalLightMaterial->SetTexture("ShadowMap", nullptr);
    pipeline->Clear();


    // point lights
    pipeline->BindShader(_pointLightShader, BeShaderType::Vertex | BeShaderType::Pixel);
    for (const auto& pointLight : submissionBuffer.GetPointLightEntries()) {
        _pointLightMaterial->SetFloat3("Position", pointLight.Position);
        _pointLightMaterial->SetFloat("Radius", pointLight.Radius);
        _pointLightMaterial->SetFloat3("Color", pointLight.Color);
        _pointLightMaterial->SetFloat("Power", pointLight.Power);
        _pointLightMaterial->SetFloat("HasShadowMap", pointLight.CastsShadows ? 1.0f : 0.0f);
        _pointLightMaterial->SetFloat("ShadowMapResolution", pointLight.ShadowMapResolution);
        _pointLightMaterial->SetFloat("ShadowNearPlane", pointLight.ShadowNearPlane);
        // TODO: super uncool, material shouldnt own anything ideally. or should it?
        _pointLightMaterial->SetTexture("PointLightShadowMap", pointLight.ShadowMap.lock()); 
        pipeline->BindMaterialAutomatic(_pointLightMaterial);
    
        context->Draw(4, 0);
    }
    
    _pointLightMaterial->SetTexture("PointLightShadowMap", nullptr);
    pipeline->Clear();
}