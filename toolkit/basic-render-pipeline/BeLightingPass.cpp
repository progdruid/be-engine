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
    _pointLightScheme = BeAssetRegistry::GetMaterialScheme("point-light-material");

    _emissiveAddShader = BeAssetRegistry::GetShader("emissive-add").lock();
    const auto& emissiveScheme = BeAssetRegistry::GetMaterialScheme("emissive-add-material");
    _emissiveMaterial = BeMaterial::Create("EmissiveMaterial", emissiveScheme, false, *_renderer);
    _emissiveMaterial->SetTexture("InputEmissive", InputTexture3.lock());

    const SenFormat outputFormat = OutputTexture.lock()->Format;

    auto directionalDesc = _directionalLightShader->GetPipelineDesc();
    directionalDesc.BlendState.Enable = true;
    directionalDesc.BlendState.SrcBlend = SenBlendFactor::One;
    directionalDesc.BlendState.DstBlend = SenBlendFactor::One;
    directionalDesc.BlendState.BlendOp = SenBlendOp::Add;
    directionalDesc.BlendState.SrcBlendAlpha = SenBlendFactor::One;
    directionalDesc.BlendState.DstBlendAlpha = SenBlendFactor::One;
    directionalDesc.BlendState.BlendOpAlpha = SenBlendOp::Add;
    directionalDesc.RenderTargetFormats = { outputFormat };
    directionalDesc.BindGroupLayouts = { _renderer->GetUniformBindGroupLayout(), _directionalLightMaterial->GetBindGroupLayout() };
    _directionalLightPipeline = SenBackend::CreatePipeline(directionalDesc);

    // Point light pipeline created lazily in Render() once we have a material instance.

    auto emissiveDesc = _emissiveAddShader->GetPipelineDesc();
    emissiveDesc.BlendState.Enable = true;
    emissiveDesc.BlendState.SrcBlend = SenBlendFactor::One;
    emissiveDesc.BlendState.DstBlend = SenBlendFactor::One;
    emissiveDesc.BlendState.BlendOp = SenBlendOp::Add;
    emissiveDesc.BlendState.SrcBlendAlpha = SenBlendFactor::One;
    emissiveDesc.BlendState.DstBlendAlpha = SenBlendFactor::One;
    emissiveDesc.BlendState.BlendOpAlpha = SenBlendOp::Add;
    emissiveDesc.RenderTargetFormats = { outputFormat };
    emissiveDesc.BindGroupLayouts = { _renderer->GetUniformBindGroupLayout(), _emissiveMaterial->GetBindGroupLayout() };
    _emissivePipeline = SenBackend::CreatePipeline(emissiveDesc);
}

auto BeLightingPass::Render() -> void {
    const auto& submissionBuffer = *SubmissionBuffer.lock();
    auto& cmd = _renderer->GetCommandBuffer();

    // Phase 1 (BEFORE BeginPass): flush all light material uniform buffers via vkCmdUpdateBuffer.

    // Directional light
    const auto& sunLight = submissionBuffer.GetSunLightEntries()[0];
    _directionalLightMaterial->SetFloat("HasShadowMap", sunLight.CastsShadows ? 1.0f : 0.0f);
    _directionalLightMaterial->SetFloat3("Direction", sunLight.Direction);
    _directionalLightMaterial->SetFloat3("Color", sunLight.Color);
    _directionalLightMaterial->SetFloat("Power", sunLight.Power);
    _directionalLightMaterial->SetMatrix("ProjectionView", sunLight.ShadowViewProjection);
    _directionalLightMaterial->SetFloat("TexelSize", 1.0f / sunLight.ShadowMapResolution);
    _directionalLightMaterial->SetTexture("ShadowMap", sunLight.ShadowMap.lock());
    _directionalLightMaterial->GetBindGroup();

    // Point lights
    const auto& pointLights = submissionBuffer.GetPointLightEntries();
    for (const auto& pointLight : pointLights) {
        if (!_pointLightMaterials.contains(pointLight.Name)) {
            auto mat = BeMaterial::Create("PointLight_" + pointLight.Name, _pointLightScheme, true, *_renderer);
            mat->SetTexture("Depth", InputDepthTexture.lock());
            mat->SetTexture("Diffuse", InputTexture0.lock());
            mat->SetTexture("WorldNormal", InputTexture1.lock());
            mat->SetTexture("Specular_Shininess", InputTexture2.lock());
            _pointLightMaterials[pointLight.Name] = std::move(mat);

            if (!_pointLightPipeline.IsValid()) {
                const SenFormat outputFormat = OutputTexture.lock()->Format;
                auto pointDesc = _pointLightShader->GetPipelineDesc();
                pointDesc.BlendState.Enable = true;
                pointDesc.BlendState.SrcBlend = SenBlendFactor::One;
                pointDesc.BlendState.DstBlend = SenBlendFactor::One;
                pointDesc.BlendState.BlendOp = SenBlendOp::Add;
                pointDesc.BlendState.SrcBlendAlpha = SenBlendFactor::One;
                pointDesc.BlendState.DstBlendAlpha = SenBlendFactor::One;
                pointDesc.BlendState.BlendOpAlpha = SenBlendOp::Add;
                pointDesc.RenderTargetFormats = { outputFormat };
                pointDesc.BindGroupLayouts = { _renderer->GetUniformBindGroupLayout(), _pointLightMaterials[pointLight.Name]->GetBindGroupLayout() };
                _pointLightPipeline = SenBackend::CreatePipeline(pointDesc);
            }
        }
        auto& mat = _pointLightMaterials[pointLight.Name];
        mat->SetFloat3("Position", pointLight.Position);
        mat->SetFloat("Radius", pointLight.Radius);
        mat->SetFloat3("Color", pointLight.Color);
        mat->SetFloat("Power", pointLight.Power);
        mat->SetFloat("HasShadowMap", pointLight.CastsShadows ? 1.0f : 0.0f);
        mat->SetFloat("ShadowMapResolution", (float)pointLight.ShadowMapResolution);
        mat->SetFloat("ShadowNearPlane", pointLight.ShadowNearPlane);
        mat->SetTexture("PointLightShadowMap", pointLight.ShadowMap.lock());
        mat->GetBindGroup();
    }

    // Phase 2 (INSIDE BeginPass): bind pre-warmed materials and draw.
    cmd.BeginPass({
        .ColorAttachments = {
            { OutputTexture.lock()->Handle, 0, -1, SenLoadOp::Clear, {0, 0, 0, 0} },
        },
        .Viewport = _renderer->GetViewport(),
    });
    SCOPE_EXIT { cmd.EndPass(); };

    // Directional light
    cmd.SetPipeline(_directionalLightPipeline);
    cmd.SetBindGroup(_directionalLightMaterial->GetBindGroup(), 1);
    cmd.Draw(4, 0);
    _directionalLightMaterial->SetTexture("ShadowMap", nullptr);

    // Point lights
    cmd.SetPipeline(_pointLightPipeline);
    for (const auto& pointLight : pointLights) {
        auto& mat = _pointLightMaterials[pointLight.Name];
        cmd.SetBindGroup(mat->GetBindGroup(), 1);
        cmd.Draw(4, 0);
    }

    for (const auto& pointLight : pointLights) {
        _pointLightMaterials[pointLight.Name]->SetTexture("PointLightShadowMap", nullptr);
    }

    // Emissive add
    cmd.SetPipeline(_emissivePipeline);
    cmd.SetBindGroup(_emissiveMaterial->GetBindGroup(), 1);
    cmd.Draw(4, 0);
}
