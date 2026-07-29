#include "BeStandardLightingPass.h"

#include "BeAssetRegistry.h"
#include "BeShaderLibrary.h"
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
    std::shared_ptr<BeTexture> irradianceCubemap,
    std::shared_ptr<BeTexture> prefilteredCubemap,
    std::shared_ptr<BeTexture> brdfLutTexture,
    std::shared_ptr<BeTexture> output
)
: _srm(srm)
, _gbufferInputs(std::move(gbufferInputs))
, _depthInput(std::move(depthInput))
, _irradianceCubemap(std::move(irradianceCubemap))
, _prefilteredCubemap(std::move(prefilteredCubemap))
, _brdfLutTexture(std::move(brdfLutTexture))
, _output(std::move(output)) {}

auto BeStandardLightingPass::Initialise() -> void {
    const auto directionalLightShader = BeShaderLibrary::GetShader("directional-light");
    const auto pointLightShader       = BeShaderLibrary::GetShader("point-light");
    const auto emissiveAddShader      = BeShaderLibrary::GetShader("emissive-add");

    constexpr SenBlendState additiveBlend = {
        .Enable = true,
        .SrcBlend = SenBlendFactor::One,  
        .DstBlend = SenBlendFactor::One,  
        .BlendOp = SenBlendOp::Add,
        .SrcBlendAlpha = SenBlendFactor::One, 
        .DstBlendAlpha = SenBlendFactor::One, 
        .BlendOpAlpha = SenBlendOp::Add,
    };
    const SenFormat outputFormat = _output->Format;

    _directionalLightScheme = directionalLightShader->GetMaterialScheme("main");
    _directionalLightPipeline = BePipelineBuilder::Start(*directionalLightShader)
        .SetBlend(additiveBlend)
        .SetColorFormats({ outputFormat })
        .Build()
    ;
    
    _pointLightScheme = pointLightShader->GetMaterialScheme("main");
    _pointLightPipeline = BePipelineBuilder::Start(*pointLightShader)
        .SetBlend(additiveBlend)
        .SetColorFormats({ outputFormat })
        .Build()
    ;

    const auto& emissiveAddScheme = emissiveAddShader->GetMaterialScheme("main");
    _emissiveMaterial = BeMaterial::Create(emissiveAddScheme, false);
    _emissiveMaterial->SetTexture("InputEmissive", _gbufferInputs[3]);
    _emissivePipeline = BePipelineBuilder::Start(*emissiveAddShader)
        .SetBlend(additiveBlend)
        .SetColorFormats({ outputFormat })
        .Build()
    ;

    const auto ambientShader = BeShaderLibrary::GetShader("ambient-ibl");
    const auto& ambientScheme = ambientShader->GetMaterialScheme("main");
    _ambientMaterial = BeMaterial::Create(ambientScheme, false);
    _ambientMaterial->SetTexture("Albedo_RGB", _gbufferInputs[0]);
    _ambientMaterial->SetTexture("WorldNormal_XYZ", _gbufferInputs[1]);
    _ambientMaterial->SetTexture("ORM_RGB", _gbufferInputs[2]);
    _ambientMaterial->SetTexture("Depth_Tex", _depthInput);
    if (_irradianceCubemap) {
        _ambientMaterial->SetTexture("IrradianceCubemap", _irradianceCubemap);
    }
    if (_prefilteredCubemap) {
        _ambientMaterial->SetTexture("PrefilteredCubemap", _prefilteredCubemap);
        _ambientMaterial->SetFloat1("MaxMipLevel", static_cast<float>(_prefilteredCubemap->Mips - 1));
    }
    if (_brdfLutTexture) {
        _ambientMaterial->SetTexture("BrdfLut", _brdfLutTexture);
    }
    _ambientPipeline = BePipelineBuilder::Start(*ambientShader)
        .SetBlend(additiveBlend)
        .SetColorFormats({ outputFormat })
        .Build()
    ;
}

