#include "BeStandardLightingPass.h"

#include <algorithm>
#include <span>
#include <umbrellas/include-libassert.h>

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

auto BeStandardLightingPass::Initialise(BeRenderer& renderer) -> void {
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

    const auto batchedShader = BeShaderLibrary::GetShader("batched-lights");
    _batchedScheme = batchedShader->GetMaterialScheme("main");
    _batchedCapacity = _batchedScheme.PropertyArrayLengths.at("LightPositionRadius");
    be_assert(
        _batchedCapacity == _batchedScheme.PropertyArrayLengths.at("LightColorPower"),
        "batched-lights: LightPositionRadius and LightColorPower must have the same array length"
    );
    _batchedMaterial = BeMaterial::Create(_batchedScheme);
    _batchedMaterial->SetTexture("Depth", _depthInput);
    _batchedMaterial->SetTexture("Albedo_RGB", _gbufferInputs[0]);
    _batchedMaterial->SetTexture("WorldNormal_XYZ", _gbufferInputs[1]);
    _batchedMaterial->SetTexture("ORM_RGB", _gbufferInputs[2]);
    _batchedPipeline = BePipelineBuilder::Start(*batchedShader)
        .SetBlend(additiveBlend)
        .SetColorFormats({ outputFormat })
        .Build()
    ;

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
    _emissiveMaterial = BeMaterial::Create(emissiveAddScheme);
    _emissiveMaterial->SetTexture("InputEmissive", _gbufferInputs[3]);
    _emissivePipeline = BePipelineBuilder::Start(*emissiveAddShader)
        .SetBlend(additiveBlend)
        .SetColorFormats({ outputFormat })
        .Build()
    ;

    const auto ambientShader = BeShaderLibrary::GetShader("ambient-ibl");
    const auto& ambientScheme = ambientShader->GetMaterialScheme("main");
    _ambientMaterial = BeMaterial::Create(ambientScheme);
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

auto BeStandardLightingPass::Render(BeRenderer& renderer, SenCommandBuffer& cmd) -> void {
    const auto& srm = *_srm;
    const auto& sunLights = srm.GetSunLightEntries();
    const auto& pointLights = srm.GetPointLightEntries();

    cmd.SetBindGroup(srm.UniformMaterial->GetBindGroup(), 0);
    
    const auto directionalShadowArray = srm.GetDirectionalShadowArray();
    const auto pointShadowArray = srm.GetPointShadowArray();

    BePass pass(cmd);
    pass.UseTextures(_gbufferInputs);
    pass.UseTexture(_depthInput);
    if (directionalShadowArray) { pass.UseTexture(directionalShadowArray); }
    if (pointShadowArray)       { pass.UseTexture(pointShadowArray); }
    if (_irradianceCubemap)     { pass.UseTexture(_irradianceCubemap); }
    if (_prefilteredCubemap)    { pass.UseTexture(_prefilteredCubemap); }
    if (_brdfLutTexture)        { pass.UseTexture(_brdfLutTexture); }
    pass.AddColorTarget(_output, SenLoadOp::Clear);
    pass.SetViewport(renderer.GetViewport());
    pass.Begin();

    // Unshadowed lights, batched into one looping draw per capacity-sized chunk
    _batchedPositionRadius.clear();
    _batchedColorPower.clear();
    for (const auto& sunLight : sunLights) {
        if (sunLight.CastsShadows) {
            continue;
        }
        _batchedPositionRadius.emplace_back(sunLight.Direction, -1.0f);
        _batchedColorPower.emplace_back(sunLight.Color, sunLight.Power);
    }
    for (const auto& pointLight : pointLights) {
        if (pointLight.CastsShadows) {
            continue;
        }
        _batchedPositionRadius.emplace_back(pointLight.Position, pointLight.Radius);
        _batchedColorPower.emplace_back(pointLight.Color, pointLight.Power);
    }

    const auto totalBatched = _batchedPositionRadius.size();
    for (size_t first = 0; first < totalBatched; first += _batchedCapacity) {
        const size_t chunkSize = std::min<size_t>(_batchedCapacity, totalBatched - first);
        const auto positionRadiusChunk = std::span(_batchedPositionRadius).subspan(first, chunkSize);
        const auto colorPowerChunk = std::span(_batchedColorPower).subspan(first, chunkSize);

        _batchedMaterial->SetFloat4Array("LightPositionRadius", positionRadiusChunk);
        _batchedMaterial->SetFloat4Array("LightColorPower", colorPowerChunk);
        _batchedMaterial->SetFloat1("LightCount", static_cast<float>(chunkSize));
        cmd.SetPipeline(_batchedPipeline);
        cmd.SetBindGroup(_batchedMaterial->GetBindGroup(), 1);
        cmd.Draw(4, 0);
    }

    // Shadowed directional lights
    const float directionalTexelSize = 1.0f / srm.Settings.Shadow.DirectionalResolution;
    size_t shadowedSunIndex = 0;
    for (const auto& sunLight : sunLights) {
        if (!sunLight.CastsShadows) {
            continue;
        }
        const size_t i = shadowedSunIndex++;
        if (i >= srm.Settings.Shadow.MaxDirectional) {
            break;
        }
        if (i >= _directionalLightMaterials.size()) {
            auto mat = BeMaterial::Create(_directionalLightScheme);
            mat->SetTexture("Depth", _depthInput);
            mat->SetTexture("Albedo_RGB", _gbufferInputs[0]);
            mat->SetTexture("WorldNormal_XYZ", _gbufferInputs[1]);
            mat->SetTexture("ORM_RGB", _gbufferInputs[2]);
            _directionalLightMaterials.push_back(std::move(mat));
        }
        const auto& mat = _directionalLightMaterials[i];
        mat->SetFloat1("HasShadowMap", 1.0f);
        mat->SetFloat3("Direction", sunLight.Direction);
        mat->SetFloat3("Color", sunLight.Color);
        mat->SetFloat1("Power", sunLight.Power);
        mat->SetMatrix("ProjectionView", sunLight.ShadowViewProjection);
        mat->SetFloat1("TexelSize", directionalTexelSize);
        mat->SetFloat1("ShadowBias", srm.Settings.Shadow.Bias);
        mat->SetFloat1("ShadowSlice", static_cast<float>(i));
        mat->SetTexture("ShadowMap", directionalShadowArray);
        cmd.SetPipeline(_directionalLightPipeline);
        cmd.SetBindGroup(mat->GetBindGroup(), 1);
        cmd.Draw(4, 0);
    }

    // Shadowed point lights=
    size_t shadowedPointIndex = 0;
    for (const auto& pointLight : pointLights) {
        if (!pointLight.CastsShadows) {
            continue;
        }
        const size_t i = shadowedPointIndex++;
        if (i >= srm.Settings.Shadow.MaxPoint) {
            break;
        }
        if (!_pointLightMaterials.contains(pointLight.Name)) {
            auto mat = BeMaterial::Create(_pointLightScheme);
            mat->SetTexture("Depth", _depthInput);
            mat->SetTexture("Albedo_RGB", _gbufferInputs[0]);
            mat->SetTexture("WorldNormal_XYZ", _gbufferInputs[1]);
            mat->SetTexture("ORM_RGB", _gbufferInputs[2]);
            _pointLightMaterials[pointLight.Name] = std::move(mat);
        }
        const auto& mat = _pointLightMaterials[pointLight.Name];
        mat->SetFloat3("Position", pointLight.Position);
        mat->SetFloat1("Radius", pointLight.Radius);
        mat->SetFloat3("Color", pointLight.Color);
        mat->SetFloat1("Power", pointLight.Power);
        mat->SetFloat1("HasShadowMap", 1.0f);
        mat->SetFloat1("ShadowMapResolution", static_cast<float>(srm.Settings.Shadow.PointResolution));
        mat->SetFloat1("ShadowNearPlane", pointLight.ShadowNearPlane);
        mat->SetFloat1("ShadowSlice", static_cast<float>(i));
        mat->SetTexture("PointLightShadowMap", pointShadowArray);
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
