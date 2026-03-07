#include "BeLightingPass.h"

#include <scope_guard/scope_guard.hpp>
#include <umbrellas/include-glm.h>
#include <sen-rhi/SenBackend.h>

#include "BeAssetRegistry.h"
#include "BeBRPSubmissionBuffer.h"
#include "BeMaterial.h"
#include "BeRenderer.h"
#include "BeShader.h"
#include "BeTexture.h"
#include "Utils.h"


BeLightingPass::BeLightingPass() = default;
BeLightingPass::~BeLightingPass() = default;

void BeLightingPass::Initialise() {
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
    
    _emissiveAddShader = BeAssetRegistry::GetShader("emissive-add").lock();
    const auto& emissiveScheme = BeAssetRegistry::GetMaterialScheme("emissive-add-material");
    _emissiveMaterial = BeMaterial::Create("EmissiveMaterial", emissiveScheme, false, *_renderer);
    _emissiveMaterial->SetTexture("InputEmissive", InputTexture3.lock());

    _directionalLightBinding.Make(_directionalLightMaterial, _directionalLightShader);
    _pointLightBinding.Make(_pointLightMaterial, _pointLightShader);
    _emissiveBinding.Make(_emissiveMaterial, _emissiveAddShader);

    // Create pipelines for each shader with additive blending
    auto directionalDesc = _directionalLightShader->CreatePipelineDesc();
    directionalDesc.BlendState.Enable = true;
    directionalDesc.BlendState.SrcBlend = SenBlendFactor::One;
    directionalDesc.BlendState.DstBlend = SenBlendFactor::One;
    directionalDesc.BlendState.BlendOp = SenBlendOp::Add;
    directionalDesc.BlendState.SrcBlendAlpha = SenBlendFactor::One;
    directionalDesc.BlendState.DstBlendAlpha = SenBlendFactor::One;
    directionalDesc.BlendState.BlendOpAlpha = SenBlendOp::Add;
    _directionalLightPipeline = SenBackend::CreatePipeline(directionalDesc);

    auto pointDesc = _pointLightShader->CreatePipelineDesc();
    pointDesc.BlendState.Enable = true;
    pointDesc.BlendState.SrcBlend = SenBlendFactor::One;
    pointDesc.BlendState.DstBlend = SenBlendFactor::One;
    pointDesc.BlendState.BlendOp = SenBlendOp::Add;
    pointDesc.BlendState.SrcBlendAlpha = SenBlendFactor::One;
    pointDesc.BlendState.DstBlendAlpha = SenBlendFactor::One;
    pointDesc.BlendState.BlendOpAlpha = SenBlendOp::Add;
    _pointLightPipeline = SenBackend::CreatePipeline(pointDesc);

    auto emissiveDesc = _emissiveAddShader->CreatePipelineDesc();
    emissiveDesc.BlendState.Enable = true;
    emissiveDesc.BlendState.SrcBlend = SenBlendFactor::One;
    emissiveDesc.BlendState.DstBlend = SenBlendFactor::One;
    emissiveDesc.BlendState.BlendOp = SenBlendOp::Add;
    emissiveDesc.BlendState.SrcBlendAlpha = SenBlendFactor::One;
    emissiveDesc.BlendState.DstBlendAlpha = SenBlendFactor::One;
    emissiveDesc.BlendState.BlendOpAlpha = SenBlendOp::Add;
    _emissivePipeline = SenBackend::CreatePipeline(emissiveDesc);
}

auto BeLightingPass::Render() -> void {
    const auto& submissionBuffer = *SubmissionBuffer.lock();
    auto& cmd = _renderer->GetCommandBuffer();

    // Begin pass with single color attachment
    cmd.BeginPass({
        .ColorAttachments = {
            { OutputTexture.lock()->Handle, 0, -1, SenLoadOp::Clear, {0, 0, 0, 0} },
        },
        .Viewport = _renderer->GetViewport(),
    });
    SCOPE_EXIT { cmd.EndPass(); };

    // directional light
    cmd.SetPipeline(_directionalLightPipeline);

    const auto& sunLight = submissionBuffer.GetSunLightEntries()[0];
    _directionalLightMaterial->SetFloat("HasShadowMap", sunLight.CastsShadows ? 1.0f : 0.0f);
    _directionalLightMaterial->SetFloat3("Direction", sunLight.Direction);
    _directionalLightMaterial->SetFloat3("Color", sunLight.Color);
    _directionalLightMaterial->SetFloat("Power", sunLight.Power);
    _directionalLightMaterial->SetMatrix("ProjectionView", sunLight.ShadowViewProjection);
    _directionalLightMaterial->SetFloat("TexelSize", 1.0f / sunLight.ShadowMapResolution);
    _directionalLightMaterial->SetTexture("ShadowMap", sunLight.ShadowMap.lock());
    cmd.SetBindGroup(_directionalLightBinding.Resolve(), 1);

    cmd.Draw(4, 0);
    _directionalLightMaterial->SetTexture("ShadowMap", nullptr);


    // point lights
    cmd.SetPipeline(_pointLightPipeline);
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
        cmd.SetBindGroup(_pointLightBinding.Resolve(), 1);

        cmd.Draw(4, 0);
    }

    _pointLightMaterial->SetTexture("PointLightShadowMap", nullptr);


    // emissive add
    cmd.SetPipeline(_emissivePipeline);
    cmd.SetBindGroup(_emissiveBinding.Resolve(), 1);
    cmd.Draw(4, 0);
}
