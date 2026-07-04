#include "BeStandardBloomPass.h"

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

BeStandardBloomPass::BeStandardBloomPass(
    BeStandardRenderMachine* srm,
    std::shared_ptr<BeTexture> inputHDR,
    std::shared_ptr<BeTexture> bloomTexture,
    std::shared_ptr<BeTexture> output,
    std::shared_ptr<BeTexture> dirtTexture,
    const uint32_t mipCount
) : _srm(srm), _inputHDR(std::move(inputHDR)), _bloomTexture(std::move(bloomTexture)),
    _output(std::move(output)), _dirtTexture(std::move(dirtTexture)), _mipCount(mipCount) {}

auto BeStandardBloomPass::Initialise() -> void {
    const SenFormat mipFormat = _bloomTexture->Format;

    const auto  brightShader = BeShaderLibrary::GetShader("bloom-bright");
    be_assert(  brightShader, "BeStandardBloomPass: bloom-bright shader not found");
    const auto& brightScheme = brightShader->GetMaterialScheme("main");
    _brightMaterial = BeMaterial::Create(brightScheme, false);
    _brightMaterial->SetTexture("HDRInput", _inputHDR);
    _brightPipeline = BePipelineBuilder::Start(*brightShader).SetColorFormats({ mipFormat }).Build();

    // Downsample mipTarget i (1..mipCount-1) reads source mip i-1 of the same texture.
    const auto  downsampleShader = BeShaderLibrary::GetShader("bloom-downsample");
    be_assert(  downsampleShader, "BeStandardBloomPass: bloom-downsample shader not found");
    const auto& downsampleScheme = downsampleShader->GetMaterialScheme("main");
    _downsampleMaterials.resize(_mipCount);
    for (uint32_t mipTarget = 1; mipTarget < _mipCount; ++mipTarget) {
        const auto& source = _bloomTexture->GetMipViewport(mipTarget - 1);
        const auto  mat    = BeMaterial::Create(downsampleScheme, false);
        mat->SetFloat2("TexelSize", glm::vec2(1.0f / source.Width, 1.0f / source.Height));
        mat->SetFloat1("UseKaris", mipTarget == 1 ? 1.0f : 0.0f);
        mat->SetTexture("BloomMipInput", _bloomTexture, mipTarget - 1);
        _downsampleMaterials[mipTarget] = mat;
    }

    _downsamplePipeline = BePipelineBuilder::Start(*downsampleShader).SetColorFormats({ mipFormat }).Build();

    // Upsample mipTarget i (0..mipCount-2) reads source mip i+1 of the same texture.
    const auto  upsampleShader = BeShaderLibrary::GetShader("bloom-upsample");
    be_assert(  upsampleShader, "BeStandardBloomPass: bloom-upsample shader not found");
    const auto& upsampleScheme = upsampleShader->GetMaterialScheme("main");
    _upsampleMaterials.resize(_mipCount);
    for (uint32_t mipTarget = 0; mipTarget < _mipCount - 1; ++mipTarget) {
        const auto& source = _bloomTexture->GetMipViewport(mipTarget + 1);
        const auto  mat = BeMaterial::Create(upsampleScheme, false);
        mat->SetFloat2("TexelSize", glm::vec2(1.0f / source.Width, 1.0f / source.Height));
        mat->SetFloat1("Radius", 1.0f);
        mat->SetTexture("BloomMipInput", _bloomTexture, mipTarget + 1);
        _upsampleMaterials[mipTarget] = mat;
    }
    _upsamplePipeline = BePipelineBuilder::Start(*upsampleShader)
        .SetBlend({
            .Enable = true,
            .SrcBlend = SenBlendFactor::One, .DstBlend = SenBlendFactor::One, .BlendOp = SenBlendOp::Add,
            .SrcBlendAlpha = SenBlendFactor::Zero, .DstBlendAlpha = SenBlendFactor::One, .BlendOpAlpha = SenBlendOp::Add,
        })
        .SetColorFormats({ mipFormat }).Build();

    const auto addShader = BeShaderLibrary::GetShader("bloom-add");
    be_assert( addShader, "BeStandardBloomPass: bloom-add shader not found");
    const auto addScheme = addShader->GetMaterialScheme("main");
    _addMaterial = BeMaterial::Create(addScheme, false);
    _addMaterial->SetTexture("HDRInput", _inputHDR);
    _addMaterial->SetTexture("BloomInput", _bloomTexture);
    _addMaterial->SetTexture("DirtTexture", _dirtTexture);
    _addPipeline = BePipelineBuilder::Start(*addShader).SetColorFormats({ mipFormat }).Build();
}

auto BeStandardBloomPass::Render(SenCommandBuffer& cmd) -> void {
    cmd.SetBindGroup(_srm->UniformMaterial.lock()->GetBindGroup(), 0);
    RenderBrightPass(cmd);
    RenderDownsamplePasses(cmd);
    RenderUpsamplePasses(cmd);
    RenderAddPass(cmd);
}

auto BeStandardBloomPass::RenderBrightPass(SenCommandBuffer& cmd) const -> void {
    BePass pass(cmd);
    pass.UseTexture(_inputHDR);
    pass.UseMaterial(*_brightMaterial);
    pass.AddColorTarget(_bloomTexture, SenLoadOp::DontCare, {}, 0);
    pass.SetViewport(_bloomTexture->GetMipViewport(0));
    pass.Begin();
    cmd.SetPipeline(_brightPipeline);
    cmd.SetBindGroup(_brightMaterial->GetBindGroup(), 1);
    cmd.Draw(4, 0);
    pass.End();
}

auto BeStandardBloomPass::RenderDownsamplePasses(SenCommandBuffer& cmd) const -> void {
    cmd.SetPipeline(_downsamplePipeline);
    for (uint32_t mipTarget = 1; mipTarget < _mipCount; ++mipTarget) {
        BePass pass(cmd);
        pass.UseTextureMip(_bloomTexture, mipTarget - 1);
        pass.AddColorTarget(_bloomTexture, SenLoadOp::DontCare, {}, mipTarget);
        pass.SetViewport(_bloomTexture->GetMipViewport(mipTarget));
        pass.Begin();
        cmd.SetBindGroup(_downsampleMaterials[mipTarget]->GetBindGroup(), 1);
        cmd.Draw(4, 0);
        pass.End();
    }
}

auto BeStandardBloomPass::RenderUpsamplePasses(SenCommandBuffer& cmd) const -> void {
    cmd.SetPipeline(_upsamplePipeline);
    for (int32_t mipTarget = _mipCount - 2; mipTarget >= 0; --mipTarget) {
        BePass pass(cmd);
        pass.UseTextureMip(_bloomTexture, mipTarget + 1);
        pass.AddColorTarget(_bloomTexture, SenLoadOp::Load, {}, mipTarget);
        pass.SetViewport(_bloomTexture->GetMipViewport(mipTarget));
        pass.Begin();
        cmd.SetBindGroup(_upsampleMaterials[mipTarget]->GetBindGroup(), 1);
        cmd.Draw(4, 0);
        pass.End();
    }
}

auto BeStandardBloomPass::RenderAddPass(SenCommandBuffer& cmd) const -> void {
    BePass pass(cmd);
    pass.UseMaterial(*_addMaterial);
    pass.AddColorTarget(_output, SenLoadOp::Load);
    pass.SetViewport(_renderer->GetViewport());
    pass.Begin();
    cmd.SetPipeline(_addPipeline);
    cmd.SetBindGroup(_addMaterial->GetBindGroup(), 1);
    cmd.Draw(4, 0);
    pass.End();
}
