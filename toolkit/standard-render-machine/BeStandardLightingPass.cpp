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
    const auto dirShadowBatchShader   = BeShaderLibrary::GetShader("batched-directional-light-shadowed");
    const auto pointShadowBatchShader = BeShaderLibrary::GetShader("batched-point-light-shadowed");
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

    _dirShadowBatchScheme = dirShadowBatchShader->GetMaterialScheme("main");
    _dirShadowBatchCapacity = _dirShadowBatchScheme.PropertyArrayLengths.at("LightDirection");
    _dirShadowBatchMaterial = BeMaterial::Create(_dirShadowBatchScheme);
    _dirShadowBatchMaterial->SetTexture("Depth", _depthInput);
    _dirShadowBatchMaterial->SetTexture("Albedo_RGB", _gbufferInputs[0]);
    _dirShadowBatchMaterial->SetTexture("WorldNormal_XYZ", _gbufferInputs[1]);
    _dirShadowBatchMaterial->SetTexture("ORM_RGB", _gbufferInputs[2]);
    _dirShadowBatchPipeline = BePipelineBuilder::Start(*dirShadowBatchShader)
        .SetBlend(additiveBlend)
        .SetColorFormats({ outputFormat })
        .Build()
    ;

    _pointShadowBatchScheme = pointShadowBatchShader->GetMaterialScheme("main");
    _pointShadowBatchCapacity = _pointShadowBatchScheme.PropertyArrayLengths.at("LightPositionRadius");
    _pointShadowBatchMaterial = BeMaterial::Create(_pointShadowBatchScheme);
    _pointShadowBatchMaterial->SetTexture("Depth", _depthInput);
    _pointShadowBatchMaterial->SetTexture("Albedo_RGB", _gbufferInputs[0]);
    _pointShadowBatchMaterial->SetTexture("WorldNormal_XYZ", _gbufferInputs[1]);
    _pointShadowBatchMaterial->SetTexture("ORM_RGB", _gbufferInputs[2]);
    _pointShadowBatchPipeline = BePipelineBuilder::Start(*pointShadowBatchShader)
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

    // Unshadowed lights, batched into capacity-sized chunks. 
    // A shadow-caster with no shadow array present (e.g. no shadow pass was added) 
    //  falls back to being rendered here, unshadowed.
    static std::vector<glm::vec4> unshadowedPositionRadius;
    static std::vector<glm::vec4> unshadowedColorPower;
    unshadowedPositionRadius.clear();
    unshadowedColorPower.clear();
    for (const auto& sunLight : sunLights) {
        if (sunLight.CastsShadows && directionalShadowArray) {
            continue;
        }
        unshadowedPositionRadius.emplace_back(sunLight.Direction, -1.0f);
        unshadowedColorPower.emplace_back(sunLight.Color, sunLight.Power);
    }
    for (const auto& pointLight : pointLights) {
        if (pointLight.CastsShadows && pointShadowArray) {
            continue;
        }
        unshadowedPositionRadius.emplace_back(pointLight.Position, pointLight.Radius);
        unshadowedColorPower.emplace_back(pointLight.Color, pointLight.Power);
    }
    for (size_t first = 0; first < unshadowedPositionRadius.size(); first += _batchedCapacity) {
        const size_t chunkSize = std::min<size_t>(_batchedCapacity, unshadowedPositionRadius.size() - first);
        _batchedMaterial->SetFloat4Array("LightPositionRadius", std::span(unshadowedPositionRadius).subspan(first, chunkSize));
        _batchedMaterial->SetFloat4Array("LightColorPower", std::span(unshadowedColorPower).subspan(first, chunkSize));
        _batchedMaterial->SetFloat1("LightCount", static_cast<float>(chunkSize));
        cmd.SetPipeline(_batchedPipeline);
        cmd.SetBindGroup(_batchedMaterial->GetBindGroup(), 1);
        cmd.Draw(4, 0);
    }

    // Shadowed directional lights, batched. Slice = shadowed-iteration order (matches the shadow pass).
    if (directionalShadowArray) {
        static std::vector<glm::vec4> directionSlice;
        static std::vector<glm::vec4> colorPower;
        static std::vector<glm::mat4> projectionViews;
        directionSlice.clear();
        colorPower.clear();
        projectionViews.clear();
        for (const auto& sunLight : sunLights) {
            if (!sunLight.CastsShadows) {
                continue;
            }
            if (directionSlice.size() >= srm.Settings.Shadow.MaxDirectional) {
                break;
            }
            directionSlice.emplace_back(sunLight.Direction, static_cast<float>(directionSlice.size()));
            colorPower.emplace_back(sunLight.Color, sunLight.Power);
            projectionViews.push_back(sunLight.ShadowViewProjection);
        }
        if (!directionSlice.empty()) {
            _dirShadowBatchMaterial->SetFloat1("TexelSize", 1.0f / srm.Settings.Shadow.DirectionalResolution);
            _dirShadowBatchMaterial->SetFloat1("ShadowBias", srm.Settings.Shadow.Bias);
            _dirShadowBatchMaterial->SetTexture("ShadowMap", directionalShadowArray);
            for (size_t first = 0; first < directionSlice.size(); first += _dirShadowBatchCapacity) {
                const size_t chunkSize = std::min<size_t>(_dirShadowBatchCapacity, directionSlice.size() - first);
                _dirShadowBatchMaterial->SetFloat4Array("LightDirection", std::span(directionSlice).subspan(first, chunkSize));
                _dirShadowBatchMaterial->SetFloat4Array("LightColorPower", std::span(colorPower).subspan(first, chunkSize));
                _dirShadowBatchMaterial->SetMatrixArray("LightProjectionView", std::span(projectionViews).subspan(first, chunkSize));
                _dirShadowBatchMaterial->SetFloat1("LightCount", static_cast<float>(chunkSize));
                cmd.SetPipeline(_dirShadowBatchPipeline);
                cmd.SetBindGroup(_dirShadowBatchMaterial->GetBindGroup(), 1);
                cmd.Draw(4, 0);
            }
        }
    }

    // Shadowed point lights, batched. Cube slice = shadowed-iteration order (matches the shadow pass).
    if (pointShadowArray) {
        static std::vector<glm::vec4> positionRadius;
        static std::vector<glm::vec4> colorPower;
        static std::vector<glm::vec4> shadowParams;
        positionRadius.clear();
        colorPower.clear();
        shadowParams.clear();
        for (const auto& pointLight : pointLights) {
            if (!pointLight.CastsShadows) {
                continue;
            }
            if (positionRadius.size() >= srm.Settings.Shadow.MaxPoint) {
                break;
            }
            positionRadius.emplace_back(pointLight.Position, pointLight.Radius);
            colorPower.emplace_back(pointLight.Color, pointLight.Power);
            shadowParams.emplace_back(static_cast<float>(positionRadius.size() - 1), pointLight.ShadowNearPlane, 0.0f, 0.0f);
        }
        if (!positionRadius.empty()) {
            _pointShadowBatchMaterial->SetTexture("PointLightShadowMap", pointShadowArray);
            for (size_t first = 0; first < positionRadius.size(); first += _pointShadowBatchCapacity) {
                const size_t chunkSize = std::min<size_t>(_pointShadowBatchCapacity, positionRadius.size() - first);
                _pointShadowBatchMaterial->SetFloat4Array("LightPositionRadius", std::span(positionRadius).subspan(first, chunkSize));
                _pointShadowBatchMaterial->SetFloat4Array("LightColorPower", std::span(colorPower).subspan(first, chunkSize));
                _pointShadowBatchMaterial->SetFloat4Array("LightShadowParams", std::span(shadowParams).subspan(first, chunkSize));
                _pointShadowBatchMaterial->SetFloat1("LightCount", static_cast<float>(chunkSize));
                cmd.SetPipeline(_pointShadowBatchPipeline);
                cmd.SetBindGroup(_pointShadowBatchMaterial->GetBindGroup(), 1);
                cmd.Draw(4, 0);
            }
        }
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