auto BeStandardLightingPass::Render(SenCommandBuffer& cmd) -> void {
    const auto& srm = *_srm;
    const auto& sunLights = srm.GetSunLightEntries();
    const auto& pointLights = srm.GetPointLightEntries();

    cmd.SetBindGroup(srm.UniformMaterial.lock()->GetBindGroup(), 0);
    
    BePass pass(cmd);
    pass.UseTextures(_gbufferInputs);
    pass.UseTexture(_depthInput);
    for (const auto& sunLight : sunLights) {
        if (sunLight.CastsShadows) {
            pass.UseTexture(sunLight.ShadowMap.lock());
        }
    }
    for (const auto& pointLight : pointLights) {
        if (pointLight.CastsShadows) {
            pass.UseTexture(pointLight.ShadowMap.lock());
        }
    }
    if (_irradianceCubemap) {
        pass.UseTexture(_irradianceCubemap);
    }
    if (_prefilteredCubemap) {
        pass.UseTexture(_prefilteredCubemap);
    }
    if (_brdfLutTexture) {
        pass.UseTexture(_brdfLutTexture);
    }
    pass.AddColorTarget(_output, SenLoadOp::Clear);
    pass.SetViewport(_renderer->GetViewport());
    pass.Begin();

    // Directional lights
    for (size_t i = 0; i < sunLights.size(); ++i) {
        const auto& sunLight = sunLights[i];
        if (i >= _directionalLightMaterials.size()) {
            auto mat = BeMaterial::Create(_directionalLightScheme, true);
            mat->SetTexture("Depth", _depthInput);
            mat->SetTexture("Albedo_RGB", _gbufferInputs[0]);
            mat->SetTexture("WorldNormal_XYZ", _gbufferInputs[1]);
            mat->SetTexture("ORM_RGB", _gbufferInputs[2]);
            _directionalLightMaterials.push_back(std::move(mat));
        }
        const auto& mat = _directionalLightMaterials[i];
        mat->SetFloat1("HasShadowMap",   sunLight.CastsShadows ? 1.0f : 0.0f);
        mat->SetFloat3("Direction",      sunLight.Direction);
        mat->SetFloat3("Color",          sunLight.Color);
        mat->SetFloat1("Power",          sunLight.Power);
        mat->SetMatrix("ProjectionView", sunLight.ShadowViewProjection);
        mat->SetFloat1("TexelSize",      1.0f / sunLight.ShadowMapResolution);
        mat->SetFloat1("ShadowBias",     srm.Settings.Shadow.Bias);
        if (sunLight.CastsShadows) {
            mat->SetTexture("ShadowMap", sunLight.ShadowMap.lock());
        }
        cmd.SetPipeline(_directionalLightPipeline);
        cmd.SetBindGroup(mat->GetBindGroup(), 1);
        cmd.Draw(4, 0);
    }

    // Point lights
    for (const auto& pointLight : pointLights) {
        if (!_pointLightMaterials.contains(pointLight.Name)) {
            auto mat = BeMaterial::Create(_pointLightScheme, true);
            mat->SetTexture("Depth", _depthInput);
            mat->SetTexture("Albedo_RGB", _gbufferInputs[0]);
            mat->SetTexture("WorldNormal_XYZ", _gbufferInputs[1]);
            mat->SetTexture("ORM_RGB", _gbufferInputs[2]);
            _pointLightMaterials[pointLight.Name] = std::move(mat);
        }
        const auto& mat = _pointLightMaterials[pointLight.Name];
        mat->SetFloat3("Position",            pointLight.Position);
        mat->SetFloat1("Radius",              pointLight.Radius);
        mat->SetFloat3("Color",               pointLight.Color);
        mat->SetFloat1("Power",               pointLight.Power);
        mat->SetFloat1("HasShadowMap",        pointLight.CastsShadows ? 1.0f : 0.0f);
        mat->SetFloat1("ShadowMapResolution", static_cast<float>(pointLight.ShadowMapResolution));
        mat->SetFloat1("ShadowNearPlane",     pointLight.ShadowNearPlane);
        if (pointLight.CastsShadows) {
            mat->SetTexture("PointLightShadowMap", pointLight.ShadowMap.lock());
        }
        cmd.SetPipeline(_pointLightPipeline);
        cmd.SetBindGroup(mat->GetBindGroup(), 1);
        cmd.Draw(4, 0);
    }

    // Emissive
    cmd.SetPipeline(_emissivePipeline);
    cmd.SetBindGroup(_emissiveMaterial->GetBindGroup(), 1);
    cmd.Draw(4, 0);

    if (_ambientMaterial) {
        cmd.SetPipeline(_ambientPipeline);
        cmd.SetBindGroup(_ambientMaterial->GetBindGroup(), 1);
        cmd.Draw(4, 0);
    }

    pass.End();
}
