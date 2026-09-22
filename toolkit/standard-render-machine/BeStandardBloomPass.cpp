#include "BeStandardBloomPass.h"

#include <include-libassert.h>
#include <Sen.h>

#include "BeAssetRegistry.h"
#include "BeShaderLibrary.h"
#include "BePass.h"
#include "BeMaterial.h"
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

auto BeStandardBloomPass::Initialise(BeRenderer& renderer) -> void {
    _brightShader = BeShaderLibrary::GetShader("bloom-bright");
    be_assert(  _brightShader, "BeStandardBloomPass: bloom-bright shader not found");
    const auto& brightScheme = BeShaderLibrary::GetShaderScheme(*_brightShader, "main");
    _brightMaterial = BeMaterial::Create(brightScheme);
    _brightMaterial->SetTexture("HDRInput", _inputHDR);
    _brightState = BeDrawState::Create(*_brightShader).Build();

    // Downsample mipTarget i (1..mipCount-1) reads source mip i-1 of the same texture.
    _downsampleShader = BeShaderLibrary::GetShader("bloom-downsample");
    be_assert(  _downsampleShader, "BeStandardBloomPass: bloom-downsample shader not found");
    const auto& downsampleScheme = BeShaderLibrary::GetShaderScheme(*_downsampleShader, "main");
    _downsampleMaterials.resize(_mipCount);
    for (uint32_t mipTarget = 1; mipTarget < _mipCount; ++mipTarget) {
        const auto& source = _bloomTexture->GetMipViewport(mipTarget - 1);
        const auto  mat    = BeMaterial::Create(downsampleScheme);
        mat->SetFloat2("TexelSize", glm::vec2(1.0f / source.Width, 1.0f / source.Height));
        mat->SetFloat1("UseKaris", mipTarget == 1 ? 1.0f : 0.0f);
        mat->SetTexture("BloomMipInput", _bloomTexture, mipTarget - 1);
        _downsampleMaterials[mipTarget] = mat;
    }

    _downsampleState = BeDrawState::Create(*_downsampleShader).Build();

    // Upsample mipTarget i (0..mipCount-2) reads source mip i+1 of the same texture.
    _upsampleShader = BeShaderLibrary::GetShader("bloom-upsample");
    be_assert(  _upsampleShader, "BeStandardBloomPass: bloom-upsample shader not found");
    const auto& upsampleScheme = BeShaderLibrary::GetShaderScheme(*_upsampleShader, "main");
    _upsampleMaterials.resize(_mipCount);
    for (uint32_t mipTarget = 0; mipTarget < _mipCount - 1; ++mipTarget) {
        const auto& source = _bloomTexture->GetMipViewport(mipTarget + 1);
        const auto  mat = BeMaterial::Create(upsampleScheme);
        mat->SetFloat2("TexelSize", glm::vec2(1.0f / source.Width, 1.0f / source.Height));
        mat->SetTexture("BloomMipInput", _bloomTexture, mipTarget + 1);
        _upsampleMaterials[mipTarget] = mat;
    }
    _upsampleState = BeDrawState::Create(*_upsampleShader)
        .SetBlend({
            .Enable = true,
            .SrcBlend = SenBlendFactor::One, .DstBlend = SenBlendFactor::One, .BlendOp = SenBlendOp::Add,
            .SrcBlendAlpha = SenBlendFactor::Zero, .DstBlendAlpha = SenBlendFactor::One, .BlendOpAlpha = SenBlendOp::Add,
        })
        .Build();

    _addShader = BeShaderLibrary::GetShader("bloom-add");
    be_assert( _addShader, "BeStandardBloomPass: bloom-add shader not found");
    const auto addScheme = BeShaderLibrary::GetShaderScheme(*_addShader, "main");
    _addMaterial = BeMaterial::Create(addScheme);
    _addMaterial->SetTexture("HDRInput", _inputHDR);
    _addMaterial->SetTexture("BloomInput", _bloomTexture);
    _addMaterial->SetTexture("DirtTexture", _dirtTexture);
    _addState = BeDrawState::Create(*_addShader).Build();
}

auto BeStandardBloomPass::Render(BeRenderer& renderer, SenCommandList cmd) -> void {
    const auto& settings = _srm->Settings.Bloom;
    _brightMaterial->SetFloat1("Threshold", settings.Threshold);
    _brightMaterial->SetFloat1("Knee", settings.Knee);
    _brightMaterial->SetFloat1("Intensity", settings.Intensity);
    _brightMaterial->SetFloat1("Clamp", settings.Clamp);
    for (uint32_t mipTarget = 0; mipTarget < _mipCount - 1; ++mipTarget) {
        _upsampleMaterials[mipTarget]->SetFloat1("Radius", settings.UpsampleRadius);
    }

    RenderBrightPass(cmd);
    RenderDownsamplePasses(cmd);
    RenderUpsamplePasses(cmd);
    RenderAddPass(renderer, cmd);
}

auto BeStandardBloomPass::RenderBrightPass(SenCommandList cmd) const -> void {
    BePass pass(cmd);
    pass.UseTexture(_inputHDR);
    pass.UseMaterial(*_brightMaterial);
    pass.AddColorTarget(_bloomTexture, SenLoadOp::DontCare, {}, 0);
    pass.SetViewport(_bloomTexture->GetViewport());
    pass.Begin();
    pass.SetState(_brightState);
    pass.Bind("frame", *_srm->UniformMaterial);
    pass.Bind("main", *_brightMaterial);
    pass.Draw(4);
    pass.End();
}

auto BeStandardBloomPass::RenderDownsamplePasses(SenCommandList cmd) const -> void {
    for (uint32_t mipTarget = 1; mipTarget < _mipCount; ++mipTarget) {
        BePass pass(cmd);
        pass.UseTextureMip(_bloomTexture, mipTarget - 1);
        pass.AddColorTarget(_bloomTexture, SenLoadOp::DontCare, {}, mipTarget);
        pass.SetViewport(_bloomTexture->GetMipViewport(mipTarget));
        pass.Begin();
        pass.SetState(_downsampleState);
        pass.Bind("frame", *_srm->UniformMaterial);
        pass.Bind("main", *_downsampleMaterials[mipTarget]);
        pass.Draw(4);
        pass.End();
    }
}

auto BeStandardBloomPass::RenderUpsamplePasses(SenCommandList cmd) const -> void {
    for (int32_t mipTarget = _mipCount - 2; mipTarget >= 0; --mipTarget) {
        BePass pass(cmd);
        pass.UseTextureMip(_bloomTexture, mipTarget + 1);
        pass.AddColorTarget(_bloomTexture, SenLoadOp::Load, {}, mipTarget);
        pass.SetViewport(_bloomTexture->GetMipViewport(mipTarget));
        pass.Begin();
        pass.SetState(_upsampleState);
        pass.Bind("frame", *_srm->UniformMaterial);
        pass.Bind("main", *_upsampleMaterials[mipTarget]);
        pass.Draw(4);
        pass.End();
    }
}

auto BeStandardBloomPass::RenderAddPass(BeRenderer& renderer, SenCommandList cmd) const -> void {
    BePass pass(cmd);
    pass.UseMaterial(*_addMaterial);
    pass.AddColorTarget(_output, SenLoadOp::Load);
    pass.SetViewport(_output->GetViewport());
    pass.Begin();
    pass.SetState(_addState);
    pass.Bind("frame", *_srm->UniformMaterial);
    pass.Bind("main", *_addMaterial);
    pass.Draw(4);
    pass.End();
}
