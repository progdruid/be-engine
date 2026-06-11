#include "BeStandardLightingPass.h"

#include <scope_guard/scope_guard.hpp>
#include <sen-rhi/SenBackend.h>

#include "BeAssetRegistry.h"
#include "BePass.h"
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
    _directionalLightMaterial->SetTexture("Depth", _depthInput);
    _directionalLightMaterial->SetTexture("Albedo_RGB", _gbufferInputs[0]);
    _directionalLightMaterial->SetTexture("WorldNormal_XYZ", _gbufferInputs[1]);
    _directionalLightMaterial->SetTexture("ORM_RGB", _gbufferInputs[2]);

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

    const auto& sunLight    = srm.GetSunLightEntries()[0];
    const auto& pointLights = srm.GetPointLightEntries();

    BePass pass;
    pass.UseTextures(_gbufferInputs);
    pass.UseTexture(_depthInput);
    if (const auto shadowMap = sunLight.ShadowMap.lock()) {
        pass.UseTexture(shadowMap);
    }
    for (const auto& pointLight : pointLights) {
        if (const auto shadowMap = pointLight.ShadowMap.lock()) {
            pass.UseTexture(shadowMap);
        }
    }
    pass.AddColorTarget(_output, SenLoadOp::Clear);
    pass.SetViewport(_renderer->GetViewport());

    cmd.SetBindGroup(srm.UniformMaterial.lock()->GetBindGroup(), 0);

    pass.Begin();
    SCOPE_EXIT { pass.End(); };

    // Directional light
    _directionalLightMaterial->SetFloat("HasShadowMap",  sunLight.CastsShadows ? 1.0f : 0.0f);
    _directionalLightMaterial->SetFloat3("Direction",    sunLight.Direction);
    _directionalLightMaterial->SetFloat3("Color",        sunLight.Color);
    _directionalLightMaterial->SetFloat("Power",         sunLight.Power);
    _directionalLightMaterial->SetMatrix("ProjectionView", sunLight.ShadowViewProjection);
    _directionalLightMaterial->SetFloat("TexelSize",     1.0f / sunLight.ShadowMapResolution);
    if (sunLight.CastsShadows)
        _directionalLightMaterial->SetTexture("ShadowMap", sunLight.ShadowMap.lock());
    cmd.SetPipeline(_directionalLightPipeline);
    cmd.SetBindGroup(_directionalLightMaterial->GetBindGroup(), 1);
    cmd.Draw(4, 0);

    // Point lights
    for (const auto& pointLight : pointLights) {
        if (!_pointLightMaterials.contains(pointLight.Name)) {
            auto mat = BeMaterial::Create("point-light-material", true);
            mat->SetTexture("Depth", _depthInput);
            mat->SetTexture("Albedo_RGB", _gbufferInputs[0]);
            mat->SetTexture("WorldNormal_XYZ", _gbufferInputs[1]);
            mat->SetTexture("ORM_RGB", _gbufferInputs[2]);
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
        if (pointLight.CastsShadows)
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
