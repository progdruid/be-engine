#include "BeStandardEnvironmentBakePass.h"

#include <umbrellas/include-libassert.h>
#include <sen-rhi/SenBackend.h>

#include "BeAssetRegistry.h"
#include "BeShaderLibrary.h"
#include "BePass.h"
#include "BeMaterial.h"
#include "BePipelineBuilder.h"
#include "BeRenderer.h"
#include "BeShader.h"
#include "BeTexture.h"
#include "standard-render-machine/BeStandardRenderMachine.h"

BeStandardEnvironmentBakePass::BeStandardEnvironmentBakePass(
    BeStandardRenderMachine* srm,
    std::shared_ptr<BeTexture> equirect,
    std::shared_ptr<BeTexture> envCubemap,
    std::shared_ptr<BeTexture> irradianceCubemap,
    std::shared_ptr<BeTexture> prefilteredCubemap,
    std::shared_ptr<BeTexture> brdfLutTexture
) : _srm(srm), _equirect(std::move(equirect)), _envCubemap(std::move(envCubemap)),
    _irradianceCubemap(std::move(irradianceCubemap)), _prefilteredCubemap(std::move(prefilteredCubemap)),
    _brdfLutTexture(std::move(brdfLutTexture)) {}

auto BeStandardEnvironmentBakePass::Initialise(BeRenderer& renderer) -> void {

    const auto envShader = BeShaderLibrary::GetShader("environment-bake");
    be_assert(envShader, "BeStandardEnvironmentBakePass: environment-bake shader not found");

    const auto& envScheme = envShader->GetMaterialScheme("main");
    for (uint32_t face = 0; face < FaceCount; ++face) {
        const auto mat = BeMaterial::Create(envScheme);
        mat->SetFloat1("FaceIndex", static_cast<float>(face));
        mat->SetTexture("Equirect", _equirect);
        _envFaceMaterials[face] = mat;
    }
    _envPipeline = BePipelineBuilder::Start(*envShader).SetColorFormats({ _envCubemap->Format }).Build();

    const auto irradianceShader = BeShaderLibrary::GetShader("irradiance-bake");
    be_assert(irradianceShader, "BeStandardEnvironmentBakePass: irradiance-bake shader not found");

    const auto& irradianceScheme = irradianceShader->GetMaterialScheme("main");
    for (uint32_t face = 0; face < FaceCount; ++face) {
        const auto mat = BeMaterial::Create(irradianceScheme);
        mat->SetFloat1("FaceIndex", static_cast<float>(face));
        mat->SetFloat1("MaxSampleRadiance", _srm->Settings.IBL.MaxSampleRadiance);
        mat->SetTexture("EnvCubemap", _envCubemap);
        _irradianceFaceMaterials[face] = mat;
    }
    _irradiancePipeline = BePipelineBuilder::Start(*irradianceShader).SetColorFormats({ _irradianceCubemap->Format }).Build();

    const auto prefilterShader = BeShaderLibrary::GetShader("prefilter-bake");
    be_assert(prefilterShader, "BeStandardEnvironmentBakePass: prefilter-bake shader not found");

    const auto& prefilterScheme = prefilterShader->GetMaterialScheme("main");
    const uint32_t mipCount = _prefilteredCubemap->Mips;
    _prefilterFaceMaterials.resize(mipCount);
    for (uint32_t mip = 0; mip < mipCount; ++mip) {
        const float roughness = mipCount > 1 ? static_cast<float>(mip) / static_cast<float>(mipCount - 1) : 0.0f;
        for (uint32_t face = 0; face < FaceCount; ++face) {
            const auto mat = BeMaterial::Create(prefilterScheme);
            mat->SetFloat1("FaceIndex", static_cast<float>(face));
            mat->SetFloat1("Roughness", roughness);
            mat->SetFloat1("MaxSampleRadiance", _srm->Settings.IBL.MaxSampleRadiance);
            mat->SetTexture("EnvCubemap", _envCubemap);
            _prefilterFaceMaterials[mip][face] = mat;
        }
    }
    _prefilterPipeline = BePipelineBuilder::Start(*prefilterShader).SetColorFormats({ _prefilteredCubemap->Format }).Build();

    const auto brdfLutShader = BeShaderLibrary::GetShader("brdf-lut");
    be_assert(brdfLutShader, "BeStandardEnvironmentBakePass: brdf-lut shader not found");

    const auto& brdfLutScheme = brdfLutShader->GetMaterialScheme("main");
    _brdfLutMaterial = BeMaterial::Create(brdfLutScheme);
    _brdfLutPipeline = BePipelineBuilder::Start(*brdfLutShader).SetColorFormats({ _brdfLutTexture->Format }).Build();
}

auto BeStandardEnvironmentBakePass::Render(BeRenderer& renderer, SenCommandBuffer& cmd) -> void {
    cmd.SetPipeline(_envPipeline);
    for (uint32_t face = 0; face < FaceCount; ++face) {
        BePass pass(cmd);
        pass.UseTexture(_equirect);
        pass.AddColorTarget(_envCubemap, SenLoadOp::DontCare, {}, 0, static_cast<int8_t>(face));
        pass.SetViewport(_envCubemap->GetViewport());
        pass.Begin();
        cmd.SetBindGroup(_envFaceMaterials[face]->GetBindGroup(), 0);
        cmd.Draw(4, 0);
        pass.End();
    }

    cmd.SetPipeline(_irradiancePipeline);
    for (uint32_t face = 0; face < FaceCount; ++face) {
        BePass pass(cmd);
        pass.UseTexture(_envCubemap);
        pass.AddColorTarget(_irradianceCubemap, SenLoadOp::DontCare, {}, 0, static_cast<int8_t>(face));
        pass.SetViewport(_irradianceCubemap->GetViewport());
        pass.Begin();
        cmd.SetBindGroup(_irradianceFaceMaterials[face]->GetBindGroup(), 0);
        cmd.Draw(4, 0);
        pass.End();
    }

    cmd.SetPipeline(_prefilterPipeline);
    const uint32_t mipCount = _prefilteredCubemap->Mips;
    for (uint32_t mip = 0; mip < mipCount; ++mip) {
        for (uint32_t face = 0; face < FaceCount; ++face) {
            BePass pass(cmd);
            pass.UseTexture(_envCubemap);
            pass.AddColorTarget(_prefilteredCubemap, SenLoadOp::DontCare, {}, static_cast<uint8_t>(mip), static_cast<int8_t>(face));
            pass.SetViewport(_prefilteredCubemap->GetMipViewport(mip));
            pass.Begin();
            cmd.SetBindGroup(_prefilterFaceMaterials[mip][face]->GetBindGroup(), 0);
            cmd.Draw(4, 0);
            pass.End();
        }
    }

    cmd.SetPipeline(_brdfLutPipeline);
    {
        BePass pass(cmd);
        pass.AddColorTarget(_brdfLutTexture, SenLoadOp::DontCare, {});
        pass.SetViewport(_brdfLutTexture->GetViewport());
        pass.Begin();
        cmd.SetBindGroup(_brdfLutMaterial->GetBindGroup(), 0);
        cmd.Draw(4, 0);
        pass.End();
    }
}
