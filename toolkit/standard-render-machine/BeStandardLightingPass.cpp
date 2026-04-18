#include "BeStandardLightingPass.h"

#include <scope_guard/scope_guard.hpp>
#include <sen-rhi/SenBackend.h>

#include "BeAssetRegistry.h"
#include "BeMaterial.h"
#include "BePipelineBuilder.h"
#include "BeRenderer.h"
#include "BeShader.h"
#include "BeTexture.h"
#include "standard-render-machine/BeStandardRenderMachine.h"

BeStandardLightingPass::BeStandardLightingPass(
    BeStandardRenderMachine* srm,
    std::vector<std::shared_ptr<BeTexture>> gbufferInputs,
    std::shared_ptr<BeTexture> depthInput,
    std::shared_ptr<BeTexture> output
) : _srm(srm), _gbufferInputs(std::move(gbufferInputs)), _depthInput(std::move(depthInput)), _output(std::move(output)) {}

auto BeStandardLightingPass::Initialise() -> void {
    const auto directionalLightShader = BeAssetRegistry::GetShader("directional-light").lock();
    const auto pointLightShader       = BeAssetRegistry::GetShader("point-light").lock();
    const auto emissiveAddShader      = BeAssetRegistry::GetShader("emissive-add").lock();

    constexpr SenBlendState additiveBlend = {
        .Enable = true,
        .SrcBlend = SenBlendFactor::One,  .DstBlend = SenBlendFactor::One,  .BlendOp = SenBlendOp::Add,
        .SrcBlendAlpha = SenBlendFactor::One, .DstBlendAlpha = SenBlendFactor::One, .BlendOpAlpha = SenBlendOp::Add,
    };
    const SenFormat outputFormat = _output->Format;

    _directionalLightMaterial = BeMaterial::Create("directional-light-material", true);
    _directionalLightMaterial->SetTexture("Depth",                     _depthInput);
    _directionalLightMaterial->SetTexture("Diffuse_RGB_or_Albedo_RGB", _gbufferInputs[0]);
    _directionalLightMaterial->SetTexture("WorldNormal_XYZ_LMF_W",     _gbufferInputs[1]);
    _directionalLightMaterial->SetTexture("SpecShin_RGBA_or_MRAO_RGB", _gbufferInputs[2]);

    _directionalLightPipeline = BePipelineBuilder::Start(*directionalLightShader)
        .SetBlend(additiveBlend).SetColorFormats({ outputFormat }).Build();

    _pointLightPipeline = BePipelineBuilder::Start(*pointLightShader)
        .SetBlend(additiveBlend).SetColorFormats({ outputFormat }).Build();

    _emissiveMaterial = BeMaterial::Create("emissive-add-material", false);
    _emissiveMaterial->SetTexture("InputEmissive", _gbufferInputs[3]);

    _emissivePipeline = BePipelineBuilder::Start(*emissiveAddShader)
        .SetBlend(additiveBlend).SetColorFormats({ outputFormat }).Build();
}

auto BeStandardLightingPass::Render() -> void {
    const auto& srm = *_srm;
    auto& cmd = _renderer->GetCommandBuffer();

    cmd.SetBindGroup(srm.UniformMaterial.lock()->GetBindGroup(), 0);

    cmd.BeginPass({
        .ColorAttachments = { { _output->Handle, 0, -1, SenLoadOp::Clear, {0, 0, 0, 0} } },
        .Viewport = _renderer->GetViewport(),
    });
    SCOPE_EXIT { cmd.EndPass(); };

    // Directional light
    const auto& sunLight = srm.GetSunLightEntries()[0];
    _directionalLightMaterial->SetFloat("HasShadowMap",  sunLight.CastsShadows ? 1.0f : 0.0f);
    _directionalLightMaterial->SetFloat3("Direction",    sunLight.Direction);
    _directionalLightMaterial->SetFloat3("Color",        sunLight.Color);
    _directionalLightMaterial->SetFloat("Power",         sunLight.Power);
    _directionalLightMaterial->SetMatrix("ProjectionView", sunLight.ShadowViewProjection);
    _directionalLightMaterial->SetFloat("TexelSize",     1.0f / sunLight.ShadowMapResolution);
    _directionalLightMaterial->SetTexture("ShadowMap",   sunLight.ShadowMap.lock());
    cmd.SetPipeline(_directionalLightPipeline);
    cmd.SetBindGroup(_directionalLightMaterial->GetBindGroup(), 1);
    cmd.Draw(4, 0);

    // Point lights
    for (const auto& pointLight : srm.GetPointLightEntries()) {
        if (!_pointLightMaterials.contains(pointLight.Name)) {
            auto mat = BeMaterial::Create("point-light-material", true);
            mat->SetTexture("Depth",                     _depthInput);
            mat->SetTexture("Diffuse_RGB_or_Albedo_RGB", _gbufferInputs[0]);
            mat->SetTexture("WorldNormal_XYZ_LMF_W",     _gbufferInputs[1]);
            mat->SetTexture("SpecShin_RGBA_or_MRAO_RGB", _gbufferInputs[2]);
            _pointLightMaterials[pointLight.Name] = std::move(mat);
        }
        auto& mat = _pointLightMaterials[pointLight.Name];
        mat->SetFloat3("Position",          pointLight.Position);
        mat->SetFloat("Radius",             pointLight.Radius);
        mat->SetFloat3("Color",             pointLight.Color);
        mat->SetFloat("Power",              pointLight.Power);
        mat->SetFloat("HasShadowMap",       pointLight.CastsShadows ? 1.0f : 0.0f);
        mat->SetFloat("ShadowMapResolution", (float)pointLight.ShadowMapResolution);
        mat->SetFloat("ShadowNearPlane",    pointLight.ShadowNearPlane);
        mat->SetTexture("PointLightShadowMap", pointLight.ShadowMap.lock());
        cmd.SetPipeline(_pointLightPipeline);
        cmd.SetBindGroup(mat->GetBindGroup(), 1);
        cmd.Draw(4, 0);
    }

    // Emissive
    cmd.SetPipeline(_emissivePipeline);
    cmd.SetBindGroup(_emissiveMaterial->GetBindGroup(), 1);
    cmd.Draw(4, 0);
}
